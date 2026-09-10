/* LEGOLAND -- the animation-part applier, the boating school's boat painter,
 * the driving school's road re-stitcher, the boating-school lake relinker and
 * the jungle cruise's along-the-river boat step.
 *
 * Five unexported functions from four different subsystems that share one
 * property: each is the "something next to me changed, work out what I look
 * like now" half of its ride.  Reconstructed from original/legoland.exe with
 * the VC6 SP3 toolchain (/O2 /Gy /Gd); struct field OFFSETS, record sizes and
 * global addresses are load-bearing, the names are ours.  Types are defined
 * LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 *   0x00418fe0  BoatingSchool_DrawBoats  212/212 insns   [EXACT]
 *   0x0041bab0  BsWater_Relink           230/230 insns   [OK]
 *   0x00413650  Road_Restitch            284/284 insns   [OK]
 *   0x004334c0  JcBoat_Step              294/294 insns   [OK]
 *   0x00442040  AnimApplyPart            331/331 insns   exact
 *
 * ONE RENAME.  0x00418fe0 was called BoatingSchool_UpdateWater by the extern
 * in ridecb5.c; it touches no water tile and no map cell, it PAINTS the boats
 * and their riders, so it is BoatingSchool_DrawBoats here.  ONE CORRECTION:
 * junglecruise.c describes JcBoat_Step's second argument as "which of the two
 * turn tables to use"; it is a flag that switches on the route hint, and
 * nothing else.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */
