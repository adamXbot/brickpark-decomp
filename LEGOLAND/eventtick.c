/* LEGOLAND -- scope V: the script-event tick handlers, kinds 2..37.
 *
 * UpdateHelpTick (fpui3.c) walks the pending event list every frame and
 * calls g_event_tick[e->kind](e) for each due event. A handler returns 1
 * when the event is done (it is freed unless its flags say it survives the
 * step) and 0 when it is still waiting. Kinds 2..32 are the step events the
 * keyword handlers queued (act once); kinds 33..37 are the first goal
 * events (NEED, NEEDAT, NEEDIN, CONNECT, LINK), which re-check their
 * condition each tick and queue a hint through scope X's goal primitives
 * while it is unmet. Also here: the helpers only these handlers use.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; the
 * ScriptEvent fields are eventmake.c's, the map records gameframe.c's.
 * Names are ours (the integrator's provisional readings in
 * docs/SCOPE_V_event_ticks_1.md, renamed where the body says otherwise --
 * recorded in docs/lanes/scope-v.md). Offsets and addresses are load-bearing.
 */

/* ---- types --------------------------------------------------------------- */
typedef struct Pos  { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; } Rect;
typedef struct BPos  { unsigned char x, y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;

typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    void*         elem;          /* +0x04 */
    char*         text;          /* +0x08 */
    int           kind;          /* +0x0c */
    unsigned char flags;         /* +0x10 */
    char          pad11[3];
    int           f14;           /* +0x14  per-kind arguments (eventmake.c) */
    int           f18;           /* +0x18 */
    int           f1c;           /* +0x1c */
    Pos           pos;           /* +0x20 */
    Rect          area;          /* +0x28 */
    int           mode;          /* +0x38 */
    int           time;          /* +0x3c */
    int           strid;         /* +0x40 */
} ScriptEvent;                   /* 0x44 */

/* An LLIDB element: the type/flag word at +0x08 (1 loaded, 2 unavailable,
 * 0x10000 new, 0x20000 given), the parsed class record at +0x0c. */
typedef struct ObjDef ObjDef;
typedef struct LLElem {
    char*        name;           /* +0x00 */
    char*        image;          /* +0x04 */
    unsigned int flags;          /* +0x08 */
    ObjDef*      data;           /* +0x0c */
} LLElem;

/* A placed instance in a class's instance list: its map square as bytes at
 * +0x0e/+0x0f. */
typedef struct Inst {
    struct Inst*  next;          /* +0x00 */
    char          pad04[0x0a];   /* +0x04 */
    unsigned char x;             /* +0x0e */
    unsigned char y;             /* +0x0f */
} Inst;

/* A placed object as the map cells refer to it (gameframe.c's MapObj). */
typedef struct MapObj { void* f0; void* f4; void* f8; ObjDef* def; } MapObj;

/* The class record (fpui2.c's ObjDef); only what the handlers read. */
struct ObjDef {
    char          pad00[4];
    Inst*         insts;         /* +0x04  instance list */
    int           inst_count;    /* +0x08 */
    int           dx;            /* +0x0c  added to an instance's square for CONNECT/LINK */
    int           dy;            /* +0x10 */
    char          pad14[0x0c];   /* +0x14 */
    short         type;          /* +0x20 */
    char          pad22[0x0a];   /* +0x22 */
    unsigned char rate;          /* +0x2c  DEGRADE: damage per unit */
    char          pad2d[0x0f];   /* +0x2d */
    int           left;          /* +0x3c  footprint, relative to the base square */
    int           top;           /* +0x40 */
    int           right;         /* +0x44 */
    int           bottom;        /* +0x48 */
    char          pad4c[0x0c];   /* +0x4c */
    LLElem*       parent;        /* +0x58  the menu it hangs off (fpui2.c) */
    LLElem*       theme;         /* +0x5c  its theme element */
    char          pad60[0x30];   /* +0x60 */
    void        (*place)(LLElem* elem, Pos* pt, int a);   /* +0x90 */
    void        (*query)(MapObj* inst, Pos* sq);          /* +0x94 */
    char          pad98[0x2c];   /* +0x98 */
    MapObj*       inst;          /* +0xc4 */
};

/* A map cell (gameframe.c's Cell, 0x14 bytes) plus the damage byte. */
typedef struct Cell {
    MapObj*        obj;          /* +0x00 */
    unsigned char  x;            /* +0x04 */
    unsigned char  y;            /* +0x05 */
    unsigned short pad06;        /* +0x06 */
    unsigned short tile;         /* +0x08 */
    unsigned short base;         /* +0x0a */
    unsigned short flags;        /* +0x0c  0x80 has an object, 0x40 glued, 0x10 path */
    unsigned short uflags;       /* +0x0e */
    unsigned char  pad10;        /* +0x10 */
    unsigned char  damage;       /* +0x11 */
    unsigned char  pad12[2];     /* +0x12 */
} Cell;

