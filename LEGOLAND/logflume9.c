/* LEGOLAND -- the LOG FLUME, part 9: the shared drop / track tick helpers
 * (inventory group 2).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow logflume.c
 * / logflume2.c.  Neighbour-helper names (LFGeom_ProbeNeighbours, LFNb_*)
 * are the ones scope LL1 committed in logflume8.c.
 *
 *   0x0040d420  LFGeom_ApplyCursors
 *   0x0040d520  LFTrack_CommitPlacement
 *   0x0040d6f0  LFPiece_UpdateCommon
 *   0x0040d900  LFPiece_AddCommon
 *   0x0040da10  LFTrack_UnlinkNeighbours
 *   0x0040db00  LFPiece_RemoveCommon
 */

typedef struct Pos { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct FootPart FootPart;
typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;                    /* 0x14 */

typedef struct Rect {
    int left;
    int top;
    int right;
    int bottom;
} Rect;

typedef struct RideDef {
    unsigned char pad00[0x3c];
    Footprint     footprint;    /* +0x3c */
} RideDef;

typedef struct RideElem {
    char*     name;
    char*     image;
    unsigned int flags;
    RideDef*  data;             /* +0x0c */
} RideElem;

typedef struct LFPiece LFPiece;
typedef struct LFRun   LFRun;

struct LFPiece {
    LFPiece*      next;         /* +0x00 */
    LFPiece*      prev;         /* +0x04 */
    LFPiece*      fwd;          /* +0x08 */
    LFPiece*      back;         /* +0x0c */
    unsigned int  flags;        /* +0x10 */
    BPosW         sq;           /* +0x14 */
    unsigned char pad16[2];
    int           kind;         /* +0x18 */
    int           dir;          /* +0x1c */
    RideDef*      def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c  head of this piece's sub-list */
    LFPiece*      end_a;        /* +0x30 */
    LFPiece*      end_b;        /* +0x34 */
};

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned char pad04[0xd4 - 0x04];
};

typedef struct LFGeom {
    int dirs;                   /* +0x00  N=1 E=2 S=4 W=8 */
    Pos pt[4];                  /* +0x04 */
} LFGeom;                       /* 0x24 */

typedef struct EditCursorRec {
    unsigned char pad0000[0x1404];
    int           x;            /* +0x1404 */
    int           y;            /* +0x1408 */
    unsigned char pad140c[8];
    Footprint     footprint;    /* +0x1414 */
    unsigned char pad1428[0x1828 - 0x1428];
    int           f1828;        /* +0x1828 */
    unsigned char pad182c[4];
    struct EditCursorRec* next; /* +0x1830 */
} EditCursorRec;

/* ---- module globals ---------------------------------------------------- */
extern Footprint     g_lf_footprint;        /* 0x004b4728 */
extern EditCursorRec g_edit_cursor;         /* 0x007febc0 */
extern Pos           g_mapref;              /* 0x007fffc4  == edit origin */
extern void*         g_8003f0;              /* 0x008003f0  == edit next */
extern EditCursorRec g_lf_geom_cursor_a;    /* 0x004c4468 */
extern EditCursorRec g_lf_geom_cursor_b;    /* 0x004c5ca0 */
extern EditCursorRec g_lf_place_cursor_a;   /* 0x004ca5b0 */
extern EditCursorRec g_lf_place_cursor_b;   /* 0x004c1260 */
extern int           g_lf_commit_a;         /* 0x004cbde0 */
extern int           g_lf_commit_b;         /* 0x004c2a90 */

/* ---- engine ------------------------------------------------------------ */
#ifndef LEGOLAND_PORTABLE
extern void ScreenToMapRef(int screen, Pos* out, int mode);      /* 0x0045be90 */
#else
extern int ScreenToMapRef(int screen, Pos* out, int mode);      /* 0x0045be90 */
#endif
extern void ResetCursorFootprint(EditCursorRec* c);              /* 0x0045f460 */
extern void SetCursorError(EditCursorRec* c, int code);          /* 0x0045f480 */
extern int  CursorIsValid(EditCursorRec* c);                     /* 0x0045f4b0 */
extern void PropagateCursorStatus(EditCursorRec* c);             /* 0x0045f4d0 */
extern void ValidateCursor(EditCursorRec* c, RideDef* def);      /* 0x0045f810 */
extern int  GetObjCost(RideDef* def);                            /* 0x00480da0 */
extern int  GetBrickCount(void);                                 /* 0x004578e0 */
extern int  CheckForPeople(const Rect* r);                       /* 0x00485260 */
extern void AddBasicObject(RideElem* elem, const Pos* p);        /* 0x0045efe0 */
extern void StandardRemoveObject(void* a, BPosW sq, void* c);    /* 0x0045f220 */

