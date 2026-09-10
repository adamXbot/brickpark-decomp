/* LEGOLAND — InitMan / rider texture helpers (scope AC).
 *
 * Alt-texture loaders called from InitMan (data2.c) and the 3D-rider pose
 * helper under Put3DBlokesOnRide (rides.c). SkipStrings / LookupTextureName
 * live elsewhere — declare only.
 *
 * VC6 SP3 /O2 /Gy /Gd. Addresses are load-bearing; names are ours except
 * LoadAltTextures / PutOne3DBlokeOnRide (already named by callers).
 */
#include "legoland.h"
#include <string.h>

#pragma intrinsic(strlen)

typedef struct Vec2f {
    float x;
    float y;
} Vec2f;

/* First arg of RiderTrackToScreen is the RideAnim* from the caller; +0x1c/+0x20
 * are origin ints inside its padding. */
typedef struct RideAnim {
    int    frame_count;     /* +0x00 */
    int    seat_count;      /* +0x04 */
    char   pad08[0x1c - 8];
    int    origin_x;        /* +0x1c */
    int    origin_y;        /* +0x20 */
    void** seat_tracks;     /* +0x24 */
} RideAnim;

typedef struct PersonXY {
    char pad0[0x58];
    int  matrix[9];         /* +0x58  16.16 rotation */
} PersonXY;

/* ---- CRT / RES ---------------------------------------------------------- */
extern int   RES_ReadFile(void* f, void* buf, int n);           /* 0x00489cf0 */
extern void* RES_OpenFile(const char* path);                    /* 0x00489b60 */
extern void  RES_CloseFile(void* f);                            /* 0x00489de0 */
extern int   NameCompare(const char* a, const char* b);         /* 0x004aab90 */
extern void* HeapAlloc_w(unsigned int n);                       /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                               /* 0x0049e4d0 */
extern int   sprintf(char* buf, const char* fmt, ...);          /* 0x0049e573 */
extern int   sscanf(const char* buf, const char* fmt, ...);     /* 0x0049eee7 */
extern int   tolower(int c);                                    /* 0x0049ef23 */
extern char* LoadTextFile(const char* dir, const char* file);   /* 0x004402d0 */
extern void  SetPersonPosition(void* p, int x, int y);          /* 0x00440190 */

/* 3x3 channel tables for the seat-track → person matrix fill. */
extern int g_ride_mtx_chan[3];                                  /* 0x004b7ae0  {0,1,2} */
extern int g_ride_mtx_row[3];                                   /* 0x004b7aec  {0,2,1} */
extern int g_ride_mtx_sign_a[3];                                /* 0x004b7af8  {1,-1,-1} */
extern int g_ride_mtx_sign_b[3];                                /* 0x004b7b04  {-1,1,1} */

extern const char kAltTexPathFmt[];                             /* 0x004b7b10 ".\\3ddata\\new\\%s\\%s" */
extern const char kAltTexLineFmt[];                             /* 0x004b7d00 "%s %i %i %i %i %i" */

/* Outfit tables filled by LoadAltTextures (savechunks.c / savemisc2.c). */
extern int    g_outfitA_n0;                                     /* 0x0063810c */
extern void** g_outfitA_tab0;                                   /* 0x00655a38 */
extern int    g_outfitB_n0;                                     /* 0x0062feb8 */
extern void** g_outfitB_tab0;                                   /* 0x0062fea8 */
extern void*  g_outfitB_pal0;                                   /* 0x0064cd90 */
extern void*  g_outfitA_pal0;                                   /* 0x00641000 */
extern int    g_outfitA_n1;                                     /* 0x0064cd88 */
extern void** g_outfitA_tab1;                                   /* 0x0062fef8 */
extern int    g_outfitB_n1;                                     /* 0x0063835c */
extern void** g_outfitB_tab1;                                   /* 0x0064cd8c */
extern void*  g_outfitB_pal1;                                   /* 0x00638108 */
extern void*  g_outfitA_pal1;                                   /* 0x00638110 */

/* Convert a seat-track float pair into anim-relative screen ints.
 * out = ((int)(pos - cam)) / 2 + anim->origin. The float[3] frame is the
 * lever that yields sub esp,0xc and the fld/mov-ybits/fsub schedule. */
