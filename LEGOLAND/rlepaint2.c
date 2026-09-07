/* LEGOLAND -- SoftBlitRLE recolour + highlight type-3 painters (scope AD).
 *
 * HAND-WRITTEN ASSEMBLY, same family as rlepaint.c (AB).  Type-3 A/B/C
 * grammar is unchanged; every opaque store is `pixel & g_sp_recolour`
 * (recolour) or `(pixel & g_sp_recolour) >> 1` (highlight).  Both leaves
 * are the Hit+ClipLR shape: left skip, width budget, right-edge run
 * splits, and g_blit_hit on a singleton mouse match or when
 * ((mouse-dst)>>1) is unsigned-below the drawn opaque run.  Top-skip
 * mishandles primary code 1 (fall through after the 0xAAAAAAAA test),
 * same dormant defect as HitL/HitR/Hit.
 *
 * Alignment nops after the esi load and `mov ebx, 3` are in the original
 * stream and are kept.  Literal runs cannot use `rep movsw` because of
 * the per-pixel mask; they are a software loop.  Repeat runs still use
 * `and ax, [mask]` (highlight also `shr ax, 1`) then `rep stosw`.
 */

#ifndef LEGOLAND_PORTABLE
#define NAKED __declspec(naked)
#else
#define NAKED
#endif

extern int g_blit_hit;       /* 0x007feb14 */
extern int g_sp_recolour;    /* 0x007fe998  16-bit AND mask */

