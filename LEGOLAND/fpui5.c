/* LEGOLAND -- the side panel, the pop-up and the help queue: five leaf
 * routines the front-end files declare but do not own.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  None of
 * these functions is exported; every extent comes from the disassembly by
 * control flow (tools/audit.py).  Struct field OFFSETS, record sizes and
 * global addresses are load-bearing; the names are ours.  Types are declared
 * LOCALLY on purpose (legoland.h is owned elsewhere) and follow fpui.c,
 * fpui2.c, fpui3.c, fpui4.c, iconui.c, bighelp.c and popup2.c.
 *
 *   0x0048a840  FreePlayItemUpdate     41/41 insns,  88/88 B,   exact
 *   0x00468c00  KillObjectHelp         45/45 insns, 123/123 B,   exact
 *   0x004734d0  PU_Delete2Input        45/45 insns, 135/135 B,   exact
 *   0x004755c0  InsertObjectNode       45/45 insns, 104/104 B,   exact
 *   0x00471ca0  RemoveNewObjectMarker  53/53 insns, 155 B vs 154, 5 mismatches
 *                                      (the inner shift loop's cursor anchor;
 *                                       see the note above its marker)
 *
 * =========================================================================
 * WHAT THIS FILE ADDS TO THE PICTURE  (spec notes for a browser runtime)
 * =========================================================================
 * THE FREE-PLAY TABLE IS SEARCHED BY NAME AND TERMINATED BY AN EMPTY STRING.
 * `g_fp_table` (0x004bdeb8) holds 133 sixteen-byte rows {id, name, cost,
 * chosen} followed by a row whose `name` points at "" -- and THAT, not the
 * count, is what ends every walk over it: the loop condition is
 * `strlen(e->name)`, inlined as `repne scasb`.  0x0048a840 is the one lookup
 * all three free-play money routines go through (fpui3.c records
 * FreePlayItemAvailable 0x0048aef0 and the charge/refund pair 0x0048af40 /
 * 0x0048afa0 as its callers): given a class NAME it returns that class's
 * free-play brick cost -- HEDGE 32, PALM TREE 27, WATERPUMP 59, WATER WORKS
 * WATER BLOCK 171 -- and, through an optional out-pointer, the row itself so
 * the caller can flip the row's "chosen" word.  A name that is not in the
 * table costs 0 and yields no row, so an unknown class is free.
 * The comparison is the case-insensitive `_stricmp` (0x004aab90) the whole
 * LLIDB side of the game uses, not `strcmp`.
 *
 * OBJECT HELP IS A QUEUE OF ScriptEvent RECORDS, and its text can carry a
 * SPEECH FILE.  The queue head is 0x00668724 -- a second list of the same
 * 0x44-byte record fpui3.c documents at 0x00668784 -- and the record's `text`
 * (+0x08) is split on an '@':
 *
 *      "Watch out for the queue!@advisor12"
 *       \___ advisor bubble text ___/ \_ narration file _/
 *
 * The '@' is overwritten with a NUL so the bubble gets only the left half,
 * and the right half is handed to the media module (0x00498630, which takes
 * a 0x404-byte path buffer) between a stop/restart pair (0x00498920 /
 * 0x00498b00) that ducks whatever is already playing.  The help face state
 * (0x006687a4) then goes to 4.
 *
 * The queue is only popped when DisplayAdvisorHelp actually accepts the text
 * -- it refuses while advisor help is already up -- so an item stays at the
 * head, and the '@' split runs AGAIN on the truncated string next frame
 * (finding no '@' the second time, so the narration plays once).  Its second
 * argument is `kind == 0`, i.e. "this is a plain object-help item", and the
 * pop happens through the GLOBAL (`g_object_help = g_object_help->next`), not
 * through the local cursor.
 *
 * THE NEW-OBJECTS STRIP IS ONE 0xa8-BYTE RECORD, not four globals: two
 * parallel 20-slot arrays -- the class definitions at 0x007fded4 and their
 * preview sprites at 0x007fdf24 -- followed by the live count at 0x007fdf74
 * and the highlighted index at 0x007fdf78.  That it is one object is read
 * straight off the codegen: a SINGLE cursor reaches both arrays (`[esi]` and
 * `[esi-0x50]`), and the shift's stores force `count` to be reloaded from
 * memory on every iteration, which is the alias kill a store into the same
 * object produces.  Removing one entry kills its sprite, shifts BOTH arrays
 * down over the hole, clamps the highlight to the new last slot and -- when
 * the strip empties -- closes the pop-up if it was in the strip's own state
 * 2.  Twenty is the strip's capacity; nothing here checks it.
 *
 * ORIGINAL BUG, reproduced: the scan does not stop or step back after a
 * removal.  Slot `i` now holds what used to be slot `i+1`, but the loop
 * advances to `i+1` regardless, so the entry that moved into the hole is
 * never examined.  Harmless only because a class appears in the strip once.
 *
 * THE OBJECT LIST IS KEPT SORTED BY COST, CHEAPEST FIRST.  InsertObjectNode
 * allocates a 12-byte {next, obj, keep} node with `keep` = 1 -- the opposite
 * of InsertChildIntoList's 0, which is what distinguishes a top-level entry
 * from a child -- and walks the list until it finds a node that costs MORE
 * than the new one, inserting before it.  A node whose cost ties goes AFTER
 * the incumbent (the test is a strict `>`), so equal-priced classes keep
 * their insertion order.
 *
 * THE POP-UP'S SECOND DELETE BUTTON cancels a WORK ORDER rather than an
 * object.  The order being shown lives at 0x007fdf80 ({+0x04 object element,
 * +0x08 map position}) and the pop-up's kind word (0x007fdf9c) picks the
 * queue: 0x10b is the gardeners', anything else the mechanics'.  Both arms
 * clear the object's work-order render flag first, so the map stops drawing
 * the marker, and both end by clearing the whole info block, which closes the
 * panel.
 * ========================================================================= */

