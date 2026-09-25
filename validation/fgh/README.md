# F/G/H validation

These checks supplement the repository's `tools/audit.py` and `tools/relocs.py`.
They do not change the matching toolchain, game image, completion markers, or
shared reporting tools. Passing execution tests does **not** make a WIP exact.

## Requirements

Use the existing VC6 SP3 toolchain and Python environment described in
[docs/DECOMP.md](../../docs/DECOMP.md) (the coordination note this once cited is
archived; see [docs/ARCHIVE.md](../../docs/ARCHIVE.md)). The worktree needs its own ignored `toolchain` link
and `original/legoland.exe`. Python dependencies are listed in `requirements.txt`.
Only the execution tests need Unicorn.

The September 6 checkpoint used Unicorn 2.1.4 installed in the temporary directory
`/private/tmp/legoland-fgh-deps`, exposed through `PYTHONPATH`. It did not modify
the shared virtual environment. In the Codex macOS sandbox, Unicorn failed during
CPU cache initialization; the same smoke test and function tests succeeded with
approved execution outside the sandbox. Do not interpret that initialization
failure as a game-code mismatch.

## Run

From this worktree, after setting `LEGOLAND_CL` and making dependencies available:

```sh
python validation/fgh/check.py --output /tmp/fgh-check.json
python validation/fgh/blocked_ahead.py --random-cases 2000 --self-test --output /tmp/fgh-blocked.json
python validation/fgh/river_seats.py --output /tmp/fgh-river.json
```

Use the project's Python executable in place of `python` when necessary.
`blocked_ahead.py --source /absolute/path/candidate.c` tests a scratch candidate
translation unit without changing the retained reconstruction.

## What is checked

- `check.py` derives the 48 assigned functions from the original scope briefs,
  compiles their 30 files with `/W3 /O2 /Gy /Gd`, and measures all 419 annotated
  functions. For normalized exact bodies it also checks instruction widths,
  internal branch destinations, relocated jump-table entries, annotated address
  identities, and floating/string literal contents. Any compilation warning,
  previous normalized-match regression, or falsely exact assigned target fails.
  Existing issues outside the assigned targets remain visible in the report.
- `blocked_ahead.py` executes the original and independently compiled
  `SchoolCarBlockedAhead`. It compares return values, car memory, non-stack
  writes, floating-point state, helper calls, stack balance, and preserved
  registers. A separate arithmetic model checks the result. Fixed boundaries,
  empty/self-only lists, last-hit selection, exceptional headings, integer
  wrapping, randomized lists and all four rounding modes are covered.
  The original `0x458930` conversion helper executes unchanged: it uses the x87
  rounding mode, which is significant here. With `--self-test`, two deliberately
  faulty C candidates must fail.
- `river_seats.py` executes the two seat-switch **blocks** inside
  `JungleCruise_UpdateRiverAnim`, covering both drawing directions, 16 headings,
  all three seats, three frame indices and three initial heights. It compares
  height, rotation bits, writes and floating-point state. It recreates the
  historical missing-seat-1 adjustment and a wrong angle constant in temporary
  C candidates. Both pass normalized matching; branch/literal checks and
  execution must reject them. This does not exercise the whole rendering loop.

The emulator loads the user's PE image and actual COFF section relocations;
it does not execute the address-normalized bytes used by the matching tools.
Ambiguous external symbols and unsupported relocations fail explicitly.
It is a function-test environment, not a Windows emulator or a full-game test.

Unannotated helpers, named static data and unsupported literal forms remain
unresolved in the address report, as permitted by the existing contract.
Literal checks establish content equality rather than linker placement.
Sampled execution is evidence for covered cases, not a proof for every input.

The committed `results/` files record this checkpoint. Full listings, exploratory
candidates and the full 419-function report remain in local `scratchpad/fgh/`.