typedef struct Pos { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;
typedef struct SeatOfs { int x; int y; } SeatOfs;
typedef struct Vec3 { float x; float y; float z; } Vec3;

/* The map header (screen origin of cell (0,0) only). */
typedef struct MapHdr {
    unsigned char  pad00[0x20];
    unsigned short origin_x;        /* +0x20 */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

/* A loaded image list (the LLIDB .ILF/.CSP descriptor). */
typedef struct ImageList {
    unsigned char pad00[8];
    void**        sprites;          /* +0x08 */
    int*          dx;               /* +0x0c  doubled at load, halved here */
    int*          dy;               /* +0x10 */
} ImageList;

/* The 3D person record, as this file touches it. */
typedef struct Person3D {
    unsigned char pad00[0x1c];
    int           sx;               /* +0x1c  screen position */
    int           sy;               /* +0x20 */
    unsigned char pad24[0x40 - 0x24];
    Vec3          rot;              /* +0x40  rot.y (+0x44) is the heading */
} Person3D;

/* The visitor's ride state machine byte, the only Bloke field used here. */
typedef struct Bloke {
    unsigned char pad00[0x60];
    unsigned char stage;            /* +0x60 */
} Bloke;

/* One boating-school boat: the jungle cruise's JcBoat with ONE rider instead
 * of three, so everything from +0x3e4 on sits four bytes lower.  The two
 * animation buffers are the same size and shape: 80 rocking offsets and 80
 * sprite codes, indexed by the ride-wide playback cursor g_bs_tick. */
typedef struct BsWobble { int x; int y; } BsWobble;

typedef struct BsBoat {
    BPosW          key;             /* +0x00  the station that launched it */
    unsigned char  pad02[2];
    int            cx;              /* +0x04  the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c  the map square it is heading for */
    int            ny;              /* +0x10 */
    int            sx;              /* +0x14  screen position, this frame */
    int            sy;              /* +0x18 */
    BsWobble       wob[0x50];       /* +0x1c   80 precomputed sub-steps */
    int            frame[0x50];     /* +0x29c  80 precomputed sprite codes */
    int            f3dc;            /* +0x3dc */
    int            f3e0;            /* +0x3e0 */
    int            state;           /* +0x3e4  1..0x10, the mover's state */
    int            leg;             /* +0x3e8  which way this leg turns */
    Bloke*         rider;           /* +0x3ec  the single passenger */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

/* ---- other subsystems --------------------------------------------------- */
extern void       GetTileDimensions(int* out_w, int* out_h);   /* 0x00460540 */
extern void       AdjustOffsetForViewMode(Pos* o);             /* 0x00442d30 */
extern void       AdjustBlokePosition(Pos* p);                 /* 0x00442d60 */
extern int        PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern Person3D*  Find3DPersonFromBloke(void* bloke);          /* 0x0043f890 */
extern void       SetPersonRotation(Person3D* p, Vec3* rot);   /* 0x00440020 */
extern void       IP_RenderBlokeIn3DNow(void* bloke);          /* 0x00440010 */

extern MapHdr*    g_map;            /* 0x004bcbf4 (lpConfig) */
extern BsBoat*    g_bs_boats;       /* 0x004cc03c */
extern int        g_bs_tick;        /* 0x004cc08c  playback cursor, 0..0x4f */
extern ImageList* g_bs_boat_ilf;    /* 0x0082c65c  the boat image list */
/* Where the single rider sits inside the boat sprite, per heading code. */
extern SeatOfs    g_bs_seat_pos[16];                           /* 0x004b51d8 */
extern int        g_scroll_x;       /* 0x00667cb4  ScrollX (24.8) */
extern int        g_scroll_y;       /* 0x00667cb8  ScrollY (24.8) */

/* =========================================================================
 * 0x00418fe0 -- BoatingSchool_DrawBoats (RENAMED; ridecb5.c calls it
 * BoatingSchool_UpdateWater, which is wrong -- it touches no water tile and
 * no map cell, it PAINTS the boats).
 *
 * It is the boating school's copy of junglecruise.c's
 * JungleCruise_UpdateRiverAnim (0x00432d00), written from it line for line
 * with one rider instead of three and without the "standing on the station's
 * own column" half of the pass test:
 *
 *   mode != 0  paint the boats INSIDE the station (state 1 = just launched,
 *              state 0x10 = arriving)
 *   mode == 0  paint the boats on open water (every other state)
 *
 * so the station building's sprite layers can be interleaved between the two
 * passes.  BoatingSchool_Tick calls it with 0 once a frame; the BOATING
 * SCHOOL class's own painter calls it with 1.
 *
 * The projection is the standard isometric one:
 *      sx = (cx - cy) * (tw/2) - (tw+1)/2 - ScrollX>>8
 *      sy = (cx + cy) * (th/2)            - ScrollY>>8
 * plus the map's screen origin and the boat's rocking offset for this
 * sub-step, itself converted from isometric (wx-wy, wx+wy) and scaled by
 * 1/512.  Note GetTileDimensions is called a SECOND time inside the loop for
 * the rocking offset even though the answer cannot have changed: original.
 *
 * The boat is two sprites -- the hull (frame code) and one overlay
 * (code + 0x30) -- with the rider drawn between them, so the passenger is
 * always sandwiched inside the boat.  The rider is turned to the boat's
 * heading with the same constants as the jungle cruise
 *      angle = -(code * 22.5 + 45) / 360 * 2*pi
 * (0x004ab3dc..0x004ab3e8) and offset by the per-heading seat position
 * g_bs_seat_pos[code & 0xf] plus (0x44, 0x34).
 *
 * Finally, on the LAST sub-step of an arriving boat's last leg (state 0x10,
 * leg 2, tick 0x4f) and only in the station pass, the rider is put ashore:
 * its stage byte is advanced and the seat cleared, which is what hands it
 * back to BoatingSchool_Tick.  That test runs for EVERY boat, drawn or not
 * (the pass test jumps into it, not past it) -- harmless, because a boat in
 * state 0x10 is always drawn when mode != 0.
 * ========================================================================= */

/* Scope H closure (2026-09-05): complete 212 instructions / 744 bytes exact.
 * Group the two horizontal values (rocking and screen) in one nonescaping
 * Axis, and the two vertical values in another. Keep rock before screen in
 * each record. The screen-Y read view and screen-X initial-write view stay
 * at function entry; a named rock-X view is also used for its initial write.
 * These local identities add no runtime operation and preserve the existing
 * coordinate formulas, global reloads, calls and writes.
 *
 * Both axis records and the rock-X write identity matter: direct rock-X
 * initialization gives 7 mismatches, grouping only the horizontal pair gives
 * 30, and reversing the fields gives 11. The previously separate scalar
 * rocking offsets and screen Pos give 5. No padding or sampled table bytes
 * are involved; the full function ends in RET. All exact neighbours pass.
 * Evidence: scratchpad/scope-h/root5/report.md and verify_boats_axes.py.
 */
// FUNCTION: LEGOLAND 0x00418fe0
void BoatingSchool_DrawBoats(int mode)
{
    struct Axis { int rock; int screen; };
    struct Axis axisx;
    struct Axis axisy;
    const int* screeny = &axisy.screen;
    int* screenx = &axisx.screen;
    int* rockx = &axisx.rock;
    int     tw;
    int     th;
    BsBoat* b;

    b = g_bs_boats;
    GetTileDimensions(&tw, &th);
    if (b == 0)
        return;
    do {
        if (mode != 0) {
            if (b->state == 1)
                goto draw;
            if (b->state == 0x10)
                goto draw;
            goto next;
        } else {
            if (b->state == 1)
                goto next;
            if (b->state == 0x10)
                goto next;
        }
draw:
        {
            int wy = b->wob[g_bs_tick].y;
            int wx = b->wob[g_bs_tick].x;
            int tw2;
            int th2;
            Pos ofs;
            const int* offsety = &ofs.y;

            GetTileDimensions(&tw2, &th2);
            *rockx = ((wx - wy) * tw2) >> 9;
            axisy.rock = ((wx + wy) * th2) >> 9;
            *screenx = (b->cx - b->cy) * (tw >> 1) - ((tw + 1) >> 1) - (g_scroll_x >> 8);
            axisy.screen = (b->cx + b->cy) * (th >> 1) - (g_scroll_y >> 8);
            {
                ofs.x = g_bs_boat_ilf->dx[b->frame[g_bs_tick] & 0xff] >> 1;
                ofs.y = g_bs_boat_ilf->dy[b->frame[g_bs_tick] & 0xff] >> 1;
                AdjustOffsetForViewMode(&ofs);
                b->sx = g_map->origin_x + axisx.rock + ofs.x + axisx.screen;
                b->sy = g_map->origin_y + axisy.rock + (*offsety) + (*screeny);
                PrintSprite(g_bs_boat_ilf->sprites[b->frame[g_bs_tick] & 0xff],
                            b->sx, b->sy, 0, 0);
                if (b->rider == 0)
                    goto next;
                {
                    Person3D* p3 = Find3DPersonFromBloke(b->rider);
                    float     ang = ((float)b->frame[g_bs_tick] * 22.5f + 45.0f)
                                    * -0.0027777769f;

                    p3->rot.y = ang * 6.2831855f;
                    SetPersonRotation(p3, &p3->rot);
                    ofs.x = g_map->origin_x + axisx.rock + axisx.screen;
                    ofs.y = g_map->origin_y + axisy.rock + (*screeny);
                    AdjustBlokePosition(&ofs);
                    {
                        Pos so;

                        so.x = g_bs_seat_pos[b->frame[g_bs_tick] & 0xf].x + 0x44;
                        so.y = g_bs_seat_pos[b->frame[g_bs_tick] & 0xf].y + 0x34;
                        AdjustOffsetForViewMode(&so);
                        p3->sx = so.x + ofs.x;
                        p3->sy = so.y + ofs.y;
                    }
                }
                IP_RenderBlokeIn3DNow(b->rider);
                PrintSprite(g_bs_boat_ilf->sprites[(b->frame[g_bs_tick] + 0x30) & 0xff],
                            b->sx, b->sy, 0, 0);
            }
        }
next:
        if (b->state == 0x10 && b->leg == 2 && g_bs_tick == 0x4f && mode != 0
            && b->rider != 0) {
            b->rider->stage++;
            b->rider = 0;
        }
        b = b->next;
    } while (b);
}

/* =========================================================================
 * THE BOATING SCHOOL'S LAKE, AND WHY IT NEEDS A SECOND PASS
 *
 * A lake square is a 5x5 block of map cells whose CENTRE cell is the square's
 * map coordinate, so neighbouring squares are five cells apart (every +-5
 * below is one lake step).  Each square paints itself from its own four-bit
 * link mask (1 N, 2 E, 4 S, 8 W), which leaves the four 2x2 blocks BETWEEN
 * diagonally adjacent squares as bare ground.  BsWater_Relink is the pass
 * that fills those in.
 *
 * It is line for line junglecruise.c's JungleCruise_RelinkRiverCell
 * (0x004367b0) -- the two rides' water is one piece of code written twice,
 * exactly as ridecb6.c found for BsWater_Add / JcWater_Add.  The only
 * differences are the record sizes and which globals hold the list heads.
 * ========================================================================= */

/* One placed boating school (the full field list is in ridecb5.c). */
typedef struct BsStation {
    BPosW             pos;          /* +0x00 packed map square */
    BPosW             a;            /* +0x02 route START square {ax, ay} */
    BPosW             b;            /* +0x04 route END square {bx, by} */
    unsigned char     pad06[0x2c - 6];
    struct BsStation* next;         /* +0x2c */
} BsStation;                        /* 0x34 */

/* One square of boating-school water. */
typedef struct BsWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the school that owns this cell */
    int             links;          /* +0x04 link mask: 1 N, 2 E, 4 S, 8 W */
    unsigned char   pad08[8];
    struct BsWater* next;           /* +0x10 */
} BsWater;                          /* 0x1c */

extern BsStation* g_bs_stations;    /* 0x004cc074 */
/* The BOATING SCHOOL WATER class's TSM record array (LLIDB_LoadTSMData's
 * 8-byte {LLElem* entry, void* loaded} records); the loaded TSF descriptor's
 * first word is the tileset's base slot, i.e. the plain corner filler. */
extern void*      g_bs_tsm;         /* 0x0082adf4 */

extern BsWater*   BsWater_FindAt(int x, int y);                /* 0x0041c890 */
extern void       SetMapTile(int x, int y, unsigned short t);  /* 0x00461780 */

/* Tile 0 of the water tileset.  Written out at every call site (never cached)
 * because SetMapTile may move the tables, and the original reloads
 * 0x0082adf4 before each one. */
#define BS_CORNER_TILE  (**(unsigned short**)((char*)g_bs_tsm + 4))

/* =========================================================================
 * 0x0041bab0 -- BsWater_Relink: fill in the inside corners where two arms of
 * the lake meet at (x, y).
 *
 * For each of the four diagonal pairs -- (west, north), (west, south),
 * (east, north), (east, south) -- it checks that the vertical neighbour also
 * carries the matching horizontal link, and if so stamps the 2x2 corner block
 * with tile 0 of the water tileset.
 *
 * Before that the square's own mask is adjusted for the school it belongs to
 * (`owner` is the school's packed map square): the route START square loses
 * its NORTH link and the route END square its SOUTH link, so no corner is
 * ever drawn into the school building.
 *
 * ORIGINAL BUGS reproduced: the station search may end on a null cursor which
 * is then dereferenced, and each neighbour lookup is dereferenced without a
 * null check (it cannot fail while the link bit is set, but nothing enforces
 * that).
 *
 * CODEGEN NOTE: both parameter homes carry locals -- `links` lives in y's
 * slot and the `links & 8` / `links & 2` flag in x's, while x and y stay in
 * edi/esi for the whole body.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041bab0
void BsWater_Relink(int x, int y, BPosW* owner)
{
    BsStation* st = g_bs_stations;
    BsWater* w;
    BsWater* n;
    int links;

    w = BsWater_FindAt(x, y);
    while (st && st->pos.w != owner->w)
        st = st->next;
    if (w == 0)
        return;
    links = w->links;
    if (w->pos.w == st->a.w)
        links &= ~1;
    else if (w->pos.w == st->b.w)
        links &= ~4;

    if ((links & 8) && (links & 1)) {
        n = BsWater_FindAt(x, y - 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y - 3, BS_CORNER_TILE);
            SetMapTile(x - 2, y - 3, BS_CORNER_TILE);
            SetMapTile(x - 3, y - 2, BS_CORNER_TILE);
            SetMapTile(x - 2, y - 2, BS_CORNER_TILE);
        }
    }
    if ((links & 8) && (links & 4)) {
        n = BsWater_FindAt(x, y + 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y + 3, BS_CORNER_TILE);
            SetMapTile(x - 2, y + 3, BS_CORNER_TILE);
            SetMapTile(x - 3, y + 2, BS_CORNER_TILE);
            SetMapTile(x - 2, y + 2, BS_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 1)) {
        n = BsWater_FindAt(x, y - 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y - 3, BS_CORNER_TILE);
            SetMapTile(x + 2, y - 3, BS_CORNER_TILE);
            SetMapTile(x + 3, y - 2, BS_CORNER_TILE);
            SetMapTile(x + 2, y - 2, BS_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 4)) {
        n = BsWater_FindAt(x, y + 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y + 3, BS_CORNER_TILE);
            SetMapTile(x + 2, y + 3, BS_CORNER_TILE);
            SetMapTile(x + 3, y + 2, BS_CORNER_TILE);
            SetMapTile(x + 2, y + 2, BS_CORNER_TILE);
        }
    }
}

/* =========================================================================
 * THE DRIVING SCHOOL'S ROADS, AND WHAT A ROAD TILE IS
 *
 * A road block is one map square with a record on the driving school's list
 * (ridecb6.c: `group` at +0x08 is the OWNING SCHOOL's packed map square, and
 * bit 0x10 of the kind byte at +0x14 says the block also carries a ZEBRA
 * CROSSING).  Which of the road sprites a block shows is not stored: it is
 * recomputed from the blocks around it every time a neighbour is added or
 * taken away, and Road_Restitch is that recomputation for ONE block.
 *
 * The neighbourhood is probed in two steps into one array of four
 * {orthogonal, diagonal} pairs -- the orthogonal probe (0x00413520) fills the
 * `o` fields, and only if the answer is still ambiguous does the diagonal
 * probe (0x00413450) fill the `d` fields on top of them.  The four pairs are
 * the four quadrants in order, so the 8-bit mask the second pass builds runs
 *      1 N   2 NE   4 E   8 SE   0x10 S   0x20 SW   0x40 W   0x80 NW
 * while the 4-bit mask the first pass builds is just its orthogonal half
 *      1 N   2 E    4 S   8 W
 * and a neighbour only counts when it belongs to the SAME school.
 *
 * The tile is then chosen by how many orthogonal neighbours matched:
 *      0  nothing at all -- the block is left exactly as it is (no refund)
 *      1  a dead end, or 2 when the two are opposite: shape 1/7 (see below)
 *      2  a corner: shape 3, rotation from which two arms are joined
 *      3  a T junction: shape 4
 *      4  a crossroads: shape 5
 * and only the dead-end/straight family (the `case 1` block) is allowed to
 * keep the zebra-crossing bit.  Every other shape drops it, which is why the
 * tail refunds the crossing's cost with AddBricks(GetObjCost(...)) whenever
 * the block had one: re-stitching a crossing into a corner, a T or a
 * crossroads DESTROYS the crossing and gives the player the bricks back.
 * ========================================================================= */

/* One road block, as this function touches it. */
typedef struct RoadBlock {
    unsigned char  pad00[8];
    unsigned short group;           /* +0x08 the owning school's map square */
    unsigned char  pad0a[0x14 - 0x0a];
    unsigned char  kind;            /* +0x14 bit 0x10 = has a zebra crossing */
} RoadBlock;

/* One quadrant of the neighbourhood: the orthogonal block and the diagonal
 * one just past it.  The two probes fill the two fields separately. */
typedef struct RoadNb {
    RoadBlock* o;                   /* +0x00 */
    RoadBlock* d;                   /* +0x04 */
} RoadNb;

extern RoadBlock* Road_FindAt(int x, int y);                   /* 0x004125a0 */
/* Both probes return how many they found; Road_Restitch ignores it and
 * counts the ones belonging to its own school itself. */
extern int  Road_ProbeOrtho(int x, int y, RoadNb* out);        /* 0x00413520 */
extern int  Road_ProbeDiag(int x, int y, RoadNb* out);         /* 0x00413450 */
/* Stamp the road square's tiles: `shape` 1..5 (bit 0x10 keeps the zebra
 * crossing) and `rot` the quarter turn. */
extern void Road_SetTile(int x, int y, int shape, int rot);    /* 0x00412680 */
extern int  GetObjCost(void* def);                             /* 0x00480da0 */
extern void AddBricks(int bricks);                             /* 0x004578a0 */

extern void* g_zebra_def;           /* 0x0082c678  the ZEBRA CROSSING ObjDef */

/* =========================================================================
 * 0x00413650 -- Road_Restitch: re-pick the road tile at (x, y) for the
 * driving school whose packed map square is `school`.
 *
 * Called after any edit to a neighbouring square, exactly like pathtile2.c's
 * AdjustPathTile.  `n` and `m` are counted/ored in memory rather than in
 * registers because all four callee-saved registers are already spoken for
 * (ebx = the 4-bit mask, ebp = the zebra bit, esi = y, edi = x).
 *
 * ORIGINAL QUIRK reproduced: `case 0` returns without the crossing refund, so
 * an isolated block keeps both its old tile and its crossing.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00413650
void Road_Restitch(BPosW school, int x, int y)
{
    RoadNb     nb[4];
    int        n = 0;
    int        m = 0;
    int        mask = 0;
    int        zebra = 0;
    RoadBlock* me;
    int        cost;

    me = Road_FindAt(x, y);
    if (me != 0)
        zebra = me->kind & 0x10;

    Road_ProbeOrtho(x, y, nb);
    if (nb[0].o != 0 && nb[0].o->group == school.w)
        mask = n = 1;
    if (nb[1].o != 0 && nb[1].o->group == school.w) {
        n++;
        mask |= 2;
    }
    if (nb[2].o != 0 && nb[2].o->group == school.w) {
        n++;
        mask |= 4;
    }
    if (nb[3].o != 0 && nb[3].o->group == school.w) {
        n++;
        mask |= 8;
    }

    switch (n) {
    case 0:
        return;
    case 2:
        if ((mask & 5) != 5 && (mask & 0xa) != 0xa) {
            switch (mask) {
            case 6:
                Road_SetTile(x, y, 3, 0);
                break;
            case 12:
                Road_SetTile(x, y, 3, 1);
                break;
            case 9:
                Road_SetTile(x, y, 3, 2);
                break;
            case 3:
                Road_SetTile(x, y, 3, 3);
                break;
            }
            break;
        }
        /* two OPPOSITE arms is a straight road: fall through */
    case 1:
        Road_ProbeDiag(x, y, nb);
        if (nb[0].o != 0 && nb[0].o->group == school.w)
            m = 1;
        if (nb[0].d != 0 && nb[0].d->group == school.w)
            m |= 2;
        if (nb[1].o != 0 && nb[1].o->group == school.w)
            m |= 4;
        if (nb[1].d != 0 && nb[1].d->group == school.w)
            m |= 8;
        if (nb[2].o != 0 && nb[2].o->group == school.w)
            m |= 0x10;
        if (nb[2].d != 0 && nb[2].d->group == school.w)
            m |= 0x20;
        if (nb[3].o != 0 && nb[3].o->group == school.w)
            m |= 0x40;
        if (nb[3].d != 0 && nb[3].d->group == school.w)
            m |= 0x80;
        if (m & 0x11) {
            if ((m & 0x83) == 0x83) {
                if ((m & 0x38) == 0x38) {
                    zebra |= 7;
                    Road_SetTile(x, y, zebra, 0);
                    return;
                }
                zebra |= 1;
                Road_SetTile(x, y, zebra, 0);
                return;
            }
            if ((m & 0x38) == 0x38) {
                zebra |= 1;
                Road_SetTile(x, y, zebra, 2);
                return;
            }
            Road_SetTile(x, y, zebra, 0);
            return;
        }
        if ((m & 0xe0) == 0xe0) {
            if ((m & 0xe) == 0xe) {
                zebra |= 7;
                Road_SetTile(x, y, zebra, 1);
                return;
            }
            zebra |= 1;
            Road_SetTile(x, y, zebra, 3);
            return;
        }
        if ((m & 0xe) == 0xe) {
            zebra |= 1;
            Road_SetTile(x, y, zebra, 1);
            return;
        }
        Road_SetTile(x, y, zebra, 1);
        return;
    case 3:
        switch (mask) {
        case 11:
            Road_SetTile(x, y, 4, 0);
            break;
        case 7:
            Road_SetTile(x, y, 4, 1);
            break;
        case 14:
            Road_SetTile(x, y, 4, 2);
            break;
        case 13:
            Road_SetTile(x, y, 4, 3);
            break;
        }
        break;
    case 4:
        Road_SetTile(x, y, 5, 0);
        break;
    }
    if (zebra != 0) {
        cost = GetObjCost(g_zebra_def);
        AddBricks(cost);
    }
}

