/* LEGOLAND -- save chunks, texture records, model teardown and narration,
 * lane w18b.  Reconstructed for VC6 SP3 /O2 /Gy /Gd.
 *
 * WHAT THIS FILE RECOVERS
 * =======================
 * Eighteen leaf helpers that the matched save, render and audio files call.
 * Five mechanisms come out of them:
 *
 *  * THE SAVE-FILE STRING AND SLOT FRAMING.  SaveScriptString is the exact
 *    twin of savegame2.c's LoadScriptString: a u32 length (-1 for a NULL
 *    string) followed by exactly `len` bytes with NO terminator -- the NUL is
 *    added by the loader, which is why the loader allocates len+1.  A zero
 *    length writes the header and nothing else.  LoadBuildSlots is the twin of
 *    savegame2.c's SaveBuildSlots: a count then that many 12-byte records, and
 *    each record's object field arrives as an e-list INDEX which is turned
 *    back into a pointer through GeteListPtr(i)->data.  Every slot from the
 *    count up to 256 is then cleared, so a short file leaves no stale objects.
 *
 *  * THE ICON-STATE CHUNK.  SaveIconStateChunk writes FOUR DWORDS (not, as
 *    savechunks.c's format note guesses, sixteen bytes), one per control icon
 *    in the 0x007fdd70 block: 1 when the icon is enabled, 0 when its flag
 *    0x400 (disabled) is set.  So the saved value is the INVERSE of the flag.
 *
 *  * THE TEXTURE SLOT TABLE.  g_textures (0x00798190) is 256 slots of a
 *    0x2c-byte record built by 0x004437d0 from a decoded source image.
 *    RegisterTextureImage mallocs and files one; UnInitRasterTables frees all
 *    256 -- each record's +0x08 and +0x0c blocks and then the record.  It
 *    re-reads g_textures[i] after every free because a call kills the CSE,
 *    which is exactly what the original does.
 *
 *  * THE SHADING-RAMP CACHE.  rin.c's `UnInit3DPrintList` (0x00486250) is
 *    really the teardown of tri3d.c's ramp cache at 0x00797e6c: it walks the
 *    ShadeNode list, releases each ramp through FreeShadedColour and frees the
 *    node.  Name kept as the caller declares it, with the real job noted.
 *
 *  * THE NARRATION STREAM.  ResetNarrationStreamState clears the 0x50-byte
 *    decode-queue block at 0x0079a7e4 plus nine scattered scalars;
 *    StopNarrationPlayback is the guarded stop: it does nothing in states 0
 *    (no stream) and 1 (already stopped), otherwise it calls the DirectSound
 *    buffer's Stop (vtable +0x48), resets the stream state, rewinds the source
 *    and parks the state at 1.
 *
 * DIVERGENCES FROM THE CALLERS' EXTERNS (deliberate, each a caller-side
 * codegen lever; nothing shared was changed):
 *   - rin.c declares UnInitModelTextures / UnInit3DPrintList / UnInitRasterTables
 *     `void`; the first really returns 0.
 *   - audio4.c declares StopNarrationPlayback `void`; it returns 0/1.
 *   - sprite2.c declares UnregisterDetailImage `void`; it returns 0/1.
 *   - memdb.c declares LLIDB_UnLoadTSMData `int`; it falls off its end (no
 *     value), so it is spelled `void` here.
 *   - savegame.c calls 0x00450b10 `LoadBlock11`; renamed LoadBuildSlots the
 *     way savegame2.c renamed its save-side twin.
 *
 * CODEGEN NOTES (measured on these bodies):
 *   - ResetNarrationStreamState: the three stores at 0x0079a7d8..0x0079a7e0
 *     come BEFORE the memset in the source. VC6 hoists the intrinsic's
 *     `rep stosd` above them anyway, but the zero those three stores need
 *     already exists, so it materialises `xor edx,edx` in the fill's
 *     scheduling slot and every one of the nine plain stores then uses edx.
 *     With the memset first, VC6 forwards the fill's own zero out of eax and
 *     each store shrinks to the 5-byte `mov moffs32,eax` form -- 63 bytes
 *     against the original's 73. The source order is also address order.
 *   - LLIDB_UnLoadTSMData: the walk is a SUBSCRIPT over one index, not a
 *     pointer. Both `table[i].elem` and `table[i].kind` as subscripts of the
 *     same `i` leave VC6 with TWO strength-reduced cursors and a `mov eax,esi`
 *     copy in the latch, because the CALL between the two accesses stops the
 *     induction variables being coalesced; a plain `p++` pointer walk emits
 *     one cursor and is two instructions short. (Refines the recorded "spell
 *     two lockstep cursors the SAME way to eliminate one" rule: a call
 *     between the accesses defeats the elimination.) Two explicit lockstep
 *     pointers reach the same code.
 *   - CollectUsedTSFTables: `void** dst = out;` -- an eager root copy of the
 *     out parameter. It moves the load between `push esi` and `push edi` and
 *     swaps the esi/edi ranking of the output cursor and the tile-info
 *     cursor; every other spelling (pointer walks, `out[n]`, a `continue`
 *     chain, an `&&` chain, a free volatile read) floors at 7 or worse.
 *   - SkipStrings: the walked pointer must be a LOCAL copy of the parameter.
 *     Stepping the parameter itself leaves the zero-trip arm returning
 *     `[esp+8]` where the original returns edx, and delays the entry load
 *     past `push esi` (6 mismatches, 2 bytes).
 *   - UnregisterDetailImage: the failure arm is the EXILED one --
 *     `if (slot) { ...; return 1; } return 0;` gives `je <end>` with a
 *     materialised `xor eax,eax` at the end; `if (!slot) return 0;` emits the
 *     bare `ret` VC6 knows is enough and is one byte short.
 *   - GetObjRiderN: `int i = 0;` as an INITIALISER after the first call puts
 *     `xor esi,esi` before `test eax,eax`, where the zero register then
 *     doubles as the operand of `cmp word ptr [edi+0x2e],si`. Initialising it
 *     in the `for` inside the guard emits the `xor` after the branch.
 *   - SaveScriptString and LoadBuildSlots were exact first try; the two
 *     trailing SaveGameWrite arms cross-jump into one call site with no
 *     construct, and `len` is homed in the (root-copied, hence dead) `s`
 *     argument slot.
 * ========================================================================= */

