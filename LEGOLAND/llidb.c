/* LEGOLAND — LLIDB image-database core accessors + loaders. */
#include "legoland.h"

int stricmp(const char*, const char*);   /* the game's case-insensitive compare */
void* malloc(unsigned int);
void* realloc(void*, unsigned int);
unsigned int strlen(const char*);
char* strcpy(char*, const char*);
unsigned int LLIDB_GrowAndGetIndex(void);   /* 0x0047b5a0 page allocator */

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

// FUNCTION: LEGOLAND 0x0047b5a0
unsigned int LLIDB_GrowAndGetIndex(void)
{
    unsigned int cap = g_llidb_capacity;
    unsigned int count;
    if (((g_llidb_count ^ cap) & 0xffffff00) == 0) {
        count = g_llidb_count;
        cap = (count + 0x100) & 0xffffff00;
        g_llidb_capacity = cap;
        g_llidb_pages = (LLElem**)realloc(g_llidb_pages, (cap >> 8) * 4);
        g_llidb_pages[(g_llidb_capacity >> 8) - 1] = (LLElem*)malloc(0x1400);
    }
    return g_llidb_count;
}

// FUNCTION: LEGOLAND 0x0047b610
int LLIDB_RegisterNewElement(char* name, char* image, unsigned int type)
{
    LLElem* found;
    unsigned int index;

    if (!name || !name[0])
        return -4;
    if ((!image || !image[0]) && type != 0x200)
        return -5;

    if (LLIDB_FindElement(name, &found, 0) == 0) {
        if (type != 0x200 && stricmp(found->image, image) != 0)
            return -1;
        return 0;
    }

    index = LLIDB_GrowAndGetIndex();

    g_llidb_pages[index>>8][index&0xff].name = (char*)malloc(strlen(name) + 1);
    strcpy(g_llidb_pages[index>>8][index&0xff].name, name);
    if (!image) {
        g_llidb_pages[index>>8][index&0xff].image = (char*)malloc(1);
        g_llidb_pages[index>>8][index&0xff].image[0] = '\0';
    } else {
        g_llidb_pages[index>>8][index&0xff].image = (char*)malloc(strlen(image) + 1);
        strcpy(g_llidb_pages[index>>8][index&0xff].image, image);
    }
    g_llidb_pages[index>>8][index&0xff].type_flags = type & 0xfff0;
    g_llidb_pages[index>>8][index&0xff].refcount = 0;
    g_llidb_count++;
    return 0;
}
