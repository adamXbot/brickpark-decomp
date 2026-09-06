/* LEGOLAND -- scope Z: the sixteen low-level bloke AI states and helpers.
 * VC6 SP3 /O2 /Gy /Gd. Names and recovered mechanics: docs/lanes/scope-z.md.
 * A world tile is 256 units; direction is +0x72 and speed is +0x7f.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct MoveLine { int x, y; short dx, dy; } MoveLine;
typedef struct Person { char pad00[8]; int kind; } Person;
typedef struct Bloke {
    struct Bloke* next;
    Person* person;
    char pad08[4];
    unsigned short plan, state, pending; /* +0x0c, +0x0e, +0x10 */
    char pad12[0x0e];
    int tile_wait;               /* +0x20 */
    Pos target;                  /* +0x24 */
    char pad2c[0x0e];
    unsigned short bounces;      /* +0x3a */
    char pad3c[8];
    short bounce_speed;          /* +0x44 */
    short bounce_delta;          /* +0x46 */
    char pad48[0x0c];
    unsigned int last_job;       /* +0x54 */
    char pad58[4];
    int tick;                    /* +0x5c */
    unsigned char action, pad61;
    unsigned short flags;        /* +0x62 */
    unsigned char result;        /* +0x64 */
    char pad65[3];
    Pos world;                   /* +0x68 */
    unsigned short height;       /* +0x70 */
    unsigned char dir, new_dir, frame, delay; /* +0x72..+0x75 */
    char pad76[9];
    unsigned char speed;         /* +0x7f */
    char pad80[0x18];
    MoveLine line;               /* +0x98 */
    char pada4[8];
} Bloke;
typedef struct Map { char pad00[0x14]; unsigned short width, height; } Map;
typedef struct Cell { char pad00[8]; unsigned short tile; char pad0a[10]; } Cell;
typedef struct TileElem {
    char pad00[0x18];
    unsigned char (*flags)(int, int); /* +0x18 */
    void (*enter)(int, int);     /* +0x1c */
    void (*leave)(int, int);     /* +0x20 */
} TileElem;
typedef struct TileInfo { TileElem* elem; unsigned int code; } TileInfo;
typedef struct Direction { short x, y; } Direction;

extern Map* g_map;                       /* 0x004bcbf4 */
extern Cell** g_map_rows;                /* 0x00801400 */
extern TileInfo g_tile_info[];           /* 0x00801f40 */
extern Direction g_heading_delta[8];     /* 0x004bd32c */
extern unsigned int g_sim_frame;         /* 0x008119a4 */
extern int g_hit_type;                   /* 0x004bdd00 */
extern Bloke* g_icon_value;              /* 0x004bdd04 */
typedef void (*LowAIFn)(Bloke*);
extern LowAIFn g_lowlevel_ai[];           /* 0x004bd34c */

extern int DBPrintf(const char*, ...);                        /* 0x00453a20 */
extern int rand(void);                                        /* 0x0049e4b2 */
extern unsigned short DoPendingAction(Bloke*);                 /* 0x00483240 */
extern Pos GetTileInDir(Pos, unsigned char);                   /* 0x004846a0 */
extern int OverNewTile(Bloke*, int, int);                      /* 0x00483650 */
extern unsigned char Get_RFFlags(int, int);                   /* 0x00461610 */
extern unsigned char GetCurrentRFFlags(int, int);              /* 0x00461630 */
extern unsigned short Get_MapFlags(int, int);                 /* 0x00461760 */
extern int HitPathEdge(Bloke*, int, int);                      /* 0x004834a0 */
extern int HitObstacle(Bloke*, int, int);                      /* 0x00483510 */
extern int RndWalk_LeftTile(Bloke*, int, int);                  /* 0x004837a0 */
extern int CrossTileCentre(Bloke*, int, int);                  /* 0x004837d0 */
extern int Handle_RndWalk_TileSpecifics(Bloke*, int, int);      /* 0x00483b10 */
extern void* FindPathSquare(Pos*);                            /* 0x00481790 */
extern int NewDirForAction(Bloke*, unsigned char);             /* 0x004833d0 */
extern void sub_483830(Bloke*);                                /* 0x00483830 */
extern void NavigMoveLine(MoveLine*, short, Pos*);             /* 0x004807f0 */
extern unsigned char Get_Path_Directions(Pos*, int, int);      /* 0x0045c050 */
extern unsigned char ExcludeIsolatedDiags(unsigned char);      /* 0x0045c830 */
extern unsigned char Dir_To_Bit(unsigned char);                /* 0x0045c010 */
extern unsigned char Bit_To_Dir(unsigned char);                /* 0x0045c020 */