unsigned int strlen(const char* s);
void*        memset(void* p, int c, unsigned int n);
#pragma intrinsic(strlen, memset)

typedef struct IDSBuffer IDSBuffer;
typedef struct IDSBufferVtbl {
    unsigned char pad00[0x48];
    long (__stdcall *Stop)(IDSBuffer*);        /* +0x48 */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

/* data3.c's decoded source image. */
typedef struct SrcImage {
    int   pad00[2];
    short w;                       /* +0x08 */
    short h;                       /* +0x0a */
} SrcImage;

/* One entry of g_textures: 0x2c bytes with two owned blocks. */
typedef struct TexRec {
    unsigned char pad00[8];
    void*         bits;            /* +0x08 */
    void*         rows;            /* +0x0c */
    unsigned char pad10[0x2c - 0x10];
} TexRec;

/* sprite2.c's SpriteRec, only as far as this file reads it. */
typedef struct SpriteRec {
    unsigned char pad00[0x10];
    unsigned int  flags;           /* +0x10  bit 5 = needs recreating */
} SpriteRec;

typedef struct ImageRec ImageRec;

/* tri3d.c's shading-ramp cache node. */
typedef struct Shade Shade;
typedef struct ShadeNode {
    struct ShadeNode* next;        /* +0x00 */
    unsigned char     rgb[4];      /* +0x04 */
    Shade*            shade;       /* +0x08 */
} ShadeNode;

/* iconui.c's Icon; only the flag word matters here. */
typedef struct Icon {
    unsigned char pad00[0x34];
    unsigned int  flags;           /* +0x34  bit 0x400 = disabled */
} Icon;

/* legoland.h's LLIDB element; the parsed table hangs off +0x0c. */
typedef struct LLElem {
    char*         name;
    char*         image;
    unsigned int  type_flags;
    void*         data;            /* +0x0c */
    unsigned int  refcount;
} LLElem;

/* One entry of a .TSM element's table: the referenced element and a kind word
 * whose -1 terminates the list. */
typedef struct TsmEntry {
    LLElem* elem;                  /* +0x00 */
    int     kind;                  /* +0x04  -1 = end of table */
} TsmEntry;

/* renderview.c's parallel tile-info table (stride 8). */
typedef struct TileInfo {
    void*        set;              /* +0x00  the .TSF table this tile came from */
    unsigned int code;             /* +0x04 */
} TileInfo;

/* softblit.c's goal record; the goal list and the script-event list share a
 * record type, which is why goals are released with FreeScriptEvent. */
typedef struct Goal {
    struct Goal* next;             /* +0x00 */
    void*        target;           /* +0x04 */
    int          pad08;
    int          code;             /* +0x0c */
} Goal;

/* rin.c's rider slot occupant, and the item whose riders are walked. */
typedef struct Rider Rider;
typedef struct RiderItem {
    unsigned char pad00[0x2e];
    short         count;           /* +0x2e  number of rider slots */
} RiderItem;

/* savegame2.c's construction slot. */
typedef struct Obj Obj;
typedef struct BuildSlot {
    Obj*  obj;                     /* +0x00  an e-list index in the file */
    short key;                     /* +0x04 */
    short pad6;
    int   timer;                   /* +0x08 */
} BuildSlot;                       /* 0x0c */

/* ---- callees ------------------------------------------------------------ */

/* Statically-linked CRT (>= 0x0049e000): NOT decompilation targets. */
extern void* MemAlloc(unsigned int n);                        /* 0x0049e4ff */
extern void  MemFree(void* p);                                /* 0x0049e4d0 */

extern void  ResetNarrationStreamState(void);                 /* 0x00498870 */
extern void  RewindNarrationSource(void);                     /* 0x00498120 */
extern void  BuildTextureRecord(SrcImage* img, TexRec* out);  /* 0x004437d0 */
extern int   RecreateSprite(SpriteRec* s);                    /* 0x00466640 */
extern int   MakeSprite(SpriteRec* s);                        /* 0x00497b70 */
extern void  FreeShadedColour(Shade* s);                      /* 0x00486220 */
extern ImageRec** FindDetailImageSlot(ImageRec* p);           /* 0x00496ff0 */
extern int   SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int   LLIDB_UnLoadData(LLElem* e);                     /* 0x0047d450 */
extern SrcImage* CreateSourceImage(const char* path, int fmt); /* 0x00497280 */
extern int   ConvertSourceImage(SrcImage* img);               /* 0x004434d0 */
extern void  KillImage(SrcImage* img);                        /* 0x00497510 */
extern void  FreeScriptEvent(Goal* g);                        /* 0x00468940 */
extern Rider* ObjFirstRider(RiderItem* item, void* inst);     /* 0x00441870 */
extern Rider* ObjNextRider(RiderItem* item, void* inst);      /* 0x00441890 */
extern LLElem* GeteListPtr(int id);                           /* 0x0047d8c0 */

/* ---- globals ------------------------------------------------------------ */

extern int        g_sfx_master_db;        /* 0x007988a0 */
extern int        g_speech_state;         /* 0x0079a84c */
extern IDSBuffer* g_speech_buffer;        /* 0x0079a848 */
extern int        g_narr_a;               /* 0x0079a7d8 */
extern int        g_narr_b;               /* 0x0079a7dc */
extern int        g_narr_c;               /* 0x0079a7e0 */
extern int        g_narr_queue[20];       /* 0x0079a7e4  the 0x50-byte block */
extern int        g_narr_d;               /* 0x0079a834 */
extern int        g_narr_e;               /* 0x0079a838 */
extern int        g_narr_f;               /* 0x0079a840 */
extern int        g_narr_g;               /* 0x0079a844 */
extern int        g_speech_chunk_pos;     /* 0x007caca4 */
extern int        g_speech_header_flag;   /* 0x007aac24 */
extern TexRec*    g_textures[256];        /* 0x00798190 */
extern ShadeNode* g_shade_head;           /* 0x00797e6c */
extern ImageRec** g_detail_images;        /* 0x0079a7c4 */
extern int        g_detail_images_count;  /* 0x0079a7cc */
extern Icon*      g_ctrl_icons[4];        /* 0x007fdd70 */
extern int        g_icon_state[4];        /* 0x00668e20 */
extern void**     g_outfitA_tab0;         /* 0x00655a38 */
extern void**     g_outfitB_tab0;         /* 0x0062fea8 */
extern void**     g_outfitA_tab1;         /* 0x0062fef8 */
extern void**     g_outfitB_tab1;         /* 0x0064cd8c */
extern TileInfo   g_tile_info[2048];      /* 0x00801f40 */
extern void*      g_tile_sprites[2048];   /* 0x00805f60 */
extern Goal*      g_goal_list;            /* 0x00668728 */
extern BuildSlot  g_build_slots[256];     /* 0x006664f8 */

/* -------------------------------------------------------------------------
 * Squared screen distance to a DirectSound attenuation. Anything louder than
 * the master level or quieter than -3000 dB collapses to DSBVOLUME_MIN.
 * ------------------------------------------------------------------------- */
// FUNCTION: LEGOLAND 0x00496540
int VolumeFromDistSq(int d2)
{
    int vol = g_sfx_master_db - d2 / 60;
    if (vol < -3000 || vol > 0)
        vol = -10000;
    return vol;
}

/* Stop the narration buffer; states 0 and 1 are already stopped. */
// FUNCTION: LEGOLAND 0x004988c0
int StopNarrationPlayback(void)
{
    if (g_speech_state == 0 || g_speech_state == 1)
        return 0;
    g_speech_buffer->lpVtbl->Stop(g_speech_buffer);
    ResetNarrationStreamState();
    RewindNarrationSource();
    g_speech_state = 1;
    return 1;
}

/* Build a texture record from a decoded image and file it in a slot. */
// FUNCTION: LEGOLAND 0x00488670
void RegisterTextureImage(SrcImage* img, int slot)
{
    TexRec* t = (TexRec*)MemAlloc(0x2c);
    if (t) {
        BuildTextureRecord(img, t);
        g_textures[slot] = t;
    }
}

/* Make a sprite paintable, recreating its surface first when flagged. */
// FUNCTION: LEGOLAND 0x00499500
int MakeSpriteDrawable(SpriteRec* s)
{
    if ((s->flags & 0x20) && !RecreateSprite(s))
        return 0;
    MakeSprite(s);
    return 1;
}

/* Clear the narration decode queue and every stream cursor. */
// FUNCTION: LEGOLAND 0x00498870
void ResetNarrationStreamState(void)
{
    g_narr_a = 0;
    g_narr_b = 0;
    g_narr_c = 0;
    memset(g_narr_queue, 0, sizeof(g_narr_queue));
    g_narr_d = 0;
    g_narr_e = 0;
    g_narr_f = 0;
    g_narr_g = 0;
    g_speech_chunk_pos = 0;
    g_speech_header_flag = 0;
}

/* Free the whole shading-ramp cache (rin.c's name for it). */
// FUNCTION: LEGOLAND 0x00486250
void UnInit3DPrintList(void)
{
    ShadeNode* n = g_shade_head;
    while (n) {
        ShadeNode* next = n->next;
        FreeShadedColour(n->shade);
        MemFree(n);
        n = next;
    }
}

/* Drop an image from the detail list; the last one frees the list itself. */
// FUNCTION: LEGOLAND 0x00497020
int UnregisterDetailImage(ImageRec* p)
{
    ImageRec** slot = FindDetailImageSlot(p);
    if (slot) {
        *slot = 0;
        if (--g_detail_images_count == 0) {
            MemFree(g_detail_images);
            g_detail_images = 0;
        }
        return 1;
    }
    return 0;
}

/* One dword per control icon: 1 when enabled, 0 when flag 0x400 is set. */
// FUNCTION: LEGOLAND 0x00474920
int SaveIconStateChunk(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (g_ctrl_icons[i]->flags & 0x400)
            g_icon_state[i] = 0;
        else
            g_icon_state[i] = 1;
    }
    return SaveGameWrite(g_icon_state, 0x10) != 0;
}

