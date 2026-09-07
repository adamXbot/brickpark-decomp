/* LEGOLAND — path-square route search and neighbour collection, the
 * object-class companions (dependent classes, Water Works origins, the
 * edit-object setter), the bloke list teardown, the visitor mood icon and
 * the RES master-directory registry.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Struct
 * field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are declared LOCALLY (legoland.h is
 * owned elsewhere); the PathSquare shape is pathsq.c's.
 *
 * Scope AI (docs/SCOPE_AI_midi_path.md).
 *
 * ---------------------------------------------------------------------------
 * PATH-SQUARE ROUTES
 *
 * CollectPathSquareNeighboursCounted is mappath.c's CollectPathSquareNeighbours
 * without the four NULL terminators: the same four edge scans (row above, row
 * below, column left, column right; a hit jumps the cursor to the square's far
 * edge so a square is recorded once), but the result is a dense array of
 * `g_path_square_neighbour_count` squares. It is the neighbour function of
 * the breadth-first search and of pathmask2.c's reachability flood.
 *
 * FindPathSquareRoute(from, to, next) is the BFS bnvmove.c's SuggestNextMove
 * calls. It expands from `to` (not `from`), one wave at a time, over two
 * 1020-slot wave buffers, marking each square it enqueues with visited bit 0
 * of PathSquare+0x20 (ClearPathSquareVisited, tinystubs.c, clears them). When
 * a neighbour of the square being expanded is `from`, that square — the
 * square adjacent to `from` on a shortest route towards `to` — is returned in
 * `*next` with 1. `from == to` returns `from` itself. 0 when `from` is
 * already marked visited on entry or the frontier empties. Nothing bounds a
 * wave at 1020 entries; the buffers are sized for the map.
 * ------------------------------------------------------------------------- */
#include <string.h>
#include "legoland.h"

#pragma intrinsic(strlen, strcpy)

/* ------------------------------------------------------------------ types -- */

/* One walkable rectangle of path (pathsq.c; 0x24 bytes). */
typedef struct PathSquare {
    struct PathSquare* next;      /* +0x00 */
    int                pad4;      /* +0x04 */
    Rect               rect;      /* +0x08 inclusive; .next unused */
    int                distance2; /* +0x1c */
    int                flags;     /* +0x20 bit 0 = visited */
} PathSquare;

/* The LLIDB element as the class loaders see it (saveprof.c's ClassElem).
 * type_flags bit 2 (0x4) = "loaded as an object class". */
typedef struct ClassElem {
    char*          name;        /* +0x00 */
    char*          image;       /* +0x04 */
    unsigned int   type_flags;  /* +0x08 */
    struct ObjDef* data;        /* +0x0c */
    unsigned int   refcount;    /* +0x10 */
} ClassElem;

/* The 0xd0-byte ODF class record; only the fields touched here are named. */
typedef struct ObjDef {
    unsigned char  pad00[0x0c];
    int            origin_x;    /* +0x0c  the class's base map square */
    int            origin_y;    /* +0x10 */
    unsigned char  pad14[0x10];
    unsigned char  origin_bx;   /* +0x24  byte copies of the origin */
    unsigned char  origin_by;   /* +0x25 */
    unsigned char  pad26[0x08];
    short          f2e;         /* +0x2e */
    unsigned char  pad30[0x0c];
    Rect           footprint;   /* +0x3c  what SetEditCursorFootPrint takes */
} ObjDef;

/* A ride class and the companion classes (track, water, roads) that must be
 * loaded with it: `members` is a ';'-terminated list of LLIDB names. */
typedef struct ClassGroup {
    const char* name;           /* +0x00 */
    const char* members;        /* +0x04 */
} ClassGroup;

/* A bloke, as the mood icon and the list teardown read it. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    unsigned char  pad04[0x08];
    unsigned short lt_action;   /* +0x0c  long-term plan (blokeai.c) */
    unsigned char  pad0e[0x6c];
    short          mood;        /* +0x7a */
} Bloke;

