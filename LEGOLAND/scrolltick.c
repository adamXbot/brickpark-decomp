/* LEGOLAND — map scrolling and the object-damage tick.
 *
 * Two small simulation subsystems that share this file only by lane:
 *
 *  - Scrolling.  The scroll origin lives in the 8.8 fixed-point pair
 *    g_scroll_x / g_scroll_y (0x00667cb4 / 0x00667cb8) that scroll.c's
 *    Get_XScroll / Get_YScroll shift down by 8.  ProcessScrolling applies a
 *    delta and then hands the (now stale) origin to ClampScrollToMap, which
 *    re-derives the isometric diamond that bounds the map and pushes the
 *    origin back inside it.
 *
 *  - Damage.  "Damage" here is gameplay wear, NOT dirty-rectangle repaint
 *    tracking: DamageTickDue gates the sweep on a wall-clock rate, every cell
 *    on the damage chain has its condition byte decremented, and
 *    UpdateDamagedCell breaks the object, raises a repair order and withdraws
 *    the object's power contribution once its condition falls below a quarter
 *    of its class maximum.
 */
#include "legoland.h"

/* --------------------------------------------------------------- scrolling */

extern int g_scroll_x;          /* 0x00667cb4  8.8 fixed point */
extern int g_scroll_y;          /* 0x00667cb8  8.8 fixed point */

/* The eight view-geometry values at 0x004b95f4.. are all used at half value,
 * so the scroll clamp reads them as a block and halves each one.  They are
 * SHIPPED CONSTANTS in .data (950.0 / 950.0 / 950.0 / 950.0 / 360.0 / 360.0 /
 * 300.0 / 300.0 in 8.8, i.e. 243200 x4, 92160 x2, 76800 x2), and nothing in
 * the game ever writes them: this block is the SLACK the clamp allows past
 * each map edge, not a view rectangle.
 *
 * They were called `g_view_left/top/right/bottom/w/h/ox/oy` until PORT-M15.
 * coaster3d.c / coaster10.c / unref3.c use those four names for a DIFFERENT
 * object -- the coaster's 3D clip rect in .bss at 0x008299ac, which
 * Coaster3D_SetupView publishes from lpConfig's view rect -- so the closure
 * generator emitted one object for each colliding name and this file read the
 * coaster's zeroed rect.  With hl == 0 the first edge clamp settles on
 * d == 0 and the view stops 475 px (243200 >> 1, in 8.8) short of the east
 * edge on every map (PORT-P3 section 3.1 measured it three times).  The names
 * here are the ones that moved, because these are the slack and those are
 * really a view. */
extern int g_scroll_slack_left;   /* 0x004b95f4 */
extern int g_scroll_slack_top;    /* 0x004b95f8 */
extern int g_scroll_slack_right;  /* 0x004b95fc */
extern int g_scroll_slack_bottom; /* 0x004b9600 */
extern int g_scroll_slack_w;      /* 0x004b9604 */
extern int g_scroll_slack_h;      /* 0x004b9608 */
extern int g_scroll_slack_ox;     /* 0x004b960c */
extern int g_scroll_slack_oy;     /* 0x004b9610 */

/* The map header carries the view/window extent the clamp needs at +0x10/+0x12
 * (tile units), so give this file its own view of it. */
typedef struct ScrollMap {
    char           pad0[0x10];
    unsigned short view_w;      /* +0x10 */
    unsigned short view_h;      /* +0x12 */
    unsigned short width;       /* +0x14 */
    unsigned short height;      /* +0x16 */
} ScrollMap;
extern ScrollMap* g_scroll_map; /* 0x004bcbf4 — the same object as g_map */

extern void GetTileDimensions(int* out_w, int* out_h);   /* 0x00460540 */
extern void ClampScrollToMap(int vw, int vh, int pad_x, int pad_y);

