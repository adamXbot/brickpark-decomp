/* LEGOLAND — LLIDB image-database core accessors + loaders. */
#include "legoland.h"

int stricmp(const char*, const char*);   /* the game's case-insensitive compare */

/* per-type asset loaders (dispatched by LLIDB_LoadData). */
void* LLIDB_LoadODFData(LLElem* elem);
void* LLIDB_LoadTSMData(LLElem* elem);
void* LLIDB_LoadTSFData(LLElem* elem);
void* LLIDB_LoadILFData(LLElem* elem);
void* LLIDB_LoadCSPData(LLElem* elem);

// FUNCTION: LEGOLAND 0x0047d3a0
void* LLIDB_LoadData(LLElem* elem)
{
    unsigned int flags = elem->type_flags;
    void* result;
    if (flags & 1) {
        elem->refcount++;
        return elem->data;
    }
    flags |= 1;
    elem->data = 0;
    elem->type_flags = flags;
    switch (flags & 0xfff0) {
    case 0x10:
    case 0x1010:
        elem->data = LLIDB_LoadODFData(elem);
        break;
    case 0x20:
        elem->data = LLIDB_LoadTSMData(elem);
        break;
    case 0x40:
        elem->data = LLIDB_LoadTSFData(elem);
        break;
    case 0x200:
    case 0x800:
        return 0;
    case 0x400:
        elem->data = LLIDB_LoadILFData(elem);
        break;
    case 0x2000:
        elem->data = LLIDB_LoadCSPData(elem);
        break;
    }
    result = elem->data;
    if (result == 0)
        elem->type_flags &= 0xfffffffe;
    else
        elem->refcount++;
    return result;
}

// FUNCTION: LEGOLAND 0x0047b2d0
unsigned int LLIDB_GetCount(void)
{
    return g_llidb_count;
}

// FUNCTION: LEGOLAND 0x0047b2e0
int LLIDB_GetElement(unsigned int idx, LLElem** out)
{
    if (idx < g_llidb_count) {
        if (out)
            *out = &g_llidb_pages[idx >> 8][idx & 0xff];
        return 0;
    }
    if (out)
        *out = 0;
    return -3;
}

// FUNCTION: LEGOLAND 0x0047b330
int LLIDB_FindElement(char* name, LLElem** out_elem, unsigned int* out_idx)
{
    unsigned int i;
    if (!name) {
        if (out_elem)
            *out_elem = name;
        if (out_idx)
            *out_idx = 0;
        return -3;
    }
    for (i = 0; i < g_llidb_count; i++) {
        if (stricmp(name, g_llidb_pages[i >> 8][i & 0xff].name) == 0) {
            if (out_elem)
                *out_elem = &g_llidb_pages[i >> 8][i & 0xff];
            if (out_idx)
                *out_idx = i;
            return 0;
        }
    }
    if (out_elem)
        *out_elem = 0;
    if (out_idx)
        *out_idx = 0;
    return -3;
}

// FUNCTION: LEGOLAND 0x0047b410
int LLIDB_FindElementFromDataPtr(void* data, LLElem** out_elem, unsigned int* out_idx)
{
    unsigned int i;
    if (!data) {
        if (out_elem)
            *out_elem = data;
        if (out_idx)
            *out_idx = 0;
        return -3;
    }
    for (i = 0; i < g_llidb_count; i++) {
        if (data == g_llidb_pages[i >> 8][i & 0xff].data) {
            if (out_elem)
                *out_elem = &g_llidb_pages[i >> 8][i & 0xff];
            if (out_idx)
                *out_idx = i;
            return 0;
        }
    }
    if (out_elem)
        *out_elem = 0;
    if (out_idx)
        *out_idx = 0;
    return -3;
}
