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
