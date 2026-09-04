/* LEGOLAND -- the animation-part applier, the boating school's boat painter,
 * the driving school's road re-stitcher, the boating-school lake relinker and
 * the jungle cruise's along-the-river boat step.
 *
 * Five unexported functions from four different subsystems that share one
 * property: each is the "something next to me changed, work out what I look
 * like now" half of its ride.  Reconstructed from original/legoland.exe with
 * the VC6 SP3 toolchain (/O2 /Gy /Gd); struct field OFFSETS, record sizes and
 * global addresses are load-bearing, the names are ours.  Types are defined
 * LOCALLY on purpose (legoland.h is owned elsewhere).
 *
 *   0x00418fe0  BoatingSchool_DrawBoats  212/212 insns   [WIP, 32 mismatches]
 *   0x0041bab0  BsWater_Relink           230/230 insns   [OK]
 *   0x00413650  Road_Restitch            284/284 insns   [OK]
 *   0x004334c0  JcBoat_Step              294/294 insns   [OK]
 *   0x00442040  AnimApplyPart            331/331 insns   [WIP, 182 mismatches]
 *
 * ONE RENAME.  0x00418fe0 was called BoatingSchool_UpdateWater by the extern
 * in ridecb5.c; it touches no water tile and no map cell, it PAINTS the boats
 * and their riders, so it is BoatingSchool_DrawBoats here.  ONE CORRECTION:
 * junglecruise.c describes JcBoat_Step's second argument as "which of the two
 * turn tables to use"; it is a flag that switches on the route hint, and
 * nothing else.
 * ========================================================================= */

/* ---------------------------------------------------------------- types -- */
typedef struct Pos { int x; int y; } Pos;
typedef struct BPos { unsigned char x; unsigned char y; } BPos;
typedef union  BPosW { unsigned short w; BPos b; } BPosW;
typedef struct SeatOfs { int x; int y; } SeatOfs;
typedef struct Vec3 { float x; float y; float z; } Vec3;

/* The map header (screen origin of cell (0,0) only). */
typedef struct MapHdr {
    unsigned char  pad00[0x20];
    unsigned short origin_x;        /* +0x20 */
    unsigned short origin_y;        /* +0x22 */
} MapHdr;

/* A loaded image list (the LLIDB .ILF/.CSP descriptor). */
typedef struct ImageList {
    unsigned char pad00[8];
    void**        sprites;          /* +0x08 */
    int*          dx;               /* +0x0c  doubled at load, halved here */
    int*          dy;               /* +0x10 */
} ImageList;

/* The 3D person record, as this file touches it. */
typedef struct Person3D {
    unsigned char pad00[0x1c];
    int           sx;               /* +0x1c  screen position */
    int           sy;               /* +0x20 */
    unsigned char pad24[0x40 - 0x24];
    Vec3          rot;              /* +0x40  rot.y (+0x44) is the heading */
} Person3D;

/* The visitor's ride state machine byte, the only Bloke field used here. */
typedef struct Bloke {
    unsigned char pad00[0x60];
    unsigned char stage;            /* +0x60 */
} Bloke;

/* One boating-school boat: the jungle cruise's JcBoat with ONE rider instead
 * of three, so everything from +0x3e4 on sits four bytes lower.  The two
 * animation buffers are the same size and shape: 80 rocking offsets and 80
 * sprite codes, indexed by the ride-wide playback cursor g_bs_tick. */
typedef struct BsWobble { int x; int y; } BsWobble;

typedef struct BsBoat {
    BPosW          key;             /* +0x00  the station that launched it */
    unsigned char  pad02[2];
    int            cx;              /* +0x04  the map square it is on */
    int            cy;              /* +0x08 */
    int            nx;              /* +0x0c  the map square it is heading for */
    int            ny;              /* +0x10 */
    int            sx;              /* +0x14  screen position, this frame */
    int            sy;              /* +0x18 */
    BsWobble       wob[0x50];       /* +0x1c   80 precomputed sub-steps */
    int            frame[0x50];     /* +0x29c  80 precomputed sprite codes */
    int            f3dc;            /* +0x3dc */
    int            f3e0;            /* +0x3e0 */
    int            state;           /* +0x3e4  1..0x10, the mover's state */
    int            leg;             /* +0x3e8  which way this leg turns */
    Bloke*         rider;           /* +0x3ec  the single passenger */
    struct BsBoat* next;            /* +0x3f0 */
} BsBoat;                           /* 0x3f4 */

/* ---- other subsystems --------------------------------------------------- */
extern void       GetTileDimensions(int* out_w, int* out_h);   /* 0x00460540 */
extern void       AdjustOffsetForViewMode(Pos* o);             /* 0x00442d30 */
extern void       AdjustBlokePosition(Pos* p);                 /* 0x00442d60 */
extern int        PrintSprite(void* s, int x, int y, int mode, void* ctx); /* 0x004853a0 */
extern Person3D*  Find3DPersonFromBloke(void* bloke);          /* 0x0043f890 */
extern void       SetPersonRotation(Person3D* p, Vec3* rot);   /* 0x00440020 */
extern void       IP_RenderBlokeIn3DNow(void* bloke);          /* 0x00440010 */

extern MapHdr*    g_map;            /* 0x004bcbf4 (lpConfig) */
extern BsBoat*    g_bs_boats;       /* 0x004cc03c */
extern int        g_bs_tick;        /* 0x004cc08c  playback cursor, 0..0x4f */
extern ImageList* g_bs_boat_ilf;    /* 0x0082c65c  the boat image list */
/* Where the single rider sits inside the boat sprite, per heading code. */
extern SeatOfs    g_bs_seat_pos[16];                           /* 0x004b51d8 */
extern int        g_scroll_x;       /* 0x00667cb4  ScrollX (24.8) */
extern int        g_scroll_y;       /* 0x00667cb8  ScrollY (24.8) */

/* =========================================================================
 * 0x00418fe0 -- BoatingSchool_DrawBoats (RENAMED; ridecb5.c calls it
 * BoatingSchool_UpdateWater, which is wrong -- it touches no water tile and
 * no map cell, it PAINTS the boats).
 *
 * It is the boating school's copy of junglecruise.c's
 * JungleCruise_UpdateRiverAnim (0x00432d00), written from it line for line
 * with one rider instead of three and without the "standing on the station's
 * own column" half of the pass test:
 *
 *   mode != 0  paint the boats INSIDE the station (state 1 = just launched,
 *              state 0x10 = arriving)
 *   mode == 0  paint the boats on open water (every other state)
 *
 * so the station building's sprite layers can be interleaved between the two
 * passes.  BoatingSchool_Tick calls it with 0 once a frame; the BOATING
 * SCHOOL class's own painter calls it with 1.
 *
 * The projection is the standard isometric one:
 *      sx = (cx - cy) * (tw/2) - (tw+1)/2 - ScrollX>>8
 *      sy = (cx + cy) * (th/2)            - ScrollY>>8
 * plus the map's screen origin and the boat's rocking offset for this
 * sub-step, itself converted from isometric (wx-wy, wx+wy) and scaled by
 * 1/512.  Note GetTileDimensions is called a SECOND time inside the loop for
 * the rocking offset even though the answer cannot have changed: original.
 *
 * The boat is two sprites -- the hull (frame code) and one overlay
 * (code + 0x30) -- with the rider drawn between them, so the passenger is
 * always sandwiched inside the boat.  The rider is turned to the boat's
 * heading with the same constants as the jungle cruise
 *      angle = -(code * 22.5 + 45) / 360 * 2*pi
 * (0x004ab3dc..0x004ab3e8) and offset by the per-heading seat position
 * g_bs_seat_pos[code & 0xf] plus (0x44, 0x34).
 *
 * Finally, on the LAST sub-step of an arriving boat's last leg (state 0x10,
 * leg 2, tick 0x4f) and only in the station pass, the rider is put ashore:
 * its stage byte is advanced and the seat cleared, which is what hands it
 * back to BoatingSchool_Tick.  That test runs for EVERY boat, drawn or not
 * (the pass test jumps into it, not past it) -- harmless, because a boat in
 * state 0x10 is always drawn when mode != 0.
 * ========================================================================= */

