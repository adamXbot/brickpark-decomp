/* LEGOLAND — BNV path creation, move-line navigation and the random-walk
 * junction rules.
 *
 * Reconstructed C matched instruction-for-instruction against
 * original/legoland.exe with the VC6 SP3 toolchain (/O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; the field and type names are ours.
 * Each matched function carries its original VA marker on the line above it.
 *
 * ---------------------------------------------------------------------------
 * THE MOVE LINE
 *
 * A MoveLine is a 12-byte record {int x @0, int y @4, short dx @8, short dy
 * @0xa}: a 24.8 world position plus a per-step velocity in 1/256 units.
 * CalcMoveLine(x0,y0,x1,y1,line) aims it from tile (x0,y0) at tile (x1,y1):
 * the position is the CENTRE of the start tile ((t<<8)+0x80), the velocity is
 * the unit vector of atan2(y1-y0, x1-x0) scaled by 256 and rounded
 * (floor(v*256+0.5)), and the return value is that angle as a 0..255 byte
 * (angle * 256/2pi).  NavigMoveLine(line, speed, out) advances the line by
 * speed steps (x += dx*speed) and optionally reports the tile it is now on
 * (pos >> 8) in `out`.
 *
 * ---------------------------------------------------------------------------
 * THE VISITOR TICK (ControlPeople)
 *
 * Every tick: try to spawn a new visitor (0x0044ea50 — one every 30 ticks
 * while the park is under its visitor cap), then for every bloke on the
 * people chain: publish its +0x81 byte in 0x00813b08, print "Processing your
 * bloke" for the bloke the player is watching, run the leave-check
 * (0x0044eb50), and then the AI: a bloke with flags62 bit 0x20 (riding /
 * inside an attraction) only runs its low-level state machine; everybody else
 * first gets the per-tick bookkeeping (0x0044eae0), a tick counter at +0x5c
 * that every 16 ticks drops the mood by AdjustMood(b, 8, 1) and re-evaluates
 * the surroundings (0x00450530); a bloke whose low-level state is 0 (idle)
 * gets a new plan from DoHighLevelAI, and whoever now has a non-zero state
 * runs DoLowLevelAI.
 *
 * ---------------------------------------------------------------------------
 * NEXT-MOVE SUGGESTION (how a visitor picks where to go)
 *
 * SuggestNextMove(from, to, out) works on the PATH SQUARES (pathsq.c: the
 * rectangles paths are made of, list head 0x0066b44c).  Both points are
 * 24.8 world coordinates.  Return -2 when `from` is on no square, -1 when `to`
 * is on no square or no route exists, 2 when both are on the SAME square
 * (out = to, centred), otherwise 1 with `out` = the point on the NEXT square
 * of the route closest to `from`, clamped back into the current square when
 * the next square is more than sqrt(0x18000) ~ 1.5 tiles away, and centred
 * (+0x80).  The route search (0x00481f00) is a breadth-first walk over the
 * square neighbour table; bit 0 of PathSquare+0x20 is its visited mark.
 *
 * PTPSuggestNextMove(from, to, out) is the tile-level ("point to point")
 * flood fill used off the path squares: a breadth-first expansion over the
 * four tile neighbours (N, E, S, W in that order) using 16-byte open-list
 * nodes {next, parent, x, y} (head 0x0066b450, nodes-added-this-wave count
 * 0x00669250) and a 256x256-bit visited map (0x00669258).  When the target
 * tile is reached (0x0066b454), 0x00482430 walks the parent chain back into
 * the route list (head 0x0066b458); return 2 when it says the first step is
 * the current tile (out = from), else 1 with out = the centre of the first
 * route tile, and 0 when the fill dies out.
 *
 * DoRndWalkPathTileAction is the junction rule for a randomly walking
 * visitor, fired once per tile at the tile centre (tilehelp.c): the RF flags
 * of the tile decide — bit 8: "follow the path": take the exit that is not
 * the way we came (Bit_To_Dir of the exits minus the reverse heading); bits
 * 0x24: "junction": pick a random exit that is not the reverse heading or its
 * two neighbours; bit 0x10: "wander": a random exit, or a completely random
 * heading when the tile has none.  Each arm sets flags62 bit 4 (action done)
 * and returns NewDirForAction's result.  Off-map / off-path tiles do 0.
 *
 * ---------------------------------------------------------------------------
 * BNV PATHS
 *
 * NewBNVPath(bin, tag, name, near, far, z_base, pos) builds the 0x48-byte
 * BNVPath record bnvpath.c follows: vertical_scale = 49152/(near-far), start
 * position from `pos`, dframe 0, recalc 1, and person_height = GetZSkew of the
 * named object's first vertex in frame 0.  SetBlokePositionFromBNV(bin,
 * bloke, name, frame, near, far, extra) places a bloke straight onto a BNV
 * frame: it normalises the object's three orientation rows in place, sums the
 * eight vertices, writes slope (+0x34) and height (+0x38) into the 3D person,
 * the map cell (+0x3c/+0x3e = centroid/2) into the bloke and applies the
 * orientation.
 * ------------------------------------------------------------------------- */
