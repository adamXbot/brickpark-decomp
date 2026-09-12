/* LEGOLAND -- the LOG FLUME, part 8: track reshape / entrance-neighbour
 * helpers (inventory group 1).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow logflume.c
 * / logflume2.c.
 *
 * THE FOUR-SLOT NEIGHBOUR ARRAY (g_lf_nb, 0x004cbe20) is indexed
 * N=0, E=1, S=2, W=3.  The TRACK class uses the unrolled helpers in
 * logflume2.c; the shared Add/Update/Remove path uses the loop forms here.
 *
 * kind 3/4 still have a free end; attaching a neighbour turns 4 into 3
 * (one end) and 3 into 1 (straight) or 2 (corner).  Detaching is the
 * inverse and lives in the four LFNb_Detach* helpers.
 *
 *   0x0040cf10  LFGeom_ProbeNeighbours      7/7 exact
 *   0x0040ce20  LFGeom_FillNeighbours      72/72 exact
 *   0x0040cf30  LFNb_Count                 10/10 exact
 *   0x0040cf50  LFNb_KeepRun               15/15 exact
 *   0x0040cf80  LFNb_FirstRun              14/14 exact
 *   0x0040cfa0  LFNb_DropFull              15/15 exact
 *   0x00409a50  LFPiece_MakeStraight       12/12 exact
 *   0x00409620  LFPiece_AttachN            25/25 exact
 *   0x00409680  LFPiece_AttachS            25/25 exact
 *   0x004096e0  LFPiece_AttachE            26/26 exact
 *   0x00409740  LFPiece_AttachW            25/25 exact
 *   0x004097a0  LFTrack_ReshapeNeighbours 170/170 exact
 *   0x00409a90  LFTrack_AttachNeighbours   49/49 exact
 *   0x00409b10  LFRoute_OrientPair         37/37 exact
 *   0x0040a010  LFRoute_SplicePair         46/46 exact
 *   0x0040a080  LFTrack_SpliceNeighbours   45/45 exact
 *   0x0040a0f0  LFNb_DetachN               38/38 exact
 *   0x0040a160  LFNb_DetachE               37/37 exact
 *   0x0040a1d0  LFNb_DetachS               37/37 exact
 *   0x0040a230  LFNb_DetachW               40/40 exact
 *   0x0040a2a0  LFTrack_RedrawNeighbours   23/23 exact
 *   0x0040b290  LFPiece_DrawAnim           93/93 exact
 */

typedef struct Pos { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

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
    int           kind;         /* +0x18  3/4 = still has a free end */
    int           dir;          /* +0x1c  orientation 0..3 = N,E,S,W */
    void*         def;          /* +0x20 */
    LFRun*        run;          /* +0x24 */
    int           f28;          /* +0x28 */
    LFPiece*      sub;          /* +0x2c */
    LFPiece*      end_a;        /* +0x30 */
    LFPiece*      end_b;        /* +0x34 */
};

typedef struct LFBoat {
    unsigned char pad[0x24];
} LFBoat;

struct LFRun {
    LFRun*        next;         /* +0x00 */
    unsigned char pad04[0x3c - 0x04];
    int           boat_count;   /* +0x3c */
    LFBoat        boats[4];     /* +0x40 */
};

extern LFPiece* g_lf_nb[4];             /* 0x004cbe20 */

typedef struct FootPart FootPart;
typedef struct Footprint {
    int       v[4];             /* +0x00 */
    FootPart* parts;            /* +0x10 */
} Footprint;

/* Per-class probe geom: a flag byte and the four end squares as Pos. */
typedef struct LFGeom {
    unsigned char flags;        /* +0x00  bit0=N, bit1=E, bit2=S, bit3=W */
    unsigned char pad[3];
    Pos           ends[4];      /* +0x04  N, E, S, W */
} LFGeom;

extern Footprint g_lf_footprint;        /* 0x004b4728 */
extern LFPiece*  LFTrack_FindPiece(const BPos* sq);              /* 0x00408f30 */
extern int       LFTrack_NeighbourMask(LFPiece** nb);            /* 0x00409410 */

