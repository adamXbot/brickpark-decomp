/* LEGOLAND — small-leaf sweep (chunk 2). */
#include "legoland.h"
#ifdef LEGOLAND_PORTABLE
#define PrintBackground PrintBackground_vc6_body
#endif
#ifdef LEGOLAND_PORTABLE
#define ApplyDestrTileMap ApplyDestrTileMap_vc6_body
#endif
#ifdef LEGOLAND_PORTABLE
#define ApplyConsTileMap ApplyConsTileMap_vc6_body
#endif

/* ---- globals touched (deduped) ---- */
extern unsigned char g_dir_bit_table[];     /* 0x004b9550 */
extern int   g_host_gpu;                    /* 0x00667d74 */
extern int   g_pointer_index;               /* 0x0066814c */
extern void* g_current_pointer;             /* 0x00668148 */
extern void* g_pointer_table[];             /* 0x007fe9c0 */
extern void* g_video_surface;               /* 0x00668144 */
extern void* g_group_cb0;                   /* 0x006688a8 */
extern void* g_group_cb1;                   /* 0x006688ac */
extern void* g_group_cb2;                   /* 0x006688b0 */
extern int   g_icon_mode;                   /* 0x004bdd00 */
extern int   g_icon_value;                  /* 0x004bdd04 */
extern int   g_focussed_icon;               /* 0x006687d0 */
extern int   g_worker_a;                    /* 0x00668954 */
extern int   g_worker_b;                    /* 0x007fdff0 */
extern int   g_worker_c;                    /* 0x007fdffc */
extern int   g_info_active;                 /* 0x007fdfa0 */
extern int   g_info_resize;                 /* 0x007fdfa8 */
extern int   g_follow_target;               /* 0x007fdf98 */
extern int   g_advisor_state;               /* 0x007fe040 */
extern void  ResetInfoStruct(void);         /* 0x00471510 */

/* Stand-ins used only in the not-compared tails of functions whose first `ret`
 * is an early-out. The matcher (check.py / match.py) stops at the first `ret`,
 * so only the prologue + early-out is compared; these keep the compiler from
 * folding the early-out away (and, for ScreenToMapRef, force the local
 * footprint onto the stack so the 0xc frame matches). Never linked. */
extern int*  g_screenref_scratch;
extern int   AdvisorHelpBody(void);

/* a 20-byte footprint record copied wholesale */
typedef struct { int f[5]; } FootPrint;
extern FootPrint g_edit_cursor_footprint;   /* 0x007fffd4 */

/* a UI widget: flags at +0x04, check callback at +0x18, click callback at +0x1c */
typedef struct {
    int          pad0;               /* 0x00 */
    unsigned int flags;              /* 0x04 */
    char         pad8[0x18 - 8];     /* 0x08 */
    void*        checkfn;            /* 0x18 */
    void*        clickfn;            /* 0x1c */
} Widget;

// FUNCTION: LEGOLAND 0x0045c010
unsigned char Dir_To_Bit(unsigned char dir)
{
    return g_dir_bit_table[dir & 7];
}

// FUNCTION: LEGOLAND 0x0045efc0
void ApplyConsTileMap(void)
{
}

// FUNCTION: LEGOLAND 0x0045efd0
void ApplyDestrTileMap(void)
{
}

// FUNCTION: LEGOLAND 0x0045f440
void SetEditCursorFootPrint(FootPrint* src)
{
    g_edit_cursor_footprint = *src;
}

// FUNCTION: LEGOLAND 0x00461710
unsigned short Get_UserFlags(int x, int y)
{
    x >>= 8;
    y >>= 8;
    return g_map_rows[y][x].uflags;
}

// FUNCTION: LEGOLAND 0x00461730
void Set_UserFlags(int x, int y, unsigned short value)
{
    x >>= 8;
    y >>= 8;
    g_map_rows[y][x].uflags = value;
}

// FUNCTION: LEGOLAND 0x004636f0
int InstallDirectDraw(void)
{
    return 0;
}

// FUNCTION: LEGOLAND 0x00463850
int SetPointer(int idx)
{
    int old = g_pointer_index;
    g_current_pointer = g_pointer_table[idx];
    g_pointer_index = idx;
    return old;
}

// FUNCTION: LEGOLAND 0x00464360
void PrintBackground(void)
{
}

// FUNCTION: LEGOLAND 0x0046d740
void SetNewGroup_Callbacks(void* a, void* b, void* c)
{
    g_group_cb0 = a;
    g_group_cb1 = b;
    g_group_cb2 = c;
}

// FUNCTION: LEGOLAND 0x0046fd00
void SetCheckFunc(Widget* w, void* fn)
{
    w->checkfn = fn;
    w->flags |= 4;
}

// FUNCTION: LEGOLAND 0x0046fd20
void SetClickFunc(Widget* w, void* fn)
{
    w->clickfn = fn;
    w->flags |= 4;
}

// FUNCTION: LEGOLAND 0x004700a0
void UpdateFocussedIconPtr(void)
{
    g_focussed_icon = (g_icon_mode == 2) ? g_icon_value : 0;
}

// FUNCTION: LEGOLAND 0x00470930
void ResetMoveAWorkerStruct(void)
{
    g_worker_a = 0;
    g_worker_b = 0;
    g_worker_c = 0;
}

// FUNCTION: LEGOLAND 0x00471550
void PopInfoSizeMayChange(void)
{
    if (g_info_active)
        g_info_resize = 1;
}

// FUNCTION: LEGOLAND 0x00471570
void StopFollowingBloke(void)
{
    if (g_follow_target)
        ResetInfoStruct();
}

// FUNCTION: LEGOLAND 0x004761c0
void InitRAndDCheckBox(void)
{
}

// FUNCTION: LEGOLAND 0x004761d0
void Unload_RAndDCheckBox(void)
{
}

// FUNCTION: LEGOLAND 0x004761f0
void RenderRAndDCheckBox(void)
{
}

#ifdef LEGOLAND_PORTABLE
/* ApplyConsTileMap is called with 2 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
/* PORT-M11: the second argument is the packed map square BY VALUE at the only
 * call site (objmap2.c:207/491, `BPos bp`), not an int.  Nothing reads it here,
 * so this was harmless -- but a scalar parameter against an indirect aggregate
 * is exactly the shape that IS harmful elsewhere, and the sweep cannot tell
 * the two apart, so the forwarder carries the caller's own shape. */
typedef struct LLSquare { unsigned char x, y; } LLSquare;   /* objmap2.c's BPos */
#undef ApplyConsTileMap
void ApplyConsTileMap(void* ll_a1, LLSquare ll_a2) { (void)ll_a1; (void)ll_a2; ApplyConsTileMap_vc6_body(); }
#endif

#ifdef LEGOLAND_PORTABLE
/* ApplyDestrTileMap is called with 2 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
/* PORT-M11: as ApplyConsTileMap above -- objmap2.c:208/1000 passes `BPos bp`
 * by value, and StandardRemoveObject is the caller. */
#undef ApplyDestrTileMap
void ApplyDestrTileMap(void* ll_a1, LLSquare ll_a2) { (void)ll_a1; (void)ll_a2; ApplyDestrTileMap_vc6_body(); }
#endif

#ifdef LEGOLAND_PORTABLE
/* PrintBackground is called with 2 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef PrintBackground
void PrintBackground(int ll_a1, int ll_a2) { (void)ll_a1; (void)ll_a2; PrintBackground_vc6_body(); }
#endif