#include "legoland.h"
#include <math.h>
#include <string.h>

#pragma intrinsic(strcpy)

/* ------------------------------------------------------------------ types -- */

/* A move line: 24.8 position plus a per-step velocity in 1/256 units. */
typedef struct MoveLine {
    int   x;        /* +0x00 world x, 24.8 */
    int   y;        /* +0x04 world y, 24.8 */
    short dx;       /* +0x08 per-step delta x (cos * 256) */
    short dy;       /* +0x0a per-step delta y (sin * 256) */
} MoveLine;

/* The 3D person fields written here. */
typedef struct Person3D {
    unsigned char pad00[0x34];
    int           slope;       /* +0x34 */
    float         height;      /* +0x38 */
} Person3D;

/* A bloke (visitor / worker), stride 172; only the fields touched here. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    Person3D*      person;      /* +0x04 */
    unsigned char  pad08[6];    /* +0x08..0x0d */
    unsigned short state;       /* +0x0e  low-level AI state */
    unsigned char  pad10[0x2c]; /* +0x10..0x3b */
    short          map_x;       /* +0x3c */
    short          map_y;       /* +0x3e */
    unsigned char  pad40[0x1c]; /* +0x40..0x5b */
    int            tick;        /* +0x5c  per-bloke tick counter */
    unsigned char  pad60[2];    /* +0x60..0x61 */
    unsigned short flags62;     /* +0x62  0x04 tile action done, 0x20 in ride */
    unsigned char  pad64[4];    /* +0x64..0x67 */
    int            wx;          /* +0x68  world x, 24.8 */
    int            wy;          /* +0x6c  world y, 24.8 */
    unsigned char  pad70[2];    /* +0x70..0x71 */
    unsigned char  dir;         /* +0x72  heading 0..7 */
    unsigned char  pad73[0x0e]; /* +0x73..0x80 */
    unsigned char  f81;         /* +0x81  published in g_cur_bloke_f81 */
    unsigned char  pad82[0x2a]; /* +0x82..0xab */
} Bloke;

/* BNV (binary vector animation) records — see bnvpath.c. */
typedef struct BNVVertex {
    short         x;             /* +0x00 */
    short         y;             /* +0x02 */
    float         z;             /* +0x04 */
    unsigned char pad08[12];
} BNVVertex;

typedef struct BNVNameNode {
    unsigned char       pad00[4];
    struct BNVNameNode* next;           /* +0x04 */
    BNVVertex*          vertices;       /* +0x08 */
    char*               name;           /* +0x0c */
    float               orientation[9]; /* +0x10..+0x30 */
} BNVNameNode;

