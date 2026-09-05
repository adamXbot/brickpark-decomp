/* LEGOLAND -- the DRIVING SCHOOL ROAD GRAPH: how a road block is created,
 * and how the route the cars follow is walked out of it.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere).  ridecb5.c/ridecb6.c own the placement callbacks and the record
 * list walkers; roads.c owns Road_SetTile, the tiler these two feed;
 * schoolcar.c/schoolcar2.c/schoolcar3.c/goldrush.c own the cars that drive
 * the result.
 *
 * =========================================================================
 * THE WHOLE MECHANISM, END TO END
 * =========================================================================
 * A driving school owns a set of ROAD BLOCKS.  A block is 4x4 map squares,
 * not one, and every block of every school lives on ONE global singly-linked
 * list at 0x004cbeac; a block belongs to the school whose packed map square
 * sits in its +0x08 field.
 *
 *   NewRoadRecord(school, x, y, shape, rot)      0x004132a0  (this file)
 *      allocates the 0x20-byte record, pushes it on that list, hands the
 *      (shape, rot) pair to roads.c's tiler, and clears the walk permission
 *      of the sixteen map cells the block covers.
 *
 *   DrivingSchool_ResetPaths(school)             0x00405310  (schoolcar.c)
 *      clears +0x04, +0x15 and +0x18 on every block of the school, seeds
 *      the frontier with the school's kind-6 ENTRANCE block, and then calls
 *
 *   DrivingSchool_WalkStep(school)               0x004051a0  (this file)
 *      until nothing is queued.  One call expands one breadth-first ply and
 *      writes, on every block it reaches, the block it was reached FROM
 *      (+0x18) and that block's distance from the entrance in blocks (+0x15).
 *
 * The +0x18 parent pointers ARE the route: they form a shortest-path tree
 * rooted at the entrance, and schoolcar2.c's SchoolCarNextManoeuvre reads
 * nothing else to decide whether a car turns left, goes straight on or turns
 * right.  Three consequences fall straight out of the walk and are worth
 * stating for anyone re-implementing it:
 *
 *   * The entrance is a ONE-WAY GATE.  On a kind-6 block the walk drops
 *     north, south and west before the ownership filter runs, so a lesson
 *     always leaves the school driving EAST.
 *   * The entrance keeps +0x18 == 0, so "no parent" and "on the entrance"
 *     are the same test -- which is exactly how the chooser detects a car
 *     sitting on the school.
 *   * A block is claimed by the FIRST ply that reaches it and is never
 *     re-parented, so the route never contains a cycle and every block's
 *     +0x15 is its true block distance from the entrance.
 * ========================================================================= */

/* ---- the road records --------------------------------------------------- */

/* A packed 2-byte map square passed BY VALUE, and its 16-bit view.  A
 * driving school is IDENTIFIED by the map square its building sits on, so
 * this doubles as the school id. */
