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

// FUNCTION: LEGOLAND 0x00461780
void SetMapTile(int x, int y, unsigned short value)
{
    if (x < 0 || x > g_map->width || y < 0 || y > g_map->height)
        return;
    g_map_rows[y][x].tile = value;
}

// FUNCTION: LEGOLAND 0x00461810
void SetMapFlags(int x, int y, unsigned short value)
{
    if (x < 0 || x > 0x100 || y < 0 || y > 0x100)
        return;
    g_map_rows[y][x].flags = value;
}

// FUNCTION: LEGOLAND 0x004616e0
void Set_RFFlags(int x, int y, unsigned char value)
{
    x >>= 8;
    y >>= 8;
    g_map_rows[y][x].rf = value;
}

// FUNCTION: LEGOLAND 0x00461630
unsigned char GetCurrentRFFlags(int x, int y)
{
    unsigned char rf;
    int cx, cy;
    Cell* cell;
    unsigned short tile;

    if (x < 0 || x >= (g_map->width << 8) || y < 0 || y >= (g_map->height << 8))
        return 2;
    rf = Get_RFFlags(x, y);
    if (!(rf & 2))
        return rf;
    if (!(rf & 1))
        return rf;

    cx = x >> 8;
    cy = y >> 8;
    if (cx < 0 || cx >= g_map->width || cy < 0 || cy >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[cy][cx];
    tile = *(unsigned short*)((char*)cell + 8);
    {
        RFHandler handler = g_tile_info[tile].elem->handler;
        if (!handler)
            return 2;
        return handler(x, y);
    }
}
