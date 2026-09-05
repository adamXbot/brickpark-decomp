/* LEGOLAND -- OBJECT DOORS, EXITS AND EDIT-CURSOR STATUS.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, record sizes and global addresses are load-bearing; the
 * names are ours.  Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere); ObjDef, Rect and Cursor are objrect.c's and objmap2.c's,
 * unchanged.
 *
 *   0x0045e620  ObjHasEntrance          does this class have a door at all?
 *   0x0045e6b0  GetObjEntranceDir       which side the entrance is on
 *   0x0045e710  GetObjExitDir           which side the exit is on
 *   0x0045ea40  GetObjectDoorOffset     the tile a bloke walks to
 *   0x00482a90  UpdateEntranceTile      cache the park entrance's tile
 *   0x0045f4d0  PropagateCursorStatus   push the worst error down a chain
 *
 * =========================================================================
 * 1. A CLASS'S DOOR IS AN OFFSET OUTSIDE ITS OWN FOOTPRINT
 * =========================================================================
 * An object class (the 0xd0-byte ODF record) carries
 *
 *      +0x0c        the ENTRANCE offset, a Pos
 *      +0x24/+0x25   the EXIT offset, two SIGNED bytes
 *      +0x20         a 16-bit class TYPE
 *      +0x3c         the footprint Rect {left, top, right, bottom, next}
 *
 * all in map squares relative to the object's base cell.  A door EXISTS when
 * the class is non-null, its type is none of 0, 2 or 3, and the entrance
 * offset falls OUTSIDE the footprint rect -- because a door has to be a tile
 * a bloke can stand on, and every tile inside the footprint is occupied by
 * the object itself.  That is ObjHasEntrance, and it is the reason the two
 * direction helpers can simply ask which side the offset fell off:
 *
 *      GetObjEntranceDir      x > right -> 4    x < left -> 8
 *                             y > bottom -> 1   otherwise 2
 *      GetObjExitDir          x > right -> 0x80 x < left -> 0x40
 *                             y > bottom -> 0x20  otherwise 0x10
 *
 * so the two doors share four compass bits in the low and high nibbles of
 * the cell's door word (MapCell +0x12).  Note the ASYMMETRY, which is the
 * original's: the x axis has two real tests and a `return`, while the y axis
 * has ONE test whose false arm is simply "the other side" -- an offset level
 * with the rect on x but inside it on y answers "north" rather than "no
 * door".  ObjHasEntrance is what stops that mattering.
 *
 * All three functions copy the WHOLE 20-byte Rect to the stack first
 * (`mov ecx,5 / rep movsd`) and read it back from there, even though
 * GetObjectDoorOffset -- next door, same record -- reads the same four
 * fields straight out of the class.  Reproduced: it is a by-value struct
 * copy in the source, not an optimisation.
 *
 * =========================================================================
 * 2. WHERE A BLOKE ACTUALLY WALKS TO  (GetObjectDoorOffset)
 * =========================================================================
 * The entrance offset is only usable when it really is outside the footprint
 * AND the class type is 1, 4 or 5.  When either fails, the door falls back
 * to the middle of the footprint's RIGHT edge:
 *
 *      out->x = right + 1
 *      out->y = (bottom + top) / 2          (signed halving: cdq/sub/sar)
 *
 * i.e. one square clear of the object, level with its centre.  So a class
 * with no usable door still gets a walkable target rather than none.
 *
 * =========================================================================
 * 3. THE PARK ENTRANCE  (UpdateEntranceTile)
 * =========================================================================
 * The park's own entrance is the placed object of class "ENTRANCE 1".  Its
 * walk-to tile is computed ONCE and cached in the Pos at 0x0066b460 (which
 * GetEntranceTile hands out and RefreshEntranceTile invalidates); a non-zero
 * x is the "already cached" flag, so an entrance whose door tile really is
 * x = 0 would be recomputed every call -- harmless, and reproduced.
 *
 * The element handle itself is cached separately at 0x006661c4 and looked up
 * lazily.  The tile is
 *
 *      x = cell.bx + class->rect.left - 1
 *      y = cell.by + (class->rect.bottom + class->rect.top) / 2
 *
 * -- the middle of the footprint's LEFT edge, one square clear, which is the
 * mirror of GetObjectDoorOffset's fallback.  Note the class is re-read from
 * the global AFTER the object search, not carried across it.
 *
 * =========================================================================
 * 4. THE EDIT-CURSOR CHAIN  (PropagateCursorStatus)
 * =========================================================================
 * An edit cursor is a chain of 0x1834-byte blocks linked by +0x1830, one per
 * rect of the thing being placed.  Each block carries a `status` (+0x140c;
 * SetCursorError stores the NEGATED error code there, and CursorIsValid is
 * `status > 0`) and the `error` code itself (+0x1410).
 *
 * This is the chain-wide reconciliation, run after every block has been
 * probed:
 *
 *   1. walk the chain, set flag bit 8 on every block, and remember the block
 *      with the LOWEST status -- the worst failure;
 *   2. if that block is not valid, push its error onto every block in the
 *      chain with SetCursorError, which keeps whichever error is worse.
 *
 * So one bad square makes the whole placement report the same reason, which
 * is what the pop-up needs to print a single message.  The two walks are
 * separate: the second re-tests the head for null, and it reuses the
 * parameter's register as its cursor.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */

typedef struct Pos { int x; int y; } Pos;

/* The footprint rect list -- 20 bytes, and the three door predicates copy
 * all 20 by value. */
typedef struct Rect {
    int          left;           /* +0x00 */
    int          top;            /* +0x04 */
    int          right;          /* +0x08 */
    int          bottom;         /* +0x0c */
    struct Rect* next;           /* +0x10 */
} Rect;

/* An object class / definition (objrect.c's ObjDef, unchanged). */
typedef struct ObjDef {
    char           pad0[0x0c];   /* +0x00 */
    /* The entrance offset is ONE Pos, not two ints: GetObjectDoorOffset's
     * fall-through arm is `*out = d->entrance;` and only the whole-struct
     * assignment reproduces it (it breaks the CSE with the bounds test, which
     * is what lets `push esi` sink into the other arm -- 23 mismatches with
     * two int fields, at the same instruction count, and no operand order,
     * layout or free-volatile spelling reaches it).  objrect.c spells the
     * same pair as two ints; nothing there depends on it. */
    Pos            entrance;     /* +0x0c entrance offset from the base cell */
    char           pad14[0x1c - 0x14];
    unsigned int   flags;        /* +0x1c */
    short          type;         /* +0x20 */
    char           pad22[0x24 - 0x22];
    signed char    ex;           /* +0x24 exit offset from the base cell */
    signed char    ey;           /* +0x25 */
    char           pad26[0x3c - 0x26];
    Rect           rect;         /* +0x3c footprint rect list */
    char           pad50[0xd0 - 0x50];
} ObjDef;

/* An LLIDB element: the class record hangs off +0x0c. */
typedef struct ObjElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int type_flags;     /* +0x08 */
    ObjDef*      data;           /* +0x0c */
} ObjElem;

/* A map cell -- only the owning object and its base square are used here. */
typedef struct Cell {
    void*         obj;           /* +0x00 */
    unsigned char bx;            /* +0x04  base cell of the owning object */
    unsigned char by;            /* +0x05 */
    char          pad06[0x14 - 6];
} Cell;

