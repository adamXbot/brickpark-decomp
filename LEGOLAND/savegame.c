/* LEGOLAND -- SaveGame / LoadGame: the complete .sav file format.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field OFFSETS, record sizes, callee argument counts and the global addresses
 * are load-bearing; the names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere) except for the handful legoland.h already
 * carries (Cell, Map, LLElem, TileInfo, Elem/ElemData).
 *
 * ==========================================================================
 *  THE SAVED-GAME FILE  ("profiles\<profile>save<slot>.sav")
 * ==========================================================================
 * The companion "%dsave%d.sh" file next to it is the 0x110-byte Profile
 * snapshot (see profiles.c / saveprof.c); THIS file is the world state.
 *
 * Everything goes through SaveGameWrite / SaveGameRead (saveprof.c) on the
 * raw CRT descriptor g_savefile_fd @0x6691b0. SaveGame opens the path with
 *     _open(path, _O_BINARY|_O_TRUNC|_O_CREAT|_O_RDWR, _S_IREAD|_S_IWRITE)
 * and LoadGame with plain _open(path, _O_BINARY).
 *
 * FRAMING. Sections are wrapped in "measured blocks" (profiles.c):
 * BeginMeasuredBlock writes a u32 placeholder and pushes its file offset;
 * EndMeasuredBlock back-patches it with the ABSOLUTE end offset of the
 * block and seeks back. So a chunk is  [u32 end_offset][payload...]  and the
 * end_offset is a file position, NOT a length. The loader never uses them: it
 * just consumes each u32 with SkipMeasuredBlock (0x0047d7e0) and re-reads the
 * payload positionally. The values exist so a future/partial loader can seek.
 *
 * LAYOUT (in write order; "[BLK n]" = one measured block):
 *
 *   char   magic[32]      "00002 LEGOLAND Save Game V0.02 \x1a"  (33-byte
 *                         literal, only 32 bytes written). LoadGame NUL-
 *                         terminates it at [5] and requires atoi() == 2.
 *   [BLK 13]  <- outer block, wraps everything below
 *     [BLK 1] element table
 *        u32 n_elements                      (also kept in g_elist_count)
 *        n_elements x { u32 len; char name[len]; u32 flags & 0x0003000e }
 *           - the set is every LLIDB element whose type_flags has bit 2
 *             (0x4 = "referenced by the map") set, plus "PATH CONTROL",
 *             which SaveGame force-marks before counting.
 *           - g_elist[] (@0x669200) is rebuilt as that array of LLElem*, and
 *             everything else in the file refers to elements BY INDEX into it.
 *           - on load: FindElement(name) must succeed, LoadData() is called,
 *             then type_flags = (type_flags & 0xfffcfff1) | saved_flags.
 *        u32 n_tsf                           (also kept in g_extra_elem_count)
 *        n_tsf x { u32 len; char name[len] } - the distinct tile-sprite (.TSF)
 *             tables actually referenced by the loaded tile set, harvested by
 *             CollectUsedTSFTables (0x0045aa50) walking g_tile_sprites[] /
 *             g_tile_info[]. On load they land in g_extra_elems[] (@0x7fd660)
 *             and their descriptors in a local table used to decode tiles.
 *     [BLK 2] the map grid
 *        u8  map_header[0x44]                 <- *g_map verbatim (w@0x14,
 *                                                h@0x16, max_blokes@0x1a)
 *        h*w x Cell[0x14]                     row-major, y outer
 *           Each cell is written from a COPY with three fields re-encoded so
 *           the file is pointer-free and tile-table independent:
 *             +0x00 obj  : if (flags & 0x08a8) the object pointer is replaced
 *                          by its g_elist index (FindeIneList); else 0.
 *             +0x08 tile : re-encoded as ((tsf_index+1) << 8) | (tile -
 *                          tsf_table->base_slot), or 0 when the tile's table
 *                          is not in the n_tsf list.  Same for
 *             +0x0a base : the ground/terrain tile.
 *           On load the inverse runs: obj = g_elist[idx] (or 0), and
 *           tile = tsf[(code>>8)-1]->base_slot + (code & 0xff) when non-zero.
 *     [BLK 3] world state
 *        u8  mapai[0x3f0]      @0x832800   the MapAI block (objmap.c)
 *        u32 scroll_x          @0x667cb4
 *        u32 scroll_y          @0x667cb8
 *        u8  editmode[0x0c]    @0x8119b0   (load forces [+4]=3, [+8]=0)
 *        <scripts>             SaveScripts   0x0046c920 / LoadScripts  0x46cb60
 *        <report>              SaveReport    0x00444200 / LoadReport   0x444260
 *        <currency>            SaveCurrency  0x00457910 / LoadCurrency 0x457940
 *        u8  buttonflash[0x24] @0x7fdd00
 *     [BLK 4] blokes
 *        u32 n_blokes                        (length of the g_people_head list)
 *        n_blokes x {
 *            u32 bloke_num                   GetBlokeNum -> pool slot index;
 *                                            the loader re-creates the bloke AT
 *                                            that slot (g_bloke_base + 172*n)
 *                                            and pushes it on g_people_head.
 *            u8  blokesave[0x124]            the flattened bloke, staged in the
 *                                            global num record @0x7fda60:
 *                                            the Bloke's own fields, its four
 *                                            LLIDB element pointers turned into
 *                                            g_elist indices (-1 = none) and
 *                                            the whole 0x94-byte Person3D.
 *            u8  bnvpath[0x48]               only when blokesave.bnv (the copy
 *                                            of Bloke+0x54) is non-NULL; the
 *                                            loader mallocs 0x48 and reads it.
 *        }
 *     [BLK 5]  SaveBlock5  0x0049c140 / LoadBlock5  0x0049c3c0
 *     [BLK 6]  SaveBlock6  0x0049c630 / LoadBlock6  0x0049c8b0
 *     [BLK 7]  SaveBlock7  0x0049cb20 / LoadBlock7  0x0049cc10
 *     [BLK 8]  SaveBlock8  0x0049cd10 / LoadBlock8  0x0049ce00
 *     [BLK 9] objects, instances and rides
 *        u32 castle_placed     @0x79a8d0
 *        u32 n_build_objs      @0x6670f8
 *        u8  buildobjlist[0xc00] @0x6664f8
 *        for each g_elist[i] whose type_flags & 0x10 (an ODF class):
 *            char class_name[8]              first 8 bytes of the element name
 *                                            (fixed field, NOT NUL-checked; the
 *                                            loader reads it into an 8-byte
 *                                            buffer pre-filled "xxxxxxxx" and
 *                                            just DBPrintf()s it)
 *            u8  been_on[map.max_blokes]     only when class kind (+0x20) is
 *                                            neither 0 nor 2 -- the per-bloke
 *                                            "has been on this ride" flags
 *                                            (0xc8); the loader mallocs them.
 *            u32 count                       ObjDef+0x08
 *            u32 n_instances                 length of the ObjDef+0x04 list
 *            n_instances x { u32 @+0x0c; u32 @+0x0e; u32 @+0x10 }
 *                                            *** as shipped: three 4-byte
 *                                            fields at +0x0c/+0x0e/+0x10 of a
 *                                            0x14-byte ObjInst, so they OVERLAP
 *                                            (12 bytes written for 8 bytes of
 *                                            state). Save and load agree, so
 *                                            the file round-trips.
 *            u32 n_riders                    length of the ObjDef+0xcc list
 *            n_riders x { u32 bloke_num; u16 ride_id@+0x0c }
 *                                            loader re-links the node, sets
 *                                            +0x08 = GetBlokePtr(num) and
 *                                            +0x10 = bloke->person.
 *            <ride-specific>                 ObjDef+0xbc (save) / +0xb8 (load)
 *                                            -- the per-ride serialisers in
 *                                            ridesave.c.
 *        u8  ridetotals[0x200] @0x7cb3e0
 *     [BLK 10] SavePathRects 0x00482860 / LoadPathRects 0x00482920
 *     [BLK 11] SaveBlock11   0x00450a80 / LoadBlock11   0x00450b10
 *     [BLK 12] terrain
 *        u32 len; char terrain_name[len]     g_terrain_elem   @0x801410
 *        u32 len2; char bridge_name[len2]    g_terrain_elem_2 @0x801404;
 *                                            len2 == 0 means "none" and the
 *                                            name bytes are absent. On load
 *                                            SetBridgeDrawOffsets(name) picks
 *                                            the per-theme bridge offsets.
 *        u32 n_terrain_objs
 *        n x u8[0x14]                        the perimeter/terrain render
 *                                            nodes (list head @0x667ca8, next
 *                                            @+0x1c), replayed with AddOvSav.
 *   (end of BLK 13)
 *
 * After the last block LoadGame closes the file and finishes the world:
 * clears the "loading" flag @0x667ca0, sets 0x813a40 |= 0x20,
 * CalculateMapRenderOrder(), recomputes the ENTRANCE 1 render origin
 * (g_entrance_x/y @0x4b8320/24) exactly as PutObjOnMap does, then
 * RestoreCurrentMenu / SetMapReady(1) / RestoreScriptStepHelp and returns 1.
 *
 * KNOWN ORIGINAL DEFECTS, reproduced faithfully:
 *  - LoadGame leaks the descriptor when the version check fails: it returns 0
 *    without _close().
 *  - The instance triple above writes overlapping 4-byte fields.
 *  - SaveGame ignores the result of several SaveGameWrite calls (the 8-byte
 *    class tag, and every write in the terrain block).
 *  - The Person3D field at +0x38 is staged twice (0x7fdb24 written from
 *    p->f38 in two separate statements).
 *
 * ==========================================================================
 */