#include <string.h>

#pragma intrinsic(strlen)

typedef struct Pos    { int x; int y; } Pos;
typedef struct Icon   Icon;
typedef struct Sprite Sprite;
typedef struct ObjDef ObjDef;

/* =========================================================================
 * 0x0048a840 -- look a class up in the free-play table.
 *
 * fpui.c declares this `void FreePlayItemUpdate(int value, int a)` and calls
 * it as `FreePlayItemUpdate(p->u1c.value, 0)` -- the icon's +0x1c union read
 * through its `int` member while it really holds the class NAME.  The types
 * here are what this body's disassembly needs (`_stricmp` on the first
 * argument, a store through the second); the ABI is identical and no other
 * file is touched.  See the file header for what the row means.
 *
 * LEVERS:
 *  - `while (strlen(e->name))` under `#pragma intrinsic(strlen)` is the whole
 *    loop control: the inlined `or ecx,-1 / xor eax,eax / repne scasb /
 *    not ecx / dec ecx` leaves the length in the FLAGS, so the guard is a
 *    bare `je` and the latch a bare `jne` with no compare of its own.  The
 *    rotated latch loads `e[1].name` (`mov edi,[esi+0x14]`) BEFORE the
 *    `e++`, which is the `while` rotation, not a source `e[1]`.
 *  - The found block is EXILED past the not-found tail: the loop's
 *    fall-through exit is `return 0` and the hit is a forward `je`.  That is
 *    the same layout LFRun_FindWaitingBoat has in logflume7.c and it falls
 *    out of `if (hit) { ...; return X; }` inside the loop with a single
 *    trailing `return 0`.
 *  - `name` is read from its argument slot INSIDE the loop preheader, i.e.
 *    after the zero-trip guard -- the recorded rule that a stack argument's
 *    read is CSE'd function-wide and sinks to its first real use.
 * ========================================================================= */

/* The free-play object table @ 0x004bdeb8 (fpui2.c's FPTableEntry), 133 rows
 * plus an empty-name terminator. */
typedef struct FPTableEntry {
    unsigned char id;      /* +0x00 */
    char          pad01[3];
    char*         name;    /* +0x04  the LLIDB class name */
    int           cost;    /* +0x08  free-play brick cost */
    int           chosen;  /* +0x0c  runtime: in the player's park */
} FPTableEntry;

extern FPTableEntry g_fp_table[0x86];                            /* 0x004bdeb8 */

/* The case-insensitive compare the whole LLIDB side uses (castleobj.c,
 * interfaces.c and loaders.c all declare 0x004aab90 this way). */
extern int NameCompare(const char* a, const char* b);            /* 0x004aab90 (_stricmp) */