// FUNCTION: LEGOLAND 0x004614a0
void ProcessScrolling(int dx, int dy)
{
    int vw;
    int vh;

    g_scroll_x += dx;
    g_scroll_y += dy;
    vw = g_scroll_map->view_w;
    vh = g_scroll_map->view_h;
    ClampScrollToMap(vw << 8, vh << 8, 0, 0);
}

/* ------------------------------------------------------------------ damage */

/* A cell's position on the damage chain is stored packed into two bytes at
 * Cell+0x06 — the map coordinates of the NEXT damaged cell.  0 terminates the
 * chain (and so cell 0,0 can only ever be its head). */
typedef union DamageRef {
    unsigned short w;
    struct { unsigned char x, y; } b;
} DamageRef;

/* Cell as this file needs it: the chain link at +0x06 and the condition byte
 * at +0x11 are not named in legoland.h. */
typedef struct DmgCell {
    void*          obj;         /* +0x00  the placed object */
    unsigned char  bx;          /* +0x04 */
    unsigned char  by;          /* +0x05 */
    DamageRef      next;        /* +0x06  next cell on the damage chain */
    unsigned short tile;        /* +0x08 */
    unsigned short base;        /* +0x0a */
    unsigned short flags;       /* +0x0c */
    unsigned short uflags;      /* +0x0e */
    unsigned char  rf;          /* +0x10 */
    unsigned char  cond;        /* +0x11  condition, counts down to 1 */
    unsigned char  pad12[2];
} DmgCell;

/* Cell flag bits this subsystem owns. */
#define CF_NOWEAR       0x0004  /* never allowed to fall below the threshold */
#define CF_DAMAGEABLE   0x0080  /* takes part in the damage sweep */
#define CF_BROKEN       0x0200  /* condition is under a quarter of maximum */
#define CF_REPAIRORDER  0x4000  /* a repair order has already been raised */

/* The object class record hangs off the placed object at +0x0c; its maximum
 * condition is the byte at +0x2c. */
typedef struct DmgClass {
    char          pad0[0x2c];
    unsigned char max_cond;     /* +0x2c */
} DmgClass;
typedef struct DmgObj {
    char       pad0[0x0c];
    DmgClass*  cls;             /* +0x0c */
} DmgObj;

extern DmgCell**  g_dmg_rows;   /* 0x00801400 — same table as g_map_rows */
extern DamageRef  g_dmg_head;   /* 0x007febb8 — head of the damage chain */

extern int  g_dmg_rate;         /* 0x00832980  sweeps per 360 clock units */
extern int  g_dmg_clock;        /* 0x00667d58  clock at the last sweep */

extern int  g_power_supply;     /* 0x00832bd0 */
extern int  g_power_load;       /* 0x00832bd4 */
extern int  g_power_load_base;  /* 0x00832bd8 */

/* Whether the map is running repair orders at all. */
typedef struct RepairMap {
    char pad0[0x3c];
    int  repairs_on;            /* +0x3c */
} RepairMap;
extern RepairMap* g_repair_map; /* 0x004bcbf4 — the same object as g_map */

extern int  GetSimClock(void);                              /* 0x00499460 */
#ifndef LEGOLAND_PORTABLE
extern int  AddRepairOrderForObject(DmgClass* cls, int x, int y); /* 0x0049b930 */
#else
/* workers2.c defines the second parameter as a by-value `Pos`, which on x86
 * is just the two dwords this flattened declaration pushes -- but on wasm32 a
 * by-value struct is passed as a POINTER to a copy, so the two prototypes are
 * different function types and the link resolves this call to a trapping
 * stub. Build the aggregate at the call instead; the x/y pair below is
 * already a `Pos*`'s two fields. */
extern int  AddRepairOrderForObject(DmgClass* cls, Pos pos);      /* 0x0049b930 */
static int ll_add_repair_order_xy(DmgClass* cls, int x, int y)
{
    Pos p;
    p.x = x;
    p.y = y;
    return AddRepairOrderForObject(cls, p);
}
#define AddRepairOrderForObject(_cls, _x, _y) \
    ll_add_repair_order_xy((_cls), (_x), (_y))
