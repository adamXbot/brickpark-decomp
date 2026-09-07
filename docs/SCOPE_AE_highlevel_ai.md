# Scope AE — high-level bloke AI table handlers (inventory group 13 live) (2026-09-07)

> **Status: IN PROGRESS (integrator cut 2026-09-07 for Grok).** Branch
> `scope/AE`. Notes: `docs/lanes/scope-ae.md`. Object prefix `/tmp/sae_`.
> Cut from inventory group 13 live members only (skip the 11 DEAD bodies).

**Read `docs/PARALLEL_CONTRACT.md` first.** Relocation gate required.

NEW-FUNCTION scope, **7 functions, ≈655 instructions**, one new file.
Neighbours: `blokeai.c` (`DoHighLevelAI`), tables at **0x004b8300**. Name
handlers from what they do and from the table slots that reach them.

## `LEGOLAND/highlevelai.c`

| address | provisional name | insns | reached by |
| --- | --- | ---: | --- |
| 0x0044fe10 | `HighAI_sub_44fe10` | 43 | table 0x004b83c4 |
| 0x0044fe80 | `HighAI_sub_44fe80` | 337 | table 0x004b839c — largest; leave until siblings land |
| 0x00450250 | `HighAI_sub_450250` | 77 | table 0x004b83a0 |
| 0x00450330 | `HighAI_sub_450330` | 43 | table 0x004b83b8 |
| 0x004503a0 | `HighAI_sub_4503a0` | 87 | called by 0x00450450 |
| 0x00450450 | `HighAI_sub_450450` | 48 | table 0x004b83a4 |
| 0x00450a40 | `HighAI_sub_450a40` | 20 | called by gameframe 0x00458ee0 |

**Order:** smallest first → `0x0044fe80` last. Rename from body evidence;
provisional `HighAI_sub_*` names are placeholders.

**Do not** match the DEAD cluster at 0x004511e0..0x00451550 unless a live
caller appears (inventory marks them unreferenced).

## Owned elsewhere

`blokeai.c`, `lowlevelai.c` (Z), `goalstate.c` (AA), F/G/H, V, AB, AC,
Codex-F, every existing `.c`.