/* ---- log-flume (matched elsewhere) ------------------------------------ */
extern LFPiece* LFPiece_FindAt(const BPosW* sq);                 /* 0x00408ef0 */
extern LFPiece* LFPiece_Alloc(void);                             /* 0x00409010 */
extern void     LFRun_AddPiece(LFRun* run, LFPiece* piece);      /* 0x004091f0 */
extern void     LFRun_RemovePiece(LFRun* run, LFPiece* piece);   /* 0x00409270 */
extern int      LFTrack_NeighbourMask(LFPiece** nb);             /* 0x00409410 */
extern void     LFPiece_QueryRect(LFPiece* p, Footprint** out_fp,
                                  BPos* out_sq);                 /* 0x0040d090 */
extern void     LFRun_AddCount(LFRun* run, int delta);           /* 0x004119a0 */

/* scope LL1 (logflume8.c) -- one name per address */
extern void     LFGeom_ProbeNeighbours(void* a, LFPiece*** out); /* 0x0040cf10 */
extern int      LFNb_Count(LFPiece** nb);                        /* 0x0040cf30 */
extern void     LFNb_KeepRun(LFRun* run, LFPiece** nb);          /* 0x0040cf50 */
extern LFRun*   LFNb_FirstRun(LFPiece** nb);                     /* 0x0040cf80 */
extern void     LFNb_DropFull(LFPiece** nb);                     /* 0x0040cfa0 */
extern void     LFTrack_RedrawNeighbours(LFPiece* piece,
                                         LFPiece** nb);          /* 0x0040a2a0 */

/* still unmatched in LL1; named from the AddCommon call sites */
extern void     LFTrack_ReshapeEnds(LFPiece** ends, LFPiece** nb); /* 0x00409a90 */
extern void     LFTrack_LinkEnds(LFPiece** ends, LFPiece** nb);    /* 0x0040a080 */

