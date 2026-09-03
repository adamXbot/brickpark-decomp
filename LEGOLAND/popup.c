/* LEGOLAND — object construction, the 16-bit "transparent" blitter and the
 * object information pop-up.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS are load-bearing; names are ours.
 *
 *   0x0045eb30  BuildObject         100% full-body (188/188)
 *   0x00489190  RenderTransSprite   100% full-body (188/188)
 *   0x004724a0  DrawPopUpInfo       962/962 instructions, 3139/3141 bytes,
 *                                   53 mismatches (886 -> 705 -> 398 -> 60 -> 53)
 *                                   (see its note)
 *
 * ---------------------------------------------------------------------------
 * WHAT THE OBJECT RECORDS ARE
 *
 * The thing the map stores in a cell and that BuildObject is handed is the
 * LLIDB ELEMENT of the object class (legoland.h's Elem: name/image/flags/
 * data/refcount), and its `data` at +0x0c is the 0xd0-byte ObjDef parsed by
 * LLIDB_LoadODFData.  objmap2.c / mapobj.c call the same pointer `MapObj` /
 * `obj` with a `cls` at +0x0c — that is the same thing seen from the other
 * end.  Two consequences worth recording:
 *
 *  - 0x0080ff64 is ElemID("CASTLE OBJ") (InitGameMap 0x00459850), so
 *    `obj == *(void**)0x0080ff64` in BuildObject and in PutObjOnMap is "the
 *    thing just placed IS the castle", and 0x0079a8d0 is "the castle has been
 *    built".  mapobj.c / objmap2.c name those two g_placing_obj /
 *    g_placed_flag, which reads them as a generic "object being placed"; the
 *    castle reading is the right one.
 *  - AddObjectToBuildList's second parameter (sweep1.c calls it `type`) is
 *    the PACKED MAP TILE {u8 x, u8 y} of the build, passed BY VALUE as a
 *    2-byte struct.  BuildObject builds it from the Pos it is given, and
 *    buildtick.c's BuildSlot.key (the same 16 bits) is what DrawPopUpInfo
 *    searches the 256 build slots with.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A packed 2-byte map coordinate passed BY VALUE (objmap2.c's BPos). */
typedef struct BPos {
    unsigned char x;            /* +0x00 */
    unsigned char y;            /* +0x01 */
} BPos;

/* An object class / definition (the 0xd0-byte ODF record; objmap2.c ObjDef). */
typedef struct ObjDef {
    char           pad0[0x0c];  /* +0x00 */
    int            dx;          /* +0x0c door / entry offset */
    int            dy;          /* +0x10 */
    char           pad14[0x1c - 0x14];
    unsigned int   flags;       /* +0x1c */
    short          type;        /* +0x20 */
    char           pad22[0x2c - 0x22];
    unsigned char  life;        /* +0x2c initial life (the pop-up's 100%) */
    char           pad2d[0x3c - 0x2d];
    Rect           rect;        /* +0x3c footprint rect list */
    char           pad50[0x78 - 0x50];
    char*          name;        /* +0x78 display name */
    char           pad7c[0xd0 - 0x7c];
} ObjDef;

/* The LLIDB element a placed object is known by: its ObjDef is at +0x0c
 * (legoland.h's Elem with a different data type). */
typedef struct ObjElem {
    char    pad0[0x0c];         /* +0x00 */
    ObjDef* def;                /* +0x0c */
} ObjElem;


/* An inclusive-exclusive clip rectangle (SPRITE_ClipRect is {0,0,640,480}). */
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;

/* A loaded sprite record (printlist.c SpriteRec): w/h at +0x14/+0x16. */
typedef struct SpriteRec {
    struct SpriteRec* next;     /* +0x00 */
    void*          surface;     /* +0x04 */
    void*          image;       /* +0x08 */
    int            detail;      /* +0x0c */
    unsigned int   flags;       /* +0x10 */
    short          w;           /* +0x14 */
    short          h;           /* +0x16 */
    short          src_x;       /* +0x18 */
    short          src_y;       /* +0x1a */
    unsigned short refs;        /* +0x1c */
    short          pad1e;       /* +0x1e */
} SpriteRec;

/* What GetSprite fills: a locked surface (printlist.c SpriteHandle). */
typedef struct SpriteHandle {
    int   pitch;                /* +0x00 */
    int   w;                    /* +0x04 */
    int   h;                    /* +0x08 */
    void* pixels;               /* +0x0c */
    void* surface;              /* +0x10 */
    int   bpp;                  /* +0x14  1 = 8-bit, 2 = 16-bit */
} SpriteHandle;

/* ---------------------------------------------------------------- globals -- */

/* ElemID("CASTLE OBJ") — InitGameMap (0x00459850) puts it here.  mapobj.c /
 * objmap2.c call this one g_placing_obj; it is really the castle element, and
 * 0x0079a8d0 is "the castle has been built". */
extern ObjElem* g_castle_obj_elem;      /* 0x0080ff64 */
extern int      g_castle_built;         /* 0x0079a8d0 */
extern int      g_map_loading;          /* 0x00667cd8 */
extern int      g_render_order_dirty;   /* 0x00667cdc */

/* ---------------------------------------------------------------- callees -- */
extern ClipRect g_clip_rect;            /* 0x004bdea0 SPRITE_ClipRect */
extern int      g_screen_depth;         /* 0x00668088  0 = 8-bit, 1 = 555, 2 = 565 */
extern int   GetSprite(SpriteHandle* out, SpriteRec* s);        /* 0x00497c30 */
extern void  ReleaseSprite(SpriteHandle* h);                    /* 0x00497dc0 */


extern int   GetObjCost(ObjDef* d);                             /* 0x00480da0 */
extern int   GetBrickCount(void);                               /* 0x004578e0 */
extern void  UseBricks(int n);                                  /* 0x004578c0 */
extern void  PlayAppropriateBuildEffect(ObjDef* d, Pos* pos);   /* 0x00462d10 */
extern int   AddObjectToBuildList(ObjDef* d, BPos bp);          /* 0x00450b90 */
extern int   ClassAllowsObjects(ObjDef* d);                     /* 0x0045eab0 */
extern int   ClassNeedsPath(ObjDef* d);                         /* 0x0045eaf0 */
extern void  AddObjectToMap(ObjElem* obj, Pos* pos, unsigned int flags); /* 0x0045e080 */
extern void  SetObjRectFlags(ObjElem* obj, Pos* pos, unsigned int flags);/* 0x0045dee0 */
extern void  GetObjectDoorOffset(ObjDef* d, Pos* out);          /* 0x0045ea40 */
extern void  UpdateEntranceTile(void);                          /* 0x00482a90 */
extern void  RefreshEntranceTile(int force);                    /* 0x00482b20 */
extern Pos*  GetEntranceTile(void);                             /* 0x00482b00 */
/* Defined (Pos from, Pos to) in simcore.c; declared the same way here because
 * the by-value copy of *GetEntranceTile() is what schedules the four argument
 * loads as the original does (see the note above BuildObject). */
extern void  RequestRoute(Pos from, Pos to);                    /* 0x00477bd0 */
extern void  CalculateMapRenderOrder(void);                     /* 0x0045a4a0 */
extern void  SetObjectDoorFlags(ObjElem* obj, Pos* pos);        /* 0x0045e770 */
extern void  PutObjOnMap(ObjDef* d, ObjElem* obj, Pos* pos);    /* 0x00459ad0 */

/* -------------------------------------------------------------------------
 * 0x0045eb30 -- build one object of class `obj` at map tile `pos`.
 * ------------------------------------------------------------------------- */