void LFGeom_FillNeighbours(LFGeom* g);

/* =========================================================================
 * 0x0040cf10 -- fill the global neighbour array via the geom helper and
 * hand back its address.  Unlike LFTrack_ProbeNeighbours this does not
 * zero the slots first; 0x0040ce20 owns that.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x0040cf10
void LFGeom_ProbeNeighbours(LFGeom* g, LFPiece*** out)
{
    LFGeom_FillNeighbours(g);
    *out = g_lf_nb;
}

/* Fill g_lf_nb from a set-piece geom: step one flume cell from each
 * published end, same dx/dy as LFTrack_FillNeighbours. */
// FUNCTION: LEGOLAND 0x0040ce20
void LFGeom_FillNeighbours(LFGeom* g)
{
    int  dx = g_lf_footprint.v[2] - g_lf_footprint.v[0];
    int  dy = g_lf_footprint.v[3] - g_lf_footprint.v[1];
    BPos c;

    g_lf_nb[0] = 0;
    g_lf_nb[1] = 0;
    g_lf_nb[2] = 0;
    g_lf_nb[3] = 0;
    if (g->flags & 1) {
        unsigned char ny = (unsigned char)g->ends[0].y;
        unsigned char nx = (unsigned char)g->ends[0].x;
        c.x = nx;
        c.y = (unsigned char)(ny - dy);
        g_lf_nb[0] = LFTrack_FindPiece(&c);
    }
    if (g->flags & 2) {
        unsigned char ex = (unsigned char)g->ends[1].x;
        unsigned char ey = (unsigned char)g->ends[1].y;
        c.x = (unsigned char)(ex + dx);
        c.y = ey;
        g_lf_nb[1] = LFTrack_FindPiece(&c);
    }
    if (g->flags & 4) {
        unsigned char sy = (unsigned char)g->ends[2].y;
        unsigned char sx = (unsigned char)g->ends[2].x;
        c.x = sx;
        c.y = (unsigned char)(sy + dy);
        g_lf_nb[2] = LFTrack_FindPiece(&c);
    }
    if (g->flags & 8) {
        unsigned char wx = (unsigned char)g->ends[3].x;
        unsigned char wy = (unsigned char)g->ends[3].y;
        c.x = (unsigned char)(wx - dx);
        c.y = wy;
        g_lf_nb[3] = LFTrack_FindPiece(&c);
    }
}

/* How many of the four passed slots are occupied.  Unlike
 * LFTrack_CountNeighbours this walks the POINTER it is handed. */
// FUNCTION: LEGOLAND 0x0040cf30
int LFNb_Count(LFPiece** nb)
{
    int n = 0;
    int i = 4;

    do {
        if (*nb)
            n++;
        nb++;
    } while (--i);
    return n;
}

/* Drop every neighbour that belongs to a different run.  Loop form of
 * LFRun_KeepOnly. */
// FUNCTION: LEGOLAND 0x0040cf50
void LFNb_KeepRun(LFRun* run, LFPiece** nb)
{
    LFPiece** q = nb;
    int i = 4;

    do {
        LFPiece* p = *q;
        if (p != 0 && p->run != run)
            *q = 0;
        q++;
    } while (--i);
}

/* The run of the first occupied slot, or 0. */
// FUNCTION: LEGOLAND 0x0040cf80
LFRun* LFNb_FirstRun(LFPiece** nb)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (nb[i])
            return nb[i]->run;
    }
    return 0;
}

/* Discard every neighbour that has no free end (kind 3 or 4).  Loop form
 * of LFTrack_DropFullNeighbours. */
// FUNCTION: LEGOLAND 0x0040cfa0
void LFNb_DropFull(LFPiece** nb)
{
    int i = 4;

    do {
        LFPiece* p = *nb;
        if (p) {
            int kind = p->kind;
            if (kind != 3 && kind != 4)
                *nb = 0;
        }
        nb++;
    } while (--i);
}