#include "legoland.h"
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#pragma intrinsic(strlen, memcpy)

/* ---- save-game primitives (saveprof.c / profiles.c) --------------------- */
extern int  SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int  SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern int  SkipMeasuredBlock(void);                         /* 0x0047d7e0 */
extern int  BeginMeasuredBlock(void);                        /* 0x0047d790 */
extern int  EndMeasuredBlock(void);                          /* 0x0047d800 */
extern int  FindeIneList(void* pval);                        /* 0x0047d880 [sic] */
extern int  g_savefile_fd;                                   /* 0x006691b0 */
extern int  g_chunk_depth;                                   /* 0x006691fc */
extern LLElem** g_elist;                                     /* 0x00669200 */
extern int  g_elist_count;                                   /* 0x006691b4 */

/* ---- debug output -------------------------------------------------------- */
extern void DBPrintf(const char* fmt, ...);   /* 0x00453a20 */
extern void DBError(const char* fmt, ...);    /* 0x00453ce0 */

/* ---- LLIDB --------------------------------------------------------------- */
extern int   LLIDB_GetCount(void);                                   /* 0x47b2d0 */
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_GetElement(int index, LLElem** out);              /* 0x47b2e0 */
#else
extern int LLIDB_GetElement(int index, LLElem** out);              /* 0x47b2e0 */
#endif
extern int   LLIDB_FindElement(const char* name, LLElem** out, unsigned int* idx); /* 0x47b330 */
extern int   LLIDB_FindElementFromDataPtr(void* data, LLElem** out, unsigned int* idx); /* 0x47b410 */
extern void* LLIDB_LoadData(LLElem* elem);                           /* 0x47d3a0 */
extern LLElem* ElemID(const char* name);                             /* 0x47b3f0 */

extern void* HeapAlloc_w(unsigned int size);   /* 0x0049e4ff */
extern void  HeapFree_w(void* p);              /* 0x0049e4d0 */
extern void  progress_tick(void);              /* 0x004663f0 */

/* ---- string literals (only the addresses are load-bearing) --------------- */
extern const char kPathControl[];      /* 0x004b8a70 "PATH CONTROL" */
extern const char kEntrance1[];        /* 0x004b83d0 "ENTRANCE 1" */

/* ==========================================================================
 *  local types
 * ========================================================================== */

/* The map header seen with its per-map bloke capacity at +0x1a (legoland.h's
 * Map only names width/height); it is the size of a class's "been on" array. */
typedef struct MapEx {
    unsigned char  pad00[0x1a];
    unsigned short max_blokes;    /* +0x1a */
} MapEx;
extern MapEx* g_mapx;             /* 0x004bcbf4 (== g_map) */

/* VC6 copies a 2- or 3-dword aggregate with register moves off ONE base, and
 * that grouping is exactly what the original's staging code shows: the person
 * carries a 3-dword vector at +0x10 and +0x40 and a 2-dword pair at +0x1c and
 * +0x24 (position / scale / velocity triples). */
typedef struct Vec3i { int a, b, c; } Vec3i;
typedef struct Vec2i { int a, b; } Vec2i;

/* The 0x94-byte renderable person (blokeai.c). */
typedef struct Person3D {
    struct Person3D* prev;        /* +0x00 */
    struct Person3D* next;        /* +0x04 */
    int              kind;        /* +0x08 */
    struct Bloke*    bloke;       /* +0x0c */
    Vec3i            v10;         /* +0x10 */
    Vec2i            v1c;         /* +0x1c */
    Vec2i            v24;         /* +0x24 */
    int              f2c;         /* +0x2c */
    int              f30;         /* +0x30 */
    int              f34;         /* +0x34 */
    int              f38;         /* +0x38 */
    int              f3c;         /* +0x3c */
    Vec3i            v40;         /* +0x40 */
    int              f4c;         /* +0x4c */
    int              f50;         /* +0x50 */
    int              f54;         /* +0x54 */
    int              blk58[9];    /* +0x58..0x7b */
    int              f7c;         /* +0x7c */
    int              f80;         /* +0x80 */
    int              f84;         /* +0x84 */
    int              f88;         /* +0x88 */
    int              f8c;         /* +0x8c */
    int              f90;         /* +0x90 */
} Person3D;                       /* 0x94 */

/* A bloke; allocation stride 172 (0xac) out of the pool at g_bloke_base. */
typedef struct Bloke {
    struct Bloke*  next;          /* +0x00 */
    Person3D*      person;        /* +0x04 */
    unsigned char  b08;           /* +0x08 */
    unsigned char  pad09;         /* +0x09 */
    unsigned short w0a;           /* +0x0a */
    unsigned short w0c;           /* +0x0c */
    unsigned short w0e;           /* +0x0e */
    unsigned short w10;           /* +0x10 */
    unsigned char  pad12[2];      /* +0x12 */
    LLElem*        e14;           /* +0x14 */
    LLElem*        e18;           /* +0x18 */
    int            f1c, f20, f24, f28, f2c, f30;   /* +0x1c..0x30 */
    int            blk34[10];     /* +0x34..0x5b  (+0x54 = the BNV path) */
    int            f5c;           /* +0x5c */
    unsigned char  b60;           /* +0x60 */
    unsigned char  pad61;         /* +0x61 */
    unsigned short w62;           /* +0x62 */
    unsigned char  b64;           /* +0x64 */
    unsigned char  pad65[3];      /* +0x65 */
    int            f68;           /* +0x68 */
    int            f6c;           /* +0x6c */
    unsigned short w70;           /* +0x70 */
    unsigned char  b72, b73, b74, b75;  /* +0x72..0x75 */
    unsigned char  pad76[2];      /* +0x76 */
    unsigned short w78;           /* +0x78 */
    unsigned short w7a;           /* +0x7a */
    unsigned short w7c;           /* +0x7c */
    unsigned char  b7e, b7f, b80, b81, b82;  /* +0x7e..0x82 */
    unsigned char  pad83[5];      /* +0x83 */
    LLElem*        e88;           /* +0x88 */
    LLElem*        e8c;           /* +0x8c */
    LLElem*        e90;           /* +0x90 */
    LLElem*        e94;           /* +0x94 */
    int            blk98[5];      /* +0x98..0xab */
} Bloke;                          /* 0xac */