/* 212/212 instructions and 744/744 BYTES, 32 mismatches (was 111), first
 * diverging index 82.  The frame layout (0x2c, with the screen-position
 * aggregate at the top and ONE pooled Pos home shared by the sprite offset and
 * the bloke position) is the original's exactly.
 *
 * WHAT CLOSED 79 OF THE 111.  VC6 flattens `origin + ox + ofs.x + scr.x` and
 * sorts the addends in DESCENDING DEFINITION ORDER (the latest-defined operand
 * is added first), which put the wobble offset LAST where the original adds it
 * FIRST; every operand permutation, parenthesisation and cast is inert against
 * that sort, because it happens after forward substitution has folded every
 * scalar temp into one flat sum.  The cure is to write the sum's leading pair
 * into the fields of a NON-ADDRESS-TAKEN aggregate, which defeats the forward
 * substitution and lets the source order survive (the `IsAdjacentPos` lever,
 * applied to a PARTIAL SUM rather than to the operands -- merely holding
 * `ox`/`oy` or `ofs.x`/`ofs.y` in an aggregate is completely inert).
 *
 * The protection has a rule of its own, measured here: a struct's fields are
 * protected only while its assignments are CONTIGUOUS, i.e. only up to the
 * first READ of any field.  So one `Pos` cannot carry both screen sums --
 * `t.x = origin_x + ox; b->sx = t.x + ..; t.y = origin_y + oy; b->sy = ..`
 * scalarises `t.y` again (38), and assigning both fields up front protects
 * both but then issues ONE `g_map` load for the two coordinates where the
 * original reloads it after the `b->sx` store (39).  TWO aggregates of two
 * fields each -- `t.x = origin_x + ox; t.y = ofs.x;` then `u.x = origin_y +
 * oy; u.y = ofs.y;` -- give both sums the original's addend order AND its two
 * `g_map` loads: 38 -> 32, and the byte length becomes exact.  A one-field
 * aggregate is inert, so the second field must carry a real value; `scr.x`
 * instead of `ofs.x`, a third field for `scr.x`, swapped field names, swapped
 * t/u roles, and both assignment orders within a pair are all byte-identical.
 *
 * RESIDUAL (32 strict, 22 register-blind), three clusters, all downstream of
 * one eax/ecx/edx rotation at index 82: the original loads `g_map` into edx,
 * `ofs.x` into eax and accumulates in ecx, where this build uses ecx, edx and
 * eax; it also loads `ofs.y` at index 87 (before the `b->sx` store) and
 * RE-LOADS `b->sx` from memory for the hull PrintSprite where we still have it
 * in a register; and the two bloke-position sums (134-141) still add `ox`/`oy`
 * last -- the same sort, but the partial-sum aggregate costs an instruction
 * there (41) because `ofs` is escaped, so it is not applied.
 * Re-measured on this baseline and inert: naming `ofs.x`/`ofs.y` as int locals
 * before the sums; hoisting `ofs.y`'s read above the `b->sx` store; locals for
 * the PrintSprite coordinates or for the sprite pointer; `ox`/`oy` at function
 * level; a 3-field struct; writing `scr` as two ints (frame shrinks to 0x28,
 * so `Pos scr` is confirmed).
 *
 * ROUND OF 2026-09-04 -- the residual is now characterised as TWO ATTRACTORS
 * that each hold half the answer, and the search for a third has failed:
 *  * the FLAT/chain spelling (`b->sx = g_map->origin_x + ox + ofs.x + scr.x;`
 *    or `sxv = g_map->origin_x + ox; sxv += ofs.x; sxv += scr.x;`) gives the
 *    ORIGINAL'S REGISTER ROTATION exactly -- indices 82-87 match, g_map into
 *    edx, ofs.x into eax, the accumulator ecx, `ofs.y` read early at 87 --
 *    AND the original's reload of `b->sx` from memory for the hull
 *    PrintSprite; but it sorts the addends ofs.x, scr.x, ox (descending
 *    definition order) where the original adds ox, ofs.x, scr.x.
 *  * the AGGREGATE spelling (shipped) gives the addend order but the rotation
 *    one step behind from index 82 on, and keeps b->sx in a register.
 * Both spellings carry ONE instruction the original does not: `mov edx,eax`
 * before the `push` of `b->rider` into Find3DPersonFromBloke (the original
 * pushes eax straight); the flat form is 213 because it has that AND the
 * b->sx reload, the aggregate form is 212 because it has the copy but not the
 * reload.  So the original = flat rotation + aggregate order - the copy.
 * The blocking fact, stated exactly: BOTH remaining clusters need `ox` to
 * sort AFTER `scr.x` (indices 88-92 and the bloke sums at 134-141), i.e. `ox`
 * must be the later-defined symbol, while `ox` is unavoidably computed first
 * -- moving `ox`/`oy` below `scr.x`/`scr.y`, or below
 * AdjustOffsetForViewMode, rewrites the whole prologue (214-216 instructions,
 * divergence at index 0), because the wobble products have to be computed
 * before the screen position.
 * Measured inert on this baseline (all still 32, or worse): in-place `+=` on
 * the aggregate's own fields; `sxv = t.x + t.y; sxv += scr.x;`; three- and
 * four-term protected partial sums (a FOUR-term one falls back into the flat
 * attractor, 112); constant-indexed `int oa[2]` carriers for ox/oy (VC6
 * copy-propagates them -- arrays do NOT defeat forward substitution here);
 * a 3-field struct and a single 4-field struct; MIXED attractors (flat x with
 * aggregate y and the reverse, 38/116 -- the rotation is decided by the FIRST
 * sum); the empty `if (v) { }` flatten breaker on the chain (it restores the
 * sort and loses the rotation, 119-121); the two-statement `ofs.x = ..;
 * ofs.x += scr.x;` form for the bloke sums (folded back); a non-escaped
 * `Pos w` partial-sum aggregate for the bloke sums (41); and four rider-guard
 * restructurings (a `Bloke* rid` local, `if (b->rider != 0) {..}` in place of
 * the `goto next`), all of which move the divergence to index 0.
 *
 * STRUCTURAL RE-RANK (the strict count misleads here too).  Measured with
 * scratchpad/laneG/sc.py: the shipped two-aggregate form is register+offset-
 * blind LCS 205 of 212 with only SEVEN original indices inside a differing
 * region; every flat/chain spelling is 204 with eight.  So the aggregate is
 * the better shape on structure as well as on the strict count, and it stays.
 * Re-tested against the corrected add-destination RANK rule (inline memory >
 * compiler temporary > named local, last-defined of two locals): routing the
 * memory operands through named `int` locals -- `ofs.x`/`ofs.y`, `scr.x`/
 * `scr.y`, `ox`/`oy`, and all four together, before or between the sums -- is
 * completely inert, every one landing in the flat attractor at 111.  The rank
 * rule does not reach this residual because the destination is already the
 * rank-1 inline memory operand (`g_map->origin_x`) in BOTH attractors; what
 * differs is only which register the eax/ecx/edx rotation hands it.
 * More inert on this baseline, recorded so they are not re-run: spelling the
 * wobble offsets INLINE at their use sites so they become CSE temporaries
 * (rank 2, which by the rank rule should outrank the named-local addends) --
 * in the screen sums, in the bloke sums, in both, and with a flat sum: all
 * rewrite the whole block (robl 169-182, ESCAPES); an eight-way extern
 * prototype sweep (AdjustOffsetForViewMode/AdjustBlokePosition/PrintSprite/
 * GetTileDimensions/Find3DPersonFromBloke/IP_RenderBlokeIn3DNow/
 * SetPersonRotation return and parameter types) -- every one byte-identical;
 * and three attempts to force the original's `b->sx` reload (`long`,
 * `unsigned`, `unsigned long` for the two fields -- the same-width-conversion
 * CSE barrier does not fire on a field read feeding a call argument -- and a
 * `volatile` read at the PrintSprite, which costs an instruction).
 * The three structural regions that remain: orig[87:89] (the original loads
 * `ofs.y` before the `b->sx` store), orig[103:109] (it RELOADS `b->sx` from
 * memory for the hull PrintSprite where we still have it in a register), and
 * one spurious `mov edx, eax` before the `push` of `b->rider` into
 * Find3DPersonFromBloke.  All three follow from the rotation.
 * The twin, junglecruise.c's JungleCruise_UpdateRiverAnim (0x00432d00), was
 * sitting on exactly the same 111 and the same cause -- the two-aggregate
 * partial sum should transfer to it.
 *
 * ROUND OF 2026-09-04.  Unchanged at 32 / robl 205 / 7 structural indices.
 * The causal chain behind the rotation is now fully traced, which narrows
 * what a future round has to look for:
 *   - The original reads `ofs.y` at index 87, BEFORE the `b->sx` store.  That
 *     keeps edx busy, so the SECOND `g_map` load at 94 has to take ecx, which
 *     kills the x sum still sitting there -- and THAT is why the original
 *     reloads `b->sx` at 103.  Our build reads ofs.y at 95 (edx is free by
 *     then), keeps b->sx in ecx and needs no reload.  All three "structural"
 *     regions are that one fact.
 *   - The read cannot be hoisted by the scheduler: `ofs` is address-taken and
 *     the store through `b` may alias it.  So the original's IR has the
 *     ofs.y read before the store, and only a source change can put it there.
 *     Forcing it is what fails: `u.y = ofs.y;` moved above the `b->sx` store
 *     (two positions), a third `Pos v` copying BOTH ofs fields immediately
 *     after AdjustOffsetForViewMode, and `t.y = ofs.y` carried by the x
 *     aggregate are ALL byte-identical to the shipped form -- VC6
 *     copy-propagates a field that merely holds a value, exactly as the
 *     earlier round found for `ox`/`oy`.  Only a field holding a partial SUM
 *     resists, and there is no sum to put there.
 *   - Both accumulators want eax in our build (the x sum therefore ends in a
 *     `lea ecx,[eax+edi]` instead of the original's `add ecx,edi`); in the
 *     original eax is still holding `ofs.x`, so the x accumulator is forced
 *     to ecx and the y accumulator gets eax.  Same single cause.
 * NEW AND INERT this round (all 32/robl 205 unless noted): `u.y = ofs.y`
 * hoisted above the store in two positions; a `Pos v` ofs copy; `t.y = ofs.y`
 * with `ofs.x` inline (38, and it loses the y sum's addend order -- its
 * bad=10 is a metric artefact, not an improvement); both sums into `sxv`/
 * `syv` temps stored at the end (39); protected LATE partial pairs meant to
 * give `ox` a late definition so the FLAT sort would put it first --
 * `t.x = ox + ofs.x` (127), `t.x = ofs.x + scr.x` (127, ESCAPES),
 * `t.x = ox + ofs.x; t.y = scr.x;` (120); `t.y = scr.x` instead of `ofs.x`
 * (byte-identical); the aggregate form re-spelled as a `+=` chain
 * (byte-identical); and -- the one genuinely new idea -- the empty-`if`
 * flatten breaker applied at ONE join of the chain instead of all of them,
 * so that only `origin_x + ox` is protected and `ofs.x`/`scr.x` still
 * flatten into the original's order: after the first join 119, after the
 * second 118, after both 120, x-only 38.  The breaker always costs the
 * rotation, wherever it is put.  Rider-guard re-tests on this baseline:
 * `if (!b->rider)` is byte-identical, a `Bloke* rid` local is 119.
 * Also inert (and the most promising idea of the round): a `static __inline`
 * BsScreenPos helper taking the two sums' operands, so that its ARGUMENTS are
 * evaluated into temporaries before its body runs (the joust.c seat-helper
 * lever) -- with `ofs.x`/`ofs.y` as int parameters or with `ofs` and `scr`
 * passed as whole `Pos` values, and with either the flat or the aggregate
 * body.  Both aggregate spellings are byte-identical to the shipped form and
 * both flat ones land in the flat attractor at 111: VC6 forward-substitutes
 * an inline argument that is a plain memory reference, so the helper does not
 * create the barrier.
 * SO: the wall is unchanged and is one register rotation, but the thing to
 * hunt for is now specific -- a source shape that reads `ofs.y` into the IR
 * before the `b->sx` store WITHOUT being copy-propagated.  Everything that
 * merely NAMES the value (local, aggregate field, array element, inline
 * argument) is propagated back to the use; the only constructs known to
 * resist are a partial SUM in an aggregate field (there is no sum available
 * here) and a volatile read (which costs an instruction). */