/* 188/188, audit [OK] (2026-09-03, pass 4).  CLOSED BY: the entrance tile is
 * passed to RequestRoute as a BY-VALUE Pos copied straight off the call
 * result -- `RequestRoute(door, *GetEntranceTile());` with the callee declared
 * `(Pos from, Pos to)` as simcore.c defines it.  `Pos ent = *GetEntranceTile();
 * RequestRoute(door, ent);` is byte-identical.  Every form that keeps the
 * result as a POINTER local (`Pos* ent = GetEntranceTile(); RequestRoute(...,
 * ent->x, ent->y)` / `(door, *ent)` / `e = *ent` / `e.x = ent->x; e.y = ...`)
 * sits at 8 mismatches: same registers, same push order, but VC6 hoists the
 * two `[eax]`/`[eax+4]` loads above the first push at both sites, where the
 * original interleaves them (site 1: ent->y, PUSH, door.x, ent->x, door.y;
 * site 2: ent->y, door.y, PUSH, door.x, ent->x).  Copying the struct from the
 * unnamed call-result pointer makes the two field loads part of a by-value
 * argument copy instead of two independent CSE'd loads through a named
 * pointer, and the scheduler then emits them in the original's order.
 * LEVER, stated for the playbook: when four argument loads through a
 * call-result pointer come out hoisted above a push, drop the pointer local
 * and pass `*f()` (or a Pos copied from `*f()`) to a callee declared with a
 * struct-by-value parameter; the `(int,int,int,int)` / `(Pos,Pos)` prototype
 * alone is inert (ABI-identical), it is the unnamed-pointer copy that moves
 * the loads.
 * History (three earlier passes, ~300 measured variants, all 8 or worse):
 * Pos-by-value parameters in every 2-/3-/4-argument arrangement with a NAMED
 * pointer; hoisting any subset of the four values into temps; volatile casts
 * (best 7); scheduler-window padding (inert); every int/unsigned/long
 * prototype mix; static __inline wrappers; `Pos d = door` copies; two plain
 * int locals instead of the address-taken `door` (170 instructions -- the
 * address-taken Pos IS required, GetObjectDoorOffset takes &door). */
// FUNCTION: LEGOLAND 0x0045eb30
int BuildObject(ObjElem* obj, Pos* pos)
{
    ObjDef* def = obj->def;
    BPos    bp;
    Pos     door;
    int     cost;

    bp.x = (unsigned char)pos->x;
    bp.y = (unsigned char)pos->y;
    cost = GetObjCost(def);
    if (GetBrickCount() < cost)
        return 0;

    if (obj == g_castle_obj_elem)
        g_castle_built = 1;
    if (g_map_loading == 0)
        PlayAppropriateBuildEffect(def, pos);
    else
        g_render_order_dirty = 1;

    if (def->flags & 0x80000) {
        if (!AddObjectToBuildList(def, bp))
            return 0;
        UseBricks(GetObjCost(def));
        if (ClassAllowsObjects(def) || ClassNeedsPath(def))
            AddObjectToMap(obj, pos, 0x20);
        else
            SetObjRectFlags(obj, pos, 0x20);
        GetObjectDoorOffset(def, &door);
        door.x += pos->x;
        door.y += pos->y;
        if (def->flags & 0x400000) {
            UpdateEntranceTile();
            RefreshEntranceTile(1);
            RequestRoute(door, *GetEntranceTile());
        }
        if (g_map_loading == 0)
            CalculateMapRenderOrder();
        SetObjectDoorFlags(obj, pos);
        return 1;
    }

    UseBricks(GetObjCost(def));
    if (ClassAllowsObjects(def) || ClassNeedsPath(def))
        AddObjectToMap(obj, pos, 0);
    GetObjectDoorOffset(def, &door);
    door.x += pos->x;
    door.y += pos->y;
    PutObjOnMap(def, obj, pos);
    if (def->flags & 0x400000) {
        UpdateEntranceTile();
        RefreshEntranceTile(1);
        RequestRoute(door, *GetEntranceTile());
    }
    SetObjectDoorFlags(obj, pos);
    return 1;
}

/* -------------------------------------------------------------------------
 * 0x00489190 -- blend a sprite onto the SCREEN at 50% opacity, 16-bit RGB565
 * only, at HALF vertical resolution.
 *
 * The screen is locked with GetSprite(&dst, 0) and the sprite with
 * GetSprite(&src, s); the sprite rectangle (x, y, x+w-1, y+h-1) is clipped
 * against SPRITE_ClipRect into `drect`, and the amount trimmed off each edge
 * becomes `srect` -- so srect is the source sub-rectangle in sprite-local
 * coordinates and drect the destination rectangle in screen coordinates.
 *
 * The blit itself is hand-written assembler working two pixels (one dword) at
 * a time: each source dword and the destination dword are masked with
 * 0xf7def7de (clearing the low bit of every 5/6/5 channel) and halved, and the
 * sum is the 50% blend.  A source dword that is zero after the mask is treated
 * as transparent and skipped.  Each result is stored to the destination row
 * AND to the row one pitch below, and both pointers step two rows per pass --
 * so the effect is drawn at half vertical resolution, every other scanline
 * duplicated.  The source pointer is aligned DOWN to a dword boundary.
 *
 * Two faults are reproduced deliberately:
 *   - the destination pixel pair that is read for the blend is the one AFTER
 *     the pair being written (`mov edx,[eax]` runs after `add eax,4`, while the
 *     stores use [eax-4]); the blend therefore mixes source pixel n with
 *     destination pixel n+1.
 *   - the inner loop's exit falls THROUGH the transparent-skip block, so one
 *     extra `add edi,4 / sub ecx,2` runs at the end of every row (harmless:
 *     both are reloaded per row).
 * Depth 0 (8-bit) and depth 1 (RGB555) are unimplemented stubs that only load
 * eax with 0 -- they never store the result slot, so the value returned for
 * those depths is whatever was last left at [ebp-0x20].  Reproduced as is.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00489190
int RenderTransSprite(SpriteRec* s, int x, int y)
{
    SpriteHandle   dst;
    SpriteHandle   src;
    ClipRect       srect;
    ClipRect       drect;
    int            rc;
    int            dpitch2;
    int            dpitch;
    int            dptr;
    int            spitch;
    int            sptr;
    int            cols;
    unsigned short w;
    unsigned short h;

    w = s->w;
    h = s->h;
    if (!GetSprite(&dst, 0))
        return 0;
    if (!GetSprite(&src, s))
        return 0;
    switch (g_screen_depth) {
    case 0:
        __asm { mov eax, 0 }
        break;
    case 1:
        __asm { mov eax, 0 }
        break;
    case 2:
        __asm {
            lea     edx, g_clip_rect
            lea     esi, srect
            mov     eax, x
            mov     ecx, [edx]
            lea     edi, drect
            cmp     eax, ecx
            jl      clipl
            mov     [edi], eax
            mov     dword ptr [esi], 0
            jmp     donel
clipl:      mov     [edi], ecx
            sub     ecx, eax
            mov     [esi], ecx
donel:      movzx   ebx, word ptr w
            mov     ecx, [edx+8]
            dec     ebx
            add     eax, ebx
            cmp     eax, ecx
            jg      clipr
            mov     [edi+8], eax
            mov     [esi+8], ebx
            jmp     doner
clipr:      sub     eax, ecx
            sub     ebx, eax
            mov     [edi+8], ecx
            mov     [esi+8], ebx
doner:      mov     eax, y
            mov     ecx, [edx+4]
            cmp     eax, ecx
            jl      clipt
            mov     [edi+4], eax
            mov     dword ptr [esi+4], 0
            jmp     donet
clipt:      mov     [edi+4], ecx
            sub     ecx, eax
            mov     [esi+4], ecx
donet:      movzx   ebx, word ptr h
            dec     ebx
            mov     ecx, [edx+0Ch]
            add     eax, ebx
            cmp     eax, ecx
            jg      clipb
            mov     [edi+0Ch], eax
            mov     [esi+0Ch], ebx
            jmp     doneb
clipb:      sub     eax, ecx
            sub     ebx, eax
            mov     [edi+0Ch], ecx
            mov     [esi+0Ch], ebx
doneb:      xor     eax, eax
            mov     edx, [edi]
            mov     ebx, [edi+8]
            mov     ecx, [edi+4]
            cmp     edx, ebx
            jge     done
            mov     edx, [edi+0Ch]
            cmp     ecx, edx
            jge     done
            lea     esi, dst
            mov     eax, [esi]
            mov     dpitch2, eax
            mov     dpitch, eax
            mov     ebx, [edi+4]
            mul     ebx
            mov     ecx, [edi]
            shl     ecx, 1
            add     eax, [esi+0Ch]
            add     eax, ecx
            mov     dptr, eax
            lea     esi, src
            mov     eax, [esi]
            lea     edi, srect
            mov     spitch, eax
            mov     ebx, [edi+4]
            mul     ebx
            mov     ecx, [edi]
            shl     ecx, 1
            add     eax, [esi+0Ch]
            add     eax, ecx
            and     eax, 0FFFFFFFCh
            mov     sptr, eax
            mov     eax, dptr
            mov     edx, [edi+0Ch]
            sub     edx, [edi+4]
            mov     ecx, [edi+8]
            sub     edx, 1
            sub     ecx, [edi]
            sub     ecx, 2
            and     edx, 0FFFFFFFFh
            and     ecx, 0FFFFFFFFh
            mov     cols, ecx
            shl     spitch, 1
            shl     dpitch2, 1
            mov     ebx, eax
row:        mov     edi, sptr
            push    ebp
            mov     ebp, dpitch
            push    edx
pixel:      mov     esi, [edi]
            add     eax, 4
            and     esi, 0F7DEF7DEh
            je      skip
            shr     esi, 1
            mov     edx, [eax]
            add     edi, 4
            and     edx, 0F7DEF7DEh
            shr     edx, 1
            nop
            add     esi, edx
            sub     ecx, 2
            mov     [eax-4], esi
            mov     [eax+ebp-4], esi
            jns     pixel
skip:       add     edi, 4
            sub     ecx, 2
            jns     pixel
            pop     edx
            pop     ebp
            mov     eax, spitch
            add     ebx, dpitch2
            add     sptr, eax
            mov     eax, ebx
            mov     ecx, cols
            sub     edx, 2
            jns     row
done:       mov     rc, eax
        }
        break;
    }
    ReleaseSprite(&src);
    ReleaseSprite(&dst);
    return rc;
}

/* ------------------------------------------------- the info pop-up types -- */
typedef struct Icon {
    struct Icon*  next;
    void*         sprite;
    void**        data;
    short         x;            /* +0x0c */
    short         y;            /* +0x0e */
    short         w;
    short         h;
    unsigned short group;
    short         pad16;
    char          pad18[0x34 - 0x18];
    unsigned int  flags;        /* +0x34 */
} Icon;