// FUNCTION: LEGOLAND 0x0048a840
int FreePlayItemUpdate(const char* name, FPTableEntry** out)
{
    FPTableEntry* e = g_fp_table;

    while (strlen(e->name)) {
        if (NameCompare(name, e->name) == 0) {
            if (out)
                *out = e;
            return e->cost;
        }
        e++;
    }
    return 0;
}

/* ---- the object-help queue (fpui3.c owns the record) -------------------- */

/* A 0x44-byte scripted event; only the two fields this reads are named. */
typedef struct ScriptEvent {
    struct ScriptEvent* next;    /* +0x00 */
    void*               elem;    /* +0x04 */
    char*               text;    /* +0x08  owned string, may carry "@file" */
    int                 kind;    /* +0x0c */
    char                pad10[0x44 - 0x10];
} ScriptEvent;                   /* 0x44 */

extern ScriptEvent* g_object_help;                               /* 0x00668724 */

/* 0x00468940 (not exported, fpui3.c): free an event record and its string. */
extern void FreeScriptEvent(ScriptEvent* e);                     /* 0x00468940 */
/* DIVERGENCE, deliberate: iconui.c defines 0x0046ce60 with TWO parameters
 * (`text`, `arg`); this caller pushes THREE.  Under __cdecl the extra
 * argument is harmless and the callee simply ignores it, but the push is in
 * the original and has to be in the source.  Do not "align" either side. */
extern int DisplayAdvisorHelp(const char* t, int plain, int x); /* 0x0046ce60 */
/* The media module's duck / play / restore trio.  0x00498630 takes a path in
 * a 0x404-byte buffer, so it is the file player; the other two gate on the
 * same playback-state global (0x0079a84c) and bracket it.  Names are ours. */
extern void PauseCurrentTrack(void);                             /* 0x00498920 */
extern void PlayNarrationFile(const char* path);                 /* 0x00498630 */
extern void ResumeCurrentTrack(void);                            /* 0x00498b00 */
/* 0x0046d3a0 (not exported): put the help face state (0x006687a4) to 4; its
 * neighbour at 0x0046d3b0 puts it to 6. */
extern void SetHelpFaceTalking(void);                            /* 0x0046d3a0 */
/* 0x00444070 (not exported): set the advisor's pose (0x00665fec) and its
 * argument (0x0081c09c), and reset the pose timer (0x0081c088). */
extern void SetAdvisorPose(int pose, int arg);                   /* 0x00444070 */

/* =========================================================================
 * 0x00468c00 -- show the next object-help item, and pop it if it took.
 *
 * iconui.c's ProcessInGameHelp calls this once per frame while no advisor
 * text is up and ObjectHelpExpired says the last one is done.  See the file
 * header for the '@' narration split and for why the item is only popped on
 * a successful DisplayAdvisorHelp.
 *
 * LEVERS:
 *  - `push edi` sinks past the `if (p)` guard because `at` is first defined
 *    inside it, and the matching `pop edi` lands between the
 *    DisplayAdvisorHelp result test and its branch.
 *  - `at + 1` is emitted as `inc edi` in place, between the two media calls:
 *    `at` is dead afterwards, so no copy is made.  Spelling it `at++` first
 *    would be the same object; what matters is that `*at = 0;` comes before
 *    the pause call, so the store cannot migrate across it.
 *  - **The pop reads the GLOBAL twice** (`mov eax,[0x668724] / mov ecx,[eax] /
 *    mov [0x668724],ecx`) even though the local cursor holds the same value:
 *    `g_object_help = g_object_help->next;` is what the source says, and a
 *    `p->next` spelling would use the register already in esi.
 *  - SetAdvisorPose's two pushes are not cleaned on their own; they merge
 *    with FreeScriptEvent's into one `add esp,0xc`.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x00468c00
void KillObjectHelp(void)
{
    ScriptEvent* p = g_object_help;
    char*        at;

    if (p) {
        at = strchr(p->text, '@');
        if (at) {
            *at = 0;
            PauseCurrentTrack();
            PlayNarrationFile(at + 1);
            ResumeCurrentTrack();
            SetHelpFaceTalking();
        }
        if (DisplayAdvisorHelp(p->text, p->kind == 0, 0)) {
            SetAdvisorPose(5, 0);
            g_object_help = g_object_help->next;
            FreeScriptEvent(p);
        }
    }
}

/* ---- the info pop-up's work-order delete ------------------------------- */

