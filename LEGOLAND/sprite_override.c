/* LEGOLAND — global sprite render overrides.
 *
 * A one-shot palette/frame override consulted by the sprite blitter: set before
 * a draw to force a specific palette (0x6681e8) or animation frame (0x4b9ca8,
 * -1 = "no override"), cleared afterwards. */
#include "legoland.h"

extern void* g_override_palette;   /* 0x006681e8  (0 = none) */
extern int   g_override_frame;     /* 0x004b9ca8  (-1 = none) */

// FUNCTION: LEGOLAND 0x00464400
void SetOverridePalette(void* pal)
{
    g_override_palette = pal;
}

// FUNCTION: LEGOLAND 0x00464410
void* GetOverridePalette(void)
{
    return g_override_palette;
}

// FUNCTION: LEGOLAND 0x00464420
void SetOverrideFrame(int frame)
{
    g_override_frame = frame;
}

// FUNCTION: LEGOLAND 0x00464430
int GetOverrideFrame(void)
{
    return g_override_frame;
}

// FUNCTION: LEGOLAND 0x00464440
void ClearOverrideFrame(void)
{
    g_override_frame = -1;
}

// FUNCTION: LEGOLAND 0x00464450
void ClearOverridePalette(void)
{
    g_override_palette = 0;
}

// FUNCTION: LEGOLAND 0x00464460
void ClearSpriteOverrides(void)
{
    g_override_frame = -1;
    g_override_palette = 0;
}
