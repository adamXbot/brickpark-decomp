# Scope LL4 — coaster shade / blit table callees (inventory group 4)

Branch `scope/LL4`. File `LEGOLAND/coastershade2.c`. Object prefix `/tmp/sll4_`.
Brief: `docs/SCOPE_LL4_coaster_shades.md`.

## Status

| address | name | insns | % | audit | marker |
| --- | --- | ---: | ---: | --- | --- |
| 0x0041f4e0 | *(open)* | 78 | — | — | — |
| 0x0041f880 | TrackCursor_RetreatGeometry | 29 | 100 | [OK] | FUNCTION |
| 0x0041f8d0 | *(open)* | 106 | — | — | — |
| 0x0041fba0 | *(open)* | 136 | — | — | — |
| 0x0041fd80 | *(open)* | 162 | — | — | — |
| 0x0041ff80 | *(open)* | 202 | — | — | — |
| 0x00420200 | *(open)* | 81 | — | — | — |
| 0x00420780 | GetCoasterTexture | 3 | 100 | [OK] | FUNCTION |

**2 / 8 exact.** `audit.py` PASS on the two closed bodies; `relocs.py` zero
MISMATCH; `/W3` clean.

## Names

- `GetCoasterTexture` — indexed reader of `g_coaster_tab_c` (0x004d89c8),
  the `.ltx` table `LoadCoasterData` fills through `CoasterModel_LoadLTX`.
  Sibling of `LoadCoasterMesh` / `LoadCoasterMeshTex` / `GetCoasterModelSize`,
  which resolve a name first; this one takes the part index
  `FindCoasterPart` already produced.
- `TrackCursor_RetreatGeometry` — inverse of `TrackCursor_AdvanceGeometry`
  (schoolcar8.c). `RouteGeom` here carries both `+0x50` next and `+0x54`
  prev; schoolcar8's `RouteGeom` only names next.

## Mechanics

- **GetCoasterTexture(i)** returns `g_coaster_tab_c[i]`.
- **TrackCursor_RetreatGeometry(cursor)**: if `cursor->geom->prev` is live,
  store it and return. Otherwise move onto `cursor->node->prev` (+0x1c),
  reseat with `GetTrackNodeWorldPos`, then walk that piece's `+0x50` chain
  to its last object. The first `next` test uses the call result; the walk
  reloads `cursor->geom` each step.

## Levers

- Retreat walk: keep the `GetTrackNodeWorldPos` result in a named local for
  the first `if (geom->next)` test, then assign `cursor->geom =
  cursor->geom->next` in the loop (do not walk a second local). The
  `geom = cursor->geom->next; cursor->geom = geom;` spelling emitted
  `add eax,0x50` pointer arithmetic and stalled at 25/29.
