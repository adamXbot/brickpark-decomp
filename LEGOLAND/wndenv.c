/* LEGOLAND — window environment: the app's HWND / HINSTANCE handles. */
#include "legoland.h"

extern void* g_hwnd;        /* 0x00669210 */
extern void* g_hinstance;   /* 0x00669208 */

// FUNCTION: LEGOLAND 0x0047fe40
void* WNDENV_GethInstance(void)
{
    return g_hinstance;
}

// FUNCTION: LEGOLAND 0x0047fe50
void WNDENV_Sethwnd(void* hwnd)
{
    g_hwnd = hwnd;
}

// FUNCTION: LEGOLAND 0x0047fe60
void* WNDENV_Gethwnd(void)
{
    return g_hwnd;
}
