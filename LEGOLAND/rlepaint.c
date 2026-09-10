/* LEGOLAND -- SoftBlitRLEPlain specialised type-3 painters.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  These eight
 * leaves are HAND-WRITTEN ASSEMBLY: rotating 2-bit mask (ebx=3, rol 2,
 * advance C on wrap), 0xAAAAAAAA/0x55555555 hi/lo tests, and a push/register
 * schedule that no C spelling of the same algorithm reproduces.  Each body is
 * therefore `__declspec(naked)` with the original instruction stream.
 *
 * Frame layout (presentation-data supersedes softblit2.c header names for the
 * A/B/C blocks): A = u16 pixels, B = u8 run lengths, C = packed 2-bit controls.
 * Primary 0/1 emit one word; 2 skips one; 3 + B length 0 ends the row;
 * secondary 0 = literal run, 1 = repeat, 2/3 = transparent run.  Pixel colour
 * zero is opaque black, never a key.
 *
 * Names and prototypes match softblit2.c's SoftBlitRLEPlain dispatcher.
 * Hit leaves OR 1 into g_blit_hit on a singleton mouse address match or when
 * (mouse-dst)>>1 is unsigned-below the drawn opaque run length.  Five top-skip
 * paths (HitL/HitR/Hit and the out-of-scope recolor/highlight) mishandle
 * primary code 1; HitLR and all four no-hit leaves handle both literal codes.
 */

#ifndef LEGOLAND_PORTABLE
#define NAKED __declspec(naked)
#else
#define NAKED
#endif

extern int g_blit_hit;  /* 0x007feb14 */