/* ROUND OF 2026-09-04 (fifth pass).  32 -> 30, and every metric moved the same
 * way: real (mismatches surviving the best callee-saved permutation) 29 -> 27,
 * register+offset-blind LCS 205 -> 206 of 212, ORIGINAL indices inside a
 * differing region 12 -> 9 (scratchpad/w7joust/w7.py; that tool's region count
 * charges both sides of a replace, so it reads higher than the "7" quoted
 * above -- the sequence is unchanged, the counting rule is not).
 * WHAT CLOSED IT: the x aggregate's SECOND field carries a partial sum of its
 * own -- `t.y = ofs.x + scr.x;` with `b->sx = t.x + t.y;` instead of
 * `t.y = ofs.x; b->sx = t.x + t.y + scr.x;`.  Both spell the same value and
 * both keep the original's addend order, but the two-term field makes indices
 * 89, 90 and 91 (`push 0 / add ecx,eax / xor eax,eax`) come out exact: the
 * second `push 0` of the PrintSprite argument list stops being scheduled next
 * to the first.  The whole family is one attractor -- operand order inside
 * either term, `t.y + t.x`, `t.y` assigned first, a single four-field struct,
 * and `u.y` written before `u.x` are all byte-identical at 30.
 * The same trick on the Y side is NOT the original: `u.y = ofs.y + scr.y`
 * (with or without the x one) falls back into the flat attractor at 124-136.
 * A three-term `t.x = origin_x + ox + ofs.x` with `t.y = scr.x` is 32.
 * NEW AND INERT this round: splitting the wobble shift so `ox`/`oy` get a LATE
 * definition (`ox = (wx-wy)*tw2;` early, `ox >>= 9;` just before the sums) --
 * the idea was to make the flat form's descending-definition sort put `ox`
 * first; it rewrites the prologue instead (174-176, divergence at index 0),
 * with `>>=` and `= ox >> 9` identical.  An empty-`if` dummy temp before the
 * sums, meant to advance the eax/ecx/edx rotation by one step, is deleted
 * outright.  `(void)b->sx;` after the sums does not force the reload.
 * THE WALL IS UNCHANGED and is stated in the round above: the original reads
 * `ofs.y` at index 87 (before the `b->sx` store), which keeps edx busy, forces
 * the second `g_map` load onto ecx and hence the `b->sx` reload at 103.  Our
 * rotation is one step behind the original's from index 82 (ours ecx, edx, eax
 * where the original takes edx, eax, ecx), so what is wanted is one MORE
 * scratch temp before the x sum, not fewer -- but every construct that merely
 * names a value is copy-propagated back to its use. */
/* ROUND OF 2026-09-04 (sixth pass).  UNCHANGED AT 30, but the wall now has a
 * name, and it is a genuine either/or between two source shapes rather than a
 * missing lever.  THE ORIGINAL'S TWO SUMS ARE FLAT:
 *      86 mov cx,[edx+0x20]   88 add ecx,ebx   90 add ecx,eax   92 add ecx,edi
 * -- `origin_x + ox + ofs.x + scr.x`, four terms, three plain `add`s, and the
 * y sum the same.  Written flat in source, the build reproduces indices 82-87
 * EXACTLY, INCLUDING index 87 `mov edx,[esp+0x38]` -- the `ofs.y` read BEFORE
 * the `b->sx` store that five rounds have called the wall -- and it also
 * reproduces the `b->sx` RELOAD (its index 98 `mov edx,[esi+0x14]` against the
 * original's 103).  The shipped `Pos t`/`Pos u` aggregate reaches neither.
 * WHY THE FLAT FORM IS STILL NOT SHIPPED: 111 strict / real 108 / robl 204 /
 * bad 15 against 30 / 27 / 206 / 9.  What it loses is the ADDEND ORDER: VC6
 * fully reassociates a flat sum and emits it sorted by DESCENDING definition
 * point -- ofs.x (defined at 83), scr.x (59), ox (50) -- where the original
 * has source order (ox, ofs.x, scr.x).  All 24 source orders of the four
 * addends are BYTE-IDENTICAL, so the sort is unconditional.
 * THE SAME SORT IS VISIBLE IN THE SECOND SUM, which is already plain flat
 * source (`ofs.x = g_map->origin_x + ox + scr.x;`): the original adds `ox`
 * then `scr.x` (134/135) and we add `scr.x` then `ox`.  Four of the thirty
 * mismatches are that, and whatever fixes it should fix the first sum too.
 * TRIED AGAINST THE SORT, ALL WORSE OR INERT: every partial-sum barrier that
 * leaves a three-term flat tail (`t.x = origin_x + ox; b->sx = t.x + ofs.x +
 * scr.x;`, with or without the y twin, and the same through a plain `int`
 * local) collapses straight into the flat attractor at 111-126; `t.x =
 * origin_x + ox + ofs.x` with `t.y = scr.x` is 50; `t.x = origin_x` with
 * `t.y = ox + ofs.x + scr.x` is 37; the whole sum inside one field is 111;
 * barriers on the second sum are 39; all six source orders of the second sum
 * are byte-identical; the y sum first is 36.
 * THE SORT KEY IS NOT AGGREGATE-VS-SCALAR AND NOT SOURCE ORDER.  `ox`/`oy` as
 * a `Pos` instead of two ints is byte-identical; `scr` as two ints instead of
 * a `Pos` re-lays the frame (57, first divergence 0), so `scr` being an
 * aggregate is load-bearing for the frame and cannot be changed.  Moving the
 * `ox`/`oy` assignments BELOW the `scr` ones -- the direct attack on a
 * definition-point sort, and the scheduler interleaves the two blocks anyway
 * -- rewrites the prologue instead (180-184, first divergence 0, ESCAPES), as
 * does interleaving them or moving the second GetTileDimensions.
 * ONE MORE DATA POINT WORTH KEEPING.  The Y sum's shipped shape -- a ONE-term
 * second field, `u.x = origin_y + oy; u.y = ofs.y; b->sy = u.x + u.y + scr.y;`
 * -- already emits the original's flat order exactly (indices 96-100 are
 * exact).  The same shape on X (`t.y = ofs.x` instead of `ofs.x + scr.x`) also
 * gives the ORIGINAL'S ADDEND ORDER, `add eax,ebx (ox) / add eax,edx (ofs.x)`,
 * and only the third join comes out as `lea ecx,[eax+edi]` where the original
 * has `add ecx,edi`: 32 strict / robl 205 / bad 12.  So the two-term second
 * field the fifth pass shipped buys two strict indices by LOSING the addend
 * order; it is kept because every metric still prefers it, but the one-term
 * form is the shape to build on if the `lea`/`add` join can be fixed.
 * `t.x = origin_x + ox + ofs.x` with `t.y = scr.x` is the same 32/205/12.
 * SO THE CHOICE IS: the aggregate (30, wrong sum tree, right addend order) or
 * flat (111, right sum tree AND the index-87 read AND the reload, wrong addend
 * order).  A future round should look for what makes VC6 skip the flat-sum
 * reassociation, not for another barrier: every barrier tested so far either
 * changes the tree or is deleted. */