/* Force a piece onto a straight (kind 1).  Even dirs become dir 0
 * (north-south); odd dirs become dir 1 (east-west).  Shared kind store
 * via goto setkind; VC6 tail-duplicates it into case 0/2 with edx live. */
// FUNCTION: LEGOLAND 0x00409a50
void LFPiece_MakeStraight(LFPiece* piece)
{
    LFPiece* p = piece;
    int one = 1;

    switch (p->dir) {
    case 0:
    case 2:
        p->dir = 0;
        goto setkind;
    case 1:
    case 3:
        p->dir = one;
    default:
    setkind:
        p->kind = one;
        return;
    }
}

/* Reshape a piece that is gaining a connection on its south side (it is
 * the NORTH neighbour of a newly placed square).  Isolated (4) becomes
 * an endpoint facing north; an endpoint becomes a straight or a corner. */
// FUNCTION: LEGOLAND 0x00409620
void LFPiece_AttachN(LFPiece* piece)
{
    LFPiece* p = piece;
    int kind = p->kind;

    if (kind == 3) {
        int dir = p->dir;
        if (dir == 2) {
            p->kind = 1;
            p->dir = 0;
            return;
        }
        if (dir == 1) {
            p->kind = 2;
            p->dir = 2;
            return;
        }
        if (dir == 3) {
            p->kind = 2;
            p->dir = 1;
            return;
        }
    }
    if (kind == 4) {
        p->kind = 3;
        p->dir = 0;
    }
}

/* Same for a SOUTH neighbour (gaining a north connection). */
// FUNCTION: LEGOLAND 0x00409680
void LFPiece_AttachS(LFPiece* piece)
{
    LFPiece* p = piece;
    int kind = p->kind;

    if (kind == 3) {
        int dir = p->dir;
        if (dir == 0) {
            p->kind = 1;
            *(volatile int*)&p->dir = dir;
            return;
        }
        if (dir == 1) {
            p->kind = 2;
            p->dir = 3;
            return;
        }
        if (dir == 3) {
            p->kind = 2;
            p->dir = 0;
            return;
        }
    }
    if (kind == 4) {
        p->kind = 3;
        p->dir = 2;
    }
}

/* Same for an EAST neighbour (gaining a west connection). */
/* dir==2 must store kind as the literal 2, not `dir`.  `p->kind = dir`
 * value-numbers the 2,2 arm onto ecx (CSE → ESCAPES).  After `cmp ecx,2`
 * a literal 2 still emits `mov [eax+18], ecx` — same third-arm bytes,
 * without infecting the previous arm.  AttachN's 2,2 stay immediate
 * because its sibling stores `2` as a literal, not via live dir. */
// FUNCTION: LEGOLAND 0x004096e0
void LFPiece_AttachE(LFPiece* piece)
{
    LFPiece* p = piece;
    int kind = p->kind;

    if (kind == 3) {
        int dir = p->dir;
        if (dir == kind) {
            int one = 1;
            p->kind = one;
            p->dir = one;
            return;
        }
        if (dir == 0) {
            p->kind = 2;
            p->dir = 2;
            return;
        }
        if (dir == 2) {
            p->kind = 2;
            p->dir = 3;
            return;
        }
    }
    if (kind == 4) {
        p->kind = 3;
        p->dir = 1;
    }
}

/* Same for a WEST neighbour (gaining a east connection). */
// FUNCTION: LEGOLAND 0x00409740
void LFPiece_AttachW(LFPiece* piece)
{
    LFPiece* p = piece;
    int kind = p->kind;

    if (kind == 3) {
        int dir = p->dir;
        if (dir == 1) {
            p->kind = dir;
            *(volatile int*)&p->dir = dir;
            return;
        }
        if (dir == 0) {
            p->kind = 2;
            p->dir = 1;
            return;
        }
        if (dir == 2) {
            p->kind = dir;
            p->dir = 0;
            return;
        }
    }
    if (kind == 4) {
        p->kind = 3;
        p->dir = 3;
    }
}