// FUNCTION: LEGOLAND 0x00441910
void RiderTrackToScreen(RideAnim* anim, Vec2f* pos, Pos* out)
{
    int hx;
    int hy;
    float frame[3];

    frame[0] = pos->x;
    frame[1] = pos->y;
    frame[0] -= 320.0f;
    frame[1] -= 270.0f;
    hx = (int)frame[0] / 2;
    out->x = hx;
    hy = (int)frame[1] / 2;
    out->y = hy;
    out->x = anim->origin_x + hx;
    out->y = anim->origin_y + hy;
}

/* Read one CR/LF-terminated line (at most maxlen bytes) from a RES file.
 * Returns the buffer, or 0 on EOF with no bytes collected. A bare stop still
 * checks for CR so a maxlen hit on a CR byte consumes the following LF. */
// FUNCTION: LEGOLAND 0x004427e0
char* ReadAltLine(void* file, char* buf, int maxlen)
{
    char* dest = buf;
    int   n = 0;
    int   got;
    char  c;

    for (;;) {
        got = RES_ReadFile(file, &c, 1);
        if (got) {
            if (c == '\r')
                goto eat_cr;
            if (c == '\n')
                goto term;
            *dest = c;
            dest++;
            n++;
        }
        if (c == '\r')
            goto eat_cr;
        if (c == '\n')
            goto stop;
        if (n >= maxlen)
            goto stop;
        if (!got)
            goto stop;
        continue;
    stop:
        if (c != '\r')
            goto term;
    eat_cr:
        RES_ReadFile(file, &c, 1);
    term:
        break;
    }
    *dest = 0;
    if (!got && n == 0)
        return 0;
    return buf;
}

/* Walk a string list; return the matching index, or -1 at an empty string. */
// FUNCTION: LEGOLAND 0x00442860
int FindAltNameIndex(char* list, char* name)
{
    int idx = 0;

    while (NameCompare(list, name) != 0) {
        list += strlen(list) + 1;
        idx++;
        if (strlen(list) == 0)
            return -1;
    }
    return idx;
}

/* Place one 3D rider: clamp frame, sample the seat track into person->matrix
 * (column-major 3x3 via the four channel tables), then SetPersonPosition.
 *
 * Closed 2026-09-09 with the two Codex-F levers (docs/lanes/codex-f.md):
 * the scope-V cancelled pair carries the row index in a struct member and
 * anchors on the link-time address constant `(int)g_ride_mtx_chan` -- an
 * anchor receives an allocator priority bump, so it must be a value whose
 * own ranking does not matter; anchoring on `track` (the earlier body)
 * ranked the track pointer above `si` and cost an 8-line esi/edi swap.
 * The `sb` alias then stops VC6 reordering the two integer multiplies, so
 * the sign product is completed before the fixed-point factor as in the
 * original. Every source operand order is inert; only the spelling moves it.
 */
// FUNCTION: LEGOLAND 0x00441980
void PutOne3DBlokeOnRide(RideAnim* anim, int index, int frame,
                         PersonXY* person, int screen_x, int screen_y)
{
    float* track;
    Pos    out;
    Pos    base;
    int    i;
    int*   sb;
    Pos    t;

    if (frame < 0)
        frame = 0;
    if (frame >= anim->frame_count)
        frame = anim->frame_count - 1;
    track = (float*)anim->seat_tracks[index];
    track = (float*)((char*)track + frame * 48);
    RiderTrackToScreen(anim, (Vec2f*)track, &base);
    t.y = (int)g_ride_mtx_chan;
    sb = g_ride_mtx_sign_b;
    out.x = base.x + screen_x;
    out.y = base.y + screen_y;
    *(float*)&screen_y = 65536.0f;
    for (i = 0; i < 3; i++) {
        int  si = g_ride_mtx_chan[i];
        int* dest = &person->matrix[i];
        int  j;
        for (j = 0; j < 3; j++) {
            t.x = g_ride_mtx_row[j] + si * 3 + 3;
            t.x += t.y;
            t.x -= t.y;
            *(float*)&frame = track[t.x];
#ifndef LEGOLAND_PORTABLE
            __asm {
                fld   dword ptr frame
                fmul  dword ptr screen_y
                fistp dword ptr frame
            }
#else
            frame = LL_FISTP(LL_ASFLT(frame) * LL_ASFLT(screen_y));
#endif
            dest[j * 3] = g_ride_mtx_sign_a[j] * sb[si] * frame;
        }
    }
    SetPersonPosition(person, out.x, out.y);
}

