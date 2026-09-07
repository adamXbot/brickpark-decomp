/* LEGOLAND -- scope AE: live group-13 high-level AI table handlers.
 * VC6 SP3 /O2 /Gy /Gd. Names and recovered mechanics: docs/lanes/scope-ae.md.
 * Dispatch is DoHighLevelAI (blokeai.c) through the 26-entry table at
 * 0x004b8368. These are the remaining live visitor/worker leaf plans.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; } Rect;
typedef struct Person { char pad00[8]; int kind; } Person;
typedef struct ObjClass {
    char pad00[0x0c];
    int  stand_x;                /* +0x0c  standing-tile offset from owner */
    int  stand_y;                /* +0x10 */
    char pad14[0x28];
    Rect rect;                   /* +0x3c  footprint */
} ObjClass;
typedef union TileKey {
    unsigned short w;
    struct { unsigned char x, y; } b;
} TileKey;
typedef struct Cell {
    void*          obj;          /* +0x00  LLIDB element */
    TileKey        xy;           /* +0x04 packed owner */
    char           pad06[6];
    unsigned short flags;        /* +0x0c */
} Cell;
typedef struct Map {
    char           pad00[0x14];
    unsigned short width, height; /* +0x14, +0x16 */
} Map;
typedef struct Elem {
    char pad00[0x0c];
    ObjClass* cls;               /* +0x0c */
} Elem;
typedef struct Bloke {
    struct Bloke* next;
    Person*       person;        /* +0x04 */
    char          pad08[4];
    unsigned short plan;         /* +0x0c */
    unsigned short state;        /* +0x0e */
    char          pad10[4];
    Elem*         selected;      /* +0x14 */
    char          pad18[0x0c];
    Pos           target;        /* +0x24 */
    Pos           dest;          /* +0x2c  long destination */
    char          pad34[0x12];
    TileKey       owner;         /* +0x46 packed owner */
    char          pad48[0x0c];
    unsigned int  last_job;      /* +0x54 */
    int           wait;          /* +0x58 */
    int           elapsed;       /* +0x5c */
    unsigned char action;        /* +0x60 */
    char          pad61;
    unsigned short flags;        /* +0x62 */
    unsigned char result;        /* +0x64 */
    char          pad65[3];
    Pos           world;         /* +0x68 */
    unsigned short height;       /* +0x70 */
    unsigned char dir;           /* +0x72 */
    unsigned char new_dir;       /* +0x73 */
    char          pad74[0x24];
    unsigned char path[0x14];    /* +0x98 */
} Bloke;

extern int          g_entrance_tile_x;               /* 0x0066b460 */
extern int          g_entrance_tile_y;               /* 0x0066b464 */
extern unsigned int g_sim_frame;                     /* 0x008119a4 */
extern void*        g_cafe_brolly_elem;              /* 0x006661c0 */
extern Map*         g_map;                           /* 0x004bcbf4 */
extern Cell**       g_map_rows;                      /* 0x00801400 */

extern void NewLongTermAction(Bloke* b, int action); /* 0x0044e760 */
extern int  CalcMoveLine(Pos from, Pos to, void* path); /* 0x00480740 */
extern int  NewDirForAction(Bloke* b, unsigned char dir); /* 0x004833d0 */
extern void BlokeSetAnim(Bloke* b, int anim);        /* 0x004406c0 */
extern void BlokeSetFrame(Bloke* b, int frame);      /* 0x00440870 */
extern int  PlayBlokeAnim(Bloke* b);                 /* 0x004408a0 */
extern void BlokeWalkAnim(Bloke* b);                 /* 0x00440910 */
extern int  GetBlokeNum(Bloke* b);                   /* 0x00482fb0 */
extern void IncrementBlokeCounter(ObjClass* cls, int idx); /* 0x00480ec0 */
extern int  rand(void);                              /* 0x0049e4b2 */
extern Cell* GetFirstObjectMatching(void* obj);      /* 0x0045a910 */
extern Cell* GetNextObjectMatching(Cell* c, void* obj); /* 0x0045a940 */
extern int  SuggestNextMove(Pos* from, Pos* to, Pos* out); /* 0x00482050 */

void FaceClassRect(Bloke* b, Rect* r);

static __inline Cell* CellAt(int x, int y)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

/* Hover/select a map person (gameframe cases 0x306..0x308). Workers stamp
 * last_job with the current frame; visitors take plan 0xe (wave then resume). */