/* The 0x124-byte flattened bloke record staged in one global num buffer
 * (@0x007fda60) -- the exact bytes that hit the file. The two globals are one
 * object, so they are one struct. */
typedef struct BlokeSave {
    unsigned short w0c;           /* 0x7fda60 */
    unsigned short w0e;           /* 0x7fda62 */
    unsigned short w10;           /* 0x7fda64 */
    unsigned char  pad06[2];      /* 0x7fda66 */
    int            e14;           /* 0x7fda68  g_elist index or -1 */
    int            e18;           /* 0x7fda6c  g_elist index or -1 */
    int            f1c, f20, f24, f28, f2c, f30;    /* 0x7fda70..0x7fda84 */
    int            blk34[10];     /* 0x7fda88..0x7fdaaf; [8] = BNV path */
    int            f5c;           /* 0x7fdab0 */
    unsigned char  b60;           /* 0x7fdab4 */
    unsigned char  pad55;         /* 0x7fdab5 */
    unsigned short w62;           /* 0x7fdab6 */
    unsigned char  b64;           /* 0x7fdab8 */
    unsigned char  pad59;         /* 0x7fdab9 */
    unsigned short w78;           /* 0x7fdaba */
    unsigned short w7a;           /* 0x7fdabc */
    unsigned short w7c;           /* 0x7fdabe */
    unsigned char  b7e, b7f, b80, b81, b82;  /* 0x7fdac0..0x7fdac4 */
    unsigned char  pad65[3];      /* 0x7fdac5 */
    int            e88;           /* 0x7fdac8 */
    int            e8c;           /* 0x7fdacc */
    int            e90;           /* 0x7fdad0 */
    int            e94;           /* 0x7fdad4 */
    int            f68;           /* 0x7fdad8 */
    int            f6c;           /* 0x7fdadc */
    unsigned short w70;           /* 0x7fdae0 */
    unsigned char  b72, b73, b74, b75;   /* 0x7fdae2..0x7fdae5 */
    unsigned char  pad86[2];      /* 0x7fdae6 */
    int            blk98[5];      /* 0x7fdae8..0x7fdafb */
    int            p08;           /* 0x7fdafc */
    Vec3i          s10;           /* 0x7fdb00..0x7fdb08 */
    Vec2i          s1c;           /* 0x7fdb0c, 0x7fdb10 */
    Vec2i          s24;           /* 0x7fdb14, 0x7fdb18 */
    int            p34;           /* 0x7fdb1c */
    int            p30;           /* 0x7fdb20 */
    int            p38;           /* 0x7fdb24 */
    int            p3c;           /* 0x7fdb28 */
    Vec3i          s40;           /* 0x7fdb2c..0x7fdb34 */
    int            p4c;           /* 0x7fdb38 */
    int            p88;           /* 0x7fdb3c */
    int            p54;           /* 0x7fdb40 */
    int            blk58[9];      /* 0x7fdb44..0x7fdb67 */
    int            p7c;           /* 0x7fdb68 */
    int            p80;           /* 0x7fdb6c */
    int            p8c;           /* 0x7fdb70 */
    int            p90;           /* 0x7fdb74 */
    int            p84;           /* 0x7fdb78 */
    int            w0a;           /* 0x7fdb7c (widened) */
    int            b08;           /* 0x7fdb80 (widened) */
} BlokeSave;                      /* 0x124 */
extern BlokeSave g_bs;            /* 0x007fda60 */

/* An object instance record (0x14 bytes, objmap.c). */
typedef struct ObjInst {
    struct ObjInst*  next;   /* +0x00 */
    struct ObjInst*  prev;   /* +0x04 */
    void*            owner;  /* +0x08 */
    short            f0c;    /* +0x0c */
    short            f0e;    /* +0x0e */
    int              f10;    /* +0x10 */
} ObjInst;

/* A "bloke riding this object" node (0x14 bytes). */
typedef struct RiderNode {
    struct RiderNode* next;  /* +0x00 */
    struct RiderNode* prev;  /* +0x04 */
    Bloke*            bloke; /* +0x08 */
    short             f0c;   /* +0x0c */
    unsigned char     pad0e[2];
    Person3D*         person;/* +0x10 */
} RiderNode;

/* The 0xd0-byte ODF class descriptor, seen from the save path. */
typedef struct ObjDef {
    struct ObjDef*   next;        /* +0x00 */
    ObjInst*         instances;   /* +0x04 */
    int              f08;         /* +0x08 */
    unsigned char    pad0c[0x20 - 0x0c];
    short            kind;        /* +0x20 */
    unsigned char    pad22[0xb8 - 0x22];
    int            (*cb_load)(void* elem);   /* +0xb8 */
    int            (*cb_save)(void* elem);   /* +0xbc */
    unsigned char    padc0[4];
    LLElem*          elem;        /* +0xc4 */
    unsigned char*   been_on;     /* +0xc8 */
    RiderNode*       riders;      /* +0xcc */
} ObjDef;

/* A terrain/perimeter render node; the list is chained through +0x1c and only
 * the first 0x14 bytes are serialised. */
typedef struct TerrainObj {
    unsigned char        pad00[0x1c];
    struct TerrainObj*   next;    /* +0x1c */
} TerrainObj;

/* ---- other globals ------------------------------------------------------- */
extern Bloke*   g_people_head;      /* 0x0066b574 */
extern Bloke*   g_bloke_base;       /* 0x0066b57c */
extern LLElem*  g_extra_elems[];    /* 0x007fd660 */
extern int      g_extra_elem_count; /* 0x007fdb84 */
extern LLElem*  g_terrain_elem;     /* 0x00801410 */
extern LLElem*  g_terrain_elem_2;   /* 0x00801404 */
extern void*    g_terrain_bank;     /* 0x00667cac */
extern void*    g_bridge_bank;      /* 0x00667cb0 */
extern TerrainObj* g_terrain_objs;  /* 0x00667ca8 */
extern unsigned char g_mapai[];     /* 0x00832800  0x3f0 bytes */
extern int      g_scroll_x;         /* 0x00667cb4 */
extern int      g_scroll_y;         /* 0x00667cb8 */
extern int      g_editmode[];       /* 0x008119b0  0xc bytes (+ 4/8 forced) */
extern unsigned char g_btnflash[];  /* 0x007fdd00  0x24 bytes */
extern int      g_castle_placed;    /* 0x0079a8d0 */
extern int      g_num_build_objs;   /* 0x006670f8 */
extern unsigned char g_build_objs[];/* 0x006664f8  0xc00 bytes */
extern unsigned char g_ride_totals[];/* 0x007cb3e0 0x200 bytes */
extern int      g_map_loaded;       /* 0x00667d50 */
extern int      g_loading;          /* 0x00667ca0 */
extern int      g_state_813a40;     /* 0x00813a40 */
extern int      g_state_810140;     /* 0x00810140 */
extern int      g_state_667c48;     /* 0x00667c48 */
extern Elem*    g_entrance_elem;    /* 0x006661c4 */
extern int      g_entrance_x;       /* 0x004b8320 */
extern int      g_entrance_y;       /* 0x004b8324 */
extern void*    g_anim_ctx;         /* 0x0081c8c0 */
extern char     g_class_tag_init[]; /* 0x004bcb94 "xxxxxxxx" */
extern const char kFmtLine[];       /* 0x004bcb90 "%s\n" */

