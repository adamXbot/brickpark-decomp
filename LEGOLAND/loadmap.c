/* LEGOLAND — LoadBaseMap (base-map loader / terrain RLE decoder).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Reads a map
 * resource: header + tsm_mapping/terrain elements, dimensions, perimeter/env
 * object records, then a chain of RLE-encoded layer streams (base tile, map
 * flags, RF flags, user flags) applied cell-by-cell across the grid. Only the
 * struct field offsets and callee arg counts are load-bearing. */
#include "legoland.h"

/* ---- globals touched (those not already declared in legoland.h) --------- */
extern int   g_map_loaded;          /* 0x00667d50 */
extern void* g_map_elem;            /* 0x008003f4 */
extern int   g_build_in_progress;   /* 0x00667ca0 */
extern void* g_tsm_mapping_elem;    /* 0x0080140c */
extern void* g_terrain_elem;        /* 0x00801410 */
extern void* g_terrain_elem_data;   /* 0x00667cac */
extern void* g_env_class;           /* 0x007fd624 */
extern unsigned int g_perim_count;  /* 0x00801b28 (extra/perimeter record count) */
extern void** g_array_A;            /* 0x00801a68 */
extern void** g_array_B;            /* 0x00801a70 */
extern unsigned int g_perim_aux;    /* 0x00801a74 */
extern void* g_terrain_elem_2;      /* 0x00801404 */
extern void* g_terrain_texdata;     /* 0x00667cb0 */
extern int   g_state_810140;        /* 0x00810140 */
extern void** g_path_tile_ptr;      /* 0x00832bf0 */
/* g_default_tile (0x00667ca4) is declared in legoland.h; used here as the map
 * header word written into cell.base for object cells. */

extern const char FMT_map_filename[]; /* 0x004b9c30 printf-style map filename fmt */
extern const char kEnvClassName[];    /* 0x004b8a70 */
extern const char g_terrain_magic[];  /* 0x004b9c24 8-byte magic tag */

/* ---- callees (arg counts confirmed at call sites) ----------------------- */
extern int   LLIDB_FindElement(const char* name, void** out, unsigned int* outidx); /* 0x47b330 */
extern void* LLIDB_LoadData(void* elem);        /* 0x47d3a0 */
extern void* ElemID(const char* name);          /* 0x47b3f0 */
extern void* RES_OpenFile(const char* name);    /* 0x489b60 */
extern int   RES_ReadFile(void* file, void* buf, int len); /* 0x489cf0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_CloseFile(void* file);         /* 0x489de0 */
#else
extern int RES_CloseFile(void* file);         /* 0x489de0 */
#endif
extern int   RES_GetFilePointer(void* file);    /* 0x489db0 */
#ifndef LEGOLAND_PORTABLE
extern void  RES_SetFilePointer(void* file, int pos); /* 0x489d70 */
#else
extern int RES_SetFilePointer(void* file, int pos); /* 0x489d70 */
#endif
extern void  ResetBuildStats(void);             /* 0x459880 */
extern void  PutObjOnMap(void* cls, void* obj, Pos* pos); /* 0x459ad0 */
extern void  SetMapTile(int x, int y, unsigned short tile);  /* 0x461780 */
extern void  SetMapFlags(int x, int y, unsigned short flags);/* 0x461810 */
extern unsigned short GetMapFlags(int x, int y);/* 0x4617d0 */
extern void  Set_RFFlags(int x, int y, unsigned char value); /* 0x4616e0 */
extern void  Set_UserFlags(int x, int y, unsigned short value);/* 0x461730 —
   3rd param is 16-bit: the original pushes ecx straight after
   "movzx cx, byte ptr [...]", which only happens for a WORD formal.
   Confirmed against the matched definition in sweep2.c. */
extern void  Format(char* dest, const char* fmt, ...);       /* 0x49e573 */
extern void* HeapAlloc_w(unsigned int size);    /* 0x49e4ff */
extern void  HeapFree_w(void* p);               /* 0x49e4d0 */
extern void  progress_tick(void);               /* 0x4663f0 */
extern void  build_perimeter(void* rec);        /* 0x462c00 */
extern void  RenderInit(void);                  /* 0x462c60 */
extern void  AddPathTileGFX(Pos* p, unsigned short w); /* 0x45d350 */
extern void  AddPathSquare(Pos* p);             /* 0x481c50 */
extern void  map_helper_4618d0(char* name);     /* 0x4618d0 */
extern int   memcmp(const void* a, const void* b, unsigned int n);