typedef struct BNVBin BNVBin;
typedef struct BNVNameList BNVNameList;

typedef struct BNVPath {
    BNVBin*       bin;            /* +0x00 */
    int           tag;            /* +0x04 */
    char          object_name[20];/* +0x08 */
    float         vertical_scale; /* +0x1c */
    float         z_base;         /* +0x20 */
    float         x;              /* +0x24 */
    float         y;              /* +0x28 */
    float         pad2c;
    float         step_x;         /* +0x30 */
    float         step_y;         /* +0x34 */
    float         pad38;
    float         person_height;  /* +0x3c */
    int           dframe;         /* +0x40 */
    int           recalc;         /* +0x44 */
} BNVPath;

/* A path square (pathsq.c). */
typedef struct PathSquare {
    struct PathSquare* next;      /* +0x00 */
    int                pad4;      /* +0x04 */
    Rect               rect;      /* +0x08 left/top/right/bottom, next@+0x18 */
    int                distance2; /* +0x1c */
    int                flags;     /* +0x20 bit 0 = visited */
} PathSquare;

/* A point-to-point flood-fill node (16 bytes). */
typedef struct PTPNode {
    struct PTPNode* next;   /* +0x00 */
    struct PTPNode* parent; /* +0x04 */
    int             x;      /* +0x08 tile x */
    int             y;      /* +0x0c tile y */
} PTPNode;

/* ---------------------------------------------------------------- globals -- */

extern Bloke*   g_people_head;      /* 0x0066b574 */
extern int      g_cur_bloke_f81;    /* 0x00813b08 */
extern Pos      g_suggest_target;   /* 0x004bcec0 / 0x004bcec4  target tile */
extern int      g_ptp_wave_count;   /* 0x00669250 nodes added this wave */
extern PTPNode* g_ptp_open_head;    /* 0x0066b450 */
extern PTPNode* g_ptp_found;        /* 0x0066b454 */
extern PTPNode* g_ptp_route_head;   /* 0x0066b458 */

/* ------------------------------------------------------------ prototypes -- */

extern void*         MemAlloc(int size);                              /* 0x0049e4ff */
extern int           rand(void);                                      /* 0x0049e4b2 (CRT) */
extern void          DBPrintf(const char* fmt, ...);                  /* 0x00453a20 */
extern unsigned char GetCurrentRFFlags(int x, int y);                 /* 0x00461630 */
extern unsigned short Get_MapFlags(int x, int y);                     /* 0x00461760 */
extern unsigned char Get_Path_Directions(Pos* tile, int a, int b);    /* 0x0045c050 */
extern unsigned char ExcludeIsolatedDiags(unsigned char mask);        /* 0x0045c830 */
extern unsigned char Dir_To_Bit(unsigned char dir);                   /* 0x0045c010 */
extern unsigned char Bit_To_Dir(unsigned char mask);                  /* 0x0045c020 */
extern unsigned char Random_Dir_From_Bits(unsigned char mask);        /* 0x00483400 */
extern int           NewDirForAction(Bloke* b, unsigned char dir);    /* 0x004833d0 */
extern void          DoHighLevelAI(Bloke* b);                         /* 0x004504d0 */
extern void          DoLowLevelAI(Bloke* b);                          /* 0x00484920 */
extern int           AdjustMood(Bloke* b, int kind, int scale);       /* 0x00482df0 */

/* Unexported visitor-tick helpers. */
extern void SpawnVisitor(void);                  /* 0x0044ea50 */
extern int  IsWatchedBloke(Bloke* b);            /* 0x004700c0 */
extern void CheckBlokeLeaving(Bloke* b);         /* 0x0044eb50 */
extern void UpdateBlokeStay(Bloke* b);           /* 0x0044eae0 */
extern void ScanBlokeSurroundings(Bloke* b);     /* 0x00450530 */