/* ---- other callees ------------------------------------------------------- */
extern int   CollectUsedTSFTables(void** out);   /* 0x0045aa50 */
extern void  SaveSidePanelState(void);                   /* 0x00474190 */
extern int   SaveScripts(void);                  /* 0x0046c920 */
extern int   SaveReport(void);                   /* 0x00444200 */
extern int   SaveCurrency(void);                 /* 0x00457910 */
extern int   GetBlokeNum(Bloke* b);              /* 0x00482fb0 */
extern Bloke* GetBlokePtr(int num);              /* 0x00482fe0 */
extern void  DestroyBloke(Bloke* b);             /* 0x00483010 */
extern void  SaveBlock5(void);                   /* 0x0049c140 */
extern void  SaveBlock6(void);                   /* 0x0049c630 */
extern void  SaveBlock7(void);                   /* 0x0049cb20 */
extern void  SaveBlock8(void);                   /* 0x0049cd10 */
extern int   SavePathRects(void);                /* 0x00482860 */
extern void  SaveBlock11(void);                  /* 0x00450a80 */
extern void  LoadBlock5(void);                   /* 0x0049c3c0 */
extern void  LoadBlock6(void);                   /* 0x0049c8b0 */
extern void  LoadBlock7(void);                   /* 0x0049cc10 */
extern void  LoadBlock8(void);                   /* 0x0049ce00 */
extern int   LoadPathRects(void);                /* 0x00482920 */
extern void  LoadBlock11(void);                  /* 0x00450b10 */
extern void  ClearMapCells(void);                   /* 0x00463680 */
extern void  LoadSidePanelState(void);                   /* 0x004741c0 */
extern void  SetInGameIconHandlers(void);                   /* 0x00474880 */
extern int   LoadScripts(void);                  /* 0x0046cb60 */
extern int   LoadReport(void);                   /* 0x00444260 */
extern int   LoadCurrency(void);                 /* 0x00457940 */
extern void  Add3DPersonToList(Person3D* p);     /* 0x0043f810 */
extern void* GetBlokeAnim3DFromPerson(Person3D* p); /* 0x00440800 */
extern int   MakeAnimInstance(Person3D* p, void* ctx, void* a, void* b, int v); /* 0x00442580 */
extern void  AddOvSav(void* rec);                /* 0x00462b30 */
extern void  SetBridgeDrawOffsets(const char* name); /* 0x004618d0 */
extern void  CalculateMapRenderOrder(void);      /* 0x0045a4a0 */
extern Cell* GetFirstObjectMatching(void* obj);  /* 0x0045a910 */
extern void  RestoreCurrentMenu(void);                   /* 0x00475f10 */
extern void  SetMapReady(int v);                  /* 0x00458bb0 */
#ifndef LEGOLAND_PORTABLE
extern void  RestoreScriptStepHelp(void);                   /* 0x0046b760 */
#else
extern int RestoreScriptStepHelp(void);                   /* 0x0046b760 */
#endif

/* The animation record GetBlokeAnim3DFromPerson returns. */
typedef struct BlokeAnim3D {
    void*  f00;
    struct AnimSet* f04;   /* +0x04 */
    void*  f08;            /* +0x08 */
} BlokeAnim3D;
typedef struct AnimSet {
    unsigned char pad00[0x20];
    void**        f20;     /* +0x20 */
} AnimSet;

/* ==========================================================================
 *  SaveGame
 * ========================================================================== */

/* Re-encode one tile code into "(tsf_index+1) << 8 | (tile - table base)", or
 * 0 when the tile's sprite table is not one of the n tables written out. */
static __inline void EncodeTile(unsigned short* t, void** tsf, int n)
{
    TileElem* d = g_tile_info[*t].elem;
    int       k;

    for (k = 0; k < n; k++)
        if (d == (TileElem*)tsf[k])
            break;
    if (k != n)
        *t = (unsigned short)((unsigned short)(*t - *(unsigned short*)d) | ((k + 1) << 8));
    else
        *t = 0;
}

/* FRAME LAYOUT -- what finally made this exact (1196/1196 instructions,
 * 4204/4204 bytes, index for index).  SaveGame has exactly four scalar frame
 * homes, and 29 of the 1196 instructions name one of them, so the whole match
 * hangs on getting all four right.  The frame (offsets relative to the
 * `sub esp,0x448` block, i.e. [esp+0x10] == local 0x00 once the four callee
 * saves are pushed) is:
 *
 *     0x10  block-scope pool: `elem` (BLK1), the `y` spill (BLK2),
 *           `nblokes` (BLK4), `rnum` (the BLK9 rider loop)
 *     0x14  `num`      <- FUNCTION-level, and declared FIRST
 *     0x18  `len`
 *     0x1c  `n_tsf`
 *     0x20  the Cell copy `c`        0x34  hdr[33]        0x58  tsf[256]
 *
 * The rules this pinned down (VC6 SP3 /O2), all measured here:
 *   - The frame is built as [block-scope pool][function-level locals in
 *     DECLARATION order, ascending].  So the number of slots the block pool
 *     needs decides where the function-level run starts: one block slot puts
 *     the first function-level local at 0x14, two would push it to 0x18.
 *   - Address-taken locals of DISJOINT blocks are coloured onto one pool slot
 *     even when the blocks sit at very different lexical depths (`elem` at the
 *     top of BLK1 and `rnum` four levels down inside the BLK9 object loop share
 *     0x10) -- provided no other address-taken local of an ENCLOSING block is
 *     live across them.  That proviso is the whole trap: wrapping them in an
 *     outer `{ int num; ... }` makes `num` interfere with every inner block and
 *     forces a second pool slot, and then VC6 hands the OUTER variable the
 *     LOWER home (num 0x10 / nblokes 0x14) -- the mirror of what the original
 *     wants, and unfixable from inside that shape: declaration order, names and
 *     nesting depth are all irrelevant to the tie-break (measured).
 *   - The way out was to notice the original REUSES one scratch int: the BLK1
 *     "flags of interest" word and the BLK4..BLK12 bloke number / list counts /
 *     name lengths are ONE variable at 0x14.  Making it a function-level `num`
 *     declared before `len` empties the outer block scope, leaves the pool one
 *     slot wide, and every home falls into place.
 *   - Corroborating evidence for the split that led there: the original writes
 *     the rider's bloke number through [esp+0x14] under one pending push
 *     (= home 0x10) while the surrounding instance/rider COUNTS use home 0x14,
 *     so the rider number provably is not the same variable as the counts.
 *
 * (Watch the pending-push offsets when reading the disassembly: a `lea`/`mov`
 * emitted between a `push` and the matching `add esp,N` names the home 4 or 8
 * higher -- e.g. `mov [esp+0x18],eax` right after `push`+`call GetBlokeNum` is
 * home 0x14, and `mov [esp+0x1c],ecx` after two pushes is also 0x14.) */