/* The map header (gameframe.c's MapHdr); only what the handlers touch. */
typedef struct MapHdr {
    char           pad00[0x10];
    unsigned short view_w;       /* +0x10 */
    unsigned short view_h;       /* +0x12 */
    unsigned short w;            /* +0x14  map size in cells */
    unsigned short h;            /* +0x16 */
    char           pad18[0x14];  /* +0x18 */
    int            f2c;          /* +0x2c  FEATURE Adv_Tune */
    char           pad30[4];     /* +0x30 */
    int            f34;          /* +0x34  WORKERS second word != 0 */
    int            f38;          /* +0x38  WORKERS first word != 0 */
    int            f3c;          /* +0x3c  FEATURE Autorepair */
} MapHdr;

/* The destroy cursor block (objmap2.c's Cursor, 0x1834 bytes), copied whole;
 * its origin at +0x1404 is what the tree calls g_query_block (0x00811564). */
typedef struct Cursor {
    char pad0000[0x1404];
    Pos  origin;                 /* +0x1404 */
    char pad140c[0x1834 - 0x140c];
} Cursor;

/* CLEAR's locals that must share one object (see EventTick_Clear). */
typedef struct ClearLocals {
    Pos    sq;                   /* +0x00  the square handed to the query callback */
    int    pad0;                 /* +0x08  the footprint Rect's never-stored left */
    int    top;                  /* +0x0c  footprint top + y, spilled */
    int    pad1;                 /* +0x10  never-stored right */
    int    bottom;               /* +0x14  footprint bottom + y, spilled */
    Cursor saved;                /* +0x18  the destroy cursor, saved whole */
} ClearLocals;

/* ---- globals ------------------------------------------------------------- */
extern MapHdr*    g_map;                 /* 0x004bcbf4 */
extern Cell**     g_map_rows;            /* 0x00801400 */
extern int        g_map_dirty;           /* 0x00668610 */
extern int        g_menu_dirty;          /* 0x0066871c */
extern int        g_script_purge;        /* 0x00668788 */
extern int        g_need_shortfall;      /* 0x0066878c  NEED: objects still missing (first named here) */
extern unsigned char g_theme_new_count[4]; /* 0x007fe114  new objects per theme: LEGOLAND/COMMON, WESTERN, CASTLE, ADVENTURERS (bigscreens.c's DragState) */
extern ObjDef*    g_sel_def;             /* 0x00667c58 */
extern BPosW      g_sel_bpos;            /* 0x00667c54 */
extern Cursor     g_destroy_cursor;      /* 0x00810160 */
extern Pos        g_view;                /* 0x007fffc4 */
extern char       g_obj_list[];          /* 0x007febc0  head of the placed-object list */
extern void*      g_clear_sfx;           /* 0x004b92fc  the CLEAR demolition sample (first named here) */
extern int        g_game_mode;           /* 0x008119b4 */
extern int        g_icons2_mode;         /* 0x00668e38 */
extern int        g_cur_screen;          /* 0x0080ff84 */
extern int        g_screen_mode;         /* 0x0080ff88 */
extern int        g_path_overlay_active; /* 0x00832984  FEATURE 0 Terraces */
extern int        g_ride_wear;           /* 0x00832980  FEATURE 1 RideWear */
extern int        g_auto_stud;           /* 0x00832988  FEATURE 3 AutoStud (first named here) */
extern int        g_power_available;     /* 0x0083298c  FEATURE 4 Energy */
extern int        g_visitor_tire;        /* 0x00832990  FEATURE 5 Hunger */
extern int        g_inspector_on;        /* 0x00832978  FEATURE 6 Inspector; ENDLEVEL waits on it (first named here) */
extern int        g_bricks_full;         /* 0x00832974  FEATURE 9 MoneyBar */
extern int        g_show_capacity;       /* 0x00832994  FEATURE 10 CapacityCalc */
extern int        g_832ba8;              /* 0x00832ba8  FEATURE 11 FreePlayAtEnd */
extern int        g_visitor_cap;         /* 0x00832920 */
extern int        g_visitor_cap_extra;   /* 0x00832924 */
extern int        g_entrance_fee;        /* 0x00832970 */
extern int        g_scroll_x;            /* 0x00667cb4  24.8 */
extern int        g_scroll_y;            /* 0x00667cb8 */
extern void     (*g_report_setters[25])(int a, int b); /* 0x004b7e38  REPORT: per-index setter (first named here) */