/* Unexported path-square route helpers. */
extern PathSquare* FindPathSquareAt(Pos* world);                       /* 0x004817d0 */
extern void        ClearPathSquareVisited(void);                       /* 0x00481ee0 */
extern int         FindPathSquareRoute(PathSquare* from, PathSquare* to,
                                       PathSquare** next);             /* 0x00481f00 */
extern void        SetPathSquareDistance(Pos* from, PathSquare* sq);   /* 0x00481e60 */

/* Unexported point-to-point flood-fill helpers. */
extern void FreePTPOpenList(void);                    /* 0x004821e0 */
extern void FreePTPRouteList(void);                   /* 0x00482210 */
extern void ClearPTPVisited(void);                    /* 0x004821c0 */
#ifndef LEGOLAND_PORTABLE
extern int  AddPTPOpenNode(int x, int y, PTPNode* parent); /* 0x00482240 */
#else
extern void AddPTPOpenNode(int x, int y, PTPNode* parent); /* 0x00482240 */
#endif
extern int  BuildPTPRoute(void);                      /* 0x00482430 */

/* BNV accessors (bnvpath.c / math3d.c). */
extern BNVNameList* GetBinVFrame(BNVBin* bin, int frame);                   /* 0x0044dd70 */
extern BNVNameNode* GetObjectFromName(BNVNameList* list, const char* name); /* 0x0044dda0 */
extern BNVVertex*   GetVertex(BNVNameNode* object, int index);              /* 0x0044ddf0 */
extern float        GetZSkew(BNVBin* bin, BNVNameNode* object, BNVVertex* v); /* 0x0044de20 */
extern void         ApplyObjectOrientationToPerson(Person3D* person, float* orientation, int unused); /* 0x00484950 */

/* -------------------------------------------------------------- functions -- */

/* Advance a move line by `speed` steps; report its tile in `out` if given. */
// FUNCTION: LEGOLAND 0x004807f0
void NavigMoveLine(MoveLine* line, unsigned short speed, Pos* out)
{
    line->x += line->dx * speed;
    line->y += line->dy * speed;
    if (out) {
        out->x = line->x >> 8;
        out->y = line->y >> 8;
    }
}

/* Aim a move line from tile (x0,y0) at tile (x1,y1); returns the heading as
 * a 0..255 angle byte. */
// FUNCTION: LEGOLAND 0x00480740
int CalcMoveLine(Pos from, Pos to, MoveLine* line)
{
    /* Both tiles arrive BY VALUE (four argument slots): that is what lets
     * VC6 home the CSE'd angle in the dead `to` slots and keep a 4-byte frame
     * (push ecx) for the y delta; four int parameters give an 8-byte frame. */
    double angle = atan2((double)(to.y - from.y), (double)(to.x - from.x));

    line->dx = (short)(int)floor(cos(angle) * 256.0 + 0.5);
    line->dy = (short)(int)floor(sin(angle) * 256.0 + 0.5);
    line->x = (from.x << 8) + 0x80;
    line->y = (from.y << 8) + 0x80;
#ifndef LEGOLAND_PORTABLE
    return (int)(angle * 40.74366612653722);
#else
    /* PORT-M5: 0x00458930 rounds.  The two `floor(... + 0.5)` conversions
     * above are insensitive -- floor's result is already integral -- but this
     * one is not: it is the direction CODE every walker turns by. */
    return LL_FISTPD(angle * 40.74366612653722);
#endif
}

/* The per-tick visitor walker. */
// FUNCTION: LEGOLAND 0x00450990
void ControlPeople(void)
{
    Bloke* b;
    Bloke* next;

    SpawnVisitor();
    for (b = g_people_head; b; b = next) {
        next = b->next;
        g_cur_bloke_f81 = b->f81;
        if (IsWatchedBloke(b))
            DBPrintf("Processing your bloke\n");
        CheckBlokeLeaving(b);
        if (b->flags62 & 0x20) {
            /* In a ride: only the low-level state machine runs. */
            if (b->state != 0)
                DoLowLevelAI(b);
        } else {
            UpdateBlokeStay(b);
            b->tick++;
            if ((b->tick & 0xf) == 0) {
                AdjustMood(b, 8, 1);
                ScanBlokeSurroundings(b);
            }
            if (b->state == 0)
                DoHighLevelAI(b);
            if (b->state != 0)
                DoLowLevelAI(b);
        }
    }
}