/* =========================================================================
 * 0x0040da10 -- DROP NEIGHBOURS THAT ARE NOT ACTUALLY LINKED TO THIS PIECE.
 *
 * Slot order is N, E, W, S (0, 1, 3, 2).  A plain piece (sub == 0) stays
 * linked only if the neighbour's fwd or back points at it; a compound
 * piece stays linked if the neighbour's fwd/back is one of its two ends.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040da10
void LFTrack_UnlinkNeighbours(LFPiece* piece, LFPiece** nb)
{
    LFPiece* n;
    LFPiece* fwd;
    LFPiece* back;
    LFPiece* end_a;
    LFPiece* end_b;

    n = nb[0];
    if (n) {
        if (piece->sub == 0) {
            if (n->fwd != piece && n->back != piece)
                nb[0] = 0;
        } else {
            fwd = n->fwd;
            end_a = piece->end_a;
            if (fwd != end_a) {
                back = n->back;
                if (back != end_a) {
                    end_b = piece->end_b;
                    if (fwd != end_b && back != end_b)
                        nb[0] = 0;
                }
            }
        }
    }
    n = nb[1];
    if (n) {
        if (piece->sub == 0) {
            if (n->fwd != piece && n->back != piece)
                nb[1] = 0;
        } else {
            fwd = n->fwd;
            end_a = piece->end_a;
            if (fwd != end_a) {
                back = n->back;
                if (back != end_a) {
                    end_b = piece->end_b;
                    if (fwd != end_b && back != end_b)
                        nb[1] = 0;
                }
            }
        }
    }
    n = nb[3];
    if (n) {
        if (piece->sub == 0) {
            if (n->fwd != piece && n->back != piece)
                nb[3] = 0;
        } else {
            fwd = n->fwd;
            end_a = piece->end_a;
            if (fwd != end_a) {
                back = n->back;
                if (back != end_a) {
                    end_b = piece->end_b;
                    if (fwd != end_b && back != end_b)
                        nb[3] = 0;
                }
            }
        }
    }
    n = nb[2];
    if (n) {
        if (piece->sub == 0) {
            if (n->fwd != piece && n->back != piece) {
                nb[2] = 0;
                return;
            }
        } else {
            fwd = n->fwd;
            end_a = piece->end_a;
            if (fwd != end_a) {
                back = n->back;
                if (back != end_a) {
                    end_b = piece->end_b;
                    if (fwd != end_b && back != end_b)
                        nb[2] = 0;
                }
            }
        }
    }
}

/* =========================================================================
 * 0x0040d420 -- STAMP UP TO TWO GEOM POINTS ONTO THE PAIR OF PREVIEW
 * CURSORS.  Both cursors get the shrunk flume-cell footprint; A chains to
 * B.  dirs bits 1/2/4/8 pick N/E/S/W; the loop body runs twice so a two-
 * ended piece lights both cursors.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040d420
void LFGeom_ApplyCursors(LFGeom* g)
{
    int            flags = g->dirs;
    EditCursorRec* c = 0;
    int            n = 2;

    g_lf_geom_cursor_a.next = &g_lf_geom_cursor_b;
    g_lf_geom_cursor_a.footprint = g_lf_footprint;
    g_lf_geom_cursor_a.footprint.v[2] = g_lf_footprint.v[2] - 1;
    g_lf_geom_cursor_a.footprint.v[3] = g_lf_geom_cursor_a.footprint.v[3] - 1;
    g_lf_geom_cursor_b.next = 0;
    g_lf_geom_cursor_b.footprint = g_lf_geom_cursor_a.footprint;

    do {
        if (c == 0)
            c = &g_lf_geom_cursor_a;
        else
            c = &g_lf_geom_cursor_b;
        if (flags & 1) {
            c->x = g->pt[0].x;
            c->y = g->pt[0].y;
            flags &= ~1;
        } else if (flags & 2) {
            c->x = g->pt[1].x;
            c->y = g->pt[1].y;
            flags &= ~2;
        } else if (flags & 4) {
            c->x = g->pt[2].x;
            c->y = g->pt[2].y;
            flags &= ~4;
        } else if (flags & 8) {
            c->x = g->pt[3].x;
            c->y = g->pt[3].y;
            flags &= ~8;
        }
        ResetCursorFootprint(c);
    } while (--n);
}

/* =========================================================================
 * 0x0040db00 -- SHARED REMOVE.  Find the piece, drop the run count by 3
 * (set pieces occupy three units), stamp the queried rectangle onto the
 * caller's cursor, StandardRemoveObject, then unlink/redraw/remove.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040db00
void LFPiece_RemoveCommon(void* a, BPosW sq, void* c,
                          void (*geom)(BPos sq, LFGeom* out))
{
    LFPiece*       piece;
    Footprint*     fp;
    BPos           qsq;
    LFGeom         g;
    LFPiece**      nb;
    EditCursorRec* cur = (EditCursorRec*)c;

    piece = LFPiece_FindAt(&sq);
    if (!piece)
        return;
    LFRun_AddCount(piece->run, -3);
    LFPiece_QueryRect(piece, &fp, &qsq);
    cur->footprint = *fp;
    StandardRemoveObject(a, *(BPosW*)&qsq, c);
    geom(qsq, &g);
    LFGeom_ProbeNeighbours(&g, &nb);
    LFTrack_UnlinkNeighbours(piece, nb);
    LFTrack_RedrawNeighbours(piece, nb);
    LFRun_RemovePiece(piece->run, piece);
}

static __inline void LFAdd_StampAndPlace(Pos* p, unsigned int x, int y,
                                         RideDef* def, Footprint* f,
                                         RideElem* e)
{
    p->x = (int)x;
    p->y = y;
    def->footprint = *f;
    AddBasicObject(e, p);
}

static __inline void LFCommit_Stamp(EditCursorRec* c, int x, int y,
                                    Footprint* fp)
{
    c->x = x;
    c->y = y;
    c->footprint = *fp;
}

/* =========================================================================
 * 0x0040d900 -- SHARED ADD.  Allocate, stamp the square, run the class
 * geom/probe/place/shape callbacks, join the first neighbour's run (count
 * +3) and AddBasicObject.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040d900
void LFPiece_AddCommon(unsigned int sq, Footprint* fp, RideElem* elem,
                       void (*place)(LFPiece* p),
                       void (*geom)(unsigned int sq, LFGeom* out),
                       void (*shape)(LFPiece* p, LFPiece** ends))
{
    LFPiece*  piece;
    LFGeom    g;
    LFPiece** nb;
    LFRun*    run;
    LFPiece*  ends[4];
    RideDef*  def;
    int       py;

    piece = LFPiece_Alloc();
    if (!piece)
        return;
    piece->sq.b.x = (unsigned char)sq;
    piece->sq.b.y = *((unsigned char*)&sq + 1);
    piece->def = elem->data;
    piece->f28 = 0;
    geom(sq, &g);
    LFGeom_ProbeNeighbours(&g, &nb);
    LFNb_DropFull(nb);
    run = LFNb_FirstRun(nb);
    LFRun_AddCount(run, 3);
    piece->run = run;
    LFNb_KeepRun(run, nb);
    place(piece);
    LFRun_AddPiece(run, piece);
    py = *(int*)((char*)&sq + 1);
    def = elem->data;
    py &= 0xff;
    LFAdd_StampAndPlace((Pos*)ends, sq &= 0xff, py, def, fp, elem);
    shape(piece, ends);
    LFTrack_LinkEnds(ends, nb);
    LFTrack_ReshapeEnds(ends, nb);
}

/* =========================================================================
 * 0x0040d520 -- PAINT UP TO TWO NEIGHBOUR RECTANGLES ONTO THE PLACEMENT
 * CURSOR CHAIN.  The incoming cursor links to place-cursor A; a second
 * occupied slot chains A to B.  East stores y before x.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040d520
void LFTrack_CommitPlacement(LFPiece** nb, EditCursorRec* c)
{
    int            mask;
    int            i;
    EditCursorRec* cur;
    Footprint*     fp0;
    Footprint*     fp1;
    Footprint*     fp2;
    Footprint*     fp3;
    BPos           sq0;
    BPos           sq1;
    BPos           sq2;
    BPos           sq3;

    cur = 0;
    mask = LFTrack_NeighbourMask(nb);
    g_lf_commit_a = (int)cur;
    g_lf_commit_b = (int)cur;
    c->next = &g_lf_place_cursor_a;
    i = (int)cur;
    do {
        if (cur == 0)
            cur = &g_lf_place_cursor_a;
        else {
            cur->next = &g_lf_place_cursor_b;
            cur = &g_lf_place_cursor_b;
        }
        if (mask & 1) {
            LFPiece_QueryRect(nb[0], &fp0, &sq0);
            cur->x = *(int*)&sq0 & 0xff;
            cur->y = *(int*)((char*)&sq0 + 1) & 0xff;
            cur->footprint = *fp0;
            mask &= ~1;
        } else if (mask & 4) {
            LFPiece_QueryRect(nb[1], &fp1, &sq1);
            LFCommit_Stamp(cur, *(int*)&sq1 & 0xff,
                           *(int*)((char*)&sq1 + 1) & 0xff, fp1);
            mask &= ~4;
        } else if (mask & 0x10) {
            LFPiece_QueryRect(nb[2], &fp2, &sq2);
            cur->x = *(int*)&sq2 & 0xff;
            cur->y = *(int*)((char*)&sq2 + 1) & 0xff;
            cur->footprint = *fp2;
            mask &= ~0x10;
        } else if (mask & 0x40) {
            LFPiece_QueryRect(nb[3], &fp3, &sq3);
            cur->x = *(int*)&sq3 & 0xff;
            cur->y = *(int*)((char*)&sq3 + 1) & 0xff;
            cur->footprint = *fp3;
            mask &= ~0x40;
        }
        cur->f1828 = 0x2010;
        ResetCursorFootprint(cur);
        if (mask == 0)
            break;
        i++;
    } while (i < 2);
}

static __inline void LFUpd_PackSq(unsigned int* slot, int x, int y)
{
    ((unsigned char*)slot)[0] = (unsigned char)x;
    ((unsigned char*)slot)[1] = (unsigned char)y;
}

/* cdecl RTL: the ox assign (second arg) loads before v0 (first arg / return). */
static __inline int LFUpd_Fst(int a, int b)
{
    return a;
}