// FUNCTION: LEGOLAND 0x00450a40
void SelectBloke(Bloke* b)
{
    unsigned short flags = b->flags;
    int kind;

    if (flags & 0x28)
        return;
    kind = b->person->kind;
    if (kind >= 2 && kind <= 3) {
        b->last_job = g_sim_frame;
        return;
    }
    b->flags = (unsigned short)(flags | 8);
    NewLongTermAction(b, 0xe);
}

/* Plan 0x17: walk to the cached entrance tile, then resume plan 6. */
// FUNCTION: LEGOLAND 0x0044fe10
void Visitor_WalkToEntrance(Bloke* b)
{
    unsigned char a;

    switch (b->action) {
    case 0: {
        Pos t;
        t.y = g_entrance_tile_y;
        t.x = g_entrance_tile_x;
        t.x <<= 8;
        t.y <<= 8;
        b->target = t;
        a = (unsigned char)(CalcMoveLine(b->world, t, b->path) + 0x10);
        b->state = 0xf;
        b->new_dir = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        b->action++;
        break;
    }
    case 1:
        NewLongTermAction(b, 6);
        break;
    }
}

/* Plan 0x14: face direction 4, idle in state 0xd, then gardener/mechanic idle. */
// FUNCTION: LEGOLAND 0x00450330
void Worker_ResumeIdle(Bloke* b)
{
    unsigned char act;

    act = b->action;
    switch (b->action) {
    case 0:
        NewDirForAction(b, 4);
        b->action++;
        break;
    case 1:
        b->state = 0xd;
        b->action = (unsigned char)(act + 1);
        break;
    case 2:
        switch (b->person->kind) {
        case 2:
            NewLongTermAction(b, 0x10);
            break;
        case 3:
            NewLongTermAction(b, 0x11);
            break;
        }
        break;
    }
}

/* Face the class footprint biased by dest. Writes heading +0x72 only. */
// FUNCTION: LEGOLAND 0x004503a0
void FaceClassRect(Bloke* b, Rect* r)
{
    int ox = b->dest.x;
    int wx = b->world.x >> 8;
    int wy = b->world.y >> 8;
    int oy = b->dest.y;

    if (wx > r->right + ox) {
        if (wy < r->top + oy)
            b->dir = 6;
        else if (wy > r->bottom + oy)
            b->dir = 0;
        else
            b->dir = 7;
    } else if (wx < r->left + ox) {
        if (wy < r->top + oy)
            b->dir = 4;
        else if (wy > r->bottom + oy)
            b->dir = 2;
        else
            b->dir = 3;
    } else if (wy < r->top + oy) {
        b->dir = 5;
    } else {
        b->dir = 1;
    }
}

/* Plan 0x0f: face the selected class, wait a short random time, mark visit. */
// FUNCTION: LEGOLAND 0x00450450
void Visitor_FaceAndMark(Bloke* b)
{
    unsigned char act;

    act = b->action;
    switch (b->action) {
    case 0:
        FaceClassRect(b, &b->selected->cls->rect);
        b->wait = (rand() & 0x1f) + 10;
        b->action++;
        break;
    case 1:
        if (--b->wait < 0)
            b->action = (unsigned char)(act + 1);
        break;
    case 2:
        IncrementBlokeCounter(b->selected->cls, GetBlokeNum(b));
        NewLongTermAction(b, 6);
        break;
    }
}

/* Plan 0x0e: visitor wave animation, then resume by person kind. */
// FUNCTION: LEGOLAND 0x00450250
void Visitor_WaveThenResume(Bloke* b)
{
    unsigned char act;

    act = b->action;
    switch (b->action) {
    case 0:
        NewDirForAction(b, 4);
        if (b->person->kind == 1) {
            b->flags |= 0x100;
            BlokeSetAnim(b, 2);
            BlokeSetFrame(b, 0);
            b->action++;
        } else {
            b->action = 2;
        }
        break;
    case 1:
        if (PlayBlokeAnim(b)) {
            BlokeWalkAnim(b);
            BlokeSetFrame(b, 0);
            b->flags &= ~0x100;
            b->action++;
        }
        break;
    case 2:
        b->state = 0xd;
        b->action = (unsigned char)(act + 1);
        break;
    case 3:
        switch (b->person->kind) {
        case 2:
            NewLongTermAction(b, 0x10);
            break;
        case 3:
            NewLongTermAction(b, 0x11);
            break;
        default:
            NewLongTermAction(b, 6);
            break;
        }
        break;
    }
}

