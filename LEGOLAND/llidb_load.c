/* LEGOLAND — LLIDB per-type asset loaders (parsers dispatched by LoadData). */
#include "legoland.h"

int   sprintf(char*, const char*, ...);
void* malloc(unsigned int);
void* RES_OpenFile(const char*);
int   RES_ReadFile(void* file, void* buf, int len);   /* returns bytes read */
void  RES_CloseFile(void* file);
int   LLIDB_FindElement(char* name, LLElem** out_elem, unsigned int* out_idx);
void* LLIDB_LoadData(LLElem* elem);

/* .TSM parsed output: an array of {entry, loaded} records, terminated {-1,-1}. */
typedef struct TsmRec {
    LLElem* entry;    /* +0x00 the tile-set element */
    void*   loaded;   /* +0x04 its LoadData() result */
} TsmRec;

// FUNCTION: LEGOLAND 0x0047ce40
void* LLIDB_LoadTSMData(LLElem* elem)
{
    char    path[512];
    char    name[512];
    int     count;
    int     len;
    int     i;
    LLElem* found;
    void*   file;
    TsmRec* recs;

    sprintf(path, "TileData\\%s", elem->image);
    file = RES_OpenFile(path);
    if (file) {
        RES_ReadFile(file, &count, 4);
        recs = (TsmRec*)malloc((count + 1) * sizeof(TsmRec));
        if (recs) {
            RES_ReadFile(file, &len, 4);
            RES_ReadFile(file, name, len);
            for (i = 0; i < count; i++) {
                RES_ReadFile(file, &len, 4);
                RES_ReadFile(file, name, len);
                name[len] = 0;
                LLIDB_FindElement(name, &found, 0);
                recs[i].entry = found;
                recs[i].loaded = LLIDB_LoadData(found);
            }
            recs[i].entry = (LLElem*)-1;
            recs[i].loaded = (void*)-1;
            RES_CloseFile(file);
            elem->data = recs;
            elem->type_flags |= 1;
            return recs;
        }
        RES_CloseFile(file);
    }
    return 0;
}

void* AllocTileSpace(void* desc, int n, int* out_base);
void* LoadSprite(const char* name, int flag);
void  LLSPlay(void* frames, void* hdr);

/* .TSF parsed descriptor (36 bytes). base_slot is the AllocTileSpace index the
 * tile codes are added to; sprites[] holds the loaded .lls per tile. */
typedef struct TsfData {
    unsigned int  base_slot;  /* +0x00 (AllocTileSpace out & 0xffff) */
    unsigned int  n_tiles;    /* +0x04 */
    void**        sprites;    /* +0x08 */
    unsigned int* codes;      /* +0x0c */
    unsigned int* second;     /* +0x10 */
    void*         parent;     /* +0x14 */
    int           f18;        /* +0x18 */
    int           f1c;        /* +0x1c */
    int           f20;        /* +0x20 */
} TsfData;

/* loaded sprite anim probe: hdr@+8; hdr->type@+0x14; hdr->frames@+0; frames->count@+0x10. */
typedef struct LLSAnim { char pad[0x10]; short count; } LLSAnim;
typedef struct LLSHdr  { LLSAnim* frames; char pad[0x10]; int type; } LLSHdr;
typedef struct LLSprite { char pad[8]; LLSHdr* hdr; } LLSprite;

// WIP-FUNCTION: LEGOLAND 0x0047cba0  (85.7%; body-exact, parent-link tail register/block grind)
void* LLIDB_LoadTSFData(LLElem* elem)
{
    char     path[512];
    char     name[512];
    int      n;
    int      len;
    int      i;
    int      base;
    void*    file;
    TsfData* desc;
    LLElem*  parent;

    sprintf(path, "TileData\\%s", elem->image);
    file = RES_OpenFile(path);
    if (file) {
        desc = (TsfData*)malloc(0x24);
        RES_ReadFile(file, &n, 4);
        desc->codes = (unsigned int*)malloc(n * 4);
        desc->second = (unsigned int*)malloc(n * 4);
        desc->n_tiles = n;
        desc->f18 = 0;
        desc->f1c = 0;
        desc->f20 = 0;
        RES_ReadFile(file, &len, 4);
        RES_ReadFile(file, name, len);
        for (i = 0; i < n; i++) {
            RES_ReadFile(file, &desc->codes[i], 4);
            RES_ReadFile(file, &desc->second[i], 4);
        }
        desc->sprites = (void**)AllocTileSpace(desc, n, &base);
        desc->base_slot = base & 0xffff;
        for (i = 0; i < n; i++) {
            LLSprite* sprite;
            RES_ReadFile(file, &len, 4);
            RES_ReadFile(file, name, len);
            name[len] = 0;
            sprite = (LLSprite*)LoadSprite(name, 1);
            desc->sprites[i] = sprite;
            if (sprite->hdr->type == 2 || sprite->hdr->type == 3)
                if (sprite->hdr->frames->count > 1)
                    LLSPlay(sprite->hdr->frames, sprite->hdr);
        }
        desc->parent = 0;
        if (RES_ReadFile(file, &len, 4) == 4 && len != 0) {
            RES_ReadFile(file, path, len);
            path[len] = 0;
            if (LLIDB_FindElement(path, &parent, 0) == 0) {
                elem->data = desc;
                desc->parent = parent;
                LLIDB_LoadData(parent);
                *(void**)((char*)parent->data + 0x74) = desc;
            }
        }
        RES_CloseFile(file);
        elem->type_flags |= 1;
        elem->data = desc;
        return desc;
    }
    return 0;
}