extern int  LFPiece_CursorFits(LFPiece* p);                      /* 0x00409140 */
extern void LFPiece_ReverseRoute(LFPiece* p);                    /* 0x00409110 */
extern void LFPiece_LinkBefore(LFPiece* a, LFPiece* b);           /* 0x00409040 */
extern void LFPiece_LinkAfter(LFPiece* a, LFPiece* b);            /* 0x00409080 */
#ifndef LEGOLAND_PORTABLE
extern void DebugPrint(const char* msg);                         /* 0x0049e5c5 */
#else
/* As logflume2.c: 0x0049e5c5 is the variadic `printf`, so its wasm signature
 * carries the varargs buffer pointer this spelling does not mention. PORT-A11,
 * gated by portable/tools/variadic_sweep.py. */
extern void DebugPrint(const char* msg, ...);                    /* 0x0049e5c5 */
#endif
extern char g_lf_both_msg[];                                     /* 0x004b48e4 */

/* For each occupied neighbour: force our matching end onto a straight
 * and attach that neighbour.  Mask comes from `nb`. */
// FUNCTION: LEGOLAND 0x00409a90
void LFTrack_AttachNeighbours(LFPiece** ours, LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask & 1) {
        LFPiece_MakeStraight(ours[0]);
        LFPiece_AttachN(nb[0]);
    }
    if (mask & 4) {
        LFPiece_MakeStraight(ours[1]);
        LFPiece_AttachE(nb[1]);
    }
    if (mask & 0x10) {
        LFPiece_MakeStraight(ours[2]);
        LFPiece_AttachS(nb[2]);
    }
    if (mask & 0x40) {
        LFPiece_MakeStraight(ours[3]);
        LFPiece_AttachW(nb[3]);
    }
}

/* Orient two route ends so they face each other.  Same degenerate
 * reverse-a-either-way as LFRoute_Join when `a` is not on the cursor. */
// FUNCTION: LEGOLAND 0x00409b10
void LFRoute_OrientPair(LFPiece* a, LFPiece* b)
{
    int oi = LFPiece_CursorFits(a);
    int oj = LFPiece_CursorFits(b);

    if (oi && oj) {
        DebugPrint(g_lf_both_msg);
        return;
    }
    if (!oi) {
        if (oj)
            LFPiece_ReverseRoute(a);
        else
            LFPiece_ReverseRoute(a);
    } else {
        LFPiece_ReverseRoute(b);
    }
}

/* Join or splice two pieces that now share an edge. */
// FUNCTION: LEGOLAND 0x0040a010
void LFRoute_SplicePair(LFPiece* a, LFPiece* b)
{
    if ((a->fwd && b->fwd && !a->back && !b->back) ||
        (!a->fwd && !b->fwd && a->back && b->back))
        LFRoute_OrientPair(a, b);
    if (!a->fwd)
        LFPiece_LinkAfter(a, b);
    else
        LFPiece_LinkBefore(a, b);
}

/* Splice every occupied neighbour onto the matching end. */
// FUNCTION: LEGOLAND 0x0040a080
void LFTrack_SpliceNeighbours(LFPiece** ours, LFPiece** nb)
{
    int mask = LFTrack_NeighbourMask(nb);

    if (mask & 1)
        LFRoute_SplicePair(nb[0], ours[0]);
    if (mask & 4)
        LFRoute_SplicePair(nb[1], ours[1]);
    if (mask & 0x10)
        LFRoute_SplicePair(nb[2], ours[2]);
    if (mask & 0x40)
        LFRoute_SplicePair(nb[3], ours[3]);
}

/* Detach the NORTH connection of a neighbour (inverse of AttachN).
 * Later kind tests reread `p->kind` so VC6 cannot jump-thread the
 * proven-3 fail edge; dir then lands in esi. */
