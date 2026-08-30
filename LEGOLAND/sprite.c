/* LEGOLAND — sprite accessors. */
#include "legoland.h"

// FUNCTION: LEGOLAND 0x004015c0
void GetSpriteSize(Sprite* sprite, unsigned short* w, unsigned short* h)
{
    *w = sprite->w;
    *h = sprite->h;
}
