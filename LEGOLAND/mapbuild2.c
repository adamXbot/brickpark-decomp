/* LEGOLAND -- scope Q: the ten-second build tally the script-event tick runs
 * over the path squares (how much of every square's perimeter is blocked,
 * and how much of that by special objects), and four bodies the linker kept
 * that nothing live calls: the tally's clear, an object-door tile probe and
 * its step, and a tile-grid debug overlay.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-q.md.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct MapHdr { char pad00[0x14]; unsigned short w, h; } MapHdr;
typedef struct PathSquare {           /* the path-square list, 0x0066b44c */
    struct PathSquare* next;          /* +0x00 */
    int pad04;
    int x0, y0, x1, y1;               /* +0x08..+0x14  the square's rect, inclusive */
} PathSquare;
typedef struct ObjDef { char pad00[0xc]; Pos door; } ObjDef;   /* +0x0c/+0x10 door offset */
typedef struct MapInst { char pad00[0xc]; ObjDef* cls; } MapInst;
typedef struct MapObj { char pad00[0xc]; ObjDef* cls; } MapObj;
typedef struct Cell { MapObj* obj; char pad04[8]; unsigned short flags; char pad0e[6]; } Cell;   /* 0x14 bytes */

extern int   g_tally_blocked;      /* 0x00667d00  perimeter cells still counted as blocked (first named here) */
extern int   g_tally_special;      /* 0x00667d04  perimeter cells holding a type 2/3 object */
extern int   g_tally_percent;      /* 0x00667d08  special * 100 / blocked */
extern int   g_have_special;       /* 0x00667d0c */
extern int   g_build_timer;        /* 0x00667d10  last tally timestamp */
extern MapHdr* g_map;              /* 0x004bcbf4 */
extern Cell**  g_map_rows;         /* 0x00801400 */
extern void*   g_env_class;        /* 0x007fd624  the environment object class */

extern int  GetGameTimer(void);                                        /* 0x00499430 */
extern PathSquare* GetPathSquareList(void);                            /* 0x00481720  returns g_path_squares (first named here) */
extern void TallyFootprintCell(Pos* pos, int* blocked, int* special);  /* 0x004598d0 */
extern int  ObjHasEntrance(ObjDef* d);                                 /* 0x0045e620 */
extern int  ObjHasExit(ObjDef* d);                                     /* 0x0045e690 */

/* WIP. audit: 116i/344B both; matchfull 110/116. ONE scheduling residual, in
 * both `pt` loops' latches: after the second TallyFootprintCell the original
 * reloads pt.x, loads sq->x1, cleans up, increments, compares and only then
 * stores pt.x back (`mov eax,[pt.x] / mov ecx,[esi+0x10] / add esp,0x18 /
 * inc / cmp / mov [pt.x],eax / jle`); ours stores the increment before the
 * bound load. Measured identical: `pt.x++`, `++pt.x`, `pt.x += 1`,
 * `pt.x = pt.x + 1`, a while form, `int pt[2]`, the compare reversed
 * (`sq->x1 >= pt.x` changes the jump sense, worse); a separate `int x`
 * counter with `pt.x = x` at the top of the body moves x into a
 * callee-saved register and costs 50. strict == rb == ob: a permutation of
 * one store and one load. */
// WIP-FUNCTION: LEGOLAND 0x00459970  (94.8%, both loop latches: the original sinks the pt.x store below the compare and hoists the sq->x1 load above the cleanup; ours stores first)
void TallyBuildFootprints(void)
{
    int         blocked, special;
    Pos         pt;
    PathSquare* sq;
    int         now;

    now = GetGameTimer();
    if (now - g_build_timer <= 10000)
        return;
    g_build_timer = now;
    g_have_special = 0;
    special = 0;
    blocked = 0;
    for (sq = GetPathSquareList(); sq; sq = sq->next) {
        blocked += 2 * (sq->x1 - sq->x0 + sq->y1 - sq->y0) + 4;
        for (pt.x = sq->x0; pt.x <= sq->x1; pt.x++) {
            pt.y = sq->y0 - 1;
            TallyFootprintCell(&pt, &blocked, &special);
            pt.y = sq->y1 + 1;
            TallyFootprintCell(&pt, &blocked, &special);
        }
        for (pt.y = sq->y0; pt.y <= sq->y1; pt.y++) {
            pt.x = sq->x0 - 1;
            TallyFootprintCell(&pt, &blocked, &special);
            pt.x = sq->x1 + 1;
            TallyFootprintCell(&pt, &blocked, &special);
        }
    }
    g_tally_blocked = blocked;
    g_tally_special = special;
    if (blocked)
        g_tally_percent = special * 100 / blocked;
    else
        g_tally_percent = 0;
}