extern int MapPointInBounds(int, int);                        /* 0x00483160 */
extern Pos HeadingDelta(unsigned char, short);                /* 0x004831a0 */
extern void BeginTileWait(Bloke*);                            /* 0x00483260 */
extern int TryTileWait(Bloke*, int, int);                       /* 0x00483300 */
extern int HitWorkerObstacle(Bloke*, int, int);                 /* 0x00483580 */
extern void NotifyTileTransition(Bloke*, int, int);             /* 0x00483680 */
extern void StepBlokeTurn(Bloke*);                             /* 0x00483850 */
extern void SetBlokeWaitingFrame(Bloke*);                      /* 0x00483890 */
extern int ResumeOnPath(Bloke*, int, int);                      /* 0x00483b60 */
extern int TurnIfBlocked(Bloke*, Pos);                     /* 0x00483c20 */
extern int BlokeNearTarget(Bloke*, int);                       /* 0x004841a0 */

/* The original inlines these bounds checks before tile-specific callbacks.
 * A NULL cell is deliberately not checked by the callers. */
/* Reversed parameter order evaluates the x shift before y in VC6. */
static __inline Cell* CellAt(int y, int x)
{
    if (x >= 0 && x < g_map->width && y >= 0 && y < g_map->height)
        return &g_map_rows[y][x];
    return 0;
}