/* ---- callees ------------------------------------------------------------- */
extern int   DBPrintf(const char* fmt, ...);                             /* 0x00453a20 */
extern int   LLIDB_FindElement(const char* name, LLElem** out, unsigned int* idx); /* 0x0047b330 */
extern void  MarkElemAvailable(LLElem* e, int popup, int b);             /* 0x00469900 */
extern int   GetBrickCount(void);                                        /* 0x004578e0 */
extern void  SetCurrency(int bricks);                                    /* 0x00457900 */
extern void  sub_457870(int a);                                          /* 0x00457870 (Codex-F) */
extern void  sub_4969d0(void);                                           /* 0x004969d0 */
extern void  GetTileCentre(Pos* tile, Pos* out);                         /* 0x0045ad60 */
extern void  RefreshObjList(void* head);                                 /* 0x0045d770 */
extern void  PutObjOnMap(ObjDef* d, LLElem* elem, Pos* pos);             /* 0x00459ad0 */
extern void  SetSampleFade(void* sample, int rate);                      /* 0x00492af0 */
extern void* PlayInstanceOfSample(void* sample, int a, int b, void* src); /* 0x00496d20 */
extern void  SetSampleLooping(void* s);                                  /* 0x00496d10 */
extern void  AddSFX_Callback(void* sfx, int delay, void* cb);            /* 0x00496db0 */
extern Cell* GetFirstRenderObject(void);                                 /* 0x0045a850 */
extern Cell* GetNextRenderObject(Cell* c);                               /* 0x0045a8b0 */
extern int   FindObjectsPower(ObjDef* d);                                /* 0x00459fa0 */
extern void  BuildCursorPtr(Cursor* c, int a, int b);                    /* 0x0045f5f0 */
extern int   CursorIsValid(Cursor* c);                                   /* 0x0045f4b0 */
extern void  RemoveObjectPathTiles(ObjDef* def, Pos* pos);               /* 0x0045d3d0 */
extern void  RemObjFromMap(ObjDef* d, MapObj* o, BPosW sq, Cursor* c);   /* 0x00459c90 */
extern void  CalculateMapRenderOrder(void);                              /* 0x0045a4a0 */
extern int   FreezeGameClock(void);                                      /* 0x00499380 */
extern void  ThawGameClock(void);                                        /* 0x004993c0 */
extern int   SetPointer(int shape);                                      /* 0x00463850 */
extern void  sub_496e60(int a, int b);                                   /* 0x00496e60 */
extern void  PlayMovie(const char* name, int a, int b);                  /* 0x004771f0 */
extern void  KillAdvisorHelp(void);                                      /* 0x0046ce20 */
extern void  RestoreScriptStepHelp(void);                                /* 0x0046b760 */
extern void  ShowInfoPanel(int kind);                                    /* 0x00490600 */
extern int   LoadHelpTextFor(const char* key);                           /* 0x004907a0 */
extern void  ResetAppraisalDeadline(void);                                           /* 0x0044db40 */
extern void  PopInfoSizeMayChange(void);                                 /* 0x00471550 */
extern void* GenerateGardener(Pos* pos, int in_hut);                     /* 0x0049a1a0 */
extern void* GenerateMechanic(Pos* pos, int in_hut);                     /* 0x0049a340 */
extern void  UpdateDamagedCell(Cell* cell, Pos* pos);                    /* 0x00463460 */
extern void  SetSimTuningA(int i, int v);                                /* 0x00462e50 */
extern void  SetSimTuningB(int i, int v);                                /* 0x00462e70 */
extern void  StopScript(int stop);                                       /* 0x0046b240 */
extern void  GetTileDimensions(int* out_w, int* out_h);                  /* 0x00460540 */
extern void  SetThemeIcon(int icon, char on);                            /* 0x00468860 */
extern void  AddLevelFlag(int flag, char on);                            /* 0x00468890 */
extern void  SetBridges(int count, char on);                             /* 0x004688f0 */
extern void  SetBriefingFile(const char* name);                          /* 0x004687f0 */
extern void  SetHintsFile(const char* name);                             /* 0x00468810 */
extern void  FlashButton(int bits, int on);                              /* 0x00476070 */
extern int   ObjCount(void* elem);                                       /* 0x00480d30 */
extern int   TileJoinsPathNetwork(Pos* pos);                             /* 0x00482b60 */
/* scope X's hint primitives: queue a "need N of this", "connect this" or
 * "link this" hint when the hint timer has run (our reading; see
 * eventgoal.c for the four they are built on). */
extern void  GoalCheck_Need(ScriptEvent* e, void* elem, int count);       /* 0x00468d80 */
extern void  GoalCheck_Connect(ScriptEvent* e, void* elem);               /* 0x00468dc0 */
extern void  GoalCheck_Link(ScriptEvent* e, void* elem);                  /* 0x00468e00 */

/* ---- this file ----------------------------------------------------------- */
void CountNewThemeElem(LLElem* e);
void MarkElemNew(LLElem* e);
int  EventTick_Unimplemented(ScriptEvent* e);
void PlaceScriptObject(LLElem* elem, Pos* pos);
int  ClearSfxFade(void* sample);
void SetFeatureFlags(int idx, int v);
void SetReportParam(int idx, int a, int b);
int  IsLinkableClass(ObjDef* d);

/* The map cell at (x, y), or NULL off the map. Every caller but CONNECT and
 * LINK dereferences the result without testing it (original behaviour). */
