/* LEGOLAND sprite-layer visibility + LLS animation playback.
 *
 * The "LLS" is LEGOLAND's looping-sprite animation record. A global singly
 * linked list of {lls, next, owner} nodes (head @ 0x006691ac) holds every LLS
 * that is currently animating; LLSAuto walks it once per tick.
 *
 * Recovered LLS layout (offsets load-bearing, names ours):
 *
 *   +0x00 short  frame     current frame index
 *   +0x02 ushort delay     delay reload value (in half-ticks; see LLSAuto)
 *   +0x0c int    format    pixel format tag: 8 = single image + 256-entry
 *                          palette, 0x10 = multi-frame ILF table
 *   +0x10 short  count     number of frames
 *   +0x12 short  timer     countdown to the next frame; LLSAuto decrements by 2
 *   +0x14 uint   flags     bit0 = the frame table carries one extra frame,
 *                          bit2 = play once (LLSAuto self-stops on wrap)
 *   +0x18        payload   image header / frame table
 *
 * The same record is the argument to LLSPlay/LLSStop and to LLS555To565, which
 * is what ties +0x10/+0x14 in the playback code to +0x10/+0x14 in the pixel
 * code.  Field/type names are ours; only the offsets matter. */
#include "legoland.h"

/* ---- types -------------------------------------------------------------- */

typedef struct LLS {
    short          frame;       /* +0x00 */
    unsigned short delay;       /* +0x02 */
    unsigned char  pad4[8];     /* +0x04..0x0b */
    int            format;      /* +0x0c */
    short          count;       /* +0x10 */
    short          timer;       /* +0x12 */
    unsigned int   flags;       /* +0x14 */
    /* +0x18 payload follows */
} LLS;

/* One entry of the "currently playing" list; 12 bytes (see the push 0Ch). */
typedef struct LLSNode {
    LLS*            lls;        /* +0x00 */
    struct LLSNode* next;       /* +0x04 */
    void*           owner;      /* +0x08 the sprite object that asked to play */
} LLSNode;

/* ---- globals ------------------------------------------------------------ */

/* Head of the playing list (@ 0x006691ac). */
extern LLSNode* g_lls_playing;

/* ---- externals ---------------------------------------------------------- */

extern void* HeapAlloc_w(unsigned int size);        /* 0x0049e4ff */
extern void  HeapFree_w(void* p);                   /* 0x0049e4d0 */
extern void  DBPrintf(const char* fmt, ...);        /* 0x00453a20 */

/* -------------------------------------------------------------------------
 * 0x0047d4c0 -- unlink an LLS from the playing list and free its node.
 * Returns 1 when it actually removed something.
 *
 * The head-removal arm writes `g_lls_playing = head->next` using the SAVED
 * head (esi) while the interior arm writes `prev->next = node->next` using the
 * walk cursor (eax) -- the two are the same pointer when prev is null, which
 * is why the original reads the same field through two different registers.
 * Writing the head arm as a second read of the global (rather than through a
 * cached `head` local) is what puts the CSE'd global in the callee-saved esi
 * and the walk cursor in eax, exactly as the original allocates them.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d4c0
int LLSStop(LLS* lls)
{
    LLSNode* prev = 0;
    LLSNode* node = g_lls_playing;

    while (node) {
        if (node->lls == lls) {
            if (prev)
                prev->next = node->next;
            else
                g_lls_playing = g_lls_playing->next;
            HeapFree_w(node);
            return 1;
        }
        prev = node;
        node = node->next;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * 0x0047d520 -- start an LLS animating (idempotent: an LLS already on the list
 * is left alone).
 *
 * The `cmp cx,3E8h / jle / int 3` is a debug assert on the frame count; the
 * inline `int 3` is also why this is the only function in the cluster with an
 * EBP frame -- VC6 forces a frame pointer on any function containing __asm.
 *
 * The three initialising stores are written lls / next / owner but the ORIGINAL
 * stores them lls / owner / next: VC6's scheduler swaps the last two so the
 * global load of g_lls_playing gets a one-instruction load-use gap. Writing
 * them in the emitted order instead leaves the load glued to its use and costs
 * four instructions of mismatch -- source order here is load-bearing.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d520
void LLSPlay(LLS* lls, void* owner)
{
    LLSNode* node = g_lls_playing;
    short    count;

    if (lls) {
        count = lls->count;
        if (count > 1) {
            if (count > 1000) {
#ifndef LEGOLAND_PORTABLE
                __asm int 3
#else
                LL_DEBUGBREAK();
#endif
            }
            while (node) {
                if (node->lls == lls)
                    return;
                node = node->next;
            }
            node = (LLSNode*)HeapAlloc_w(sizeof(LLSNode));
            node->lls = lls;
            node->next = g_lls_playing;
            node->owner = owner;
            g_lls_playing = node;
        }
    }
}

/* -------------------------------------------------------------------------
 * 0x0047d580 -- play once: start it, then set the self-stop bit that LLSAuto
 * consumes when the frame counter wraps.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d580
void LLSPlayOnce(LLS* lls, void* owner)
{
    LLSPlay(lls, owner);
    lls->flags |= 4;
}

/* -------------------------------------------------------------------------
 * 0x0047d5a0 -- clamp a frame index into [0, count-1] and store it.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d5a0
void LLSSetFrame(LLS* lls, int frame)
{
    if (lls) {
        if (frame < 0)
            frame = 0;
        if (frame >= lls->count)
            frame = lls->count - 1;
        lls->frame = (short)frame;
    }
}

/* -------------------------------------------------------------------------
 * 0x0047d610 -- internal (not exported; one caller at 0x00466453). Same as
 * LLSNextFrame (0x0047d5d0) but without the null guard, and it wraps on
 * `>= count` rather than `== count`.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d610
void LLSAdvanceFrame(LLS* lls)
{
    lls->frame++;
    if (lls->frame >= lls->count)
        lls->frame = 0;
}

/* -------------------------------------------------------------------------
 * 0x0047d630 -- the per-tick animation pump. Walks the playing list; each LLS
 * counts its timer down by TWO per call, and on reaching the bottom advances a
 * frame and reloads the timer from `delay`. A play-once LLS clears its own bit
 * and unlinks itself on wrap.
 *
 * `next` is latched BEFORE the body because LLSStop can free the node.
 * ------------------------------------------------------------------------- */

