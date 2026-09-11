/* LEGOLAND: small class callbacks, recovered from SetCustomCallbacks slots.
 * Local layouts follow screencb.c/screencb2.c. See docs/lanes/codex-b.md.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct Rect { int left, top, right, bottom; struct Rect* next; } Rect;
typedef struct Spr { char pad[0x10]; unsigned int flags; } Spr;
typedef struct RideDef {
    char pad00[0x14]; int dx, dy; unsigned int flags;
    char pad20[0x1c]; Rect footprint;
    char pad50[0x14]; Spr* sprite;
} RideDef;
typedef struct RideElem { char pad[0xc]; RideDef* data; } RideElem;
typedef struct DrawDesc { Spr* sprite; int dx, dy; unsigned short square; } DrawDesc;
typedef struct FXEntry { const char* name; int flags; void* sample; } FXEntry;
typedef struct SoundSource { int kind; void* obj; int x, y; } SoundSource;
typedef struct Station {
    char pad00[8]; void* route; char pad0c[0x30];
    struct Station* next; int value;
} Station;

extern void Kill_FXList(void* list, int count); /* 0x00496e30 */
extern void DefaultCursor(void* cursor); /* 0x0045a390 */
extern void SetEditCursorFootPrint(Rect* rect); /* 0x0045f440 */
extern Spr* LoadSprite(const char* name, int mode); /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void KillSprite(void* sprite); /* 0x00497bd0 */
#else
extern int KillSprite(void* sprite); /* 0x00497bd0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void AddBasicObject(RideElem* elem, Pos* pos); /* 0x0045efe0 */
#else
extern void AddBasicObject(void* ll_obj, void* ll_pos, void* ll_ctx); /* 0x0045efe0 */
#define AddBasicObject(_a1, _a2) AddBasicObject((_a1), (_a2), 0)
#endif
extern void Set_UserFlags(int x, int y, int flags); /* 0x00461730 */
extern void KillMoneySFX(void); /* 0x00453930 */
#ifndef LEGOLAND_PORTABLE
extern void StandardRemoveObject(RideElem* elem, unsigned int square, void* cursor); /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* def, unsigned int square); /* 0x0048a2e0 */
#else
/* PORT-M11: both definitions take the packed map square as a 2-byte aggregate
 * BY VALUE -- objmap2.c:983 `BPos bp` (the shape the ObjClass +0x9c slot is
 * typed with at objmap2.c:99) and rides.c:400 `RideTile tile`.  On x86 cdecl
 * that is the same pushed dword as this `unsigned int`; on wasm32 it is a
 * POINTER to a shadow-stack temp, at the same i32 arity, so no link warning
 * and no trap -- the callee just reads an address as a square (PORT-M10 s1b).
 * The square is packed at the call site and VC6 sees none of it. */
typedef struct LLSquare { unsigned char x, y; } LLSquare;   /* objmap2.c's BPos */
extern void StandardRemoveObject(RideElem* elem, LLSquare square, void* cursor); /* 0x0045f220 */
extern void RemoveAllBlokesFromRide(RideDef* def, LLSquare square); /* 0x0048a2e0 */
static __inline LLSquare ll_square(unsigned int v)
{
    LLSquare s;
    s.x = (unsigned char)v;
    s.y = (unsigned char)(v >> 8);
    return s;
}
#define StandardRemoveObject(_e, _s, _c) \
    StandardRemoveObject((_e), ll_square((unsigned int)(_s)), (_c))
#define RemoveAllBlokesFromRide(_d, _s) \
    RemoveAllBlokesFromRide((_d), ll_square((unsigned int)(_s)))
#endif
extern void StopMoneySFX(unsigned int* square); /* 0x004539a0 */
#ifndef LEGOLAND_PORTABLE
extern void PlayInstanceOfSample(void* sample, int a, int b, SoundSource* src); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void* sample, int a, int b, SoundSource* src); /* 0x00496d20 */
#endif
extern unsigned int rand(void); /* 0x0049e4b2 */
extern void FreeBinV(void* bnv); /* 0x0044dd60 */
extern void Balloonz_FreeRecords(void); /* 0x0042a9f0 */
extern void Carousel_FreeRecords(void); /* 0x0042bc40 */

