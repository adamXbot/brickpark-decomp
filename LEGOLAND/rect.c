/* LEGOLAND — rectangle-list area. */
#include "legoland.h"

// FUNCTION: LEGOLAND 0x00480960
int GetRectArea(Rect* rect)
{
    int area = 0;
    while (rect) {
        area += (rect->bottom - rect->top + 1) * (rect->right - rect->left + 1);
        rect = rect->next;
    }
    return area;
}