/* 0x00466d80 -- hit + left+right clip; 325 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00466d80
void RLEPaintHitClipLR(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        test     edi, edi
        je       L_466DFE
    L_466D99:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_466DB7
        add      esi, 2
        jmp      L_466D99
    L_466DB7:
        test     ebp, 55555555h
        je       L_466D99
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_466DFB
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_466D99
        test     ebp, 55555555h
        je       L_466DF6
        add      esi, 2
        jmp      L_466D99
    L_466DF6:
        lea      esi, [esi+ecx*2]
        jmp      L_466D99
    L_466DFB:
        dec      edi
        jg       L_466D99
    L_466DFE:
        mov      eax, dword ptr [esp+30h]
        mov      edi, dword ptr [esp+14h]
        mov      dword ptr [esp+2ch], eax
        mov      eax, dword ptr [esp+34h]
        mov      dword ptr [esp+38h], eax
    L_466E12:
        cmp      dword ptr [esp+30h], 0
        jle      L_466FA7
        dec      dword ptr [esp+30h]
        add      edi, 2
        add      esi, 2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        je       L_466F91
        test     ebp, 55555555h
        lea      esi, [esi-2]
        je       L_466F91
        inc      dword ptr [esp+30h]
        xor      ecx, ecx
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_46713F
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_466EA2
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_466F91
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jmp      L_466F91
    L_466EA2:
        test     ebp, 55555555h
        jne      L_466F22
        sub      dword ptr [esp+30h], ecx
        jl       L_466EBB
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_466F91
    L_466EBB:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_466EF0
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_466EE8
        or       dword ptr [g_blit_hit], 1
    L_466EE8:
        rep     movsw
        jmp      L_466F91
    L_466EF0:
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_466F11
        or       dword ptr [g_blit_hit], 1
    L_466F11:
        rep     movsw
        mov      eax, dword ptr [esp+34h]
        neg      eax
        lea      esi, [esi+eax*2]
        jmp      L_4670DD
    L_466F22:
        sub      dword ptr [esp+30h], ecx
        jl       L_466F30
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_466F91
    L_466F30:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_466F65
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_466F5A
        or       dword ptr [g_blit_hit], 1
    L_466F5A:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_466F91
    L_466F65:
        add      eax, ecx
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_466F83
        or       dword ptr [g_blit_hit], 1
    L_466F83:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_4670DD
    L_466F91:
        cmp      dword ptr [esp+30h], 0
        jg       L_466E12
        cmp      dword ptr [esp+34h], 0
        jle      L_4670DD
    L_466FA7:
        dec      dword ptr [esp+34h]
        add      edi, 2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_466FEA
        mov      eax, dword ptr [esp+3ch]
        sub      edi, 2
        cmp      eax, edi
        jne      L_466FD9
        or       dword ptr [g_blit_hit], 1
    L_466FD9:
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi], ax
        add      edi, 2
        jmp      L_4670D2
    L_466FEA:
        test     ebp, 55555555h
        je       L_4670D2
        inc      dword ptr [esp+34h]
        xor      ecx, ecx
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_46713F
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_467037
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_4670D2
    L_467037:
        test     ebp, 55555555h
        jne      L_467086
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467063
        mov      dword ptr [esp+34h], eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_46705E
        or       dword ptr [g_blit_hit], 1
    L_46705E:
        rep     movsw
        jmp      L_4670D2
    L_467063:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        push     eax
        mov      eax, dword ptr [esp+40h]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_46707D
        or       dword ptr [g_blit_hit], 1
    L_46707D:
        rep     movsw
        pop      eax
        lea      esi, [esi+eax*2]
        jmp      L_4670DD
    L_467086:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_4670B0
        mov      dword ptr [esp+34h], eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4670A5
        or       dword ptr [g_blit_hit], 1
    L_4670A5:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_4670D2
    L_4670B0:
        mov      ecx, dword ptr [esp+34h]
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4670C7
        or       dword ptr [g_blit_hit], 1
    L_4670C7:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_4670DD
    L_4670D2:
        cmp      dword ptr [esp+34h], 0
        jg       L_466FA7
    L_4670DD:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_4670FB
        add      esi, 2
        jmp      L_4670DD
    L_4670FB:
        test     ebp, 55555555h
        je       L_4670DD
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_46713F
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_4670DD
        test     ebp, 55555555h
        je       L_46713A
        add      esi, 2
        jmp      L_4670DD
    L_46713A:
        lea      esi, [esi+ecx*2]
        jmp      L_4670DD
    L_46713F:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        mov      dword ptr [esp+24h], ecx
        mov      eax, dword ptr [esp+2ch]
        mov      dword ptr [esp+30h], eax
        mov      eax, dword ptr [esp+38h]
        mov      dword ptr [esp+34h], eax
        test     ecx, ecx
        jne      L_466E12
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00466d80 in C.  RLEPaintClipLR plus the hit test.  This is the one
     * Hit leaf whose top-skip is CORRECT (`jmp L_466D99` after `add esi,2`,
     * so primary code 1 continues instead of falling into escape
     * processing).  Every drawn span is hit-tested at its own address against
     * the count actually written, including the two left-edge splits and the
     * doubly-cut split that is clipped at both edges at once. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   lskip;
    int                   budget;
    int                   clipped;
    int                   left_over;

    (void)spare;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {                               /* the CORRECT top-skip */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    lskip = left;
    budget = w;
    for (;;) {
        /* ---- L_466E12: skip `lskip` source pixels ----------------------- */
        while (lskip > 0) {
            code = ll_rle_code(&cs);
            lskip--;
            sp++;
            dp++;
            if (!LL_RLE_HI(code))                  /* 0/1: clipped word  */
                continue;
            sp--;
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            lskip++;
            dp--;
            n = *lp++;
            if (n == 0)                            /* L_46713F: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                lskip -= (int)n;
                if (lskip < 0)
                    budget += lskip;
                continue;
            }
            if (!LL_RLE_LO(code)) {                /* L_466EA2: literal  */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp += n;
                    dp += n;
                    continue;
                }
                clipped = lskip;
                budget += clipped;
                if (budget > 0) {                  /* L_466EBB           */
                    clipped += (int)n;
                    sp += clipped;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                    continue;
                }
                clipped += (int)n;                 /* L_466EF0: cut twice */
                sp += clipped;
                dp += clipped;
                n = (unsigned int)((int)n - clipped + budget);
                if (ll_rle_hit_run(mouse, dp, n))
                    g_blit_hit |= 1;
                do { *dp++ = *sp++; } while (--n);
                sp += -budget;
                goto row_tail;
            } else {                               /* L_466F22: repeat   */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp++;
                    dp += n;
                    continue;
                }
                clipped = lskip;
                budget += clipped;
                if (budget > 0) {                  /* L_466F30           */
                    clipped += (int)n;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    continue;
                }
                clipped += (int)n;                 /* L_466F65: cut twice */
                dp += clipped;
                n = (unsigned int)((int)n - clipped + budget);
                if (ll_rle_hit_run(mouse, dp, n))
                    g_blit_hit |= 1;
                v = *sp++;
                do { *dp++ = v; } while (--n);
                goto row_tail;
            }
        }
        if (budget <= 0)                           /* L_466F91 second test */
            goto row_tail;
        /* ---- L_466FA7: the visible span --------------------------------- */
        while (budget > 0) {
            budget--;
            dp++;
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp--;
                if ((const void*)mouse == (const void*)dp)
                    g_blit_hit |= 1;
                *dp++ = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            budget++;
            dp--;
            n = *lp++;
            if (n == 0)
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                budget -= (int)n;
                continue;
            }
            left_over = budget - (int)n;
            if (LL_RLE_LO(code)) {                 /* L_467086: repeat   */
                if (left_over >= 0) {
                    budget = left_over;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                } else {                           /* L_4670B0: clipped  */
                    n = (unsigned int)budget;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    goto row_tail;
                }
            } else {                               /* L_467037: literal  */
                if (left_over >= 0) {
                    budget = left_over;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                } else {                           /* L_467063: clipped  */
                    left_over = -left_over;
                    n = (unsigned int)budget;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                    sp += left_over;
                    goto row_tail;
                }
            }
        }
    row_tail:
        /* ---- L_4670DD: consume the rest of the row ---------------------- */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        lskip = left;
        budget = w;
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x00467180 -- hit + left clip; 202 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00467180
void RLEPaintHitClipL(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        nop
        push     ebx
        mov      ebx, 3
        nop
        push     ebp
        test     edi, edi
        je       L_4671FD
    L_46719B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_4671B7
        add      esi, 2
    L_4671B7:
        test     ebp, 55555555h
        je       L_46719B
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4671FA
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_46719B
        test     ebp, 55555555h
        je       L_4671F5
        add      esi, 2
        jmp      L_46719B
    L_4671F5:
        lea      esi, [esi+ecx*2]
        jmp      L_46719B
    L_4671FA:
        dec      edi
        jg       L_46719B
    L_4671FD:
        mov      edi, dword ptr [esp+14h]
        mov      eax, dword ptr [esp+30h]
        mov      dword ptr [esp+2ch], eax
    L_467209:
        dec      dword ptr [esp+30h]
        add      edi, 2
        add      esi, 2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        je       L_467305
        sub      esi, 2
        test     ebp, 55555555h
        je       L_467305
        inc      dword ptr [esp+30h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4673B7
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_467286
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_467305
        mov      eax, dword ptr [esp+30h]
        jmp      L_467305
    L_467286:
        test     ebp, 55555555h
        jne      L_4672C8
        sub      dword ptr [esp+30h], ecx
        jl       L_46729C
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_467305
    L_46729C:
        mov      eax, dword ptr [esp+30h]
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4672C3
        or       dword ptr [g_blit_hit], 1
    L_4672C3:
        rep     movsw
        jmp      L_467305
    L_4672C8:
        sub      dword ptr [esp+30h], ecx
        jl       L_4672D6
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_467305
    L_4672D6:
        mov      eax, dword ptr [esp+30h]
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4672FA
        or       dword ptr [g_blit_hit], 1
    L_4672FA:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467305
    L_467305:
        cmp      dword ptr [esp+30h], 0
        jg       L_467209
    L_467310:
        add      edi, 2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_46734D
        mov      eax, dword ptr [esp+3ch]
        sub      edi, 2
        cmp      eax, edi
        jne      L_46733E
        or       dword ptr [g_blit_hit], 1
    L_46733E:
        mov      ax, word ptr [esi]
        add      esi, 2
        add      edi, 2
        mov      word ptr [edi-2], ax
        jmp      L_467310
    L_46734D:
        test     ebp, 55555555h
        je       L_467310
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4673B7
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_467386
        lea      edi, [edi+ecx*2]
        jmp      L_467310
    L_467386:
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_467399
        or       dword ptr [g_blit_hit], 1
    L_467399:
        test     ebp, 55555555h
        jne      L_4673A9
        rep     movsw
        jmp      L_467310
    L_4673A9:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467310
    L_4673B7:
        mov      eax, dword ptr [esp+14h]
        add      eax, dword ptr [esp+28h]
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        mov      ecx, dword ptr [esp+24h]
        dec      ecx
        mov      dword ptr [esp+24h], ecx
        mov      eax, dword ptr [esp+2ch]
        mov      dword ptr [esp+30h], eax
        test     ecx, ecx
        jne      L_467209
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00467180 in C.  RLEPaintClipL plus the hit test, and with the
     * defective top-skip.  Unlike rlepaint2.c's leaves, this one DOES test
     * the remainder of a run that straddles the left edge (L_46729C and
     * L_4672D6), against the remainder's own length and address. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   lskip;
    int                   clipped;

    (void)w; (void)spare;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) {
                sp++;                              /* the code-1 defect  */
                if (!LL_RLE_LO(code))
                    continue;
            } else if (!LL_RLE_LO(code)) {
                continue;
            }
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    lskip = left;
    for (;;) {
        /* ---- L_467209: skip `lskip` source pixels (no entry guard) ------ */
        for (;;) {
            lskip--;
            dp++;
            sp++;
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code))                  /* 0/1: clipped word  */
                goto skip_test;
            sp--;
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                goto skip_test;
            lskip++;
            dp--;
            n = *lp++;
            if (n == 0)                            /* L_4673B7: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                lskip -= (int)n;
                goto skip_test;
            }
            if (!LL_RLE_LO(code)) {                /* L_467286: literal  */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp += n;
                    dp += n;
                } else {                           /* L_46729C: split    */
                    clipped = lskip + (int)n;
                    sp += clipped;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                }
            } else {                               /* L_4672C8: repeat   */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp++;
                    dp += n;
                } else {                           /* L_4672D6: split    */
                    clipped = lskip + (int)n;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                }
            }
        skip_test:
            if (lskip <= 0)
                break;
        }
        /* ---- L_467310: paint the rest of the row ------------------------ */
        for (;;) {
            dp++;
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp--;
                if ((const void*)mouse == (const void*)dp)
                    g_blit_hit |= 1;
                *dp++ = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            dp--;
            n = *lp++;
            if (n == 0)
                break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                continue;
            }
            if (ll_rle_hit_run(mouse, dp, n))      /* L_467386: both runs */
                g_blit_hit |= 1;
            if (LL_RLE_LO(code)) {                 /* L_4673A9: repeat   */
                v = *sp++;
                do { *dp++ = v; } while (--n);
            } else {                               /* literal            */
                do { *dp++ = *sp++; } while (--n);
            }
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        lskip = left;
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x004673f0 -- hit + right clip; 197 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x004673f0
void RLEPaintHitClipR(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        nop
        push     ebx
        mov      ebx, 3
        nop
        push     ebp
        test     edi, edi
        je       L_46746D
    L_46740B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_467427
        add      esi, 2
    L_467427:
        test     ebp, 55555555h
        je       L_46740B
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_46746A
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_46740B
        test     ebp, 55555555h
        je       L_467465
        add      esi, 2
        jmp      L_46740B
    L_467465:
        lea      esi, [esi+ecx*2]
        jmp      L_46740B
    L_46746A:
        dec      edi
        jg       L_46740B
    L_46746D:
        mov      edi, dword ptr [esp+14h]
        mov      eax, dword ptr [esp+34h]
        mov      dword ptr [esp+38h], eax
    L_467479:
        dec      dword ptr [esp+34h]
        add      edi, 2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_4674BD
        mov      eax, dword ptr [esp+3ch]
        sub      edi, 2
        cmp      eax, edi
        jne      L_4674AB
        or       dword ptr [g_blit_hit], 1
    L_4674AB:
        mov      ax, word ptr [esi]
        add      esi, 2
        add      edi, 2
        mov      word ptr [edi-2], ax
        jmp      L_4675A4
    L_4674BD:
        test     ebp, 55555555h
        je       L_4675A4
        inc      dword ptr [esp+34h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_467610
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_467509
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_4675A4
    L_467509:
        test     ebp, 55555555h
        jne      L_467558
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467535
        mov      dword ptr [esp+34h], eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_467530
        or       dword ptr [g_blit_hit], 1
    L_467530:
        rep     movsw
        jmp      L_4675A4
    L_467535:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        push     eax
        mov      eax, dword ptr [esp+40h]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_46754F
        or       dword ptr [g_blit_hit], 1
    L_46754F:
        rep     movsw
        pop      eax
        lea      esi, [esi+eax*2]
        jmp      L_4675AF
    L_467558:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467582
        mov      dword ptr [esp+34h], eax
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_467577
        or       dword ptr [g_blit_hit], 1
    L_467577:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_4675A4
    L_467582:
        mov      ecx, dword ptr [esp+34h]
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_467599
        or       dword ptr [g_blit_hit], 1
    L_467599:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_4675AF
    L_4675A4:
        cmp      dword ptr [esp+34h], 0
        jg       L_467479
    L_4675AF:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_4675CD
        add      esi, 2
        jmp      L_4675AF
    L_4675CD:
        test     ebp, 55555555h
        je       L_4675AF
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_467610
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_4675AF
        test     ebp, 55555555h
        je       L_46760B
        add      esi, 2
        jmp      L_4675AF
    L_46760B:
        lea      esi, [esi+ecx*2]
        jmp      L_4675AF
    L_467610:
        mov      eax, dword ptr [esp+14h]
        add      eax, dword ptr [esp+28h]
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        mov      ecx, dword ptr [esp+24h]
        dec      ecx
        mov      dword ptr [esp+24h], ecx
        mov      eax, dword ptr [esp+38h]
        mov      dword ptr [esp+34h], eax
        test     ecx, ecx
        jne      L_467479
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x004673f0 in C.  RLEPaintClipR plus the hit test, and with the
     * defective top-skip (primary code 1 falls through into escape
     * processing).  The run test always uses the count that is actually
     * DRAWN: the full length when the run fits the budget, the budget when
     * it is cut at the right edge (the asm re-reads `mouse` from [esp+40h]
     * there because it has just pushed the overflow). */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   budget;
    int                   left_over;

    (void)left; (void)spare;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) {
                sp++;                              /* the code-1 defect: */
                if (!LL_RLE_LO(code))              /* no jump back here  */
                    continue;
            } else if (!LL_RLE_LO(code)) {
                continue;
            }
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    budget = w;
    for (;;) {
        /* ---- L_467479: the visible span --------------------------------- */
        while (budget > 0) {
            budget--;
            code = ll_rle_code(&cs);
            dp++;
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp--;
                if ((const void*)mouse == (const void*)dp)
                    g_blit_hit |= 1;
                *dp++ = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            budget++;
            dp--;
            n = *lp++;
            if (n == 0)                            /* L_467610: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                budget -= (int)n;
                continue;
            }
            left_over = budget - (int)n;
            if (LL_RLE_LO(code)) {                 /* L_467558: repeat   */
                if (left_over >= 0) {
                    budget = left_over;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                } else {                           /* L_467582: clipped  */
                    n = (unsigned int)budget;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    goto row_tail;
                }
            } else {                               /* L_467509: literal  */
                if (left_over >= 0) {
                    budget = left_over;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                } else {                           /* L_467535: clipped  */
                    left_over = -left_over;
                    n = (unsigned int)budget;
                    if (ll_rle_hit_run(mouse, dp, n))
                        g_blit_hit |= 1;
                    do { *dp++ = *sp++; } while (--n);
                    sp += left_over;
                    goto row_tail;
                }
            }
        }
    row_tail:
        /* ---- L_4675AF: consume the rest of the row ---------------------- */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        budget = w;
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x00467640 -- hit, unclipped; 122 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00467640
void RLEPaintHit(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        push     esi
        mov      edi, dword ptr [esp+24h]
        mov      edx, dword ptr [esp+18h]
        test     edi, edi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        je       L_4676BC
    L_467659:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467675
        add      esi, 2
    L_467675:
        test     ebp, 55555555h
        je       L_467659
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_4676B9
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467659
        test     ebp, 55555555h
        je       L_4676B4
        add      esi, 2
        jmp      L_467659
    L_4676B4:
        lea      esi, [esi+ecx*2]
        jmp      L_467659
    L_4676B9:
        dec      edi
        jg       L_467659
    L_4676BC:
        mov      edi, dword ptr [esp+14h]
    L_4676C0:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        je       L_467764
        test     ebp, 55555555h
        lea      edi, [edi+2]
        je       L_4676C0
        mov      eax, dword ptr [esp+1ch]
        xor      ecx, ecx
        sub      edi, 2
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467782
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_46775C
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_46772C
        or       dword ptr [g_blit_hit], 1
    L_46772C:
        test     ebp, 55555555h
        jne      L_467748
    L_467734:
        dec      ecx
        mov      ax, word ptr [esi]
        lea      esi, [esi+2]
        mov      word ptr [edi], ax
        lea      edi, [edi+2]
        jne      L_467734
        jmp      L_4676C0
    L_467748:
        mov      ax, word ptr [esi]
        lea      esi, [esi+2]
    L_46774E:
        dec      ecx
        mov      word ptr [edi], ax
        lea      edi, [edi+2]
        jne      L_46774E
        jmp      L_4676C0
    L_46775C:
        lea      edi, [edi+ecx*2]
        jmp      L_4676C0
    L_467764:
        cmp      dword ptr [esp+3ch], edi
        jne      L_467771
        or       dword ptr [g_blit_hit], 1
    L_467771:
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi], ax
        add      edi, 2
        jmp      L_4676C0
    L_467782:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        dec      ecx
        test     ecx, ecx
        mov      dword ptr [esp+24h], ecx
        jne      L_4676C0
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00467640 in C.  RLEPaintFast plus the mouse hit test, and with the
     * DEFECTIVE top-skip: `jne L_467675` only skips the `add esi,2`, so a
     * primary code 1 consumes its literal word and then FALLS THROUGH into
     * escape processing (it reads a B length byte) instead of going back for
     * the next code.  Code 0 continues correctly.  Kept: the shipped 16-bpp
     * assets contain no primary 1, so the defect is dormant, but it is what
     * the executable does.  `left`, `w` and `spare` are dead here. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;

    (void)left; (void)w; (void)spare;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) {
                sp++;                              /* add esi,2 ...      */
                if (!LL_RLE_LO(code))              /* ... then FALL INTO */
                    continue;                      /* L_467675: code 0   */
            } else if (!LL_RLE_LO(code)) {
                continue;                          /* 2: one skip        */
            }
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    for (;;) {
        code = ll_rle_code(&cs);
        if (!LL_RLE_HI(code)) {                    /* L_467764: 0/1      */
            if ((const void*)mouse == (const void*)dp)
                g_blit_hit |= 1;                   /* singleton match    */
            *dp++ = *sp++;
            continue;
        }
        dp++;
        if (!LL_RLE_LO(code))
            continue;
        dp--;
        n = *lp++;
        if (n == 0) {                              /* L_467782: end row  */
            row += pitch;
            dp = (unsigned short*)row;
            if (--rows == 0)
                break;
            continue;
        }
        code = ll_rle_code(&cs);
        if (LL_RLE_HI(code)) {                     /* L_46775C: skip run */
            dp += n;
            continue;
        }
        if (ll_rle_hit_run(mouse, dp, n))          /* both opaque runs   */
            g_blit_hit |= 1;
        if (LL_RLE_LO(code)) {                     /* L_467748: repeat   */
            v = *sp++;
            do { *dp++ = v; } while (--n);
        } else {                                   /* L_467734: literal  */
            do { *dp++ = *sp++; } while (--n);
        }
    }