/* Load altman/altwoman name lists and map visitor.txt rows onto the outfit
 * tables for one sex. ctx is unused (caller still passes the anim context).
 * The alt file layout matches LookupTextureName's two-section pack: title,
 * count, names..., empty, title B, count B, names B... `text2` is the
 * section-A title the loop compares against (its own home at +0x30); `text`
 * is only the allocation, freed at the end.
 *
 * FLOOR: 229i/752B size-exact, 6 strict mism, one class. countA and countB are
 * deliberately left UNASSIGNED on the fail paths: the original's
 * `mov ebx,[esp+0x24]` / `mov ebp,[esp+0x14]` are uninitialised reads whose
 * phantom homes VC6 shares with `text` and `file`; any `else countA = ...`
 * folds to `xor ebx,ebx` (-2B). The residual is the home permutation of the
 * four 2-ref pointers (listB/text2/listA/titleB), which VC6 orders by first
 * load in the loop; the original's order is listB, text2, listA, titleB.
 * See docs/lanes/scope-ac.md. */
// WIP-FUNCTION: LEGOLAND 0x00442980  (229i/752B; 6 mism: listB/text2/listA/titleB homes)
void LoadAltTextures(const char* alt, const char* base, const char* dir,
                     int sex, void* ctx)
{
    struct {
        char path[0x100];
        char line[0x200];
        char name[0x80];
    } buf;
    char*  text;
    char*  text2;
    char*  listA;
    char*  listB;
    char*  titleB;
    int    countA;
    int    countB;
    void** tabA;
    void** tabB;
    int*   palA;
    int*   palB;
    void*  file;
    int    index;
    /* The five sscanf ints. Separate scalars home by argument position
     * (e,b,d,a,c ascending, names inert); the original's ascending order is
     * arg3, arg1, arg4, arg5, arg2, which only an aggregate reproduces. */
    struct {
        int i2;
        int i0;
        int i3;
        int i4;
        int i1;
    } num;
    char*  p;
    char*  q;
    int    n;
    char   ch;

    (void)ctx;
    index = 0;
    text = LoadTextFile(dir, alt);
    if (text) {
        text2 = text;
        q = text + strlen(text) + 1;
        countA = *(int*)q;
        q += 4;
        listA = q;
        if (countA) {
            do {
                q += strlen(q) + 1;
            } while (strlen(q) != 0);
            q++;
            titleB = q;
            q = q + strlen(q) + 1;
            countB = *(int*)q;
            listB = q + 4;
        }
    }
    sprintf(buf.path, kAltTexPathFmt, dir, base);
    if (!sex) {
        g_outfitA_n0 = countA;
        g_outfitA_tab0 = (void**)HeapAlloc_w(countA * 4);
        g_outfitB_n0 = countB;
        g_outfitB_tab0 = (void**)HeapAlloc_w(countB * 4);
        tabA = g_outfitA_tab0;
        tabB = g_outfitB_tab0;
        palB = (int*)&g_outfitB_pal0;
        palA = (int*)&g_outfitA_pal0;
    } else {
        g_outfitA_n1 = countA;
        g_outfitA_tab1 = (void**)HeapAlloc_w(countA * 4);
        g_outfitB_n1 = countB;
        g_outfitB_tab1 = (void**)HeapAlloc_w(countB * 4);
        tabA = g_outfitA_tab1;
        tabB = g_outfitB_tab1;
        palB = (int*)&g_outfitB_pal1;
        palA = (int*)&g_outfitA_pal1;
    }
    file = RES_OpenFile(buf.path);
    if (file) {
        ReadAltLine(file, buf.line, 0x200);
        ReadAltLine(file, buf.line, 0x200);
        if (ReadAltLine(file, buf.line, 0x200)) {
            do {
                q = buf.path;
                p = buf.line;
                for (;;) {
                    ch = (char)tolower(*p++);
                    if (ch == '.')
                        ch = 0;
                    *q = ch;
                    q++;
                    if (!ch)
                        break;
                }
                sscanf(p, kAltTexLineFmt, buf.name, &num.i0, &num.i1, &num.i2,
                       &num.i3, &num.i4);
                if (NameCompare(text2, buf.path) == 0)
                    *palA = index;
                else if (NameCompare(titleB, buf.path) == 0)
                    *palB = index;
                if (countA) {
                    n = FindAltNameIndex(listA, buf.path);
                    if (n != -1)
                        tabA[n] = (void*)index;
                }
                if (countB) {
                    n = FindAltNameIndex(listB, buf.path);
                    if (n != -1)
                        tabB[n] = (void*)index;
                }
                index++;
            } while (ReadAltLine(file, buf.line, 0x200));
        }
    }
    RES_CloseFile(file);
    HeapFree_w(text);
}
