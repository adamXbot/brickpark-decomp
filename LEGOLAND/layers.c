/* LEGOLAND — per-cell layer / render-object accessors. */
#include "legoland.h"

// FUNCTION: LEGOLAND 0x00441e80
void* GetLLSForSprite(SpriteObj* sprite)
{
    if (sprite)
        return *(sprite->lls_holder);
    return 0;
}

// FUNCTION: LEGOLAND 0x00441ea0
void* GetLLSForLayer(RenderObj* obj, int layer)
{
    SpriteObj* sprite = (SpriteObj*)obj->layers->sprites[layer];
    if (!sprite)
        return sprite;
    return *(sprite->lls_holder);
}

// FUNCTION: LEGOLAND 0x00441ec0
void* GetSpriteForLayer(RenderObj* obj, int layer)
{
    return obj->layers->sprites[layer];
}

// FUNCTION: LEGOLAND 0x00441ee0
Offset GetRenderOffsetForLayer(RenderObj* obj, int layer)
{
    Layers* layers = obj->layers;
    Offset result;
    result.ox = layers->render_ox[layer];
    result.oy = layers->render_oy[layer];
    return result;
}
