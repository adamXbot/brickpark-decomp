/* LEGOLAND — LLIDB image-database core accessors. */
#include "legoland.h"

int stricmp(const char*, const char*);   /* the game's case-insensitive compare */

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