/* =========================================================================
 * THE JUNGLE CRUISE'S BOAT MOVER
 *
 * A river square is a 5x5 block of map cells, so every +-5 below is one
 * river step, and JcWater::links (1 N, 2 E, 4 S, 8 W) says which of the four
 * neighbours exist.  JungleCruise_AdvanceBoats (junglecruise.c 0x004332f0)
 * commits the square a boat was heading for and then dispatches on its state
 * word; states 4 and 8 both land here, with `steer` 0 and 1 respectively.
 *
 * JcBoat::f3dc (+0x3dc) is NOT the boat's heading -- it is the side it came
 * IN by, which is why the mover's first move is to take the OPPOSITE bit as
 * its preferred exit and why `links & ~f3dc` is "anywhere but back the way I
 * came".  -1 means "stuck", 1/2/4/8 the four sides.
 * ========================================================================= */

typedef struct JcStation {
    BPosW             pos;          /* +0x00  the station's own map square */
    BPosW             a;            /* +0x02  route START square */
    BPosW             b;            /* +0x04  route END square */
    unsigned char     pad06[0x3c - 6];
    struct JcStation* next;         /* +0x3c */
} JcStation;                        /* 0x44 */

typedef struct JcWater {
    BPosW           pos;            /* +0x00  this square */
    BPosW           owner;          /* +0x02  the station that owns the river */
    int             links;          /* +0x04  1 N, 2 E, 4 S, 8 W */
    unsigned char   pad08[0x18 - 8];
    struct JcWater* route_next;     /* +0x18  route: the square after this one */
} JcWater;                          /* 0x1c */

