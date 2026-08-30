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
extern void  RES_CloseFile(void* file);         /* 0x489de0 */
extern int   RES_GetFilePointer(void* file);    /* 0x489db0 */
extern void  RES_SetFilePointer(void* file, int pos); /* 0x489d70 */
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

// WIP-FUNCTION: LEGOLAND 0x00461a50  (grind in progress)
int LoadBaseMap(char* mapName)
{
    int    L_10;               /* count / current column x */
    int    L_14;               /* 4-byte read scratch (size/len/control byte) */
    unsigned int L_18;         /* name length / flags temp */
    void*  L_1c;               /* LLElem* out of LLIDB_FindElement */
    void*  L_20;               /* open resource file handle */
    union { unsigned char* rle; Pos q; } u24;
    int    L_2c;               /* run / loop counter */
    Pos    pos34;              /* PutObjOnMap Pos -- lands on [esp+0x34],
                                * matching the original (S4/S5/S6). */
    void*  L_3c;               /* tsm/terrain descriptor table base */
    int    L_40;               /* fill-run branch flag (curval & 0x20) */
    int    L_48;               /* current RLE value (curval) */
    /* FRAME LAYOUT NOTES (measured, not guessed):
     *  - VC6 SP3 ignores declaration ORDER entirely in this function: reversing
     *    this whole block yields a byte-identical .obj.  Slots are handed out by
     *    first use in the optimised IR, so the only levers are which temporaries
     *    exist and which of them are address-taken.
     *  - The original keeps the S4 env-object index/count as plain 4-byte ints
     *    at [esp+0x50]/[esp+0x54].  Declaring them int (rather than unioning a
     *    Pos onto them, as an earlier revision did) is worth ~0.4% by itself.
     *  - The four S8 AddPath* Pos temporaries sit at [esp+0x24]/0x2c/0x40/0x48
     *    in the original, each overlaid on an S5/S6 int -- hence the 4-byte
     *    holes at 0x30 and 0x44.  Spelling those overlaps as unions really does
     *    put the Pos on the right slot, but it makes curval / L_2c / L_40
     *    address-taken and the extra reloads cost far more than the slots gain.
     *    A 6905-way sweep over {demotion of u50/u54} x {routing of the four Pos
     *    across fresh vars, unions and reuse} put every union variant at or
     *    below this arrangement. */
    Pos    pos_gfx;            /* AddPathTileGFX Pos (both S8 branches) */
    int    L_50;               /* S4 env-object record index */
    int    L_54;               /* S4 env-object record count */
    char   buf_58[0x14];       /* 20-byte perimeter record buffer */
    char   buf_6c[0xc8];       /* ~200-byte scratch: magic tag / texture name */
    char   buf_134[0x200];     /* map filename / element-name buffer */
    char   buf_334[0x200];     /* element-name buffer (object loops) */

    void*  file;
    void*  elem;
    unsigned char* buf;        /* RLE source cursor base */
    int    bi;                 /* RLE buffer index */
    int    x, y;
    int    saved_pos;
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
    ResetBuildStats();

    RES_ReadFile((void*)y, &L_18, 4);
    RES_ReadFile((void*)y, buf_134, L_18);            /* map name (not terminated) */

    RES_ReadFile((void*)y, &L_18, 4);
    RES_ReadFile((void*)y, buf_134, L_18);
    buf_134[L_18] = 0;
    LLIDB_FindElement(buf_134, &L_1c, 0);
    g_tsm_mapping_elem = L_1c;
    L_3c = LLIDB_LoadData(L_1c);
    g_default_tile = **(int**)((char*)L_3c + 4);

    RES_ReadFile((void*)y, &L_18, 4);
    RES_ReadFile((void*)y, buf_134, L_18);
    buf_134[L_18] = 0;
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
        RES_ReadFile((void*)y, &L_18, 4);
        RES_ReadFile((void*)y, buf_334, L_18);
        buf_334[L_18] = 0;
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

    RES_ReadFile((void*)y, &L_54, 4);
    for (i = 0; i < L_54; i++) {
        progress_tick();
        RES_ReadFile((void*)y, &L_50, 4);
        RES_ReadFile((void*)y, &pos34, 8);
        *(int*)(*(char**)((char*)g_array_A[L_50] + 0xc) + 0x4c) = 0;
        PutObjOnMap(*(void**)((char*)g_array_A[L_50] + 0xc),
                    g_array_A[L_50], &pos34);
    }

    /* ================= S4/S5: perimeter array B =================== */
    RES_ReadFile((void*)y, &L_10, 4);
    g_perim_aux = L_10;
    g_array_B = (void**)HeapAlloc_w(L_10 * 4);
    for (i = 0; i < L_10; i++) {
        void* d;
        void* q;
        progress_tick();
        RES_ReadFile((void*)y, &L_18, 4);
        RES_ReadFile((void*)y, buf_334, L_18);
        buf_334[L_18] = 0;
        LLIDB_FindElement(buf_334, &L_1c, 0);
        LLIDB_LoadData(L_1c);
        g_array_B[i] = *(void**)((char*)L_1c + 0xc);
        q = *(void**)((char*)g_array_B[i] + 0x14);
        if (q != 0 && *(int*)((char*)q + 0xc) != 0)
            *(unsigned int*)((char*)q + 8) |= 4;
    }

    /* ================= S5/S6: base-tile RLE decode chunks ========== */
    L_10 = 0;                    /* x */
    y = 0;
    L_48 = 0;                    /* curval */
    RES_ReadFile(L_20, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    RES_ReadFile(L_20, buf, L_14);
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
            L_48 = runlen;
            goto base_tail;
        case 0x40:
            /* run: place curval over runlen cells.
             * LOAD-BEARING IDIOM: `while (runlen-- != 0)` is what VC6 SP3
             * lowers to the original's guard
             *   mov ecx,eax / dec eax / test ecx,ecx / je end / inc eax
             *   mov [esp+0x2c],eax        (trip count) ... do {} while(--trip)
             * (0x461efe, 0x46204d, 0x46219a).  Writing the guard and the
             * counter out by hand lets VC6 delete the dead `dec`/`inc` pair.
             * The `L_40 = L_48 & 0x20` assignment must sit INSIDE the loop so
             * VC6's LICM sinks it into the preheader AFTER the guard, matching
             * 0x461f09-0x461f0f. */
            while (runlen-- != 0) {
                L_40 = L_48 & 0x20;
                if (L_40 != 0) {
                    /* object path */
                    void* obj;
                    void* cls;
                    unsigned char* cellptr;
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) =
                        (unsigned short)g_default_tile;
                    pos34.x = L_10;
                    pos34.y = y;
                    if (L_10 < 0 || L_10 >= (unsigned short)g_map->width ||
                        y < 0 || y >= (unsigned short)g_map->height)
                        cellptr = 0;
                    else
                        cellptr = (unsigned char*)g_map_rows[y] + L_10 * 20;
                    obj = *(void**)((char*)g_array_B[L_48 & 0x1f] + 0x14);
                    *(void**)cellptr = obj;
                    cls = *(void**)((char*)obj + 0xc);
                    PutObjOnMap(cls, obj, &pos34);
                    bi++;
                } else {
                    /* Tile path.  The (unsigned)(unsigned char) casts below are
                     * LOAD-BEARING: they are what makes VC6 emit the explicit
                     * byte-narrowing the original has and a plain `unsigned db`
                     * does not --
                     *   mov ebp,edx / and ebp,0xff / shr ebp,8  (0x00461fb7,
                     *   0x00462107)  and, on the SetMapTile operand,
                     *   and edx,0xff (0x00461fed, 0x0046213d) plus
                     *   movzx cx,cl  (0x00461ff8, 0x00462148).
                     * The asymmetry is measured, not stylistic: the
                     * (unsigned char) cast belongs on the SetMapTile addend
                     * (db2) but NOT on the cell-store addend (db) -- adding it
                     * there makes VC6 swap ecx/edx across the whole block and
                     * costs ~35 instructions.
                     * db/db2 stay `unsigned` rather than `unsigned char`: a
                     * byte-typed local makes VC6 round-trip it through a fresh
                     * stack slot, which pushes the frame past sub esp,0x524 and
                     * renumbers every [esp+N] in the function. */
                    unsigned ib = (unsigned char)(((L_48 << 8) - 1) >> 8);
                    unsigned db = buf[bi];
                    unsigned short* rec = *(unsigned short**)((char*)L_3c + (((unsigned)(unsigned char)db >> 8) | ib) * 8 + 4);
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) =
                        (unsigned short)(*rec + db);
                    {
                        unsigned db2 = buf[bi];
                        unsigned short* rec2 = *(unsigned short**)((char*)L_3c + (((unsigned)(unsigned char)db2 >> 8) | ib) * 8 + 4);
                        SetMapTile(L_10, y, (unsigned short)(*rec2 + (unsigned char)db2));
                    }
                    bi++;
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            goto base_tail;
        case 0x80:
            /* fill run */
            while (runlen-- != 0) {
                L_40 = L_48 & 0x20;
                if (L_40 != 0) {
                    void* obj;
                    void* cls;
                    unsigned char* cellptr;
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) =
                        (unsigned short)g_default_tile;
                    pos34.x = L_10;
                    pos34.y = y;
                    if (L_10 < 0 || L_10 >= (unsigned short)g_map->width ||
                        y < 0 || y >= (unsigned short)g_map->height)
                        cellptr = 0;
                    else
                        cellptr = (unsigned char*)g_map_rows[y] + L_10 * 20;
                    obj = *(void**)((char*)g_array_B[L_48 & 0x1f] + 0x14);
                    *(void**)cellptr = obj;
                    cls = *(void**)((char*)obj + 0xc);
                    PutObjOnMap(cls, obj, &pos34);
                } else {
                    unsigned ib = (unsigned char)(((L_48 << 8) - 1) >> 8);
                    unsigned db = buf[bi];
                    unsigned short* rec = *(unsigned short**)((char*)L_3c + (((unsigned)(unsigned char)db >> 8) | ib) * 8 + 4);
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) =
                        (unsigned short)(*rec + db);
                    {
                        unsigned db2 = buf[bi];
                        unsigned short* rec2 = *(unsigned short**)((char*)L_3c + (((unsigned)(unsigned char)db2 >> 8) | ib) * 8 + 4);
                        SetMapTile(L_10, y, (unsigned short)(*rec2 + (unsigned char)db2));
                    }
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            bi++;
            goto base_tail;
        case 0xc0:
            /* op == 0xc0: zero run */
            while (runlen-- != 0) {
                *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) = 0;
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
                SetMapTile(L_10, y, 0);
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
    L_10 = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    RES_ReadFile(L_20, buf, L_14);

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
                Cell* c = &g_map_rows[y][L_10];
                /* NOTE: both arms are deliberately identical. The original
                 * (0x004622ae / 0x00462333) tests the tile-info flag and then
                 * emits two blocks that compute the same value in different
                 * registers (cx vs dx) — a VC6 SP3 artefact. Reproducing the
                 * degenerate branch is required to match; do not "simplify". */
                if (g_tile_info[c->tile].code & 0x20)
                    SetMapFlags(L_10, y, (unsigned short)(c->flags | db));
                else
                    SetMapFlags(L_10, y, (unsigned short)(c->flags | db));
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            break;
        case 0x80:
            /* FILL: one data byte over runlen cells */
            {
                unsigned rv = buf[bi++];
                /* FRAME-SLOT LEVER (load-bearing, worth ~36 instructions
                 * whole-function): mirroring the run length into L_14 keeps the
                 * [esp+0x14] read scratch live across S7.  Without it VC6 SP3
                 * coalesces that slot away and EVERY [esp+N] from S2 onward
                 * shifts down by one slot.  The original reuses the same slot
                 * as a counter in S10 (dec dword [esp+0x14] at 0x0046284d), so
                 * the variable really does stay live past S7.  L_14 is
                 * unconditionally re-read by the S8 chunk header read below, so
                 * the store is dead as far as behaviour goes.
                 * Costs exactly one extra `mov [esp+0x14],eax` at 0x00462290. */
                L_14 = (int)runlen;
                while (runlen-- != 0) {
                    Cell* c = &g_map_rows[y][L_10];
                /* NOTE: both arms are deliberately identical. The original
                 * (0x004622ae / 0x00462333) tests the tile-info flag and then
                 * emits two blocks that compute the same value in different
                 * registers (cx vs dx) — a VC6 SP3 artefact. Reproducing the
                 * degenerate branch is required to match; do not "simplify". */
                    if (g_tile_info[c->tile].code & 0x20)
                        SetMapFlags(L_10, y, (unsigned short)(c->flags | rv));
                    else
                        SetMapFlags(L_10, y, (unsigned short)(c->flags | rv));
                    L_10++;
                    if (L_10 >= (int)g_map->width) {
                        L_10 = 0;
                        y++;
                    }
                }
            }
            break;
        }
    }

    /* ================= S8: RF-flags terrain RLE decode ============ */
    HeapFree_w(buf);
    L_10 = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    RES_ReadFile(L_20, buf, L_14);
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
                unsigned short cf = GetMapFlags(L_10, y);
                int byte;
                Cell* c;
                unsigned char rf;
                L_18 = cf;
                byte = buf[bi];
                bi++;
                Set_RFFlags(L_10 << 8, y << 8, (unsigned char)byte);
                if (!((unsigned char)L_18 & 8) && ((unsigned char)L_18 & 0x10)) {
                    pos_gfx.x = L_10;
                    pos_gfx.y = y;
                    AddPathTileGFX(&pos_gfx, *(unsigned short*)g_path_tile_ptr);
                }
                c = &g_map_rows[y][L_10];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    u24.q.x = L_10;
                    u24.q.y = y;
                    AddPathSquare(&u24.q);
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            break;
        }
        case 0x80: {
            /* FILL: one data byte over runlen cells (same while(runlen--) form,
             * 0x004623fd-0x00462408; the trailing bi++ is the je target 0x004624ca). */
            while (runlen--) {
                unsigned short cf = GetMapFlags(L_10, y);
                int rfarg;
                Cell* c;
                unsigned char rf;
                L_18 = cf;
                rfarg = (cf & 0xff00) | buf[bi];
                Set_RFFlags(L_10 << 8, y << 8, (unsigned char)rfarg);
                if (!((unsigned char)L_18 & 8) && ((unsigned char)L_18 & 0x10)) {
                    pos_gfx.x = L_10;
                    pos_gfx.y = y;
                    AddPathTileGFX(&pos_gfx, *(unsigned short*)g_path_tile_ptr);
                }
                c = &g_map_rows[y][L_10];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    u24.q.x = L_10;
                    u24.q.y = y;
                    AddPathSquare(&u24.q);
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
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
    L_10 = 0;
    y = 0;
    bi = 0;
    RES_ReadFile(L_20, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    RES_ReadFile(L_20, buf, L_14);
    {
    while (y < (int)(unsigned short)g_map->height) {
        progress_tick();
        opbyte = buf[bi++];
        runlen = opbyte & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = opbyte & 0xc0;
        switch (op) {
        case 0x40:
            while (runlen--) {
                unsigned short val = buf[bi++];
                Set_UserFlags(L_10 << 8, y << 8, val);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            break;
        case 0x80:
            while (runlen--) {
                unsigned short val = buf[bi];
                Set_UserFlags(L_10 << 8, y << 8, val);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            }
            bi++;
            break;
        }
    }
    }
    HeapFree_w(buf);

    /* perimeter records */
    RES_ReadFile(L_20, &u24.rle, 4);
    for (i = 0; i < (int)(unsigned int)u24.rle; i++) {
        RES_ReadFile(L_20, buf_58, 0x14);
        build_perimeter(&buf_58);
    }

    /* optional magic-tagged terrain-texture name */
    file = L_20;
    L_14 = 0;
    L_2c = 0;
    saved_pos = RES_GetFilePointer(file);
    {
        /* VC6 inlines this as 'mov byte[buf],0' + 'rep stosd' of 0x31 dwords
         * starting at buf+1 + stosw + stosb — matching 0x462734..0x462750. */
        extern void* memset(void* d, int c, unsigned int n);
        buf_6c[0] = 0;
        memset(buf_6c + 1, 0, sizeof(buf_6c) - 1);
    }
    RES_ReadFile(file, buf_6c, 8);
    if (memcmp(buf_6c, g_terrain_magic, 8) == 0) {
        if (RES_ReadFile(file, &u24.rle, 4) == 4) {
            RES_ReadFile(file, buf_6c, (int)(unsigned int)u24.rle);
            buf_6c[(unsigned int)u24.rle] = 0;
            map_helper_4618d0(buf_6c);
            g_terrain_elem_2 = ElemID(buf_6c);
            g_terrain_texdata = LLIDB_LoadData(g_terrain_elem_2);
        }
    } else {
        RES_SetFilePointer(file, saved_pos);
    }

    /* ================= S10: base-layer RLE decode into cell.base === */
    {
        for (y = 0; y < (int)g_map->height; y++) {
            progress_tick();
            for (x = 0; x < (int)g_map->width; x++) {
                for (;;) {
                    switch (L_2c) {
                    case 0:
                        if (RES_ReadFile(L_20, &L_14, 1) != 1)
                            goto s10_close;
                        L_2c = (L_14 != 0) ? 2 : 1;
                        continue;
                    case 1:
                        {
                            unsigned int v;
                            RES_ReadFile(L_20, &pos34, 2);
                            v = *(unsigned int*)&pos34;
                            if (v == 0xffff) {
                                L_2c = 0;
                            } else {
                                unsigned int low = v & 0xff;
                                unsigned int idx = ((v - 0x100) >> 8) & 0xff;
                                unsigned short* p =
                                    *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                                *(unsigned short*)((char*)g_map_rows[y] + x * 0x14 + 0xa) =
                                    (unsigned short)(*p + low);
                            }
                        }
                        break;
                    case 2:
                        if (L_14 != 0) {
                            L_14--;
                            break;
                        }
                        L_2c = 1;
                        continue;
                    }
                    break;
                }
            }
        }
    }

s10_close:
    RES_CloseFile(L_20);
    g_map_loaded = 1;
    g_build_in_progress = 0;
    g_state_810140 = 0;
    RenderInit();
    return 1;
}