/* RES master directory bucket (audio3.c's MasterDir, 12 bytes). */
typedef struct MasterDir {
    struct MasterDir* next;     /* +0x00 */
    int               pad4;     /* +0x04 */
    char*             name;     /* +0x08 */
} MasterDir;

/* One member of a mounted volume (data2.c's RDirEnt). */
typedef struct RDirEnt {
    int              pad0;      /* +0x00 */
    struct RDirEnt*  next;      /* +0x04 */
    struct RDirEnt*  vnext;     /* +0x08  next member of the same volume */
    MasterDir*       dir;       /* +0x0c  the directory bucket it is filed under */
} RDirEnt;

/* A mounted volume (audio3.c's MasterVol): its member chain hangs at +0x04. */
typedef struct MasterVol {
    struct MasterVol* next;     /* +0x00 */
    RDirEnt*          members;  /* +0x04 */
    char              name[1];  /* +0x08 */
} MasterVol;

/* ---------------------------------------------------------------- globals -- */

extern PathSquare* g_path_squares;                 /* 0x0066b44c */
extern PathSquare* g_path_square_neighbours[];     /* 0x0066a45c */
extern int         g_path_square_neighbour_count;  /* 0x00669254 */

extern ClassGroup  g_class_groups[5];              /* 0x004bcdcc */
extern const char  kWaterWorksShower[];            /* 0x004bce90 "Water Works Shower" */
extern const char  kWaterWorksWaterBlock[];        /* 0x004b9828 "Water Works Water Block" */
extern const char  kWaterWorksElephantFountain[];  /* 0x004b9840 "Water Works Elephant Fountain" */

extern int         g_edit_changed;                 /* 0x008119b0 */
extern ObjDef*     g_edit_object;                  /* 0x008119b8 */
extern char        g_edit_cursor[];                /* 0x007febc0  the EditCursor block */

extern Pos         g_entrance_tile;                /* 0x0066b460 */
extern void*       g_entrance_elem;                /* 0x006661c4 */
extern int         g_entrance_tile_time;           /* 0x0066b468 */

extern int         g_mood_low;                     /* 0x0083292c */
extern int         g_mood_high;                    /* 0x00832934 */
extern int         g_mood_adjustments[13];         /* 0x0083293c */

extern Bloke*      g_people_head;                  /* 0x0066b574 */
extern Bloke*      g_bloke_base;                   /* 0x0066b57c */
extern int         g_visitor_count;                /* 0x006661bc */

extern MasterDir*  g_master_dirs;                  /* 0x00798624 */

/* ---------------------------------------------------------------- callees -- */

extern int         _stricmp(const char* a, const char* b);     /* 0x004aab90 (CRT) */
extern char*       strchr(const char* s, int c);               /* 0x004a0050 (CRT) */
extern void*       MemAlloc(unsigned int size);                /* 0x0049e4ff (malloc) */
extern void        MemFree(void* p);                           /* 0x0049e4d0 (free) */

extern PathSquare* FindPathSquare(Pos* pos);                   /* 0x00481790 */
extern ClassElem*  ElemID(const char* name);                   /* 0x0047b3f0 */
extern void*       LLIDB_LoadData(ClassElem* elem);            /* 0x0047d3a0 */
extern void        DefaultCursor(void* cursor);                /* 0x0045a390 */
extern void        SetEditCursorFootPrint(Rect* footprint);    /* 0x0045f440 */
extern int         GetGameTimer(void);                         /* 0x00499430 */
extern void        DestroyBloke(Bloke* b);                     /* 0x00483010 */
extern MasterVol*  GetMasterVolPtr(const char* name);          /* 0x00489510 */

/* ========================================================================== *
 *  Object classes                                                            *
 * ========================================================================== */

