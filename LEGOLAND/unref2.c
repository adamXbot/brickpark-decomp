/* LEGOLAND -- scope LL10: the DEAD (unreferenced) functions of the coaster
 * model / raster translation units, 0x00420fd0..0x004237f0.
 *
 * Nothing live in the executable calls, tail-jumps to or takes the address of
 * any of these; the linker kept them because the game was built without
 * /OPT:REF.  They are ordinary C from the same translation units as their
 * matched neighbours (coaster9.c, schoolcar4.c, schoolcar5.c, schoolcar8.c,
 * coastertiny.c), and read as the module's developer tooling: a wireframe
 * debug painter, a box-model builder, a "dump the two model images back to
 * disk" pair and a shaded sprite blitter.
 *
 * Reconstructed for VC6 SP3 /O2 /Gy /Gd.  Struct field OFFSETS, record sizes
 * and global addresses are load-bearing; the names are ours.  Types are
 * defined LOCALLY (legoland.h is owned elsewhere).  See docs/lanes/scope-ll10.md.
 */

typedef struct Pos { int x, y; } Pos;
typedef struct Vec3f { float x, y, z; } Vec3f;

/* {image, byte length} -- schoolcar4.c's g_cc_txt / g_cc_obj pair. */
typedef struct ModelImage { void* data; int length; } ModelImage;

extern ModelImage g_cc_txt;                    /* 0x004dd758 */
extern char       g_cc_name[0x100];            /* 0x004dd760 */
extern ModelImage g_cc_obj;                    /* 0x004dd860 */
extern void*      g_shade_block;               /* 0x00829c54 */

extern int __declspec(dllimport) __cdecl wsprintfA(char* out, const char* fmt, ...); /* 0x004ab298 */
__declspec(dllimport) int __stdcall CreateFileA(const char* name,
                                                unsigned int access,
                                                unsigned int share, void* sa,
                                                unsigned int disp,
                                                unsigned int flags,
                                                void* tmpl);            /* [0x4ab258] */
__declspec(dllimport) int __stdcall WriteFile(int h, const void* buf,
                                              unsigned int n,
                                              unsigned int* written,
                                              void* ov);                /* [0x4ab254] */
__declspec(dllimport) int __stdcall CloseHandle(int h);                 /* [0x4ab260] */

extern void ModelRecord_GetName(ModelImage* image, char* out, int index); /* 0x00422390 */
extern void Free_w(void* p);                                             /* 0x004775d0 */

/* ==========================================================================
 * 0x00423750 -- a one-byte `ret`.  No callers, no arguments visible, nothing
 * to name it after; recorded as Unref_00423750.  Its neighbours are the
 * raster save/restore pair (0x00423730 Raster_RestoreFloatMode, 0x00423790
 * Raster_RestoreState, itself an empty body), so this is a third hook of the
 * same shape that the module never filled in.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00423750
void Unref_00423750(void) {}

/* ==========================================================================
 * 0x00421530 -- store the argument in 0x004b5958, the dword that immediately
 * follows the box-model template at 0x004b58c8.  The global has exactly ONE
 * reference in the whole image (this store), so it is write-only and there is
 * no behaviour to name the function after: Unref_00421530.
 * ======================================================================== */
extern int g_4b5958;                                                   /* 0x004b5958 */
// FUNCTION: LEGOLAND 0x00421530
void Unref_00421530(int value)
{
    g_4b5958 = value;
}