// FUNCTION: LEGOLAND 0x0040a0f0
void LFNb_DetachN(LFPiece* p)
{
    int kind;
    int three;
    int dir;

    if (!p)
        return;
    kind = p->kind;
    three = 3;
    if (kind == three) {
        dir = p->dir;
        if (dir == 0) {
            p->kind = 4;
            return;
        }
    }
    if (p->kind == 1) {
        dir = p->dir;
        if (dir == 0) {
            p->kind = three;
            p->dir = 2;
            return;
        }
    }
    if (p->kind == 2) {
        int d = p->dir;
        if (d == 2) {
            p->kind = three;
            p->dir = 1;
            return;
        }
        if (d == 1) {
            p->kind = three;
            p->dir = three;
            return;
        }
    }
}

/* Detach the EAST connection (inverse of AttachE). */
// FUNCTION: LEGOLAND 0x0040a160
void LFNb_DetachE(LFPiece* p)
{
    int kind;
    int three;
    int one;

    if (!p)
        return;
    kind = p->kind;
    three = 3;
    one = 1;
    if (kind == three) {
        if (p->dir == one) {
            p->kind = 4;
            return;
        }
    }
    if (p->kind == one) {
        if (p->dir == one) {
            p->kind = three;
            p->dir = three;
            return;
        }
    }
    if (p->kind == 2) {
        int d = p->dir;
        if (d == three) {
            p->kind = three;
            p->dir = 2;
            return;
        }
        if (d == 2) {
            p->kind = three;
            p->dir = 0;
            return;
        }
    }
}

/* Detach the SOUTH connection (inverse of AttachS). */
// FUNCTION: LEGOLAND 0x0040a1d0
void LFNb_DetachS(LFPiece* p)
{
    int kind;
    int three;
    int dir;

    if (!p)
        return;
    kind = p->kind;
    three = 3;
    if (kind == three) {
        if (p->dir == 2) {
            p->kind = 4;
            return;
        }
    }
    if (p->kind == 1) {
        dir = p->dir;
        if (dir == 0) {
            p->kind = three;
            p->dir = 0;
            return;
        }
    }
    if (p->kind == 2) {
        int d = p->dir;
        if (d == three) {
            p->kind = three;
            p->dir = 1;
            return;
        }
        if (d == 0) {
            p->kind = three;
            p->dir = three;
            return;
        }
    }
}

/* Detach the WEST connection (inverse of AttachW). */
// FUNCTION: LEGOLAND 0x0040a230
void LFNb_DetachW(LFPiece* p)
{
    int kind;
    int three;
    int one;

    if (!p)
        return;
    kind = p->kind;
    three = 3;
    if (kind == three) {
        if (p->dir == three) {
            p->kind = 4;
            return;
        }
    }
    one = 1;
    if (p->kind == one) {
        int dir = p->dir;
        if (dir == one) {
            p->kind = three;
            p->dir = one;
            return;
        }
    }
    if (p->kind == 2) {
        int d = p->dir;
        if (d == 0) {
            p->kind = three;
            p->dir = 2;
            return;
        }
        if (d == one) {
            p->kind = three;
            p->dir = 0;
            return;
        }
    }
}

/* Redraw every neighbour of a just-removed piece.  NeighbourMask is
 * called and discarded; the guard is on `piece`. */
// FUNCTION: LEGOLAND 0x0040a2a0
void LFTrack_RedrawNeighbours(LFPiece* piece, LFPiece** nb)
{
    LFTrack_NeighbourMask(nb);
    if (piece) {
        LFNb_DetachN(nb[0]);
        LFNb_DetachE(nb[1]);
        LFNb_DetachS(nb[2]);
        LFNb_DetachW(nb[3]);
    }
}

typedef struct LFAnimFrame {
    int   dx;                   /* +0x00 */
    int   dy;                   /* +0x04  pieces this overlay spans */
    void* sprite;               /* +0x08 */
} LFAnimFrame;

typedef struct LFAnimSet {
    int          count;         /* +0x00 */
    LFAnimFrame* frames;        /* +0x04 */
} LFAnimSet;