/* Load the companion classes of a ride class: for the group whose name is
 * `elem`'s, copy its member list, and for each ';'-terminated name look the
 * element up and LLIDB_LoadData it unless bit 2 says it is already loaded as
 * a class. The five groups (0x004bcdcc):
 *
 *   CASTLE OBJ          -> CASTLE_DUMMY;ROLLER COASTER TRACK;SQUARE_TRACK;
 *   LOG FLUME ENTRANCE  -> LOG FLUME TRACK;
 *   JUNGLE CRUISE       -> JUNGLE CRUISE WATER;
 *   DRIVING SCHOOL      -> DRIVING SCHOOL ROADS;ZEBRA CROSSING;
 *   BOATING SCHOOL      -> BOATING SCHOOL WATER;
 *
 * Only names followed by ';' are seen — a trailing unterminated name would be
 * skipped. Every group's list ends in ';'. The name saveprof.c gave it is
 * kept (one name per address); "sibling" = companion class.
 *
 * Levers: the group walk is an UNSIGNED index (`jb` against the table end,
 * cursor anchored on `.members`; a pointer walk anchors on `.name`, a signed
 * index compares `jl`). `tok = buf` must come BEFORE the strcpy: live across
 * the `rep movs` it can only sit in ebp, and with cursor/tok/p/e holding all
 * four callee-saved registers VC6 leaves the flag constant 4 as an immediate
 * (`test byte ptr [e+8],4` / `or al,4`). Assigned after the copy, tok shares
 * edi with `e` and the freed ebx carries a hoisted 4 (15 strict). */
// FUNCTION: LEGOLAND 0x004809d0
void LoadObjectClassSibling(ClassElem* elem)
{
    char         buf[256];
    unsigned int i;
    char*        tok;
    char*        p;
    ClassElem*   e;

    for (i = 0; i < 5; i++) {
        if (_stricmp(elem->name, g_class_groups[i].name) != 0)
            continue;
        tok = buf;
        strcpy(buf, g_class_groups[i].members);
        p = strchr(buf, ';');
        while (p) {
            *p = 0;
            e = ElemID(tok);
            if (e && !(e->type_flags & 4)) {
                if (LLIDB_LoadData(e))
                    e->type_flags |= 4;
            }
            tok = p + 1;
            *p = ';';
            p = strchr(tok, ';');
        }
    }
}

/* The three Water Works pieces carry no origin in their ODF: patch it in by
 * name after the class is loaded. The Water Block additionally gets +0x2e =
 * 10. */
// FUNCTION: LEGOLAND 0x00480aa0
void SetWaterWorksClassOrigins(ClassElem* elem, ObjDef* def)
{
    if (_stricmp(elem->name, kWaterWorksShower) == 0) {
        def->origin_x = 1;
        def->origin_y = 0;
        def->origin_bx = 1;
        def->origin_by = 0;
    } else if (_stricmp(elem->name, kWaterWorksWaterBlock) == 0) {
        def->origin_x = 1;
        def->origin_y = 0;
        def->origin_bx = 1;
        def->origin_by = 0;
        def->f2e = 10;
    } else if (_stricmp(elem->name, kWaterWorksElephantFountain) == 0) {
        def->origin_x = 6;
        def->origin_y = 2;
        def->origin_bx = 6;
        def->origin_by = 2;
    }
}

/* Make an element's class the one the edit cursor places: reset the cursor
 * and give it the class footprint. objmap.c's SetEditObject is the same
 * operation from the class record. The class must be a named local: written
 * inline, `g_edit_object = elem->data` loads into ecx and the post-call
 * reload lands in edx (5 strict); the named load reuses eax (RA01). The
 * footprint is read back through the GLOBAL after DefaultCursor (the original
 * reloads it). */