/* Plan 0x0d: find an unreserved CAFE BROLLY, walk there, reserve, wait, leave.
 * First diverge i19: original `test dl,1` vs hoisted `mov ebx,1` / `test bl,dl`
 * (the already-pushed ebx steals the reservation mask). Loop-empty arm is
 * `jne again` + inline NewLongTermAction; ours still `je` to the shared tail.
 * SuggestNextMove leas and dest.y = by+stand_y match when aligned. */
// WIP-FUNCTION: LEGOLAND 0x0044fe80  (76.7%, i19 ebx=1 hoist / loop polarity)
void Visitor_ReserveCafeBrolly(Bloke* b)
{
    unsigned char act;
    Pos           leg;
    Cell*         cell;
    ObjClass*     cls;
    unsigned char a;

    act = b->action;
    switch (b->action) {
    case 0:
        cell = GetFirstObjectMatching(g_cafe_brolly_elem);
        if (!cell) {
            NewLongTermAction(b, 6);
            break;
        }
        for (;;) {
            cls = ((Elem*)cell->obj)->cls;
            if (!(cell->flags & 1))
                break;
            cell = GetNextObjectMatching(cell, g_cafe_brolly_elem);
            if (cell)
                continue;
            NewLongTermAction(b, 6);
            return;
        }
        b->owner = cell->xy;
        b->dest.x = (cls->stand_x + cell->xy.b.x) << 8;
        b->dest.y = (cell->xy.b.y + cls->stand_y) << 8;
        b->action++;
        if (!cell)
            NewLongTermAction(b, 6);
        break;
    case 1: {
        Pos* world = &b->world;
        Pos* dest = &b->dest;
        Pos* out = &leg;
        switch (SuggestNextMove(world, dest, out) + 3) {
        case 1:
            b->state = 0xa;
            break;
        case 0:
        case 2:
        case 3:
            b->state = 4;
            break;
        case 5:
            b->target.x = out->x;
            b->target.y = out->y;
            a = (unsigned char)(CalcMoveLine(*world, b->target, b->path) + 0x10);
            b->state = 6;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->result)
                NewLongTermAction(b, 6);
            else
                b->action = 2;
            break;
        case 4:
            b->target.x = out->x;
            b->target.y = out->y;
            a = (unsigned char)(CalcMoveLine(*world, b->target, b->path) + 0x10);
            b->state = 6;
            b->new_dir = a;
            NewDirForAction(b, (unsigned char)((a >> 5) + 3));
            if (b->result)
                NewLongTermAction(b, 6);
            else
                b->action = 1;
            break;
        }
        break;
    }
    case 2:
        cell = CellAt(b->owner.b.x, b->owner.b.y);
        if (cell->obj != g_cafe_brolly_elem || !(cell->flags & 0x80)) {
            NewLongTermAction(b, 6);
            break;
        }
        if (cell->flags & 1) {
            b->action = 0;
            break;
        }
        cell->flags |= 1;
        b->flags |= 8;
        b->target.x = b->dest.x - 0x80;
        b->target.y = b->dest.y;
        a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        b->new_dir = a;
        b->state = 7;
        NewDirForAction(b, 7);
        b->elapsed = 0;
        b->action++;
        break;
    case 3:
        cell = CellAt(b->owner.b.x, b->owner.b.y);
        if (cell->obj != g_cafe_brolly_elem || !(cell->flags & 0x80)) {
            NewLongTermAction(b, 6);
            break;
        }
        if (b->elapsed > 0x12c)
            b->action = (unsigned char)(act + 1);
        break;
    case 4:
        cell = CellAt(b->owner.b.x, b->owner.b.y);
        if (cell->obj != g_cafe_brolly_elem || !(cell->flags & 0x80)) {
            NewLongTermAction(b, 6);
            break;
        }
        cell->flags &= ~1;
        b->target.x = b->dest.x + 0x80;
        b->target.y = b->dest.y;
        a = (unsigned char)(CalcMoveLine(b->world, b->target, b->path) + 0x10);
        b->state = 7;
        b->new_dir = a;
        NewDirForAction(b, (unsigned char)((a >> 5) + 3));
        b->action++;
        break;
    case 5:
        b->flags &= ~8;
        NewLongTermAction(b, 6);
        break;
    }
}