static __inline Cell* CellAt(int x, int y)
{
    if (x >= 0 && x < g_map->w && y >= 0 && y < g_map->h)
        return &g_map_rows[y][x];
    return 0;
}

/* CLEAR pass 0 keeps looking past an object while it is unpowered and its
 * class hangs off a flag-0x10 menu (the power is looked up before the menu). */
static __inline int ClearSkipsUnpoweredChild(Cell* next)
{
    ObjDef* d = next->obj->def;
    int     power = FindObjectsPower(d);
    LLElem* parent = d->parent;

    return parent != 0 && (parent->flags & 0x10) && power <= 0;
}


/* ========================================================================= */

/* Count a newly available class under its theme's new-object counter
 * (LEGOLAND and COMMON share the first). The brief's RefreshThemeElements. */
// FUNCTION: LEGOLAND 0x00469980
void CountNewThemeElem(LLElem* e)
{
    ObjDef* d = e->data;
    LLElem* t;

    if (LLIDB_FindElement("LEGOLAND THEME", &t, 0) == 0) {
        if (d->theme == t) {
            g_theme_new_count[0]++;
            return;
        }
        if (LLIDB_FindElement("COMMON THEME", &t, 0) == 0 && d->theme == t) {
            g_theme_new_count[0]++;
            return;
        }
    }
    if (LLIDB_FindElement("WESTERN THEME", &t, 0) == 0 && d->theme == t) {
        g_theme_new_count[1]++;
        return;
    }
    if (LLIDB_FindElement("CASTLE THEME", &t, 0) == 0 && d->theme == t) {
        g_theme_new_count[2]++;
        return;
    }
    if (LLIDB_FindElement("ADVENTURERS THEME", &t, 0) == 0 && d->theme == t)
        g_theme_new_count[3]++;
}

/* Kind 3: a loaded class becomes available and new (flag 2 off, 0x10000
 * on), counted under its theme; the menu is rebuilt either way. The
 * brief's RefreshThemeMenu. */
// FUNCTION: LEGOLAND 0x00469a80
void MarkElemNew(LLElem* e)
{
    if (e && (e->flags & 1)) {
        e->flags = (e->flags & ~2) | 0x10000;
        CountNewThemeElem(e);
    }
    g_menu_dirty = 1;
}

/* TAKE: a loaded class becomes unavailable (flags 2 and 0x10000 off);
 * movie3.c's name. */
// FUNCTION: LEGOLAND 0x00469ab0
void MarkElemUnavailable(LLElem* e)
{
    if (e) {
        if (e->flags & 1)
            e->flags &= ~0x10002;
    }
    g_menu_dirty = 1;
}

// FUNCTION: LEGOLAND 0x00469ae0
int EventTick_Unimplemented(ScriptEvent* e)
{
    DBPrintf("Processing unimplemented reward [ Type = %d ]\n", e->kind);
    return 1;
}

// FUNCTION: LEGOLAND 0x00469b00
int EventTick_Unimplemented2(ScriptEvent* e)
{
    DBPrintf("Processing unimplemented objective [ Type = %d ]\n", e->kind);
    return 1;
}

/* GIVE: make the class available (with or without the pop-up) and mark it
 * given. */
// FUNCTION: LEGOLAND 0x00469b20
int EventTick_Give(ScriptEvent* e)
{
    MarkElemAvailable(e->elem, e->f14, 0);
    ((LLElem*)e->elem)->flags |= 0x20000;
    return 1;
}

// FUNCTION: LEGOLAND 0x00469b50
int EventTick_Kind3(ScriptEvent* e)
{
    MarkElemNew(e->elem);
    return 1;
}

// FUNCTION: LEGOLAND 0x00469b70
int EventTick_Take(ScriptEvent* e)
{
    MarkElemUnavailable(e->elem);
    return 1;
}

