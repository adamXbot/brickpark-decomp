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
extern void  Set_UserFlags(int x, int y, unsigned int value);/* 0x461730 */
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
    Pos    pos34;              /* Pos{x,y} @ 0x34 */
    void*  L_3c;               /* tsm/terrain descriptor table base */
    int    L_40;               /* fill-run branch flag (curval & 0x20) */
    int    L_48;               /* current RLE value (curval) */
    union { int n; Pos q; } u50;
    union { int n; Pos q; } u54;
    char   buf_58[0x14];       /* 20-byte perimeter record buffer */
    char   buf_6c[0xc8];       /* ~200-byte scratch: magic tag / texture name */
    char   buf_134[0x200];     /* map filename / element-name buffer */
    char   buf_334[0x204];     /* element-name buffer (object loops) */

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

    RES_ReadFile((void*)y, &u54.n, 4);
    for (i = 0; i < u54.n; i++) {
        progress_tick();
        RES_ReadFile((void*)y, &u50.n, 4);
        RES_ReadFile((void*)y, &pos34, 8);
        *(int*)(*(char**)((char*)g_array_A[u50.n] + 0xc) + 0x4c) = 0;
        PutObjOnMap(*(void**)((char*)g_array_A[u50.n] + 0xc),
                    g_array_A[u50.n], &pos34);
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
    file = *(void* volatile*)&L_20;
    L_10 = 0;                    /* x */
    y = 0;
    L_48 = 0;                    /* curval */
    RES_ReadFile(file, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    u24.rle = buf;
    RES_ReadFile(file, buf, L_14);
    bi = 2;

    while ((unsigned)y < (unsigned short)g_map->height) {
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
            /* run: place curval over runlen cells */
            if (runlen == 0)
                goto base_tail;
            L_40 = L_48 & 0x20;
            L_2c = runlen;
            for (;;) {
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
                    /* tile path */
                    unsigned idx = (unsigned char)(((L_48 << 8) - 1) >> 8);
                    unsigned char db = buf[bi];
                    unsigned short* rec = *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                    unsigned short base = (unsigned short)(*rec + db);
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) = base;
                    {
                        unsigned char db2 = u24.rle[bi];
                        unsigned short* rec2 = *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                        unsigned short disp = (unsigned short)(*rec2 + db2);
                        SetMapTile(L_10, y, disp);
                    }
                    bi++;
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
                if (--L_2c == 0)
                    break;
            }
            goto base_tail;
        case 0x80:
            /* fill run */
            if (runlen == 0)
                goto base_fillskip;
            L_40 = L_48 & 0x20;
            L_2c = runlen;
            for (;;) {
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
                    unsigned idx = (unsigned char)(((L_48 << 8) - 1) >> 8);
                    unsigned char db = buf[bi];
                    unsigned short* rec = *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                    unsigned short base = (unsigned short)(*rec + db);
                    *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) = base;
                    {
                        unsigned char db2 = u24.rle[bi];
                        unsigned short* rec2 = *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                        unsigned short disp = (unsigned short)(*rec2 + db2);
                        SetMapTile(L_10, y, disp);
                    }
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
                if (--L_2c == 0)
                    break;
            }
        base_fillskip:
            bi++;
            goto base_tail;
        case 0xc0:
            /* op == 0xc0: zero run */
            if (runlen == 0)
                goto base_tail;
            L_2c = runlen;
            for (;;) {
                *(unsigned short*)((char*)g_map_rows[y] + L_10 * 20 + 0xa) = 0;
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
                SetMapTile(L_10, y, 0);
                if (--L_2c == 0)
                    break;
            }
            break;
        }

    base_tail:
        if ((unsigned)y < (unsigned short)g_map->height)
            continue;
        /* chunk exhausted: free and read next chunk */
        HeapFree_w(buf);
        file = L_20;
        bi = 0;
        L_10 = 0;
        y = 0;
        RES_ReadFile(file, &L_14, 4);
        buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
        u24.rle = buf;
        RES_ReadFile(file, buf, L_14);
        if ((unsigned short)g_map->height <= (unsigned)y)
            goto after_baserle;
        /* fall through into next chunk decode via S7? no: continue base loop */
    }

