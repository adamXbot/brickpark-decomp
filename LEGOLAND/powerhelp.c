/* LEGOLAND — power subsystem, the load-shedding half.
 *
 * LEGOLAND/power.c holds the public entry points (FindObjectsPower,
 * AddObjectsPowerStats, RemoveObjectsPowerStats, GetObjRepairCost) and the
 * description of the five accumulators at 0x00832bcc..0x00832bdc.  This file
 * completes the subsystem with the four UNEXPORTED helpers those entry points
 * call, plus the salvage-value leaf that GetObjRepairCost is built from.
 *
 * THE SHEDDING MODEL
 * ------------------
 * The park runs one park-wide electricity pool.  There is no grid, no
 * adjacency, no wiring: an object is either served or blacked out, and the
 * only rule is the invariant
 *
 *       demand - unserved  <=  supply          (served load <= generation)
 *
 * Two mirror-image walkers restore that invariant whenever supply or demand
 * moves:
 *
 *   RecheckPoweredObjects (0x0045a060) runs after supply GROWS or demand
 *     SHRINKS.  It walks the render list looking for objects that are blacked
 *     out (flags & 0x100) and switches each one back on IF its load still fits
 *     under the remaining headroom.  Note it does not sort or prioritise, and
 *     it does not stop at the first object that does not fit — a big blacked-out
 *     ride is skipped over and a later small one gets the power instead.
 *
 *   ShedPoweredObjects (0x0045a0d0) runs after supply SHRINKS.  It walks the
 *     same list looking for objects that are still powered (no 0x100) and
 *     blacks them out one at a time until the invariant holds again.
 *
 * Both stop early the moment served load exactly meets supply — Recheck tests
 * for `==` (there is provably no headroom left, so no later object can fit),
 * Shed tests for `<=` (the invariant is satisfied, stop cutting).  Shedding
 * order is therefore RENDER-LIST order, not cost order and not priority order:
 * which of the player's rides go dark when a power station is demolished is
 * decided purely by where they sit in the map's object chain.
 *
 * Both walkers call FindObjectsPower afresh on every object rather than
 * caching a per-object load, so the name->power table lookup (a linear scan of
 * 65 entries doing a case-insensitive string compare) runs once per object per
 * walk.  That is the whole cost model; nothing is memoised.
 *
 * SIGN CONVENTION.  FindObjectsPower returns a SIGNED figure: positive for
 * generators, negative for consumers.  Both walkers immediately negate it, so
 * `load` here is a POSITIVE consumption figure.  ShedPoweredObjects guards with
 * `load > 0`, which skips both generators (load < 0) and off-grid objects
 * (load == 0) in one test.  RecheckPoweredObjects has NO such guard, because it
 * only ever looks at objects already carrying the 0x100 blacked-out bit and
 * only a consumer can have been given that bit.
 *
 * THE RENDER LIST IS A CELL CHAIN.  GetFirstRenderObject (0x0045a850) and
 * GetNextRenderObject (0x0045a8b0) both return a Cell*, not an object pointer:
 * the walkers read cell->flags at +0x0c and cell->obj at +0x00, then the
 * object's data record at obj+0x0c.  GetNextRenderObject reads a PACKED
 * next-cell COORDINATE from Cell +0x06/+0x07 (the field legoland.h calls
 * `pad6`: +0x06 is x, +0x07 is y), bounds-checks it against g_map->width /
 * height, and returns &g_map_rows[y][x].  So the "render list" is not a pointer
 * list at all — it is a chain threaded through the map itself, one byte-pair of
 * link per cell, terminated when the pair reads as zero.  That is why the
 * chain can never leave the map and why it costs no memory beyond the grid.
 *
 * PowerOn/PowerOff (0x0045a000 / 0x0045a030) are the only writers of the
 * blacked-out bit outside AddObjectsPowerStats.  They take (load, cell) and
 * keep the bit and the two unserved accumulators in lockstep — the bit on a
 * cell and the count in 0x832bdc can only ever disagree if something writes
 * flags behind their back.
 *
 * SALVAGE (0x00480db0) is straight-line depreciation with a divide-by-zero
 * guard: an ObjDesc whose lifetime byte (+0x2c) is zero is worth its full
 * purchase price (+0x26) forever, otherwise it is worth cost*remaining/life.
 * The division is SIGNED (cdq/idiv) even though the divisor is a zero-extended
 * unsigned byte, so a negative `age` would round toward zero.
 */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* An object's data record; +0xc4 is its LLIDB element (see power.c). */
