/* LEGOLAND decompilation — shared types.
 *
 * Struct layouts are recovered from the binary (field offsets confirmed against
 * the disassembly). Names are ours; only offsets are load-bearing. */
#ifndef LEGOLAND_H
#define LEGOLAND_H

/* The per-cell "layer" holder: parallel arrays indexed by layer number, holding
 * each layer's sprite object and its render offset. (GetSpriteForLayer /
 * GetLLSForLayer / GetRenderOffsetForLayer @ 0x00441e80..0x00441ee0.) */
typedef struct Layers {
    int    pad0;       /* +0x00 */
    int    pad4;       /* +0x04 */
    void** sprites;    /* +0x08  sprite object per layer */
    int*   render_ox;  /* +0x0c  x render offset per layer */
    int*   render_oy;  /* +0x10  y render offset per layer */
} Layers;

/* A map cell / render object; its layer holder lives at +0x08. */
typedef struct RenderObj {
    int     pad0;      /* +0x00 */
    int     pad4;      /* +0x04 */
    Layers* layers;    /* +0x08 */
} RenderObj;

/* A per-layer sprite object; its LLS is *(+0x08). */
typedef struct SpriteObj {
    int    pad0;       /* +0x00 */
    int    pad4;       /* +0x04 */
    void** lls_holder; /* +0x08  *lls_holder = the LLS */
} SpriteObj;

/* An 8-byte {x,y} pair, returned in eax:edx. */
typedef struct Offset {
    int ox;
    int oy;
} Offset;

/* An inclusive rectangle, chained into a list; area sums over the chain. */
typedef struct Rect {
    int          left;    /* +0x00 */
    int          top;     /* +0x04 */
    int          right;   /* +0x08 */
    int          bottom;  /* +0x0c */
    struct Rect* next;    /* +0x10 */
} Rect;

/* A loaded tile/image sprite record; width/height live at +0x14/+0x16. */
typedef struct Sprite {
    char  pad[0x14];   /* +0x00 */
    short w;           /* +0x14 */
    short h;           /* +0x16 */
} Sprite;

/* A map cell — 20 bytes (0x14). Field offsets confirmed against SetMapTile
 * (0x00461780), Set_RFFlags (0x004616e0) and the flag getters. */
typedef struct Cell {
    void*          obj;     /* +0x00 render/tile object */
    unsigned char  bx;      /* +0x04 */
    unsigned char  by;      /* +0x05 */
    unsigned short pad6;    /* +0x06 */
    unsigned short tile;    /* +0x08 displayed tile (SetMapTile) */
    unsigned short base;    /* +0x0a ground/terrain tile */
    unsigned short flags;   /* +0x0c map flags */
    unsigned short uflags;  /* +0x0e user flags */
    unsigned char  rf;      /* +0x10 RF/path flags */
    unsigned char  pad11[3];/* +0x11..0x13 */
} Cell;

/* The map header; width/height at +0x14/+0x16 (@ 0x004bcbf4). */
typedef struct Map {
    char           pad[0x14]; /* +0x00 */
    unsigned short width;     /* +0x14 */
    unsigned short height;    /* +0x16 */
} Map;
extern Map*    g_map;

/* The map row table: g_map_rows[y][x] is a Cell (row pointers @ 0x00801400). */
extern Cell**  g_map_rows;
/* Default/base tile index into the global tile-sprite array (@ 0x00667ca4). */
extern int     g_default_tile;
/* Global loaded tile-sprite table (@ 0x00805f60). */
extern Sprite* g_tile_sprites[];

/* A tile's database element; a per-tile RF handler callback lives at +0x18. */
typedef unsigned char (*RFHandler)(int x, int y);
typedef struct TileElem {
    char      pad[0x18];  /* +0x00 */
    RFHandler handler;    /* +0x18 */
} TileElem;

/* Parallel tile-info table (@ 0x00801f40, stride 8): {elem ptr, tile code}. */
typedef struct TileInfo {
    TileElem*    elem;    /* +0x00 */
    unsigned int code;    /* +0x04 */
} TileInfo;
extern TileInfo g_tile_info[];

/* A grid position (map cell coordinates). */
typedef struct Pos {
    int x;               /* +0x00 */
    int y;               /* +0x04 */
} Pos;

/* An object class/descriptor placed on the map. type at +0x20 selects the
 * stat bucket; a Rect list at +0x3c gives its footprint; +0x98 is the
 * per-class placement callback. */
typedef struct ObjClass {
    char  pad0[0x20];    /* +0x00 */
    short type;          /* +0x20 */
    char  pad22[0x3c - 0x22];
    Rect  rect;          /* +0x3c footprint rect list */
    char  pad50[0x98 - (0x3c + sizeof(Rect))];
    void  (*place)(void* obj, Pos* pos); /* +0x98 */
} ObjClass;

/* A named database element (ElemID); its data record is at +0x0c. */
typedef struct ElemData {
    char pad0[0x3c];     /* +0x00 */
    int  origin;         /* +0x3c */
    int  base;           /* +0x40 */
    char pad44[0x48 - 0x44];
    int  span;           /* +0x48 */
} ElemData;
typedef struct Elem {
    char      pad0[0x0c]; /* +0x00 */
    ElemData* data;       /* +0x0c */
} Elem;

#endif /* LEGOLAND_H */