/* The work order the pop-up is showing (PopUpInfo +0xc0, fpui2.c's
 * `pad_c0`): the object it hangs off and the map square it sits on. */
typedef struct PUWorkOrder {
    int   f00;                  /* +0x00 */
    void* obj;                  /* +0x04  the object element */
    Pos   pos;                  /* +0x08  the order's map square */
} PUWorkOrder;

extern PUWorkOrder* g_popup_order;   /* 0x007fdf80  PopUpInfo +0xc0 */
extern int          g_popup_kind;    /* 0x007fdf9c  PopUpInfo +0xdc (iconui.c
                                      *             g_info_f9c) */
extern Sprite*      g_pu_delete_on;  /* 0x00668914  (bighelp.c) */

/* 0x00471610 (not exported, misc3.c): put every pop-up icon back to its
 * unlit sprite. */
extern void ClosePopUpIcons(void);                               /* 0x00471610 */
extern void SetIconSprite(Icon* p, Sprite* s);                   /* 0x0046d680 */
extern void SetObjRectFlags(void* o, Pos* p, unsigned int f); /* 0x0045dee0 */
extern void RemoveGardenersWorkOrderAt(int x, int y);            /* 0x0049b5f0 */
extern void RemoveMechanicsWorkOrderAt(int x, int y);            /* 0x0049b640 */
extern void ResetInfoStruct(void);                               /* 0x00471510 */

/* =========================================================================
 * 0x004734d0 -- the pop-up's second delete button (bighelp.c wires it to
 * g_popup.icon_delete2, the work-order cancel).
 *
 * Its sibling PU_ToolB (fpui3.c 0x004731e0) has the same head -- unlight the
 * bar, light this gadget, then act only on event bit 1 -- and the same
 * trailing `return 1`.  See the file header for the two work-order queues.
 *
 * LEVERS:
 *  - **Both arms are written out IN FULL**, SetObjRectFlags call and
 *    ResetInfoStruct included.  Each arm carries its own `add esp,0x14` --
 *    the recorded rule that a pending cdecl clean-up cannot cross a branch
 *    join, so a "shared" tail containing calls was written twice.  Hoisting
 *    the common SetObjRectFlags above the `if` would leave one clean-up.
 *  - What VC6 DOES share is the arms' common PREFIX: `push 0` (the third
 *    argument) and `mov eax,[g_popup_order]` are emitted above the
 *    `cmp / jne`, which is head-merging, not source placement.
 *  - `g_popup_order` is read AGAIN for the RemoveXWorkOrderAt arguments,
 *    because the SetObjRectFlags call kills the load -- the global is named
 *    at every use, not cached in a local.
 *  - The `== 0x10b` arm keeps its OWN `mov al,1 / ret` (VC6 tail-duplicates
 *    the two-instruction epilogue rather than jumping over the else arm),
 *    while the guard-fail edge and the else arm share the trailing one.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004734d0
char PU_Delete2Input(Icon* p, int ev)
{
    ClosePopUpIcons();
    SetIconSprite(p, g_pu_delete_on);
    if (ev & 2) {
        if (g_popup_kind == 0x10b) {
            SetObjRectFlags(g_popup_order->obj, &g_popup_order->pos, 0);
            RemoveGardenersWorkOrderAt(g_popup_order->pos.x,
                                       g_popup_order->pos.y);
            ResetInfoStruct();
        } else {
            SetObjRectFlags(g_popup_order->obj, &g_popup_order->pos, 0);
            RemoveMechanicsWorkOrderAt(g_popup_order->pos.x,
                                       g_popup_order->pos.y);
            ResetInfoStruct();
        }
    }
    return 1;
}

/* ---- the object list (fpui.c owns the record) --------------------------- */
typedef struct ObjNode {
    struct ObjNode* next;   /* +0x00 */
    ObjDef*         obj;    /* +0x04 */
    int             keep;   /* +0x08 */
} ObjNode;

extern ObjNode* g_object_list;                                   /* 0x00668e40 */

extern void* HeapAlloc_w(unsigned int size);                     /* 0x0049e4ff */
extern int   GetObjCost(ObjDef* d);                              /* 0x00480da0 */