extern int g_dino_sound_refs; /* 0x0066711c */
extern int g_fountain_sound_refs; /* 0x00667114 */
extern FXEntry g_dino_fx[5]; /* 0x004b8768 */
extern FXEntry g_fountain_fx[5]; /* 0x004b8710 */
extern int g_edit_changed; /* 0x008119b0 */
extern RideDef* g_edit_object; /* 0x008119b8 */
extern char g_edit_cursor; /* 0x007febc0 */
extern unsigned int g_ui_flags; /* 0x008003e8 */
extern RideDef* g_jc_def; /* 0x0081cb60 */
extern RideDef* g_jc_tree_cls; /* 0x0081cb70 */
extern RideDef* g_jc_fish_cls; /* 0x0081cb74 */
extern RideDef* g_jc_water_cls; /* 0x0081cb54 */
extern Spr* g_jc_tree_mask; /* 0x0081cb68 */
extern Spr* g_jc_fish_jump; /* 0x0081cb6c */
extern Rect g_jc_area; /* 0x00629c40 */
extern Rect g_jc_dock_a; /* 0x004b7278 */
extern Rect g_jc_dock_b; /* 0x004b7260 */
extern Rect kJcWaterRect; /* 0x004b7478 */
extern DrawDesc g_shop_draw; /* 0x0082c6a0 */
extern DrawDesc g_rest2_draw; /* 0x00616120 */
extern FXEntry g_rest2_fx[3]; /* 0x004b6968 */
extern Spr* g_rest2_fdoor_m; /* 0x0081cd34 */
extern Spr* g_rest2_bdoor_m; /* 0x0081cd84 */
extern Spr* g_rest2_tower_m; /* 0x0081cd20 */
extern Spr* g_rest2_fdoor_m1; /* 0x0081cd48 */
extern Station* g_jc_stations; /* 0x00629c3c */
extern void* g_bz_base_m1; /* 0x00616048 */
extern void* g_bz_base_m2; /* 0x0061604c */
extern void* g_bz_base_m3; /* 0x00616050 */
extern void* g_bz_car_red; /* 0x00616054 */
extern void* g_bz_car_green; /* 0x00616058 */
extern void* g_bz_car_blue; /* 0x0061605c */
extern void* g_bz_zspr; /* 0x0081cde8 */
extern void* g_bz_bnv[]; /* 0x00616018 */
extern RideDef* g_carousel_item; /* 0x006160bc */
extern void* g_carousel_matte; /* 0x0061606c */
extern void* g_carousel_matte2; /* 0x00616070 */
extern void* g_carousel_zspr[]; /* 0x006160c0 */
extern void* g_carousel_bnv[]; /* 0x00616090 */
extern FXEntry g_carousel_fx[2]; /* 0x004b64d8 */

// FUNCTION: LEGOLAND 0x00452ba0
void Dino_AC(void)
{
    if (--g_dino_sound_refs == 0) Kill_FXList(g_dino_fx, 5);
}

// FUNCTION: LEGOLAND 0x004529c0
void Fountain_AC(void)
{
    if (--g_fountain_sound_refs == 0) Kill_FXList(g_fountain_fx, 5);
}

// FUNCTION: LEGOLAND 0x00434f50
void JungleCruise_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jc_def;
    DefaultCursor(&g_edit_cursor);
    g_jc_area.next = &g_jc_dock_a;
    g_jc_dock_a.next = &g_jc_dock_b;
    SetEditCursorFootPrint(&g_jc_area);
}

// FUNCTION: LEGOLAND 0x00433ca0
void JcMonkeyTree_Create(RideElem* elem)
{
    RideDef* def = elem->data;
    g_jc_tree_cls = def;
    def->flags |= 0x400;
    g_jc_tree_mask = LoadSprite("brijmask.lls", 1);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0xa0 draw handler, and that slot takes the base map square as a
 * 2-byte aggregate BY VALUE as well (renderview.c:304), so on wasm32 this body
 * was stamping a shadow-stack ADDRESS into the draw descriptor's `square`. */
#define JcMonkeyTree_GetDrawDesc JcMonkeyTree_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00434040
DrawDesc* JcMonkeyTree_GetDrawDesc(RideElem* elem, unsigned short square)
{
    RideDef* def = elem->data;
    g_shop_draw.sprite = def->sprite;
    g_shop_draw.dx = def->dx;
    g_shop_draw.dy = def->dy;
    g_shop_draw.square = square;
    return &g_shop_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef JcMonkeyTree_GetDrawDesc
DrawDesc* JcMonkeyTree_GetDrawDesc(RideElem* elem, LLSquare square)
{
    return JcMonkeyTree_GetDrawDesc_vc6_body(elem, square.x | (square.y << 8));
}
#endif

// FUNCTION: LEGOLAND 0x00434080
void JcMonkeyFish_Create(RideElem* elem)
{
    RideDef* def = elem->data;
    g_jc_fish_cls = def;
    def->flags |= 0x400;
    g_jc_fish_jump = LoadSprite("mfish2.lls", 1);
}

// FUNCTION: LEGOLAND 0x00433ce0
void JcMonkeyTree_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jc_tree_cls;
    DefaultCursor(&g_edit_cursor);
    g_ui_flags |= 8;
    SetEditCursorFootPrint(&g_jc_tree_cls->footprint);
}

// FUNCTION: LEGOLAND 0x004340c0
void JcMonkeyFish_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jc_fish_cls;
    DefaultCursor(&g_edit_cursor);
    g_ui_flags |= 8;
    SetEditCursorFootPrint(&g_jc_fish_cls->footprint);
}

