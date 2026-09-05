/* LEGOLAND -- work-order list surgery, the point-to-point route walk-back and
 * the print list's sorted insert.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Struct field OFFSETS and global addresses are load-bearing; names are ours.
 * The WorkOrder shape is workorder2.c's and the PTPNode shape bnvmove.c's; the
 * print-list node's offsets are printlist.c's but its two links are re-read
 * here as prev/next rather than tree pointers (see the note above
 * InsertPrintItem).
 *
 * =========================================================================== */
#include "legoland.h"

/* ------------------------------------------------------------------ types -- */

/* A gardener or mechanic work order -- calloc(0x3c,1) @ 0x004995d0.
 * Only the fields this file touches are named; see workorder2.c. */
typedef struct WorkOrder {
    struct WorkOrder* next;     /* +0x00 */
    void*             obj;      /* +0x04 */
    Pos               pos;      /* +0x08  map cell the rects are biased by */
} WorkOrder;

/* ---------------------------------------------------------------- globals -- */

extern WorkOrder* g_gardener_orders;       /* 0x0079a8b0 */
extern WorkOrder* g_gardener_order_tail;   /* 0x0079a8b4 */

/* ------------------------------------------------------------ prototypes -- */

extern void DBPrintf(const char* fmt, ...);   /* 0x00453a20 */

/* --------------------------------------------------------------------------
 * UnlinkGardenerOrder -- take a node out of the gardener order list, narrating
 * every step to the debug console.  FreeGardenerOrder and GiveBackGardenerOrder
 * (workorder2.c) are its only callers; the mechanic list has the same unlink
 * written out in line inside FreeMechanicOrder, without the printing.
 *
 * The `if (p)` after the search loop is dead -- the loop only leaves through
 * the break with p non-null -- but VC6 emits the test, exactly as it does in
 * FreeMechanicOrder.  Kept because it is in the original.  Writing it as
 * `if (p) {unlink} else {notfound}` rather than `if (!p) {notfound; return;}`
 * is what makes VC6 keep ONE copy of the not-found block (the `!p` spelling
 * emits it twice, because the loop-exhausted edge proves `p == 0` and folds
 * the test away).
 *
 * RESIDUAL, 34 of 68 (all of them index shift, no wrong instruction): the
 * original lays the shared tail out as
 *     [head arm] [START printf + epilogue] [ret] [search / notfound / found]
 * with the found path jumping BACKWARD into the tail, whereas VC6 gives us
 * [head arm] [search ...] [tail].  VC6 always emits [then][else][merge]; the
 * only in-tree route to [then][merge][else] is cross-jumping two written-out
 * copies of the tail, and that glues the merged block to the LAST arm (proved
 * here: with both globals read through a free `volatile` cast the two copies
 * become instruction-identical, VC6 merges them -- and puts the survivor at
 * the search arm, the mirror of what is wanted).  Getting the merge onto the
 * head arm would need the head case written LAST, which flips the compare to
 * `je` and diverges at index 23 instead.  Fifteen spellings measured; see
 * docs/lanes/fable-b-workorder3.md.
 * -------------------------------------------------------------------------- */
/* Scope I (2026-09-05): at its measured shared-tail placement floor,
 * 34/68 strict (rb 34, ob 34), first 34, 198/193 bytes. An additional
 * zero-trip do/while exit around the head/search join costs 45 and flips the
 * head guard (first 23); it cannot make the early head tail survive. Existing
 * identical-tail and source-order evidence still applies. The diagnostic's
 * missing vararg is preserved.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x00499d60  (50.0%, 34/68 strict; shared-tail ordering floor; first 34)
void UnlinkGardenerOrder(WorkOrder* o)
{
    WorkOrder* p = g_gardener_orders;

    if (!p)
        return;
    DBPrintf("unlinking gardener order %x at (%d, %d)\n", o, o->pos.x, o->pos.y);
    if (g_gardener_order_tail == o)
        DBPrintf("   Last in list,  Next = %x\n", o->next);
    if (g_gardener_orders == o) {
        DBPrintf("   First in list, Last = %x\n", g_gardener_order_tail);
        g_gardener_orders = o->next;
        if (!g_gardener_orders)
            g_gardener_order_tail = g_gardener_orders;
    } else {
        while (p) {
            if (p->next == o)
                break;
            p = p->next;
        }
        if (p) {
            p->next = o->next;
            if (!p->next)
                g_gardener_order_tail = p;
        } else {
            /* ORIGINAL BUG, reproduced: the format has THREE conversions but
             * only two arguments are pushed -- the order pointer the "(%x)"
             * is for was left out, so "%x" eats the x coordinate, the first
             * "%d" eats the y, and the second "%d" prints stack garbage. */
            DBPrintf("    Work order not found (%x) at (%d,%d)",
                     o->pos.x, o->pos.y);
            return;
        }
    }
    DBPrintf("    Work orders START (%x), END (%x)\n",
             g_gardener_orders, g_gardener_order_tail);
}

