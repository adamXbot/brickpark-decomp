/* LEGOLAND — map cell flag/tile accessors. */
#include "legoland.h"

// FUNCTION: LEGOLAND 0x00461610
unsigned char Get_RFFlags(int x, int y)
{
    x >>= 8;
    y >>= 8;
    return g_map_rows[y][x].rf;
}

// FUNCTION: LEGOLAND 0x00461760
unsigned short Get_MapFlags(int x, int y)
{
    x >>= 8;
    y >>= 8;
    return g_map_rows[y][x].flags;
}

// FUNCTION: LEGOLAND 0x004617d0
unsigned short GetMapFlags(int x, int y)
{
    if (x < 0 || x > 0x100 || y < 0 || y > 0x100)
        return 0x40;
    return g_map_rows[y][x].flags;
}

// FUNCTION: LEGOLAND 0x00460540
void GetTileDimensions(int* out_w, int* out_h)
{
    Sprite* sprite = g_tile_sprites[g_default_tile];
    int h = sprite->h;
    *out_h = h;
    *out_w = h + h;
}