// FUNCTION: LEGOLAND 0x00480b70
void SetEditObjectFromElem(ClassElem* elem)
{
    ObjDef* d = elem->data;

    g_edit_changed = 1;
    g_edit_object = d;
    DefaultCursor(g_edit_cursor);
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

/* ========================================================================== *
 *  Path squares                                                              *
 * ========================================================================== */

// FUNCTION: LEGOLAND 0x00481720
PathSquare* GetPathSquareList(void)
{
    return g_path_squares;
}

/* Fill g_path_square_neighbours with the DISTINCT path squares bordering
 * `bounds` — row above, row below, column left, column right — as a dense
 * array of g_path_square_neighbour_count entries (no terminators). */
// FUNCTION: LEGOLAND 0x004819a0
void CollectPathSquareNeighboursCounted(Rect* bounds)
{
    PathSquare* square;
    Pos         pos;
    int         n = 0;

    g_path_square_neighbour_count = 0;

    pos.y = bounds->top - 1;
    for (pos.x = bounds->left; pos.x <= bounds->right; pos.x++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.x = square->rect.right;
            n++;
            g_path_square_neighbour_count++;
        }
    }

    pos.y = bounds->bottom + 1;
    for (pos.x = bounds->left; pos.x <= bounds->right; pos.x++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.x = square->rect.right;
            n++;
            g_path_square_neighbour_count++;
        }
    }

    pos.x = bounds->left - 1;
    for (pos.y = bounds->top; pos.y <= bounds->bottom; pos.y++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.y = square->rect.bottom;
            n++;
            g_path_square_neighbour_count++;
        }
    }

    pos.x = bounds->right + 1;
    for (pos.y = bounds->top; pos.y <= bounds->bottom; pos.y++) {
        square = FindPathSquare(&pos);
        g_path_square_neighbours[n] = square;
        if (square != 0) {
            pos.y = square->rect.bottom;
            n++;
            g_path_square_neighbour_count++;
        }
    }
}

/* Breadth-first search from `to` back towards `from` over the path-square
 * adjacency; see the file header.
 *
 * Lever: the wave loop is a TOP-tested `while (count != 0)` with count = 1
 * going in. VC6 folds the constant guard and bottom-tests it, and the loop's
 * fall-through IS the `return 0` epilogue, which the entry guard shares. A
 * `do { } while (n != 0)` computes the same thing but exiles that epilogue
 * past the found-it block behind a `je`/`jmp` pair (20 strict). */
// FUNCTION: LEGOLAND 0x00481f00
int FindPathSquareRoute(PathSquare* from, PathSquare* to, PathSquare** next)
{
    PathSquare*  wave_a[1020];
    PathSquare*  wave_b[1020];
    PathSquare** cur = wave_a;
    PathSquare** nxt = wave_b;
    PathSquare*  sq;
    int          count;
    int          n;
    int          i;
    int          j;

    if (from->flags & 1)
        return 0;
    if (from == to) {
        *next = from;
        return 1;
    }
    cur[0] = to;
    count = 1;
    while (count != 0) {
        n = 0;
        for (i = 0; i < count; i++) {
            CollectPathSquareNeighboursCounted(&cur[i]->rect);
            for (j = 0; j < g_path_square_neighbour_count; j++) {
                sq = g_path_square_neighbours[j];
                if (!(sq->flags & 1)) {
                    nxt[n++] = sq;
                    if (sq == from) {
                        *next = cur[i];
                        return 1;
                    }
                    sq->flags |= 1;
                }
            }
        }
        if (cur == wave_a) {
            cur = wave_b;
            nxt = wave_a;
        } else {
            cur = wave_a;
            nxt = wave_b;
        }
        count = n;
    }
    return 0;
}

/* ========================================================================== *
 *  Park entrance                                                             *
 * ========================================================================== */

/* Forget the cached entrance tile and element; UpdateEntranceTile (objdoor.c)
 * recomputes both when the tile's x is 0. screens3.c's teardown tails here. */
// FUNCTION: LEGOLAND 0x00482a80
void ResetEntranceTile(void)
{
    g_entrance_tile.x = 0;
    g_entrance_elem = 0;
}

