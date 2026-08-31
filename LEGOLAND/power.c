/* LEGOLAND — power / economy accounting.
 *
 * The park's electricity budget is a set of five global accumulators plus one
 * static name->power table.  Every placeable object carries an LLIDB name
 * ("Restaurant 2", "Small Power Station", ...); FindObjectsPower looks that
 * name up in the table and returns a signed "power" figure:
 *
 *     power > 0  -> the object GENERATES that many units (power stations)
 *     power < 0  -> the object CONSUMES abs(power) units
 *     power == 0 -> the object is not on the electrical grid at all
 *
 * Units are arbitrary game "power units": the two generators are
 * "Small Power Station" = +800 and "Crystal Power Station" = +2500, and
 * consumers range from -2 (a food cart / jail cell) to -400 (Castle Obj).
 *
 * FIVE ACCUMULATORS (0x00832bcc..0x00832bdc — a cluster of their own, NOT the
 * build-stat block at 0x00667ce0):
 *
 *   0x832bd0  supply      sum of every live generator's power
 *   0x832bd4  demand      sum of abs(power) over every consumer on the map
 *   0x832bd8  unserved    the part of `demand` that is currently blacked out
 *   0x832bdc  unserved_n  how many objects are blacked out
 *   0x832bcc  percent     spare-capacity readout, recomputed after every change
 *
 * The invariant the whole subsystem maintains is
 *
 *       demand - unserved  <=  supply
 *
 * i.e. the SERVED load never exceeds generation.  A newly placed consumer is
 * granted power only if it still holds after adding it; otherwise the object
 * itself is marked blacked out and its load is booked into `unserved`.  There
 * is no grid/adjacency model at all — power is a single park-wide pool, and an
 * object's distance from a power station is irrelevant.
 *
 *   percent = (supply > demand) ? 100 - demand*100/supply : 0
 *
 * so 0x832bcc is the percentage of generation still spare; it pins to 0 the
 * moment total demand reaches total supply, even while some objects still run.
 *
 * CELL FLAG BITS used here (Cell.flags, +0x0c):
 *   0x0100  this cell's object is blacked out (no power)
 *   0x0200  this cell's object is switched off / disconnected — set by the
 *           "turn off" path at 0x00463517, cleared at 0x0049b0f3.  Its supply
 *           was already withdrawn, so RemoveObjectsPowerStats must not
 *           withdraw it a second time.
 *
 * Two internal (non-exported) helpers do the load shedding / restoring:
 *   0x0045a000  PowerOn(load, cell) : flags &= ~0x100; unserved -= load; n--
 *   0x0045a030  PowerOff(load, cell): flags |=  0x100; unserved += load; n++
 *   0x0045a060  RecheckPoweredObjects: walk the render list, and for every
 *               blacked-out object try to switch it back on while it fits
 *   0x0045a0d0  ShedPoweredObjects: walk the render list and black out powered
 *               objects until the invariant holds again
 * Both walkers stop early once served load exactly equals / fits supply, so
 * shedding order is render-list order, not cost or priority order.
 *
 * REPAIR/SALVAGE (0x00480da0..0x00480de0) is straight-line depreciation:
 *   ObjDesc.cost (+0x26, short) is the purchase price and ObjDesc.life (+0x2c,
 *   byte) the full lifetime; Cell +0x11 holds the object's remaining life.
 *     GetObjSalvageValue(o, remaining) = life ? cost*remaining/life : cost
 *     GetObjRepairCost  (o, remaining) = cost - salvage
 *   so repairing costs the fraction of the purchase price that has been worn
 *   away.  (Caller 0x00499a2d divides that by (life - remaining) to get the
 *   per-unit repair rate it shows the player.)
 */
#include "legoland.h"
#include <string.h>

#pragma intrinsic(strlen)

int abs(int);
#pragma intrinsic(abs)

/* ---------------------------------------------------------------- data ---- */

/* The static name->power table at 0x004b9340 (stride 8, 65 real entries,
 * terminated by an entry whose name is the empty string).  Names are matched
 * case-insensitively against the object's LLIDB key. */
typedef struct PowerEntry {
    char* name;    /* +0x00 */
    int   power;   /* +0x04 */
} PowerEntry;
extern PowerEntry g_power_table[];   /* 0x004b9340 */

/* The LLIDB element an object descriptor points at; only its name matters. */
typedef struct PowerName {
    char* name;    /* +0x00 */
} PowerName;

/* An object's data record; +0xc4 is its LLIDB element. */
typedef struct PowerData {
    unsigned char pad[0xc4];  /* +0x00..0xc3 */
    PowerName*    elem;       /* +0xc4 */
} PowerData;

