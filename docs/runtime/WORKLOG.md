# Scope J work log

Baseline: `origin/main` at `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474`, fetched 2026-09-05.
Branch: `scope/J`; worktree: `/Users/systemadmin/Downloads/legoland/legoland-scope-j`.

## Stage plan and pass conditions

1. Establish isolation and inventory. Check branch/base, clean worktree, and assign every tracked `LEGOLAND/*.c` file to one subsystem group.
2. Consolidate source evidence in parallel into subsystem pages: layouts, behaviour, constants, original bugs, and callbacks. Each group must account for every assigned source and cite facts to readable local links. Ambiguous or incomplete evidence stays explicitly partial.
3. Integrate the coverage table, current callback names, reference formats and disagreement register in `docs/RUNTIME_SPEC.md`. Check that every source file is accounted for, every relative source link resolves, and the brief's named mechanics are present.
4. Review uncertain claims against their sources, run documentation-only diff/link/coverage checks, then commit on `scope/J` and push this branch. Check that all changed files are `docs/RUNTIME_SPEC.md` or below `docs/runtime/`.

## Scope interpretation

The scope-specific documentation-only brief takes precedence over the shared compiler workflow. The initial pass needed no compiler, binary verification, progress generator, toolchain setup, C edit or shared report edit. It fixed a source baseline for each audit stage. The subsequent user-authorized recovery pass added read-only binary/asset checks and explicit integrations of advancing main, recorded below; the documentation-only edit boundary remained unchanged.

## Progress

- Stage 1: new worktree created on the required branch from freshly fetched `origin/main`; no inherited changes. No applicable `AGENTS.md` found.
- Stage 2 passed: parallel transport, attraction and presentation syntheses completed; core/world, persistence and asset contracts consolidated. Every tracked C file has one primary coverage row.
- Stage 3 passed: index, disagreement register and callback reference integrated. The final automated check resolved2,343 local links across11 Markdown files, matched183 coverage rows to Git, and found no table/anchor/required-topic errors. Independent callback review matched618 direct assignments and628 linked name/address pairs.
- Stage 4 review passed: corrected ODF branching, worker-save semantics, direction encodings, raw LoadPos scalars and library null-clearing after independent review. Remaining uncertainties are visible in the main index. Documentation-only whitespace and scope checks passed. Commit and branch push follow this review; their outcome is reported with the delivered revision.

## Completion audit and PR plan

1. Compare every deliverable requirement with the committed specification; inspect the current main-branch delta and existing PR state. Pass when the target and source baseline are explicit and every requirement has an audit owner.
2. Independently audit recovered headers, decoded tables, original-bug notes and declared coverage gaps. Add any omitted recoverable material; leave only source-level unknowns. Pass when each review records concrete findings and resolutions.
3. Run reproducible link, table, source-inventory, callback and allowed-path checks on the final documents. Review the complete PR diff against current main. Pass when checks have no errors and all findings are resolved or explicitly identified as source limitations.
4. Commit and push the completed documentation, then create or update a pull request targeting main. Pass when the remote PR points to the final branch commit and reports no merge conflict.

Scope J completion means consolidation of recoverable source evidence with explicit unknowns, as required by the brief; it does not mean inventing missing game behavior or declaring the original decompilation complete.

## Current-main integration

The final pre-PR fetch found main had advanced from `f22f7cc7` to `f8f5854481b2a87fb456a37b02ce581e9206b400`. Both Scope J documentation commits were preserved and rebased onto that commit. A second parallel delta review covers all9 newly added C files,12 modified C files and8 new lane notes. No source changes originate from Scope J; the PR diff remains documentation only. Final inventories and checks target the192-source baseline.

The allowed-path gate compares the branch working tree with the PR merge base, so concurrent changes on main are not incorrectly reported as Scope J deletions. Source fingerprints and primary inventory checks still compare the checked-out source snapshot directly.

## Completion gates

- Requirements and evidence: passed for all192 current-baseline sources and26 lane notes, including the current-main delta.
- Independent review: core plus transport/attraction/presentation audits complete; recoverable omissions and discovered source/header disagreements resolved, with actual source boundaries retained.
- Reproducible gate: passed for192 primary rows,628 linked callback names/addresses,618 direct assignments,57 source fingerprints, local links/anchors, table structure, required topics and allowed paths.
- Delivery: commit/push and a pull request targeting main follow these checks. The PR's final head and mergeability are verified through GitHub and reported with the delivered PR link.