// FUNCTION: LEGOLAND 0x0047d8e0
int SaveGame(const char* path)
{
    int     num;
    int     len;
    int     n_tsf;
    Cell    c;
    char    hdr[33] = "00002 LEGOLAND Save Game V0.02 \x1a";
    void*   tsf[256];
    int     i;
    int     j;
    int     n;
    int     x, y;
    Bloke*  b;
    ObjDef* def;
    ObjInst* inst;
    RiderNode* rider;
    TerrainObj* tob;

    g_savefile_fd = _open(path, _O_BINARY | _O_TRUNC | _O_CREAT | _O_RDWR,
                          _S_IREAD | _S_IWRITE);
    if (g_savefile_fd == -1) {
        switch (errno) {
        case EACCES:
            DBError("can't open read-only file for writing, or file\x92s sharing mode forbids operations (%s)\n", path);
            return 0;
        case EINVAL:
            DBError("Invalid oflag or pmode argument (%s)\n", path);
            return 0;
        case EMFILE:
            DBError("No more file handles available (too many open files) (%s)\n", path);
            return 0;
        case ENOENT:
            DBError("File or path not found (%s)\n", path);
            return 0;
        default:
            DBError("Unknown error (%d) openning file %s\n", errno, path);
            return 0;
        }
    }

    g_chunk_depth = 0;
    if (!SaveGameWrite(hdr, 0x20)) {
        DBError("Header write failed");
        goto fail;
    }
    if (!BeginMeasuredBlock())
        goto fail;

    /* ---- BLK 1: the element table --------------------------------------- */
    if (!BeginMeasuredBlock()) {
        DBError("Measured block begin failed");
        goto fail;
    }
    {
    LLElem* elem;
    n = LLIDB_GetCount();
    g_elist_count = 0;
    ElemID(kPathControl)->type_flags |= 4;
    for (i = 0; i < n; i++) {
        LLIDB_GetElement(i, &elem);
        if (elem->type_flags & 4)
            g_elist_count++;
    }
    if (!SaveGameWrite(&g_elist_count, 4)) {
        DBError("Num elements failed");
        goto fail;
    }
    if (g_elist)
        HeapFree_w(g_elist);
    g_elist = (LLElem**)HeapAlloc_w(g_elist_count * 4);
    j = 0;
    for (i = 0; i < n; i++) {
        progress_tick();
        LLIDB_GetElement(i, &elem);
        if (elem->type_flags & 4) {
            len = strlen(elem->name);
            if (!SaveGameWrite(&len, 4))
                goto fail;
            if (!SaveGameWrite(elem->name, len)) {
                DBError("Element name write failed %s", elem->name);
                goto fail;
            }
            g_elist[j] = elem;
            j++;
            num = elem->type_flags & 0x3000e;
            if (!SaveGameWrite(&num, 4)) {
                DBError("Flags of interest write failed %s", elem->name);
                goto fail;
            }
        }
    }
    n_tsf = CollectUsedTSFTables(tsf);
    if (!SaveGameWrite(&n_tsf, 4)) {
        DBError("TSF Pointers write failed");
        goto fail;
    }
    for (i = 0; i < n_tsf; i++) {
        progress_tick();
        LLIDB_FindElementFromDataPtr(tsf[i], &elem, 0);
        len = strlen(elem->name);
        if (!SaveGameWrite(&len, 4)) {
            DBError("TSF element length write failed %s, %d", elem->name, len);
            goto fail;
        }
        if (!SaveGameWrite(elem->name, len)) {
            DBError("TSF Element name writer failed %s", elem->name);
            goto fail;
        }
    }
    }
    if (!EndMeasuredBlock()) {
        DBError("End measured block1 failed");
        goto fail;
    }

    /* ---- BLK 2: the map grid -------------------------------------------- */
    if (!BeginMeasuredBlock()) {
        DBError("Begin Measured block2 failed");
        goto fail;
    }
    if (!SaveGameWrite(g_map, 0x44)) {
        DBError("Host Config write failed");
        goto fail;
    }
    for (y = 0; y < (int)g_map->height; y++) {
        for (x = 0; x < (int)g_map->width; x++) {
            c = g_map_rows[y][x];
            progress_tick();
            if (c.flags & 0x8a8) {
                if (!FindeIneList(&c.obj)) {
                    DBError("Failed to locate Object instance at (%d,%d)", x, y);
                    goto fail;
                }
            } else {
                c.obj = 0;
            }
            EncodeTile(&c.tile, tsf, n_tsf);
            EncodeTile(&c.base, tsf, n_tsf);
            if (!SaveGameWrite(&c, 0x14)) {
                DBError("Failed to write mapinfo struct at (%d,%d)", x, y);
                goto fail;
            }
        }
    }
    if (!EndMeasuredBlock()) {
        DBError("EndMeasured Block2 failed");
        goto fail;
    }

    /* ---- BLK 3: world state --------------------------------------------- */
    if (!BeginMeasuredBlock()) {
        DBError("Begin Measured block3 failed");
        goto fail;
    }
    if (!SaveGameWrite(g_mapai, 0x3f0)) {
        DBError("Mapstats write failed");
        goto fail;
    }
    if (!SaveGameWrite(&g_scroll_x, 4)) {
        DBError("Scrollx (%d) Failed", g_scroll_x);
        goto fail;
    }
    if (!SaveGameWrite(&g_scroll_y, 4)) {
        DBError("Scrolly (%d) Failed", g_scroll_y);
        goto fail;
    }
    if (!SaveGameWrite(g_editmode, 0xc)) {
        DBError("EditMode Failed");
        goto fail;
    }
    SaveSidePanelState();
    if (!SaveScripts()) {
        DBError("Scripts Save Failed");
        goto fail;
    }
    if (!SaveReport()) {
        DBError("Report Save Failed");
        goto fail;
    }
    if (!SaveCurrency()) {
        DBError("Currency Save Failed");
        goto fail;
    }
    if (!SaveGameWrite(g_btnflash, 0x24)) {
        DBError("Button flash states Save Failed");
        goto fail;
    }
    progress_tick();
    if (!EndMeasuredBlock()) {
        DBError("EndMeasured Block 3");
        goto fail;
    }

    /* ---- BLK 4: the blokes ---------------------------------------------- */
    /* `num` (function level, home 0x14) doubles as the bloke number AND as
     * every list counter and name length from here to BLK12; `nblokes` and the
     * rider loop's `rnum` are block-scoped and share home 0x10 with BLK1's
     * `elem`.  See the frame-layout note above SaveGame -- this split is what
     * makes the function exact. */
    if (!BeginMeasuredBlock()) {
        DBError("Begin Measured Block 4");
        goto fail;
    }
    {
    int     nblokes;
    nblokes = 0;
    b = g_people_head;
    while (b) {
        nblokes++;
        b = b->next;
    }
    if (!SaveGameWrite(&nblokes, 4)) {
        DBError("NumBlokes (%d) save Failed", nblokes);
        goto fail;
    }
    }
    for (b = g_people_head; b; b = b->next) {
        progress_tick();
        num = GetBlokeNum(b);
        if (!SaveGameWrite(&num, 4)) {
            DBError("Bloke Num (d) Save Failed", num);
            goto fail;
        }
        g_bs.w0c = b->w0c;
        g_bs.w0e = b->w0e;
        g_bs.w10 = b->w10;
        if (b->e14) {
            g_bs.e14 = (int)b->e14;
            FindeIneList(&g_bs.e14);
        } else {
            g_bs.e14 = -1;
        }
        if (b->e18) {
            g_bs.e18 = (int)b->e18;
            FindeIneList(&g_bs.e18);
        } else {
            g_bs.e18 = -1;
        }
        g_bs.f1c = b->f1c;
        g_bs.f20 = b->f20;
        g_bs.f24 = b->f24;
        g_bs.f28 = b->f28;
        g_bs.f2c = b->f2c;
        g_bs.f30 = b->f30;
        memcpy(g_bs.blk34, b->blk34, sizeof(g_bs.blk34));
        g_bs.f5c = b->f5c;
        g_bs.b60 = b->b60;
        g_bs.w62 = b->w62;
        g_bs.b64 = b->b64;
        g_bs.w78 = b->w78;
        g_bs.w7a = b->w7a;
        g_bs.w7c = b->w7c;
        g_bs.b7e = b->b7e;
        g_bs.b7f = b->b7f;
        g_bs.b80 = b->b80;
        g_bs.b81 = b->b81;
        g_bs.b82 = b->b82;
        g_bs.e88 = (int)b->e88;
        FindeIneList(&g_bs.e88);
        g_bs.e8c = (int)b->e8c;
        FindeIneList(&g_bs.e8c);
        g_bs.e90 = (int)b->e90;
        FindeIneList(&g_bs.e90);
        g_bs.e94 = (int)b->e94;
        FindeIneList(&g_bs.e94);
        g_bs.f68 = b->f68;
        g_bs.f6c = b->f6c;
        g_bs.w70 = b->w70;
        g_bs.b72 = b->b72;
        g_bs.b73 = b->b73;
        g_bs.b74 = b->b74;
        g_bs.b75 = b->b75;
        memcpy(g_bs.blk98, b->blk98, sizeof(g_bs.blk98));
        g_bs.p08 = b->person->kind;
        g_bs.s10 = b->person->v10;
        g_bs.s1c = b->person->v1c;
        g_bs.s24 = b->person->v24;
        g_bs.p34 = b->person->f34;
        g_bs.p38 = b->person->f38;
        g_bs.p3c = b->person->f3c;
        g_bs.s40 = b->person->v40;
        g_bs.p4c = b->person->f4c;
        g_bs.p88 = b->person->f88;
        g_bs.p54 = b->person->f54;
        /* [sic] +0x38 is staged a second time by the original. */
        g_bs.p38 = b->person->f38;
        memcpy(g_bs.blk58, b->person->blk58, sizeof(g_bs.blk58));
        g_bs.p7c = b->person->f7c;
        g_bs.p80 = b->person->f80;
        g_bs.p8c = b->person->f8c;
        g_bs.p90 = b->person->f90;
        g_bs.p84 = b->person->f84;
        g_bs.w0a = b->w0a;
        g_bs.b08 = b->b08;
        g_bs.p30 = b->person->f30;
        if (!SaveGameWrite(&g_bs, 0x124)) {
            DBError("Bloke data failed (%d)", num);
            goto fail;
        }
        if (g_bs.blk34[8]) {
            if (!SaveGameWrite((void*)g_bs.blk34[8], 0x48)) {
                DBError("Bloke BNV path data (%d)", num);
                goto fail;
            }
        }
    }
    if (!EndMeasuredBlock()) {
        DBError("EndMeasured Block 4");
        goto fail;
    }

    /* ---- BLK 5..8 -------------------------------------------------------- */
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 5"); goto fail; }
    SaveBlock5();
    if (!EndMeasuredBlock()) { DBError("EndMeasuredBlock5"); goto fail; }
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 6"); goto fail; }
    progress_tick();
    SaveBlock6();
    if (!EndMeasuredBlock()) { DBError("End Measured Block 6"); goto fail; }
    if (!BeginMeasuredBlock()) { DBError("Begin measured block 7"); goto fail; }
    SaveBlock7();
    if (!EndMeasuredBlock()) { DBError("End Measured Block 7"); goto fail; }
    progress_tick();
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 8"); goto fail; }
    SaveBlock8();
    if (!EndMeasuredBlock()) { DBError("End Measured Block 8"); goto fail; }

    /* ---- BLK 9: objects, instances, rides -------------------------------- */
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 9"); goto fail; }
    if (!SaveGameWrite(&g_castle_placed, 4)) {
        DBError("Castle Placed Flag %s", g_castle_placed ? "TRUE" : "FALSE");
        goto fail;
    }
    if (!SaveGameWrite(&g_num_build_objs, 4)) {
        DBError("Num Build Objs (%d) Save Failed", g_num_build_objs);
        goto fail;
    }
    if (!SaveGameWrite(g_build_objs, 0xc00)) {
        DBError("BuildObjList (size %dS) Save Failed", g_num_build_objs);
        goto fail;
    }
    for (i = 0; i < g_elist_count; i++) {
        LLElem* oe;

        progress_tick();
        oe = g_elist[i];
        if (oe->type_flags & 0x10) {
            def = (ObjDef*)oe->data;
            SaveGameWrite(def->elem->name, 8);
            if (def->kind != 2 && def->kind != 0) {
                if (!SaveGameWrite(def->been_on, g_mapx->max_blokes)) {
                    DBError("BeenOn Flags for %s Save Failed", def->elem->name);
                    goto fail;
                }
            }
            if (!SaveGameWrite(&def->f08, 4)) {
                DBError("Count for Object %s", def->elem->name);
                goto fail;
            }
            inst = def->instances;
            num = 0;
            while (inst) {
                num++;
                inst = inst->next;
            }
            if (!SaveGameWrite(&num, 4)) {
                DBError("Instance Count for %s", def->elem->name);
                goto fail;
            }
            for (inst = def->instances; inst; inst = inst->next) {
                if (!SaveGameWrite(&inst->f0c, 4)) {
                    DBError("Flags for instance of %s", def->elem->name);
                    goto fail;
                }
                if (!SaveGameWrite(&inst->f0e, 4)) {
                    DBError("Objuid for instance of %s", def->elem->name);
                    goto fail;
                }
                if (!SaveGameWrite(&inst->f10, 4)) {
                    DBError("TickCount for instance of %s", def->elem->name);
                    goto fail;
                }
            }
            rider = def->riders;
            num = 0;
            while (rider) {
                num++;
                rider = rider->next;
            }
            if (!SaveGameWrite(&num, 4)) {
                DBError("Num Blokes On Ride for object %s", def->elem->name);
                goto fail;
            }
            for (rider = def->riders; rider; rider = rider->next) {
                int rnum;
                rnum = GetBlokeNum(rider->bloke);
                if (!SaveGameWrite(&rnum, 4)) {
                    DBError("Bloke Num for bloke on %s", def->elem->name);
                    goto fail;
                }
                if (!SaveGameWrite(&rider->f0c, 2)) {
                    DBError("Ride ID for bloke on %s", def->elem->name);
                    goto fail;
                }
            }
            if (def->cb_save) {
                if (!def->cb_save(def->elem)) {
                    DBError("Ride specific save data for %s", def->elem->name);
                    goto fail;
                }
            }
        }
    }
    if (!SaveGameWrite(g_ride_totals, 0x200)) {
        DBError("RideTotal");
        goto fail;
    }
    if (!EndMeasuredBlock()) { DBError("End Measured VBlock 9"); goto fail; }
    progress_tick();

    /* ---- BLK 10, 11 ------------------------------------------------------ */
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 10"); goto fail; }
    if (!SavePathRects()) { DBError("Path Rects"); goto fail; }
    if (!EndMeasuredBlock()) { DBError("End Measured Block 10"); goto fail; }
    if (!BeginMeasuredBlock()) { DBError("Begin Measured VBlock 11"); goto fail; }
    SaveBlock11();
    if (!EndMeasuredBlock()) { DBError("End Measured Block 11"); goto fail; }
    progress_tick();

    /* ---- BLK 12: terrain ------------------------------------------------- */
    if (!BeginMeasuredBlock()) { DBError("Begin Measured Block 12"); goto fail; }
    num = strlen(g_terrain_elem->name);
    SaveGameWrite(&num, 4);
    SaveGameWrite(g_terrain_elem->name, num);
    if (g_terrain_elem_2)
        num = strlen(g_terrain_elem_2->name);
    else
        num = 0;
    SaveGameWrite(&num, 4);
    if (num)
        SaveGameWrite(g_terrain_elem_2->name, num);
    num = 0;
    tob = g_terrain_objs;
    while (tob) {
        num++;
        tob = tob->next;
    }
    SaveGameWrite(&num, 4);
    for (tob = g_terrain_objs; tob; tob = tob->next)
        SaveGameWrite(tob, 0x14);
    if (!EndMeasuredBlock()) { DBError("End Measured Block 12"); goto fail; }
    progress_tick();
    if (!EndMeasuredBlock()) {
        DBError("End Measured Block 13");
        goto fail;
    }
    _close(g_savefile_fd);
    return 1;

fail:
    _close(g_savefile_fd);
    return 0;
}