/* =========================================================================
 * 0x0040d6f0 -- SHARED UPDATE (+0x90).  Stamp the class footprint onto the
 * edit cursor, convert the mouse, paint the geom preview, refuse on bricks
 * / empty neighbourhood / failed probe / people, else commit the placement
 * cursors.  Packed square and the neighbour-list pointer share one union
 * that lives in the dead footprint-argument slot; mode=0 after
 * ScreenToMapRef pins the dead mode slot so the union cannot take it.
 * People-rect left is unsigned two-def o.y plus LFUpd_Fst(v0, o.x=ox)
 * so RTL emits ox-first moffs and lea edx,[eax+ecx].
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040d6f0
void LFPiece_UpdateCommon(RideDef* def, int screen, int mode, Footprint* fp,
                          void (*geom)(unsigned int sq, LFGeom* out),
                          int (*probe)(LFPiece** nb))
{
    Rect      r;
    LFGeom    g;
    int       cost;
    int       people;
    union {
        unsigned int packed;
        LFPiece**    nb;
    } u;

    g_edit_cursor.footprint = *fp;
    ScreenToMapRef(screen, &g_mapref, mode);
    mode = 0;
    g_8003f0 = (void*)mode;
    ResetCursorFootprint(&g_edit_cursor);
    ValidateCursor(&g_edit_cursor, def);
    LFUpd_PackSq(&u.packed, g_mapref.x, g_mapref.y);
    geom(u.packed, &g);
    LFGeom_ApplyCursors(&g);
    g_8003f0 = &g_lf_geom_cursor_a;
    cost = GetObjCost(def);
    if (GetBrickCount() < cost)
        SetCursorError(&g_edit_cursor, 2);
    if (CursorIsValid(&g_edit_cursor)) {
        LFUpd_PackSq(&u.packed, g_mapref.x, g_mapref.y);
        geom(u.packed, &g);
        LFGeom_ProbeNeighbours(&g, &u.nb);
        LFNb_DropFull(u.nb);
        if (LFNb_Count(u.nb) == 0) {
            SetCursorError(&g_edit_cursor, 0xe);
        } else {
            ResetCursorFootprint(&g_edit_cursor);
            {
                LFRun* run = LFNb_FirstRun(u.nb);
                LFNb_KeepRun(run, u.nb);
            }
            if (LFNb_Count(u.nb) == 0) {
                SetCursorError(&g_edit_cursor, 0xe);
            } else if (probe(u.nb) != 0) {
                ResetCursorFootprint(&g_edit_cursor);
                LFTrack_CommitPlacement(u.nb,
                    ((EditCursorRec*)g_8003f0)->next);
            } else {
                SetCursorError(&g_edit_cursor, 0xd);
            }
        }
    }
    if (CursorIsValid(&g_edit_cursor)) {
        {
            Pos o;
            unsigned left;
            o.y = LFUpd_Fst(g_edit_cursor.footprint.v[0], o.x = g_mapref.x);
            left = (unsigned)o.x + (unsigned)o.y;
            o.y = g_mapref.y;
            r.left = (int)left;
            r.top    = *(volatile int*)&g_edit_cursor.footprint.v[1] + o.y;
            r.right  = g_edit_cursor.footprint.v[2] + o.x;
            r.bottom = g_edit_cursor.footprint.v[3] + o.y;
        }
        people = CheckForPeople(&r);
        if (people != -1) {
            if (people == 1)
                SetCursorError(&g_edit_cursor, 3);
        } else {
            SetCursorError(&g_edit_cursor, 4);
        }
    }
    PropagateCursorStatus(&g_edit_cursor);
}
