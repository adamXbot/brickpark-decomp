/* LEGOLAND -- small sprite / render helper thunks.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Only struct field offsets and callee calling conventions are load-bearing;
 * type and field names are ours.
 */

/* ---- local types -------------------------------------------------------- */

/* A reference-counted sprite/image resource. The 16-bit refcount at +0x1c is
 * the field ReferenceSprite bumps and the sprite destructor at 0x00497bd0
 * decrements before tearing the record down. */
typedef struct SpriteRes {
    int            pad0;       /* +0x00 */
    struct Owner*  owner;      /* +0x04 owning object, torn down via its vtable */
    void*          data;       /* +0x08 image / ILF frame table */
    int            pad0c;      /* +0x0c */
    unsigned int   flags;      /* +0x10 bit5 = data is borrowed, bit15 = ILF */
    char           pad14[0x1c - 0x14];
    unsigned short refs;       /* +0x1c */
} SpriteRes;

/* The owner object's vtable; slot +0x08 is its destructor (__stdcall). */
typedef struct OwnerVtbl {
    char pad0[8];                                /* +0x00 */
    void(__stdcall* Destroy)(struct Owner* self); /* +0x08 */
} OwnerVtbl;

typedef struct Owner {
    OwnerVtbl* vtbl;           /* +0x00 */
} Owner;

/* A drawing surface / device object, reached through a COM-style vtable. Only
 * the two slots these helpers use are named. */
typedef struct SurfaceVtbl {
    char pad0[0x7c];                                     /* +0x00 */
    int(__stdcall* SetPalette)(struct Surface* self, void* palette); /* +0x7c */
    int(__stdcall* Release)(struct Surface* self, void* data);       /* +0x80 */
} SurfaceVtbl;

typedef struct Surface {
    SurfaceVtbl* vtbl;         /* +0x00 */
} Surface;

/* A sprite handle: the surface that owns the pixels plus the surface-private
 * data blob handed back to it on release. */
typedef struct SpriteHandle {
    char     pad0[0x0c];       /* +0x00 */
    void*    data;             /* +0x0c */
    Surface* surface;          /* +0x10 */
} SpriteHandle;

/* A "bloke" (person actor) record; its 3D person model hangs off +0x04. */
typedef struct Bloke {
    int   pad0;                /* +0x00 */
    void* person;              /* +0x04 */
} Bloke;

/* ---- globals ------------------------------------------------------------ */

/* The primary display surface (0x00668070), the palette last handed to it
 * (0x00668084) and the screen colour-depth selector (0x00668088). The selector
 * is 0 for the 8-bit palettised mode -- the only mode where a palette means
 * anything, hence the guard in ResendPalette. */
extern Surface* g_screen_surface;   /* 0x00668070 */
extern void*    g_screen_palette;   /* 0x00668084 */
extern int      g_screen_depth;     /* 0x00668088 */

/* ---- externals ---------------------------------------------------------- */

/* LEGOLAND/layers.c 0x00441ea0 -- the per-layer LLS lookup. */
extern void* GetLLSForLayer(void* obj, int layer);
/* 0x0047d4c0 -- stop an LLS animation (tolerates a null handle). */
extern void  LLSStop(void* lls);
/* 0x0043fe50 -- draw one 3D person model. */
extern void  Render3DPerson(void* person);
/* 0x0047bef0 -- free an ILF (indexed frame) table. */
extern void  LLIDB_FreeILFTable(void* table);
/* 0x00497510 -- drop a reference on an image record. */
extern void  KillImage(void* image);
/* 0x004975b0 -- unlink a sprite record from the global sprite list and free it. */
extern void  UnlinkSprite(SpriteRes* res);

/* -------------------------------------------------------------------------
 * 0x00497bb0 -- take a reference on a sprite resource.
 *
 *   mov eax,[esp+4] / test eax,eax / jne .. / or ax,0xffff / ret
 *
 * The null answer is built with `or ax,0FFFFh` rather than `mov eax,-1`, which
 * is what VC6 emits when only the low 16 bits of the return value matter --
 * i.e. the function returns a `short`.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497bb0
short ReferenceSprite(SpriteRes* res)
{
    if (!res)
        return -1;
    return (short)++res->refs;
}

/* -------------------------------------------------------------------------
 * 0x00497bd0 -- drop a reference on a sprite resource and, on the last one,
 * tear it down: destroy the owning object, free the pixel data (an ILF frame
 * table when bit 15 is set, otherwise a plain image -- unless bit 5 says the
 * data is borrowed), then unlink the record. Returns 1 only when it actually
 * destroyed the resource.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497bd0
int UnreferenceSprite(SpriteRes* res)
{
    Owner* owner;

    if (res) {
        res->refs--;
        if (res->refs == 0) {
            owner = res->owner;
            if (owner)
                owner->vtbl->Destroy(owner);

            if (!(res->flags & 0x20)) {
                if (res->flags & 0x8000)
                    LLIDB_FreeILFTable(res->data);
                else if (res->data)
                    KillImage(res->data);
            }

            UnlinkSprite(res);
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x00497dc0 -- hand a sprite's pixels back to the surface that owns them.
 * The surface method is __stdcall (the caller does no stack cleanup).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00497dc0
int ReleaseSprite(SpriteHandle* sprite)
{
    Surface* surface = sprite->surface;
    void*    data    = sprite->data;
    return surface->vtbl->Release(surface, data);
}

/* -------------------------------------------------------------------------
 * 0x00441f00 -- stop the animation playing on one layer of a render object.
 * VC6 defers both argument pops into a single `add esp,0Ch`.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00441f00
void StopLayerPlaying(void* obj, int layer)
{
    LLSStop(GetLLSForLayer(obj, layer));
}

/* -------------------------------------------------------------------------
 * 0x0044e670 -- push the current palette at the screen surface again. Only
 * meaningful in the 8-bit mode (g_screen_depth == 0).
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0044e670
void ResendPalette(void)
{
    Surface* surface;
    void*    palette;

    if (g_screen_depth == 0) {
        surface = g_screen_surface;
        palette = g_screen_palette;
        surface->vtbl->SetPalette(surface, palette);
    }
}

/* -------------------------------------------------------------------------
 * 0x0043ffb0 -- draw a bloke's 3D model, if it has one.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0043ffb0
void RenderBlokeIn3D(Bloke* bloke)
{
    void* person = bloke->person;
    if (person)
        Render3DPerson(person);
}

/* -------------------------------------------------------------------------
 * 0x00440010 -- the "do it now" entry point; a plain forwarder that VC6 keeps
 * as a real call.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x00440010
void IP_RenderBlokeIn3DNow(Bloke* bloke)
{
    RenderBlokeIn3D(bloke);
}