/* Step forward over n NUL-terminated strings. */
// FUNCTION: LEGOLAND 0x004428c0
char* SkipStrings(char* start, int n)
{
    char* p = start;
    int   i;
    for (i = 0; i < n; i++)
        p += strlen(p) + 1;
    return p;
}

/* Free all 256 texture records and their two owned blocks. */
// FUNCTION: LEGOLAND 0x004886a0
void UnInitRasterTables(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        if (g_textures[i]) {
            MemFree(g_textures[i]->bits);
            MemFree(g_textures[i]->rows);
            MemFree(g_textures[i]);
            g_textures[i] = 0;
        }
    }
}

/* Release every element a .TSM table references, then the table. */
// FUNCTION: LEGOLAND 0x0047cf80
void LLIDB_UnLoadTSMData(LLElem* e)
{
    TsmEntry* table = (TsmEntry*)e->data;
    int       i;
    if (table->kind != -1) {
        i = 0;
        do {
            LLIDB_UnLoadData(table[i].elem);
            i++;
        } while (table[i].kind != -1);
    }
    MemFree(table);
}

/* Decode a texture bitmap and convert it; a failed conversion kills it. */
// FUNCTION: LEGOLAND 0x004436d0
SrcImage* LoadTextureImage(const char* path, int fmt)
{
    SrcImage* img = CreateSourceImage(path, fmt);
    if (!img)
        return 0;
    if (!ConvertSourceImage(img)) {
        KillImage(img);
        return 0;
    }
    return img;
}