// WIP-FUNCTION: LEGOLAND 0x00418fe0  (212/212 insns, 744/744 bytes, 30 mismatches, 27 surviving the best callee-saved permutation, 9 of 212 original indices structurally different; one eax/ecx/edx rotation at index 82)
void BoatingSchool_DrawBoats(int mode)
{
    Pos     scr;
    int     tw;
    int     th;
    BsBoat* b;

    b = g_bs_boats;
    GetTileDimensions(&tw, &th);
    if (b == 0)
        return;
    do {
        if (mode != 0) {
            if (b->state == 1)
                goto draw;
            if (b->state == 0x10)
                goto draw;
            goto next;
        } else {
            if (b->state == 1)
                goto next;
            if (b->state == 0x10)
                goto next;
        }
draw:
        {
            int wy = b->wob[g_bs_tick].y;
            int wx = b->wob[g_bs_tick].x;
            int tw2;
            int th2;
            int ox;
            int oy;

            GetTileDimensions(&tw2, &th2);
            ox = ((wx - wy) * tw2) >> 9;
            oy = ((wx + wy) * th2) >> 9;
            scr.x = (b->cx - b->cy) * (tw >> 1) - ((tw + 1) >> 1) - (g_scroll_x >> 8);
            scr.y = (b->cx + b->cy) * (th >> 1) - (g_scroll_y >> 8);
            {
                Pos ofs;

                ofs.x = g_bs_boat_ilf->dx[b->frame[g_bs_tick] & 0xff] >> 1;
                ofs.y = g_bs_boat_ilf->dy[b->frame[g_bs_tick] & 0xff] >> 1;
                AdjustOffsetForViewMode(&ofs);
                {
                Pos t;
                Pos u;

                t.x = g_map->origin_x + ox;
                t.y = ofs.x + scr.x;
                b->sx = t.x + t.y;
                u.x = g_map->origin_y + oy;
                u.y = ofs.y;
                b->sy = u.x + u.y + scr.y;
                }
                PrintSprite(g_bs_boat_ilf->sprites[b->frame[g_bs_tick] & 0xff],
                            b->sx, b->sy, 0, 0);
                if (b->rider == 0)
                    goto next;
                {
                    Person3D* p3 = Find3DPersonFromBloke(b->rider);
                    float     ang = ((float)b->frame[g_bs_tick] * 22.5f + 45.0f)
                                    * -0.0027777769f;

                    p3->rot.y = ang * 6.2831855f;
                    SetPersonRotation(p3, &p3->rot);
                    ofs.x = g_map->origin_x + ox + scr.x;
                    ofs.y = g_map->origin_y + oy + scr.y;
                    AdjustBlokePosition(&ofs);
                    {
                        Pos so;

                        so.x = g_bs_seat_pos[b->frame[g_bs_tick] & 0xf].x + 0x44;
                        so.y = g_bs_seat_pos[b->frame[g_bs_tick] & 0xf].y + 0x34;
                        AdjustOffsetForViewMode(&so);
                        p3->sx = so.x + ofs.x;
                        p3->sy = so.y + ofs.y;
                    }
                }
                IP_RenderBlokeIn3DNow(b->rider);
                PrintSprite(g_bs_boat_ilf->sprites[(b->frame[g_bs_tick] + 0x30) & 0xff],
                            b->sx, b->sy, 0, 0);
            }
        }
next:
        if (b->state == 0x10 && b->leg == 2 && g_bs_tick == 0x4f && mode != 0
            && b->rider != 0) {
            b->rider->stage++;
            b->rider = 0;
        }
        b = b->next;
    } while (b);
}

/* =========================================================================
 * THE BOATING SCHOOL'S LAKE, AND WHY IT NEEDS A SECOND PASS
 *
 * A lake square is a 5x5 block of map cells whose CENTRE cell is the square's
 * map coordinate, so neighbouring squares are five cells apart (every +-5
 * below is one lake step).  Each square paints itself from its own four-bit
 * link mask (1 N, 2 E, 4 S, 8 W), which leaves the four 2x2 blocks BETWEEN
 * diagonally adjacent squares as bare ground.  BsWater_Relink is the pass
 * that fills those in.
 *
 * It is line for line junglecruise.c's JungleCruise_RelinkRiverCell
 * (0x004367b0) -- the two rides' water is one piece of code written twice,
 * exactly as ridecb6.c found for BsWater_Add / JcWater_Add.  The only
 * differences are the record sizes and which globals hold the list heads.
 * ========================================================================= */

/* One placed boating school (the full field list is in ridecb5.c). */
typedef struct BsStation {
    BPosW             pos;          /* +0x00 packed map square */
    BPosW             a;            /* +0x02 route START square {ax, ay} */
    BPosW             b;            /* +0x04 route END square {bx, by} */
    unsigned char     pad06[0x2c - 6];
    struct BsStation* next;         /* +0x2c */
} BsStation;                        /* 0x34 */

/* One square of boating-school water. */
typedef struct BsWater {
    BPosW           pos;            /* +0x00 */
    BPosW           owner;          /* +0x02 the school that owns this cell */
    int             links;          /* +0x04 link mask: 1 N, 2 E, 4 S, 8 W */
    unsigned char   pad08[8];
    struct BsWater* next;           /* +0x10 */
} BsWater;                          /* 0x1c */

extern BsStation* g_bs_stations;    /* 0x004cc074 */
/* The BOATING SCHOOL WATER class's TSM record array (LLIDB_LoadTSMData's
 * 8-byte {LLElem* entry, void* loaded} records); the loaded TSF descriptor's
 * first word is the tileset's base slot, i.e. the plain corner filler. */
extern void*      g_bs_tsm;         /* 0x0082adf4 */

extern BsWater*   BsWater_FindAt(int x, int y);                /* 0x0041c890 */
extern void       SetMapTile(int x, int y, unsigned short t);  /* 0x00461780 */

/* Tile 0 of the water tileset.  Written out at every call site (never cached)
 * because SetMapTile may move the tables, and the original reloads
 * 0x0082adf4 before each one. */
#define BS_CORNER_TILE  (**(unsigned short**)((char*)g_bs_tsm + 4))

/* =========================================================================
 * 0x0041bab0 -- BsWater_Relink: fill in the inside corners where two arms of
 * the lake meet at (x, y).
 *
 * For each of the four diagonal pairs -- (west, north), (west, south),
 * (east, north), (east, south) -- it checks that the vertical neighbour also
 * carries the matching horizontal link, and if so stamps the 2x2 corner block
 * with tile 0 of the water tileset.
 *
 * Before that the square's own mask is adjusted for the school it belongs to
 * (`owner` is the school's packed map square): the route START square loses
 * its NORTH link and the route END square its SOUTH link, so no corner is
 * ever drawn into the school building.
 *
 * ORIGINAL BUGS reproduced: the station search may end on a null cursor which
 * is then dereferenced, and each neighbour lookup is dereferenced without a
 * null check (it cannot fail while the link bit is set, but nothing enforces
 * that).
 *
 * CODEGEN NOTE: both parameter homes carry locals -- `links` lives in y's
 * slot and the `links & 8` / `links & 2` flag in x's, while x and y stay in
 * edi/esi for the whole body.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0041bab0
void BsWater_Relink(int x, int y, BPosW* owner)
{
    BsStation* st = g_bs_stations;
    BsWater* w;
    BsWater* n;
    int links;

    w = BsWater_FindAt(x, y);
    while (st && st->pos.w != owner->w)
        st = st->next;
    if (w == 0)
        return;
    links = w->links;
    if (w->pos.w == st->a.w)
        links &= ~1;
    else if (w->pos.w == st->b.w)
        links &= ~4;

    if ((links & 8) && (links & 1)) {
        n = BsWater_FindAt(x, y - 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y - 3, BS_CORNER_TILE);
            SetMapTile(x - 2, y - 3, BS_CORNER_TILE);
            SetMapTile(x - 3, y - 2, BS_CORNER_TILE);
            SetMapTile(x - 2, y - 2, BS_CORNER_TILE);
        }
    }
    if ((links & 8) && (links & 4)) {
        n = BsWater_FindAt(x, y + 5);
        if (n->links & 8) {
            SetMapTile(x - 3, y + 3, BS_CORNER_TILE);
            SetMapTile(x - 2, y + 3, BS_CORNER_TILE);
            SetMapTile(x - 3, y + 2, BS_CORNER_TILE);
            SetMapTile(x - 2, y + 2, BS_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 1)) {
        n = BsWater_FindAt(x, y - 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y - 3, BS_CORNER_TILE);
            SetMapTile(x + 2, y - 3, BS_CORNER_TILE);
            SetMapTile(x + 3, y - 2, BS_CORNER_TILE);
            SetMapTile(x + 2, y - 2, BS_CORNER_TILE);
        }
    }
    if ((links & 2) && (links & 4)) {
        n = BsWater_FindAt(x, y + 5);
        if (n->links & 2) {
            SetMapTile(x + 3, y + 3, BS_CORNER_TILE);
            SetMapTile(x + 2, y + 3, BS_CORNER_TILE);
            SetMapTile(x + 3, y + 2, BS_CORNER_TILE);
            SetMapTile(x + 2, y + 2, BS_CORNER_TILE);
        }
    }
}

/* =========================================================================
 * THE DRIVING SCHOOL'S ROADS, AND WHAT A ROAD TILE IS
 *
 * A road block is one map square with a record on the driving school's list
 * (ridecb6.c: `group` at +0x08 is the OWNING SCHOOL's packed map square, and
 * bit 0x10 of the kind byte at +0x14 says the block also carries a ZEBRA
 * CROSSING).  Which of the road sprites a block shows is not stored: it is
 * recomputed from the blocks around it every time a neighbour is added or
 * taken away, and Road_Restitch is that recomputation for ONE block.
 *
 * The neighbourhood is probed in two steps into one array of four
 * {orthogonal, diagonal} pairs -- the orthogonal probe (0x00413520) fills the
 * `o` fields, and only if the answer is still ambiguous does the diagonal
 * probe (0x00413450) fill the `d` fields on top of them.  The four pairs are
 * the four quadrants in order, so the 8-bit mask the second pass builds runs
 *      1 N   2 NE   4 E   8 SE   0x10 S   0x20 SW   0x40 W   0x80 NW
 * while the 4-bit mask the first pass builds is just its orthogonal half
 *      1 N   2 E    4 S   8 W
 * and a neighbour only counts when it belongs to the SAME school.
 *
 * The tile is then chosen by how many orthogonal neighbours matched:
 *      0  nothing at all -- the block is left exactly as it is (no refund)
 *      1  a dead end, or 2 when the two are opposite: shape 1/7 (see below)
 *      2  a corner: shape 3, rotation from which two arms are joined
 *      3  a T junction: shape 4
 *      4  a crossroads: shape 5
 * and only the dead-end/straight family (the `case 1` block) is allowed to
 * keep the zebra-crossing bit.  Every other shape drops it, which is why the
 * tail refunds the crossing's cost with AddBricks(GetObjCost(...)) whenever
 * the block had one: re-stitching a crossing into a corner, a T or a
 * crossroads DESTROYS the crossing and gives the player the bricks back.
 * ========================================================================= */