/* The 12-byte pop-up identity record at 0x007fdec0, passed BY VALUE. */
typedef struct PopUpKey { int type; void* obj; int ref; } PopUpKey;

typedef struct PopUpUI {
    Icon*   icon_mech;          /* +0x000  0x007fdea4 */
    int     pad_004;
    void*   spr_full;           /* +0x008  0x007fdeac */
    void*   spr_norepair;       /* +0x00c */
    int     pad_010[3];
    PopUpKey key;               /* +0x01c  0x007fdec0  type/obj/ref, passed BY
                                 * VALUE as one 12-byte record to
                                 * PopUpInfoSetUp (the original builds it with
                                 * sub esp,0xc + three stores). */
    Pos     pos;                /* +0x028 */
    char    pad_030[0xd8 - 0x30];
    ObjDef* cls;                /* +0x0d8  0x007fdf7c */
    void*   ride;               /* +0x0dc  0x007fdf80 */
    Cell*   cell;               /* +0x0e0  0x007fdf84 */
    unsigned short cellpos;     /* +0x0e4 */
    short   pad_0e6;
    void*   worker;             /* +0x0e8  0x007fdf8c */
    int     w_f1c;              /* +0x0ec */
    int     w_f20;              /* +0x0f0 */
    int     named;              /* +0x0f4  0x007fdf98 */
    int     kind;               /* +0x0f8  0x007fdf9c */
    int     active;             /* +0x0fc  0x007fdfa0 */
    int     expanded;           /* +0x100  0x007fdfa4 */
    int     resize;             /* +0x104  0x007fdfa8 */
    unsigned char size;         /* +0x108  0x007fdfac */
    char    pad_109[3];
    void*   elem_shed;          /* +0x10c */
    void*   elem_hut;
    void*   elem_path;
    void*   elem_entrance;
    Icon*   icon_close;         /* +0x11c  0x007fdfc0 */
    Icon*   icon_next;
    void*   spr_sad;            /* +0x124  0x007fdfc8 */
    Icon*   icon_delete2;       /* +0x128  0x007fdfcc */
    void*   spr_hungry;         /* +0x12c */
    int     pad_130;
    Icon*   icon_corner;        /* +0x134  0x007fdfd8 */
    Icon*   icon_delete;        /* +0x138 */
    Icon*   icon_gardener;      /* +0x13c */
    void*   spr_norm;           /* +0x140  0x007fdfe4 */
    Icon*   icon_prev;          /* +0x144 */
    int     pad_148[6];
    void*   spr_repairok;       /* +0x160  0x007fe004 */
    void*   spr_peckish;        /* +0x164  0x007fe008 */
    int     pad_168[3];
    void*   spr_happy;          /* +0x174  0x007fe018 */
} PopUpUI;
extern PopUpUI g_popup;                 /* 0x007fdea4 */

typedef struct GameButton { int mask; int state; } GameButton;
typedef struct GameInput {
    int        flags;           /* +0x00 */
    Pos        point;           /* +0x04 */
    GameButton mouse_a;         /* +0x0c */
    GameButton mouse_b;         /* +0x14 */
    GameButton mouse_c;         /* +0x1c  state @0x00813a60 */
} GameInput;
extern GameInput g_input;               /* 0x00813a40 */

typedef struct BuildKey { unsigned char x, y; } BuildKey;
typedef struct BuildSlot {
    void*     obj;              /* +0x00 */
    short     key;              /* +0x04 */
    short     pad6;
    int       timer;            /* +0x08 */
} BuildSlot;
extern BuildSlot g_build_slots[256];    /* 0x006664f8 */

typedef struct RideRec {
    int    f00;
    char** name;                /* +0x04 */
    char   pad08[0x18 - 8];
    int    f18;                 /* +0x18 */
    struct RideBloke* rider;    /* +0x1c */
} RideRec;
typedef struct RideBloke {
    char          pad00[0x0c];
    short         kind;         /* +0x0c */
    char          pad0e[0x60 - 0x0e];
    unsigned char cond;         /* +0x60 */
} RideBloke;

extern int  g_edit_mode;                /* 0x008119b0 EditMode */
extern int  g_pu_building;              /* 0x0066895c */
extern int  g_power_available;          /* 0x0083298c */