/* Free the four outfit part tables. They are not nulled, and the return
 * value is a constant 0 the caller ignores. */
// FUNCTION: LEGOLAND 0x00442c70
int UnInitModelTextures(void)
{
    if (g_outfitA_tab0)
        MemFree(g_outfitA_tab0);
    if (g_outfitB_tab0)
        MemFree(g_outfitB_tab0);
    if (g_outfitA_tab1)
        MemFree(g_outfitA_tab1);
    if (g_outfitB_tab1)
        MemFree(g_outfitB_tab1);
    return 0;
}

/* Harvest the distinct .TSF tables the loaded tile set actually uses.
 * Only runs of EQUAL neighbours are collapsed, so a table that reappears
 * later in the tile list is emitted twice -- the save code tolerates it. */
// FUNCTION: LEGOLAND 0x0045aa50
int CollectUsedTSFTables(void** out)
{
    void** dst = out;
    int    n = 0;
    void*  last = 0;
    int    i;

    for (i = 0; i < 2048; i++) {
        if (g_tile_sprites[i] != (void*)-1) {
            void* set = g_tile_info[i].set;
            if (set && set != last) {
                *dst = set;
                n++;
                last = set;
                dst++;
            }
        }
    }
    return n;
}

/* Unlink and free every goal carrying one code. */
// FUNCTION: LEGOLAND 0x004693b0
void RemoveGoals(int code)
{
    Goal* prev = 0;
    Goal* g = g_goal_list;

    while (g) {
        Goal* next = g->next;
        if (g->code == code) {
            if (prev)
                prev->next = next;
            else
                g_goal_list = next;
            FreeScriptEvent(g);
        } else {
            prev = g;
        }
        g = next;
    }
}