/* 0x00468040 -- recolour Hit+ClipLR; 303 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00468040
void SoftBlitRLEFrameRecolour(void* dst, void* a, void* b, void* c, int h,
                              int pitch, int top, int left, int w, int spare,
                              void* mouse)
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
        je       L_4680BD
    L_46805B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_468077
        add      esi, 2
    L_468077:
        test     ebp, 55555555h
        je       L_46805B
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4680BA
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_46805B
        test     ebp, 55555555h
        je       L_4680B5
        add      esi, 2
        jmp      L_46805B
    L_4680B5:
        lea      esi, [esi+ecx*2]
        jmp      L_46805B
    L_4680BA:
        dec      edi
        jg       L_46805B
    L_4680BD:
        mov      edi, dword ptr [esp+14h]
        mov      eax, dword ptr [esp+30h]
        mov      dword ptr [esp+2ch], eax
        mov      eax, dword ptr [esp+34h]
        mov      dword ptr [esp+38h], eax
    L_4680D1:
        cmp      dword ptr [esp+30h], 0
        jle      L_46824D
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
        je       L_468237
        sub      esi, 2
        test     ebp, 55555555h
        je       L_468237
        inc      dword ptr [esp+30h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4683D1
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_468160
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_468237
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jmp      L_468237
    L_468160:
        test     ebp, 55555555h
        jne      L_4681E0
        sub      dword ptr [esp+30h], ecx
        jl       L_468179
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_468237
    L_468179:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_4681AE
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
    L_468193:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_468193
        jmp      L_468237
    L_4681AE:
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
    L_4681BC:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_4681BC
        mov      eax, dword ptr [esp+34h]
        neg      eax
        lea      esi, [esi+eax*2]
        jmp      L_468370
    L_4681E0:
        sub      dword ptr [esp+30h], ecx
        jl       L_4681EE
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_468237
    L_4681EE:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_468217
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        rep     stosw
        jmp      L_468237
    L_468217:
        add      eax, ecx
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        rep     stosw
        jmp      L_468370
    L_468237:
        cmp      dword ptr [esp+30h], 0
        jg       L_4680D1
        cmp      dword ptr [esp+34h], 0
        jle      L_468370
    L_46824D:
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
        jne      L_468283
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        mov      word ptr [edi - 2], ax
        jmp      L_468365
    L_468283:
        test     ebp, 55555555h
        je       L_468365
        inc      dword ptr [esp+34h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4683D1
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_4682CF
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_468365
    L_4682CF:
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4682E2
        or       dword ptr [g_blit_hit], 1
    L_4682E2:
        test     ebp, 55555555h
        jne      L_468331
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_46830E
        mov      dword ptr [esp+34h], eax
    L_4682F6:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_4682F6
        jmp      L_468365
    L_46830E:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        push     eax
    L_468315:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_468315
        pop      eax
        lea      esi, [esi+eax*2]
        jmp      L_468370
    L_468331:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_46834F
        mov      dword ptr [esp+34h], eax
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        rep     stosw
        jmp      L_468365
    L_46834F:
        mov      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        rep     stosw
        jmp      L_468370
    L_468365:
        cmp      dword ptr [esp+34h], 0
        jg       L_46824D
    L_468370:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_46838E
        add      esi, 2
        jmp      L_468370
    L_46838E:
        test     ebp, 55555555h
        je       L_468370
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4683D1
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_468370
        test     ebp, 55555555h
        je       L_4683CC
        add      esi, 2
        jmp      L_468370
    L_4683CC:
        lea      esi, [esi+ecx*2]
        jmp      L_468370
    L_4683D1:
        mov      eax, dword ptr [esp+14h]
        add      eax, dword ptr [esp+28h]
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        mov      ecx, dword ptr [esp+24h]
        dec      ecx
        mov      dword ptr [esp+24h], ecx
        mov      eax, dword ptr [esp+2ch]
        mov      dword ptr [esp+30h], eax
        mov      eax, dword ptr [esp+38h]
        mov      dword ptr [esp+34h], eax
        test     ecx, ecx
        jne      L_4680D1
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    LL_UNPORTED_ASM();
#endif
}

/* 0x00468410 -- highlight Hit+ClipLR; 312 instructions. */
NAKED
// FUNCTION: LEGOLAND 0x00468410
void SoftBlitRLEFrame(void* dst, void* a, void* b, void* c, int h, int pitch,
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
        je       L_46848D
    L_46842B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_468447
        add      esi, 2
    L_468447:
        test     ebp, 55555555h
        je       L_46842B
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_46848A
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_46842B
        test     ebp, 55555555h
        je       L_468485
        add      esi, 2
        jmp      L_46842B
    L_468485:
        lea      esi, [esi+ecx*2]
        jmp      L_46842B
    L_46848A:
        dec      edi
        jg       L_46842B
    L_46848D:
        mov      edi, dword ptr [esp+14h]
        mov      eax, dword ptr [esp+30h]
        mov      dword ptr [esp+2ch], eax
        mov      eax, dword ptr [esp+34h]
        mov      dword ptr [esp+38h], eax
    L_4684A1:
        cmp      dword ptr [esp+30h], 0
        jle      L_468629
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
        je       L_468613
        sub      esi, 2
        test     ebp, 55555555h
        je       L_468613
        inc      dword ptr [esp+30h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4687BC
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_468530
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+30h], ecx
        jge      L_468613
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jmp      L_468613
    L_468530:
        test     ebp, 55555555h
        jne      L_4685B6
        sub      dword ptr [esp+30h], ecx
        jl       L_468549
        lea      esi, [esi+ecx*2]
        lea      edi, [edi+ecx*2]
        jmp      L_468613
    L_468549:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_468581
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
    L_468563:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_468563
        jmp      L_468613
    L_468581:
        add      eax, ecx
        lea      esi, [esi+eax*2]
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
    L_46858F:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_46858F
        mov      eax, dword ptr [esp+34h]
        neg      eax
        lea      esi, [esi+eax*2]
        jmp      L_46875B
    L_4685B6:
        sub      dword ptr [esp+30h], ecx
        jl       L_4685C4
        add      esi, 2
        lea      edi, [edi+ecx*2]
        jmp      L_468613
    L_4685C4:
        mov      eax, dword ptr [esp+30h]
        add      dword ptr [esp+34h], eax
        jle      L_4685F0
        add      eax, ecx
        lea      edi, [edi+eax*2]
        mov      eax, dword ptr [esp+30h]
        neg      eax
        mov      ecx, eax
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        rep     stosw
        jmp      L_468613
    L_4685F0:
        add      eax, ecx
        lea      edi, [edi+eax*2]
        sub      ecx, eax
        add      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        rep     stosw
        jmp      L_46875B
    L_468613:
        cmp      dword ptr [esp+30h], 0
        jg       L_4684A1
        cmp      dword ptr [esp+34h], 0
        jle      L_46875B
    L_468629:
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
        jne      L_468662
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        mov      word ptr [edi - 2], ax
        jmp      L_468750
    L_468662:
        test     ebp, 55555555h
        je       L_468750
        inc      dword ptr [esp+34h]
        sub      edi, 2
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4687BC
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        je       L_4686AE
        lea      edi, [edi+ecx*2]
        sub      dword ptr [esp+34h], ecx
        jmp      L_468750
    L_4686AE:
        mov      eax, dword ptr [esp+3ch]
        sub      eax, edi
        sar      eax, 1
        cmp      eax, ecx
        jae      L_4686C1
        or       dword ptr [g_blit_hit], 1
    L_4686C1:
        test     ebp, 55555555h
        jne      L_468716
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_4686F0
        mov      dword ptr [esp+34h], eax
    L_4686D5:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_4686D5
        jmp      L_468750
    L_4686F0:
        neg      eax
        mov      ecx, dword ptr [esp+34h]
        push     eax
    L_4686F7:
        mov      ax, word ptr [esi]
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        mov      word ptr [edi], ax
        add      esi, 2
        add      edi, 2
        dec      ecx
        jne      L_4686F7
        pop      eax
        lea      esi, [esi+eax*2]
        jmp      L_46875B
    L_468716:
        mov      eax, dword ptr [esp+34h]
        sub      eax, ecx
        jl       L_468737
        mov      dword ptr [esp+34h], eax
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        rep     stosw
        jmp      L_468750
    L_468737:
        mov      ecx, dword ptr [esp+34h]
        mov      ax, word ptr [esi]
        add      esi, 2
        and      ax, word ptr [g_sp_recolour]
        shr      ax, 1
        rep     stosw
        jmp      L_46875B
    L_468750:
        cmp      dword ptr [esp+34h], 0
        jg       L_468629
    L_46875B:
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      ecx, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, ecx
        test     ebp, 0aaaaaaaah
        jne      L_468779
        add      esi, 2
        jmp      L_46875B
    L_468779:
        test     ebp, 55555555h
        je       L_46875B
        mov      eax, dword ptr [esp+1ch]
        movzx    ecx, byte ptr [eax]
        inc      eax
        mov      dword ptr [esp+1ch], eax
        test     ecx, ecx
        je       L_4687BC
        mov      ebp, dword ptr [edx]
        and      ebp, ebx
        rol      ebx, 2
        mov      eax, ebx
        and      ebx, 1
        lea      edx, [edx+ebx*4]
        mov      ebx, eax
        test     ebp, 0aaaaaaaah
        jne      L_46875B
        test     ebp, 55555555h
        je       L_4687B7
        add      esi, 2
        jmp      L_46875B
    L_4687B7:
        lea      esi, [esi+ecx*2]
        jmp      L_46875B
    L_4687BC:
        mov      eax, dword ptr [esp+14h]
        add      eax, dword ptr [esp+28h]
        mov      dword ptr [esp+14h], eax
        mov      edi, eax
        mov      ecx, dword ptr [esp+24h]
        dec      ecx
        mov      dword ptr [esp+24h], ecx
        mov      eax, dword ptr [esp+2ch]
        mov      dword ptr [esp+30h], eax
        mov      eax, dword ptr [esp+38h]
        mov      dword ptr [esp+34h], eax
        test     ecx, ecx
        jne      L_4684A1
        pop      ebp
        pop      ebx
        pop      esi
        pop      edi
        ret
    }
#else
    LL_UNPORTED_ASM();
#endif
}