extern char* GetString(int id);                                     /* 0x00498f50 */
extern int   Format(char* dst, const char* fmt, ...);               /* 0x0049e573 */
extern int   GetNearestColour(int r, int g, int b);                 /* 0x0044e6c0 */
extern void  ResetInfoStruct(void);                                 /* 0x00471510 */
extern int   GetObjSalvageValue(ObjDef* d, int life);               /* 0x00480db0 */
extern int   GetObjRepairCost(ObjDef* d, int life);                 /* 0x00480de0 */
extern int   FindObjectsPower(ObjDef* d);                           /* 0x00459fa0 */
extern int   GetGardenerCount(void);                                /* 0x00499550 */
extern int   GetMechanicCount(void);                                /* 0x00499560 */
extern char* GetVisitorName(void* bloke);                           /* 0x00482ba0 */
extern void  PopUpInfoSetUp(PopUpKey key, int x, int y);            /* 0x00471950 */
extern void  DrawPopUpMock(void);                                   /* 0x004720a0 */
extern int   MeasurePopUpTitle(const char* s, int a, int b, int c, int d, int e); /* 0x00471840 */
extern int   MeasurePopUpBody(const char* s, int a, int b, int c, int d, int e);  /* 0x004717a0 */
extern void  ClampPopUpToScreen(int size);                          /* 0x004718c0 */
extern void  DrawPopUpFrame(void);                                  /* 0x00471f10 */
extern void  PushRenderingStatusAndUnlockVideoSurface(void);        /* 0x00464080 */
extern void  PopRenderingStatus(void);                              /* 0x004641f0 */
extern void  PrintCachedText(const char* text, int x, int y, int w, int h,
                             int f1, int f2, int ink, int paper);   /* 0x00455e50 */
extern int   GetBlokeMood(void* bloke);                             /* 0x00482d30 */
extern signed char GetBlokeAgeGroup(void* bloke);                   /* 0x0044eb10 */
extern int   PrintSprite(void* s, int x, int y, int mode, void* ctx);/* 0x004853a0 */
extern void  RenderBlock(int x, int y, int w, int h, int colour);   /* 0x004890c0 */
extern int   GetBuildTime(ObjDef* d);                               /* 0x00450c40 */
extern int   PopUpCanDelete(void);                                  /* 0x004723f0 */
extern void  ClosePopUpIcons(void);                                 /* 0x00471610 */
extern void  DrawPopUpExtra(void);                                  /* 0x00471d90 */
extern void  DrawPopUpEnd(void);                                    /* 0x00472090 */

extern const char kFmtStr[];      /* 0x004b8bbc "%s" */
extern const char kFmtNl[];       /* 0x004bad38 "\n" */
extern const char kFmtNlS[];      /* 0x004bad34 "\n%s" */
extern const char kFmtNlSD[];     /* 0x004bad3c "\n%s %d" */
extern const char kFmtColon[];    /* 0x004bad2c "%s : %d" */
extern const char kFmtSDSD[];     /* 0x004bad44 "%s %d\n%s %d" */
extern const char kFmtNlSDSD[];   /* 0x004bad1c "\n%s %d\n%s %d" */
extern const char kEmpty[];       /* 0x004d8bb0 "" */

extern char* strcat(char*, const char*);
#pragma intrinsic(strcat)


