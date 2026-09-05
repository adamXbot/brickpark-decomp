# Scope J work log

Baseline: `origin/main` at `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474`, fetched 2026-09-05.
Branch: `scope/J`; worktree: `/Users/systemadmin/Downloads/legoland/legoland-scope-j`.

## Stage plan and pass conditions

1. Establish isolation and inventory. Check branch/base, clean worktree, and assign every tracked `LEGOLAND/*.c` file to one subsystem group.
2. Consolidate source evidence in parallel into subsystem pages: layouts, behaviour, constants, original bugs, and callbacks. Each group must account for every assigned source and cite facts to readable local links. Ambiguous or incomplete evidence stays explicitly partial.
3. Integrate the coverage table, current callback names, reference formats and disagreement register in `docs/RUNTIME_SPEC.md`. Check that every source file is accounted for, every relative source link resolves, and the brief's named mechanics are present.
4. Review uncertain claims against their sources, run documentation-only diff/link/coverage checks, then commit on `scope/J` and push this branch. Check that all changed files are `docs/RUNTIME_SPEC.md` or below `docs/runtime/`.

## Scope interpretation

The scope-specific documentation-only brief takes precedence over the shared compiler workflow. No compiler, binary verification, progress generator, toolchain setup, C edit, or shared report edit is needed. Source snapshot is fixed at the base commit so concurrent scope G/H work does not change this document midway.

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