/* Dead: nothing live names it (tools/inventory.py). */
// FUNCTION: LEGOLAND 0x004598b0
void ClearBuildTally(void)
{
    g_tally_special = 0;
    g_tally_blocked = 0;
    g_tally_percent = 0;
    g_have_special = 0;
}

/* Dead. 1 when the cell may be stepped through as a door tile: either it has
 * no object of the door-blocking kinds, or the object is the environment
 * class's. */
// FUNCTION: LEGOLAND 0x0045e930
int DoorTileStep(Cell* c)
{
    if (!c)
        return (int)c;
    if ((c->flags & 0x8a8) && c->obj->cls != g_env_class)
        return 0;
    return 1;
}

static __inline Cell* MapCellAt(int x, int y)
{
    if (x >= 0 && x < g_map->w && y >= 0 && y < g_map->h)
        return &g_map_rows[y][x];
    return 0;
}

/* Dead. 0 when the object's entrance or exit tile (its door offset from
 * pos) is on the map and blocked; 1 otherwise. */
/* WIP (dead code). audit: 89i/221B both; matchfull 83/89. Structure and
 * every test match (the exit check IS nested inside the entrance check: a
 * class without an entrance skips both). The residual is register choice
 * for the door-offset copy: the original loads d.y into ebx in the entrance
 * block and d.x/d.y into ebx/edi in the exit block (pos's edi freed by its
 * last read); ours uses edx and ecx/esi. rb == 0 by inspection. Spellings
 * measured: separate `dx`/`dy` temporaries (79), a Pos copy in the entrance
 * block only (69), Pos copies in both (82), then nesting (83).
 * Scope LL17 (2026-09-08): closed, 89/89.  The residual was the `Pos d`
 * copy itself: reading `cls->door.x` / `cls->door.y` directly in the sums
 * (no local aggregate, no scalars, no pointer to the door) gives the
 * original's callee-saved choices in both blocks (RA02 -- a named copy and
 * a direct field expression are different webs).  Either operand order of
 * the sums is exact; `Pos* dp = &cls->door` is 80, block-scope dx/dy
 * scalars 84, two separate Pos copies 83, y before x 73. */
// FUNCTION: LEGOLAND 0x0045e960
int FindObjDoorTile(MapInst* inst, Pos* pos)
{
    ObjDef* cls = inst->cls;
    int   x, y, r;
    Cell* cell;

    if (ObjHasEntrance(cls)) {
        x = pos->x + cls->door.x;
        y = pos->y + cls->door.y;
        cell = MapCellAt(x, y);
        if (cell) {
            r = DoorTileStep(cell);
            if (!r)
                return r;
        }
        if (ObjHasExit(cls)) {
            x = pos->x + cls->door.x;
            y = pos->y + cls->door.y;
            cell = MapCellAt(x, y);
            if (cell) {
                r = DoorTileStep(cell);
                if (!r)
                    return r;
            }
        }
    }
    return 1;
}

/* 0x0045ade0 DrawTileDebugOverlay (291 instructions, DEAD: nothing live names
 * it) was decoded but not attempted in this scope. Shape for whoever takes
 * it: SetClipping to the map header's rect (+0x20/+0x22 size, +0x00/+0x02
 * origin), scroll globals 0x00667ca4/0x00667cb4/0x00667cb8 with the tile
 * size from the 0x00805f60 sprite table entry, a four-way jump-table
 * switch on a computed quadrant (setge / +2 / -1 / cmp 3 / ja), then two
 * nested loops over the visible diamond that copy each in-range cell
 * (0x14 bytes, rep movsd) into a local, test its flags byte (+0x10) for 2,
 * and PrintSprite the 0x00805f60 entry chosen by the 0x00805f48 byte plus
 * *0x00801a6c at colour 0xff6868. Frame 0x50, four pushes. */