/* One road block, as this function touches it. */
typedef struct RoadBlock {
    unsigned char  pad00[8];
    unsigned short group;           /* +0x08 the owning school's map square */
    unsigned char  pad0a[0x14 - 0x0a];
    unsigned char  kind;            /* +0x14 bit 0x10 = has a zebra crossing */
} RoadBlock;

/* One quadrant of the neighbourhood: the orthogonal block and the diagonal
 * one just past it.  The two probes fill the two fields separately. */
typedef struct RoadNb {
    RoadBlock* o;                   /* +0x00 */
    RoadBlock* d;                   /* +0x04 */
} RoadNb;

extern RoadBlock* Road_FindAt(int x, int y);                   /* 0x004125a0 */
/* Both probes return how many they found; Road_Restitch ignores it and
 * counts the ones belonging to its own school itself. */
extern int  Road_ProbeOrtho(int x, int y, RoadNb* out);        /* 0x00413520 */
extern int  Road_ProbeDiag(int x, int y, RoadNb* out);         /* 0x00413450 */
/* Stamp the road square's tiles: `shape` 1..5 (bit 0x10 keeps the zebra
 * crossing) and `rot` the quarter turn. */
extern void Road_SetTile(int x, int y, int shape, int rot);    /* 0x00412680 */
extern int  GetObjCost(void* def);                             /* 0x00480da0 */
extern void AddBricks(int bricks);                             /* 0x004578a0 */

extern void* g_zebra_def;           /* 0x0082c678  the ZEBRA CROSSING ObjDef */

/* =========================================================================
 * 0x00413650 -- Road_Restitch: re-pick the road tile at (x, y) for the
 * driving school whose packed map square is `school`.
 *
 * Called after any edit to a neighbouring square, exactly like pathtile2.c's
 * AdjustPathTile.  `n` and `m` are counted/ored in memory rather than in
 * registers because all four callee-saved registers are already spoken for
 * (ebx = the 4-bit mask, ebp = the zebra bit, esi = y, edi = x).
 *
 * ORIGINAL QUIRK reproduced: `case 0` returns without the crossing refund, so
 * an isolated block keeps both its old tile and its crossing.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00413650
void Road_Restitch(BPosW school, int x, int y)
{
    RoadNb     nb[4];
    int        n = 0;
    int        m = 0;
    int        mask = 0;
    int        zebra = 0;
    RoadBlock* me;
    int        cost;

    me = Road_FindAt(x, y);
    if (me != 0)
        zebra = me->kind & 0x10;

    Road_ProbeOrtho(x, y, nb);
    if (nb[0].o != 0 && nb[0].o->group == school.w)
        mask = n = 1;
    if (nb[1].o != 0 && nb[1].o->group == school.w) {
        n++;
        mask |= 2;
    }
    if (nb[2].o != 0 && nb[2].o->group == school.w) {
        n++;
        mask |= 4;
    }
    if (nb[3].o != 0 && nb[3].o->group == school.w) {
        n++;
        mask |= 8;
    }

    switch (n) {
    case 0:
        return;
    case 2:
        if ((mask & 5) != 5 && (mask & 0xa) != 0xa) {
            switch (mask) {
            case 6:
                Road_SetTile(x, y, 3, 0);
                break;
            case 12:
                Road_SetTile(x, y, 3, 1);
                break;
            case 9:
                Road_SetTile(x, y, 3, 2);
                break;
            case 3:
                Road_SetTile(x, y, 3, 3);
                break;
            }
            break;
        }
        /* two OPPOSITE arms is a straight road: fall through */
    case 1:
        Road_ProbeDiag(x, y, nb);
        if (nb[0].o != 0 && nb[0].o->group == school.w)
            m = 1;
        if (nb[0].d != 0 && nb[0].d->group == school.w)
            m |= 2;
        if (nb[1].o != 0 && nb[1].o->group == school.w)
            m |= 4;
        if (nb[1].d != 0 && nb[1].d->group == school.w)
            m |= 8;
        if (nb[2].o != 0 && nb[2].o->group == school.w)
            m |= 0x10;
        if (nb[2].d != 0 && nb[2].d->group == school.w)
            m |= 0x20;
        if (nb[3].o != 0 && nb[3].o->group == school.w)
            m |= 0x40;
        if (nb[3].d != 0 && nb[3].d->group == school.w)
            m |= 0x80;
        if (m & 0x11) {
            if ((m & 0x83) == 0x83) {
                if ((m & 0x38) == 0x38) {
                    zebra |= 7;
                    Road_SetTile(x, y, zebra, 0);
                    return;
                }
                zebra |= 1;
                Road_SetTile(x, y, zebra, 0);
                return;
            }
            if ((m & 0x38) == 0x38) {
                zebra |= 1;
                Road_SetTile(x, y, zebra, 2);
                return;
            }
            Road_SetTile(x, y, zebra, 0);
            return;
        }
        if ((m & 0xe0) == 0xe0) {
            if ((m & 0xe) == 0xe) {
                zebra |= 7;
                Road_SetTile(x, y, zebra, 1);
                return;
            }
            zebra |= 1;
            Road_SetTile(x, y, zebra, 3);
            return;
        }
        if ((m & 0xe) == 0xe) {
            zebra |= 1;
            Road_SetTile(x, y, zebra, 1);
            return;
        }
        Road_SetTile(x, y, zebra, 1);
        return;
    case 3:
        switch (mask) {
        case 11:
            Road_SetTile(x, y, 4, 0);
            break;
        case 7:
            Road_SetTile(x, y, 4, 1);
            break;
        case 14:
            Road_SetTile(x, y, 4, 2);
            break;
        case 13:
            Road_SetTile(x, y, 4, 3);
            break;
        }
        break;
    case 4:
        Road_SetTile(x, y, 5, 0);
        break;
    }
    if (zebra != 0) {
        cost = GetObjCost(g_zebra_def);
        AddBricks(cost);
    }
}

/* =========================================================================
 * THE JUNGLE CRUISE'S BOAT MOVER
 *
 * A river square is a 5x5 block of map cells, so every +-5 below is one
 * river step, and JcWater::links (1 N, 2 E, 4 S, 8 W) says which of the four
 * neighbours exist.  JungleCruise_AdvanceBoats (junglecruise.c 0x004332f0)
 * commits the square a boat was heading for and then dispatches on its state
 * word; states 4 and 8 both land here, with `steer` 0 and 1 respectively.
 *
 * JcBoat::f3dc (+0x3dc) is NOT the boat's heading -- it is the side it came
 * IN by, which is why the mover's first move is to take the OPPOSITE bit as
 * its preferred exit and why `links & ~f3dc` is "anywhere but back the way I
 * came".  -1 means "stuck", 1/2/4/8 the four sides.
 * ========================================================================= */

typedef struct JcStation {
    BPosW             pos;          /* +0x00  the station's own map square */
    BPosW             a;            /* +0x02  route START square */
    BPosW             b;            /* +0x04  route END square */
    unsigned char     pad06[0x3c - 6];
    struct JcStation* next;         /* +0x3c */
} JcStation;                        /* 0x44 */

typedef struct JcWater {
    BPosW           pos;            /* +0x00  this square */
    BPosW           owner;          /* +0x02  the station that owns the river */
    int             links;          /* +0x04  1 N, 2 E, 4 S, 8 W */
    unsigned char   pad08[0x18 - 8];
    struct JcWater* route_next;     /* +0x18  route: the square after this one */
} JcWater;                          /* 0x1c */

typedef struct JcBoat {
    BPosW           key;            /* +0x00  the station that launched it */
    unsigned char   pad02[2];
    int             cx;             /* +0x04  the map square it is on */
    int             cy;             /* +0x08 */
    int             nx;             /* +0x0c  the map square it is heading for */
    int             ny;             /* +0x10 */
    unsigned char   pad14[0x3dc - 0x14];
    int             f3dc;           /* +0x3dc the side it entered this square by */
    int             state;          /* +0x3e0 */
    int             leg;            /* +0x3e4 squares left in this leg */
    unsigned char   pad3e8[0x3f4 - 0x3e8];
    struct JcBoat*  next;           /* +0x3f4 */
} JcBoat;                           /* 0x3f8 */

extern JcStation* g_jc_stations;    /* 0x00629c3c */
extern JcBoat*    g_jc_boats;       /* 0x00616164 */

extern JcWater* JcWater_FindAt(int x, int y);                  /* 0x004371b0 */
/* Refills the boat's 80-step wobble/frame buffers for a move from side
 * `from` to side `to` (-1 = stay put). */
extern void     JcBoat_Animate(JcBoat* b, int from, int to);   /* 0x00433840 */
extern int      rand(void);                                    /* 0x0049e4b2 (CRT) */