// FUNCTION: LEGOLAND 0x004314f0
void OctopusCafe_Add(RideElem* elem, Pos* pos)
{
    AddBasicObject(elem, pos);
    Set_UserFlags(pos->x, pos->y, 0);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0xa0 draw handler, and that slot takes the base map square as a
 * 2-byte aggregate BY VALUE as well (renderview.c:304), so on wasm32 this body
 * was stamping a shadow-stack ADDRESS into the draw descriptor's `square`. */
#define Restaurant2_GetDrawDesc Restaurant2_GetDrawDesc_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004304a0
DrawDesc* Restaurant2_GetDrawDesc(RideElem* elem, unsigned short square)
{
    RideDef* def = elem->data;
    g_rest2_draw.sprite = def->sprite;
    g_rest2_draw.dx = def->dx;
    g_rest2_draw.dy = def->dy;
    g_rest2_draw.square = square;
    def->sprite->flags |= 0x2000;
    return &g_rest2_draw;
}
#ifdef LEGOLAND_PORTABLE
#undef Restaurant2_GetDrawDesc
DrawDesc* Restaurant2_GetDrawDesc(RideElem* elem, LLSquare square)
{
    return Restaurant2_GetDrawDesc_vc6_body(elem, square.x | (square.y << 8));
}
#endif

// FUNCTION: LEGOLAND 0x00431120
void Restaurant2_Destroy(void)
{
    Kill_FXList(g_rest2_fx, 3);
    KillMoneySFX();
    KillSprite(g_rest2_fdoor_m);
    KillSprite(g_rest2_bdoor_m);
    KillSprite(g_rest2_tower_m);
    KillSprite(g_rest2_fdoor_m1);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M11: a +0x9c remove handler, and that slot passes the map square as a
 * 2-byte aggregate BY VALUE (objmap2.c:99) -- a POINTER on wasm32 -- while this
 * body reads it as an `unsigned int` and takes its ADDRESS for StopMoneySFX.
 * Renamed for the portable build with a twin of the slot's shape over it. */
#define FoodService_Remove FoodService_Remove_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004312c0
void FoodService_Remove(RideElem* elem, unsigned int square, void* cursor)
{
    /* Low two bytes of square are x/y; cursor is a separate third argument. */
    StandardRemoveObject(elem, square, cursor);
    RemoveAllBlokesFromRide(elem->data, square);
    StopMoneySFX(&square);
}
#ifdef LEGOLAND_PORTABLE
#undef FoodService_Remove
void FoodService_Remove(RideElem* elem, LLSquare square, void* cursor)
{
    FoodService_Remove_vc6_body(elem, square.x | (square.y << 8), cursor);
}
#endif

// FUNCTION: LEGOLAND 0x00436160
int JungleCruise_BestValue(void* unused, int working_only)
{
    Station* s = g_jc_stations;
    int result = 0;
    while (s) {
        if (s->value > result && (!working_only || s->route))
            result = s->value;
        s = s->next;
    }
    return result;
}

// FUNCTION: LEGOLAND 0x004361a0
void JcWater_SelectForPlacement(void)
{
    g_edit_changed = 1;
    g_edit_object = g_jc_water_cls;
    g_jc_water_cls->footprint = kJcWaterRect;
    DefaultCursor(&g_edit_cursor);
    g_ui_flags |= 8;
    SetEditCursorFootPrint(&g_edit_object->footprint);
}

// FUNCTION: LEGOLAND 0x004529e0
void Fountain_Add(RideElem* elem, Pos* pos)
{
    SoundSource src;
    AddBasicObject(elem, pos);
    src.kind = 2;
    src.x = pos->x;
    src.y = pos->y;
    /* Original leaves src.obj uninitialised for position source kind 2. */
    PlayInstanceOfSample(g_fountain_fx[0].sample, 1, 1, &src);
}

// FUNCTION: LEGOLAND 0x0042b9d0
void Balloonz_Destroy(void)
{
    KillSprite(g_bz_base_m1);
    KillSprite(g_bz_base_m2);
    KillSprite(g_bz_base_m3);
    KillSprite(g_bz_car_red);
    KillSprite(g_bz_car_green);
    KillSprite(g_bz_car_blue);
    KillSprite(g_bz_zspr);
    FreeBinV(g_bz_bnv[0]);
    Balloonz_FreeRecords();
}

// FUNCTION: LEGOLAND 0x0042c3f0
void Carousel_Destroy(RideElem* elem)
{
    g_carousel_item = elem->data;
    KillSprite(g_carousel_matte);
    KillSprite(g_carousel_matte2);
    KillSprite(g_carousel_zspr[0]);
    Carousel_FreeRecords();
    Kill_FXList(g_carousel_fx, 2);
    FreeBinV(g_carousel_bnv[0]);
    FreeBinV(g_carousel_bnv[1]);
    FreeBinV(g_carousel_bnv[2]);
}

// FUNCTION: LEGOLAND 0x00452bc0
void Dino_Add(RideElem* elem, Pos* pos)
{
    SoundSource src;
    unsigned int which;
    AddBasicObject(elem, pos);
    src.kind = 2;
    src.x = pos->x;
    src.y = pos->y;
    /* Original leaves src.obj uninitialised for position source kind 2. */
    which = rand() % 5;
    PlayInstanceOfSample(g_dino_fx[which].sample, 1, 1, &src);
}