/* =========================================================================
 * 0x004755c0 -- insert a class into the object list, sorted by cost.
 *
 * The sibling InsertChildIntoList (0x00475630, fpui.c -- a retired partial)
 * calls this when a class has no parent entry to hang under.  See the file
 * header for the ordering rule and for the `keep = 1` that distinguishes a
 * top-level node.
 *
 * LEVERS:
 *  - **The search is a rotated `while (p)` whose LATCH is the null test.**
 *    The original's latch is `test esi,esi / jne`, and the peeled copy of
 *    that test is the enclosing `if (p)` at the top -- so the loop must be
 *    written `while (p) { if (cost) break; prev = p; p = p->next; }` inside
 *    `if (p) { ... }`, not as a `while (p && ...)`.  Read the latch off the
 *    original to find which test the loop was written on.
 *  - **The prepend block is the `else`-free fall-through of BOTH failures.**
 *    `if (prev) { link; return; }` inside the `if (p)`, with the head store
 *    as the function's last two statements, gives ONE copy reached from the
 *    empty-list guard and from `prev == 0`.
 *  - `n->next = 0;` at the top AND `n->next = p;` at the tail are both in
 *    the source: the first shares `prev`'s zero register (`xor ebx,ebx`),
 *    the second is redundant on the empty-list path and the original emits
 *    it anyway.
 *  - `push ebp` sits INSIDE the guarded block because ebp is first defined
 *    there (the first cost result), and the matching `pop` is before the
 *    `prev == 0` branch -- the recorded push-sinking rule, here with a void
 *    return.
 * ========================================================================= */
// FUNCTION: LEGOLAND 0x004755c0
void InsertObjectNode(ObjDef* d)
{
    ObjNode* p = g_object_list;
    ObjNode* prev = 0;
    ObjNode* n = (ObjNode*)HeapAlloc_w(sizeof(ObjNode));

    n->obj = d;
    n->keep = 1;
    n->next = 0;
    if (p) {
        while (p) {
            if (GetObjCost(p->obj) > GetObjCost(n->obj))
                break;
            prev = p;
            p = p->next;
        }
        if (prev) {
            prev->next = n;
            n->next = p;
            return;
        }
    }
    g_object_list = n;
    n->next = p;
}

/* ---- the "new objects" preview strip ----------------------------------- */

/* The strip is ONE object, not four globals: the classes, their sprites and
 * the two counters are 0xa8 contiguous bytes from 0x007fded4, and the
 * original's codegen proves it twice over -- a SINGLE induction variable
 * addresses `spr[i]` as `[esi]` and `def[i]` as `[esi-0x50]`, which needs
 * the two arrays to be one allocation, and the inner loop RELOADS `count`
 * from memory after every pair of stores, which is the alias kill a store
 * into the same object forces.  As four separate externs VC6 builds two
 * induction variables, keeps `count` in a register across the shift, and
 * the body comes out 59 instructions / 172 bytes against 53 / 154. */
typedef struct NewObjStrip {
    void*   def[20];            /* +0x00  0x007fded4  the classes on the strip */
    Sprite* spr[20];            /* +0x50  0x007fdf24  their preview sprites */
    int     count;              /* +0xa0  0x007fdf74 */
    int     sel;                /* +0xa4  0x007fdf78  the highlighted slot */
} NewObjStrip;

extern NewObjStrip g_newobj;        /* 0x007fded4 */
extern int         g_popup_state;   /* 0x007fdfa0  PopUpInfo +0xe0 */

extern void KillSprite(Sprite* s);                               /* 0x00497bd0 */