/* =========================================================================
 * 0x004334c0 -- JcBoat_Step: choose the river square this boat crosses next
 * and lay down the 80-frame animation for the crossing.
 *
 * THE SECOND PARAMETER IS NOT A TABLE SELECTOR (the note in junglecruise.c
 * guessed that and it is wrong): it switches on ONE extra rule, the route
 * hint.  With `steer` set, the square's route successor (JcWater +0x18) is
 * compared with the square itself and one link is struck off -- south when
 * the route runs level, otherwise east or west by the sign of the x step.
 * That is the whole difference between states 4 and 8.
 *
 * The choice is made by elimination, in this order:
 *   1. start from the square's own link mask;
 *   2. strike off any direction whose neighbour square is occupied by, or is
 *      the target of, ANOTHER boat -- both that boat's current square
 *      (+0x04/+0x08) and the one it is heading for (+0x0c/+0x10) count, so
 *      boats never swap places or pile up;
 *   3. apply the route hint above when `steer` is set;
 *   4. if nothing is left, animate "stay put" (to = -1), mark the boat stuck
 *      (f3dc = -1) and return;
 *   5. one time in eight, if there is any way on other than straight back,
 *      throw the others away at random until exactly one remains and take
 *      it -- this is the wandering that makes the boats look unscripted;
 *   6. otherwise prefer straight ahead (the side opposite the one it came in
 *      by), then one of the two turns in random order, then back the way it
 *      came as a last resort.
 * The square's own arrival side is then recorded as the OPPOSITE of the way
 * it left, the leg counter is decremented, and reaching zero puts the boat
 * in state 8 -- the steered state, which is how a boat that has wandered
 * long enough is pulled back onto the route to the station.
 *
 * ORIGINAL QUIRKS reproduced: the station search may end on a null cursor
 * which is then dereferenced; step 5 discards the current heading from the
 * candidate set, so the one-in-eight boat NEVER carries straight on; and
 * step 6's last resort recomputes the direction it already rejected first.
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004334c0
void JcBoat_Step(JcBoat* b, int steer)
{
    JcStation* st = g_jc_stations;
    JcBoat*    o = g_jc_boats;
    JcWater*   w;
    int        links;
    int        avail;
    int        cnt;
    int        i;
    int        back;
    int        dir;
    int        step;
    Pos        d;
    char       r;

    w = JcWater_FindAt(b->cx, b->cy);
    while (st && w->owner.w != st->pos.w)
        st = st->next;
    links = w->links;
    if (w->pos.w == st->a.w) {
        links &= ~1;
    } else if (w->pos.w == st->b.w) {
        b->state = 0x10;
        b->leg = 3;
        JcBoat_Animate(b, b->f3dc, 4);
        b->f3dc = 1;
        b->ny = b->cy + 5;
        return;
    }
    while (o != 0) {
        if (o != b) {
            if ((b->cx == o->cx && b->cy - 5 == o->cy)
                || (b->cx == o->nx && b->cy - 5 == o->ny))
                links &= ~1;
            if ((b->cx + 5 == o->cx && b->cy == o->cy)
                || (b->cx + 5 == o->nx && b->cy == o->ny))
                links &= ~2;
            if ((b->cx == o->cx && b->cy + 5 == o->cy)
                || (b->cx == o->nx && b->cy + 5 == o->ny))
                links &= ~4;
            if ((b->cx - 5 == o->cx && b->cy == o->cy)
                || (b->cx - 5 == o->nx && b->cy == o->ny))
                links &= ~8;
        }
        o = o->next;
    }
    if (steer != 0 && w->route_next != 0) {
        d.x = w->route_next->pos.b.x - w->pos.b.x;
        d.y = w->route_next->pos.b.y - w->pos.b.y;
        if (d.y != 0) {
            if (d.x < 0)
                links &= ~8;
            else
                links &= ~2;
        } else {
            links &= ~4;
        }
    }
    if (links == 0) {
        JcBoat_Animate(b, b->f3dc, -1);
        b->f3dc = -1;
        return;
    }
    if ((rand() & 7) == 0) {
        avail = links & ~b->f3dc;
        if (avail != 0) {
            for (;;) {
                i = 0;
                cnt = 0;
                for (; i < 4; i++) {
                    if (avail & (1 << i))
                        cnt++;
                }
                if (cnt <= 1)
                    break;
                avail &= ~(1 << (rand() & 3));
            }
            links = avail;
        }
    }
    for (i = 0; i < 4; i++) {
        if (b->f3dc & (1 << i))
            break;
    }
    back = (i + 2) % 4;
    dir = 1 << back;
    if ((links & dir) == 0) {
        r = (char)rand();
        step = (r & 1) ? 1 : -1;
        dir = 1 << ((back + step) & 3);
        if ((links & dir) == 0) {
            dir = 1 << ((back - step) & 3);
            if ((links & dir) == 0)
                dir = 1 << ((back + 2) % 4);
        }
    }
    switch (dir) {
    case 1:
        b->nx = b->cx;
        b->ny = b->cy - 5;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 4;
        break;
    case 2:
        b->nx = b->cx + 5;
        b->ny = b->cy;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 8;
        break;
    case 4:
        b->nx = b->cx;
        b->ny = b->cy + 5;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 1;
        break;
    case 8:
        b->nx = b->cx - 5;
        b->ny = b->cy;
        JcBoat_Animate(b, b->f3dc, dir);
        b->f3dc = 2;
        break;
    }
    if (--b->leg == 0)
        b->state = 8;
}

/* =========================================================================
 * THE OUTFIT SYSTEM: HOW ONE MESH WEARS DIFFERENT CLOTHES
 *
 * savechunks.c's MakeAnimInstance (0x00442580) gives every bloke a private
 * copy of the animation's 0x24-byte part array and then calls this function
 * TWICE -- once for the "A" outfit table and once for the "B" one -- before
 * handing the copy to RecolourModelParts.  AnimApplyPart is the half that
 * moves TEXTURE COORDINATES, and it does it by rectangle substitution.
 *
 * The model context (the record data2.c gets from GetModelContext, 0x00443710)
 * carries at +0x30 a table of 6-byte rectangles, one per wearable patch:
 *
 *      +0x00  short id     texture id, RELATIVE to the context's base (+0x04)
 *      +0x02  u8    x, y   the patch's top-left corner IN TEXELS
 *      +0x04  u8    w, h   its size, as an inclusive extent (see below)
 *
 * `from` and `to` are indices into that table.  Every part of the instance
 * that is textured (flag bit 0x2000 clear) with the FROM patch's texture and
 * whose three texture coordinates all land inside the FROM rectangle is
 * re-pointed at the TO patch's texture and has its coordinates mapped into
 * the TO rectangle:
 *
 *      u' = ((u * texw(from) - from.x) / from.w * to.w + to.x) / texw(to)
 *
 * i.e. the normalised UV is expanded to texels in the source texture, made
 * relative to the source rectangle, rescaled to the destination rectangle
 * and re-normalised against the DESTINATION texture's size.  So swapping a
 * shirt is one table entry: the artist draws every variant somewhere in the
 * texture atlas, and the model itself never changes.
 *
 * The containment test uses `x .. x + w + 1` inclusive on both axes -- one
 * texel of slack past the stated extent, deliberately, so a patch's own edge
 * coordinates still count as inside.  UVs are clamped to [0,1] first (the
 * upper clamp compares against a DOUBLE 1.0 and assigns a float 1.0f, which
 * is what puts the 8-byte constant in .rdata).
 * ========================================================================= */

/* One triangle's material record inside a person's private part array. */
typedef struct AnimPart {
    int   flags;                    /* +0x00  bit 0x2000 = flat colour */
    int   rgb;                      /* +0x04 */
    int   tex;                      /* +0x08  absolute texture id */
    float uv[3][2];                 /* +0x0c  u,v per corner */
} AnimPart;                         /* 0x24 */

/* One wearable patch: where it lives inside its texture. */
typedef struct OutfitRect {
    short         id;               /* +0x00  texture id, relative to base */
    unsigned char x;                /* +0x02 */
    unsigned char y;                /* +0x03 */
    unsigned char w;                /* +0x04 */
    unsigned char h;                /* +0x05 */
} OutfitRect;                       /* 6 */

typedef struct ModelCtx {
    unsigned char pad00[4];
    int           base;             /* +0x04  texture id base */
    unsigned char pad08[0x30 - 8];
    OutfitRect*   rects;            /* +0x30  the patch table */
} ModelCtx;

/* Every loaded texture's pixel size, indexed by absolute texture id. */
typedef struct TexSize { int w; int h; } TexSize;
extern TexSize g_texsize[];         /* 0x0081c0c0 */

/* =========================================================================
 * 0x00442040 -- AnimApplyPart: re-point every part wearing patch `from` at
 * patch `to`, mapping its texture coordinates between the two rectangles.
 *
 * MakeAnimInstance declares the two index arguments as `void*` because it
 * only ever passes them through; they are plain table indices.
 *
 * ORIGINAL QUIRKS reproduced: the six coordinates are clamped into [0,1] and
 * scaled into texels IN PLACE before the containment test, so a part that
 * fails the test has already had its local copies mangled (harmless, they
 * are discarded); and a part is either wholly remapped or wholly left alone
 * -- there is no clipping.
 * ========================================================================= */