#endif
}

/* 0x004677b0 -- no hit, both edges; 271 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x004677b0
void RLEPaintClipLR(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        test     edi, edi
        je       L_46782E
    L_4677C9:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_4677E7
        add      esi, 2
        jmp      L_4677C9
    L_4677E7:
        test     ebp, 55555555h
        je       L_4677C9
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_46782B
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_4677C9
        test     ebp, 55555555h
        je       L_467826
        add      esi, 2
        jmp      L_4677C9
    L_467826:
        lea      esi, [esi+ecx*2]
        jmp      L_4677C9
    L_46782B:
        dec      edi
        jg       L_4677C9
    L_46782E:
        mov      eax, dword ptr [esp+30h]
        mov      edi, dword ptr [esp+14h]
        mov      dword ptr [esp+2ch], eax
        mov      eax, dword ptr [esp+34h]
        mov      dword ptr [esp+38h], eax
    L_467842:
        cmp      dword ptr [esp+30h], 0
        jle      L_467988
        mov      ebp, dword ptr [edx]
        dec      dword ptr [esp+30h]
        add      esi, 2
        and      ebp, ebx
        rol      ebx, 2
        add      edi, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        je       L_467972
        test     ebp, 55555555h
        lea      esi, [esi-2]
        je       L_467972
        xor      ecx, ecx
        inc      dword ptr [esp+30h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467ABD
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_4678D2
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_467972
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jmp      L_467972
    L_4678D2:
        test     ebp, 55555555h
        jne      L_467929
        sub      dword ptr [esp+30h], ecx
        jl       L_4678EB
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_467972
    L_4678EB:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_46790A
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        rep     movsw
        jmp      L_467972
    L_46790A:
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        rep     movsw
        mov      eax, dword ptr [esp+34h]
        neg      eax
        lea      esi, [esi+eax*2]
        jmp      L_467A5B
    L_467929:
        sub      dword ptr [esp+30h], ecx
        jl       L_467937
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_467972
    L_467937:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_467959
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467972
    L_467959:
        add      eax, ecx
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467A5B
    L_467972:
        cmp      dword ptr [esp+30h], 0
        jg       L_467842
        cmp      dword ptr [esp+34h], 0
        jle      L_467A5B
    L_467988:
        mov      ebp, dword ptr [edx]
        dec      dword ptr [esp+34h]
        add      edi, 2
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_4679B7
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi-2], ax
        jmp      L_467A50
    L_4679B7:
        test     ebp, 55555555h
        je       L_467A50
        xor      ecx, ecx
        inc      dword ptr [esp+34h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467ABD
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_467A01
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_467A50
    L_467A01:
        test     ebp, 55555555h
        jne      L_467A2A
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467A1A
        mov      dword ptr [esp+34h], eax
        rep     movsw
        jmp      L_467A50
    L_467A1A:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        push     eax
        rep     movsw
        pop      eax
        lea      esi, [esi+eax*2]
        jmp      L_467A5B
    L_467A2A:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467A41
        mov      dword ptr [esp+34h], eax
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467A50
    L_467A41:
        mov      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467A5B
    L_467A50:
        cmp      dword ptr [esp+34h], 0
        jg       L_467988
    L_467A5B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467A79
        add      esi, 2
        jmp      L_467A5B
    L_467A79:
        test     ebp, 55555555h
        je       L_467A5B
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467ABD
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467A5B
        test     ebp, 55555555h
        je       L_467AB8
        add      esi, 2
        jmp      L_467A5B
    L_467AB8:
        lea      esi, [esi+ecx*2]
        jmp      L_467A5B
    L_467ABD:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        mov      dword ptr [esp+24h], ecx
        mov      edi, eax
        mov      eax, dword ptr [esp+2ch]
        test     ecx, ecx
        mov      dword ptr [esp+30h], eax
        mov      eax, dword ptr [esp+38h]
        mov      dword ptr [esp+34h], eax
        jne      L_467842
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x004677b0 in C.  Both edges: the left-skip pass of ClipL followed by
     * the budgeted visible pass of ClipR, then the tail.  What is new is that
     * a run straddling the LEFT edge must also pay for the pixels it makes
     * visible: every split in the skip pass does `add [esp+34h],eax` with eax
     * the NEGATIVE leftover, i.e. budget -= visible_remainder.  When that
     * takes the budget to <= 0 the same run is also cut at the right edge,
     * and the count that is actually drawn works out to the budget the row
     * started the run with (`sub ecx,eax / add ecx,budget`).
     * Both counters live in their own argument slots ([esp+30h] left,
     * [esp+34h] w) and are saved in the `top` and `spare` slots.  The skip
     * pass DOES have an entry guard here (`cmp left,0 / jle`). */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   lskip;
    int                   budget;
    int                   clipped;     /* eax: left + n, the clipped head */
    int                   left_over;   /* eax: budget - n */

    (void)spare; (void)mouse;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {                               /* the correct top-skip */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    lskip = left;
    budget = w;
    for (;;) {
        /* ---- L_467842: skip `lskip` source pixels ---------------------- */
        while (lskip > 0) {
            code = ll_rle_code(&cs);
            lskip--;
            sp++;
            dp++;
            if (!LL_RLE_HI(code))                  /* 0/1: clipped word  */
                continue;
            sp--;
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            lskip++;
            dp--;
            n = *lp++;
            if (n == 0)                            /* L_467ABD: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                lskip -= (int)n;
                if (lskip < 0)
                    budget += lskip;
                continue;
            }
            if (!LL_RLE_LO(code)) {                /* L_4678D2: literal  */
                lskip -= (int)n;
                if (lskip >= 0) {                  /* wholly clipped     */
                    sp += n;
                    dp += n;
                    continue;
                }
                clipped = lskip;                   /* eax: negative      */
                budget += clipped;
                if (budget > 0) {                  /* L_4678EB: fits     */
                    clipped += (int)n;
                    sp += clipped;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    do { *dp++ = *sp++; } while (--n);
                    continue;
                }
                /* L_46790A: the remainder also overruns the right edge */
                clipped += (int)n;
                sp += clipped;
                dp += clipped;
                n = (unsigned int)((int)n - clipped + budget);
                do { *dp++ = *sp++; } while (--n);
                sp += -budget;                     /* skip the clipped tail */
                goto row_tail;
            } else {                               /* L_467929: repeat   */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp++;
                    dp += n;
                    continue;
                }
                clipped = lskip;
                budget += clipped;
                if (budget > 0) {                  /* L_467937: fits     */
                    clipped += (int)n;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    continue;
                }
                /* L_467959 */
                clipped += (int)n;
                dp += clipped;
                n = (unsigned int)((int)n - clipped + budget);
                v = *sp++;
                do { *dp++ = v; } while (--n);
                goto row_tail;
            }
        }
        if (budget <= 0)                           /* L_467972 second test */
            goto row_tail;
        /* ---- L_467988: the visible span, `budget` pixels wide ---------- */
        while (budget > 0) {
            budget--;
            code = ll_rle_code(&cs);
            dp++;
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp[-1] = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            budget++;
            dp--;
            n = *lp++;
            if (n == 0)
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                budget -= (int)n;
                continue;
            }
            left_over = budget - (int)n;
            if (LL_RLE_LO(code)) {                 /* L_467A2A: repeat   */
                if (left_over >= 0) {
                    budget = left_over;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                } else {                           /* L_467A41: clipped  */
                    n = (unsigned int)budget;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    goto row_tail;
                }
            } else {                               /* L_467A01: literal  */
                if (left_over >= 0) {
                    budget = left_over;
                    do { *dp++ = *sp++; } while (--n);
                } else {                           /* L_467A1A: clipped  */
                    left_over = -left_over;
                    n = (unsigned int)budget;
                    do { *dp++ = *sp++; } while (--n);
                    sp += left_over;
                    goto row_tail;
                }
            }
        }
    row_tail:
        /* ---- L_467A5B: consume the rest of the row --------------------- */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        lskip = left;
        budget = w;
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x00467b00 -- no hit, left clip; 180 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00467b00
void RLEPaintClipL(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        test     edi, edi
        je       L_467B7E
    L_467B19:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467B37
        add      esi, 2
        jmp      L_467B19
    L_467B37:
        test     ebp, 55555555h
        je       L_467B19
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467B7B
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467B19
        test     ebp, 55555555h
        je       L_467B76
        add      esi, 2
        jmp      L_467B19
    L_467B76:
        lea      esi, [esi+ecx*2]
        jmp      L_467B19
    L_467B7B:
        dec      edi
        jg       L_467B19
    L_467B7E:
        mov      eax, dword ptr [esp+30h]
        mov      edi, dword ptr [esp+14h]
        mov      dword ptr [esp+2ch], eax
    L_467B8A:
        mov      ebp, dword ptr [edx]
        dec      dword ptr [esp+30h]
        add      edi, 2
        and      ebp, ebx
        rol      ebx, 2
        add      esi, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        je       L_467C5D
        test     ebp, 55555555h
        lea      esi, [esi-2]
        je       L_467C5D
        xor      ecx, ecx
        inc      dword ptr [esp+30h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467CE2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_467C04
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_467C5D
        mov      eax, dword ptr [esp+30h]
        jmp      L_467C5D
    L_467C04:
        test     ebp, 55555555h
        jne      L_467C33
        sub      dword ptr [esp+30h], ecx
        jl       L_467C1A
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_467C5D
    L_467C1A:
        mov      eax, dword ptr [esp+30h]
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        rep     movsw
        jmp      L_467C5D
    L_467C33:
        sub      dword ptr [esp+30h], ecx
        jl       L_467C41
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_467C5D
    L_467C41:
        mov      eax, dword ptr [esp+30h]
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467C5D
    L_467C5D:
        cmp      dword ptr [esp+30h], 0
        jg       L_467B8A
    L_467C68:
        mov      ebp, dword ptr [edx]
        add      edi, 2
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467C90
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi-2], ax
        jmp      L_467C68
    L_467C90:
        test     ebp, 55555555h
        je       L_467C68
        xor      ecx, ecx
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467CE2
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_467CCA
        lea      edi, [edi+ecx*2]
        jmp      L_467C68
    L_467CCA:
        test     ebp, 55555555h
        jne      L_467CD7
        rep     movsw
        jmp      L_467C68
    L_467CD7:
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467C68
    L_467CE2:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        mov      dword ptr [esp+24h], ecx
        mov      edi, eax
        mov      eax, dword ptr [esp+2ch]
        test     ecx, ecx
        mov      dword ptr [esp+30h], eax
        jne      L_467B8A
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00467b00 in C.  Left edge only: skip `left` source pixels, then paint
     * the whole rest of the source row (no right budget at all).  The skip
     * pass advances BOTH pointers, because the dispatcher biased dst left by
     * src->left -- stepping the output over the clipped pixels is what lands
     * the first painted pixel on dst->x.  A run straddling the left edge is
     * split: the clipped head is stepped over and the remainder drawn.
     * `left` lives in its own argument slot [esp+30h], saved in the `top`
     * slot [esp+2ch]; `w` and `mouse` are dead.  NOTE the skip pass has NO
     * entry guard (no `cmp left,0 / jle`), so it always runs at least once;
     * the dispatcher only selects this leaf with left >= 1. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   lskip;
    int                   clipped;   /* eax: left + n, the clipped head */

    (void)w; (void)spare; (void)mouse;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    lskip = left;
    for (;;) {
        /* ---- L_467B8A: skip `lskip` source pixels ---------------------- */
        for (;;) {
            code = ll_rle_code(&cs);
            lskip--;
            dp++;
            sp++;
            if (!LL_RLE_HI(code))                  /* 0/1: clipped word  */
                goto skip_test;
            sp--;                                  /* lea esi,[esi-2]    */
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                goto skip_test;
            lskip++;
            dp--;
            n = *lp++;
            if (n == 0)                            /* L_467CE2: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                lskip -= (int)n;
                goto skip_test;
            }
            if (!LL_RLE_LO(code)) {                /* L_467C04: literal  */
                lskip -= (int)n;
                if (lskip >= 0) {                  /* wholly clipped     */
                    sp += n;
                    dp += n;
                } else {                           /* L_467C1A: split    */
                    clipped = lskip + (int)n;
                    sp += clipped;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    do { *dp++ = *sp++; } while (--n);
                }
            } else {                               /* L_467C33: repeat   */
                lskip -= (int)n;
                if (lskip >= 0) {
                    sp++;
                    dp += n;
                } else {                           /* L_467C41: split    */
                    clipped = lskip + (int)n;
                    dp += clipped;
                    n = (unsigned int)(-lskip);
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                }
            }
        skip_test:
            if (lskip <= 0)                        /* L_467C5D           */
                break;
        }
        /* ---- L_467C68: paint the rest of the row ----------------------- */
        for (;;) {
            code = ll_rle_code(&cs);
            dp++;
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp[-1] = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            dp--;
            n = *lp++;
            if (n == 0)                            /* end of row         */
                break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                continue;
            }
            if (LL_RLE_LO(code)) {                 /* L_467CD7: repeat   */
                v = *sp++;
                do { *dp++ = v; } while (--n);
            } else {                               /* L_467CCA: literal  */
                do { *dp++ = *sp++; } while (--n);
            }
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        lskip = left;                              /* restore from [esp+2ch] */
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x00467d10 -- no hit, right clip; 167 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00467d10
void RLEPaintClipR(void* dst, void* a, void* b, void* c, int h, int pitch,
                              int top, int left, int w, int spare, void* mouse)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        mov      edx, dword ptr [esp+14h]
        mov      edi, dword ptr [esp+20h]
        push     esi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        test     edi, edi
        je       L_467D8E
    L_467D29:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467D47
        add      esi, 2
        jmp      L_467D29
    L_467D47:
        test     ebp, 55555555h
        je       L_467D29
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467D8B
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467D29
        test     ebp, 55555555h
        je       L_467D86
        add      esi, 2
        jmp      L_467D29
    L_467D86:
        lea      esi, [esi+ecx*2]
        jmp      L_467D29
    L_467D8B:
        dec      edi
        jg       L_467D29
    L_467D8E:
        mov      eax, dword ptr [esp+34h]
        mov      edi, dword ptr [esp+14h]
        mov      dword ptr [esp+38h], eax
    L_467D9A:
        dec      dword ptr [esp+34h]
        mov      ebp, dword ptr [edx]
        add      edi, 2
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467DC9
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi-2], ax
        jmp      L_467E60
    L_467DC9:
        test     ebp, 55555555h
        je       L_467E60
        inc      dword ptr [esp+34h]
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        sub      edi, 2
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467ECD
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        je       L_467E13
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_467E60
    L_467E13:
        test     ebp, 55555555h
        jne      L_467E3A
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467E2C
        mov      dword ptr [esp+34h], eax
        rep     movsw
        jmp      L_467E60
    L_467E2C:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        rep     movsw
        lea      esi, [esi+eax*2]
        jmp      L_467E6B
    L_467E3A:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_467E51
        mov      dword ptr [esp+34h], eax
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467E60
    L_467E51:
        mov      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        rep     stosw
        jmp      L_467E6B
    L_467E60:
        cmp      dword ptr [esp+34h], 0
        jg       L_467D9A
    L_467E6B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467E89
        add      esi, 2
        jmp      L_467E6B
    L_467E89:
        test     ebp, 55555555h
        je       L_467E6B
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467ECD
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467E6B
        test     ebp, 55555555h
        je       L_467EC8
        add      esi, 2
        jmp      L_467E6B
    L_467EC8:
        lea      esi, [esi+ecx*2]
        jmp      L_467E6B
    L_467ECD:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        mov      dword ptr [esp+24h], ecx
        mov      edi, eax
        mov      eax, dword ptr [esp+38h]
        test     ecx, ecx
        mov      dword ptr [esp+34h], eax
        jne      L_467D9A
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00467d10 in C.  Right edge only: `left` is zero (the dispatcher only
     * picks this leaf when it is), so there is no left-skip pass.  Three
     * passes per row: paint while the `w` budget lasts, then consume the tail
     * to the end-of-row marker.  The budget lives in the `w` argument slot
     * [esp+34h] and its initial value is saved in the `spare` slot
     * [esp+38h]; `mouse` is dead here. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;
    int                   budget;
    int                   left_over;   /* eax: budget - n */

    (void)left; (void)spare; (void)mouse;
    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {                               /* the correct top-skip */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) {
                if (--skip > 0) continue;
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    budget = w;
    for (;;) {
        /* ---- L_467D9A: the visible span, `budget` pixels wide ---------- */
        while (budget > 0) {
            budget--;                              /* dec [esp+34h]      */
            code = ll_rle_code(&cs);
            dp++;                                  /* add edi,2          */
            if (!LL_RLE_HI(code)) {                /* 0/1: one word      */
                dp[-1] = *sp++;
                continue;
            }
            if (!LL_RLE_LO(code))                  /* 2: transparent     */
                continue;
            budget++;                              /* inc [esp+34h]      */
            dp--;                                  /* sub edi,2          */
            n = *lp++;
            if (n == 0)                            /* L_467ECD: end row  */
                goto end_of_row;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) {                 /* transparent run    */
                dp += n;
                budget -= (int)n;
                continue;
            }
            left_over = budget - (int)n;
            if (LL_RLE_LO(code)) {                 /* L_467E3A: repeat   */
                if (left_over >= 0) {
                    budget = left_over;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                } else {                           /* L_467E51: clipped  */
                    n = (unsigned int)budget;
                    v = *sp++;
                    do { *dp++ = v; } while (--n);
                    goto row_tail;
                }
            } else {                               /* literal run        */
                if (left_over >= 0) {
                    budget = left_over;
                    do { *dp++ = *sp++; } while (--n);
                } else {                           /* L_467E2C: clipped  */
                    left_over = -left_over;        /* neg eax: the excess */
                    n = (unsigned int)budget;
                    do { *dp++ = *sp++; } while (--n);
                    sp += left_over;               /* skip the clipped words */
                    goto row_tail;
                }
            }
        }
    row_tail:
        /* ---- L_467E6B: consume the rest of the row, drawing nothing ---- */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }
            if (!LL_RLE_LO(code)) continue;
            n = *lp++;
            if (n == 0) break;
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;
            if (LL_RLE_LO(code)) sp++;
            else sp += n;
        }
    end_of_row:
        row += pitch;
        dp = (unsigned short*)row;
        budget = w;                                /* restore from [esp+38h] */
        if (--rows == 0)
            break;
    }