/* -------------------------------------------------------------------------
 * 0x004724a0 -- draw the object information pop-up.
 *
 * Everything the panel shows lives in the PopUpUI block at 0x007fdea4
 * (bighelp.c's PopUpUI; fpui2.c views the same block from +0x1c as
 * PopUpInfo).  PopUpInfoSetUp (0x00471950) fills it, InitPopUpInfo
 * (0x00470bb0) created the icons and loaded the sprites, and this function
 * re-renders it every frame.
 *
 * GATES
 *   g_popup.active  0 = nothing (return), 1 = the panel, 2 = the mock panel
 *                   (DrawPopUpMock 0x004720a0) and return.
 *   EditMode (0x008119b0) non-zero        -> ResetInfoStruct, return.
 *   g_input.mouse_c.state & 2 (right button released this tick)
 *                                         -> ResetInfoStruct, return.
 * The leading GetNearestColour(0xda, 0xc6, 0x96) is dead -- its result is
 * never used; reproduced because the call is in the original.
 *
 * TEXT (title `name`, body `info`, both char[256]; `line` is a char[512]
 * scratch that is strcat'd onto `info`).  Selected by g_popup.kind:
 *
 *  0x103  a placed object
 *      title = ObjDef.name (ODF record +0x78)
 *      body  = "<STR 0x76> <GetObjRepairCost(cls, cell->life)>\n"
 *              "<STR 0x77> <GetObjSalvageValue(cls, cell->life)>"
 *              -- money.c's straight-line depreciation over the cell's
 *              REMAINING life (Cell +0x11) against the class's initial life
 *              (ObjDef +0x2c).
 *      then, only when g_power_available (0x0083298c) is set,
 *      power = FindObjectsPower(cls) and one more line is appended:
 *          power < 0 : "\n<STR 0x78> <-power>"   (consumption) and, if the
 *                      cell is blacked out (Cell.flags & 0x100), a second
 *                      line "\n" + STR 0x7a ("no power").
 *          power > 0 : cell->life >= cls->life/4 -> "\n<STR 0x79> <power>"
 *                      (generation), otherwise "\n<STR 0x7b>" (too broken
 *                      to generate).
 *          power = 0 : nothing.
 *      has_life is set when the class has an initial life, which is what
 *      turns the bar at the bottom into a CONDITION bar.
 *  0x14   mechanic's hut: title = class name, body = "<STR 0x93> : <n>"
 *      with n = GetMechanicCount(), then the same repair/salvage pair on
 *      two further lines; shows the "hire mechanic" icon.
 *  0xa    gardener's shed: identical with STR 0x91 / GetGardenerCount();
 *      shows the "hire gardener" icon.
 *  0x104  under construction: title = class name, body = STR 0xa0, and
 *      g_pu_building (0x0066895c) is set so the bar becomes a BUILD
 *      PROGRESS bar.
 *  0x306  a visitor / worker: a kind-3 bloke whose condition byte (+0x60)
 *      has reached 0xc closes the pop-up; otherwise title =
 *      GetVisitorName(worker), body = "" and g_popup.named is set.
 *  0x10b / 0x10c  a ride (queue / ride itself): if the ride's current rider
 *      (record +0x1c) has condition >= 0x6b the pop-up re-opens itself as
 *      kind 0x104; otherwise title = the ride name (*(char**)(ride +0x04))
 *      and body = STR 0xd2 (0x10b) or STR 0xd3 (0x10c); shows the second
 *      delete icon.
 *  0x103 / 0x14 / 0xa additionally allow DELETION when the cell is not
 *      flagged 0x40 (blocked for building).
 *
 * LAYOUT.  g_popup.size (0x007fdfac) is the panel height in "lines"; while
 * g_popup.resize is set it is recomputed as the larger of
 * MeasurePopUpTitle(name, 0x40, 0x14, 0xb0, 0x20, 1) and
 * MeasurePopUpBody(info, 0x40, 0x14, 0xb0, 0x20, 2) -- fixed at 2 for a
 * worker panel.  ClampPopUpToScreen keeps the panel on screen and
 * DrawPopUpFrame paints the nine-slice background.  With
 * px = g_popup.pos.x, py = g_popup.pos.y and n = size, the text is drawn
 * between PushRenderingStatusAndUnlockVideoSurface / PopRenderingStatus:
 *      title: (px+0x0c, py+0x06)  (n*0x20+0xb0) x 0x13         flags 1, 1
 *      body : (px+0x0c, py+0x23)  (n*0x20+0xb0) x (n*20+0x40)  flags 2, 0x10
 * both with ink 0xff0000 and paper 0xffffff.  The `if (name)` / `if (info)`
 * guards are always true (they test the address of a local array) but VC6
 * emits the lea/test/je, so they are written out.
 *
 * WORKER EXTRAS (kind 0x306): two half-width columns of text, STR 0x8e on
 * the left and STR 0x8f centred, at the vertical middle of the body + 0x22;
 * then two sprites at that height minus 0x20 -- the MOOD sprite at the left
 * quarter (GetBlokeMood: 3 = spr_sad, 2 = spr_happy, else spr_norm) and the
 * HUNGER sprite at the right quarter (GetBlokeAgeGroup: 0 = spr_full,
 * 1 = spr_peckish, else spr_hungry).
 *
 * THE BAR.  RenderBlock(px+6, py+n*20+0x6f, n*0x20+0xbc, 6, 0) paints the
 * trough, then the same rectangle scaled by `frac` is painted red
 * (GetNearestColour(0xff,0,0)) when frac < 0.25 AND this is a condition bar,
 * green (0,0xff,0) otherwise.
 *      condition : frac = cell->life / cls->life
 *      build     : frac = g_build_slots[i].timer / GetBuildTime(cls), where
 *                  the slot is found by matching the packed cell
 *                  (short)g_popup.key.ref against BuildSlot.key over the 256
 *                  slots at 0x006664f8 (buildtick.c).  When that ratio is
 *                  exactly 1.0 the pop-up re-opens itself as kind 0x103.
 *      With neither (no life and not building) the bar is skipped entirely.
 *
 * ICONS.  right = px + n*0x20 + 0xc8, top = py + n*20 + 0x78.  Each icon
 * shown is un-hidden (flags &= ~0x400) and positioned at (x, top):
 *      icon_close     right-0x27   always
 *      icon_delete2   right-0x4e   ride kinds (0x10b / 0x10c)
 *      icon_delete    right-0x4e   deletable cell and PopUpCanDelete()
 *      icon_gardener  right-0x75 when the delete icon is also shown, else
 *                     right-0x4e  (kind 0xa)
 *      icon_mech      same rule    (kind 0x14)
 *      icon_corner    the x of the LAST icon placed
 * Finally the mouse point (g_input.point) is checked against
 * [corner-or-close x .. icon_close->x + 0x24] x [top .. top+0x1b]; leaving
 * that strip removes the icons again (ClosePopUpIcons), and an "expanded"
 * pop-up additionally runs DrawPopUpExtra.
 *
 * EVERY FIELD THE PANEL SHOWS, AND WHERE IT COMES FROM
 * (offsets are into the PopUpUI block at 0x007fdea4 unless stated):
 *
 *   title text        char name[256], built by Format:
 *                       kinds 0x103/0x14/0xa/0x104 : cls->name    (ObjDef +0x78)
 *                       kind  0x306                : GetVisitorName(g_popup.worker)
 *                       kinds 0x10b/0x10c          : *ride->name  (*(char**)(ride+4))
 *   body text         char info[256] (+ char line[512] strcat'd onto it):
 *                       0x103 : "<STR 0x76> %d\n<STR 0x77> %d" with
 *                               GetObjRepairCost(cls, cell->life) and
 *                               GetObjSalvageValue(cls, cell->life)
 *                       0x14  : "<STR 0x93> : %d"  n = GetMechanicCount(), then
 *                               "\n<STR 0x76> %d\n<STR 0x77> %d" appended
 *                       0xa   : "<STR 0x91> : %d"  n = GetGardenerCount(), same tail
 *                       0x104 : STR 0xa0 ("under construction")
 *                       0x306 : "" (kEmpty)
 *                       0x10b : STR 0xd2   0x10c : STR 0xd3
 *   power line        only when g_power_available (0x0083298c); appended to info:
 *                       FindObjectsPower(cls) < 0 -> "\n<STR 0x78> %d" with -power,
 *                         plus "\n" + STR 0x7a when the cell is blacked out
 *                         (Cell.flags +0x0c bit 0x100)
 *                       > 0 -> cell->life >= (unsigned char)(cls->life >> 2)
 *                         ? "\n<STR 0x79> %d" with power : "\n" + STR 0x7b
 *                       = 0 -> nothing appended
 *   worker labels     STR 0x8e (left half, flag 0x11) and STR 0x8f (right half)
 *   mood sprite       GetBlokeMood(worker): 3 -> spr_sad (+0x124), 2 -> spr_happy
 *                     (+0x174), else spr_norm (+0x140)
 *   hunger sprite     GetBlokeAgeGroup(worker): 0 -> spr_full (+0x008),
 *                     1 -> spr_peckish (+0x164), else spr_hungry (+0x12c)
 *   bar value         condition bar: Cell.life (+0x11, unsigned char) / ObjDef.life
 *                                    (+0x2c, unsigned char) -- fild/fidiv, both ints
 *                     build bar    : BuildSlot.timer / GetBuildTime(cls).  The slot
 *                                    is found by scanning the 256 12-byte records at
 *                                    0x006664f8 for BuildSlot.key (+4, a WORD) equal
 *                                    to (short)g_popup.key.ref; timer is +8.  A ratio
 *                                    of exactly 1.0f (fcomp [0x4ab38c]) re-opens the
 *                                    pop-up as kind 0x103.
 *   bar colour        red   GetNearestColour(0xff,0,0) when ratio < 0.25 (a POOLED
 *                           DOUBLE at 0x4ab520, fcomp qword) AND it is a condition bar
 *                     green GetNearestColour(0,0xff,0) otherwise
 *   panel height      g_popup.size (+0x108, a BYTE) = max(MeasurePopUpTitle(name,...,1),
 *                     MeasurePopUpBody(info,...,2)), or a fixed 2 for kind 0x306
 *   panel origin      g_popup.pos (+0x028) -- px = +0x028, py = +0x02c
 *   delete allowed    kinds 0x103/0x14/0xa when !(Cell.flags & 0x40), gated again at
 *                     draw time by PopUpCanDelete()
 *
 * PopUpInfoSetUp TAKES ITS FIRST THREE PARAMETERS AS ONE 12-BYTE RECORD.
 * A previous reconstruction called it PopUpInfoSetUp(type, obj, ref, Pos) --
 * five dwords, which is what the CALLEE (fpui2.c, 0x00471950) reads.  But both
 * call sites here build the argument area with `sub esp,0xc` + three stores
 * through `mov reg,esp`, which is VC6's struct-by-value copy, not five pushes:
 *
 *      mov  edx,[0x7fded0]      ; pos.y          push edx
 *      mov  ecx,[0x7fdecc]      ; pos.x          push ecx
 *      sub  esp,0xc                              ; the 12-byte record
 *      mov  eax,0x104
 *      mov  [0x7fdec0],eax      ; g_popup.key.type = 0x104
 *      mov  [edx],eax / [edx+4],[0x7fdec4] / [edx+8],[0x7fdec8]
 *
 * so the caller-side prototype is PopUpInfoSetUp(PopUpKey key, int x, int y)
 * with PopUpKey = { int type; void* obj; int ref; } at 0x007fdec0.  The stack
 * image is identical to the five-scalar form, which is why fpui2.c's
 * definition still matches; only the CALLER's spelling differs, and spelling
 * it as the record is what makes these two blocks come out instruction for
 * instruction.  Writing it the old way costs six instructions and, worse,
 * frees ebp early enough that VC6 hoists constants into it (see below).
 *
 * RUNTIME STATE BLOCK (absolute VAs, for the browser runtime).  Everything
 * the panel needs is in the PopUpUI record based at 0x007fdea4; the fields
 * this function reads or writes are:
 *      0x007fdea4  icon_mech      +0x000   hire-mechanic icon
 *      0x007fdec0  key.type       +0x01c   the kind PopUpInfoSetUp was given
 *      0x007fdec4  key.obj        +0x020
 *      0x007fdec8  key.ref        +0x024   packed {u8 x, u8 y} map cell, and
 *                                          the build-slot search key
 *      0x007fdecc  pos.x          +0x028   panel origin, px
 *      0x007fded0  pos.y          +0x02c   panel origin, py
 *      0x007fdf7c  cls            +0x0d8   ObjDef* of the object shown
 *      0x007fdf80  ride           +0x0dc   RideRec* (kinds 0x10b / 0x10c)
 *      0x007fdf84  cell           +0x0e0   Cell* under the pop-up
 *      0x007fdf8c  worker         +0x0e8   RideBloke* (kind 0x306)
 *      0x007fdf98  named          +0x0f4   set for a named visitor
 *      0x007fdf9c  kind           +0x0f8   selects the whole panel content
 *      0x007fdfa0  active         +0x0fc   0 off, 1 panel, 2 mock panel
 *      0x007fdfa4  expanded       +0x100   run DrawPopUpExtra as well
 *      0x007fdfa8  resize         +0x104   recompute `size` this frame
 *      0x007fdfac  size           +0x108   panel height in lines (a BYTE)
 *      0x007fdfc0  icon_close     +0x11c
 *      0x007fdfc8  spr_sad        +0x124
 *      0x007fdfcc  icon_delete2   +0x128
 *      0x007fdfd0  spr_hungry     +0x12c
 *      0x007fdfd8  icon_corner    +0x134
 *      0x007fdfdc  icon_delete    +0x138
 *      0x007fdfe0  icon_gardener  +0x13c
 *      0x007fdfe4  spr_norm       +0x140
 *      0x007fdeac  spr_full       +0x008
 *      0x007fe008  spr_peckish    +0x164
 *      0x007fe018  spr_happy      +0x174
 * plus, outside the block:
 *      0x0066895c  g_pu_building  set by kind 0x104 -> the bar is a BUILD bar
 *      0x0083298c  g_power_available   gates the power line
 *      0x008119b0  EditMode       non-zero closes the pop-up
 *      0x00813a40  GamePad        g_input: mouse_c.state bit 2 closes it,
 *                                 point.x/point.y drive the icon hover strip
 *      0x006664f8  g_build_slots  256 x 12-byte {?, u16 key @+4, timer @+8}
 * An Icon is positioned by writing x at +0x0c and y at +0x0e (both SHORT) and
 * cleared of the hidden bit with flags(+0x34) &= ~0x400.
 *
 * MATCH STATE (2026-09-03, fourth pass): 962/962 instructions, 3139 vs 3141
 * bytes, mismatch = 53 by tools/audit.py (886 -> 705 -> 398 -> 60 -> 53).
 * Every block, call, string id, constant, branch and register is now the
 * original's; what is left is ONE allocation decision in the kind-0x306
 * worker head (indices 578-603, 12 instructions) and the SPILL-SLOT ORDER it
 * drags along (the other 41: has_life/cond/cls, px/show_delete2 and
 * show_mech/halfw/show_gardener sit in permuted homes, so every reload of
 * them differs by an offset).  First diverging index: 25 (a zero store to a
 * permuted home).
 *
 * HOW IT GOT HERE (each verified against the disassembly; keep them):
 *   1. The `can_delete` test is written out three times, once per object
 *      case, NOT through a shared label (705 -> 432): both spellings give the
 *      same cross-jumped block, but the copies place the post-switch join
 *      after case 0xa with the one-instruction `xor esi,esi` block the
 *      original has.
 *   2. `box.top`/`box.bottom` are re-established in the worker block before
 *      the midpoint (432 -> 398): as one expression VC6 reassociates
 *      `2*(py + 10*lines) + 0x86` with one py reload; the original keeps the
 *      `lea [eax+edx*4+0x63]` and reloads py for the `+ 0x23` term.
 *   3. (398 -> 60, third pass) the mood/hunger arms take `ty - 0x20` as an
 *      expression with a named `w = box.right - box.left` (`w / 4` in the
 *      arms) -- VC6 then keeps `w` in ebx, spills `ty` and tail-merges the
 *      three `push <sprite>` into one `call PrintSprite` exactly as the
 *      original; the two ride cases are written out in full (0x10b then
 *      0x10c, each with its own reopen block; VC6 cross-jumps them into the
 *      original's layout); the build-progress arm is `if (has_life != 0)
 *      {...} else {...}` after the slot scan, and the bar's `box.left/top`
 *      are assigned before the three RenderBlock calls.
 *   4. (60 -> 53, this pass) the mouse strip's right edge is a NAMED step,
 *      `box.right = g_popup.icon_close->x + 0x24;` before the test.  Spelled
 *      inline the three scratch temporaries (strip left, right edge, mouse
 *      x) rotate one register (ecx/edx/eax where the original has
 *      edx/eax/ecx).  Every other strip spelling measured -- `mx` temp,
 *      ternary, `box.left` as the strip variable, arm order, nested ifs,
 *      `>` instead of `<`, a `short` left, a `Pos pt` copy -- is 60-141.
 *
 * WHAT IS LEFT, precisely.  Original worker head after the two bloke calls:
 *      mov eax,[py]; mov ebx,edi; sub ebx,ebp        ; w
 *      lea ecx,[eax+edx*4+0x63]                       ; bottom (NEW register)
 *      mov eax,ebx; cdq; sub eax,edx; sar eax,1       ; halfw = w/2 FIRST
 *      mov [esp+halfw],eax                            ; stored AT THE DEF
 *      mov edx,[py]; lea eax,[ecx+edx+0x23]           ; bottom + top
 *      mov ecx,[esp+halfw]                            ; halfw RELOADED
 *      cdq; sub; sar; mov [esp+ty],eax; add eax,0x22  ; ty, ty+0x22
 * i.e. halfw is computed before the midpoint and is memory-resident between
 * its def and its push (spilled everywhere), the midpoint takes eax over it,
 * and `bottom` keeps ecx.  Ours (`ty` then `halfw` in the source) computes
 * the midpoint first with `bottom` in place, then halfw in eax straight into
 * its push and a sunk home store.  With `halfw = w / 2` textually BEFORE
 * `ty = ...` (any of the 84 valid orderings of the seven head statements,
 * all measured) VC6 emits the first six instructions exactly as the original
 * and then makes the OTHER choice: it keeps halfw's pre-`sar` partial in
 * ecx, defers the `sar` to the push and spills `bottom` to a NEW frame slot
 * (frame 0x444, 421 mismatches).  Whichever of {halfw, bottom} loses the
 * register also decides the global spill-home order: the two probes that
 * take halfw out of the register race -- `volatile int halfw` (A3) and
 * computing halfw between the two bloke calls (Vi, halfw then crosses
 * GetBlokeAgeGroup and is spilled at its def) -- both give the ORIGINAL's
 * order for has_life/cond/cls, px/show_delete2 and show_mech/show_gardener,
 * with only halfw/ty/mood themselves placed elsewhere; neither is a
 * candidate (964 instructions / calls moved).  So the lever wanted is
 * whatever lowers halfw's priority below the `bottom` temporary's without
 * moving code.  Measured and INERT (byte-identical to the current text):
 * renaming any local; `w / 2` written textually in both calls; a named
 * `ty2 = ty + 0x22`; a named `mid`; `ty` as two/three statements; block-
 * scoping any of mood/cond/halfw/ty/w; `unsigned`/`long` halfw, ty or w and
 * `(long)` casts (same-width conversion is NOT a barrier here); one-element
 * arrays and one-member structs (promoted); `int* p = &halfw; *p = ...`,
 * `static __inline` helpers taking `int* out` or with their own address-
 * taken local (all un-escaped); a dead `halfw = 0` / `ty = 0` at the top.
 * Measured and WORSE: every halfw-before-ty ordering (374-570); the same
 * with `volatile` (73, +2 instructions); degenerate `if (c) halfw = w/2;
 * else halfw = w/2;` on mood/cond/lines/w/has_life (398-771, VC6 does not
 * merge them here); `box.bottom += box.top` / `box.top += box.bottom` sum
 * reuse (527); `bottom`/`top` inline in the midpoint (387/770); halfw from
 * `lines` (535); `ty -= 0x20` inside the arms (385); `unsigned w` (371).
 * Slot facts for whoever continues: spill homes are handed out in priority
 * order low-to-high with first-fit reuse of a dead home (ty shares 0x10 with
 * frac, the int-to-float staging temps reuse cond's and cls's homes once
 * they die), so a single allocation flip in the worker head re-sorts the
 * whole frame; declaration order and names are irrelevant under /O2.
 * Tooling: scratchpad/popup/vb.py (patch-batch runner, -v keeps the
 * listings), slotmap.py (prints the frame-home map of a listing), dp_v16..29
 * are this pass's variant sets.
 *
 * Still true from earlier passes: the two ride cases share one tail (VC6
 * cross-jumps case 0x10c into case 0x10b at the `call GetString`); `top` is
 * computed before `right` in the icon section, which removes a reload of
 * g_popup.pos.y; PopUpInfoSetUp's first three arguments are ONE 12-byte
 * record (see above); the seven switch cases compile byte-identically in
 * any order (VC6 emits each dispatch group in descending case value), as
 * does an empty `default:`.
 * ------------------------------------------------------------------------- */