typedef struct PowerData {
    unsigned char pad[0xc4];  /* +0x00..0xc3 */
    void*         elem;       /* +0xc4 */
} PowerData;

/* The map object wrapper a Cell points at; +0x0c is its data record. */
typedef struct PowerObj {
    unsigned char pad[0x0c];  /* +0x00..0x0b */
    PowerData*    data;       /* +0x0c */
} PowerObj;

/* Purchase price at +0x26 (signed 16-bit), full lifetime at +0x2c (byte). */
typedef struct ObjDesc {
    unsigned char pad[0x26];   /* +0x00..0x25 */
    short         cost;        /* +0x26 */
    unsigned char pad28[4];    /* +0x28..0x2b */
    unsigned char life;        /* +0x2c */
} ObjDesc;

/* ---------------------------------------------------------------- globals -- */

extern int g_power_supply;       /* 0x00832bd0  total generation */
extern int g_power_demand;       /* 0x00832bd4  total consumption */
extern int g_power_unserved;     /* 0x00832bd8  consumption currently blacked out */
extern int g_power_unserved_n;   /* 0x00832bdc  how many objects are blacked out */

extern int   FindObjectsPower(PowerData* d);     /* 0x00459fa0 */
extern Cell* GetFirstRenderObject(void);         /* 0x0045a850 */
extern Cell* GetNextRenderObject(Cell* c);       /* 0x0045a8b0 */
extern int   GetObjCost(ObjDesc* p);             /* 0x00480da0 */

void PowerOn(int load, Cell* c);
void PowerOff(int load, Cell* c);

/* -------------------------------------------------------------- functions -- */

/* Give this cell's object power back: clear the blacked-out bit and take its
 * load off the unserved books.  VC6 emits the clear as a 16-bit
 * 'and word ptr [eax+0xc], 0xfeff' — it does not narrow AND to the high byte
 * the way it narrows OR (see PowerOff). */
// FUNCTION: LEGOLAND 0x0045a000
void PowerOn(int load, Cell* c)
{
    c->flags &= ~0x100;
    g_power_unserved -= load;
    g_power_unserved_n--;
}

/* Black this cell's object out: set the bit and book its load as unserved.
 * Here VC6 DOES narrow the OR to 'or byte ptr [eax+0xd], 1'. */
// FUNCTION: LEGOLAND 0x0045a030
void PowerOff(int load, Cell* c)
{
    c->flags |= 0x100;
    g_power_unserved += load;
    g_power_unserved_n++;
}

/* Supply grew (or demand shrank): hand power back to blacked-out objects for
 * as long as there is headroom.  Stops the instant served load exactly meets
 * supply, since nothing further can possibly fit. */
// FUNCTION: LEGOLAND 0x0045a060
void RecheckPoweredObjects(void)
{
    Cell* c = GetFirstRenderObject();

    while (c != 0) {
        if (c->flags & 0x100) {
            int load = -FindObjectsPower(((PowerObj*)c->obj)->data);

            if (g_power_demand - g_power_unserved + load <= g_power_supply) {
                PowerOn(load, c);
                if (g_power_demand - g_power_unserved == g_power_supply)
                    break;
            }
        }
        c = GetNextRenderObject(c);
    }
}

/* Supply shrank: black out powered consumers, in render-list order, until the
 * served load fits under supply again.  The 'load > 0' test skips generators
 * and off-grid objects in one go. */
// FUNCTION: LEGOLAND 0x0045a0d0
void ShedPoweredObjects(void)
{
    Cell* c = GetFirstRenderObject();

    while (c != 0) {
        if (!(c->flags & 0x100)) {
            int load = -FindObjectsPower(((PowerObj*)c->obj)->data);

            if (load > 0) {
                PowerOff(load, c);
                if (g_power_demand - g_power_unserved <= g_power_supply)
                    break;
            }
        }
        c = GetNextRenderObject(c);
    }
}

/* Straight-line depreciation.  A zero lifetime means "never wears out", and is
 * also the divide-by-zero guard. */
// FUNCTION: LEGOLAND 0x00480db0
int GetObjSalvageValue(ObjDesc* p, int age)
{
    if (p->life == 0)
        return GetObjCost(p);

    return GetObjCost(p) * age / p->life;
}
