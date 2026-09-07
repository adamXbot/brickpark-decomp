/* LEGOLAND -- Codex-F: five UI/system stubs.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/codex-f.md.
 */
typedef struct MarkedTile { unsigned short key, count; } MarkedTile;

extern int g_help_face_state;              /* 0x006687a4 */
extern int g_imt_theme;                    /* 0x0079a6ac */
extern int g_brick_lock;                   /* 0x004b90fc */
extern MarkedTile g_marked_tiles[128];     /* 0x007cb3e0 */
extern int g_clock_held;                   /* 0x0079a894 */
extern int g_clock_base;                   /* 0x0079a898 */
extern int g_clock_aux_held;               /* 0x0079a89c */
extern int g_clock_aux_base;               /* 0x0079a8a0 */
extern int g_detail;                       /* 0x008119a4 */

extern void SetTheme(int theme);           /* 0x00492ce0 */
extern int GetTicks(void);                 /* 0x00499450 */

/* Twin of SetHelpFaceTalking (tinystubs.c:0x0046d3a0) which stores 4. */
// FUNCTION: LEGOLAND 0x0046d390
void SetHelpFaceState5(void)
{
    g_help_face_state = 5;
}

/* Re-applies the currently playing IMT theme (caller-side name RestartMusic). */
// FUNCTION: LEGOLAND 0x00492da0
void RestartMusic(void)
{
    SetTheme(g_imt_theme);
}

/* Former sub_457870. Stores (arg == 0) at g_brick_lock so BricksAreLimited
 * (tinystubs.c) sees lock==0 as limited. Free-play passes 0 (unlimited). */
// FUNCTION: LEGOLAND 0x00457870
void SetBrickLimit(int limited)
{
    g_brick_lock = (limited == 0);
}

/* Former sub_489ee0. Sentinel-clears g_marked_tiles keys to 0xffff (pathmisc
 * MarkObjectTiles empty slot). Cursor compared as signed ints → jl, not jb. */
// FUNCTION: LEGOLAND 0x00489ee0
void ClearMarkedTiles(void)
{
    int p = (int)g_marked_tiles;
    do {
        *(unsigned short*)p = 0xffff;
        p += 4;
    } while (p < (int)(g_marked_tiles + 128));
}

// FUNCTION: LEGOLAND 0x00499410
void ResetGameClock(void)
{
    int now = GetTicks();
    g_clock_held = now;
    g_clock_base = now;
    now = g_detail;
    g_clock_aux_held = now;
    g_clock_aux_base = now;
}
