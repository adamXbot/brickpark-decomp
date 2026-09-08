/* LEGOLAND -- scope Q: the two cursor-segment fills RenderCursor (bigrender.c)
 * draws the build cursor's outline with.  Both walk one tile edge in 16-bit
 * pixels through the video surface, painting a marching-ants pattern whose
 * phase is the global frame counter: B is solid (12 pixels of the first
 * colour, 4 of the second), A is transparent (4 of each colour in 32, the
 * rest untouched).  kind 0 is the vertical edge going up, 1 and 2 the two
 * diagonals; the diagonal walks step x every pixel and y every other pixel.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-q.md.
 */
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;
typedef struct VideoSurfaceInfo {
    long  pitch;                /* +0x00  bytes per scanline */
    int   width;                /* +0x04 */
    int   height;               /* +0x08 */
    void* bits;                 /* +0x0c */
    int   unused;               /* +0x10 */
    int   format;               /* +0x14  2 = 16-bit */
} VideoSurfaceInfo;

extern int       g_cursor_phase;                  /* 0x007cacd4  the marching-ants frame counter */
extern ClipRect  g_clip;                          /* 0x004bdea0  SPRITE_ClipRect */

extern int GetNearestColour(int r, int g, int b);                  /* 0x0044e6c0 */
__declspec(dllimport) int __stdcall PtInRect(const ClipRect* r, int x, int y);   /* [0x4ab2c0] */

/* WIP. audit: ours 160i/460B, original 160i/458B, mismatch 93; matchfull
 * 145/154. ONE block-layout difference, everything else identical: after the
 * kind chain (`sub eax,0 / je vertical / dec / je case1 / dec / je case2`)
 * the original falls through into a CLONED exit epilogue for the default
 * arm, then places case 1 (`dir = -1; jmp`) and case 2 (`dir = 1`, adjacent
 * to the shared diagonal loop); ours inverts the last test (`jne end`) and
 * places case 2 first, so case 1 becomes the loop's fall-through and the
 * default arm shares the function's end epilogue. First diverging index 63.
 * Measured identical (17 spellings): default first/last/absent, `return`
 * vs `break` vs `goto` to an end label, empty default (`{}` / `;`), dead
 * stores in the default (`dir = 0`, `h = 0`, `phase++`), case 2 before
 * case 1 in both label and break forms, the vertical loop inside case 0
 * (first or last) or after the switch via `goto`, both cases jumping to a
 * label, `h++; while (h--)`, and `if (kind == 0) goto` before a 1/2 switch
 * (that one breaks the chain: 92/154). The shape `dec / je / four pops /
 * add esp / ret` exists nowhere else in the binary (both cursor fills have
 * it), so there is no template. STRUCTURAL by class, but the lever has not
 * been found.
 *
 * LL19 (2026-09-08), ~40 more spellings in a scratch harness, at its floor.
 * MECHANISM: the default arm is a void `return`, i.e. an EMPTY block whose
 * only content is the jump to the exit; VC6 threads it away before block
 * layout, which leaves the last test with the exit as fall-through and
 * inverts it (`jne end`, case 2 falls through). Any real instruction in
 * the default arm (a `volatile` read of `flip`, a store to a global) keeps
 * the block, and THEN the chain falls through into a cloned epilogue exactly
 * as the original -- so the shape is reachable only with a code-bearing
 * default, and the original's default has no code. Every zero-cost pin was
 * threaded with the block: `if (h) ;`, `if (vs) ;`, `if (h) return;`,
 * `while (0) ;`, `for (;;) return;`, `h = h;`, a dead store to flip / pat[0]
 * / phase / x / c / p, a dead read of pat[0] / col[0] / vs->pitch, an
 * unreferenced label, `{}`, a comma of casts, `goto` to the post-diagonal
 * `return` label, a nested `switch (kind)` / `switch (h)` default, a shared
 * `case 3:`; `__asm { }` keeps the block but gives the function an EBP
 * frame. Also identical to the baseline: the diagonal loop as the switch's
 * break join (both arms `break`), the vertical loop inside case 0 with
 * `return` or `break`, every order of {0, 1, 2, default} with and without a
 * default, `dir` pre-assigned before the switch (VC6 hoists the store above
 * the chain, never sinks it), and a switch of `goto`s dispatching to
 * labelled arms. SECOND OBSTACLE: whenever the default does fall through,
 * VC6 lays the remaining arms in DESCENDING case value (2 then 1, case 0
 * last) whatever their source order or which arm carries the goto -- the
 * same order as the four matched chain-fall-through templates in the binary
 * (GetTransparentColour / GetNearestColour 0x0044e690/0x0044e6c0,
 * CurLevelFlags 0x00478610, SelectFont 0x00454b40) -- while the original
 * has case 1 (`dir = -1; jmp`) BEFORE case 2. No construct produced the
 * ascending order. Flags do not reach it either: /O1, /Os, /Ox, /Ob0, /Oy-,
 * /Oa, /Ow, /Gf, /Gs, /GB, /G3-/G6, /Zp1 are all equal or worse. The 9
 * residual instructions (LCS, all blinds equal) are exactly this layout. */