/* ==========================================================================
 * THE POINT-TO-POINT ROUTE WALK-BACK
 *
 * PTPSuggestNextMove (bnvmove.c, exact) floods tiles outward from the walker
 * with 16-byte open-list nodes {next, parent, x, y}; when the target tile is
 * reached the node is parked in g_ptp_found and this function walks the parent
 * chain back to the START tile, then pushes ONE tile onto the route list --
 * the tile the walker should step to next.
 *
 * How far ahead that step may be is decided by 0x00482330 (named
 * PTPShortcutSteps here; nothing had named it before).  It takes the first
 * FOUR nodes of the route from the start -- a (the tile we are on), b, c and
 * d -- and returns how many of them may be skipped: 0 = step to b, 1 = step
 * to c, 2 = step to d.  It answers 0 as soon as an argument is null, and
 * otherwise corner-cuts: for each diagonal it forms it looks the intervening
 * tile up in the cell grid (0x00801400) and refuses the shortcut when the
 * tile is not walkable (Cell +0x10 bit 1) or is blocked (Cell +0x1d bit 3).
 *
 * The return value is `d == 0`, i.e. TRUE when the whole route back from the
 * target is shorter than four tiles -- which is what makes PTPSuggestNextMove
 * answer 2 and send the walker straight at the target instead of at a tile.
 * ========================================================================== */

/* A point-to-point flood-fill node (16 bytes), as bnvmove.c has it. */
typedef struct PTPNode {
    struct PTPNode* next;   /* +0x00 */
    struct PTPNode* parent; /* +0x04 */
    int             x;      /* +0x08 tile x */
    int             y;      /* +0x0c tile y */
} PTPNode;

extern PTPNode* g_ptp_found;                          /* 0x0066b454 */

/* Push a tile on the FRONT of the route list (0x0066b458). */
extern void AddPTPRouteNode(int x, int y);            /* 0x00482300 */
/* 0, 1 or 2: how many of b, c, d the walker may step past in one go. */
extern int  PTPShortcutSteps(PTPNode* a, PTPNode* b, PTPNode* c, PTPNode* d); /* 0x00482330 */

/* Note on the shape: `c` is declared before `b` because that is the order the
 * original zeroes the two registers in (the three `xor`s follow declaration
 * order; the register CHOICE does not).
 *
 * RESIDUAL, 10 of 76 (76/76 instructions, every instruction right): `b` and
 * `c` have each other's callee-saved register -- the original puts b (the
 * node the walker steps to when the shortcut test says 0) in esi and c in
 * edi, we get the opposite.  Measured as a pure allocation residual: 24
 * declaration orders, 6 switch case orders, an if/else chain, `register`,
 * `void *` and `int` retypings of either local, five loop spellings (while / for /
 * do-while / hoisted parent / temporaries), b+c, b+c+d, b+d and c+d struct
 * wrappers, volatile stores at either definition site and two extern
 * prototypes are ALL byte-identical.  The one thing that flips it is a
 * reference-count change: adding a redundant `if (c)` to case 1 gives b esi
 * (and costs two instructions), which says the original's `c` outranks its
 * `b` by one weighted reference we have not found a free way to spend.
 */
/* Scope I (2026-09-05): at its measured allocation floor, 10/76
 * strict, rb 0, ob 10, first 16, 152/152 bytes. Naming c's two field values
 * in either order stays at 10; grouping them in Pos costs 13; a free
 * volatile read of c->x costs 11, c->y stays at 10, and the parent read
 * costs 11 plus one byte. These extend the recorded declaration/loop/store
 * order negatives without adding a guard or a non-original reference.
 * Full measurements: docs/lanes/scope-i.md.
 */
// WIP-FUNCTION: LEGOLAND 0x00482430  (86.8%, 10/76 strict; allocation floor; first 16)
int BuildPTPRoute(void)
{
    PTPNode* a = g_ptp_found;
    PTPNode* c = 0;
    PTPNode* b = 0;
    PTPNode* d = 0;

    if (!a)
        return 0;
    while (a->parent) {
        d = c;
        c = b;
        b = a;
        a = a->parent;
    }
    switch (PTPShortcutSteps(a, b, c, d)) {
    case 0:
        if (b)
            AddPTPRouteNode(b->x, b->y);
        break;
    case 1:
        AddPTPRouteNode(c->x, c->y);
        return d == 0;
    case 2:
        AddPTPRouteNode(d->x, d->y);
        return d == 0;
    }
    return d == 0;
}