/* 331/331 instructions, no branch escaping the extent, and every phase --
 * the rectangle set-up, the twelve clamps, the six texel scalings, the
 * twelve containment compares and the six remapped stores -- is present in
 * the original's order.  Everything up to the containment test matches
 * except the [esp+N] numbering; the residual is ONE decision inside the
 * remap block, and it is the x87 register allocator's, not the source's.
 *
 * The remap needs TEN float values at once (the eight rectangle edges plus
 * the two destination texture sizes) and the x87 stack holds eight, so VC6
 * keeps six and spills four.  The ORIGINAL keeps `a->x` and the whole Y
 * group (a->y, a->h, b->h, b->y, (float)th) in registers and spills the X
 * group (a->w, b->w, b->x, (float)tw) to the frame; this reconstruction
 * makes the MIRRORED choice -- X group and (float)tw in registers, Y group
 * and (float)th spilled.  Two things follow from that one flip:
 *   1. the original's spilled X group needs three float homes AND a second
 *      int scratch for the `fild`s (the first, at -0x1c, is overwritten by
 *      (float)b->x), which is why its frame has one more pool slot and every
 *      local sits 4 bytes lower than ours;
 *   2. with the stack exactly full the original cannot keep a result in a
 *      register, so each new UV is `fstp`'d into its own local's home and
 *      copied to p->uv with an integer mov pair -- ten instructions this
 *      build saves by storing straight through with `fstp [edx+N]`.
 * The flip survives every source permutation tried: the declaration order of
 * the eight rectangle floats, the assignment order, interleaving the u0
 * computation between the two groups (which is what the original's schedule
 * looks like), computing all six u's before the v's, naming (float)tw and
 * (float)th as float locals, and moving the p->tex store -- VC6 reorders all
 * of them back to the same DAG and makes the same allocation.
 *
 * MEASURED AGAIN 2026-09-04, all worse or structurally wrong (best = 182 with
 * the original's 0x30 frame): the Y group assigned before the X group (191,
 * frame 0x24); all eight assigned before u0 (183); the p->tex store first,
 * last or between the two groups (183-188); the tw/th reads after the X group
 * (188); v0 computed before u0 (217); named `ftw`/`fth` float locals for the
 * two divisors, early (168, frame 0x2c) or in place (165, 321 insns); the six
 * results into six FRESH locals (identical, 182); the p->uv stores interleaved
 * one per computation (156 but frame 0x2c -- fewer locals, so not the
 * original); `p->uv[0][0] = u0;` moved last (195); srcw/dstw/dstx written
 * inline as `(float)a->w` etc. (176, 1166/1174 bytes with the right frame, but
 * it spills BOTH groups and reorders every conversion, so it is not the shape
 * either).  Diagnostic worth recording: the offset-blind distance (all
 * `[esp+N]` collapsed) is 110 for this build and no variant beat it, so ~72 of
 * the 182 are pure frame numbering that follows from the spill flip and cannot
 * be fixed independently.  What VC6 has to be made to do is decide, WHEN IT
 * CREATES the X group, that ten FP values will be live -- the original spills
 * four of the first five as it converts them and then keeps the whole second
 * group, filling the x87 stack exactly (srcx, u0's result, the four Y values
 * and (float)th = 7 plus one working slot); this build greedily keeps the
 * first five and is then forced to spill all five of the second group, leaving
 * two stack slots unused.
 *
 * ROUND OF 2026-09-04 (second pass), re-ranked on STRUCTURE rather than the
 * strict count (scratchpad/laneG/sc.py, regions.py): the shipped body is
 * register+offset-blind LCS 289 of 331 with 42 original indices in a
 * differing region, and it is the BEST of everything measured -- every
 * variant with a lower strict count is structurally worse, which is why the
 * earlier note's `ftw`/`fth` candidates (165/168 strict) must stay rejected
 * (robl 284/265).  New this round, all worse: a `static __inline` remap
 * helper taking the six operands (byte-identical -- the argument temporaries
 * do not reach the x87 allocator); the numerator split into a two-step float
 * local (byte-identical); the Y group and v0 written before the X group and
 * u0 (robl 265); `p->uv[0][0] = u0;` stored immediately after u0 (156 strict,
 * robl 279); a 17-position sweep of the `p->tex` store (byte-identical for
 * positions 0-7, robl 281-291 after, so the store is NOT the may-alias
 * barrier that would stop the Y-group conversions being hoisted); and a
 * `volatile` X group, both as `volatile float` declarations and as
 * `*(volatile float*)&x` reads at each use (331i/1182B, mnemonic LCS 308 --
 * the best mnemonic match seen -- but robl 277/282, because it forces BOTH
 * groups into memory).
 * One more negative: a `volatile` cast on the OutfitRect pointer at the Y
 * group's four reads (the obvious way to stop them being hoisted above u0) --
 * and at the X group's, and at both -- widens the byte loads and rewrites the
 * block (robl 271-276), so the hoist cannot be blocked that way either.
 * THE MECHANISM, now stated exactly.  The original computes u0 BETWEEN the
 * two conversion groups: at that point srcx and the new u0 are on the x87
 * stack, the Y group and (float)th are then pushed on top (7 of 8), and the
 * X group had to be spilled as it was created because 11 values would
 * otherwise be live.  We emit both groups first, so at each Y conversion the
 * X group's next use (u0) is nearer than the Y group's (v0) and VC6 spills
 * the Y group instead -- the exact mirror.  The whole residual is therefore
 * "VC6 hoisted the Y-group `fild`s above u0", and no source order, alias
 * barrier or helper found so far stops it.
 *
 * ROUND OF 2026-09-04 (third pass).  UNCHANGED AT 182 -- nothing credible was
 * found -- but the mechanism is now PROVEN by a probe, and the probe says
 * exactly what a future round has to reproduce.  Two corrections to the
 * analysis above first:
 *   - the shipped body is 321 REAL instructions, not 331: audit.py trims to
 *     the original's extent and the last ten are alignment `nop`s.  The ten
 *     missing instructions are precisely the original's `mov eax,<home>;
 *     mov [edx+N],eax` pairs for five of the six results.
 *   - the original does NOT compute u0 and then store it: u0's result stays
 *     on the x87 stack from index 252 to 317, and is written with
 *     `fstp [edx]` only after five `fstp st(0)` have torn the stack down.
 *     That one permanently-live value is what fills the stack (srcx, u0's
 *     result, srcy, srch, dsth, dsty, (float)th = 7 of 8), which is why
 *     every LATER result has to be spilled to its own home and copied out
 *     with integer movs.  Our build stores u0 straight to p->uv[0][0] and
 *     therefore folds all six.
 * THE PROBE.  Declaring just TWO of the eight rectangle floats `volatile` --
 * `dstx` and `srcy` -- flips the whole allocation to the original's:
 *      metric        shipped   volatile probe   original
 *      audit strict    182         144             0
 *      robl LCS        289         308           331
 *      bad regions      73          38             0
 *      real insns      321         331           331
 *      bytes          1142        1182          1174
 * A full 256-mask sweep of `volatile` declarations and a second full sweep of
 * the volatile-READ form (`*(volatile float*)&v` at each use) were run: the
 * declaration form's optimum is {dstx, srcy} at 308/38/144 and the read
 * form's is 307/39/145 ({dstx,srcy}, {dstw,srcy}, {srcw,dstw,srcy} and three
 * more all tie).  Making the WHOLE X group volatile -- what the previous
 * round tried -- is much worse (272-282), because the original keeps `srcx`
 * in a REGISTER and spills only srcw/dstw/dstx and (float)tw.
 * WHY IT IS NOT SHIPPED: it is not what the original had.  It costs a frame
 * slot (`add esp,0x34` where the original has 0x30), it is 8 bytes long, and
 * it leaves five `fsubr dword ptr [esp+N]` where the original has
 * `fsub st(5)` -- so it approximates the answer rather than reproducing it.
 * WHAT IT PROVES.  VC6 spills by FURTHEST NEXT USE.  With the Y-group
 * conversions left BELOW u0 (as the source has them), the X group's next use
 * after u0 is u1 and the Y group's is v0 -- v0 is nearer, so the X group is
 * spilled, which is the original.  Our build hoists the Y-group `fild`s ABOVE
 * u0, and then at each Y conversion the X group's next use (u0) is nearer, so
 * the Y group is spilled: the exact mirror.  The entire 182 is that hoist.
 * NEW AND INERT AGAINST THE HOIST this round: a hard barrier between u0 and
 * the Y group (`*(volatile int*)&tw = tw;`, a volatile read, a volatile store
 * through u0 -- 207/155/149 strict but robl 271-279, and all three rewrite
 * the prologue); a 7-position re-sweep of the `p->tex` store (byte-identical
 * at position 0-1, robl 281 everywhere else -- confirming again that it is
 * not an alias barrier); a `union { float f; int i; }` carrier for dstx and
 * srcy (byte-identical -- VC6 enregisters it anyway, so the "union makes a
 * scalar address-taken" trap does NOT fire on a float member here); a
 * two-field `struct FPair` carrier (165 strict, robl 287); reading the
 * destination texture size inline as `(float)g_texsize[idB].w` (163, robl
 * 278); fresh `tw2`/`th2` int locals for the second read (165, robl 287);
 * explicit `(float)` cast tuples on the products and on the numerators (both
 * byte-identical -- the cast-tuple lever does not reach the x87 allocator);
 * `srcx` converted last in the X group (184, robl 272); and all three u's
 * before all three v's (byte-identical).
 * Two variants beat the shipped body slightly and are REJECTED as not the
 * original's source: `srcy` assigned last in the Y group (robl 290, bad 69,
 * strict 183 -- but it emits the four Y conversions in the wrong order), and
 * a named `fth` float for the v divisor with `(float)tw` left inline for the
 * u's (strict 179, bad 71, robl unchanged -- an asymmetry no one would write).
 *
 * ROUND OF 2026-09-04 (fourth pass).  UNCHANGED AT 182.  Two diagnostics that
 * narrow where a future round should NOT look, and five more negatives.
 * DIAGNOSTIC 1: the permutation-aware count equals the strict count exactly
 * (real = 182 = mism, best permutation `ebx->ebx ebp->ebp edi->edi esi->esi`,
 * i.e. the identity), and that permutation is invariant across every variant
 * measured here.  The INTEGER allocation is therefore already the original's
 * everywhere; the whole 182 is x87 order plus the frame numbering that follows
 * from it.  Nothing that touches integer register pressure can help.
 * DIAGNOSTIC 2: the frame difference is now itemised.  Both frames are 0x30.
 *   original  0x10 v2 | 0x14 th | 0x18 tw->(float)tw | 0x1c srcw | 0x20 dstw |
 *             0x24 int-scratch->dstx | 0x28 i | 0x2c..0x38 x0,x1,y0,y1 |
 *             0x3c a SECOND int scratch, above the named locals
 *   ours      0x10 int-scratch | 0x14 (float)th | 0x18 v2 | 0x1c srcy |
 *             0x20 srch | 0x24 dsth | 0x28 dsty | 0x2c i | 0x30..0x3c x0..y1
 * Seven pool slots below `i` here against the original's six, so every named
 * local sits 4 bytes high; the original's seventh scratch is a COMPILER TEMP
 * created after the Y group and therefore laid out after the named locals.
 * That is a consequence of the spill flip, not something a declaration can
 * reach: the pool below `i` holds whichever group got spilled.
 * NEW AND INERT (all exactly 182/robl 289/bad 73, byte-identical): the Y group
 * moved into its OWN nested block scope opened after u0 (the obvious way to
 * give it a separate pool and a later live range); u0 split so its division
 * happens BELOW the Y group's four assignments (`u0 = (u0-srcx)/srcw*dstw+dstx;
 * ... u0 = u0/(float)tw;`); and the last four computations reordered to
 * u1,u2,v1,v2.  VC6 rebuilds the same DAG from all three.
 * NEW AND WORSE: the six `p->uv` stores interleaved one per computation but
 * with `p->uv[0][0] = u0;` kept LAST -- which is exactly the original's STORE
 * ORDER (v0, u1, v1, u2, then u0 and v2) -- is 195/robl 270/1136 bytes, and so
 * is the same with the `p->tex` store moved to the end.  The original's
 * interleaving is the scheduler filling FP latency slots with the integer copy
 * pairs, not a source order; asking for it in source destroys the block.
 * The `p->tex` position was re-swept over all 23 positions with the full metric
 * set this time (the earlier sweep recorded only robl): positions 4-7 are
 * byte-identical to the shipped body, position 9 is the only one that beats it
 * structurally (robl 291, bad 69) and it costs a strict index and puts the
 * store in the middle of the Y group's four assignments, which is not source.
 * `ftw`/`fth` float locals were re-measured with all metrics at the exact
 * positions the original converts them (just above u0 and just above v0):
 * 165 strict but robl 284, bad 69, 321 real instructions and 1136 bytes -- the
 * same attractor the earlier rounds rejected, still rejected.
 *
 * Two levers that ARE settled and must not be undone: the int->float
 * conversions have to go through named float locals (written inline VC6
 * lowers `float_expr - int_field` to `fisub`/`fidiv` on the widened byte
 * instead of the original's convert-once `fild`/`fstp dword`/`fsub st(n)`),
 * and `tw`/`th` must be read from g_texsize BEFORE the eight rectangle
 * edges, which is what puts the two table loads at the head of the block as
 * the original has them. */