/* Build a BNV path record for `name` in `bin`, starting at tile `pos`. */
// FUNCTION: LEGOLAND 0x00484c20
BNVPath* NewBNVPath(BNVBin* bin, int tag, const char* name, float near_z,
                    float far_z, Pos* pos)
{
    BNVPath*     path = (BNVPath*)MemAlloc(sizeof(BNVPath));
    BNVNameList* frame;
    BNVNameNode* object;
    BNVVertex*   vertex;

    path->bin = bin;
    strcpy(path->object_name, name);
    path->vertical_scale = 49152.0f / (near_z - far_z);
    path->dframe = 0;
    path->z_base = far_z;
    path->recalc = 1;
    path->x = (float)pos->x;
    path->y = (float)pos->y;
    frame = GetBinVFrame(bin, 0);
    object = GetObjectFromName(frame, name);
    vertex = GetVertex(object, 0);
    path->person_height = GetZSkew(bin, object, vertex);
    path->tag = tag;
    return path;
}

/* Tile-level breadth-first "point to point" next-move suggestion. */
// FUNCTION: LEGOLAND 0x004824d0
int PTPSuggestNextMove(Pos* from, Pos* to, Pos* out)
{
    int          sx = from->x >> 8;
    int          sy = from->y >> 8;
    int          tx = to->x >> 8;
    int          ty = to->y >> 8;
    PTPNode*     p;
    unsigned int n;

    FreePTPOpenList();
    FreePTPRouteList();
    ClearPTPVisited();
    g_ptp_wave_count = 0;
    g_ptp_found = 0;
    AddPTPOpenNode(sx, sy, 0);
    n = g_ptp_wave_count;
    while (n) {
        p = g_ptp_open_head;
        g_ptp_wave_count = 0;
        while (n-- != 0) {
            if (p->x == tx && p->y == ty) {
                g_ptp_found = p;
                if (BuildPTPRoute()) {
                    out->x = to->x;
                    out->y = to->y;
                    FreePTPOpenList();
                    FreePTPRouteList();
                    return 2;
                }
                out->x = (g_ptp_route_head->x << 8) + 0x80;
                out->y = (g_ptp_route_head->y << 8) + 0x80;
                FreePTPOpenList();
                FreePTPRouteList();
                return 1;
            }
            AddPTPOpenNode(p->x, p->y - 1, p);
            AddPTPOpenNode(p->x + 1, p->y, p);
            AddPTPOpenNode(p->x, p->y + 1, p);
            AddPTPOpenNode(p->x - 1, p->y, p);
            p = p->next;
        }
        n = g_ptp_wave_count;
    }
    FreePTPOpenList();
    FreePTPRouteList();
    return 0;
}

