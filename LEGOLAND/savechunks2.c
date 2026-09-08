/* LEGOLAND -- the script EVENT serialiser pair (0x0046c700 / 0x0046c7e0).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field OFFSETS, record sizes, callee argument counts and global addresses are
 * load-bearing; the names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere).
 *
 * savechunks.c's header documents the script chunk that contains these: both
 * ends of BLK 3 stamp g_script_now (0x007fe994) with GetGameTimer() first, and
 * every serialiser in the family reports failure by BUMPING the error counter
 * g_script_errors (0x006687a0) as well as returning 0 -- LoadScripts polls the
 * counter instead of testing the return value.
 *
 * ==========================================================================
 *  The ScriptEvent record (0x44 bytes, script.c 0x00468910/40/70)
 * ==========================================================================
 * savechunks.c calls the pair "over the pending event 0x00668784", singular.
 * It is actually a LIST: SaveScriptEvent walks +0x00 and writes one 0x44-byte
 * record per node, and LoadScriptEvent rebuilds the chain. The stream is
 * terminated by a record whose +0x00 is -1, so the link field doubles as the
 * end-of-list sentinel on disk (the writer's terminator is an all-zero record
 * with +0x00 = -1; the reader stops on `next == (ScriptEvent*)-1`).
 *
 *   +0x00  next        chain link; -1 on disk terminates the list
 *   +0x04  elem        LLIDB element the event refers to. NOT serialised as a
 *                      pointer: the writer stores elem->name (a <script
 *                      string>) and the reader resolves it with ElemID().
 *   +0x08  text        an owned string, stored as a <script string>
 *   +0x0c  kind        set from NewScriptEvent's 1st argument
 *   +0x10  flags       bit 0x20 = "text is heap-owned"; FreeScriptEvent
 *                      (0x00468940) frees +0x08 only when that bit is set, so
 *                      the loader must set it after a successful read
 *   +0x38  param       set from NewScriptEvent's 2nd argument
 *   +0x3c  time        absolute game time; rebased to RELATIVE across the
 *                      write and put back afterwards, and rebased back to
 *                      absolute on the read
 *
 * Per node the stream is:  u8 rec[0x44] ; <script string> elem name (NULL when
 * the event has no element) ; <script string> text.  Both strings always go
 * out -- a NULL field is written as SaveScriptString(NULL), which the string
 * serialiser stores as length -1.
 *
 * The writer's on-disk +0x3c is `time - g_script_now`, so a saved game is
 * portable across the absolute clock; the reader adds the loading session's
 * g_script_now straight back. Both ends trace the value with DBPrintf, which
 * is why the arithmetic is visible twice at each site.
 *
 * ==========================================================================
 *  KNOWN ORIGINAL DEFECTS, reproduced faithfully
 * ==========================================================================
 *  - SaveScriptEvent leaves the LAST node's +0x3c rebased correctly but
 *    mutates the caller's live list while writing (the -= / += pair straddles
 *    the SaveGameWrite); an abort between the two leaves +0x3c relative. The
 *    += is executed before the write's result is tested, so only a crash
 *    inside SaveGameWrite can expose it. Reproduced as written.
 *  - LoadScriptEvent's "errors after the element name" path frees the record
 *    with free() and LEAKS the name string it just read (the `name` local is
 *    dropped on the floor). The sibling path after the text string uses the
 *    real destructor FreeScriptEvent. Reproduced.
 *  - LoadScriptEvent does not check NewScriptEvent's result: 0x00468910
 *    returns NULL when calloc fails and the very next SaveGameRead reads
 *    through it. Reproduced (no NULL test exists in the original).
 * ==========================================================================
 */

#include "legoland.h"
#include <string.h>

#pragma intrinsic(memset)

/* ---- local types -------------------------------------------------------- */

/* LLElem (the LLIDB element, name at +0x00) comes from legoland.h. */

typedef struct ScriptEvent {
    struct ScriptEvent* next;  /* +0x00 */
    LLElem*             elem;  /* +0x04 */
    char*               text;  /* +0x08 */
    int                 kind;  /* +0x0c */
    unsigned char       flags; /* +0x10 */
    unsigned char       pad11[0x38 - 0x11];
    int                 param; /* +0x38 */
    int                 time;  /* +0x3c */
    int                 f40;   /* +0x40 */
} ScriptEvent;                 /* 0x44 */

/* ---- globals ------------------------------------------------------------ */

/* GetGameTimer() at the start of SaveScripts / LoadScripts; every stored time
 * is relative to it. */
extern int g_script_now;                          /* 0x007fe994 */
/* Bumped by every script serialiser that fails. */
extern int g_script_errors;                       /* 0x006687a0 */