// WIP-FUNCTION: LEGOLAND 0x004724a0  (962/962 instructions, 3139 vs 3141 bytes, mismatch=886->705->398->60->53 by audit.py; one allocation choice in the kind-0x306 worker head -- halfw is spilled at its def before the midpoint in the original, kept in eax after it in ours -- and the spill-home order it drags along; first diff at index 25)
void DrawPopUpInfo(void)
{
    char    name[256] = {0};
    char    info[256] = {0};
    char    line[512];
    ObjDef* cls;
    RideBloke* worker;
    int     can_delete;
    int     has_life;
    int     show_gardener;
    int     show_mech;
    int     show_delete2;
    int     px, py;
    /* The text/icon rectangle.  It must be ONE aggregate: with four separate
     * ints VC6 propagates each corner's definition into the `right - left` /
     * `bottom - top` argument and reassociates it ((py - top) + 0x19 instead
     * of (py + 0x19) - top), which costs an instruction in each of the two
     * PrintCachedText blocks and stops `top` being given ebp. */
    struct { int left, top, right, bottom; } box;
    int     lines;
    int     power;
    int     mood, cond;
    int     halfw, ty, barw, w;
    int     i;
    float   frac;

    has_life = 0;
    can_delete = 0;
    show_gardener = 0;
    show_mech = 0;
    show_delete2 = 0;
    cls = g_popup.cls;
    worker = (RideBloke*)g_popup.worker;
    GetNearestColour(0xda, 0xc6, 0x96);
    g_pu_building = 0;
    if (g_popup.active == 2) {
        DrawPopUpMock();
        return;
    }
    if (g_edit_mode != 0) {
        ResetInfoStruct();
        return;
    }
    if (g_popup.active == 0)
        return;
    if (g_input.mouse_c.state & 2) {
        ResetInfoStruct();
        return;
    }

    switch (g_popup.kind) {
    case 0x103:
        Format(name, kFmtStr, cls->name);
        Format(info, kFmtSDSD,
               GetString(0x76), GetObjRepairCost(cls, g_popup.cell->life),
               GetString(0x77), GetObjSalvageValue(cls, g_popup.cell->life));
        if (g_power_available != 0) {
            power = FindObjectsPower(cls);
            if (power < 0) {
                Format(line, kFmtNlSD, GetString(0x78), -power);
                if (g_popup.cell->flags & 0x100) {
                    strcat(line, kFmtNl);
                    strcat(line, GetString(0x7a));
                }
            } else if (power != 0) {   /* > 0; spelled != 0 -- the original's
                                        * second test is a bare `je`, not `jle` */
                if (g_popup.cell->life >= (unsigned char)(cls->life >> 2))
                    Format(line, kFmtNlSD, GetString(0x79), power);
                else
                    Format(line, kFmtNlS, GetString(0x7b));
            }
            if (power != 0)
                strcat(info, line);
        }
        if (cls->life != 0)
            has_life = 1;
        /* The `can_delete` test is written out in all three object cases (VC6
         * cross-jumps them into the single block at 0x004728e3 that the
         * original has).  Routing them through one `goto object_common:` label
         * instead compiles to the same three-predecessor block but places the
         * post-switch join AFTER the last case, which costs the shared
         * `xor esi,esi` and shifts every later instruction. */
        if (!(g_popup.cell->flags & 0x40))
            can_delete = 1;
        break;
    case 0x14:
        Format(name, kFmtStr, cls->name);
        Format(info, kFmtColon, GetString(0x93), GetMechanicCount());
        Format(line, kFmtNlSDSD,
               GetString(0x76), GetObjRepairCost(cls, g_popup.cell->life),
               GetString(0x77), GetObjSalvageValue(cls, g_popup.cell->life));
        strcat(info, line);
        show_mech = 1;
        if (!(g_popup.cell->flags & 0x40))
            can_delete = 1;
        break;
    case 0xa:
        Format(name, kFmtStr, cls->name);
        Format(info, kFmtColon, GetString(0x91), GetGardenerCount());
        Format(line, kFmtNlSDSD,
               GetString(0x76), GetObjRepairCost(cls, g_popup.cell->life),
               GetString(0x77), GetObjSalvageValue(cls, g_popup.cell->life));
        strcat(info, line);
        show_gardener = 1;
        if (!(g_popup.cell->flags & 0x40))
            can_delete = 1;
        break;
    case 0x104:
        Format(name, kFmtStr, cls->name);
        Format(info, kFmtStr, GetString(0xa0));
        g_pu_building = 1;
        break;
    case 0x306:
        if (worker->kind == 3 && worker->cond >= 0xc) {
            ResetInfoStruct();
            return;
        }
        Format(name, GetVisitorName(g_popup.worker));
        Format(info, kEmpty);
        g_popup.named = 1;
        break;
    case 0x10b:
        if (((RideRec*)g_popup.ride)->f18 != 0
            && ((RideRec*)g_popup.ride)->rider->cond >= 0x6b) {
            g_popup.key.type = 0x104;
            PopUpInfoSetUp(g_popup.key, g_popup.pos.x, g_popup.pos.y);
            return;
        }
        Format(name, kFmtStr, *((RideRec*)g_popup.ride)->name);
        Format(info, kFmtStr, GetString(0xd2));
        show_delete2 = 1;
        break;
    case 0x10c:
        if (((RideRec*)g_popup.ride)->f18 != 0
            && ((RideRec*)g_popup.ride)->rider->cond >= 0x6b) {
            g_popup.key.type = 0x104;
            PopUpInfoSetUp(g_popup.key, g_popup.pos.x, g_popup.pos.y);
            return;
        }
        Format(name, kFmtStr, *((RideRec*)g_popup.ride)->name);
        Format(info, kFmtStr, GetString(0xd3));
        show_delete2 = 1;
        break;
    }

    if (g_popup.resize != 0) {
        if (g_popup.kind == 0x306) {
            g_popup.size = 2;
        } else {
            int a = MeasurePopUpTitle(name, 0x40, 0x14, 0xb0, 0x20, 1);
            int b = MeasurePopUpBody(info, 0x40, 0x14, 0xb0, 0x20, 2);
            g_popup.size = (unsigned char)b;
            if (b <= a)
                g_popup.size = (unsigned char)a;
        }
        g_popup.resize = 0;
    }
    ClampPopUpToScreen(g_popup.size);
    lines = g_popup.size;
    DrawPopUpFrame();
    PushRenderingStatusAndUnlockVideoSurface();
    px = g_popup.pos.x;
    py = g_popup.pos.y;
    if (name) {
        box.left = px + 0xc;
        box.top = py + 6;
        box.right = lines * 0x20 + px + 0xbc;
        box.bottom = py + 0x19;
        PrintCachedText(name, box.left, box.top, box.right - box.left, box.bottom - box.top,
                        1, 1, 0xff0000, 0xffffff);
    }
    if (info) {
        box.left = px + 0xc;
        box.top = py + 0x23;
        box.right = lines * 0x20 + px + 0xbc;
        box.bottom = py + lines * 20 + 0x63;
        PrintCachedText(info, box.left, box.top, box.right - box.left, box.bottom - box.top,
                        2, 0x10, 0xff0000, 0xffffff);
    }
    PopRenderingStatus();

    if (g_popup.kind == 0x306) {
        mood = GetBlokeMood(worker);
        cond = GetBlokeAgeGroup(worker);
        box.left = px + 0xc;
        box.right = lines * 0x20 + px + 0xb0;
        /* box.top/box.bottom are re-established here (they still hold these
         * values from the `info` block) so VC6 keeps the two halves of the
         * midpoint apart: spelled as one expression it reassociates into
         * 2*(py + 10*lines) + 0x86 and reloads py only once, where the
         * original recomputes `py + lines*20 + 0x63` with an lea and reloads
         * py for the `+ 0x23` term. */
        box.top = py + 0x23;
        box.bottom = py + lines * 20 + 0x63;
        w = box.right - box.left;
        ty = (box.bottom + box.top) / 2;
        halfw = w / 2;
        PrintCachedText(GetString(0x8e), box.left, ty + 0x22, halfw, 0x14,
                        2, 0x11, 0xff0000, 0xffffff);
        PrintCachedText(GetString(0x8f), (box.left + box.right) / 2, ty + 0x22, halfw, 0x14,
                        2, 0x11, 0xff0000, 0xffffff);
        if (mood == 3)
            PrintSprite(g_popup.spr_sad, box.left + w / 4 - 0x20, ty - 0x20, 0, 0);
        else if (mood == 2)
            PrintSprite(g_popup.spr_happy, box.left + w / 4 - 0x20, ty - 0x20, 0, 0);
        else
            PrintSprite(g_popup.spr_norm, box.left + w / 4 - 0x20, ty - 0x20, 0, 0);
        if (cond == 0)
            PrintSprite(g_popup.spr_full, box.right - w / 4 - 0x20, ty - 0x20, 0, 0);
        else if (cond == 1)
            PrintSprite(g_popup.spr_peckish, box.right - w / 4 - 0x20, ty - 0x20, 0, 0);
        else
            PrintSprite(g_popup.spr_hungry, box.right - w / 4 - 0x20, ty - 0x20, 0, 0);
    }

    if (has_life == 0) {
        if (g_pu_building == 0)
            goto icons;
        for (i = 0; i < 256; i++)
            if (g_build_slots[i].key == (short)g_popup.key.ref)
                break;
    }
    if (has_life != 0) {
        frac = (float)g_popup.cell->life / (float)cls->life;
    } else {
        if (i >= 256)
            return;
        frac = (float)g_build_slots[i].timer / (float)GetBuildTime(g_popup.cls);
        if (frac == 1.0f) {
            g_popup.key.type = 0x103;
            PopUpInfoSetUp(g_popup.key, g_popup.pos.x, g_popup.pos.y);
            return;
        }
    }
    box.left = px + 6;
    box.top = py + lines * 20 + 0x6f;
    barw = lines * 0x20 + 0xbc;
    RenderBlock(box.left, box.top, barw, 6, 0);
    if (frac < 0.25 && has_life)
        RenderBlock(box.left, box.top, (int)(barw * frac), 6,
                    GetNearestColour(0xff, 0, 0));
    else
        RenderBlock(box.left, box.top, (int)(barw * frac), 6,
                    GetNearestColour(0, 0xff, 0));

icons:
    box.top = py + (lines * 5 + 0x1e) * 4;
    box.right = lines * 0x20 + px + 0xc8;
    g_popup.icon_close->flags &= 0xfffffbff;
    g_popup.icon_close->x = (short)(box.right - 0x27);
    g_popup.icon_close->y = (short)box.top;
    box.left = g_popup.icon_close->x;
    if (show_delete2) {
        g_popup.icon_delete2->flags &= 0xfffffbff;
        g_popup.icon_delete2->x = (short)(box.right - 0x4e);
        g_popup.icon_delete2->y = (short)box.top;
        box.left = g_popup.icon_delete2->x;
    }
    if (can_delete && PopUpCanDelete()) {
        g_popup.icon_delete->flags &= 0xfffffbff;
        g_popup.icon_delete->x = (short)(box.right - 0x4e);
        g_popup.icon_delete->y = (short)box.top;
        box.left = g_popup.icon_delete->x;
    }
    if (show_gardener) {
        g_popup.icon_gardener->flags &= 0xfffffbff;
        g_popup.icon_gardener->y = (short)box.top;
        if (can_delete)
            g_popup.icon_gardener->x = (short)(box.right - 0x75);
        else
            g_popup.icon_gardener->x = (short)(box.right - 0x4e);
        box.left = g_popup.icon_gardener->x;
    }
    if (show_mech) {
        g_popup.icon_mech->flags &= 0xfffffbff;
        g_popup.icon_mech->y = (short)box.top;
        if (can_delete)
            g_popup.icon_mech->x = (short)(box.right - 0x75);
        else
            g_popup.icon_mech->x = (short)(box.right - 0x4e);
        box.left = g_popup.icon_mech->x;
    }
    g_popup.icon_corner->x = (short)box.left;
    g_popup.icon_corner->y = (short)box.top;
    g_popup.icon_corner->flags &= 0xfffffbff;
    box.bottom = box.top + 0x1b;
    if (g_popup.expanded)
        halfw = g_popup.icon_close->x;
    else
        halfw = g_popup.icon_corner->x;
    /* The strip's right edge is a named step: spelled inline in the test the
     * three scratch temporaries (strip left, right edge, mouse x) rotate one
     * register (ecx/edx/eax instead of edx/eax/ecx); as `box.right` the
     * original's allocation comes out exactly. */
    box.right = g_popup.icon_close->x + 0x24;
    if (box.right < g_input.point.x || g_input.point.x < halfw)
        ClosePopUpIcons();
    if (box.bottom < g_input.point.y || g_input.point.y < box.top)
        ClosePopUpIcons();
    if (g_popup.expanded)
        DrawPopUpExtra();
    DrawPopUpEnd();
}