/* ROUND OF 2026-09-04 (fifth pass).  UNCHANGED AT 182.  One new structural
 * observation and eleven more negatives; the diagnosis in the round above is
 * confirmed in detail and NOTHING in this function responds to statement order.
 * WHAT THE ORIGINAL'S x87 STACK ACTUALLY DOES, traced slot by slot, so that a
 * future round can check a candidate against it without re-deriving it:
 *   226 fild srcx  -> KEPT on the stack for the whole block (it is `fsub st(1)`
 *       at u0, `fsub st(7)` at u1 and at u2, and is popped by the LAST
 *       `fstp st(0)` at 319)
 *   229/233 srcw, 234/238 dstw, 239/243 dstx, 244/246 (float)tw -> each
 *       converted and IMMEDIATELY spilled, so u0 reads four memory operands
 *   247-252 u0 -> stays on the stack from here to index 317, where it is
 *       stored DIRECTLY as `fstp dword [edx]` (p->uv[0][0]); its stack home
 *       [esp+0x50] is never written
 *   253-264 srcy, srch, dsth, dsty, (float)th -> ALL FIVE kept, so v0/v1/v2
 *       are `fsub st(5) / fdiv st(4) / fmul st(3) / fadd st(2) / fdiv st(1)`
 *   the other five results go back to their float locals' homes and are copied
 *       into p->uv with INTEGER moves (288/289, 290/291, 295/296, 304/305,
 *       311/318) -- which is what a trailing block of six `p->uv[i][j] = ...`
 *       stores lowers to when the x87 stack is full, and is exactly the source
 *       shape this file already has.
 * Our build is the mirror: the X group stays (5 registers), the Y group is
 * spilled (5 homes, one more than the original's four), which is the seventh
 * pool slot below `i` and hence the +4 on every named local from index 22 on.
 * INDICES 22-216 ARE OTHERWISE IDENTICAL -- the whole clamp/compare block
 * matches instruction for instruction under a constant +4 frame shift, so the
 * strict count massively overstates the distance.
 * NEW AND INERT THIS ROUND (all exactly 182/robl 289/bad 73, byte-identical):
 * the `p->tex` store moved to just after the u0 line (the alias-barrier idea,
 * re-tested with the full metric set); the Y group moved below u1; all three
 * u's before the Y group and all three v's after.  NEW AND WORSE: `p->tex`
 * last (192), `p->tex` just above v0 (183), `th` read after u0 (189), `th`
 * read with the Y group (190), the Y group and v0 hoisted above the X group
 * (191, first divergence 0), `p->uv[0][0] = u0;` immediately after u0 (165 but
 * robl 272, first divergence 0), the u0 store moved to the end of the store
 * block (195, robl 270).
 * The conclusion of the fourth pass stands and is now certain: VC6 rebuilds the
 * same DAG from every statement order, so the Y-group hoist has to be stopped
 * by an aliasing or a volatile fact, and every such fact tested so far costs
 * the frame.  Nothing that touches integer register pressure can help (the
 * permutation-aware count still equals the strict count under the IDENTITY
 * permutation). */
// WIP-FUNCTION: LEGOLAND 0x00442040  (321 real insns of the original's 331 -- audit pads to 331 with alignment nops -- 182 mismatches, and 182 under the best callee-saved permutation too, so the integer allocation is already exact; the x87 spill group is mirrored)
void AnimApplyPart(ModelCtx* ctx, int from, int to, AnimPart* parts, int n)
{
    OutfitRect* a = &ctx->rects[from];
    OutfitRect* b = &ctx->rects[to];
    int         idA = a->id + ctx->base;
    int         idB = b->id + ctx->base;
    int         x0 = a->x;
    int         x1 = a->x + a->w + 1;
    int         y0 = a->y;
    int         y1 = a->y + a->h + 1;
    AnimPart*   p;
    int         i;
    int         tw;
    int         th;

    if (n <= 0)
        return;
    p = parts;
    i = n;
    do {
        if ((p->flags & 0x2000) == 0 && p->tex == idA) {
            float u0 = p->uv[0][0];
            float v0 = p->uv[0][1];
            float u1 = p->uv[1][0];
            float v1 = p->uv[1][1];
            float u2 = p->uv[2][0];
            float v2 = p->uv[2][1];

            if (u0 < 0.0f)
                u0 = 0.0f;
            if (u0 > 1.0)
                u0 = 1.0f;
            if (v0 < 0.0f)
                v0 = 0.0f;
            if (v0 > 1.0)
                v0 = 1.0f;
            if (u1 < 0.0f)
                u1 = 0.0f;
            if (u1 > 1.0)
                u1 = 1.0f;
            if (v1 < 0.0f)
                v1 = 0.0f;
            if (v1 > 1.0)
                v1 = 1.0f;
            if (u2 < 0.0f)
                u2 = 0.0f;
            if (u2 > 1.0)
                u2 = 1.0f;
            if (v2 < 0.0f)
                v2 = 0.0f;
            if (v2 > 1.0)
                v2 = 1.0f;
            tw = g_texsize[idA].w;
            th = g_texsize[idA].h;
            u0 = u0 * tw;
            u1 = u1 * tw;
            u2 = u2 * tw;
            v0 = v0 * th;
            v1 = v1 * th;
            v2 = v2 * th;
            if (u0 >= x0 && u0 <= x1 && u1 >= x0 && u1 <= x1
                && u2 >= x0 && u2 <= x1
                && v0 >= y0 && v0 <= y1 && v1 >= y0 && v1 <= y1
                && v2 >= y0 && v2 <= y1) {
                float srcx;
                float srcw;
                float dstw;
                float dstx;
                float srcy;
                float srch;
                float dsth;
                float dsty;

                tw = g_texsize[idB].w;
                th = g_texsize[idB].h;
                srcx = a->x;
                srcw = a->w;
                dstw = b->w;
                dstx = b->x;
                p->tex = b->id + ctx->base;
                u0 = ((u0 - srcx) / srcw * dstw + dstx) / (float)tw;
                srcy = a->y;
                srch = a->h;
                dsth = b->h;
                dsty = b->y;
                v0 = ((v0 - srcy) / srch * dsth + dsty) / (float)th;
                u1 = ((u1 - srcx) / srcw * dstw + dstx) / (float)tw;
                v1 = ((v1 - srcy) / srch * dsth + dsty) / (float)th;
                u2 = ((u2 - srcx) / srcw * dstw + dstx) / (float)tw;
                v2 = ((v2 - srcy) / srch * dsth + dsty) / (float)th;
                p->uv[0][0] = u0;
                p->uv[0][1] = v0;
                p->uv[1][0] = u1;
                p->uv[1][1] = v1;
                p->uv[2][0] = u2;
                p->uv[2][1] = v2;
            }
        }
        p++;
    } while (--i);
}