/* ---- callees ------------------------------------------------------------ */

extern void  DBPrintf(const char* fmt, ...);                  /* 0x00453a20 */
extern int   SaveGameWrite(const void* buf, unsigned int n);  /* 0x0047d760 */
extern int   SaveGameRead(void* buf, unsigned int n);         /* 0x0047d730 */
extern int   SaveScriptString(const char* s);                 /* 0x0046c620 */
extern char* LoadScriptString(void);                          /* 0x0046c680 */
extern LLElem* ElemID(const char* name);                      /* 0x0047b3f0 */

/* script.c's ScriptEvent allocator/destructors. NewScriptEvent calloc's 0x44
 * bytes and stores its two arguments in +0x0c and +0x38. */
extern ScriptEvent* NewScriptEvent(int kind, int param);      /* 0x00468910 */
extern void  FreeScriptEvent(ScriptEvent* ev);                /* 0x00468940 */
extern void  FreeScriptEventList(ScriptEvent* ev);            /* 0x00468970 */

/* Statically-linked CRT free (>= 0x0049e000): NOT a decompilation target. */
extern void  free(void* p);                                   /* 0x0049e4d0 */

/* ---- the writer --------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0046c700
int SaveScriptEvent(ScriptEvent* ev)
{
    ScriptEvent term;
    int         ok;

    while (ev) {
        ev->time -= g_script_now;
        DBPrintf("Saving Event, Timestamp = %d\n", ev->time);
        ok = SaveGameWrite(ev, 0x44);
        /* Put the absolute time back BEFORE the failure test -- the original
         * schedules the restore ahead of the branch. */
        ev->time += g_script_now;
        if (!ok)
            return 0;
        if (ev->elem) {
            if (!SaveScriptString(ev->elem->name))
                return 0;
        } else {
            if (!SaveScriptString(0))
                return 0;
        }
        if (ev->text) {
            if (!SaveScriptString(ev->text))
                return 0;
        } else {
            if (!SaveScriptString(0))
                return 0;
        }
        ev = ev->next;
    }
    memset(&term, 0, sizeof(term));
    term.next = (ScriptEvent*)-1;
    return SaveGameWrite(&term, 0x44) != 0;
}

/* ---- the reader --------------------------------------------------------- */

/* 0x0046c7e0 -- LoadScriptEvent.  All 124 instructions are present and every
 * block is right; the ONLY residual is that THREE COLD BLOCKS ARE PERMUTED, so
 * audit.py's index-for-index gate fails from index 95 (mismatch 26).
 *
 *   original    readfail 0x46c8ae | terminator 0x46c8cf | err-after-name
 *               0x46c8ea | err-after-text 0x46c900 | `head = 0` 0x46c916
 *   this body   readfail | terminator | `head = 0` | err-after-name |
 *               err-after-text
 *
 * VC6 puts the `else head = 0;` arm IMMEDIATELY after the terminator block it
 * belongs to; the original has it LAST, after both error blocks.  Measured this
 * round over ~45 spellings (structured if/else in both polarities and both arm
 * orders, `goto` with the label first/last/between the error handlers, a shared
 * `out:` join placed at five different points, the loop body moved into the
 * `else` of the terminator test, a `switch`, a ternary, and the terminator and
 * both error handlers each inline or as labels):
 *
 *  - VC6 lays cold blocks out in SOURCE-GENERATION order, and a SINGLE-
 *    PREDECESSOR `goto` target is generated INLINE with the branch that reaches
 *    it -- so the arm always lands adjacent to its `if`, no matter where the
 *    label is written.  (The two error blocks obey the same rule and therefore
 *    land in source order after it.)
 *  - The one shape that DOES generate the block last -- `if (prev == 0) goto L;
 *    ... return head;` with `L: head = 0; return head;` textually after the
 *    error handlers -- is CONST-FOLDED: VC6 propagates the 0 into the return,
 *    rewrites it as `return 0` (`xor eax,eax`) and cross-jumps it into the
 *    error handler's tail, so the block disappears entirely (117 instructions).
 *    The fold survives `return head = 0;`, `*&head = 0;`, a cast, `memset`, a
 *    second coalesced variable, `head = prev;` (VC6 knows prev == 0 from the
 *    compare) and splitting the store from the return across a label or a
 *    `goto` -- its const-propagation is global, not block-local.
 *  - The original's `xor ebx,ebx / ... / mov eax,ebx` proves the 0 was NOT
 *    folded there, i.e. the original really is a two-predecessor join: a phi
 *    copy in one arm plus a tail-duplicated `return head`.  The body below IS
 *    that join (VC6 duplicates the single `return head` into both arms and
 *    emits the phi copy) -- but every spelling that keeps the value unfolded
 *    also drags the block back next to the `if`, and the two effects are
 *    rigidly coupled: forcing the block to stay last inverts the branch
 *    (`jne` at index 87 and the block at 88 instead of `je` and 117).
 *
 * Residual class (HANDOFF 6B): STRUCTURAL -- a pure cold-block permutation, not
 * allocation, frame or scheduling.  Everything else is exact: the rotated
 * `while (SaveGameRead(...))` with the allocate-and-read duplicated at the loop
 * bottom, both DBPrintf traces and the timestamp rebase between them, the byte
 * OR of flag 0x20, and the free()/FreeScriptEvent asymmetry of the two error
 * paths. */