// WIP-FUNCTION: LEGOLAND 0x0045fad0  (94.2%, the switch's default arm: the original clones the exit epilogue as the chain's fall-through and keeps case 1 then case 2; ours inverts the last test)
void DrawCursorSegmentB(VideoSurfaceInfo* vs, int kind, int x, int y, const unsigned char* col, int h)
{
    int            flip = 0;
    unsigned short pat[16];
    int            c1, c2, i, phase, dir;
    unsigned short c;
    unsigned char* p;
    unsigned char* q;

    c1 = GetNearestColour(col[0], col[1], col[2]);
    c2 = GetNearestColour(col[4], col[5], col[6]);
    phase = g_cursor_phase;
    pat[0] = (unsigned short)c1;
    pat[1] = (unsigned short)c1;
    pat[2] = (unsigned short)c1;
    pat[3] = (unsigned short)c1;
    pat[4] = (unsigned short)c1;
    pat[5] = (unsigned short)c1;
    pat[6] = (unsigned short)c1;
    pat[7] = (unsigned short)c1;
    pat[8] = (unsigned short)c1;
    pat[9] = (unsigned short)c1;
    pat[10] = (unsigned short)c1;
    pat[11] = (unsigned short)c1;
    pat[12] = (unsigned short)c2;
    pat[13] = (unsigned short)c2;
    pat[14] = (unsigned short)c2;
    pat[15] = (unsigned short)c2;
    if (vs->format != 2)
        return;
    p = (unsigned char*)vs->bits + vs->pitch * y + x * 2;
    q = p - vs->pitch;
    switch (kind) {
    default:
        return;
    case 1:
        dir = -1;
        goto diagonal;
    case 2:
        dir = 1;
    diagonal:
        i = h + 1;
        while (i--) {
            if (PtInRect(&g_clip, x, y)) {
                c = pat[phase & 0xf];
                *(unsigned short*)p = c;
                if (y > 0)
                    *(unsigned short*)q = c;
            }
            x += dir;
            p += dir * 2;
            q += dir * 2;
            if (flip) {
                y++;
                q = p;
                p += vs->pitch;
            }
            flip ^= 1;
            phase++;
        }
        return;
    case 0:
        break;
    }
    while (h--) {
        if (PtInRect(&g_clip, x, y)) {
            c = pat[phase & 0xf];
            *(unsigned short*)p = c;
            if (x > 0)
                *(unsigned short*)(p - 2) = c;
        }
        y--;
        p -= vs->pitch;
        phase++;
    }
}