/* The map object wrapper handed to Add/RemoveObjectsPowerStats. */
typedef struct PowerObj {
    unsigned char pad[0x0c];  /* +0x00..0x0b */
    PowerData*    data;       /* +0x0c */
} PowerObj;

/* A packed 2-byte map coordinate, passed BY VALUE. */
typedef struct BPos {
    unsigned char x;   /* +0x00 */
    unsigned char y;   /* +0x01 */
} BPos;

/* GetObjCost subject: signed 16-bit cost at +0x26, salvage divisor at +0x2c. */
typedef struct ObjDesc {
    unsigned char pad[0x26];   /* +0x00..0x25 */
    short         cost;        /* +0x26 */
    unsigned char pad28[4];    /* +0x28..0x2b */
    unsigned char life;        /* +0x2c */
} ObjDesc;

/* ------------------------------------------------------------- globals ---- */

extern int g_power_percent;      /* 0x00832bcc  spare-capacity percentage */
extern int g_power_supply;       /* 0x00832bd0  total generation */
extern int g_power_demand;       /* 0x00832bd4  total consumption */
extern int g_power_unserved;     /* 0x00832bd8  consumption currently blacked out */
extern int g_power_unserved_n;   /* 0x00832bdc  how many objects are blacked out */

extern int  NameCompare(const char* a, const char* b);  /* 0x004aab90 */
extern void RecheckPoweredObjects(void);                /* 0x0045a060 */
extern void ShedPoweredObjects(void);                   /* 0x0045a0d0 */
extern int  GetObjCost(ObjDesc* p);                     /* 0x00480da0 */
extern int  GetObjSalvageValue(ObjDesc* p, int age);    /* 0x00480db0 */

/* ----------------------------------------------------------- functions ---- */

// FUNCTION: LEGOLAND 0x00480de0
int GetObjRepairCost(ObjDesc* p, int age)
{
    int salvage = GetObjSalvageValue(p, age);
    return GetObjCost(p) - salvage;
}

// FUNCTION: LEGOLAND 0x00459fa0
int FindObjectsPower(PowerData* d)
{
    PowerEntry* p = g_power_table;

    while (strlen(p->name) != 0) {
        if (NameCompare(d->elem->name, p->name) == 0)
            return p->power;
        p++;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x0045a130
void AddObjectsPowerStats(PowerObj* obj, Pos* pos)
{
    int power = FindObjectsPower(obj->data);

    if (power == 0)
        return;

    if (power > 0) {
        g_power_supply += power;
        if (g_power_unserved != 0)
            RecheckPoweredObjects();
        return;
    }

    {
        Cell* cell;
        int   x = pos->x;
        int   load;

        if (x < 0 || x >= g_map->width) goto nocell;
        {
            int y = pos->y;
            if (y < 0 || y >= g_map->height) goto nocell;
            cell = &g_map_rows[y][x];
            goto have;
        }
    nocell:
        cell = 0;
    have:
        load = abs(power);
        g_power_demand += load;
        if (g_power_demand - g_power_unserved > g_power_supply) {
            g_power_unserved += load;
            g_power_unserved_n++;
            cell->flags |= 0x100;
        } else {
            cell->flags &= ~0x100;
        }
    }

    if (g_power_supply <= g_power_demand)
        g_power_percent = 0;
    else
        g_power_percent = 100 - (g_power_demand * 100) / g_power_supply;
}

// FUNCTION: LEGOLAND 0x0045a230
void RemoveObjectsPowerStats(PowerObj* obj, BPos bp)
{
    int power = FindObjectsPower(obj->data);

    if (power == 0)
        return;

    if (power > 0) {
        Cell* cell;
        int   x = bp.x;
        int   y = bp.y;

        if (x < 0 || x >= g_map->width) goto nocell1;
        if (y < 0 || y >= g_map->height) goto nocell1;
        cell = &g_map_rows[y][x];
        goto have1;
    nocell1:
        cell = 0;
    have1:
        if (!(cell->flags & 0x200)) {
            g_power_supply -= power;
            if (g_power_demand - g_power_unserved > g_power_supply)
                ShedPoweredObjects();
        }
    } else {
        int load = abs(power);
        g_power_demand -= load;
        if (g_power_unserved != 0) {
            Cell* cell;
            int   x = bp.x;
            int   y = bp.y;

            if (x < 0 || x >= g_map->width) goto nocell2;
            if (y < 0 || y >= g_map->height) goto nocell2;
            cell = &g_map_rows[y][x];
            goto have2;
        nocell2:
            cell = 0;
        have2:
            if (cell->flags & 0x100) {
                g_power_unserved -= load;
                g_power_unserved_n--;
            }
            RecheckPoweredObjects();
        }
    }

    if (g_power_supply <= g_power_demand)
        g_power_percent = 0;
    else
        g_power_percent = 100 - (g_power_demand * 100) / g_power_supply;
}