/* The n'th rider of an object, by walking the shared rider cursor. */
// FUNCTION: LEGOLAND 0x004418c0
Rider* GetObjRiderN(int n, RiderItem* item, void* inst)
{
    Rider* rider = ObjFirstRider(item, inst);
    int    i = 0;

    if (rider) {
        for (; i < item->count; i++) {
            if (i == n)
                return rider;
            rider = ObjNextRider(item, inst);
        }
    }
    return 0;
}

/* u32 length (-1 for NULL) then exactly that many bytes, no terminator. */
// FUNCTION: LEGOLAND 0x0046c620
int SaveScriptString(const char* s)
{
    int len;

    if (s) {
        len = strlen(s);
        if (!SaveGameWrite(&len, 4))
            return 0;
        if (len != 0) {
            if (!SaveGameWrite(s, len))
                return 0;
        }
    } else {
        len = -1;
        if (!SaveGameWrite(&len, 4))
            return 0;
    }
    return 1;
}

/* BLK 11: the construction slots. Neither read is checked. */
// FUNCTION: LEGOLAND 0x00450b10
void LoadBuildSlots(void)
{
    int n = 0;
    int i;

    SaveGameRead(&n, 4);
    for (i = 0; i < n; i++) {
        SaveGameRead(&g_build_slots[i], 0xc);
        g_build_slots[i].obj = (Obj*)GeteListPtr((int)g_build_slots[i].obj)->data;
    }
    for (; i < 256; i++)
        g_build_slots[i].obj = 0;
}