/* Path-square next-move suggestion (see the header comment). */
// FUNCTION: LEGOLAND 0x00482050
int SuggestNextMove(Pos* from, Pos* to, Pos* out)
{
    PathSquare* cur;
    PathSquare* dst;
    PathSquare* next;
    int left, top, right, bottom;

    g_suggest_target.x = to->x >> 8;
    g_suggest_target.y = to->y >> 8;
    cur = FindPathSquareAt(from);
    dst = FindPathSquareAt(to);
    if (!cur)
        return -2;
    if (!dst)
        return -1;
    if (cur == dst) {
        *out = *to;
        out->x += 0x80;
        out->y += 0x80;
        return 2;
    }
    ClearPathSquareVisited();
    if (!FindPathSquareRoute(cur, dst, &next))
        return -1;
    SetPathSquareDistance(from, next);

    left = next->rect.left << 8;
    right = next->rect.right << 8;
    top = next->rect.top << 8;
    bottom = next->rect.bottom << 8;
    if (from->x < left)
        out->x = left;
    else if (from->x > right)
        out->x = right;
    else
        out->x = from->x;
    if (from->y < top)
        out->y = top;
    else if (from->y > bottom)
        out->y = bottom;
    else
        out->y = from->y;

    if (next->distance2 > 0x18000) {
        left = cur->rect.left << 8;
        right = cur->rect.right << 8;
        top = cur->rect.top << 8;
        bottom = cur->rect.bottom << 8;
        if (out->x < left)
            out->x = left;
        else if (out->x > right)
            out->x = right;
        if (out->y < top)
            out->y = top;
        else if (out->y > bottom)
            out->y = bottom;
    }
    out->x += 0x80;
    out->y += 0x80;
    return 1;
}

/* Place a bloke straight onto BNV frame `frame` of object `name`. */
/* Exact.  The last four mismatches (the loop counter's `xor edi,edi` sitting
 * after the third row's fsqrt instead of before that row's last faddp) were
 * closed by the `(float)` casts on the nine multiply-backs below: VC6's
 * Pentium scheduler works on FIXED-SIZE WINDOWS OF IR TUPLES counted from the
 * FUNCTION START (about 81 here, straight across the two calls), and a root
 * instruction whose tuple falls in the second window is emitted at that
 * window's first stall slot.  The explicit double->float conversion tuple of
 * `x = (float)(x * inv)` produces no code but occupies a slot, so every such
 * cast ahead of the boundary moves the boundary one instruction up the FP
 * stream (three are needed; more than three change nothing, the xor stays in
 * the fmul->faddp stall at 0x484b36).  `x *= inv` and `x = x * inv` have no
 * such tuple.  Measured: k global stores or k casts anywhere before the
 * boundary (even before the first call) move the xor identically; padding
 * after the boundary never does; and a root written BEFORE the boundary is
 * hoisted to the first window's top (index 19, right after the flag-writing
 * `add esp,10h`), which is why no placement of `i = 0` could reach index 78.
 * The third row is still summed through a running float with its components
 * read into locals FIRST (z, y, x): that is what fixes the fld order at
 * 0x484ac6 and makes the accumulator live in orientation[8]'s x87 slot. */