static __inline int OnWalkablePath(Pos* pos)
{
    unsigned short mapflags, rf;
    if (pos->x >= 0 && pos->x < (g_map->width << 8) &&
        pos->y >= 0 && pos->y < (g_map->height << 8)) {
        mapflags = Get_MapFlags(pos->x, pos->y);
        rf = GetCurrentRFFlags(pos->x, pos->y);
        if ((rf & 1) || ((mapflags & 0x10) && !(rf & 2)))
            return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00483160
int MapPointInBounds(int x, int y)
{
    if (x > 0 && x < (g_map->width << 8) &&
        y > 0 && y < (g_map->height << 8))
        return 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x004831a0
Pos HeadingDelta(unsigned char dir, short speed)
{
    Pos delta;
    delta.x = (g_heading_delta[dir].x * speed) >> 8;
    delta.y = (g_heading_delta[dir].y * speed) >> 8;
    return delta;
}

// FUNCTION: LEGOLAND 0x00483260
void BeginTileWait(Bloke* b)
{
    Pos next;
    Cell* cell;
    TileElem* elem;
    b->flags |= 8;
    b->pending = b->state;
    b->state = 9;
    b->tile_wait = 0;
    next = GetTileInDir(b->world, b->dir);
    cell = CellAt((unsigned int)next.y >> 8, (unsigned int)next.x >> 8);
    elem = g_tile_info[cell->tile].elem;
    if (elem->enter)
        elem->enter(next.x, next.y);
}

// FUNCTION: LEGOLAND 0x00483300
int TryTileWait(Bloke* b, int x, int y)
{
    Cell* cell;
    TileElem* elem;
    unsigned char flags;
    if (OverNewTile(b, x, y) && MapPointInBounds(x, y) &&
        (Get_RFFlags(x, y) & 3) == 3) {
        cell = CellAt(y >> 8, x >> 8);
        elem = g_tile_info[cell->tile].elem;
        if (elem->flags)
            flags = elem->flags(x, y);
        else
            flags = 2;
        if ((flags & 3) == 3) {
            BeginTileWait(b);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00483580
int HitWorkerObstacle(Bloke* b, int x, int y)
{
    int current, next;
    int rf, mapflags;
    if (x >= 0 && x < (g_map->width << 8) &&
        y >= 0 && y < (g_map->height << 8)) {
        rf = GetCurrentRFFlags(b->world.x, b->world.y);
        mapflags = Get_MapFlags(b->world.x, b->world.y);
        current = (rf & 2) && !(mapflags & 0x8800);
        rf = GetCurrentRFFlags(x, y);
        mapflags = Get_MapFlags(x, y);
        next = (rf & 2) && !(mapflags & 0x8800);
        if (!current && next)
            return 1;
        return 0;
    }
    return 1;
}

/* Original bug: the leave call tests enter, not leave, for NULL. */
// FUNCTION: LEGOLAND 0x00483680
void NotifyTileTransition(Bloke* b, int x, int y)
{
    Cell* cell;
    TileElem* elem;
    if (OverNewTile(b, x, y)) {
        if ((Get_RFFlags(b->world.x, b->world.y) & 3) == 3) {
            int old_y = b->world.y;
            int old_x = b->world.x;
            cell = CellAt((unsigned int)old_y >> 8, (unsigned int)old_x >> 8);
            elem = g_tile_info[cell->tile].elem;
            if (elem->enter)
                elem->leave(old_x, old_y);
            b->flags &= ~8;
        }
        if ((Get_RFFlags(x, y) & 3) == 3) {
            cell = CellAt((unsigned int)y >> 8, (unsigned int)x >> 8);
            elem = g_tile_info[cell->tile].elem;
            if (elem->enter)
                elem->enter(x, y);
            b->flags |= 8;
        }
    }
}

// FUNCTION: LEGOLAND 0x00483850
void StepBlokeTurn(Bloke* b)
{
    if (--b->delay == 0) {
        signed char step;
        b->delay = 3;
        step = ((b->dir - b->new_dir) & 4) ? 1 : -1;
        b->dir = (b->dir + step) & 7;
    }
}

// FUNCTION: LEGOLAND 0x00483890
void SetBlokeWaitingFrame(Bloke* b)
{
    b->frame = 2;
}

// FUNCTION: LEGOLAND 0x004838a0
void LowAI_Unimplemented(Bloke* b)
{
    DBPrintf("Frame %d.. Bloke %d rethinking\n", g_sim_frame, b);
}

// FUNCTION: LEGOLAND 0x004838c0
void LowAI_Wait(Bloke* b)
{
    if (--b->delay == 0) {
        b->delay = 1;
        DoPendingAction(b);
    }
}

// FUNCTION: LEGOLAND 0x004838e0
void LowAI_Turn(Bloke* b)
{
    if (b->dir != b->new_dir)
        StepBlokeTurn(b);
    if (b->dir == b->new_dir) {
        b->delay = 1;
        DoPendingAction(b);
    }
}

// FUNCTION: LEGOLAND 0x00483b60
int ResumeOnPath(Bloke* b, int x, int y)
{
    if (!RndWalk_LeftTile(b, x, y) && CrossTileCentre(b, x, y) &&
        !(b->flags & 4) && OnWalkablePath(&b->world) && FindPathSquare(&b->world)) {
        b->state = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x00483c20
int TurnIfBlocked(Bloke* b, Pos next)
{
    int chance = (b->flags & 2) ? 0 : 20;
    unsigned char dir;
    if (b->person->kind != 2 && b->person->kind != 3) {
        if (HitPathEdge(b, next.x, next.y) || HitObstacle(b, next.x, next.y) || (rand() & 1023) < chance) {
            dir = rand() & 7;
            if (b->flags & 2) dir |= 1;
            NewDirForAction(b, dir);
            return 1;
        }
    } else {
        if (HitPathEdge(b, next.x, next.y) || HitWorkerObstacle(b, next.x, next.y)) {
            dir = rand() & 7;
            if (b->flags & 2) dir |= 1;
            NewDirForAction(b, dir);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGOLAND 0x004841a0
int BlokeNearTarget(Bloke* b, int radius)
{
    Pos delta;
    delta.x = b->target.x - b->world.x;
    delta.y = b->target.y - b->world.y;
    return radius * radius >= delta.x * delta.x + delta.y * delta.y;
}

/* Unreferenced in the shipped image. */
// FUNCTION: LEGOLAND 0x004841e0
int MoveLineNearTarget(Bloke* b)
{
    Pos delta;
    delta.x = b->target.x - b->line.x;
    delta.y = b->target.y - b->line.y;
    return delta.x * delta.x + delta.y * delta.y <= 1024;
}

// FUNCTION: LEGOLAND 0x004848e0
void LowAI_WaitWhilePointed(Bloke* b)
{
    if (!(g_hit_type & 0x200) || g_icon_value != b) {
        b->flags &= ~8;
        DoPendingAction(b);
    }
}

// FUNCTION: LEGOLAND 0x00483d10
void LowAI_Wander(Bloke* b)
{
    Pos next = HeadingDelta(b->dir, b->speed);
    next.x += b->world.x;
    next.y += b->world.y;
    if (!TryTileWait(b, next.x, next.y)) {
        if (!Handle_RndWalk_TileSpecifics(b, next.x, next.y) &&
            !TurnIfBlocked(b, next)) {
            NotifyTileTransition(b, next.x, next.y);
            b->world = next;
            sub_483830(b);
        }
        if (!(b->tick & 127)) {
            b->state = 0;
            b->tick = 0;
        }
    }
}

// FUNCTION: LEGOLAND 0x00483d90
void LowAI_WorkerWander(Bloke* b)
{
    Pos next = HeadingDelta(b->dir, b->speed);
    next.x += b->world.x;
    next.y += b->world.y;
    if (g_sim_frame - b->last_job >= 50 && !TryTileWait(b, next.x, next.y)) {
        if (!Handle_RndWalk_TileSpecifics(b, next.x, next.y) &&
            !TurnIfBlocked(b, next)) {
            NotifyTileTransition(b, next.x, next.y);
            b->world = next;
            sub_483830(b);
        }
        if (!(b->tick & 127)) {
            b->state = 0;
            b->tick = 0;
        }
    }
}

// FUNCTION: LEGOLAND 0x00483e20
void LowAI_SeekPath(Bloke* b)
{
    Pos next;
    if (OnWalkablePath(&b->world)) {
        b->state = 0;
        return;
    }
    next = HeadingDelta(b->dir, b->speed);
    next.x += b->world.x;
    next.y += b->world.y;
    if (!TryTileWait(b, next.x, next.y) &&
        !ResumeOnPath(b, next.x, next.y) && !TurnIfBlocked(b, next)) {
        NotifyTileTransition(b, next.x, next.y);
        b->world = next;
        sub_483830(b);
    }
}

// FUNCTION: LEGOLAND 0x00483ef0
void LowAI_FollowPath(Bloke* b)
{
    Pos next = HeadingDelta(b->dir, b->speed);
    unsigned short rf;
    Pos tile;
    unsigned char bits, mask, dir;
    next.x += b->world.x;
    next.y += b->world.y;
    if (!TryTileWait(b, next.x, next.y)) {
        RndWalk_LeftTile(b, next.x, next.y);
        if (CrossTileCentre(b, next.x, next.y)) {
            b->flags |= 4;
            rf = GetCurrentRFFlags(b->world.x, b->world.y);
            if (!OnWalkablePath(&b->world) && !(Get_MapFlags(b->world.x, b->world.y) & 0x10)) {
                b->state = 0;
                b->result |= 2;
                return;
            }
            if (rf & 0x24) {
                b->state = 0;
                b->result |= 4;
                return;
            }
            if (rf & 8) {
                tile.x = next.x >> 8;
                tile.y = next.y >> 8;
                bits = Get_Path_Directions(&tile, 0, 0);
                mask = ExcludeIsolatedDiags(bits);
                mask &= ~Dir_To_Bit(b->dir + 4);
                dir = Bit_To_Dir(mask);
                NewDirForAction(b, dir);
            }
        }
        NotifyTileTransition(b, next.x, next.y);
        b->world = next;
        sub_483830(b);
    }
}

// FUNCTION: LEGOLAND 0x00484090
void LowAI_WalkToJunction(Bloke* b)
{
    Pos next = HeadingDelta(b->dir, b->speed);
    unsigned short rf;
    next.x += b->world.x;
    next.y += b->world.y;
    RndWalk_LeftTile(b, next.x, next.y);
    if (CrossTileCentre(b, next.x, next.y)) {
        b->flags |= 4;
        rf = GetCurrentRFFlags(b->world.x, b->world.y);
        if (!OnWalkablePath(&b->world)) {
            b->state = 0;
            b->result |= 2;
            return;
        }
        if (rf & 0x24) {
            b->state = 0;
            b->result |= 4;
            return;
        }
        if (rf & 8) {
            b->state = 0;
            return;
        }
    }
    NotifyTileTransition(b, next.x, next.y);
    b->world = next;
    sub_483830(b);
}

// FUNCTION: LEGOLAND 0x00484220
void LowAI_WalkLineOnPath(Bloke* b)
{
    Pos next;
    if (BlokeNearTarget(b, b->speed * 2)) {
        b->state = 0;
        return;
    }
    NavigMoveLine(&b->line, b->speed, &next);
    if (RndWalk_LeftTile(b, next.x, next.y)) {
        if (HitObstacle(b, next.x, next.y)) {
            b->state = 0;
            b->result |= 1;
            return;
        }
        if (!OnWalkablePath(&next)) {
            b->state = 0;
            b->result |= 2;
            return;
        }
    }
    NotifyTileTransition(b, next.x, next.y);
    b->world = next;
    sub_483830(b);
}

// FUNCTION: LEGOLAND 0x00484350
void LowAI_WalkLineToPath(Bloke* b)
{
    Pos next;
    if (b->flags & 2) {
        b->state = 0;
        return;
    }
    if (BlokeNearTarget(b, b->speed * 2)) {
        b->state = 0;
        return;
    }
    NavigMoveLine(&b->line, b->speed, &next);
    if (RndWalk_LeftTile(b, next.x, next.y)) {
        if (HitObstacle(b, next.x, next.y) || OnWalkablePath(&next)) {
            b->state = 0;
            return;
        }
    }
    NotifyTileTransition(b, next.x, next.y);
    b->world = next;
    sub_483830(b);
}

// FUNCTION: LEGOLAND 0x00484470
void LowAI_WalkLine(Bloke* b)
{
    Pos next;
    if (BlokeNearTarget(b, b->speed * 2)) {
        b->state = 0;
        return;
    }
    NavigMoveLine(&b->line, b->speed, &next);
    if (RndWalk_LeftTile(b, next.x, next.y) && HitObstacle(b, next.x, next.y)) {
        b->state = 0;
        b->result |= 1;
        return;
    }
    NotifyTileTransition(b, next.x, next.y);
    b->world = next;
    sub_483830(b);
}

// FUNCTION: LEGOLAND 0x00484520
void LowAI_WorkerWalkLine(Bloke* b)
{
    Pos next;
    if (BlokeNearTarget(b, b->speed * 2)) {
        b->state = 0;
        return;
    }
    NavigMoveLine(&b->line, b->speed, &next);
    if (RndWalk_LeftTile(b, next.x, next.y) && HitWorkerObstacle(b, next.x, next.y)) {
        b->state = 0;
        b->result |= 1;
        return;
    }
    NotifyTileTransition(b, next.x, next.y);
    b->world = next;
    sub_483830(b);
}

// FUNCTION: LEGOLAND 0x004845d0
void LowAI_WalkLineResume(Bloke* b)
{
    Pos next;
    if (BlokeNearTarget(b, b->speed * 2)) {
        DoPendingAction(b);
        return;
    }
    NavigMoveLine(&b->line, b->speed, &next);
    b->world = next;
    sub_483830(b);
}

/* Retains the unsigned height comparison and post-decrement bounce counter. */
// FUNCTION: LEGOLAND 0x00484630
void LowAI_BounceTurn(Bloke* b)
{
    if (b->height == 0) {
        b->bounce_delta = b->bounce_speed;
        if (b->bounces-- == 0) {
            DoPendingAction(b);
            return;
        }
    }
    b->height += b->bounce_delta;
    b->bounce_delta--;
    if (b->height <= 0)
        b->height = 0;
    b->frame = 0;
    b->new_dir = (b->dir + 1) & 7;
    StepBlokeTurn(b);
}

/* Original leave-callback guard tests enter instead of leave, as above. */
// FUNCTION: LEGOLAND 0x00484790
void LowAI_WaitForTile(Bloke* b)
{
    Pos next = GetTileInDir(b->world, b->dir);
    Cell* cell;
    TileElem* elem;
    unsigned char flags;
    /* Intentionally continues querying the tile after resuming pending AI. */
    if ((Get_RFFlags(next.x, next.y) & 3) != 3) {
        b->flags &= ~8;
        DoPendingAction(b);
    }
    cell = CellAt(next.y >> 8, next.x >> 8);
    elem = g_tile_info[cell->tile].elem;
    if (elem->flags)
        flags = elem->flags(next.x, next.y);
    else
        flags = 2;
    switch (flags & 3) {
    case 1:
    case 2:
        cell = CellAt((unsigned int)next.y >> 8, (unsigned int)next.x >> 8);
        elem = g_tile_info[cell->tile].elem;
        if (elem->enter)
            elem->leave(next.x, next.y);
        b->flags &= ~8;
        DoPendingAction(b);
        g_lowlevel_ai[b->state](b);
        break;
    default:
        b->tile_wait++;
        SetBlokeWaitingFrame(b);
    }
}