extern int  LFBoat_IsOnPiece(LFBoat* b, LFPiece* p);             /* 0x0040b210 */
extern void LFBoat_Draw(LFBoat* b, LFPiece* p, int mode);        /* 0x0040ae90 */
extern int  PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */

/* Draw a multi-sprite overlay set while walking one end of the piece's
 * sub-route, queuing any boat that sits on those squares.  flag==0 walks
 * end_a backwards (prev); flag!=0 walks end_b forwards (next). */
// FUNCTION: LEGOLAND 0x0040b290
void LFPiece_DrawAnim(LFPiece* piece, int x, int y, LFAnimSet* anim, int flag)
{
    LFRun*       run = piece->run;
    LFPiece*     walk;
    LFAnimFrame* fr;
    int          i = 0;

    if (flag == 0)
        walk = piece->end_a;
    else
        walk = piece->end_b;
    fr = anim->frames;
    if (walk) {
        int off = 0;
        while (walk) {
            int b;
            for (b = 0; b < run->boat_count; b++) {
                if (LFBoat_IsOnPiece(&run->boats[b], walk))
                    LFBoat_Draw(&run->boats[b], walk, 1);
            }
            if (flag == 0)
                walk = walk->prev;
            else
                walk = walk->next;
            i++;
            if (i >= fr->dy) {
                if (fr->sprite)
                    PrintSprite(fr->sprite, x, y, 0, 0);
                off += 12;
                fr = (LFAnimFrame*)((char*)anim->frames + off);
            }
        }
    }
    if (fr->sprite)
        PrintSprite(fr->sprite, x, y, 0, 0);
}

/* After placing a square: set the new piece's kind/dir from the neighbour
 * mask (only if it has no sub-list) and attach every occupied neighbour. */
// FUNCTION: LEGOLAND 0x004097a0
void LFTrack_ReshapeNeighbours(LFPiece* piece, LFPiece** nb)
{
    int      mask = LFTrack_NeighbourMask(nb);
    int      two = 2;
    LFPiece* sub;

    if (!piece)
        return;
    sub = piece->sub;
    if (!sub) {
        switch (mask) {
        case 1:
        case 4:
        case 0x10:
        case 0x40:
            piece->kind = 3;
            break;
        case 0x11:
        case 0x44:
            piece->kind = 1;
            break;
        case 5:
        case 0x14:
        case 0x41:
        case 0x50:
            piece->kind = two;
            break;
        }
    }
    switch (mask) {
    case 1:
        if (!sub)
            piece->dir = two;
        LFPiece_AttachN(nb[0]);
        return;
    case 0x10:
        if (!sub)
            piece->dir = 0;
        LFPiece_AttachS(nb[2]);
        return;
    case 4:
        if (!sub)
            piece->dir = 3;
        LFPiece_AttachE(nb[1]);
        return;
    case 0x40:
        if (!sub)
            piece->dir = 1;
        LFPiece_AttachW(nb[3]);
        return;
    case 0x11:
        if (!sub)
            piece->dir = 0;
        LFPiece_AttachN(nb[0]);
        LFPiece_AttachS(nb[2]);
        return;
    case 0x44:
        if (!sub)
            piece->dir = 1;
        LFPiece_AttachE(nb[1]);
        LFPiece_AttachW(nb[3]);
        return;
    case 5:
        if (!sub)
            piece->dir = 0;
        LFPiece_AttachN(nb[0]);
        LFPiece_AttachE(nb[1]);
        return;
    case 0x14:
        if (!sub)
            piece->dir = 1;
        LFPiece_AttachE(nb[1]);
        LFPiece_AttachS(nb[2]);
        return;
    case 0x50:
        if (!sub)
            piece->dir = two;
        LFPiece_AttachS(nb[2]);
        LFPiece_AttachW(nb[3]);
        return;
    case 0x41:
        if (!sub)
            piece->dir = 3;
        LFPiece_AttachW(nb[3]);
        LFPiece_AttachN(nb[0]);
        return;
    }
}