#endif
}

/* 0x00467f00 -- no hit, unclipped; 114 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00467f00
void RLEPaintFast(void* dst, void* a, void* b, void* c, int h, int pitch,
                         int top)
{
#ifndef LEGOLAND_PORTABLE
    __asm {
        push     edi
        push     esi
        mov      edi, dword ptr [esp+24h]
        mov      edx, dword ptr [esp+18h]
        test     edi, edi
        mov      esi, dword ptr [esp+10h]
        push     ebx
        mov      ebx, 3
        push     ebp
        je       L_467F7E
    L_467F19:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        jne      L_467F37
        add      esi, 2
        jmp      L_467F19
    L_467F37:
        test     ebp, 55555555h
        je       L_467F19
        xor      ecx, ecx
        mov      eax, dword ptr [esp+1ch]
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_467F7B
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467F19
        test     ebp, 55555555h
        je       L_467F76
        add      esi, 2
        jmp      L_467F19
    L_467F76:
        lea      esi, [esi+ecx*2]
        jmp      L_467F19
    L_467F7B:
        dec      edi
        jg       L_467F19
    L_467F7E:
        mov      edi, dword ptr [esp+14h]
    L_467F82:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        je       L_468002
        test     ebp, 55555555h
        lea      edi, [edi+2]
        je       L_467F82
        mov      eax, dword ptr [esp+1ch]
        xor      ecx, ecx
        sub      edi, 2
        mov      cl, byte ptr [eax]
        inc      eax
        test     ecx, ecx
        mov      dword ptr [esp+1ch], eax
        je       L_468013
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        test     ebp, 0aaaaaaaah
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        jne      L_467FFD
        test     ebp, 55555555h
        jne      L_467FEC
    L_467FDB:
        dec      ecx
        mov      ax, word ptr [esi]
        lea      esi, [esi+2]
        mov      word ptr [edi], ax
        lea      edi, [edi+2]
        jne      L_467FDB
        jmp      L_467F82
    L_467FEC:
        mov      ax, word ptr [esi]
        lea      esi, [esi+2]
    L_467FF2:
        dec      ecx
        mov      word ptr [edi], ax
        lea      edi, [edi+2]
        jne      L_467FF2
        jmp      L_467F82
    L_467FFD:
        lea      edi, [edi+ecx*2]
        jmp      L_467F82
    L_468002:
        mov      ax, word ptr [esi]
        add      esi, 2
        mov      word ptr [edi], ax
        add      edi, 2
        jmp      L_467F82
    L_468013:
        mov      eax, dword ptr [esp+14h]
        mov      ecx, dword ptr [esp+24h]
        add      eax, dword ptr [esp+28h]
        dec      ecx
        mov      dword ptr [esp+14h], eax
        test     ecx, ecx
        mov      edi, eax
        mov      dword ptr [esp+24h], ecx
        jne      L_467F82
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    /* 0x00467f00 in C.  The simplest leaf: no clipping, no hit test.
     *   edi = dp (the output pixel), esi = sp (the u16 pixel stream),
     *   [esp+1ch] = lp (the u8 length stream), edx/ebx = the control reader,
     *   [esp+14h] = row (the saved row base), [esp+24h] = rows.
     * The top-skip loop handles BOTH literal codes (`jmp L_467F19` after
     * `add esi,2`), unlike the five defective leaves. */
    unsigned char*        row;
    unsigned short*       dp;
    const unsigned short* sp = (const unsigned short*)a;
    const unsigned char*  lp = (const unsigned char*)b;
    LLRleCtl              cs;
    unsigned int          code;
    unsigned int          n;
    unsigned short        v;
    int                   rows;
    int                   skip;

    ll_rle_open(&cs, c);

    skip = top;
    if (skip != 0) {                               /* test edi,edi / je */
        for (;;) {
            code = ll_rle_code(&cs);
            if (!LL_RLE_HI(code)) { sp++; continue; }        /* 0/1: one word */
            if (!LL_RLE_LO(code)) continue;                  /* 2: one skip   */
            n = *lp++;
            if (n == 0) {                                    /* 3+0: end row  */
                if (--skip > 0) continue;                    /* dec edi / jg  */
                break;
            }
            code = ll_rle_code(&cs);
            if (LL_RLE_HI(code)) continue;                   /* 2/3: skip run */
            if (LL_RLE_LO(code)) sp++;                       /* 1: repeat     */
            else sp += n;                                    /* 0: literal    */
        }
    }

    row = (unsigned char*)dst;
    dp = (unsigned short*)row;
    rows = h;
    for (;;) {
        code = ll_rle_code(&cs);
        if (!LL_RLE_HI(code)) {                    /* je L_468002: 0/1  */
            *dp++ = *sp++;
            continue;
        }
        dp++;                                      /* lea edi,[edi+2]   */
        if (!LL_RLE_LO(code))                      /* 2: transparent    */
            continue;
        dp--;                                      /* sub edi,2         */
        n = *lp++;
        if (n == 0) {                              /* L_468013: end row */
            row += pitch;
            dp = (unsigned short*)row;
            if (--rows == 0)
                break;
            continue;
        }
        code = ll_rle_code(&cs);
        if (LL_RLE_HI(code)) {                     /* L_467FFD: skip run */
            dp += n;
            continue;
        }
        if (LL_RLE_LO(code)) {                     /* L_467FEC: repeat   */
            v = *sp++;
            do { *dp++ = v; } while (--n);
        } else {                                   /* L_467FDB: literal  */
            do { *dp++ = *sp++; } while (--n);
        }
    }
#endif
}

