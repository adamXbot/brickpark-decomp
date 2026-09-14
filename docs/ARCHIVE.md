# Archived working notes

Most of this project was built as a series of parallel work sessions, each with
a written brief and a notes file. Those briefs and notes (`docs/SCOPE_*.md`,
`docs/lanes/`, the session handoff and a few coordination documents) recorded
measurements, dead ends and decisions as they happened. They were useful while
the work was under way, but they are a work log rather than documentation, so
they were removed from the tree.

They remain in the repository history. Source comments and the documents in
`docs/` still cite some of them by path, for example `docs/lanes/scope-f.md`.
To read one:

```bash
# The commit that removed the notes, then its parent, which still has them.
REMOVED=$(git log --diff-filter=D --format=%h -1 -- docs/lanes)
git show "$REMOVED^:docs/lanes/scope-f.md"

# List everything that was archived.
git ls-tree -r --name-only "$REMOVED^" docs | grep -E 'docs/(SCOPE_|lanes/|HANDOFF)'
```

The documents that stay in `docs/` are the maintained references:
[DECOMP.md](DECOMP.md) (matching workflow), [LEVERS.md](LEVERS.md) (VC6 code
generation levers), [FORMATS.md](FORMATS.md), [BINARIES.md](BINARIES.md),
[INSTALLSHIELD_Z.md](INSTALLSHIELD_Z.md), [RIDE_CALLBACKS.md](RIDE_CALLBACKS.md),
[RUNTIME_SPEC.md](RUNTIME_SPEC.md) with [runtime/](runtime/), and
[QUIRKS.md](QUIRKS.md).
