/* LEGOLAND — map scroll position accessors. The scroll origin is stored in
 * 8.8 fixed point (0x667cb4 / 0x667cb8); the getters return whole tiles. */
#include "legoland.h"

extern int g_scroll_x;   /* 0x00667cb4  (8.8 fixed point) */
extern int g_scroll_y;   /* 0x00667cb8 */

// FUNCTION: LEGOLAND 0x004615f0
int Get_XScroll(void)
{
    return g_scroll_x >> 8;
}

// FUNCTION: LEGOLAND 0x00461600
int Get_YScroll(void)
{
    return g_scroll_y >> 8;
}