typedef struct BPos  { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct CarPos { int x; int y; } CarPos;

/* One 4x4-map-square road block. */
typedef struct RoadRec {
    struct RoadRec* next;           /* +0x00  the global road list link */
    struct RoadRec* qnext;          /* +0x04  the path walk's frontier link */
    BPosW           school;         /* +0x08  the owning driving school */
    unsigned char   pad0a[2];
    CarPos          pos;            /* +0x0c  the block ORIGIN, a multiple of 4 */
    unsigned char   kind;           /* +0x14  shape | ((rot & 3) << 5) */
    unsigned char   f15;            /* +0x15  steps from the entrance block */
    unsigned char   pad16[2];
    struct RoadRec* prev;           /* +0x18  the block the path walk came from */
    unsigned char   f1c;            /* +0x1c */
    unsigned char   f1d;            /* +0x1d */
    unsigned char   pad1e[2];
} RoadRec;                          /* 0x20 */

extern void* MemAlloc(unsigned int n);                          /* 0x0049e4ff */

/* The head of the one global road-block list (every school's blocks share
 * it; a block is filed under its school by the +0x08 id). */
extern RoadRec* g_road_list;                                    /* 0x004cbeac */

/* roads.c's tiler: turns a (shape, rotation) pair into the block's sixteen
 * map tiles and re-stamps the block's own kind byte. */
extern void Road_SetTile(int x, int y, int shape, int rot);     /* 0x00412680 */
/* sweep2.c's per-cell user-flag store; the world coordinates are map
 * squares in 8.8. */
extern void Set_UserFlags(int x, int y, int value);             /* 0x00461730 */

/* The 4x4 block whose ORIGIN is exactly (x, y) (ridecb5.c owns it). */
extern RoadRec* Road_FindAt(int x, int y);                      /* 0x004125a0 */

/* The path walk's two frontier heads, both linked through +0x04: the blocks
 * being expanded this step, and the ones queued for the next. */
extern RoadRec* g_road_walk_cur;                                /* 0x004c11c4 */
extern RoadRec* g_road_walk_next;                               /* 0x004c11c8 */

/* =========================================================================
 * 0x004132a0 -- NewRoadRecord: create one 4x4 road block.
 *
 * Allocates a 0x20-byte record, fills the school id, the block ORIGIN and
 * the piece kind, pushes it on the FRONT of the one global road list, tiles
 * the block, and then CLEARS THE USER FLAGS of all sixteen map cells the
 * block covers -- (x + j, y + i) << 8 for i, j in 0..3, sixteen unrolled
 * calls, not a loop.  Clearing those flags is what stops blokes routing
 * across a road: sweep2.c's user-flag word is the per-cell walk permission,
 * and a road cell is walkable only where a zebra crossing puts it back.
 *
 * WHAT IS *NOT* INITIALISED IS LOAD-BEARING.  Only +0x00, +0x08, +0x0c,
 * +0x10, +0x14, +0x1c and +0x1d are written; +0x04 (the reset flag), +0x15
 * (the walk depth) and +0x18 (the path-walk parent) keep whatever the heap
 * hands back.  That is safe only because DrivingSchool_ResetPaths (0x00405310)
 * clears +0x04/+0x15/+0x18 on every record of the school before every walk,
 * and NewRoadRecord's callers (Roads_Add, Roads_Remove) always run it
 * immediately afterwards.  A record is therefore never legal to read between
 * creation and the next reset.
 *
 * LEVER: the two zero bytes and the list push are ONE reordering group.  The
 * original loads `g_road_list` BEFORE storing +0x1c and stores `next`
 * BETWEEN +0x1c and +0x1d.  Writing the source in the disassembly's apparent
 * order (`f1c = 0; next = g_road_list; f1d = 0;`) leaves the load one step
 * late (2 mismatches at indices 14-15); writing the LIST PUSH FIRST and the
 * two zero bytes after it is exact -- VC6 hoists the load and then sinks the
 * pointer store back between the two adjacent byte stores.  A named `head`
 * temporary for the load costs 19, and moving both byte stores ahead of the
 * push costs 4.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004132a0
void NewRoadRecord(BPosW school, int x, int y, int shape, int rot)
{
    RoadRec* r;

    r = (RoadRec*)MemAlloc(0x20);
    r->school = school;
    r->pos.x = x;
    r->pos.y = y;
    r->kind = (unsigned char)shape;
    r->next = g_road_list;
    r->f1c = 0;
    r->f1d = 0;
    g_road_list = r;

    Road_SetTile(x, y, shape, rot);

    Set_UserFlags(x << 8, y << 8, 0);
    Set_UserFlags((x + 1) << 8, y << 8, 0);
    Set_UserFlags((x + 2) << 8, y << 8, 0);
    Set_UserFlags((x + 3) << 8, y << 8, 0);
    Set_UserFlags(x << 8, (y + 1) << 8, 0);
    Set_UserFlags((x + 1) << 8, (y + 1) << 8, 0);
    Set_UserFlags((x + 2) << 8, (y + 1) << 8, 0);
    Set_UserFlags((x + 3) << 8, (y + 1) << 8, 0);
    Set_UserFlags(x << 8, (y + 2) << 8, 0);
    Set_UserFlags((x + 1) << 8, (y + 2) << 8, 0);
    Set_UserFlags((x + 2) << 8, (y + 2) << 8, 0);
    Set_UserFlags((x + 3) << 8, (y + 2) << 8, 0);
    Set_UserFlags(x << 8, (y + 3) << 8, 0);
    Set_UserFlags((x + 1) << 8, (y + 3) << 8, 0);
    Set_UserFlags((x + 2) << 8, (y + 3) << 8, 0);
    Set_UserFlags((x + 3) << 8, (y + 3) << 8, 0);
}

/* =========================================================================
 * 0x004051a0 -- DrivingSchool_WalkStep: one BREADTH-FIRST ply of the road
 * path walk.  This is the routine that builds the whole route graph the
 * driving-school cars follow.
 *
 * THE WALK.  DrivingSchool_ResetPaths (schoolcar.c, 0x00405310) clears +0x04,
 * +0x15 and +0x18 on every one of the school's blocks, seeds
 * g_road_walk_cur with the school's kind-6 ENTRANCE block and then calls this
 * routine until nothing more is queued.  One call drains the CURRENT frontier
 * (g_road_walk_cur, a list linked through +0x04) and builds the NEXT one
 * (g_road_walk_next, the same link):
 *
 *   for every block on the current frontier
 *     look up its four cardinal neighbours, one whole block (4 map squares)
 *       away: north (x, y-4), east (x+4, y), south (x, y+4), west (x-4, y)
 *     drop any neighbour that is missing, belongs to ANOTHER school, or has
 *       already been reached (+0x18 non-zero)
 *     for each survivor: +0x18 = the block we came from, +0x15 = our depth
 *       plus one
 *     push each survivor on the next frontier
 *
 * So +0x18 is a PARENT pointer -- following it from any block leads back to
 * the entrance -- and +0x15 is that block's distance from the entrance in
 * blocks.  A block is claimed by the FIRST ply that reaches it and never
 * re-parented, which is what makes the route a shortest-path tree; and the
 * entrance block itself keeps +0x18 == 0, which is the test
 * SchoolCarNextManoeuvre uses to notice a car sitting on the entrance.
 *
 * THE ENTRANCE IS A ONE-WAY GATE.  On a kind-6 block (`kind & 0xf`, since the
 * rotation lives in the top bits) north, south and west are dropped BEFORE
 * the filter runs, so the walk leaves the school going EAST and only east.
 * Every lesson therefore starts by driving east out of the entrance, which is
 * the fact SchoolCarNextManoeuvre relies on when it demands that the block
 * ahead be the one at `entrance.x + 4, entrance.y`.
 *
 * The four `if (p != 0 && (p->school.w != school.w || p->prev != 0)) p = 0;`
 * filters are the SAME idiom schoolcar2.c's SchoolCarNextManoeuvreHorn uses
 * on its three exits -- it transferred verbatim, including the short-circuit
 * spelling.  So did the four-way `if (p) { push }` chain: written as four
 * plain list pushes of a global head, VC6 caches the head in edi after the
 * first one (`mov edi,ebp / mov [g],edi`), which is exactly the original.
 *
 * The kind-6 arm is written as the `if` (it falls through inline) and the
 * north filter as the `else`; the else arm ends in an unconditional jump and
 * is duly exiled past the whole loop body, re-entering at two different
 * points depending on whether it still needs the school id loaded.  That is
 * the recorded block-exile IFF, unchanged.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004051a0
void DrivingSchool_WalkStep(BPosW school)
{
    RoadRec* cur;

    cur = g_road_walk_cur;
    while (cur) {
        RoadRec* n;
        RoadRec* e;
        RoadRec* s;
        RoadRec* w;

        n = Road_FindAt(cur->pos.x, cur->pos.y - 4);
        e = Road_FindAt(cur->pos.x + 4, cur->pos.y);
        s = Road_FindAt(cur->pos.x, cur->pos.y + 4);
        w = Road_FindAt(cur->pos.x - 4, cur->pos.y);

        if ((cur->kind & 0xf) == 6) {
            s = 0;
            w = 0;
            n = 0;
        } else {
            if (n != 0 && (n->school.w != school.w || n->prev != 0))
                n = 0;
        }
        if (e != 0 && (e->school.w != school.w || e->prev != 0))
            e = 0;
        if (s != 0 && (s->school.w != school.w || s->prev != 0))
            s = 0;
        if (w != 0 && (w->school.w != school.w || w->prev != 0))
            w = 0;

        if (n) {
            n->prev = cur;
            n->f15 = (unsigned char)(cur->f15 + 1);
        }
        if (e) {
            e->prev = cur;
            e->f15 = (unsigned char)(cur->f15 + 1);
        }
        if (s) {
            s->prev = cur;
            s->f15 = (unsigned char)(cur->f15 + 1);
        }
        if (w) {
            w->prev = cur;
            w->f15 = (unsigned char)(cur->f15 + 1);
        }

        if (n) {
            n->qnext = g_road_walk_next;
            g_road_walk_next = n;
        }
        if (e) {
            e->qnext = g_road_walk_next;
            g_road_walk_next = e;
        }
        if (s) {
            s->qnext = g_road_walk_next;
            g_road_walk_next = s;
        }
        if (w) {
            w->qnext = g_road_walk_next;
            g_road_walk_next = w;
        }

        cur = cur->qnext;
    }
}
