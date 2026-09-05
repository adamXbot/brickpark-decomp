# Scope J verification

Baseline: `f22f7cc7fa95f2d5740f89f4b53ae2624cc9e474`; branch `scope/J`; review date 2026-09-05. This is a documentation check, not runtime, compiler or original-binary validation. [Scope restrictions](../SCOPE_J_runtime_spec.md)

## Checks and results

- **Inventory:** compared the coverage table against `git ls-files 'LEGOLAND/*.c'`. All183 sources appear once as primary rows, and each has a source citation on its assigned page. Coverage is94 documented and89 partial.
- **Links:** resolved every relative Markdown file target and Markdown heading fragment in the index and runtime pages. No missing target or anchor remains.
- **Formatting:** checked unescaped pipe counts in each Markdown table and ran `git diff --check`. No malformed table row or whitespace error remains.
- **Registration audit:** independently compared618 final direct assignments across screen, interfaces, ridesave and castle providers. All matched; the alternate library table was checked separately. All628 linked function-name/address pairs matched source definitions. Two power-station add callbacks retain declaration names and addresses because their bodies are absent.
- **Named mechanics:** presence checks cover15 required topics, including boat size, TrackJoint, RK4, CSP, z commands, visitor thresholds, Gold Rush drift, Joust freeze, Balloonz, footer/caption faults, pan slots, one-way roads, bisection and the flume parent-null defect. This checks discoverability; it does not prove those mechanics correct.
- **Scope isolation:** all changed paths are `docs/RUNTIME_SPEC.md` or Markdown below `docs/runtime/`. No C, tool, shared report or unrelated worktree file was changed. No compiler ran.

The inventories and independent audit are reproducible from the [coverage rows](coverage.md) and [callback source links](callbacks.md). The worktree and staged plan are recorded in the [work log](WORKLOG.md).

## Skeptical review corrections

An independent review of the assembled documentation caught and corrected the ODF DLL/custom branching, worker-save removal predicate and direction of position copying, two movement direction encodings, and LoadPos scalar interpretation. A second pass corrected the library exception to the omitted-callback rule. The callback audit found no omitted direct registrations or incorrect linked implementation names. [Callbacks](callbacks.md), [persistence](persistence.md), [world](world.md), [assets](assets.md)

Numerical spot checks covered WorkerSave, COMP framing/opcodes, BNV/RIN offsets and morph-face layout. The subsystem authors separately checked transport, attraction and presentation source coverage and citations. Remaining uncertain interpretations, external data and incomplete recovered behavior are retained in the [main disagreement register](../RUNTIME_SPEC.md#reconciled-disagreements) and [source coverage](coverage.md).

These checks establish an internally navigable, scoped evidence document. They do not establish save compatibility, visual parity, complete table recovery or execution equivalence with the original game.