/* ==========================================================================
 * 0x004225e0 -- the ".txt" twin of schoolcar8.c's CoasterModel_GetRecordName
 * (0x004225b0), which is the same three pushes against g_cc_obj.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x004225e0
void CoasterModel_GetTextureName(int index, char* out)
{
    ModelRecord_GetName(&g_cc_txt, out, index);
}

/* ==========================================================================
 * 0x004227a0 -- release both model images.  Neither pointer is cleared and
 * neither is null-checked, so this is LoadCoasterModelSet's (0x004226c0)
 * unwinder rather than a general teardown.  The two one-argument cdecl calls
 * share ONE `add esp, 8`.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x004227a0
void CoasterModel_FreeImages(void)
{
    Free_w(g_cc_txt.data);
    Free_w(g_cc_obj.data);
}

/* ==========================================================================
 * 0x00423060 -- release the shade-ramp block schoolcar5.c's CoasterShades_Init
 * (0x00422fe0) allocated at 0x00829c54.  The 1024 slot pointers into it are
 * left dangling.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00423060
void CoasterShades_Free(void)
{
    if (g_shade_block)
        Free_w(g_shade_block);
}

/* ==========================================================================
 * 0x00422520 -- write a block to a file, whole.  CREATE_ALWAYS, no sharing,
 * FILE_FLAG_SEQUENTIAL_SCAN, and success is "WriteFile reported exactly the
 * requested byte count".  The `written` out-parameter is homed in the DEAD
 * `name` argument slot (there is no `sub esp` at all), and the shared
 * `push esi` in front of the two CloseHandle calls is hoisted above the
 * comparison's branch.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00422520
int WriteWholeFile(const char* name, const void* data, unsigned int length)
{
    int          h;
    unsigned int written;

    if (!name)
        goto fail;
    if (!data)
        goto fail;
    h = CreateFileA(name, 0x40000000, 0, 0, 2, 0x8000000, 0);
    if (h == -1)
        goto fail;
    WriteFile(h, data, length, &written, 0);
    if (written != length) {
        CloseHandle(h);
        goto fail;
    }
    CloseHandle(h);
    return 1;
fail:
    return 0;
}

/* ==========================================================================
 * 0x00422650 -- the exact inverse of LoadCoasterModelSet (schoolcar4.c,
 * 0x004226c0): rebuild "<name>.txt" and "<name>.obj" from the base name kept
 * at 0x004dd760 and write both images back out.  The ".txt" image goes first
 * here, where the loader reads ".obj" first.
 *
 * All four cdecl calls share ONE `add esp, 0x30`, and the 0x100-byte path
 * buffer is reused for both names; wsprintfA's import thunk is hoisted into
 * esi exactly as it is in the loader.
 * ======================================================================== */
// FUNCTION: LEGOLAND 0x00422650
void CoasterModel_SaveImages(void)
{
    char path[0x100];

    wsprintfA(path, "%s.txt", g_cc_name);
    WriteWholeFile(path, g_cc_txt.data, g_cc_txt.length);
    wsprintfA(path, "%s.obj", g_cc_name);
    WriteWholeFile(path, g_cc_obj.data, g_cc_obj.length);
}

/* ==========================================================================
 * 0x004237f0 -- paint a straight line between two integer points by walking
 * THIRTY interpolated samples into a local vertex array and handing the whole
 * array to the point plotter at 0x004238a0 (which clips each sample against
 * the view bounds 0x008299ac..0x008299b8 and pokes a 16-bit pixel).
 *
 * The step is `(to - from) * (1/30)`, so the last sample is one step short of
 * `to`; both accumulators live on the x87 stack for the whole loop and each
 * sample is truncated with the game's __ftol helper at 0x00458930.  The
 * plotter's vertex stride is 0x10 and only the first two dwords are read.
 * ======================================================================== */
typedef struct PlotPoint { int x, y; int pad[2]; } PlotPoint;           /* 0x10 */
extern void Raster_PlotPoints(PlotPoint* pts, int count, int colour);   /* 0x004238a0 */

// FUNCTION: LEGOLAND 0x004237f0
void Raster_DrawLine(const Pos* from, const Pos* to, int colour)
{
    PlotPoint pts[30];
    float     x  = (float)from->x;
    float     y  = (float)from->y;
    float     sx = (float)(to->x - from->x) * 0.033333335f;
    float     sy = (float)(to->y - from->y) * 0.033333335f;
    int       i;

    for (i = 0; i < 30; i++) {
        pts[i].x = (int)x;
        pts[i].y = (int)y;
        x += sx;
        y += sy;
    }
    Raster_PlotPoints(pts, 30, colour);
}

/* ==========================================================================
 * 0x004237a0 -- draw every edge of a wireframe object in white (-1).  The
 * object is {..., int edges (+8), Vertex* vert (+0x10), Edge* edge (+0x14)};
 * the vertex stride is 0x14 and the edge is a pair of dword indices.  The
 * edge count is re-read from the object after each call, because the call
 * may alias it.
 * ======================================================================== */
typedef struct WireVert { Pos at; int pad[3]; } WireVert;              /* 0x14 */
typedef struct WireEdge { int a, b; } WireEdge;
typedef struct WireObj {
    int       pad00[2];
    int       edges;            /* +0x08 */
    int       pad0c;
    WireVert* vert;             /* +0x10 */
    WireEdge* edge;             /* +0x14 */
} WireObj;

// FUNCTION: LEGOLAND 0x004237a0
void Raster_DrawWireframe(WireObj* obj)
{
    int i;

    for (i = 0; i < obj->edges; i++)
        Raster_DrawLine(&obj->vert[obj->edge[i].a].at,
                        &obj->vert[obj->edge[i].b].at, -1);
}