static __inline void s8_gfx(int gx, int gy) { Pos p; p.x = gx; p.y = gy; AddPathTileGFX(&p, *(unsigned short*)g_path_tile_ptr); }
static __inline void s8_sq(int gx, int gy) { Pos p; p.x = gx; p.y = gy; AddPathSquare(&p); }


static __inline void s9_perims(void* f, char* rec) {
    int n, k;
    RES_ReadFile(f, &n, 4);
    for (k = 0; k < n; k++) { RES_ReadFile(f, rec, 0x14); build_perimeter(rec); }
}
static __inline void s9_texname(void* f, char* dst) {
    int n;
    if (RES_ReadFile(f, &n, 4) == 4) {
        RES_ReadFile(f, dst, n);
        dst[n] = 0;
        map_helper_4618d0(dst);
        g_terrain_elem_2 = ElemID(dst);
        g_terrain_texdata = LLIDB_LoadData(g_terrain_elem_2);
    }
}

static __inline void s9b_s10(void* f, char* nameb, void* tsm, int* plen, int state)
{
    unsigned int w = 0;
    int saved;
    int x, y;
    saved = RES_GetFilePointer(f);
    {
        extern void* memset(void* d, int c, unsigned int n);
        nameb[0] = 0;
        memset(nameb + 1, 0, 0xc8 - 1);
    }
    RES_ReadFile(f, nameb, 8);
    if (memcmp(nameb, g_terrain_magic, 8) == 0) {
        s9_texname(f, nameb);
    } else {
        RES_SetFilePointer(f, saved);
    }
    for (y = 0; y < (int)g_map->height; y++) {
        progress_tick();
        for (x = 0; x < (int)g_map->width; x++) {
            for (;;) {
                switch (state) {
                case 0:
                    if (RES_ReadFile(f, plen, 1) != 1)
                        return;
                    state = (*plen != 0) ? 2 : 1;
                    continue;
                case 1:
                    {
                        unsigned int v;
                        RES_ReadFile(f, &w, 2);
                        v = w;
                        if (v == 0xffff) {
                            state = 0;
                        } else {
                            unsigned int low = v & 0xff;
                            unsigned int idx = ((v - 0x100) >> 8) & 0xff;
                            unsigned short* p =
                                *(unsigned short**)((char*)tsm + idx * 8 + 4);
                            *(unsigned short*)((char*)g_map_rows[y] + x * 0x14 + 0xa) =
                                (unsigned short)(*p + low);
                        }
                    }
                    break;
                case 2:
                    if (*plen != 0) {
                        (*plen)--;
                        break;
                    }
                    state = 1;
                    continue;
                }
                break;
            }
        }
    }
}