// FUNCTION: LEGOLAND 0x00484a70
void SetBlokePositionFromBNV(BNVBin* bin, Bloke* bloke, const char* name,
                             int frame, float near_z, float far_z, int extra)
{
    BNVNameList* list;
    BNVNameNode* object;
    BNVVertex*   vertex;
    float        sum_z = 0.0f;
    int          sum_x = 0;
    int          sum_y = 0;
    float        inv;
    float        len2;
    float        cx;
    float        cy;
    float        cz;
    float        skew;
    float        delta_z;
    int          height;
    int          i;

    list = GetBinVFrame(bin, frame);
    object = GetObjectFromName(list, name);
    inv = 1.0f / (float)sqrt(object->orientation[0] * object->orientation[0] +
                             object->orientation[1] * object->orientation[1] +
                             object->orientation[2] * object->orientation[2]);
    object->orientation[0] = (float)(object->orientation[0] * inv);
    object->orientation[1] = (float)(object->orientation[1] * inv);
    object->orientation[2] = (float)(object->orientation[2] * inv);
    inv = 1.0f / (float)sqrt(object->orientation[3] * object->orientation[3] +
                             object->orientation[4] * object->orientation[4] +
                             object->orientation[5] * object->orientation[5]);
    object->orientation[3] = (float)(object->orientation[3] * inv);
    object->orientation[4] = (float)(object->orientation[4] * inv);
    object->orientation[5] = (float)(object->orientation[5] * inv);
    /* Running float sum, components read into locals first (z, y, x): see
     * the note above the marker.  Written as one expression the row compiles
     * to the first two rows' shape instead. */
    cz = object->orientation[8];
    cy = object->orientation[7];
    cx = object->orientation[6];
    len2 = cz * cz;
    len2 += cx * cx;
    len2 += cy * cy;
    inv = 1.0f / (float)sqrt(len2);
    object->orientation[6] = (float)(object->orientation[6] * inv);
    object->orientation[7] = (float)(object->orientation[7] * inv);
    object->orientation[8] = (float)(object->orientation[8] * inv);
    for (i = 0; i < 8; i++) {
        vertex = GetVertex(object, i);
        sum_x += vertex->x;
        sum_z += vertex->z;
        sum_y += vertex->y;
    }
    skew = GetZSkew(bin, object, vertex);
    /* The float cast sits on the average alone: it is what places the
     * fxch pair before the subtraction of far_z. */
    delta_z = (float)(sum_z * 0.125) - far_z;
#ifndef LEGOLAND_PORTABLE
    height = (int)(delta_z * (49152.0f / (near_z - far_z)) + 8192.0f);
#else
    height = LL_FISTP(delta_z * (49152.0f / (near_z - far_z)) + 8192.0f); /* PORT-M5 */
#endif
    bloke->person->slope = height >> 8;
    bloke->map_x = (short)(sum_x / 8 / 2);
    bloke->map_y = (short)(sum_y / 8 / 2);
    bloke->person->height = skew + skew;
    ApplyObjectOrientationToPerson(bloke->person, &object->orientation[0], extra);
}

/* Junction rule for a randomly walking visitor at a tile centre. */
// FUNCTION: LEGOLAND 0x00483920
int DoRndWalkPathTileAction(Bloke* w)
{
    Pos            tile;
    unsigned short rf;
    unsigned short rf2;
    unsigned short mapflags;
    unsigned char  bits;
    unsigned char  mask;
    unsigned char  dir;

    tile.x = w->wx >> 8;
    tile.y = w->wy >> 8;
    rf = GetCurrentRFFlags(w->wx, w->wy);
    if (w->wx >= 0 && w->wx < (g_map->width << 8) &&
        w->wy >= 0 && w->wy < (g_map->height << 8)) {
        mapflags = Get_MapFlags(w->wx, w->wy);
        rf2 = GetCurrentRFFlags(w->wx, w->wy);
        if ((rf2 & 1) || ((mapflags & 0x10) && !(rf2 & 2))) {
            if (rf & 8) {
                bits = Get_Path_Directions(&tile, 0, 0);
                mask = ExcludeIsolatedDiags(bits);
                mask &= ~Dir_To_Bit(w->dir + 4);
                dir = Bit_To_Dir(mask);
                w->flags62 |= 4;
                return NewDirForAction(w, dir);
            }
            if (rf & 0x24) {
                bits = Get_Path_Directions(&tile, 0, 0);
                bits = ExcludeIsolatedDiags(bits);
                /* The three-bit exclusion mask must be ONE expression: a
                 * named mask gives the AND a fresh temp instead of storing
                 * back into bits' home (the dead `w` argument slot). */
                bits &= ~(Dir_To_Bit(w->dir + 5) | Dir_To_Bit(w->dir + 4) |
                          Dir_To_Bit(w->dir + 3));
                dir = Random_Dir_From_Bits(bits);
                w->flags62 |= 4;
                return NewDirForAction(w, dir);
            }
            if (rf & 0x10) {
                bits = Get_Path_Directions(&tile, 0, 0);
                mask = ExcludeIsolatedDiags(bits);
                w->flags62 |= 4;
                if (mask)
                    dir = Bit_To_Dir(mask);
                else
                    dir = rand() & 7;
                return NewDirForAction(w, dir);
            }
        }
    }
    return 0;
}