/* ==========================================================================
 * THE PRINT LIST IS A SORTED DOUBLY-LINKED LIST, NOT A TREE
 *
 * blokeai.c ("binary-tree it by the KEY") and printlist.c ("+0x00 left, tree
 * link (InsertPrintItem)") both read the two links as tree pointers.  They are
 * not: +0x00 is PREV and +0x04 is NEXT of one list kept sorted by the depth
 * key, and 0x0066b5a4 is its HEAD.  DrawAndClearPrintList's walk down +0x04 is
 * the list order, not an in-order traversal.
 *
 * The insert is linear from a ROVING CURSOR at 0x007fd600 -- the node inserted
 * last -- which is why sorting a frame's sprites is cheap: consecutive sprites
 * are usually near each other in depth, so the walk from the previous
 * insertion point is short.  The cursor is written at EVERY step of the walk
 * (not just at the end), so an interrupted walk still leaves it on the list.
 *
 * Two debug checks bracket the body, and BOTH are broken in the shipped code:
 *   - the "Not enough RAM" message is printed when `it` is null and the
 *     function then dereferences it anyway, so the report is followed
 *     immediately by the fault it was meant to explain;
 *   - the closing `if (!it)` check can never fire for the same reason.
 * Both are reproduced.
 *
 * The tail-jump note for readers of the disassembly: the two search walks are
 * ONE `if / else if` on a single `cmp`, and each arm is a `while` whose
 * condition is the KEY comparison with the null-link test as a `break` inside
 * the body -- that is what makes VC6 peel the key compare to the top (where it
 * doubles as the if's own test, and the `else if` reuses its flags) and leave
 * the null test at the head of the loop body with the back edge above it.
 * Spelling either arm the other way round -- `while (cur->prev) { advance;
 * if (key >= cur->key) break; }`, the same as a `for (;;)`, as a `do/while`,
 * or with explicit `goto`s -- rotates the loop instead: VC6 peels the NULL
 * test and duplicates it into the latch, three instructions longer per arm.
 * ========================================================================== */

/* A depth-sorted print-list node; only the header this function touches. */
typedef struct PrintNode {
    struct PrintNode* prev;   /* +0x00 */
    struct PrintNode* next;   /* +0x04 */
    int               key;    /* +0x08  depth */
} PrintNode;

extern PrintNode* g_printlist;      /* 0x0066b5a4  head of the sorted list */
/* The node inserted last: where the next insertion's walk starts. */
extern PrintNode* g_print_cursor;   /* 0x007fd600 */

// FUNCTION: LEGOLAND 0x00485bd0
void InsertPrintItem(PrintNode* it)
{
    if (!g_print_cursor)
        DBPrintf("Oh drat, Bad stuff in the sprite sorter\n");
    /* ORIGINAL BUG, reproduced: this reports the out-of-memory case and then
     * carries straight on to dereference the null node. */
    if (!it)
        DBPrintf("Oh no, Not enough RAM for sprite sort list\n");
    if (!g_printlist) {
        it->prev = 0;
        it->next = 0;
        g_printlist = it;
    } else {
        if (it->key < g_print_cursor->key) {
            while (it->key < g_print_cursor->key) {
                if (!g_print_cursor->prev)
                    break;
                g_print_cursor = g_print_cursor->prev;
            }
        } else if (it->key > g_print_cursor->key) {
            while (it->key > g_print_cursor->key) {
                if (!g_print_cursor->next)
                    break;
                g_print_cursor = g_print_cursor->next;
            }
        }
        if (it->key < g_print_cursor->key) {
            PrintNode* p = g_print_cursor->prev;
            it->prev = p;
            if (!p)
                g_printlist = it;
            else
                p->next = it;
            it->next = g_print_cursor;
            g_print_cursor->prev = it;
        } else {
            PrintNode* n = g_print_cursor->next;
            it->next = n;
            if (n)
                n->prev = it;
            it->prev = g_print_cursor;
            g_print_cursor->next = it;
        }
    }
    g_print_cursor = it;
    /* ORIGINAL BUG, reproduced: dead -- `it` has been dereferenced above, so
     * a null node has already faulted by the time this test is reached. */
    if (!it)
        DBPrintf("Oh drat, Bad stuff in the sprite sorter\n");
}