/* Scope I (2026-09-05): at its measured cold-block-order floor,
 * 26/124 strict, first 95, 323/319 bytes. Moving the entire terminator
 * handler out of the loop behind a goto gives the SAME object. A saved read
 * result with a break and shared post-loop dispatch gives 37 or 47 by
 * polarity, moving the first mismatch to 85/69. The original two-predecessor
 * head-zero join cannot be placed last by these forms; keep the proven
 * ownership cleanup and name-error leak rather than trading them for score.
 * Full measurements: docs/lanes/scope-i.md.
 */
/* Scope LL18 (2026-09-08): unchanged, 26/124, first 95.  Read against the
 * layout: the original's cold blocks sit in the order of the branch that
 * reaches each -- terminator (from 17), err-after-name (34), err-after-text
 * (52), then `head = 0` (from 87, INSIDE the terminator block) -- so the
 * head-zero block is the ONLY cold block not placed adjacent to its single
 * predecessor.  Six more forms confirm the two walls above: `if (prev == 0)
 * goto empty; prev->next = 0; goto done;` with `empty: head = 0; done:
 * return head;` textually after the read-failure return (15 -- VC6 pulls
 * the single-predecessor block back next to the terminator and INVERTS the
 * branch), the same with `if (prev) { ...; goto done; } goto empty;` (14,
 * block adjacent again); a `for (;;)` with the read test at the top and
 * `if (prev) { prev->next = 0; return head; } break;` plus a post-loop
 * `head = 0; return head;` / `return head;` / the `prev == 0` polarity (all
 * 117 instructions: the zero is folded into `return 0` and cross-jumped into
 * the last error handler's tail, as recorded); and the same `for (;;)` with
 * `if (prev) prev->next = 0; else head = 0; break;` (the committed object
 * exactly).  A block that VC6 leaves last must have had a second predecessor
 * or a non-foldable value at layout time, and no C reaching this instruction
 * stream provides one.  Floor stands. */
// WIP-FUNCTION: LEGOLAND 0x0046c7e0  (79.0%, 26/124 strict; cold-block ordering floor; first 95)
ScriptEvent* LoadScriptEvent(void)
{
    ScriptEvent* head;
    ScriptEvent* prev;
    ScriptEvent* ev;
    char*        name;

    head = 0;
    prev = 0;
    ev = NewScriptEvent(0, 0);
    while (SaveGameRead(ev, 0x44)) {
        if (ev->next == (ScriptEvent*)-1) {
            /* End of the stream: the sentinel record is not part of the list. */
            free(ev);
            if (prev)
                prev->next = 0;
            else
                head = 0;
            return head;
        }
        DBPrintf("Loading Event, TimeStamp = %d", ev->time);
        ev->time += g_script_now;
        DBPrintf("Fixed up to %d\n", ev->time);
        name = LoadScriptString();
        if (g_script_errors) {
            /* ORIGINAL BUG, reproduced: this path drops `name` on the floor
             * (it leaks) and releases the record with the raw CRT free()
             * rather than FreeScriptEvent -- which is correct only because
             * +0x08 has not been filled in yet. */
            free(ev);
            FreeScriptEventList(head);
            return 0;
        }
        if (name) {
            ev->elem = ElemID(name);
            free(name);
        } else {
            ev->elem = 0;
        }
        ev->text = LoadScriptString();
        if (ev->text)
            ev->flags |= 0x20;   /* +0x10 bit 0x20: FreeScriptEvent owns +0x08 */
        if (g_script_errors) {
            /* The record owns its text now, so the real destructor is used. */
            FreeScriptEvent(ev);
            FreeScriptEventList(head);
            return 0;
        }
        if (!head)
            head = ev;
        else
            prev->next = ev;
        prev = ev;
        ev = NewScriptEvent(0, 0);
    }
    /* Short read: the half-built list is destroyed and the error counter that
     * LoadScripts polls (instead of the return value) is bumped. */
    free(ev);
    FreeScriptEventList(head);
    g_script_errors++;
    return 0;
}