// FUNCTION: LEGOLAND 0x00482b10
void ResetEntranceTileTime(void)
{
    g_entrance_tile_time = GetGameTimer();
}

/* ========================================================================== *
 *  Blokes                                                                    *
 * ========================================================================== */

/* The 1-based mood-icon index VisitorBubbleHelp draws beside a visitor's
 * name (gameframe.c, hit 0x306): the plan wins for three plans, otherwise
 * the mood against the two thresholds. */
// FUNCTION: LEGOLAND 0x00482cb0
int GetVisitorMoodIcon(Bloke* b)
{
    switch (b->lt_action) {
    case 3:
        return 4;
    case 11:
    case 12:
        return 1;
    case 13:
        return 5;
    }
    if (b->mood < g_mood_low)
        return 3;
    return b->mood < g_mood_high ? 10 : 2;
}

/* The default table AdjustMood (simcore2.c) scales; SetHappinessFactor
 * (eventgoalprim.c) overrides single entries from a level script. */
// FUNCTION: LEGOLAND 0x00482d70
void ResetMoodAdjustments(void)
{
    g_mood_adjustments[0]  = -100;
    g_mood_adjustments[1]  = -400;
    g_mood_adjustments[2]  = 33;
    g_mood_adjustments[3]  = 20;
    g_mood_adjustments[4]  = 5;
    g_mood_adjustments[5]  = 5;
    g_mood_adjustments[6]  = 5;
    g_mood_adjustments[7]  = -200;
    g_mood_adjustments[8]  = -1600;
    g_mood_adjustments[9]  = 400;
    g_mood_adjustments[10] = 100;
    g_mood_adjustments[11] = 400;
    g_mood_adjustments[12] = 100;
}

/* Free the bloke pool (blokeai.c's g_bloke_base) and forget the live list. */
// FUNCTION: LEGOLAND 0x00482ec0
void UnInitialiseBlokes(void)
{
    if (g_bloke_base)
        MemFree(g_bloke_base);
    g_people_head = 0;
    g_bloke_base = 0;
}

/* Destroy every live bloke (DestroyBloke unlinks the head) and zero the
 * visitor count. */
// FUNCTION: LEGOLAND 0x00483090
void DestroyAllBlokes(void)
{
    while (g_people_head)
        DestroyBloke(g_people_head);
    g_visitor_count = 0;
}

/* ========================================================================== *
 *  RES master directory                                                      *
 * ========================================================================== */

/* Find or create the master-directory bucket for `name` (case-insensitive;
 * GetMasterDirPtr in audio3.c is the find half). A new bucket copies the
 * name and is pushed on the front of the list. */
// FUNCTION: LEGOLAND 0x00489440
MasterDir* AddMasterDir(const char* name)
{
    MasterDir* d;

    for (d = g_master_dirs; d; d = d->next) {
        if (_stricmp(name, d->name) == 0)
            return d;
    }
    {
        MasterDir* nd = (MasterDir*)MemAlloc(sizeof(MasterDir));

        nd->name = (char*)MemAlloc(strlen(name) + 1);
        strcpy(nd->name, name);
        nd->next = g_master_dirs;
        nd->pad4 = 0;
        g_master_dirs = nd;
        return nd;
    }
}

/* Look a directory name up among the members of one volume: returns the
 * directory bucket (0 when the volume or the directory is unknown) and hands
 * back the member that matched in `*member`. data2.c's
 * RES_OpenFileFromVolume walks on from that member. */
// FUNCTION: LEGOLAND 0x00489550
MasterDir* RES_FindVolumeDir(const char* vol, const char* dir, RDirEnt** member)
{
    MasterVol* v = GetMasterVolPtr(vol);
    RDirEnt*   e;

    if (!v)
        return 0;
    for (e = v->members; e; e = e->vnext) {
        if (_stricmp(dir, e->dir->name) == 0) {
            *member = e;
            return e->dir;
        }
    }
    return 0;
}