static const char kBadSprite[] = "Trying to animate Bad Sprite";

// FUNCTION: LEGOLAND 0x0047d630
void LLSAuto(void)
{
    LLSNode*     node = g_lls_playing;
    LLSNode*     next;
    LLS*         lls;
    short        timer;
    unsigned int flags;

    if (node) {
        do {
            lls = node->lls;
            next = node->next;
            if (lls) {
                timer = lls->timer;
                if (timer < 2) {
                    lls->frame++;
                    if (lls->frame >= lls->count) {
                        flags = lls->flags;
                        lls->frame = 0;
                        if (flags & 4) {
                            lls->flags = flags & ~4u;
                            LLSStop(lls);
                        }
                    }
                    lls->timer = (short)lls->delay;
                } else {
                    lls->timer = (short)(timer - 2);
                }
            } else {
                DBPrintf(kBadSprite);
            }
            node = next;
        } while (next);
    }
}

/* -------------------------------------------------------------------------
 * 0x0047d6a0 -- rewrite an LLS's palette(s) from 5-5-5 to 5-6-5 in place.
 *
 *   565 = ((c & 0xFFE0) << 1) | (c & 0x1F)
 *
 * i.e. blue stays put and green+red shift up one bit, leaving the new low
 * green bit zero. Two payload shapes:
 *
 *   format 8    a single image: payload[+0x04] is the pixel-data length and a
 *               fixed 256-entry palette sits immediately after it.
 *   format 0x10 a frame table: `count` frames (plus one more when flags bit 0
 *               is set), each frame being payload[+0x00] bytes long, with
 *               payload[+0x04] palette entries starting at payload[+0x10].
 *
 * The frame walk uses a SEPARATE counter `i` copied from `frames` inside the
 * guarded scope. That split live range is what defers `push ebx / push edi`
 * past the `frames > 0` test, matching the original's sunk prologue; counting
 * `frames` down directly hoists the pushes to the top of the branch.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d6a0
void LLS555To565(LLS* lls)
{
    unsigned short* pal;
    unsigned short  c;
    int             n;
    char*           payload = (char*)lls + 0x18;

    if (lls->format == 8) {
        pal = (unsigned short*)(payload + 8 + *(int*)(payload + 4));
        n = 0x100;
        do {
            c = *pal;
            *pal++ = (unsigned short)(((c & 0xffe0) << 1) | (c & 0x1f));
        } while (--n);
        return;
    }

    if (lls->format == 0x10) {
        int frames = lls->count;
        if (lls->flags & 1)
            frames++;
        if (frames > 0) {
            char* frame = payload;
            int   i = frames;
            do {
                n = *(int*)(frame + 4);
                pal = (unsigned short*)(frame + 0x10);
                while (n > 0) {
                    c = *pal;
                    *pal++ = (unsigned short)(((c & 0xffe0) << 1) | (c & 0x1f));
                    n--;
                }
                frame += *(int*)frame;
            } while (--i);
        }
    }
}