/* An edit / destroy cursor block (objmap2.c; 0x1834 bytes). */
typedef struct Cursor {
    char           pad0000[0x140c];
    int            status;       /* +0x140c  negated error code, > 0 = ok */
    int            error;        /* +0x1410 */
    char           pad1414[0x1828 - 0x1414];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* --------------------------------------------------------------- globals -- */

/* The cached park-entrance tile; GetEntranceTile (0x00482b00) hands out its
 * address and RefreshEntranceTile (0x00482b20) invalidates it. */
extern Pos      g_entrance_tile;                /* 0x0066b460 */
extern ObjElem* g_entrance_elem;                /* 0x006661c4 */

/* --------------------------------------------------------------- callees -- */

extern ObjElem* ElemID(const char* name);                       /* 0x0047b3f0 */
extern Cell*    GetFirstObjectMatching(void* obj);              /* 0x0045a910 */
extern int      CursorIsValid(Cursor* c);                       /* 0x0045f4b0 */
extern void     SetCursorError(Cursor* c, int code);            /* 0x0045f480 */

/* =========================================================================
 * THE DOOR PREDICATES
 * ========================================================================= */

/* True when this class has an entrance a bloke could stand on: the class
 * exists, its type is none of 3, 2 or 0, and the entrance offset falls
 * outside the footprint rect. */
// FUNCTION: LEGOLAND 0x0045e620
int ObjHasEntrance(ObjDef* d)
{
    Rect fp;

    if (d) {
        fp = d->rect;
        if (d->type != 3 && d->type != 2 && d->type != 0) {
            if (fp.left > d->entrance.x || d->entrance.x > fp.right ||
                fp.top > d->entrance.y || d->entrance.y > fp.bottom)
                return 1;
        }
    }
    return 0;
}

/* Which side of the footprint the entrance sits on: 4 east, 8 west, and on
 * the y axis 1 or 2 with no third answer. */
// FUNCTION: LEGOLAND 0x0045e6b0
int GetObjEntranceDir(ObjDef* d)
{
    Rect fp;

    fp = d->rect;
    if (fp.right < d->entrance.x)
        return 4;
    if (d->entrance.x < fp.left)
        return 8;
    return (fp.bottom < d->entrance.y) ? 1 : 2;
}

/* The same four sides for the EXIT offset, in the high nibble.  The exit
 * offset is a pair of SIGNED bytes, so both reads are `movsx`. */
// FUNCTION: LEGOLAND 0x0045e710
int GetObjExitDir(ObjDef* d)
{
    Rect fp;

    fp = d->rect;
    if (fp.right < d->ex)
        return 0x80;
    if (d->ex < fp.left)
        return 0x40;
    return (fp.bottom < d->ey) ? 0x20 : 0x10;
}

/* The tile a bloke walks to for this class: the entrance offset when it is
 * usable, otherwise one square clear of the footprint's right edge, level
 * with its centre. */
// FUNCTION: LEGOLAND 0x0045ea40
void GetObjectDoorOffset(ObjDef* d, Pos* out)
{
    if ((d->entrance.x >= d->rect.left && d->entrance.x <= d->rect.right &&
         d->entrance.y >= d->rect.top && d->entrance.y <= d->rect.bottom) ||
        (d->type != 1 && d->type != 4 && d->type != 5)) {
        out->x = d->rect.right + 1;
        out->y = (d->rect.bottom + d->rect.top) / 2;
        return;
    }
    *out = d->entrance;
}

/* =========================================================================
 * THE PARK ENTRANCE
 * ========================================================================= */

/* The class MUST be named in a local: the store to g_entrance_tile.x kills
 * the CSE of `g_entrance_elem` (a store to any global kills every global
 * load), so spelling `g_entrance_elem->data->rect...` twice reloads the
 * element and re-derives the class, which is one instruction too many. */
// FUNCTION: LEGOLAND 0x00482a90
void UpdateEntranceTile(void)
{
    Cell*   c;
    ObjDef* d;

    if (g_entrance_tile.x == 0) {
        if (g_entrance_elem == 0)
            g_entrance_elem = ElemID("ENTRANCE 1");
        c = GetFirstObjectMatching(g_entrance_elem);
        d = g_entrance_elem->data;
        g_entrance_tile.x = c->bx + d->rect.left - 1;
        g_entrance_tile.y = (d->rect.bottom + d->rect.top) / 2 + c->by;
    }
}

/* =========================================================================
 * THE EDIT-CURSOR CHAIN
 * ========================================================================= */

/* Mark every block of the chain, find the worst status, and if that block is
 * a failure push its error onto every block. */
// FUNCTION: LEGOLAND 0x0045f4d0
void PropagateCursorStatus(Cursor* c)
{
    Cursor* worst = c;
    Cursor* p = c;

    while (p) {
        p->flags |= 8;
        if (p->status < worst->status)
            worst = p;
        p = p->next;
    }
    if (!CursorIsValid(worst)) {
        while (c) {
            SetCursorError(c, worst->error);
            c = c->next;
        }
    }
}
