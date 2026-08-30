/* LEGOLAND — LLIDB per-type asset loaders (parsers dispatched by LoadData). */
#include "legoland.h"

int   sprintf(char*, const char*, ...);
void* malloc(unsigned int);
void* RES_OpenFile(const char*);
void  RES_ReadFile(void* file, void* buf, int len);
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