## Recovery pass toward full coverage

The user requested continued work beyond the completed consolidation. This pass targets the63 partial source entries directly, using read-only examination of the original executable and available game assets in the main checkout as additional evidence. Scope J still changes only its Markdown documents and runs no compiler.

1. Establish available evidence and a reproducible baseline: inventory every partial reason, hash the original executable, identify its PE sections and available resource archives. Pass when each gap has an assigned owner and a concrete source/binary/data investigation.
2. Recover in parallel: transport, attractions and presentation own their existing pages plus separate data-evidence pages; the parent owns world/persistence/assets and shared coverage. Recover static tables, trace ambiguous consumers and document missing helpers from original instructions where bounded. Pass when new conclusions have named addresses, byte interpretations or consumer references, and unresolved items retain precise reasons.
3. Integrate and challenge: verify extracted bytes/table cardinalities and cross-page contracts, rerun the documentation gate and audit every proposed partial-to-documented change. Opaque padding may remain opaque when it has no recovered behavioral use; an actual missing operation must remain a gap.
4. Update the existing PR: commit and push only verified documentation, inspect the remote head and diff, and report exact remaining coverage. Do not claim100% recovery or silently change the coverage definition to hide missing evidence.

## Recovery evidence and second main integration

- Original executable verified: SHA-256`c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`; strict PE reads reject BSS as file data. All recovery is read-only; only Markdown changes are committed.
- Core checks pass19 binary ranges and all displayed original tables,19 complete character assets,595 resource paths/610nodes/581physical offsets. The strict directory check caught an obsolete duplicate animation and corrected an inherited member-count claim.
- Transport, attraction and presentation data/behavior recovery passes completed with independent byte/asset checks; their former63-partial ledger is being regenerated after the AI closure and new-main audit.
- A final-target fetch found main advanced to`263cf60b173a8d054e356c5916341cc664033713`. It added coaster8,lfmisc2,musicthread,savemisc2,uimisc3. Merged into the Scope J worktree without conflicts; audit owners are transport/transport/assets/persistence/presentation respectively. Inventory is now197. The new source files are main’s changes, not Scope J changes.
- The new savemisc2 writer independently confirms the original binary string proof; lfmisc2 independently supplies both power-station callbacks. MusicThread’s full scheduling contract is integrated, correcting header overclaims about failed segment lookup and unchecked download pointers.

## Final target update

The next pre-delivery fetch advanced main to `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`. Its new coaster9.c and ridemachine.c bring the inventory to199; the two existing C edits only update matching markers/comments after main's extent-walker fix. Scope J merged main without conflicts and assigned the new files plus codex-e lane to the transport/attraction reviewers. All prior original-data checks remain valid. The final inventory includes27 lane notes.

This integration does not change Scope J's boundary: no compiler, game execution, C edits or tool edits originate from this branch. Main's independent changes are excluded by the PR merge-base path gate.

## Final recovery gates and delivery

- Coverage gate passed:199 of199 sources documented,0 partial,0 unassigned;27 lane references. The six primary groups are36 world,7 persistence,33 assets,35 transport,30 attractions and58 presentation sources.
- Final documentation gate passed across22 Markdown files:4,132 local links,199 source rows,630 callback links,618 direct assignments and60 source fingerprints. Table headers/widths, anchors, required topics, whitespace and allowed paths all pass.
- All six evidence pages' published recipes pass. Final delta checks included47 transport static blocks,24 attraction bodies/five assets,36 exact hint formats,26 high-level slots and16 low-level states. The last review resolved Carousel's ten actual seat bytes and the whole-line model parser, rather than preserving stale “unknown” notes.
- Independent reviewers checked the AI fault paths, core binary/asset contracts, new music/texture/narration source, and each transport/attraction/presentation source delta. No open documentation finding remains.
- Delivery uses the existing PR#1 targeting main. The final documentation is committed and pushed on scope/J; the remote head and mergeability are checked after the update. Original-game execution, save round-trips and pixel/audio equivalence remain outside this documentation-only scope.