// FUNCTION: LEGOLAND 0x00469b90
int EventTick_Addbricks(ScriptEvent* e)
{
    SetCurrency(GetBrickCount() + e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x00469bb0
int EventTick_Currency(ScriptEvent* e)
{
    SetCurrency(e->f1c);
    return 1;
}

/* PLACE: put one instance of the class on the map at the tile, through the
 * class's own place callback, with the view pointed at the tile centre. */
// FUNCTION: LEGOLAND 0x00469bd0
void PlaceScriptObject(LLElem* elem, Pos* pos)
{
    Pos     centre;
    ObjDef* d = elem->data;

    sub_457870(0);
    GetTileCentre(pos, &centre);
    g_view.x = centre.x;
    g_view.y = centre.y;
    d->place(elem, &centre, 0x8f8);
    RefreshObjList(g_obj_list);
    PutObjOnMap(elem->data, elem, &g_view);
    sub_457870(1);
}

// FUNCTION: LEGOLAND 0x00469c40
int EventTick_Place(ScriptEvent* e)
{
    PlaceScriptObject(e->elem, &e->pos);
    return 1;
}

/* The CLEAR sample's 3 s callback: fade it out. */
// FUNCTION: LEGOLAND 0x00469c60
int ClearSfxFade(void* sample)
{
    SetSampleFade(sample, -100);
    return 0;
}

/* CLEAR: demolish every object whose footprint overlaps the area, in three
 * passes over the render list -- first the unpowered objects hanging off a
 * flag-0x10 menu, then the rest of the unpowered ones, then everything --
 * each through the destroy cursor, which is saved and restored around it.
 *
 * Exact. Three things in this body are load-bearing and look odd; each is
 * documented with its measurements in docs/lanes/scope-v.md:
 *
 * - The query square and the saved cursor live in ONE local aggregate
 *   (ClearLocals) with the two spilled footprint sums between them. The
 *   6196-byte copy into L.saved is then a block copy into a sibling member
 *   of the same aggregate, which is the one memory-kill VC6 SP3 has that
 *   costs nothing: it stops the store of L.sq.y being forwarded to its
 *   later byte read, so g_sel_bpos.b.y comes back out of L.sq.y's home
 *   (`mov dl, [esp+0x1c]`) exactly as the original does. The two `pad`
 *   members are the Rect's never-stored left/right slots; top/bottom are
 *   members because the original stores and reloads exactly those two.
 * - The base x reaches the cursor stores through `t.x = bx + (int)d - (int)d`.
 *   The add/sub cancel in instruction selection, but the allocator has
 *   already treated t.x as a web distinct from bx, so bx keeps ebp with no
 *   byte need and the byte-capable copy `mov ecx, ebp` feeds sq.x, origin.x
 *   and g_sel_bpos.b.x. Every identity expression written directly folds
 *   back onto bx and puts x in edx.
 * - The footprint sums are computed through a register Rect and only the
 *   two y-sums are stored into the aggregate, in the statement order below,
 *   which reproduces the original's load/add/spill interleave. */
// FUNCTION: LEGOLAND 0x00469c80
int EventTick_Clear(ScriptEvent* e)
{
    ClearLocals L;
    ObjDef*  saved_def;
    int      pass;
    Cell*    c;
    Cell*    next;
    ObjDef*  d;
    Rect     f;
    int      bx, by;
    Pos      t;
    void*    sfx;

    sfx = PlayInstanceOfSample(g_clear_sfx, 1, 1, 0);
    SetSampleLooping(sfx);
    AddSFX_Callback(sfx, 3000, ClearSfxFade);
    for (pass = 0; pass < 3; pass++) {
        for (c = GetFirstRenderObject(); c; c = next) {
            sub_4969d0();
            switch (pass) {
            case 0:
                next = c;
                do {
                    next = GetNextRenderObject(next);
                } while (next && ClearSkipsUnpoweredChild(next));
                break;
            case 1:
                next = c;
                do {
                    next = GetNextRenderObject(next);
                } while (next && FindObjectsPower(next->obj->def) <= 0);
                break;
            case 2:
                next = GetNextRenderObject(c);
                break;
            }
            if (c->flags & 0x80) {
                d = c->obj->def;
                bx = c->x;
                by = c->y;
                f.top = d->top;
                f.bottom = d->bottom;
                f.left = d->left;
                L.top = f.top + by;
                f.right = d->right;
                L.bottom = f.bottom + by;
                f.left += bx;
                f.right += bx;
                if (f.left <= e->area.right && f.right >= e->area.left &&
                    L.top <= e->area.bottom && L.bottom >= e->area.top) {
                    saved_def = g_sel_def;
                    L.sq.y = by;
                    L.saved = g_destroy_cursor;
                    t.y = (int)d;
                    t.x = bx;
                    t.x += t.y;
                    t.x -= t.y;
                    g_destroy_cursor.origin.y = by;
                    L.sq.x = t.x;
                    g_destroy_cursor.origin.x = t.x;
                    g_sel_bpos.b.x = (unsigned char)t.x;
                    g_sel_def = d;
                    g_sel_bpos.b.y = (unsigned char)L.sq.y;
                    d->query(d->inst, &L.sq);
                    BuildCursorPtr(&g_destroy_cursor, 0, 0);
                    if (CursorIsValid(&g_destroy_cursor)) {
                        RemoveObjectPathTiles(g_sel_def, &L.sq);
                        RemObjFromMap(g_sel_def, g_sel_def->inst, g_sel_bpos, &g_destroy_cursor);
                    }
                    g_destroy_cursor = L.saved;
                    g_sel_def = saved_def;
                }
            }
        }
    }
    CalculateMapRenderOrder();
    PlayInstanceOfSample(g_clear_sfx, 0, 1, 0);
    return 1;
}

/* UNGLUE / GLUE: clear or set the glued bit (0x40) of every cell in the area. */
// FUNCTION: LEGOLAND 0x00469ed0
int EventTick_Unglue(ScriptEvent* e)
{
    int x, y;

    for (y = e->area.top; y <= e->area.bottom; y++)
        for (x = e->area.left; x <= e->area.right; x++)
            g_map_rows[y][x].flags &= ~0x40;
    return 1;
}

// FUNCTION: LEGOLAND 0x00469f20
int EventTick_Glue(ScriptEvent* e)
{
    int x, y;

    for (y = e->area.top; y <= e->area.bottom; y++)
        for (x = e->area.left; x <= e->area.right; x++)
            g_map_rows[y][x].flags |= 0x40;
    return 1;
}

// FUNCTION: LEGOLAND 0x00469f70
int EventTick_Extendpark(ScriptEvent* e)
{
    return EventTick_Unimplemented(e);
}

/* FMV: play the movie with the clock frozen, then put the step help back. */
// FUNCTION: LEGOLAND 0x00469f80
int EventTick_Fmv(ScriptEvent* e)
{
    FreezeGameClock();
    SetPointer(0);
    sub_496e60(1, 15);
    PlayMovie(e->text, 1, 1);
    ThawGameClock();
    KillAdvisorHelp();
    RestoreScriptStepHelp();
    return 1;
}

/* INTERVAL: open the info panel on the named help text and switch to the
 * front-end interval screen; a text that will not load is retried three
 * times, then given up with a debug line. */
// FUNCTION: LEGOLAND 0x00469fc0
int EventTick_Interval(ScriptEvent* e)
{
    ShowInfoPanel(0);
    if (LoadHelpTextFor(e->text)) {
        g_game_mode = 2;
        g_icons2_mode = 1;
        g_cur_screen = -1;
        g_screen_mode = 7;
        return 1;
    }
    if (e->f1c++ < 3)
        return 0;
    DBPrintf("Giving up trying to load Interval %s\n", e->text);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a030
int EventTick_Message(ScriptEvent* e)
{
    return EventTick_Unimplemented(e);
}

/* FEATURE <name> <v>: the twelve feature switches (levelkw3.c's
 * g_feature_names order); PlantWear (2) has no store. Ends in a tail call. */
// FUNCTION: LEGOLAND 0x0046a040
void SetFeatureFlags(int idx, int v)
{
    switch (idx) {
    case 0:  g_path_overlay_active = v; break;
    case 1:  g_ride_wear = v; break;
    case 2:  break;
    case 3:  g_auto_stud = v; break;
    case 4:  g_power_available = v; break;
    case 5:  g_visitor_tire = v; break;
    case 6:  g_inspector_on = v; ResetAppraisalDeadline(); break;
    case 7:  g_map->f3c = v; break;
    case 8:  g_map->f2c = v; break;
    case 9:  g_bricks_full = v; break;
    case 10: g_show_capacity = v; break;
    case 11: g_832ba8 = v; break;
    }
    PopInfoSizeMayChange();
}

// FUNCTION: LEGOLAND 0x0046a120
int EventTick_Feature(ScriptEvent* e)
{
    SetFeatureFlags(e->f1c, e->f14);
    return 1;
}

/* REPORT <name> a b: dispatch to the report's setter (25 of them at
 * 0x004b7e38), which takes the two numbers in reverse. The brief's
 * SetReportMode. */
// FUNCTION: LEGOLAND 0x0046a140
void SetReportParam(int idx, int a, int b)
{
    if (idx >= 0 && idx < 25)
        g_report_setters[idx](b, a);
}

// FUNCTION: LEGOLAND 0x0046a170
int EventTick_Report(ScriptEvent* e)
{
    SetReportParam(e->f18, e->f1c, e->f14);
    return 1;
}

/* GARDENER / MECHANIC: generate f1c workers of the kind at the tile. */
// FUNCTION: LEGOLAND 0x0046a190
int EventTick_Gardener_Mechanic(ScriptEvent* e)
{
    int i;

    sub_457870(0);
    for (i = 0; i < e->f1c; i++) {
        if (e->f14)
            GenerateGardener(&e->pos, 0);
        else
            GenerateMechanic(&e->pos, 0);
    }
    sub_457870(1);
    return 1;
}

/* WORKERS a b: two map switches, each left alone when its word is negative. */
// FUNCTION: LEGOLAND 0x0046a1f0
int EventTick_Workers(ScriptEvent* e)
{
    if (e->f1c >= 0)
        g_map->f38 = e->f1c != 0;
    if (e->f14 >= 0)
        g_map->f34 = e->f14 != 0;
    return 1;
}

/* DEGRADE <class> v n: set the damage of the first n instances' cells to
 * rate * v / 100 where it is higher (n = 0 or too big means all). The cell
 * lookup is not NULL-checked (original). */
// FUNCTION: LEGOLAND 0x0046a230
int EventTick_Degrade(ScriptEvent* e)
{
    ObjDef* d = ((LLElem*)e->elem)->data;
    Inst*   o;
    Cell*   cell;
    Pos     pos;
    int     n;
    unsigned char amount;

    amount = (unsigned char)(d->rate * e->f14 / 100);
    n = e->f1c;
    o = d->insts;
    if (n > d->inst_count || n == 0)
        n = d->inst_count;
    for (; o && n; o = o->next, n--) {
        pos.x = o->x;
        pos.y = o->y;
        cell = CellAt(pos.x, pos.y);
        if (cell->damage > amount) {
            cell->damage = amount;
            UpdateDamagedCell(cell, &pos);
            g_map_dirty |= 0x200;
        }
    }
    return 1;
}

/* MAX/MIN CAPACITY/VISITORS: f14 says which cap. */
// FUNCTION: LEGOLAND 0x0046a300
int EventTick_Capacity(ScriptEvent* e)
{
    if (e->f14) {
        g_visitor_cap = e->f1c;
        return 1;
    }
    g_visitor_cap_extra = e->f1c;
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a330
int EventTick_Capacityscale(ScriptEvent* e)
{
    SetSimTuningA(e->f14, e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a350
int EventTick_Capacitycap(ScriptEvent* e)
{
    SetSimTuningB(e->f14, e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a370
int EventTick_Entrancefee(ScriptEvent* e)
{
    g_entrance_fee = (short)e->f1c;
    return 1;
}

/* ENDLEVEL: stop the script once the inspector is done. */
// FUNCTION: LEGOLAND 0x0046a390
int EventTick_Endlevel(ScriptEvent* e)
{
    if (g_inspector_on == 0)
        StopScript(1);
    return 1;
}

/* LOOKAT x y: scroll so the tile is centred (isometric: (x - y) across,
 * (x + y) down, in 24.8 fixed point).
 *
 * The two projected offsets must be members of one non-escaping Pos: as
 * plain ints VC6 forward-substitutes them into the stores, evaluates the
 * `g_map->view_w >> 1` operand first and sinks x + y to its multiply
 * (`add edi,esi`, g_map in eax). Aggregate members keep their own webs,
 * so both sums form first (`sub eax,esi` / `lea ecx,[edi+esi]`) and g_map
 * rotates to edx. `* 256` for the final `<< 8` is load-bearing too (`<< 8`
 * lets VC6 fold `>> 9 << 8` into `sar 1 / and`). */
// FUNCTION: LEGOLAND 0x0046a3b0
int EventTick_Lookat(ScriptEvent* e)
{
    int w, h;
    Pos s;
    int y = e->pos.y;
    int x = e->pos.x;

    GetTileDimensions(&w, &h);
    s.x = ((x - y) * w) >> 9;
    s.y = ((x + y) * h) >> 9;
    g_scroll_x = (s.x - (g_map->view_w >> 1)) * 256;
    g_scroll_y = (s.y - (g_map->view_h >> 1)) * 256;
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a420
int EventTick_Themeicon(ScriptEvent* e)
{
    SetThemeIcon(e->f14, (char)e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a440
int EventTick_Addflag(ScriptEvent* e)
{
    AddLevelFlag(e->f14, (char)e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a460
int EventTick_Bridges(ScriptEvent* e)
{
    SetBridges(e->f14, (char)e->f1c);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a480
int EventTick_Breifingfile_Briefingfile(ScriptEvent* e)
{
    SetBriefingFile(e->text);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a4a0
int EventTick_Hintsfile(ScriptEvent* e)
{
    SetHintsFile(e->text);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a4c0
int EventTick_Flashbutton(ScriptEvent* e)
{
    FlashButton(e->f1c, e->f14);
    return 1;
}

// FUNCTION: LEGOLAND 0x0046a4e0
int EventTick_Purge(ScriptEvent* e)
{
    g_script_purge = 1;
    return 1;
}

/* ---- goal events ---------------------------------------------------------- */

/* NEED <class> n: n placed objects of the class. */
// FUNCTION: LEGOLAND 0x0046a4f0
int EventTick_Need(ScriptEvent* e)
{
    int have = ObjCount(e->elem);

    if (have < e->f1c) {
        GoalCheck_Need(e, e->elem, e->f1c - have);
        g_need_shortfall = e->f1c - have;
        return 0;
    }
    return 1;
}

/* NEEDAT <class> x y: the class's object on that very cell. */
// FUNCTION: LEGOLAND 0x0046a540
int EventTick_Needat(ScriptEvent* e)
{
    Cell* cell;

    if (e->pos.x >= 0 && e->pos.x < g_map->w && e->pos.y >= 0 && e->pos.y < g_map->h)
        cell = &g_map_rows[e->pos.y][e->pos.x];
    else
        cell = 0;
    if ((cell->flags & 0x80) && cell->obj == e->elem)
        return 1;
    GoalCheck_Need(e, e->elem, 1);
    return 0;
}

/* NEEDIN <class> n rect: n of the class's objects based inside the area. */
// FUNCTION: LEGOLAND 0x0046a5b0
int EventTick_Needin(ScriptEvent* e)
{
    int   count = 0;
    int   x, y;
    Cell* cell;

    for (y = e->area.top; y <= e->area.bottom; y++) {
        for (x = e->area.left; x <= e->area.right; x++) {
            /* The row table is re-read at every cell in the original (no
             * hoisting out of either loop); the volatile view of the global
             * is the lever that reproduces it -- see docs/lanes/scope-v.md. */
            if (x >= 0 && x < g_map->w && y >= 0 && y < g_map->h)
                cell = &(*(Cell** volatile*)&g_map_rows)[y][x];
            else
                cell = 0;
            if ((cell->flags & 0x80) && cell->obj == e->elem && cell->x == x && cell->y == y)
                count++;
        }
    }
    if (count >= e->f1c)
        return 1;
    GoalCheck_Need(e, e->elem, e->f1c - count);
    return 0;
}

/* CONNECT <class>: every instance's link square is on the map and holds a
 * path. The path test is `!cell->flags & 0x10` -- the original's precedence
 * slip (it reads as `(!flags) & 0x10`, always false), reproduced.
 *
 * The instance's two bytes go through one non-escaping Pos (`b`): as two
 * scalar locals (int or unsigned char, or read inline) the y sum wins ecx
 * over the loop cursor and the cursor lands in edx, dragging the fail-path
 * pushes with it; the shared aggregate lets the cursor keep ecx. */
// FUNCTION: LEGOLAND 0x0046a690
int EventTick_Connect(ScriptEvent* e)
{
    LLElem* elem = e->elem;
    ObjDef* d = elem->data;
    Inst*   o;
    Cell*   cell;
    Pos     b;
    int     x, y;

    for (o = d->insts; o; o = o->next) {
        b.x = o->x;
        b.y = o->y;
        x = b.x + d->dx;
        y = b.y + d->dy;
        cell = CellAt(x, y);
        if (!cell)
            goto fail;
        if (!cell->flags & 0x10)
            goto fail;
    }
    return 1;
fail:
    GoalCheck_Connect(e, elem);
    return 0;
}

/* Classes whose objects take part in LINK: type 1, 4 or 5. */
// FUNCTION: LEGOLAND 0x0046a730
int IsLinkableClass(ObjDef* d)
{
    int t = d->type;

    if (t == 1 || (t > 3 && t <= 5))
        return 1;
    return 0;
}

/* LINK [<class>]: every instance of the class (or of every linkable class
 * when none is named) has its link square on a path joined to the path
 * network. An off-map or non-path square counts as "not connected" (the
 * CONNECT hint wins), an unjoined path as "not linked". Same `!flags & 0x10`
 * slip as CONNECT.
 *
 * The link square is built in a non-escaping aggregate whose byte member is
 * assigned BEFORE its coordinate: that order loads both instance bytes ahead
 * of either sum (`mov cl,[esi+0xe]`, `mov dl,[esi+0xf]`, then `add eax,ecx`)
 * and leaves the pos.x store between the d->dy load and the y sum, which is
 * what stops VC6 folding that delta into `add ecx,[edi+0x10]`. Plain int
 * locals hoist both deltas instead, and building each sum coordinate-first
 * hands the two counters the opposite callee-saved pair. */
// FUNCTION: LEGOLAND 0x0046a750
int EventTick_Link(ScriptEvent* e)
{
    int     missing = 0;
    int     outside = 0;
    Pos     pos;
    struct { int x, y; int z; } t;
    ObjDef* d;
    Inst*   o;
    Cell*   c;
    Cell*   cell;

    if (e->elem) {
        d = ((LLElem*)e->elem)->data;
        o = d->insts;
        if (!o)
            return 1;
        for (; o; o = o->next) {
            t.z = o->x;
            t.x = d->dx;
            t.x += t.z;
            t.z = o->y;
            t.y = d->dy;
            t.y += t.z;
            pos.x = t.x;
            pos.y = t.y;
            cell = CellAt(pos.x, pos.y);
            if (cell && !(!cell->flags & 0x10)) {
                if (!TileJoinsPathNetwork(&pos))
                    missing++;
            } else {
                missing++;
                outside++;
            }
        }
    } else {
        for (c = GetFirstRenderObject(); c; c = GetNextRenderObject(c)) {
            d = c->obj->def;
            if (!IsLinkableClass(d))
                continue;
            t.z = c->x;
            t.x = d->dx;
            t.x += t.z;
            t.z = c->y;
            t.y = d->dy;
            t.y += t.z;
            pos.x = t.x;
            pos.y = t.y;
            cell = CellAt(pos.x, pos.y);
            if (cell && !(!cell->flags & 0x10)) {
                if (!TileJoinsPathNetwork(&pos))
                    missing++;
            } else {
                missing++;
                outside++;
            }
        }
    }
    if (outside) {
        GoalCheck_Connect(e, e->elem);
        return 0;
    }
    if (missing) {
        GoalCheck_Link(e, e->elem);
        return 0;
    }
    return 1;
}