/* ==========================================================================
 *  LoadGame -- the exact mirror of SaveGame
 * ========================================================================== */

/* Undo EncodeTile: (index+1)<<8 | offset  ->  tsf_table->base_slot + offset.
 * A code of 0 means "no tile" and is left alone. */
static __inline void DecodeTile(unsigned short* t, void** tsf)
{
    unsigned short v = *t;

    if (v)
        *t = (unsigned short)(*(unsigned short*)tsf[(v >> 8) - 1] + (v & 0xff));
}

/* Frame-layout lever (this is what took LoadGame from 94.7% to exact, and it
 * generalises): VC6 overlays the locals of LEXICALLY SIBLING scopes, matching
 * them up by nesting DEPTH. The original has only three scalar frame homes --
 * 0x10 shared by the BLK1 element pointer and every later list counter, 0x14
 * the BLK1 name length, 0x18 shared by the BLK1 element flags and the bloke
 * number. Two address-taken locals only ever share a home when they sit in
 * LEXICALLY SIBLING scopes, so the two pairs are written as two scope nests:
 * {int flags { LLElem* elem ...BLK1... }} and {int num { int count
 * ...BLK4..BLK12... }}. Coextensive nests collapse into one scope, and inside a
 * scope VC6 gives the LOWER home to the longer live range -- which is why
 * `elem` beats `flags` and `count` beats `num` here without further coaxing.
 * Declaration order, names and nesting depth are all irrelevant.
 * REFINED while matching SaveGame (see the frame-layout note there): sibling
 * scopes are not required to be at the same DEPTH -- disjoint blocks share a
 * pool slot however deeply nested they are -- and the pool of shared slots sits
 * BELOW the function-level locals, which are laid out in declaration order.
 * LoadGame happens to need three slots either way, so its shape stands; the
 * same shape was wrong for SaveGame, where one slot plus a function-level
 * scratch int is what the original has. */