#endif
extern int  FindObjectsPower(DmgClass* cls);                /* 0x00459fa0 */
extern void ShedPowerLoad(void);                            /* 0x0045a0d0 */

// FUNCTION: LEGOLAND 0x00463520
int DamageTickDue(void)
{
    if (g_dmg_rate != 0) {
        if (GetSimClock() - g_dmg_clock > 360 / g_dmg_rate) {
            g_dmg_clock = GetSimClock();
            return 1;
        }
    }
    return 0;
}

/* Re-evaluate one worn cell.  CF_BROKEN is recomputed from scratch on every
 * call: cleared on entry, and re-set on the way out once the condition is known
 * to be under a quarter of the class maximum.  Note the single trailing
 * `|= CF_BROKEN` shared by both arms — VC6 duplicates it into the CF_NOWEAR arm
 * rather than branching, which is why the original has two identical
 * `or byte ptr [esi+0dh], 2` instructions. */
// FUNCTION: LEGOLAND 0x00463460
void UpdateDamagedCell(DmgCell* cell, Pos* pos)
{
    DmgClass* cls;
    int was;
    int threshold;
    int power;

    if (cell->cond == 0)
        return;

    was = cell->flags;
    cls = ((DmgObj*)cell->obj)->cls;
    threshold = cls->max_cond >> 2;
    cell->flags &= 0xfdff;              /* ~CF_BROKEN */
    if (cell->cond >= threshold)
        return;

    if (cell->flags & CF_NOWEAR) {
        /* Protected object: pinned at the threshold, never breaks further. */
        cell->cond = (unsigned char)threshold;
    } else {
        if (g_repair_map->repairs_on && !(cell->flags & CF_REPAIRORDER)) {
            if (AddRepairOrderForObject(cls, pos->x, pos->y))
                cell->flags |= CF_REPAIRORDER;
        }
        if (!(was & CF_BROKEN)) {
            /* Newly broken this tick: take its output off the grid, and if the
             * grid now can't meet demand, shed load. */
            power = FindObjectsPower(cls);
            if (power > 0) {
                g_power_supply -= power;
                if (g_power_load - g_power_load_base > g_power_supply)
                    ShedPowerLoad();
            }
        }
    }
    cell->flags |= CF_BROKEN;
}

/* One damage sweep: walk the chain of damageable cells, wear each one down by
 * the tick's step and re-evaluate it.  The chain head is packed the same way as
 * Cell.next, so a head of 0 means cell (0,0) — the only cell that can be the
 * head, since 0 is also the terminator.  It is only treated as a real entry if
 * (0,0) carries CF_DAMAGEABLE|0x20. */
// FUNCTION: LEGOLAND 0x00463580
void ProcessDamage(void)
{
    DamageRef link;
    Pos pos;
    DmgCell* cell;
    int step;

    link = g_dmg_head;
    step = DamageTickDue();
    if (step == 0)
        return;

    if (link.w == 0) {
        pos.x = 0;
        pos.y = 0;
        if (0 < g_map->width && 0 < g_map->height)
            cell = &g_dmg_rows[0][0];
        else
            cell = 0;
        if ((cell->flags & 0xa0) == 0)
            return;
    }

    do {
        pos.x = link.b.x;
        pos.y = link.b.y;
        if (pos.x >= 0 && pos.x < g_map->width &&
            pos.y >= 0 && pos.y < g_map->height)
            cell = &g_dmg_rows[pos.y][pos.x];
        else
            cell = 0;

        if (cell->flags & CF_DAMAGEABLE) {
            if (cell->cond > 1) {
                if (cell->cond > step)
                    cell->cond -= (unsigned char)step;
                else
                    cell->cond = 1;
            }
            UpdateDamagedCell(cell, &pos);
        }
        link = cell->next;
    } while (link.w != 0);
}