/* =========================================================================
 * 0x00471ca0 -- drop a class's entry from the new-objects strip.
 *
 * fpui4.c's BuildObjectIconInput calls this the moment the player clicks a
 * class whose LLIDB element still carries the "NEW" bit (0x20000).  See the
 * file header for the strip's one-object layout and for the skip-one bug.
 *
 * LEVERS (53/53 instructions, 155 bytes against 154, 5 mismatches; the first
 * cut was 59 instructions / 172 bytes with the first divergence at index 1):
 *  - **The strip is ONE struct.**  Four separate externs give two induction
 *    variables (one per array), keep `count` in a register across the shift,
 *    reload the `def` PARAMETER from its stack slot every iteration and pull
 *    a fourth callee-saved push in for a hoisted zero register.  As one
 *    object VC6 runs a single cursor addressing `spr[i]` as `[esi]` and
 *    `def[i]` as `[esi-0x50]`, and the pointer stores kill the CSE of
 *    `count`, which is exactly the reload the original makes in the inner
 *    latch.  See the declaration above.
 *  - `if (def == g_newobj.def[i])` with the PARAMETER first gives the
 *    original's `cmp ebx,[esi-0x50]`; the array first gives
 *    `cmp [esi-0x50],ebx` (same length, one mismatch).
 *  - The inner loop must be `for (j = i + 1; j < count; j++)` with the shift
 *    written `spr[j-1] = spr[j]`.  The other natural spelling,
 *    `for (j = i; j < count - 1; j++) { spr[j] = spr[j+1]; }`, has to
 *    materialise `count - 1` and rebuilds the whole outer allocation
 *    (56 instructions, four pushes).  The original's `cmp edi,edx / jge` --
 *    `i + 1` against `count` with no decrement anywhere -- is the proof that
 *    the source counted from `i + 1`.
 *
 * THE RESIDUAL, precisely: indices 22, 23, 25, 26 and 27 -- the inner shift
 * loop's strength-reduced cursor.  The original anchors it on the shift's
 * DESTINATION (`lea eax,[esi]` = `&spr[j-1]`, then `[eax+4] / [eax] /
 * [eax-0x4c] / [eax-0x50]`); this build anchors it on the SOURCE
 * (`lea eax,[esi+4]` = `&spr[j]`, then `[eax] / [eax-4] / [eax-0x50] /
 * [eax-0x54]`).  Same five instructions, same registers, same order, every
 * displacement 4 low, and the preheader `lea` carries a disp8 the original
 * does not -- which is the entire one-byte deficit.  Everything else in the
 * body, including the whole schedule and both latches, is index-for-index
 * exact.
 *
 * Measured and eliminated (about 30 builds): both statement orders; naming
 * `k = j - 1`, `k = j + 1`, or the two loaded values in locals; a `Sprite**`
 * pointer named at the destination, one pointer reaching both arrays by
 * negative subscript, and a two-pointer walk; a `do/while` written out under
 * its own `if` guard, a `while`, pre-increment, and the increment moved into
 * the body; `count > j + 1` reversed; and free `volatile` reads on the count
 * (inner and outer), on `def[i]`, on `spr[i]` and on `spr[j+1]` -- all inert
 * or worse.  The one family that DOES move the anchor is any spelling whose
 * bare induction variable IS the destination (`spr[j] = spr[j+1]`, or a
 * second variable `k = i` stepped alongside `j`): those reach 53
 * instructions and 154 BYTES exactly, with the four displacements right --
 * but then VC6 makes `j + 1` a derived IV instead of the source one, which
 * moves `mov ecx,edi` below the guard branch, sinks `inc ecx` to the end of
 * the body, turns `lea eax,[esi]` into `mov eax,esi` and swaps the outer
 * latch's two updates: 11 mismatches instead of 5.  The anchor and the
 * schedule are one decision and cannot be had at the same time from C.
 * ========================================================================= */
/* Scope I (2026-09-05): at its recorded cursor-anchor floor, 5/53 strict
 * mismatches (rb 5, ob 5), first 22, 155/154 bytes. Instruction reading
 * confirms that all four array accesses remain equivalent; the skip-one bug
 * is preserved. The recorded pointer/counter/order/volatile families already
 * test the coupled anchor and induction scheduling; they were not repeated.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x00471ca0  (90.6%, 5/53 strict; cursor-anchor floor; first 22)
void RemoveNewObjectMarker(void* def)
{
    int i;
    int j;

    for (i = 0; i < g_newobj.count; i++) {
        if (def == g_newobj.def[i]) {
            if (g_newobj.spr[i]) {
                KillSprite(g_newobj.spr[i]);
                g_newobj.spr[i] = 0;
            }
            for (j = i + 1; j < g_newobj.count; j++) {
                g_newobj.spr[j - 1] = g_newobj.spr[j];
                g_newobj.def[j - 1] = g_newobj.def[j];
            }
            g_newobj.count--;
            if (g_newobj.sel >= g_newobj.count)
                g_newobj.sel = g_newobj.count - 1;
            if (g_newobj.count == 0 && g_popup_state == 2)
                g_popup_state = 0;
        }
    }
}