// FUNCTION: LEGOLAND 0x0047e980
int LoadGame(const char* path)
{
    int        len;
    char       ov[0x14];
    char       hdr[0x20];
    char       name[0x200];
    void*      tsf[256];
    int        i;
    int        x, y;
    Cell*      cell;
    Bloke*     b;
    ObjDef*    def;
    ObjInst*   inst;
    ObjInst*   iprev;
    RiderNode* rider;
    RiderNode* rprev;

    g_savefile_fd = _open(path, _O_BINARY);
    if (g_savefile_fd == -1)
        return 0;

    g_chunk_depth = 0;
    g_loading = 1;
    if (!SaveGameRead(hdr, 0x20))
        goto fail;
    hdr[5] = 0;
    /* [sic] the version bail-out leaks the descriptor -- no _close here. */
    if (atoi(hdr) != 2)
        return 0;

    if (!SkipMeasuredBlock())          /* the outer block's end offset */
        goto fail;
    {
    int flags;
    {
    LLElem* elem;

    /* ---- BLK 1: the element table --------------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    if (!SaveGameRead(&g_elist_count, 4))
        goto fail;
    if (g_elist) {
        HeapFree_w(g_elist);
        g_elist = 0;
    }
    g_elist = (LLElem**)HeapAlloc_w(g_elist_count * 4);
    for (i = 0; i < g_elist_count; i++) {
        progress_tick();
        if (!SaveGameRead(&len, 4))
            goto fail;
        if (!SaveGameRead(name, len))
            goto fail;
        name[len] = 0;
        if (LLIDB_FindElement(name, &elem, 0))
            goto fail;
        g_elist[i] = elem;
        if (!SaveGameRead(&flags, 4))
            goto fail;
        LLIDB_LoadData(elem);
        elem->type_flags &= ~0x3000e;
        elem->type_flags |= flags;
    }
    if (!SaveGameRead(&g_extra_elem_count, 4))
        goto fail;
    for (i = 0; i < g_extra_elem_count; i++) {
        progress_tick();
        if (!SaveGameRead(&len, 4))
            goto fail;
        if (!SaveGameRead(name, len))
            goto fail;
        name[len] = 0;
        if (LLIDB_FindElement(name, &elem, 0))
            goto fail;
        g_extra_elems[i] = elem;
        LLIDB_LoadData(elem);
        tsf[i] = elem->data;
    }

    }
    }
    /* ---- BLK 2: the map grid -------------------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    if (!SaveGameRead(g_map, 0x44))
        goto fail;
    ClearMapCells();
    for (y = 0; y < (int)g_map->height; y++) {
        progress_tick();
        for (x = 0; x < (int)g_map->width; x++) {
            cell = &g_map_rows[y][x];
            if (!SaveGameRead(cell, 0x14))
                goto fail;
            if (cell->flags & 0x8a8)
                cell->obj = g_elist[(int)cell->obj];
            else
                cell->obj = 0;
            DecodeTile(&cell->tile, tsf);
            DecodeTile(&cell->base, tsf);
        }
    }

    /* ---- BLK 3: world state --------------------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    if (!SaveGameRead(g_mapai, 0x3f0))
        goto fail;
    if (!SaveGameRead(&g_scroll_x, 4))
        goto fail;
    if (!SaveGameRead(&g_scroll_y, 4))
        goto fail;
    if (!SaveGameRead(g_editmode, 0xc))
        goto fail;
    g_editmode[1] = 3;
    g_editmode[2] = 0;
    LoadSidePanelState();
    SetInGameIconHandlers();
    progress_tick();
    if (!LoadScripts())
        goto fail;
    if (!LoadReport())
        goto fail;
    if (!LoadCurrency())
        goto fail;
    if (!SaveGameRead(g_btnflash, 0x24))
        goto fail;
    progress_tick();
    g_map_loaded = 1;

    /* ---- BLK 4: the blokes ---------------------------------------------- */
    {
    int num;
    {
    int count;
    if (!SkipMeasuredBlock())
        goto fail;
    while (g_people_head)
        DestroyBloke(g_people_head);
    count = 0;
    progress_tick();
    if (!SaveGameRead(&count, 4))
        goto fail;
    while (count-- != 0) {
        if (!SaveGameRead(&num, 4))
            goto fail;
        b = (Bloke*)((char*)g_bloke_base + num * 172);
        b->next = g_people_head;
        g_people_head = b;
        if (!SaveGameRead(&g_bs, 0x124))
            goto fail;
        if (g_bs.blk34[8]) {
            g_bs.blk34[8] = (int)HeapAlloc_w(0x48);
            if (!SaveGameRead((void*)g_bs.blk34[8], 0x48))
                goto fail;
        }
        b->w0c = g_bs.w0c;
        b->w0e = g_bs.w0e;
        b->w10 = g_bs.w10;
        if (g_bs.e14 != -1)
            b->e14 = g_elist[g_bs.e14];
        else
            b->e14 = 0;
        if (g_bs.e18 != -1)
            b->e18 = g_elist[g_bs.e18];
        else
            b->e18 = 0;
        b->f1c = g_bs.f1c;
        b->f20 = g_bs.f20;
        b->f24 = g_bs.f24;
        b->f28 = g_bs.f28;
        b->f2c = g_bs.f2c;
        b->f30 = g_bs.f30;
        memcpy(b->blk34, g_bs.blk34, sizeof(b->blk34));
        b->f5c = g_bs.f5c;
        b->b60 = g_bs.b60;
        b->w62 = g_bs.w62;
        b->b64 = g_bs.b64;
        b->w78 = g_bs.w78;
        b->w7a = g_bs.w7a;
        b->w7c = g_bs.w7c;
        b->b7e = g_bs.b7e;
        b->b7f = g_bs.b7f;
        b->b80 = g_bs.b80;
        b->b81 = g_bs.b81;
        b->b82 = g_bs.b82;
        if (g_bs.e88 < g_elist_count)
            b->e88 = g_elist[g_bs.e88];
        else
            b->e88 = 0;
        if (g_bs.e8c < g_elist_count)
            b->e8c = g_elist[g_bs.e8c];
        else
            b->e8c = 0;
        if (g_bs.e90 < g_elist_count)
            b->e90 = g_elist[g_bs.e90];
        else
            b->e90 = 0;
        if (g_bs.e94 < g_elist_count)
            b->e94 = g_elist[g_bs.e94];
        else
            b->e94 = 0;
        b->f68 = g_bs.f68;
        b->f6c = g_bs.f6c;
        b->w70 = g_bs.w70;
        b->b72 = g_bs.b72;
        b->b73 = g_bs.b73;
        b->b74 = g_bs.b74;
        b->b75 = g_bs.b75;
        memcpy(b->blk98, g_bs.blk98, sizeof(b->blk98));
        b->person = (Person3D*)HeapAlloc_w(0x94);
        Add3DPersonToList(b->person);
        b->person->bloke = b;
        b->person->kind = g_bs.p08;
        b->person->v10 = g_bs.s10;
        b->person->v1c = g_bs.s1c;
        b->person->v24 = g_bs.s24;
        b->person->f34 = g_bs.p34;
        b->person->f38 = g_bs.p38;
        b->person->f3c = g_bs.p3c;
        b->person->v40 = g_bs.s40;
        b->person->f4c = g_bs.p4c;
        b->person->f88 = g_bs.p88;
        b->person->f54 = g_bs.p54;
        /* [sic] +0x38 restored a second time, mirroring the writer. */
        b->person->f38 = g_bs.p38;
        memcpy(b->person->blk58, g_bs.blk58, sizeof(b->person->blk58));
        b->person->f7c = g_bs.p7c;
        b->person->f80 = g_bs.p80;
        b->person->f8c = g_bs.p8c;
        b->person->f90 = g_bs.p90;
        b->person->f84 = g_bs.p84;
        b->w0a = (unsigned short)g_bs.w0a;
        b->b08 = (unsigned char)g_bs.b08;
        b->person->f2c = 0;
        b->person->f30 = g_bs.p30;
        {
            BlokeAnim3D* an = (BlokeAnim3D*)GetBlokeAnim3DFromPerson(b->person);
            b->person->f50 = MakeAnimInstance(b->person, g_anim_ctx, an->f08,
                                        an->f04->f20[0], b->person->f84);
        }
    }

    /* ---- BLK 5..8 -------------------------------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    LoadBlock5();
    if (!SkipMeasuredBlock())
        goto fail;
    LoadBlock6();
    progress_tick();
    if (!SkipMeasuredBlock())
        goto fail;
    LoadBlock7();
    if (!SkipMeasuredBlock())
        goto fail;
    LoadBlock8();
    progress_tick();

    /* ---- BLK 9: objects, instances, rides -------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    if (!SaveGameRead(&g_castle_placed, 4))
        goto fail;
    if (!SaveGameRead(&g_num_build_objs, 4))
        goto fail;
    if (!SaveGameRead(g_build_objs, 0xc00))
        goto fail;
    for (i = 0; i < g_elist_count; i++) {
        progress_tick();
        if (g_elist[i]->type_flags & 0x10) {
            g_elist[i]->type_flags |= 4;
            def = (ObjDef*)g_elist[i]->data;
            {
                char tagbuf[9] = "xxxxxxxx";

                if (!SaveGameRead(tagbuf, 8))
                    goto fail;
                DBPrintf(kFmtLine, tagbuf);
            }
            if (def->kind != 2 && def->kind != 0) {
                def->been_on = (unsigned char*)HeapAlloc_w(g_mapx->max_blokes);
                if (!SaveGameRead(def->been_on, g_mapx->max_blokes))
                    goto fail;
            }
            if (!SaveGameRead(&def->f08, 4))
                goto fail;
            iprev = 0;
            count = 0;
            if (!SaveGameRead(&count, 4))
                goto fail;
            while (count-- != 0) {
                inst = (ObjInst*)HeapAlloc_w(0x14);
                inst->next = 0;
                inst->owner = def;
                if (iprev == 0) {
                    def->instances = inst;
                    inst->prev = iprev;
                } else {
                    iprev->next = inst;
                    inst->prev = iprev;
                }
                iprev = inst;
                if (!SaveGameRead(&inst->f0c, 4))
                    goto fail;
                if (!SaveGameRead(&inst->f0e, 4))
                    goto fail;
                if (!SaveGameRead(&inst->f10, 4))
                    goto fail;
            }
            rprev = 0;
            count = 0;
            if (!SaveGameRead(&count, 4))
                goto fail;
            while (count-- != 0) {
                rider = (RiderNode*)HeapAlloc_w(0x14);
                rider->next = 0;
                if (rprev == 0) {
                    def->riders = rider;
                    rider->prev = rprev;
                } else {
                    rprev->next = rider;
                    rider->prev = rprev;
                }
                rprev = rider;
                if (!SaveGameRead(&num, 4))
                    goto fail;
                if (!SaveGameRead(&rider->f0c, 2))
                    goto fail;
                rider->bloke = GetBlokePtr(num);
                rider->person = rider->bloke->person;
            }
            if (def->cb_load) {
                if (!def->cb_load(def->elem))
                    goto fail;
            }
        }
    }
    if (!SaveGameRead(g_ride_totals, 0x200))
        goto fail;
    progress_tick();

    /* ---- BLK 10, 11 ------------------------------------------------------ */
    if (!SkipMeasuredBlock())
        goto fail;
    if (!LoadPathRects())
        goto fail;
    if (!SkipMeasuredBlock())
        goto fail;
    LoadBlock11();
    progress_tick();

    /* ---- BLK 12: terrain ------------------------------------------------- */
    if (!SkipMeasuredBlock())
        goto fail;
    {
        /* The terrain names are read back over the (now dead) TSF descriptor
         * table -- the original reuses that exact frame address. */
        char* tname = (char*)tsf;

        SaveGameRead(&count, 4);
        SaveGameRead(tname, count);
        tname[count] = 0;
        LLIDB_FindElement(tname, &g_terrain_elem, 0);
        g_terrain_bank = LLIDB_LoadData(g_terrain_elem);
        SaveGameRead(&count, 4);
        if (count != 0) {
            SaveGameRead(tname, count);
            tname[count] = 0;
            LLIDB_FindElement(tname, &g_terrain_elem_2, 0);
            g_bridge_bank = LLIDB_LoadData(g_terrain_elem_2);
            SetBridgeDrawOffsets(tname);
        } else {
            g_terrain_elem_2 = 0;
            g_bridge_bank = 0;
        }
        SaveGameRead(&count, 4);
        for (i = 0; i < count; i++) {
            SaveGameRead(ov, 0x14);
            AddOvSav(ov);
        }
    }

    }
    }
    _close(g_savefile_fd);
    progress_tick();
    g_loading = 0;
    g_state_813a40 |= 0x20;
    CalculateMapRenderOrder();
    g_entrance_elem = (Elem*)ElemID(kEntrance1);
    {
        ElemData* d = g_entrance_elem->data;
        Cell*     e = GetFirstObjectMatching(g_entrance_elem);

        if (e) {
            g_entrance_x = ((e->bx + d->origin) << 8) - 0x100;
            {
                int base = d->base;
                g_entrance_y = (((d->span - base) << 7) & ~0xFF)
                             + ((e->by + base) << 8);
            }
        }
    }
    RestoreCurrentMenu();
    SetMapReady(1);
    g_state_810140 = 1;
    g_state_667c48 = 1;
    g_editmode[0] = 0;
    g_state_813a40 &= ~0x1000;
    RestoreScriptStepHelp();
    return 1;

fail:
    _close(g_savefile_fd);
    g_loading = 0;
    return 0;
}