after_baserle:
    /* ================= S7: map-flags RLE decode =================== */
    while ((int)y < (int)g_map->height) {
        progress_tick();
        opbyte = buf[bi++];
        runlen = opbyte & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = opbyte & 0xc0;
        if (op == 0x40) {
            L_2c = runlen;
            do {
                unsigned char fb = buf[bi++];
                Cell* c = &g_map_rows[y][L_10];
                unsigned short fl = (unsigned short)(c->flags | fb);
                SetMapFlags(L_10, y, fl);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
        } else if (op == 0x80) {
            unsigned char rv = buf[bi++];
            L_2c = runlen;
            do {
                Cell* c = &g_map_rows[y][L_10];
                unsigned short fl = (unsigned short)(c->flags | rv);
                SetMapFlags(L_10, y, fl);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
        }
    }

    /* ================= S8: RF-flags terrain RLE decode ============ */
    HeapFree_w(buf);
    bi = 0;
    L_10 = 0;
    y = 0;
    RES_ReadFile(L_20, &L_14, 4);
    buf = (unsigned char*)HeapAlloc_w((unsigned)L_14);
    RES_ReadFile(L_20, buf, L_14);
    while ((unsigned)y < (unsigned short)g_map->height) {
        unsigned char b;
        progress_tick();
        b = buf[bi++];
        runlen = b & 0x3f;
        if (runlen == 0)
            runlen = 0x40;
        op = b & 0xc0;
        if (op == 0x40) {
            /* COPY: distinct data byte per cell */
            L_2c = runlen;
            do {
                unsigned short cf = GetMapFlags(L_10, y);
                int byte = buf[bi++];
                Cell* c;
                unsigned char rf;
                L_18 = cf;
                Set_RFFlags(L_10 << 8, y << 8, (unsigned char)byte);
                if (!((unsigned char)L_18 & 8) && ((unsigned char)L_18 & 0x10)) {
                    u24.q.x = L_10;
                    u24.q.y = y;
                    AddPathTileGFX(&u24.q, *(unsigned short*)(*g_path_tile_ptr));
                }
                c = &g_map_rows[y][L_10];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    u50.q.x = L_10;
                    u50.q.y = y;
                    AddPathSquare(&u50.q);
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
        } else if (op == 0x80) {
            /* FILL: one data byte over runlen cells */
            L_2c = runlen;
            do {
                unsigned short cf = GetMapFlags(L_10, y);
                int rfarg;
                Cell* c;
                unsigned char rf;
                L_18 = cf;
                rfarg = (cf & 0xff00) | buf[bi];
                Set_RFFlags(L_10 << 8, y << 8, (unsigned char)rfarg);
                if (!((unsigned char)L_18 & 8) && ((unsigned char)L_18 & 0x10)) {
                    pos34.x = L_10;
                    pos34.y = y;
                    AddPathTileGFX(&pos34, *(unsigned short*)(*g_path_tile_ptr));
                }
                c = &g_map_rows[y][L_10];
                rf = c->rf;
                if ((rf & 1) || ((c->flags & 0x10) && !(rf & 2))) {
                    u54.q.x = L_10;
                    u54.q.y = y;
                    AddPathSquare(&u54.q);
                }
                L_10++;
                if (L_10 >= (unsigned short)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
            bi++;
        }
    }

    /* ================= S9: user-flags RLE + perimeter + texture ==== */
    HeapFree_w(buf);
    L_10 = 0;
    bi = 0;
    y = 0;
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
        case 0x40:
            L_2c = runlen;
            do {
                unsigned val = buf[bi++];
                Set_UserFlags(L_10 << 8, y << 8, val);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
            break;
        case 0x80:
            L_2c = runlen;
            do {
                unsigned val = buf[bi];
                Set_UserFlags(L_10 << 8, y << 8, val);
                L_10++;
                if (L_10 >= (int)g_map->width) {
                    L_10 = 0;
                    y++;
                }
            } while (--L_2c != 0);
            bi++;
            break;
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
        int k;
        for (k = 0; k < 0xc8; k++)
            buf_6c[k] = 0;
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
                                unsigned idx = (unsigned char)(((v - 0x100) >> 8) & 0xff);
                                unsigned low = v & 0xff;
                                unsigned short* p =
                                    *(unsigned short**)((char*)L_3c + idx * 8 + 4);
                                unsigned short base = (unsigned short)(*p + low);
                                *(unsigned short*)((char*)g_map_rows[y] + x * 0x14 + 0xa) = base;
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
