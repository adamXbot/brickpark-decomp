# Scope LL18 — partials wave B: scheduling / cold-block residuals on main

Branch `scope/LL18` from `origin/main` `b39f261b`. **PARTIAL scope**: edit only
the WIP bodies below and the notes above their markers (see
`docs/PARALLEL_CONTRACT.md`, "Extra rules for PARTIAL scopes"). Object
prefix `/tmp/sll18_`. Findings go in `docs/lanes/scope-ll18.md`.

## Functions (4)

| address | name | file | residual (from the marker) |
| --- | --- | --- | --- |
| 0x00470620 | CheckWorkerOnMouseStatus | workers2.c | 55.4%, 82/184 strict; dead rematerialization and cmp/test floor; first 102 |
| 0x0046c7e0 | LoadScriptEvent | savechunks2.c | 79.0%, 26/124 strict; cold-block ordering floor; first 95 |
| 0x004724a0 | DrawPopUpInfo | popup.c | 98.6%, 13/962 strict; memory-home/scheduling floor; first 590 |
| 0x00492db0 | MusicThread | musicthread.c | 3161/3161 insns, 11325/11335 bytes, mismatch 7 |

Also carry the LL14 lever (`docs/lanes/scope-ll14.md`, "Closing 0x0046f9a0"):
VC6 ranks oversubscribed callee-saved candidates by static name-appearance
count; a named temporary that carries one load to several consumers changes
the ranking. Where a residual is register-blind zero it will not help; where
a value's register decides a schedule it may.

## Rules

Same as every PARTIAL scope: read the note above each marker first and do
not repeat it; reconstruction-error pass first; `audit.py` on the whole file
after every change (PASS, `[OK]` count never drops); `relocs.py` zero
`MISMATCH` and `/W3` clean before each commit; WIP marker until `[OK]`;
retire honestly with a paragraph in the note. Do not run `verify.py` /
`progress.py` / `coverage.py`; do not edit `tools/`, `docs/DECOMP.md`,
`docs/HANDOFF.md`, `README.md`. Commit to `scope/LL18` only; no push; no
merge; **no Co-Authored-By trailer of any kind.**