/* Push the (already updated) scroll origin back inside the map's isometric
 * diamond.  vw/vh are the visible extent and pad_x/pad_y an origin bias that is
 * added on the way in and taken off on the way out; ProcessScrolling passes 0.
 *
 * The diamond, in 8.8 fixed screen units with the map's north corner at (0,0):
 *   tx = tile_w * 128 = half a tile width; ty = tx/2 = half a tile height.
 *   map x+1 -> screen (+tx, +ty);  map y+1 -> screen (-tx, +ty)
 *   east  = mapw*tx  (east corner), west = -(maph*tx) (west corner),
 *   south_w = mapw*ty, south_h = maph*ty, south = south_w + south_h.
 * Four axis clamps bound the box; four edge clamps push the origin back along
 * the diamond's edges, moving 1/2 of the overshoot in x and 1/4 in y (the 2:1
 * isometric slope).  tx/ty are reused as south_h and the (east-west) span after
 * the corner values are derived — the original reuses the same two slots.
 *
 * Codegen notes (VC6's register allocation hinges on these; playbook material):
 *  - the four multiplies must be ordered east, south_w, west, south_h and
 *    x must be assigned before y: this is what keeps hw in ebx across the
 *    GetTileDimensions call, spills hh, and puts south_w in eax / y in ebp;
 *  - every axis clamp is written with the limit expression inline (no `lim`
 *    temporary).  Clamp 3 in particular must not go through a temporary: a
 *    named local there links its `vh` read with the edge-clamp reads and the
 *    allocator then refuses to keep vh in ecx across edge clamps 3->4 (the
 *    `lea ebx,[ecx+ebp]` / `jmp` / `mov ecx,[vh]` fix-up block at 0x461440). */
// FUNCTION: LEGOLAND 0x00461290
void ClampScrollToMap(int vw, int vh, int pad_x, int pad_y)
{
    int tx, ty;
    int hl, ht, hr, hb, hw, hh, hox, hoy;
    int mapw, maph;
    int east, west, south_w, south;
    int x, y, d;

    hl  = g_scroll_slack_left   >> 1;
    ht  = g_scroll_slack_top    >> 1;
    hr  = g_scroll_slack_right  >> 1;
    hb  = g_scroll_slack_bottom >> 1;
    hox = g_scroll_slack_ox     >> 1;
    hoy = g_scroll_slack_oy     >> 1;
    hh  = g_scroll_slack_h      >> 1;
    hw  = g_scroll_slack_w      >> 1;
    GetTileDimensions(&tx, &ty);

    tx <<= 7;
    ty = tx >> 1;
    mapw = g_scroll_map->width;
    maph = g_scroll_map->height;
    east    = mapw * tx;
    south_w = mapw * ty;
    west    = -(maph * tx);
    tx      = maph * ty;
    ty      = west + east;
    south   = tx + south_w;
    x = pad_x + g_scroll_x;
    y = pad_y + g_scroll_y;

    if (x > hw + (east - vw))
        x = hw + (east - vw);
    if (x < west - hh)
        x = west - hh;
    if (y > south - vh + hoy)
        y = south - vh + hoy;
    if (y < -hox)
        y = -hox;

    if (y < south_w && vw + x > 0) {
        d = x - (y + y) - hl + vw;
        if (d > 0) {
            x -= d >> 1;
            y += d >> 2;
        }
    }
    if (y < tx && x < 0) {
        d = -x - (y + y) - hr;
        if (d > 0) {
            x += d >> 1;
            y += d >> 2;
        }
    }
    if (vh + y > south_w && vw + x > ty) {
        d = ((y - south_w + vh) << 1) - east - hb + x + vw;
        if (d > 0) {
            x -= d >> 1;
            y -= d >> 2;
        }
    }
    if (vh + y > tx && x < ty) {
        d = ((y - tx + vh) << 1) - x - ht + west;
        if (d > 0) {
            x += d >> 1;
            y -= d >> 2;
        }
    }

    g_scroll_x = x - pad_x;
    g_scroll_y = y - pad_y;
}