/* WIP. audit: ours 195i/608B, original 195i/606B, mismatch 111 (positional;
 * the whole count is the block shift below); matchfull 180/189 = 95.2%,
 * LCS residual 9 with strict == rb == ob. LL19 (2026-09-08): the diagonal
 * loop's head must be spelled `h++; while (h--)` -- the counted-down copy
 * `i = h + 1; while (i--)` gives the same code in DrawCursorSegmentB (where
 * x and y live in ebp/ebx) but here, with x, y, phase, flip and the count
 * all memory-homed, the extra name made VC6 load h into edx and form h + 1
 * with `lea`, and that one scratch choice rotated eax/ecx/edx through the
 * whole loop (the count copy, the PtInRect argument loads, the pattern
 * index, the x step) and re-scheduled the `if (flip)` block so the pitch
 * load folded into the add. With `h` stepped in place the loop, its
 * preheader, the `mov ecx,[eax] / mov [y],edx / add esi,ecx` pitch shape
 * and the vertical loop are all instruction-, register- and offset-exact.
 * (`*(volatile int*)&h + 1` in the head reaches the same rotation, 180/190,
 * 607B; `while (h-- >= 0)` 176/188.) Measured inert or worse here: the
 * flip block as `q = p; p += vs->pitch; y++` or `q = p; y++; p += pitch`
 * or with a named `pitch` local (only the fold moves, 147/189 at best),
 * `y = y + 1`, `for (i = h + 1; i--; )`, `i = h; i++;`, and free volatile
 * reads of flip (86), phase (146), y in the test (146), x (133), dir (88),
 * vs (95). The residual is exactly DrawCursorSegmentB's switch layout --
 * see its note for the mechanism and why it is at its floor. */
// WIP-FUNCTION: LEGOLAND 0x0045fca0  (95.2%, only the DrawCursorSegmentB default-arm layout: the chain falls through into a cloned epilogue and keeps case 1 then case 2)
void DrawCursorSegmentA(VideoSurfaceInfo* vs, int kind, int x, int y, const unsigned char* col, int h)
{
    int            flip = 0;
    unsigned short pat[32];
    int            c1, c2, phase, dir;
    unsigned short c;
    unsigned char* p;
    unsigned char* q;

    c1 = GetNearestColour(col[0], col[1], col[2]);
    c2 = GetNearestColour(col[4], col[5], col[6]);
    phase = g_cursor_phase;
    pat[0] = 0;
    pat[1] = 0;
    pat[2] = 0;
    pat[3] = 0;
    pat[4] = 0;
    pat[5] = 0;
    pat[6] = 0;
    pat[7] = 0;
    pat[8] = 0;
    pat[9] = 0;
    pat[10] = 0;
    pat[11] = 0;
    pat[12] = (unsigned short)c1;
    pat[13] = (unsigned short)c1;
    pat[14] = (unsigned short)c1;
    pat[15] = (unsigned short)c1;
    pat[16] = 0;
    pat[17] = 0;
    pat[18] = 0;
    pat[19] = 0;
    pat[20] = 0;
    pat[21] = 0;
    pat[22] = 0;
    pat[23] = 0;
    pat[24] = 0;
    pat[25] = 0;
    pat[26] = 0;
    pat[27] = 0;
    pat[28] = (unsigned short)c2;
    pat[29] = (unsigned short)c2;
    pat[30] = (unsigned short)c2;
    pat[31] = (unsigned short)c2;
    if (vs->format != 2)
        return;
    p = (unsigned char*)vs->bits + vs->pitch * y + x * 2;
    q = p - vs->pitch;
    switch (kind) {
    default:
        return;
    case 1:
        dir = -1;
        goto diagonal;
    case 2:
        dir = 1;
    diagonal:
        h++;
        while (h--) {
            if (PtInRect(&g_clip, x, y)) {
                c = pat[phase & 0x1f];
                if (c)
                    *(unsigned short*)p = c;
                if (y > 0 && c)
                    *(unsigned short*)q = c;
            }
            x += dir;
            p += dir * 2;
            q += dir * 2;
            if (flip) {
                y++;
                q = p;
                p += vs->pitch;
            }
            flip ^= 1;
            phase++;
        }
        return;
    case 0:
        break;
    }
    while (h--) {
        if (PtInRect(&g_clip, x, y)) {
            c = pat[phase & 0x1f];
            if (c)
                *(unsigned short*)p = c;
            if (x > 0 && c)
                *(unsigned short*)(p - 2) = c;
        }
        y--;
        p -= vs->pitch;
        phase++;
    }
}