typedef struct JcBoat {
    BPosW           key;            /* +0x00  the station that launched it */
    unsigned char   pad02[2];
    int             cx;             /* +0x04  the map square it is on */
    int             cy;             /* +0x08 */
    int             nx;             /* +0x0c  the map square it is heading for */
    int             ny;             /* +0x10 */
    unsigned char   pad14[0x3dc - 0x14];
    int             f3dc;           /* +0x3dc the side it entered this square by */
    int             state;          /* +0x3e0 */
    int             leg;            /* +0x3e4 squares left in this leg */
    unsigned char   pad3e8[0x3f4 - 0x3e8];
    struct JcBoat*  next;           /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern JcStation* g_jc_stations;    /* 0x00629c3c */
extern JcBoat*    g_jc_boats;       /* 0x00616164 */

extern JcWater* JcWater_FindAt(int x, int y);                  /* 0x004371b0 */
/* Refills the boat's 80-step wobble/frame buffers for a move from side
 * `from` to side `to` (-1 = stay put). */
extern void     JcBoat_Animate(JcBoat* b, int from, int to);   /* 0x00433840 */
extern int      rand(void);                                    /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x004334c0 -- JcBoat_Step: choose the river square this boat crosses next
 * and lay down the 80-frame animation for the crossing.
 *
 * THE SECOND PARAMETER IS NOT A TABLE SELECTOR (the note in junglecruise.c
 * guessed that and it is wrong): it switches on ONE extra rule, the route
 * hint.  With `steer` set, the square's route successor (JcWater +0x18) is
 * compared with the square itself and one link is struck off -- south when
 * the route runs level, otherwise east or west by the sign of the x step.
 * That is the whole difference between states 4 and 8.
 *
 * The choice is made by elimination, in this order:
 *   1. start from the square's own link mask;
 *   2. strike off any direction whose neighbour square is occupied by, or is
 *      the target of, ANOTHER boat -- both that boat's current square
 *      (+0x04/+0x08) and the one it is heading for (+0x0c/+0x10) count, so
 *      boats never swap places or pile up;
 *   3. apply the route hint above when `steer` is set;
 *   4. if nothing is left, animate "stay put" (to = -1), mark the boat stuck
 *      (f3dc = -1) and return;
 *   5. one time in eight, if there is any way on other than straight back,
 *      throw the others away at random until exactly one remains and take
 *      it -- this is the wandering that makes the boats look unscripted;
 *   6. otherwise prefer straight ahead (the side opposite the one it came in
 *      by), then one of the two turns in random order, then back the way it
 *      came as a last resort.
 * The square's own arrival side is then recorded as the OPPOSITE of the way
 * it left, the leg counter is decremented, and reaching zero puts the boat
 * in state 8 -- the steered state, which is how a boat that has wandered
 * long enough is pulled back onto the route to the station.
 *
 * ORIGINAL QUIRKS reproduced: the station search may end on a null cursor
 * which is then dereferenced; step 5 discards the current heading from the
 * candidate set, so the one-in-eight boat NEVER carries straight on; and
 * step 6's last resort recomputes the direction it already rejected first.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004334c0
void JcBoat_Step(JcBoat* b, int steer)
{
    JcStation* st = g_jc_stations;
    JcBoat*    o = g_jc_boats;
    JcWater*   w;
    int        links;
    int        avail;
    int        cnt;
    int        i;
    int        back;
    int        dir;
    int        step;
    Pos        d;
    char       r;

    w = JcWater_FindAt(b->cx, b->cy);
    while (st && w->owner.w != st->pos.w)
        st = st->next;
    links = w->links;
    if (w->pos.w == st->a.w) {
        links &= ~1;
    } else if (w->pos.w == st->b.w) {
        b->state = 0x10;
        b->leg = 3;
        JcBoat_Animate(b, b->f3dc, 4);
        b->f3dc = 1;
        b->ny = b->cy + 5;
        return;
    }
    while (o != 0) {
        if (o != b) {
            if ((b->cx == o->cx && b->cy - 5 == o->cy)
                || (b->cx == o->nx && b->cy - 5 == o->ny))
                links &= ~1;
            if ((b->cx + 5 == o->cx && b->cy == o->cy)
                || (b->cx + 5 == o->nx && b->cy == o->ny))
                links &= ~2;
            if ((b->cx == o->cx && b->cy + 5 == o->cy)
                || (b->cx == o->nx && b->cy + 5 == o->ny))
                links &= ~4;
            if ((b->cx - 5 == o->cx && b->cy == o->cy)
                || (b->cx - 5 == o->nx && b->cy == o->ny))
                links &= ~8;
        }
        o = o->next;
    }
    if (steer != 0 && w->route_next != 0) {
        d.x = w->route_next->pos.b.x - w->pos.b.x;
        d.y = w->route_next->pos.b.y - w->pos.b.y;
        if (d.y != 0) {
            if (d.x < 0)
                links &= ~8;
            else
                links &= ~2;
        } else {
            links &= ~4;
        }
    }
    if (links == 0) {
        JcBoat_Animate(b, b->f3dc, -1);
        b->f3dc = -1;
        return;
    }
    if ((rand() & 7) == 0) {
        avail = links & ~b->f3dc;
        if (avail != 0) {
            for (;;) {
                i = 0;
                cnt = 0;
                for (; i < 4; i++) {
                    if (avail & (1 << i))
                        cnt++;
                }
                if (cnt <= 1)
                    break;
                avail &= ~(1 << (rand() & 3));
            }
            links = avail;
        }
    }
    for (i = 0; i < 4; i++) {
        if (b->f3dc & (1 << i))
            break;
    }
    back = (i + 2) % 4;
    dir = 1 << back;
    if ((links & dir) == 0) {
        r = (char)rand();
        step = (r & 1) ? 1 : -1;
        dir = 1 << ((back + step) & 3);
        if ((links & dir) == 0) {
            dir = 1 << ((back - step) & 3);
            if ((links & dir) == 0)
                dir = 1 << ((back + 2) % 4);
        }
    }
    switch (dir) {
    case 1:
        b->nx = b->cx;
        b->ny = b->cy - 5;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 4;
        break;
    case 2:
        b->nx = b->cx + 5;
        b->ny = b->cy;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 8;
        break;
    case 4:
        b->nx = b->cx;
        b->ny = b->cy + 5;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 1;
        break;
    case 8:
        b->nx = b->cx - 5;
        b->ny = b->cy;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 2;
        break;
    }
    if (--b->leg == 0)
        b->state = 8;
}

/* =========================================================================
 * THE OUTFIT SYSTEM: HOW ONE MESH WEARS DIFFERENT CLOTHES
 *
 * savechunks.c's MakeAnimInstance (0x00442580) gives every bloke a private
 * copy of the animation's 0x24-byte part array and then calls this function
 * TWICE -- once for the "A" outfit table and once for the "B" one -- before
 * handing the copy to RecolourModelParts.  AnimApplyPart is the half that
 * moves TEXTURE COORDINATES, and it does it by rectangle substitution.
 *
 * The model context (the record data2.c gets from GetModelContext, 0x00443710)
 * carries at +0x30 a table of 6-byte rectangles, one per wearable patch:
 *
 *      +0x00  short id     texture id, RELATIVE to the context's base (+0x04)
 *      +0x02  u8    x, y   the patch's top-left corner IN TEXELS
 *      +0x04  u8    w, h   its size, as an inclusive extent (see below)
 *
 * `from` and `to` are indices into that table.  Every part of the instance
 * that is textured (flag bit 0x2000 clear) with the FROM patch's texture and
 * whose three texture coordinates all land inside the FROM rectangle is
 * re-pointed at the TO patch's texture and has its coordinates mapped into
 * the TO rectangle:
 *
 *      u' = ((u * texw(from) - from.x) / from.w * to.w + to.x) / texw(to)
 *
 * i.e. the normalised UV is expanded to texels in the source texture, made
 * relative to the source rectangle, rescaled to the destination rectangle
 * and re-normalised against the DESTINATION texture's size.  So swapping a
 * shirt is one table entry: the artist draws every variant somewhere in the
 * texture atlas, and the model itself never changes.
 *
 * The containment test uses `x .. x + w + 1` inclusive on both axes -- one
 * texel of slack past the stated extent, deliberately, so a patch's own edge
 * coordinates still count as inside.  UVs are clamped to [0,1] first (the
 * upper clamp compares against a DOUBLE 1.0 and assigns a float 1.0f, which
 * is what puts the 8-byte constant in .rdata).
 * ========================================================================= */

/* One triangle's material record inside a person's private part array. */
typedef struct AnimPart {
    int   flags;                    /* +0x00  bit 0x2000 = flat colour */
    int   rgb;                      /* +0x04 */
    int   tex;                      /* +0x08  absolute texture id */
    float uv[3][2];                 /* +0x0c  u,v per corner */
} AnimPart;                         /* 0x24 */

/* One wearable patch: where it lives inside its texture. */
typedef struct OutfitRect {
    short         id;               /* +0x00  texture id, relative to base */
    unsigned char x;                /* +0x02 */
    unsigned char y;                /* +0x03 */
    unsigned char w;                /* +0x04 */
    unsigned char h;                /* +0x05 */
} OutfitRect;                       /* 6 */

typedef struct ModelCtx {
    unsigned char pad00[4];
    int           base;             /* +0x04  texture id base */
    unsigned char pad08[0x30 - 8];
    OutfitRect*   rects;            /* +0x30  the patch table */
} ModelCtx;

/* Every loaded texture's pixel size, indexed by absolute texture id. */
typedef struct TexSize { int w; int h; } TexSize;
extern TexSize g_texsize[];         /* 0x0081c0c0 */

/* =========================================================================
 * 0x00442040 -- AnimApplyPart: re-point every part wearing patch `from` at
 * patch `to`, mapping its texture coordinates between the two rectangles.
 *
 * MakeAnimInstance declares the two index arguments as `void*` because it
 * only ever passes them through; they are plain table indices.
 *
 * ORIGINAL QUIRKS reproduced: the six coordinates are clamped into [0,1] and
 * scaled into texels IN PLACE before the containment test, so a part that
 * fails the test has already had its local copies mangled (harmless, they
 * are discarded); and a part is either wholly remapped or wholly left alone
 * -- there is no clipping.
 * ========================================================================= */

/* Exact: 331 instructions / 1174 bytes. The six named scalar UV addresses
 * preserve the original x87 plan: u0 stays live until final copyback, while
 * the X conversions spill and the Y conversions remain on the FPU stack.
 *
 * Explicit float conversions around the relative-coordinate division and
 * destination-scale multiplication complete the instruction schedule. VC6
 * keeps these conversion tuples while emitting no extra rounding operations;
 * they advance the texture reload/store and first three integer UV copies
 * into the original v1-remap phase. The same two-stage spelling is used for
 * all six coordinates. Removing all casts restores 17 differences. A minimal
 * control keeps only the first four coordinates' eight casts; deleting any
 * one of those eight gives two differences. The uniform six-coordinate form
 * is retained. No FP operation, storage home or arithmetic order changes.
 * Evidence and deletion controls: scratchpad/scope-h/animation8/report.md. */
// FUNCTION: LEGOLAND 0x00442040
void AnimApplyPart(ModelCtx* ctx, int from, int to, AnimPart* parts, int n)
{
    OutfitRect* a = &ctx->rects[from];
    OutfitRect* b = &ctx->rects[to];
    int         idA = a->id + ctx->base;
    int         idB = b->id + ctx->base;
    int         x0 = a->x;
    int         x1 = a->x + a->w + 1;
    int         y0 = a->y;
    int         y1 = a->y + a->h + 1;
    AnimPart*   p;
    int         i;
    int         tw;
    int         th;

    if (n <= 0)
        return;
    p = parts;
    i = n;
    do {
        if ((p->flags & 0x2000) == 0 && p->tex == idA) {
            float u0 = p->uv[0][0];
            float v0 = p->uv[0][1];
            float u1 = p->uv[1][0];
            float v1 = p->uv[1][1];
            float u2 = p->uv[2][0];
            float v2 = p->uv[2][1];

            const float* p_u0 = &u0;
            const float* p_v0 = &v0;
            const float* p_u1 = &u1;
            const float* p_v1 = &v1;
            const float* p_u2 = &u2;
            const float* p_v2 = &v2;

            if (u0 < 0.0f)
                u0 = 0.0f;
            if (u0 > 1.0)
                u0 = 1.0f;
            if (v0 < 0.0f)
                v0 = 0.0f;
            if (v0 > 1.0)
                v0 = 1.0f;
            if (u1 < 0.0f)
                u1 = 0.0f;
            if (u1 > 1.0)
                u1 = 1.0f;
            if (v1 < 0.0f)
                v1 = 0.0f;
            if (v1 > 1.0)
                v1 = 1.0f;
            if (u2 < 0.0f)
                u2 = 0.0f;
            if (u2 > 1.0)
                u2 = 1.0f;
            if (v2 < 0.0f)
                v2 = 0.0f;
            if (v2 > 1.0)
                v2 = 1.0f;
            tw = g_texsize[idA].w;
            th = g_texsize[idA].h;
            u0 = u0 * tw;
            u1 = u1 * tw;
            u2 = u2 * tw;
            v0 = v0 * th;
            v1 = v1 * th;
            v2 = v2 * th;
            if (u0 >= x0 && u0 <= x1 && u1 >= x0 && u1 <= x1
                && u2 >= x0 && u2 <= x1
                && v0 >= y0 && v0 <= y1 && v1 >= y0 && v1 <= y1
                && v2 >= y0 && v2 <= y1) {
                float srcx;
                float srcw;
                float dstw;
                float dstx;
                float srcy;
                float srch;
                float dsth;
                float dsty;

                tw = g_texsize[idB].w;
                th = g_texsize[idB].h;
                srcx = a->x;
                srcw = a->w;
                dstw = b->w;
                dstx = b->x;
                u0 = ((float)((float)((u0 - srcx) / srcw) * dstw) + dstx) / (float)tw;
                srcy = a->y;
                srch = a->h;
                dsth = b->h;
                dsty = b->y;
                v0 = ((float)((float)((v0 - srcy) / srch) * dsth) + dsty) / (float)th;
                u1 = ((float)((float)((u1 - srcx) / srcw) * dstw) + dstx) / (float)tw;
                v1 = ((float)((float)((v1 - srcy) / srch) * dsth) + dsty) / (float)th;
                u2 = ((float)((float)((u2 - srcx) / srcw) * dstw) + dstx) / (float)tw;
                v2 = ((float)((float)((v2 - srcy) / srch) * dsth) + dsty) / (float)th;
                p->tex = b->id + ctx->base;
                p->uv[0][0] = (*p_u0);
                p->uv[0][1] = (*p_v0);
                p->uv[1][0] = (*p_u1);
                p->uv[1][1] = (*p_v1);
                p->uv[2][0] = (*p_u2);
                p->uv[2][1] = (*p_v2);
            }
        }
        p++;
    } while (--i);
}