// FUNCTION: LEGOLAND 0x00461a50
int LoadBaseMap(char* mapName)
{
    void*  L_1c;               /* LLElem* out of LLIDB_FindElement */
    void*  L_20;               /* open resource file handle */
    /* LOAD-BEARING AGGREGATE (worth 36 matched instructions, and it removes the
     * dead `L_14 = runlen` mirror store the S7 fill arm used to need).
     * LB.a/LB.b/LB.c are the former L_10 / L_14 / L_18:
     *   LB.a  x / column counter                          -> [esp+0x10]
     *   LB.b  chunk-size read scratch (S5/S7/S8/S9/S10)   -> [esp+0x14]
     *   LB.c  name-length + S8 map-flag temp              -> [esp+0x18]
     * As three separate locals VC6 SP3 orders them L_10, L_18, L_14 (0x10,
     * 0x14, 0x18) -- i.e. LB.b and LB.c come out swapped versus the original,
     * and every [esp+N] from S2 onward shifts.  The ordering key is NOT
     * declaration order, type, name, first-def position, or reference count
     * (all measured, all inert); the only thing that reordered the pair was
     * giving the loser one extra *surviving* store, which costs an instruction.
     * Putting the three in one aggregate takes them out of that ordering
     * entirely: C fixes the member offsets, so the layout is exact for free.
     * This is a pure regrouping -- every use site is the identical expression
     * with a different spelling; no type, order of operations, or control flow
     * changes. */
    struct { int a; int b; unsigned int c; } LB;
    int    L_2c;               /* run / loop counter */
    Pos    pos34;              /* PutObjOnMap Pos -- lands on [esp+0x34],
                                * matching the original (S4/S5/S6). */
    void* F_tsm; int F_fillflag; int F_curval;
    /* FRAME LAYOUT (measured; supersedes the earlier "unreachable" note).
     *
     * The original's 0x24..0x57 band is SIX objects:
     *   0x24 (8)  AddPathTileGFX Pos, S8 copy-branch   + buf's ebp spill home
     *             (S5/S6) + the S7 runlen spill + the S9 perimeter-count and
     *             terrain-name-length read targets
     *   0x2c (8)  AddPathSquare Pos, S8 copy-branch    + the S5/S6 trip counter
     *             + the S10 16-bit word (cleared once at 0x00462729)
     *   0x34 (8)  PutObjOnMap Pos (S4 + both S5/S6 arms)
     *   0x3c (4)  tsm descriptor-table base (spill home)
     *   0x40 (8)  AddPathSquare Pos, S8 fill-branch    + F_fillflag
     *   0x48 (8)  AddPathTileGFX Pos, S8 fill-branch   + F_curval
     *   0x50/0x54 (4+4) the S4 env-object index/count
     *
     * The overlays are REACHABLE, and the lever is inline expansion: VC6 SP3
     * packs *inline-expansion temporaries* into the same lifetime-coloured pool
     * as register spill homes, so an address-taken temp born inside a
     * `static __inline` helper lands on a slot that is dead at that point.  A
     * named address-taken local never joins that pool and always gets a slot of
     * its own -- which is why the shared `pos_gfx`/`u24` locals used before
     * could only ever hit 2 of the 4 S8 Pos slots.  Hence:
     *   - s8_gfx / s8_sq          -> four distinct Pos temps (0x24/0x2c/0x40/0x48)
     *   - s9_perims / s9_texname  -> the two S9 read targets fold onto 0x24
     *   - s9b_s10                 -> `w` is born before the terrain-name block,
     *                                so it conflicts with the 0x24 group and is
     *                                forced onto 0x2c, exactly as the original.
     * F_fillflag / F_curval must be plain scalars, NOT members of one struct:
     * as struct members they are a named local and the two fill-branch Pos
     * temps cannot fold onto 0x40/0x48.
     *
     * SOLVED (was: "pos34 and the tsm spill home come out swapped").  See the
     * FRAME-SLOT LEVER note at the head of S2 -- this is the ONE remaining
     * extra instruction in the function.
     */
    /* LOAD-BEARING AGGREGATE (worth 6 matched instructions).  LR.a/LR.b are
     * the former L_50 / L_54, the S4 env-object record index and count:
     *   LR.a -> [esp+0x50]   LR.b -> [esp+0x54]
     * As two separate locals VC6 emits them swapped (L_54@0x50, L_50@0x54)
     * once LB above is an aggregate.  Same reasoning as LB: one aggregate
     * fixes the member offsets in C instead of relying on VC6's ordering.
     * Pure regrouping; no behavioural change. */
    struct { int a, b; } LR;
    /* LOAD-BEARING AGGREGATE (worth the LAST extra instruction in the
     * function).  BA.a/BA.b/BA.c/BA.d are the former buf_58 / buf_6c /
     * buf_134 / buf_334, in that order and at those exact sizes:
     *   BA.a  [esp+0x58] 0x14   20-byte perimeter record buffer
     *   BA.b  [esp+0x6c] 0xc8   magic tag / texture name scratch
     *   BA.c  [esp+0x134] 0x200 map filename / element-name buffer
     *   BA.d  [esp+0x334] 0x200 element-name buffer (object loops)
     * All four are char arrays, so the struct's alignment is 1 and no padding
     * is inserted: the members land on exactly the offsets the four separate
     * arrays already occupied.  This is a pure regrouping -- every use site is
     * the identical expression with a different spelling (the #defines below
     * keep the original names), no type, size, order or control flow changes.
     *
     * Why it is load-bearing: pos34 (Pos, 8B, address-taken) and F_tsm (void*,
     * register-allocated with a 4-byte spill home) contend for the same slot
     * in VC6 SP3's stack-colouring pool
     *     0x24(8) 0x2c(8) [ X ] 0x40(8) 0x48(8) 0x50(4) 0x54(4)
     * and whichever the packer visits first takes the low end.  With the four
     * buffers as four separate objects F_tsm wins ([esp+0x34]) and pos34 is
     * pushed to [esp+0x38] -- 12 instructions in S2/S4/S5/S6/S10 then use the
     * wrong displacement.  The only previously known fix was an extra dead
     * `pos34.x = 0;` store, which cost one emitted instruction.  Collapsing
     * the four buffers into one object removes three objects from the packer's
     * work list, the visit order of the contended pair flips, and pos34 lands
     * on [esp+0x34] with F_tsm at [esp+0x3c] -- exactly the original -- for
     * free.  Nothing else in the frame moves (verified against the original
     * with a /FAs symbol->offset readout and r4_slots.py).
     * MEASURED, none of which flip the pair on their own: merging any proper
     * subset of the buffers (58+6c, 6c+134, 134+334), merging LR with buf_58,
     * merging L_1c/L_20, folding L_1c/L_20 into LB, declaration order of
     * pos34 / F_tsm / F_fillflag / F_curval anywhere in the block, and
     * spelling pos34 as a bare struct instead of the Pos typedef.  Merging
     * 6c+134+334 or 58+6c+134 does flip the pair but drags F_curval from
     * 0x48 to 0x40; only the full four-way merge leaves the rest of the frame
     * untouched. */
    struct {
        char a[0x14];
        char b[0xc8];
        char c[0x200];
        char d[0x200];
    } BA;
#define buf_58  BA.a
#define buf_6c  BA.b
#define buf_134 BA.c
#define buf_334 BA.d

    void*  file;
    void*  elem;
    unsigned char* buf;
    unsigned char* ubuf;      /* S9 gets its own RLE cursor (orig encodes [ebx+esi]) */        /* RLE source cursor base */
    int    bi;                 /* RLE buffer index */
    int    x, y;
    unsigned opbyte, runlen, op;
    int    i;

    /* ================= S1: prologue / early returns ================= */
    if (g_map_loaded != 0)
        return -1;
    if (LLIDB_FindElement(mapName, &L_1c, 0) != 0)
        return -2;
    g_map_elem = L_1c;
    Format(buf_134, FMT_map_filename, *(int*)((char*)L_1c + 4));
    y = (int)RES_OpenFile(buf_134);
    L_20 = (void*)y;
    if (y == 0)
        return -3;

    /* ================= S2: header, tsm_mapping, terrain ============= */
    g_build_in_progress = 1;
    /* FRAME-SLOT NOTE (historical).  pos34 and F_tsm are laid out by VC6 SP3
     * in a single linear stack-colouring list that also contains the four
     * inlined S8 Pos temps:
     *   0x24(8) 0x2c(8) [pos34(8) | F_tsm(4)] 0x40(8) 0x48(8) 0x50(4) 0x54(4)
     * Whichever of pos34 / F_tsm the packer visits first takes the low end.
     * The original has pos34@0x34 + F_tsm@0x3c; getting the other order costs
     * 12 matched instructions in S2 (0x00461b71), S4 (0x00461d67, 0x00461d90),
     * S5/S6 (0x00461f3f, 0x00461f8d, 0x00461faa, 0x00462090, 0x004620de,
     * 0x004620fa) and S10 (0x00462880).
     * This used to be forced with a dead `pos34.x = 0;` store here, which was
     * the last remaining extra instruction in the function.  It is no longer
     * needed: collapsing the four frame buffers into the BA aggregate (see the
     * declaration block) flips the visit order for free.
     * MEASURED FACTS about the ordering key (all inert on their own):
     * declaration order, symbol name, type/signedness, block scope, 1-member
     * struct / 1-element-array wrappers, `(void)&pos34`, an early
     * `Pos* pp = &pos34`, self-assignment, `pos34 = tmp` whole-object copies,
     * `if(0)`/`while(0)`/`sizeof()`/short-circuit phantom defs, a dead store
     * that VC6 then deletes, duplicated identical stores, extra *loads* of
     * pos34 that store-forwarding removes, reducing F_tsm's source-level use
     * count, writing F_tsm through an inlined `*p = v` helper, and deferring
     * F_tsm's assignment past S4 (copy propagation just renames it).
     * pos34 and F_tsm also cannot be merged into one aggregate: as a struct
     * member the tsm pointer stops being register-allocated and the S5/S6 arms
     * each gain a reload (measured: +6 instructions, -110 matched). */
    ResetBuildStats();

    RES_ReadFile((void*)y, &LB.c, 4);
    RES_ReadFile((void*)y, buf_134, LB.c);            /* map name (not terminated) */

    RES_ReadFile((void*)y, &LB.c, 4);
    RES_ReadFile((void*)y, buf_134, LB.c);
    buf_134[LB.c] = 0;
    LLIDB_FindElement(buf_134, &L_1c, 0);
    g_tsm_mapping_elem = L_1c;
    F_tsm = LLIDB_LoadData(L_1c);
    g_default_tile = **(int**)((char*)F_tsm + 4);

    RES_ReadFile((void*)y, &LB.c, 4);
    RES_ReadFile((void*)y, buf_134, LB.c);
    buf_134[LB.c] = 0;
    LLIDB_FindElement(buf_134, &L_1c, 0);
    g_terrain_elem = L_1c;
    g_terrain_elem_data = LLIDB_LoadData(L_1c);

    RES_ReadFile((void*)y, (char*)g_map + 0x14, 2);   /* width  */
    RES_ReadFile((void*)y, (char*)g_map + 0x16, 2);   /* height */

    if (RES_ReadFile((void*)y, &g_perim_count, 4) != 4) {
        RES_CloseFile((void*)y);
        g_map_loaded = 1;
        g_build_in_progress = 0;
        return 1;
    }

    /* ================= S3: zero cell flags/trailing word =========== */
    for (L_2c = 0; L_2c < (int)g_map->height; L_2c++) {
        for (x = 0; x < (int)g_map->width; x++) {
            *(unsigned short*)((char*)g_map_rows[L_2c] + x * 20 + 0xc) = 0;
            *(unsigned short*)((char*)g_map_rows[L_2c] + x * 20 + 0x12) = 0;
        }
    }

    /* ================= S4: perimeter array A + env objects ========= */
    g_array_A = (void**)HeapAlloc_w(g_perim_count * 4);
    for (i = 0; i < (int)g_perim_count; i++) {
        progress_tick();
        RES_ReadFile((void*)y, &LB.c, 4);
        RES_ReadFile((void*)y, buf_334, LB.c);
        buf_334[LB.c] = 0;
        LLIDB_FindElement(buf_334, &L_1c, 0);
        g_array_A[i] = L_1c;
        LLIDB_LoadData(L_1c);
        elem = L_1c;
        if (*(void**)((char*)elem + 0xc) != 0)
            *(unsigned int*)((char*)elem + 8) |= 4;
    }

    LLIDB_FindElement(kEnvClassName, &L_1c, 0);
    LLIDB_LoadData(L_1c);
    elem = L_1c;
    g_env_class = *(void**)((char*)elem + 0xc);

    RES_ReadFile((void*)y, &LR.b, 4);
    for (i = 0; i < LR.b; i++) {
        progress_tick();
        RES_ReadFile((void*)y, &LR.a, 4);
        RES_ReadFile((void*)y, &pos34, 8);
        *(int*)(*(char**)((char*)g_array_A[LR.a] + 0xc) + 0x4c) = 0;
        PutObjOnMap(*(void**)((char*)g_array_A[LR.a] + 0xc),
                    g_array_A[LR.a], &pos34);
    }

    /* ================= S4/S5: perimeter array B =================== */
    RES_ReadFile((void*)y, &LB.a, 4);
    g_perim_aux = LB.a;
    g_array_B = (void**)HeapAlloc_w(LB.a * 4);
    for (i = 0; i < LB.a; i++) {
        void* d;
        void* q;
        progress_tick();
        RES_ReadFile((void*)y, &LB.c, 4);
        RES_ReadFile((void*)y, buf_334, LB.c);
        buf_334[LB.c] = 0;
        LLIDB_FindElement(buf_334, &L_1c, 0);
        LLIDB_LoadData(L_1c);
        g_array_B[i] = *(void**)((char*)L_1c + 0xc);
        q = *(void**)((char*)g_array_B[i] + 0x14);
        if (q != 0 && *(int*)((char*)q + 0xc) != 0)
            *(unsigned int*)((char*)q + 8) |= 4;
    }

    /* ================= S5/S6: base-tile RLE decode chunks ========== */
    LB.a = 0;                    /* x */
    y = 0;
    F_curval = 0;                    /* curval */
    RES_ReadFile(L_20, &LB.b, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)LB.b);
    RES_ReadFile(L_20, buf, LB.b);
    bi = 2;

    while (y < (int)g_map->height) {
        progress_tick();
        opbyte = buf[bi++];
        runlen = opbyte & 0x3f;
        op = opbyte & 0xc0;
        if (op != 0 && runlen == 0)
            runlen = 0x40;
        if (op > 0xc0)
            goto base_tail;

        switch (op) {
        case 0x00:
            /* set-value */
            F_curval = runlen;
            goto base_tail;
        case 0x40:
            /* run: place curval over runlen cells.
             * LOAD-BEARING IDIOM: `while (runlen-- != 0)` is what VC6 SP3
             * lowers to the original's guard
             *   mov ecx,eax / dec eax / test ecx,ecx / je end / inc eax
             *   mov [esp+0x2c],eax        (trip count) ... do {} while(--trip)
             * (0x461efe, 0x46204d, 0x46219a).  Writing the guard and the
             * counter out by hand lets VC6 delete the dead `dec`/`inc` pair.
             * The `F_fillflag = F_curval & 0x20` assignment must sit INSIDE the loop so
             * VC6's LICM sinks it into the preheader AFTER the guard, matching
             * 0x461f09-0x461f0f. */
            while (runlen-- != 0) {
                F_fillflag = F_curval & 0x20;
                if (F_fillflag != 0) {
                    /* object path */
                    void* obj;
                    void* cls;
                    unsigned char* cellptr;
                    *(unsigned short*)((char*)g_map_rows[y] + LB.a * 20 + 0xa) =
                        (unsigned short)g_default_tile;
                    pos34.x = LB.a;
                    pos34.y = y;
                    if (LB.a < 0 || LB.a >= (unsigned short)g_map->width ||
                        y < 0 || y >= (unsigned short)g_map->height)
                        cellptr = 0;
                    else
                        cellptr = (unsigned char*)g_map_rows[y] + LB.a * 20;
                    obj = *(void**)((char*)g_array_B[F_curval & 0x1f] + 0x14);
                    *(void**)cellptr = obj;
                    cls = *(void**)((char*)obj + 0xc);
                    PutObjOnMap(cls, obj, &pos34);
                    bi++;
                } else {
                    /* Tile path.  The (unsigned)(unsigned char) casts below are
                     * LOAD-BEARING: they are what makes VC6 emit the explicit
                     * byte-narrowing the original has --
                     *   mov ebp,edx / and ebp,0xff / shr ebp,8  (0x00461fb7,
                     *   0x00462107)  and, on the SetMapTile operand,
                     *   and edx,0xff (0x00461fed, 0x0046213d) plus
                     *   movzx cx,cl  (0x00461ff8, 0x00462148).
                     *
                     * NO NAMED BYTE LOCAL.  The data byte must be spelled as the
                     * bare `buf[bi]` subexpression in every use, with no `db` /
                     * `db2` local in between.  A named local of ANY width loses:
                     *   - `unsigned short db` makes VC6 widen at the load
                     *     (movzx dx, byte ptr [edi+ebp]) where the original
                     *     leaves the upper bits dirty (mov dl, byte ptr
                     *     [edi+ebp], 0x00461fa2 / 0x004620f2) and re-masks at
                     *     each use; the wider load also perturbs the U/V-pipe
                     *     schedule, pushing `mov ebx,[esp+0x3c]` one slot early
                     *     and `lea ecx,[ecx+ecx*4]` five slots late vs
                     *     0x00461faa / 0x00461fb1.
                     *   - `unsigned db2` adds a leading `xor ecx,ecx` the
                     *     original does not have before `mov cl,[edi+ebp]`
                     *     (0x00461fe8 / 0x00462138).
                     *   - `unsigned char` / `char` DOES give `mov dl` and does
                     *     fix the schedule, but a byte-typed *named local* gets
                     *     homed to a stack byte (mov byte ptr [esp+0x48],dl then
                     *     reloaded twice), which renumbers the frame and costs
                     *     ~100 instructions function-wide.  Only a CSE temp --
                     *     i.e. the inlined `buf[bi]` -- lives in dl with no home
                     *     slot, which is what the original does.
                     * Written this way the whole 0x00461fa2-0x00462011 and
                     * 0x004620f2-0x00462161 tile paths match instruction for
                     * instruction.  Worth +6 matched and -2 emitted instructions.
                     * The two reads of buf[bi] are value-identical (bi is not
                     * advanced until after the block and nothing writes into
                     * buf), so this is a pure spelling change. */
                    unsigned ib = (unsigned char)(((F_curval << 8) - 1) >> 8);
                    unsigned short* rec = *(unsigned short**)((char*)F_tsm + (((unsigned)(unsigned char)buf[bi] >> 8) | ib) * 8 + 4);
                    *(unsigned short*)((char*)g_map_rows[y] + LB.a * 20 + 0xa) =
                        (unsigned short)(*rec + (unsigned char)buf[bi]);
                    {
                        unsigned short* rec2 = *(unsigned short**)((char*)F_tsm + (((unsigned)(unsigned char)buf[bi] >> 8) | ib) * 8 + 4);
                        SetMapTile(LB.a, y, (unsigned short)(*rec2 + (unsigned char)buf[bi]));
                    }
                    bi++;
                }
                LB.a++;
                if (LB.a >= (unsigned short)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            goto base_tail;
        case 0x80:
            /* fill run */
            while (runlen-- != 0) {
                F_fillflag = F_curval & 0x20;
                if (F_fillflag != 0) {
                    void* obj;
                    void* cls;
                    unsigned char* cellptr;
                    *(unsigned short*)((char*)g_map_rows[y] + LB.a * 20 + 0xa) =
                        (unsigned short)g_default_tile;
                    pos34.x = LB.a;
                    pos34.y = y;
                    if (LB.a < 0 || LB.a >= (unsigned short)g_map->width ||
                        y < 0 || y >= (unsigned short)g_map->height)
                        cellptr = 0;
                    else
                        cellptr = (unsigned char*)g_map_rows[y] + LB.a * 20;
                    obj = *(void**)((char*)g_array_B[F_curval & 0x1f] + 0x14);
                    *(void**)cellptr = obj;
                    cls = *(void**)((char*)obj + 0xc);
                    PutObjOnMap(cls, obj, &pos34);
                } else {
                    unsigned ib = (unsigned char)(((F_curval << 8) - 1) >> 8);
                    /* Same no-named-byte-local rule as the 0x40 arm above. */
                    unsigned short* rec = *(unsigned short**)((char*)F_tsm + (((unsigned)(unsigned char)buf[bi] >> 8) | ib) * 8 + 4);
                    *(unsigned short*)((char*)g_map_rows[y] + LB.a * 20 + 0xa) =
                        (unsigned short)(*rec + (unsigned char)buf[bi]);
                    {
                        unsigned short* rec2 = *(unsigned short**)((char*)F_tsm + (((unsigned)(unsigned char)buf[bi] >> 8) | ib) * 8 + 4);
                        SetMapTile(LB.a, y, (unsigned short)(*rec2 + (unsigned char)buf[bi]));
                    }
                }
                LB.a++;
                if (LB.a >= (unsigned short)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            bi++;
            goto base_tail;
        case 0xc0:
            /* op == 0xc0: zero run */
            while (runlen-- != 0) {
                *(unsigned short*)((char*)g_map_rows[y] + LB.a * 20 + 0xa) = 0;
                LB.a++;
                if (LB.a >= (unsigned short)g_map->width) {
                    LB.a = 0;
                    y++;
                }
                SetMapTile(LB.a, y, 0);
            }
            break;
        }

    /* Merge point for every opcode arm; 0x004621f3 re-tests y against the map
     * height and 0x00462200 is the only back-edge of the base-tile loop. */
    base_tail:
        ;
    }

    /* Chunk exhausted: free the base-tile stream and read the next chunk.
     * The original does NOT re-enter the base decoder here.  Both the entry
     * guard (0x00461eb1 jbe) and the loop exit (0x00462200) land on
     * 0x00462206 = HeapFree_w, and the single test at 0x00462246
     * `cmp word [ecx+0x16], si / jbe 0x462388` either skips S7 outright or
     * falls straight through into the S7 body at 0x00462250.
     * The previous revision spliced this transition INTO the base-tile loop
     * and branched back to its top, which re-decoded the map-flags chunk as
     * base tiles (and then kept consuming chunks) for any map with height > 0
     * -- a real bug, not just a mismatch. */
    HeapFree_w(buf);
    file = L_20;
    LB.a = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &LB.b, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)LB.b);
    RES_ReadFile(L_20, buf, LB.b);

    /* ================= S7: map-flags RLE decode =================== */
    while (y < (int)(unsigned short)g_map->height) {
        progress_tick();
        opbyte = buf[bi++];
        runlen = opbyte & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = opbyte & 0xc0;
        switch (op) {
        case 0x40:
            /* COPY: distinct data byte per cell */
            while (runlen-- != 0) {
                unsigned db = buf[bi++];
                Cell* c = &g_map_rows[y][LB.a];
                /* NOTE: both arms are deliberately identical. The original
                 * (0x004622ae / 0x00462333) tests the tile-info flag and then
                 * emits two blocks that compute the same value in different
                 * registers (cx vs dx) — a VC6 SP3 artefact. Reproducing the
                 * degenerate branch is required to match; do not "simplify". */
                if (g_tile_info[c->tile].code & 0x20)
                    SetMapFlags(LB.a, y, (unsigned short)(c->flags | db));
                else
                    SetMapFlags(LB.a, y, (unsigned short)(c->flags | db));
                LB.a++;
                if (LB.a >= (int)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            break;
        case 0x80:
            /* FILL: one data byte over runlen cells */
            {
                unsigned rv = buf[bi++];
                /* (Earlier revisions needed a dead `L_14 = runlen` mirror store
                 * here, worth ~36 instructions, purely to win the [esp+0x14] /
                 * [esp+0x18] ordering.  The LB aggregate at the head of the
                 * function makes that ordering explicit in C, so the mirror is
                 * gone and 0x00462290 no longer carries an extra store.) */
                while (runlen-- != 0) {
                    Cell* c = &g_map_rows[y][LB.a];
                /* NOTE: both arms are deliberately identical. The original
                 * (0x004622ae / 0x00462333) tests the tile-info flag and then
                 * emits two blocks that compute the same value in different
                 * registers (cx vs dx) — a VC6 SP3 artefact. Reproducing the
                 * degenerate branch is required to match; do not "simplify". */
                    if (g_tile_info[c->tile].code & 0x20)
                        SetMapFlags(LB.a, y, (unsigned short)(c->flags | rv));
                    else
                        SetMapFlags(LB.a, y, (unsigned short)(c->flags | rv));
                    LB.a++;
                    if (LB.a >= (int)g_map->width) {
                        LB.a = 0;
                        y++;
                    }
                }
            }
            break;
        }
    }

    /* ================= S8: RF-flags terrain RLE decode ============ */
    HeapFree_w(buf);
    LB.a = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &LB.b, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)LB.b);
    RES_ReadFile(L_20, buf, LB.b);
    while (y < (int)(unsigned short)g_map->height) {
        progress_tick();
        opbyte = buf[bi++];
        runlen = opbyte & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = opbyte & 0xc0;
        switch (op) {
        case 0x40: {
            /* COPY: distinct data byte per cell.
             * 'while (runlen--)' (NOT do/while) is deliberate: VC6 lowers it to
             * mov r,eax / dec eax / test r,r / je end / lea ebx,[eax+1], which is
             * exactly 0x004624d0-0x004624db.  A do/while drops the entry guard. */
            while (runlen--) {
                unsigned short cf = GetMapFlags(LB.a, y);
                int byte;
                Cell* c;
                unsigned char rf;
                LB.c = cf;
                byte = buf[bi];
                bi++;
                Set_RFFlags(LB.a << 8, y << 8, (unsigned char)byte);
                if (!((unsigned char)LB.c & 8) && ((unsigned char)LB.c & 0x10)) {
                    s8_gfx(LB.a, y);
                }
                c = &g_map_rows[y][LB.a];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    s8_sq(LB.a, y);
                }
                LB.a++;
                if (LB.a >= (unsigned short)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            break;
        }
        case 0x80: {
            /* FILL: one data byte over runlen cells (same while(runlen--) form,
             * 0x004623fd-0x00462408; the trailing bi++ is the je target 0x004624ca). */
            while (runlen--) {
                unsigned short cf = GetMapFlags(LB.a, y);
                int rfarg;
                Cell* c;
                unsigned char rf;
                LB.c = cf;
                rfarg = (cf & 0xff00) | buf[bi];
                Set_RFFlags(LB.a << 8, y << 8, (unsigned char)rfarg);
                if (!((unsigned char)LB.c & 8) && ((unsigned char)LB.c & 0x10)) {
                    s8_gfx(LB.a, y);
                }
                c = &g_map_rows[y][LB.a];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    s8_sq(LB.a, y);
                }
                LB.a++;
                if (LB.a >= (unsigned short)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            bi++;
            break;
        }
        }
    }

    /* ================= S9: user-flags RLE + perimeter + texture ==== */
    HeapFree_w(buf);
    LB.a = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &LB.b, 4);
    ubuf = (unsigned char*)HeapAlloc_w((unsigned)LB.b);
    RES_ReadFile(L_20, ubuf, LB.b);
    {
    while (y < (int)(unsigned short)g_map->height) {
        progress_tick();
        opbyte = ubuf[bi++];
        runlen = opbyte & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = opbyte & 0xc0;
        switch (op) {
        case 0x40:
            while (runlen--) {
                unsigned short val = ubuf[bi++];
                Set_UserFlags(LB.a << 8, y << 8, val);
                LB.a++;
                if (LB.a >= (int)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            break;
        case 0x80:
            while (runlen--) {
                unsigned short val = ubuf[bi];
                Set_UserFlags(LB.a << 8, y << 8, val);
                LB.a++;
                if (LB.a >= (int)g_map->width) {
                    LB.a = 0;
                    y++;
                }
            }
            bi++;
            break;
        }
    }
    }
    HeapFree_w(ubuf);

    /* perimeter records */
    s9_perims(L_20, buf_58);

    /* optional magic-tagged terrain-texture name + S10 base-layer RLE */
    file = L_20;
    LB.b = 0;
    L_2c = 0;
    s9b_s10(file, buf_6c, F_tsm, &LB.b, L_2c);

    RES_CloseFile(L_20);
    g_map_loaded = 1;
    g_build_in_progress = 0;
    g_state_810140 = 0;
    RenderInit();
    return 1;
}
#undef buf_58
#undef buf_6c
#undef buf_134
#undef buf_334
