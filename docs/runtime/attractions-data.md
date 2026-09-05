# Attraction binary and data evidence

This supplement closes the missing attraction tables and machine helpers identified in the source audit. It reads the original x86 executable and selected installed archive members; it does not execute the game or treat a new C reconstruction as verification. Addresses are original virtual addresses, integers are little-endian, and function end addresses are exclusive. The matching source baseline is `cf8e88c845dc4b109bbb2bd9f2a31d1177a9c9aa`. [original executable](../../../legoland/original/legoland.exe), [source audit](attractions-audit.md), [Scope J brief](../SCOPE_J_runtime_spec.md)

## Evidence identity and bounds

The executable has 802,857 bytes and SHA-256 `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`. The archive has 16,424,086 bytes and SHA-256 `b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2`. Every executable read below is confined to a PE section’s file-backed raw bytes; uninitialised virtual tails cannot supply table values. The extractor at the end independently validates these identities and all listed byte digests. [original executable](../../../legoland/original/legoland.exe), [original Legoland.res](../../../legoland/gamedata/disc/Legoland.res)

## Table byte manifest

`i`, `I` and `b` denote signed32, unsigned32/pointer bits and signed8. Counts are scalar counts, with rows decoded below. The Spider ON/OFF and Cafe walk/depth entries deliberately describe indexed byte views: overlap is preserved rather than assigned a fictitious independent allocation. SHA-256 covers exactly the byte count shown. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [ridecb4.c](../../LEGOLAND/ridecb4.c)

| Table | VA | Type × count / bytes | SHA-256 |
| --- | --- | --- | --- |
| Catapult layers | `0x004b40a4` | `i × 4` / 16 | `3bd7a5422018f7829fea9b76532513d2998d68bdab66927ee9bb7ccbd3fd69c9` |
| Catapult landing Y | `0x004b40b4` | `i × 4` / 16 | `7b1624186471d7452be26a65de6e27237b3f6e8d85c4efb620dc547d105deecb` |
| Catapult FX rows | `0x004b40c8` | `I × 12` / 48 | `fbbee2ee290740329019282d141d7169932d07eab985f82a55a1ebf965b5d3f7` |
| Safari ON limits | `0x004b4cc4` | `i × 8` / 32 | `2831b391d126d8fb23a14e9d1d9386bafad64f36e0b7e4c2dc7527cb832eedec` |
| Safari OFF limits | `0x004b4ce4` | `i × 8` / 32 | `cfd02af43158da469da0d0cb3017febd87f5d260ef6a1fb751522f24cddc329b` |
| Spider ON indexed view | `0x004b4d9c` | `i × 17` / 68 | `fa098e27430a4734e5272ee4a9d8602d54d54ffd619fb9bbe2dcc404818f2bf9` |
| Spider OFF indexed view | `0x004b4ddc` | `i × 17` / 68 | `a955952c1f9a018338d032c6bcd9f0d7bf1964f94e5addfec6e25a49ebdba126` |
| Spider swing offsets | `0x004b4e20` | `i × 2` / 8 | `cbde49dbb8a415bcbad65b3920c19a39d56f4bf85354994f905272b941739d95` |
| Temple path pointers | `0x004b4f08` | `I × 4` / 16 | `cb6192ea2a3b5398291b62371d038533e8bf52ee576f83291a500493266e84f3` |
| Temple walk limits | `0x004b4f18` | `b × 4` / 4 | `37aa68d56311c49c3c474ea7eaebc4c28c5621c3036789b24cc04615b55f9fc2` |
| Temple end limits | `0x004b4f1c` | `b × 4` / 4 | `f19d88f5a76f7523e9650c9a769b37d23d3390f61550d7a8d73a77dbff9445c9` |
| Tower car geometry | `0x004b7798` | `i × 20` / 80 | `cf22df728f067866215b823db8ddd73b57060dc63912d0497c356cea3514a202` |
| Tower world seat offsets | `0x004b77e8` | `i × 16` / 64 | `a6c55ce3180f905a8bf53d9ae62fed1ad842b77da7146df3d2f57c290949d118` |
| Tower animation references | `0x004b7758` | `I × 16` / 64 | `3cd57b740495d6bed638d98fb69917e1e6aef9d5dc1d847449479ab1646311b8` |
| Tower animation programs | `0x004b7628` | `I × 76` / 304 | `bb03e830bf99dd9e3db948100c42f4ec3544ef3a94bae2dff90e31369e5175f7` |
| Restaurant1 waypoints | `0x004b66f4` | `i × 90` / 360 | `85a354abccdf071b9caf0e6b444fd2c5645eb424c7fadb133e5931148d4d5fc1` |
| Waiter X | `0x004b685c` | `i × 33` / 132 | `a5dfd63c95a725f58e9ce2a3a305e21785a05fe1c48eef4cae63825f71091da8` |
| Waiter Y plus observed pad | `0x004b68e0` | `i × 34` / 136 | `51ffbebb8a567cc6b01cdc1b124b45af3dd0ace346f2763d03d3842b352da81e` |
| Cafe positions | `0x004b6990` | `i × 42` / 168 | `c6ad7d3e59455281f184425f34674a8a8ef0c3b5c38837768220999984c181b4` |
| Cafe walk indexed view | `0x004b6a34` | `i × 65` / 260 | `3570c992e6a7f53a980eaf9d1031f95ce9f67034ef6a3c6ca5fb118c400bbaa5` |
| Cafe chair offsets | `0x004b6b38` | `i × 44` / 176 | `1fe4cf4a4d63e98dde60bd3118ae6232cb281d6ddefcb34426bcc1bd752f85c7` |
| Cafe directions | `0x004b6be8` | `i × 32` / 128 | `20cca3d14d048eae4f9f1998d8b8ba71798415d27de577bbbbe593ea5baa55f8` |
| Cafe depth storage view | `0x004b6c68` | `i × 132` / 528 | `b6f82e249b6f5b8a02248873056ef7c78c176f59f20d1d6a6a4f2860b78750a1` |
| Boating physical queue offsets | `0x004b5290` | `i × 10` / 40 | `b4df82ead7830e1323794d74a904a220605cd28dc362a66733d8badb26c35ef3` |
| Copters sprite pointers | `0x004b4170` | `I × 10` / 40 | `08934deb9894a6521759fe443ed20716ef0536ec48b1ac42ec26956530af9253` |
| Copters FX rows | `0x004b4140` | `I × 12` / 48 | `d5e7747a64cbca419547fa6f66ea72fb3f95f1caa77da4caa18587430b028b15` |
| Copters polyline A points | `0x004b4198` | `i × 12` / 48 | `fdb2ed21d27308adb97177c371f53a5ebb2ba6a5746607aeca293f14e5fc32ad` |
| Copters polyline A descriptor | `0x004b41c8` | `I × 2` / 8 | `6e60d62938cbe6ecbad64ab25726fa7338bb8507ae8e8d2326435bee94e37b9f` |
| Copters polyline B points | `0x004b41d0` | `i × 14` / 56 | `eb879f787f1ba191b9d8f7e8814d31fe4bf7a8ba54c12bb7ca13b4602325c425` |
| Copters polyline B descriptor | `0x004b4208` | `I × 2` / 8 | `b4e16e0ce8f0a00334b24f0801c8c7a6f289795cc723b5130a2d4fb2f7942593` |
| Copters polyline C points | `0x004b4210` | `i × 12` / 48 | `609e1474405af8accec37e0ac28b0229a07e11f90e12e7c7d0691248a87b8116` |
| Copters polyline C descriptor | `0x004b4240` | `I × 2` / 8 | `e5aa69adb9a25d2c59dbe531f9865287e24edf0b219eede18f2165d9dd92d3f1` |
| Copters polyline D points | `0x004b4248` | `i × 10` / 40 | `62f5179c43d6a118350c0578f0cc6913555c85f1a287eca06ba10eef8200f713` |
| Copters polyline D descriptor | `0x004b4270` | `I × 2` / 8 | `9dcf01e49a43e8556ab3798e23aabb6db627cd0260b9983c8a60892c84b2187a` |
| Copters polyline E points | `0x004b4278` | `i × 8` / 32 | `a8a63b32b3859df727a69d50ab7e75e287508bf969a6320d0c312e7024fb347f` |
| Copters polyline E descriptor | `0x004b4298` | `I × 2` / 8 | `56f110c3082a4e87024af27ea46239e684b8fd06cd9a3f610a5e86041827b514` |
| Copters matrix selectors | `0x004b42a0` | `i × 12` / 48 | `2a5635b7fe6620342095a70bc9faa1bc5729f3b693240ff7978d84936d1ea689` |
| Copter pose jump table | `0x00404840` | `I × 5` / 20 | `122d94df31082cd6ab82108c654ed42ee54f09ca9b7e6dc3efe5ed743b146e61` |

## Catapult, Safari, Spider and Temple Slide

Catapult layer IDs and landing offsets follow seat0..3. FX names resolve to `Dunk01.wav`, `Dunk02.wav`, `Dunk03.wav`, `dunker01.wav`; both remaining DWORDs in each initial FX record are zero. The FX sample-column symbol `0x004b40d0` is row base plus8. [original executable](../../../legoland/original/legoland.exe), [catapult.c](../../LEGOLAND/catapult.c)

**Catapult layers**, `0x004b40a4`, `i` × 4. [original executable](../../../legoland/original/legoland.exe)

`5, 2, 3, 4`

**Catapult landing Y**, `0x004b40b4`, `i` × 4. [original executable](../../../legoland/original/legoland.exe)

`656, 448, -32, -496`

**Safari ON limits**, `0x004b4cc4`, `i` × 8. [original executable](../../../legoland/original/legoland.exe)

`66, 66, 47, 47, 80, 80, 48, 48`

**Safari OFF limits**, `0x004b4ce4`, `i` × 8. [original executable](../../../legoland/original/legoland.exe)

`63, 63, 63, 63, 48, 48, 32, 32`

**Spider ON indexed view**, `0x004b4d9c`, `i` × 17. [original executable](../../../legoland/original/legoland.exe)

`0, 32, 28, 20, 24, 12, 16, 20, 12, 20, 24, 28, 33, 36, 33, 36, 32`

**Spider OFF indexed view**, `0x004b4ddc`, `i` × 17. [original executable](../../../legoland/original/legoland.exe)

`32, 20, 24, 32, 24, 32, 32, 24, 32, 20, 24, 16, 12, 8, 12, 18, 20`

**Spider swing offsets**, `0x004b4e20`, `i` × 2. [original executable](../../../legoland/original/legoland.exe)

`-21, -11`

Spider valid seat IDs1..16 directly index these views. ON[16] and OFF[0] are the same DWORD at `0x004b4ddc`, equal32. The 16 DWORDs preceding OFF are not a complete indexed ON view by themselves. OFF[16] is20; the following two words are the separately used swing offsets −21 and −11. This corrects the header’s eight-element declaration without hiding the original address overlap. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c)

Temple path names at `0x004b4f08` resolve in lane order to `manbox01`, `manbox02`, `manbox03`, `manbox04`. Walk and end thresholds are four **signed bytes each**, not DWORDs. Their exact comparisons remain those in the rider consumer; normal allocation offers lanes0 and3. [original executable](../../../legoland/original/legoland.exe), [joust.c](../../LEGOLAND/joust.c)

**Temple walk limits**, `0x004b4f18`, `b` × 4. [original executable](../../../legoland/original/legoland.exe)

`64, 69, 63, 60`

**Temple end limits**, `0x004b4f1c`, `b` × 4. [original executable](../../../legoland/original/legoland.exe)

`81, 83, 83, 80`

## Tower geometry and animation programs

Car rows contain `(seat0_x,seat0_y,seat1_x,seat1_y,direction)` in screen units. `0x004b77a8` is the direction column at row base+16, with20-byte stride. The first four fields are proven by `TowerPlaceRiders` at `0x0043b8f4..0x0043b933`, and facing by its following direction call. World-seat rows are whole map-square offsets selected independently during boarding. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [bswater3.c](../../LEGOLAND/bswater3.c)

**Tower car geometry**, `0x004b7798`, `i` × 20. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `16, 26, 41, 39, 5` |
| 1 | `16, 31, 40, 20, 7` |
| 2 | `27, 20, 48, 31, 1` |
| 3 | `24, 40, 48, 28, 3` |

**Tower world seat offsets**, `0x004b77e8`, `i` × 16. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, 2` |
| 1 | `1, 2` |
| 2 | `-1, 1` |
| 3 | `-1, 0` |
| 4 | `-1, -1` |
| 5 | `1, -1` |
| 6 | `3, 1` |
| 7 | `3, 0` |

Animation references at `0x004b7758` are `{stop_part,pointer}`. In seat order they are `(4,A),(3,A),(4,B),(3,B),(2,B),(1,B),(2,A),(1,A)`, where A=`0x004b76b8`, B=`0x004b7750`. The shifted `0x004b775c` symbol names the pointer column. Both programs have four parts. Each16-byte step is `(whole_dx,whole_dy,fraction_dx,fraction_dy)`; `Anim3D_OffsetAt` accumulates `(whole<<8)+fraction` through the selected part/frame. [original executable](../../../legoland/original/legoland.exe), [ridemisc2.c](../../LEGOLAND/ridemisc2.c), [bswater3.c](../../LEGOLAND/bswater3.c), [bswater2.c](../../LEGOLAND/bswater2.c)

| Program/part | Part VA | Count / step VA | Exact step rows |
| --- | --- | --- | --- |
| A/0 | `0x004b7688` | 2 / `0x004b7628` | `0,1,208,0`; `0,2,0,0` |
| A/1 | `0x004b7690` | 1 / `0x004b7648` | `0,1,0,0` |
| A/2 | `0x004b7698` | 2 / `0x004b7658` | `-1,1,128,128`; `-1,0,0,128` |
| A/3 | `0x004b76a0` | 1 / `0x004b7678` | `-1,0,0,0` |
| B/0 | `0x004b7720` | 2 / `0x004b76c0` | `0,1,0,240`; `-1,0,0,0` |
| B/1 | `0x004b7728` | 1 / `0x004b76e0` | `-1,0,0,0` |
| B/2 | `0x004b7730` | 2 / `0x004b76f0` | `0,1,0,0`; `0,0,0,128` |
| B/3 | `0x004b7738` | 1 / `0x004b7710` | `0,1,0,0` |

## Restaurant and Cafe tables

Restaurant1 rows are indexed `seat*5+phase`, with fields `(dx,dy,ox,oy,turn,band)` and target `((placement+base+delta)<<8)+offset`. All15 rows are present below; the source header’s first five are only seat0. [original executable](../../../legoland/original/legoland.exe), [ridemisc2.c](../../LEGOLAND/ridemisc2.c)

**Restaurant1 waypoints**, `0x004b66f4`, `i` × 90. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `-2, 0, 128, 128, 1, 3` |
| 1 | `-2, 1, 128, 150, 1, 5` |
| 2 | `-2, 1, -80, 150, 0, 4` |
| 3 | `-2, 1, 128, 150, 0, 4` |
| 4 | `-2, 0, 128, 128, 1, 5` |
| 5 | `-4, 2, 0, 140, 1, 3` |
| 6 | `-3, 2, 128, 140, 1, 5` |
| 7 | `-3, 2, 0, -60, 0, 4` |
| 8 | `-3, 3, 0, 0, 0, 4` |
| 9 | `-4, 0, 0, 0, 1, 3` |
| 10 | `-2, 0, 128, 128, 1, 3` |
| 11 | `-2, -1, 128, 100, 1, 2` |
| 12 | `-2, -1, -90, 100, 0, 1` |
| 13 | `-2, -1, 128, 100, 0, 1` |
| 14 | `-2, 0, 128, 128, 1, 2` |

Waiter X occupies33 DWORDs. The observed Y view includes its nominal33 DWORDs plus the following zero DWORD, stopping immediately before the FX record at `0x004b6968`. Thus the original forward loop’s step33 reads X[33]=Y[0]=0 and Y[33]=0. The next incoming-customer pass can read X[34]=Y[1]=9, applying −72 to each world axis when phase2 remains active. These values establish the actual effects; whether the author intended adjacent-table sharing is unnecessary to execute the contract. Returning phase3 reverses the frame index without undoing the accumulated coordinates. [original executable](../../../legoland/original/legoland.exe), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc3.c](../../LEGOLAND/ridemisc3.c)

**Waiter X**, `0x004b685c`, `i` × 33. [original executable](../../../legoland/original/legoland.exe)

`0, 9, 10, 10, 9, 10, 9, 10, 10, 10, 9, 10, 9, 10, 10, 10, 9, 10, 10, 9, 10, 9, 10, 10, 9, 10, 10, 10, 9, 10, 10, 9, 0`

**Waiter Y plus observed pad**, `0x004b68e0`, `i` × 34. [original executable](../../../legoland/original/legoland.exe)

`0, 9, 9, 10, 9, 9, 10, 9, 9, 10, 9, 9, 9, 10, 9, 9, 10, 9, 9, 10, 9, 9, 10, 9, 9, 9, 10, 9, 9, 10, 9, 9, 0, 0`

Cafe positions0..4 are approach points and5..20 are chair-pair positions in24.8 units. Walk entry0 equals608 and physically aliases position20.y; each following group of four belongs to `(seat>>1)`, so the listing below numbers chair pairs0..15. Directions map all32 seat IDs to one of11 `(standing_x,standing_y,sitting_x,sitting_y)` offset rows. [original executable](../../../legoland/original/legoland.exe), [ridecb4.c](../../LEGOLAND/ridecb4.c)

**Cafe positions**, `0x004b6990`, `i` × 42. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `640, 128` |
| 1 | `768, 640` |
| 2 | `768, -640` |
| 3 | `-384, -640` |
| 4 | `-384, 640` |
| 5 | `704, -256` |
| 6 | `1280, -832` |
| 7 | `64, -768` |
| 8 | `640, -1344` |
| 9 | `0, -704` |
| 10 | `-576, -1280` |
| 11 | `-512, -192` |
| 12 | `-1088, -768` |
| 13 | `-512, -64` |
| 14 | `-1088, 512` |
| 15 | `0, 448` |
| 16 | `-576, 1088` |
| 17 | `64, 512` |
| 18 | `640, 1088` |
| 19 | `704, 0` |
| 20 | `1312, 608` |
| Cafe chair pair | Walk stages1..4 |
| --- | --- |
| 0 | `0,-1,-1,5` |
| 1 | `0,-1,2,6` |
| 2 | `0,-1,2,7` |
| 3 | `0,-1,2,8` |
| 4 | `0,-1,2,9` |
| 5 | `0,2,3,10` |
| 6 | `0,2,3,11` |
| 7 | `0,2,3,12` |
| 8 | `0,1,4,13` |
| 9 | `0,1,4,14` |
| 10 | `0,-1,1,15` |
| 11 | `0,1,4,16` |
| 12 | `0,-1,1,17` |
| 13 | `0,-1,1,18` |
| 14 | `0,-1,-1,19` |
| 15 | `0,-1,1,20` |

**Cafe chair offsets**, `0x004b6b38`, `i` × 44. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -304, 128, -304` |
| 1 | `304, 0, 304, 160` |
| 2 | `0, 304, -128, 304` |
| 3 | `-304, 0, -304, -128` |
| 4 | `0, 304, 128, 304` |
| 5 | `-304, 0, -304, 128` |
| 6 | `0, -240, -128, -240` |
| 7 | `296, 0, 296, -96` |
| 8 | `-272, 0, -272, -96` |
| 9 | `0, 240, -128, 240` |
| 10 | `0, -304, -128, -304` |

**Cafe directions**, `0x004b6be8`, `i` × 32. [original executable](../../../legoland/original/legoland.exe)

`0, 7, 2, 5, 0, 7, 2, 5, 3, 6, 1, 4, 8, 10, 1, 4, 2, 5, 0, 7, 2, 5, 0, 7, 1, 4, 8, 6, 1, 4, 3, 6`

The Cafe depth consumer base `0x004b6d58` points60 DWORDs into the132-DWORD storage view beginning `0x004b6c68`. For `d=11*dx-dy`, its address is storage index `60+d`. The12 rows below each contain11 words; equivalently label rows `dx=-5..6` and columns `dy=5..-5`. This is a representation of the observed storage, not a newly invented footprint check. The consumer remains unbounded: indices outside the view read adjacent data, and the next byte at `0x004b6e78` starts `Rest2LiftStop.wav`. [original executable](../../../legoland/original/legoland.exe), [ridecb4.c](../../LEGOLAND/ridecb4.c)

**Cafe depth storage view**, `0x004b6c68`, `i` × 132. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1` |
| 1 | `19, 5, 5, 5, 4, 3, 3, 2, 1, 1, 1` |
| 2 | `19, 5, 5, 5, 5, 3, 3, 2, 1, 1, 1` |
| 3 | `19, 13, 12, 11, 11, 10, 7, 7, 6, 6, 1` |
| 4 | `19, 13, 13, 11, 11, 10, 7, 7, 7, 7, 1` |
| 5 | `19, 14, 14, 13, 13, 10, 10, 8, 8, 7, 1` |
| 6 | `19, 15, 15, 13, 13, 10, 10, 8, 8, 7, 1` |
| 7 | `19, 19, 19, 13, 13, 15, 15, 8, 8, 7, 1` |
| 8 | `19, 19, 19, 19, 18, 17, 17, 16, 8, 7, 1` |
| 9 | `19, 19, 19, 19, 19, 17, 17, 16, 15, 15, 1` |
| 10 | `19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 1` |
| 11 | `19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 1` |

Boating queue lookup uses `0x004b52b0[-slot]` with pair stride8, so slots0..4 produce `(-768,560),(-512,560),(-256,544),(-256,272),(-256,0)`. The physical ascending-address rows are recorded below to preserve the negative-index contract. [original executable](../../../legoland/original/legoland.exe), [ridecb5.c](../../LEGOLAND/ridecb5.c)

**Boating physical queue offsets**, `0x004b5290`, `i` × 10. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `-256, 0` |
| 1 | `-256, 272` |
| 2 | `-256, 544` |
| 3 | `-512, 560` |
| 4 | `-768, 560` |

## Copters resources and pose data

Sprite pointers at `0x004b4170` resolve in index order to `mcop_gs.lls`, `mcop_b2s.lls`, `mcop_rs.lls`, `mcop_b1s.lls`, `mcop_ys.lls`, `mcop_b1m.lls`, `mcop_gm.lls`, `mcop_rm.lls`, `mcop_ym.lls`, `mcop_b2m.lls`. Four FX rows at `0x004b4140` resolve to the takeoff, flying, landing and breakdown names printed by the extractor; the initial flags/sample DWORDs are all zero. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c)

| FX slot | Name |
| --- | --- |
| 0 | `Helicopter takeoff.wav` |
| 1 | `Helicopter Flying.wav` |
| 2 | `Helicopter Landing.wav` |
| 3 | `Helicopter breakdown.wav` |

The five integer polylines are descriptors `{point_count,point_pointer}` with signed whole-square deltas. The loader assigns A/B/C/D/E to global path slots0/4/1/2/3 respectively. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

**Copters polyline A points**, `0x004b4198`, `i` × 12. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -1` |
| 1 | `1, 0` |
| 2 | `1, 0` |
| 3 | `1, 0` |
| 4 | `-1, -1` |
| 5 | `-1, -1` |

**Copters polyline B points**, `0x004b41d0`, `i` × 14. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -1` |
| 1 | `-1, 0` |
| 2 | `0, -1` |
| 3 | `0, -4` |
| 4 | `-3, -1` |
| 5 | `1, 0` |
| 6 | `1, 0` |

**Copters polyline C points**, `0x004b4210`, `i` × 12. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -1` |
| 1 | `-1, -1` |
| 2 | `2, -4` |
| 3 | `2, -2` |
| 4 | `-2, 0` |
| 5 | `0, -1` |

**Copters polyline D points**, `0x004b4248`, `i` × 10. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -1` |
| 1 | `4, -1` |
| 2 | `0, -1` |
| 3 | `-1, -1` |
| 4 | `-1, -2` |

**Copters polyline E points**, `0x004b4278`, `i` × 8. [original executable](../../../legoland/original/legoland.exe)

| Row | Values |
| --- | --- |
| 0 | `0, -1` |
| 1 | `-1, -1` |
| 2 | `0, -1` |
| 3 | `0, -1` |

### Selected original asset members

The full directory paths below match the source loader requests; offsets are absolute bytes within the archive. POS files start with `(frames_per_stream,stream_count)` and contain contiguous48-byte records per stream. This corrects reversed field labels in some local PosTable views. Copters has32 frames ×5 streams; Earth has16 frames ×1 stream. The older `3DData/eslide.pos` file is not the Earth Slide loader’s requested asset. [original Legoland.res](../../../legoland/gamedata/disc/Legoland.res), [loaders.c](../../LEGOLAND/loaders.c), [screencb3.c](../../LEGOLAND/screencb3.c), [mechrides.c](../../LEGOLAND/mechrides.c)

| Full archive path | Offset | Bytes | SHA-256 |
| --- | --- | --- | --- |
| `3DData/copters.pos` | 4145344 | 7688 | `50f717bccf4f22b2f5f06b3cacf568acf9faae7f29bb8f1724dc1236a405dc2b` |
| `3DData/earth.pos` | 4143704 | 776 | `08092f3c9ef3bae53b99df362106671db2871290dff1bfa97113aae350cb29b1` |
| `3DData/earthslide.rin` | 4145256 | 78 | `3044f6298405949d55f9c368fd24bb10cd028e42fa6a885fb80c18892fd54900` |
| `CompSprite/COPTERS SPRITE.CSP` | 4532132 | 296 | `db0ab647bf727d2031ea4715e9b5ab69ae7c34a0a4353948b9c26815ea01c54f` |
| `Objdesc/CAROUSEL.ODF` | 4769656 | 467 | `3b4c3c011ca6494d3bfcb3d7c134eae9f93bfcc44eeed606429faf417497bca7` |

The Copters POS record’s first12 bytes are retained without inventing a three-coordinate interpretation. In all160 original records, word+8 is integer4; scalar+0 is an unused float in the placement consumer; scalar+4 supplies the vertical displacement. `CopterPlaceRider` reads it with `FLD [entry+4]` at `0x00404701`, calls the x87 integer conversion at `0x00404705`, adds the per-copter bias, then applies the view-scale adjustment. It does not use word+8 as a coordinate. [original executable](../../../legoland/original/legoland.exe), [original Legoland.res](../../../legoland/gamedata/disc/Legoland.res), [loaders.c](../../LEGOLAND/loaders.c)

Copter index0..4 selects `(model_stream,y_bias)` as `(3,215),(0,235),(4,225),(1,230),(2,230)`. The switch table at `0x00404840` points to `0x004046a8,0x00404681,0x004046b6,0x0040468c,0x0040469a`. With layerB offset adjusted for view scale, screen x is placement-origin x + layer x + arithmetic-half sprite width; screen y is origin y + layer y + adjusted `(RoundX87(entry.float+4)+bias)`. Finally `AdjustBlokePosition` subtracts `(75,77)` before `SetPersonPosition`. [original executable](../../../legoland/original/legoland.exe), [world.md](world.md), [goldrush2.c](../../LEGOLAND/goldrush2.c)

Before copying the POS orientation into Person+0x58, instructions `0x00404780..0x004047b0` overwrite the cached record’s last vector: `F[0x24]=F[0x14]*F[0x1c]-F[0x20]*F[0x10]`, `F[0x28]=F[0x20]*F[0x0c]-F[0x18]*F[0x14]`, `F[0x2c]=F[0x18]*F[0x10]-F[0x0c]*F[0x1c]`. With axis0..2 and component0..2, `M[component][axis]=RoundX87(F[3+3*axis+perm[component]]*65536)*component_sign[component]*axis_sign[axis]`; here F in the matrix expression denotes a float-indexed record, `perm={0,2,1}`, component signs `{1,-1,-1}`, axis signs `{-1,1,1}`. The loop stores each component at a12-byte destination stride. Both rounding sites use the current x87 rounding mode, not a C truncating cast. [original executable](../../../legoland/original/legoland.exe), [loaders.c](../../LEGOLAND/loaders.c)

Earth Slide loads `earthslide.rin`: one object, NUL-terminated name `Box97`,16 frames and16 zero draw IDs. Creation sets RIN origin `(−158,−6)`, zero riders, and POS fields+14/+18/+1c/+20 to `83,196,3,65`. The draw hides RIN frames11..14 and draws frames≤10 or≥15. The complete selected asset is776 bytes, not the unused eight-byte `eslide.pos` file. [original Legoland.res](../../../legoland/gamedata/disc/Legoland.res), [screencb3.c](../../LEGOLAND/screencb3.c)

## Carousel allocation and seat capacity

The selected `Objdesc/CAROUSEL.ODF` archive member above has a120-byte fixed prefix. `LLIDB_LoadODFData` copies bytes4..119 to ObjDef+4..119, preserving offsets; the signed word at file/ObjDef+0x2e is10. `Carousel_Tick` passes that capacity as a signed byte to `Carousel_PickSeat`. Therefore normal indices0..9 access record+0x20..+0x29, and stored rider IDs1..10 later release the same bytes through `seat[id-1]`. The0x2c-byte allocator zeroes the entire record. The four-byte array in short0x24 caller views is incomplete: +24..+29 are six additional live seats, while +2a/+2b are reserved/padding bytes with no field access in the reviewed valid-index consumers. Modified oversized capacities remain unchecked and can reach that padding or beyond. [Original archive](../../../legoland/gamedata/disc/Legoland.res), [llidb_odf.c](../../LEGOLAND/llidb_odf.c), [ridecb3.c](../../LEGOLAND/ridecb3.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [bswater.c](../../LEGOLAND/bswater.c)

The original picker is `[0x0042cd20,0x0042cd61)`,25 instructions/65 bytes. It sign-extends capacity at0x0042cd21, divides random by that capacity at0x0042cd2c, reads `[record+index+0x20]` at0x0042cd32/0x0042cd41, wraps index against capacity and writes1 at0x0042cd4d. It stores ID=index+1 at Bloke+0x36. Capacity0 divides by zero; full occupancy loops; invalid capacity is not clamped. Its exact hash is in the function manifest, and the same read-only checker verifies the ODF count and instruction boundaries. [Original executable](../../../legoland/original/legoland.exe), [ridecb3.c](../../LEGOLAND/ridecb3.c)

## Bounded recovered functions

These24 bounded bodies contain1,517 decoded x86 instructions and4,489 bytes. The first18 entries now have corresponding bodies in [ridemachine.c](../../LEGOLAND/ridemachine.c); their address set and1,077-instruction/3,117-byte total agree with [machine lane](../lanes/codex-e.md). The next five are their immediate animation/pose/reset callees recovered from original bytes; the final entry independently checks the existing Carousel seat picker. Every final instruction is RET, every immediate intra-function jump resolves to an instruction start within its range, and the five Copter switch targets also resolve inside the pose function. The extractor repeats those checks and can print every instruction. [original executable](../../../legoland/original/legoland.exe), [Scope E brief](../SCOPE_CODEX_E.md), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

| Function | VA → exclusive end | Instructions / bytes | SHA-256 |
| --- | --- | --- | --- |
| `Copters_StepMachine` | `0x00404a90 → 0x00404bb8` | 115 / 296 | `c4937d6c8dec9ca115ab828022dd4970d86fe46c13ef37188a57f514de0b17aa` |
| `SpinningBarrels_StepMachine` | `0x0043c7f0 → 0x0043c92a` | 113 / 314 | `6edc81f512645077953b0791f0c3fa956f97e046d36a98461f9d762818e2c91f` |
| `PlaneRide_StepMachine` | `0x0043e2b0 → 0x0043e3e5` | 112 / 309 | `876567972feb58a2f6840e77e1f6b424b5146ea03b6aa3b9e859e1f040ce9539` |
| `SafariRide_StepMachine` | `0x004150c0 → 0x004151f3` | 111 / 307 | `0757277ff1af74d18d977ee9a9dde9881a1478a3cf94fde1122495005d36a805` |
| `SpiderRide_StepMachine` | `0x004161f0 → 0x00416310` | 103 / 288 | `72993a85af4d7ea0c313859058379e67f516a792f6224ffdda05d4aa16967c48` |
| `SpaceTower_StepMachine` | `0x0043b990 → 0x0043ba9e` | 94 / 270 | `9161963f40af6a506963ff2577b6f16b27319228ef42f1d7b5d91c214c8aafb7` |
| `EarthSlide_StepMachine` | `0x0042d560 → 0x0042d5ed` | 53 / 141 | `dd3ef7bb439e5874edc07e3d89a4620b6e3a058dd641da2a1802547763d16cfb` |
| `Copters_InitRecord` | `0x00403e90 → 0x0040403a` | 118 / 426 | `b2d4093a0b671d996aa23e91f10259d78c92fe1b797210541f2c4616bec3652e` |
| `SpaceTower_StopRide` | `0x0043aac0 → 0x0043ab6f` | 56 / 175 | `891f9dc7640e7783d957ee34e980ded0db8902a756759aaa713b1981442b2318` |
| `PlaneRide_StopRide` | `0x0043d9f0 → 0x0043da5c` | 37 / 108 | `cc243baf9d3179ca9ded82cfea28198f14c28cb6807606a4b8dad19da301fe97` |
| `SafariRide_StopRide` | `0x00414b10 → 0x00414b7b` | 36 / 107 | `0c40b9c266c73a2ca41042aaedf70d4464039c060f08e0fb084e314be00cf280` |
| `SpiderRide_StopRide` | `0x00415a90 → 0x00415ad6` | 26 / 70 | `66186cf09d73bbd981a50e51385f4f444f9ac5775dc567e7866aae9dae7264e1` |
| `SpinningBarrels_StopRide` | `0x0043c2f0 → 0x0043c313` | 11 / 35 | `e3e9db001f7f541f4aa4509ebfbd644a5e18da708aca29ef944ab595d8473cc1` |
| `SpaceTower_PickSeat` | `0x0043acb0 → 0x0043acf6` | 22 / 70 | `8f5cbc051799c7d398bb530ba1ce75f9b96420d4d81d8226e58dc44652c6774e` |
| `SpaceTower_StartSound` | `0x0043aa10 → 0x0043aa49` | 18 / 57 | `baca11e73fd07dc9f58ee65982940f1e9cd9520ab35273f1d0ae6d058f1f8647` |
| `SpiderRide_StartSound` | `0x004159e0 → 0x00415a19` | 18 / 57 | `00d75a922e194982c41a580279ba36bfbb50997c4c8aa7b534b91be8c5751ecf` |
| `GoldRush_HasFreePan` | `0x00406e90 → 0x00406ebd` | 18 / 45 | `db8251172fd4276828e62e44b00cbb5f5cd7be764ad31facad4aa576f3ada6b7` |
| `EarthSlide_AppendQueue` | `0x0042ce90 → 0x0042ceba` | 16 / 42 | `242d3e1ab0ad1f1d1d534e6f670bad09529cfcde1aaf4e2c0fda8260f1d2b1bf` |
| `TowerCarStep` | `0x0043a940 → 0x0043a9ac` | 37 / 108 | `6d5f42565b03be1854b4645a059c884b085c8d77b01d908b778abb9a660f28a4` |
| `TowerPlaceRiders` | `0x0043b810 → 0x0043b990` | 116 / 384 | `1ebc9c5c6f39094f1419207994fd036d12b0feed1379f96629fe5c249eaabe39` |
| `CopterAdvanceFrame` | `0x00404860 → 0x0040489b` | 23 / 59 | `4ee86601f5d4ebaad7449e898b7560b2da260bc567419f50cfe1e09040c37aab` |
| `CoptersReset` | `0x004049a0 → 0x00404a84` | 71 / 228 | `8097b33d67dbb5ff84849c6917bf689f63f86e71035768bb6a224fbe2fd4526d` |
| `CopterPlaceRider` | `0x00404630 → 0x00404840` | 168 / 528 | `faf978eeeba63a5a232b12b6d47737589d0ae1169ad4a0c68e55228d658f2f3c` |
| `Carousel_PickSeat` | `0x0042cd20 → 0x0042cd61` | 25 / 65 | `b0a9305220a58cb58b4390f5ff47f8dfc2925d96c03a8006cfbe96f61c550ce8` |

### Safari, Spider, Plane and Barrels machine steps

The common running path increments its half counter, releases and stops immediately if the revolution count was already0, otherwise advances the frame at the threshold and decrements revolutions on frame wrap. Release calls `GetAllBlokesOffRide`, which returns1 immediately. When not running, filling flag0x4000 plus `seated==joined` clears filling and starts the ride, returning before pose updates. An idle nonempty machine with a timer already0 closes admission and latches filling; otherwise it decrements the timer. Spider also decrements the timer on the closure tick, yielding−1. The other normal paths update pose1 riders of the matching placement on their RUN BNV path then publish the frame word through the z-sprite object+08 holder (two pointer dereferences). [ridemachine.c](../../LEGOLAND/ridemachine.c). Safari additionally refreshes nonriding customers. [original executable](../../../legoland/original/legoland.exe), [rides.c](../../LEGOLAND/rides.c), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

| Machine | Flag/revolutions/half/frame fields | Cadence and wrap | RUN BNV / z-sprite globals; near/far depth |
| --- | --- | --- | --- |
| Safari | `+14 u32/+18 i32/+1c i32/+0c i32` | 2 ticks;48 frames | `0x4cbef4/0x82c66c`; `−1617787.0,−1618006.0` |
| Spider | `+08 u32/+0c i8/+10 i32/+04 i8` | 2 ticks;32 frames | `0x4cbf10/0x82c668`; `−1617787.75,−1618096.5` |
| Plane | `+08 u32/+0c i8/+10 i32/+04 i8` | 2 ticks;97 frames; cosmetic+05 wraps24 every tick | `0x62fe90/0x81cae0`; `−1617706.75,−1617948.625` |
| Barrels | `+0c u32/+10 i8/+14 i32/+08 i8` | 3 ticks;64 frames; cosmetic+20 wraps32 every tick | `0x62fde8/0x62fe00`; `−1617922.25,−1618065.75` |

Safari/Spider/Plane write `%02d` into mutable `manbox??` buffers at byte6; Barrels writes at byte8 of `BoxBloke??`. Safari RUN name uses `manbox%02d` with seat+1, whereas its boarding consumer uses seat/2+1. Spider uses its one-based seat directly, and Barrels uses `BoxBloke%02d`. The older ridemisc4 i32 Plane revolution view is wider than the byte consumed by the original step/reset functions; the new [ridemachine.c](../../LEGOLAND/ridemachine.c) correctly uses signed bytes for Plane/Spider/Barrels. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c)

All four stop helpers clear running/filling flags0x4001, joined, seated, main frame and half counter. Safari reseeds revolutions with `rand()%2+3`; Spider uses `boolean(rand()%2)+3`; Plane uses `boolean(rand()%2)+1`; Barrels sets3. Safari and Plane fade with−200; Spider calls its square fade helper; Barrels has no fade here. Spider does not clear the seat array. Plane/Barrels do not clear the independent cosmetic frame. Rider count stays available for departure. [original executable](../../../legoland/original/legoland.exe), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

Exact witnesses: Barrels `0x0043c852` compares its half counter with2 and branches while≤2; Spider’s zero-timer closure falls through to `DEC [esi+0x18]` at `0x0041629e`; the four reset extents and all immediate frame/revolution comparisons are included verbatim by `--disasm`. [original executable](../../../legoland/original/legoland.exe)

### Tower machine, car, rider pose and picker

Tower advances signed cosmetic bytes+ad then+ac every tick, wrapping at16. The new source calls its car rise-speed byte `revs`; `TowerCarStep` below proves it is added to height rather than used as a revolution countdown. [ridemachine.c](../../LEGOLAND/ridemachine.c). While running it steps the four cars and tests all four state DWORDs; all zero causes immediate release then stop. Otherwise it positions riders, refreshes nonriding customers, and returns if running. Filling compares seated+2 with joined+4 and dispatches; an idle nonempty machine whose timer+b0 is already0 closes admission and returns, otherwise decrements it. [original executable](../../../legoland/original/legoland.exe), [bswater3.c](../../LEGOLAND/bswater3.c), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

Each0x24-byte car has service bit1 at+0, height i32+8, state i32+c, gate i32+10, descent flag i32+14, two rider pointers+18/+1c, and signed rise-speed byte+20. The service gate is tested at `0x0043a944`. State2 ascends by signed speed until height>200, then clamps200 and starts descent; descent subtracts2 until height<0, then clamps0, clears descent and sets state0. Both endpoints use strict comparisons. State1 with gate0 changes it to−1 and returns; a subsequent nonzero gate changes state to2. [original executable](../../../legoland/original/legoland.exe), [bswater3.c](../../LEGOLAND/bswater3.c)

Tower stop clears0x4001, all four service bits and heights; it assigns four independently sampled `rand()%3+7` rise speeds, clears joined/seated, and fades−200. It does not explicitly clear the car states, seat occupancy, RiderNode pointers or remaining rider count. The seat picker starts at `rand()%8`, wraps over occupied bytes+a4, sets the first free byte, stores Bloke seat+36 and returns its zero-based index. Full occupancy has no exit. [original executable](../../../legoland/original/legoland.exe), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [bswater3.c](../../LEGOLAND/bswater3.c)

Tower pose clears all eight car RiderNode pointers, selects matching-placement class riders with riding flag0x80, derives `car=seat>>1`, `side=seat&1`, and stores each node into car+18/+1c. It obtains the placement’s screen origin, subtracts car height from the cached car y offset before view scaling, adds the separately scaled seat-pixel pair, applies `AdjustBlokePosition`, stores Person position and sets direction from the five-field row. Seat indices are unchecked. [original executable](../../../legoland/original/legoland.exe), [bswater3.c](../../LEGOLAND/bswater3.c), [goldrush2.c](../../LEGOLAND/goldrush2.c)

### Copters machine, initialization and reset

Copter record+c is a full i32 cadence countdown, not merely a u16 mode. It decrements each tick; negative resets it to2 and advances slots in order1,0,2,3,4. If running and all five active bits are clear it immediately releases riders and resets with sound enabled. The remaining paths position each of those five slots every tick and refresh nonriding customers; filling and idle-dwell logic use joined byte+10, seated byte+2 and timer i32+14. [original executable](../../../legoland/original/legoland.exe), [mechrides.c](../../LEGOLAND/mechrides.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

Each active slot increments its signed frame byte+4; reaching the signed cached frame count+1c resets frame0 and decrements signed stage+1d. Negative stage clears active bit1. Dispatch stage3 therefore runs four complete cycles, with animation advancement every third tick. Initialization chooses the pairs below, then for slots1,0,2,3,4 obtains the layerB sprite and LLS. The selected `cop_gm`, `cop_rm`, `cop_ym`, `cop_b1m` and `cop_b2m` LLS files each have32 frames; their original archive headers are recorded in [presentation-data.md](presentation-data.md). When both exist it caches LLS byte+10 in slot+1c and sets frame=count−1. Failed sprite/LLS lookup preserves those two initializer stores; the final reset still sets frame=count−1 from the retained cached count. It then calls reset with sound suppressed. [ridemachine.c](../../LEGOLAND/ridemachine.c). [original executable](../../../legoland/original/legoland.exe), [joust2.c](../../LEGOLAND/joust2.c), [ridemisc.c](../../LEGOLAND/ridemisc.c)

| Slot | layerA | layerB | spriteA | spriteB |
| --- | --- | --- | --- | --- |
| 0 | 2 | 1 | 0 | 6 |
| 1 | 10 | 3 | 2 | 7 |
| 2 | 4 | 11 | 4 | 8 |
| 3 | 5 | 6 | 3 | 5 |
| 4 | 8 | 7 | 1 | 9 |

Reset clears all five active bits and rider pointers, sets frames=count−1, clears cadence+c, joined+10 and seated+2, and clears flags0x4001. Argument1 suppresses sound; argument0 fades−200 and plays landing FX sample at `0x004b4160` with `(loop=0,arg=1)`. The sixth serialized slot is untouched by these five-slot loops. [original executable](../../../legoland/original/legoland.exe), [ridemisc.c](../../LEGOLAND/ridemisc.c), [ridesave.c](../../LEGOLAND/ridesave.c)

Copter pose passes an offset pair with uninitialised x to the view-adjust helper at `0x00404716`, then overwrites that x before use. It also mutates the cached POS matrix vector as described above. Preserve both observations when reasoning about renderer side effects. [original executable](../../../legoland/original/legoland.exe), [joust2.c](../../LEGOLAND/joust2.c)

### Earth Slide, Gold free-pan and sound helpers

Earth Slide tests running bit1 at+10. Running increments half+14, and only when it becomes>2 resets half0 and advances signed frame+b. Reaching RIN frame count+14 clears running, resets frame0, releases riders, sets record+4=1 and returns before pose refresh. Otherwise it calls `Put3DBlokesOnRide(class,record,frame,POS)` while running; both idle and running paths call `Put3DBlokesOnRide2` unless completion returned early. The selected RIN contains16 frames, so a fresh zeroed half advances one frame per three ticks. [original executable](../../../legoland/original/legoland.exe), [ridecb1.c](../../LEGOLAND/ridecb1.c), [screencb3.c](../../LEGOLAND/screencb3.c)

Earth queue append sets both head+1c and tail+20 only when both are null; otherwise it writes tail.next and moves tail. It does not clear the supplied node.next. The public join helper zeroes its newly allocated node, so ordinary enqueue supplies the required null next; inconsistent head/tail state can still dereference null. Gold’s free-pan predicate returns false for no record, true on the first zero of six DWORDs, and false if all six are nonzero. Tower/Spider sound starters use kind2 placement sources, FX sample addresses `0x004b7620`/`0x004b4d90`, loop1,arg1; the unused source word remains uninitialised. [original executable](../../../legoland/original/legoland.exe), [ridemisc4.c](../../LEGOLAND/ridemisc4.c), [ridetiny.c](../../LEGOLAND/ridetiny.c)

## Read-only reproduction

Save the following fence to a temporary file and run `python3 check_attractions.py /path/to/legoland.exe /path/to/Legoland.res`; add `--disasm` to print all bounded function instructions. Capstone is used only to decode instructions; the script never calls the executable, loads game DLLs, modifies assets or writes repository files. A wrong hash, a virtual-only PE read, a table mismatch, a malformed asset extent, a bad function boundary or an invalid jump target raises an exception. [original executable](../../../legoland/original/legoland.exe), [original Legoland.res](../../../legoland/gamedata/disc/Legoland.res)

```python

from pathlib import Path
import sys, struct, hashlib
import capstone
from capstone.x86_const import X86_OP_IMM
exe = Path(sys.argv[1]).read_bytes()
assert len(exe) == 802857
assert hashlib.sha256(exe).hexdigest() == "c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9"
def u(fmt, data, at=0):
    return struct.unpack_from("<" + fmt, data, at)
pe = u("I", exe, 60)[0]
assert exe[pe:pe+4] == b"PE\0\0"
count, optional_size = u("H", exe, pe+6)[0], u("H", exe, pe+20)[0]
opt = pe+24
assert u("H", exe, opt)[0] == 0x10b
base = u("I", exe, opt+28)[0]
sections = []
for i in range(count):
    at = opt+optional_size+i*40
    virtual_size, rva, size, offset = u("4I", exe, at+8)
    assert offset+size <= len(exe)
    sections.append((base+rva, size, offset))
def read(va, size):
    for start, raw_size, offset in sections:
        if start <= va and va+size <= start+raw_size:
            at = offset+va-start
            return exe[at:at+size]
    raise ValueError((hex(va), size, "not file-backed"))
def words(fmt, va, count):
    return u(str(count)+fmt, read(va, struct.calcsize("<"+fmt)*count))
def cstring(va):
    value = bytearray()
    while True:
        c = read(va+len(value), 1)[0]
        if c == 0:
            return value.decode("cp1252")
        value.append(c)
        assert len(value) < 4096
# name, original VA, scalar type, scalar count, SHA-256
tables = [

    ('Catapult layers', 0x4b40a4, 'i', 4, '3bd7a5422018f7829fea9b76532513d2998d68bdab66927ee9bb7ccbd3fd69c9'),

    ('Catapult landing Y', 0x4b40b4, 'i', 4, '7b1624186471d7452be26a65de6e27237b3f6e8d85c4efb620dc547d105deecb'),

    ('Catapult FX rows', 0x4b40c8, 'I', 12, 'fbbee2ee290740329019282d141d7169932d07eab985f82a55a1ebf965b5d3f7'),

    ('Safari ON limits', 0x4b4cc4, 'i', 8, '2831b391d126d8fb23a14e9d1d9386bafad64f36e0b7e4c2dc7527cb832eedec'),

    ('Safari OFF limits', 0x4b4ce4, 'i', 8, 'cfd02af43158da469da0d0cb3017febd87f5d260ef6a1fb751522f24cddc329b'),

    ('Spider ON indexed view', 0x4b4d9c, 'i', 17, 'fa098e27430a4734e5272ee4a9d8602d54d54ffd619fb9bbe2dcc404818f2bf9'),

    ('Spider OFF indexed view', 0x4b4ddc, 'i', 17, 'a955952c1f9a018338d032c6bcd9f0d7bf1964f94e5addfec6e25a49ebdba126'),

    ('Spider swing offsets', 0x4b4e20, 'i', 2, 'cbde49dbb8a415bcbad65b3920c19a39d56f4bf85354994f905272b941739d95'),

    ('Temple path pointers', 0x4b4f08, 'I', 4, 'cb6192ea2a3b5398291b62371d038533e8bf52ee576f83291a500493266e84f3'),

    ('Temple walk limits', 0x4b4f18, 'b', 4, '37aa68d56311c49c3c474ea7eaebc4c28c5621c3036789b24cc04615b55f9fc2'),

    ('Temple end limits', 0x4b4f1c, 'b', 4, 'f19d88f5a76f7523e9650c9a769b37d23d3390f61550d7a8d73a77dbff9445c9'),

    ('Tower car geometry', 0x4b7798, 'i', 20, 'cf22df728f067866215b823db8ddd73b57060dc63912d0497c356cea3514a202'),

    ('Tower world seat offsets', 0x4b77e8, 'i', 16, 'a6c55ce3180f905a8bf53d9ae62fed1ad842b77da7146df3d2f57c290949d118'),

    ('Tower animation references', 0x4b7758, 'I', 16, '3cd57b740495d6bed638d98fb69917e1e6aef9d5dc1d847449479ab1646311b8'),

    ('Tower animation programs', 0x4b7628, 'I', 76, 'bb03e830bf99dd9e3db948100c42f4ec3544ef3a94bae2dff90e31369e5175f7'),

    ('Restaurant1 waypoints', 0x4b66f4, 'i', 90, '85a354abccdf071b9caf0e6b444fd2c5645eb424c7fadb133e5931148d4d5fc1'),

    ('Waiter X', 0x4b685c, 'i', 33, 'a5dfd63c95a725f58e9ce2a3a305e21785a05fe1c48eef4cae63825f71091da8'),

    ('Waiter Y plus observed pad', 0x4b68e0, 'i', 34, '51ffbebb8a567cc6b01cdc1b124b45af3dd0ace346f2763d03d3842b352da81e'),

    ('Cafe positions', 0x4b6990, 'i', 42, 'c6ad7d3e59455281f184425f34674a8a8ef0c3b5c38837768220999984c181b4'),

    ('Cafe walk indexed view', 0x4b6a34, 'i', 65, '3570c992e6a7f53a980eaf9d1031f95ce9f67034ef6a3c6ca5fb118c400bbaa5'),

    ('Cafe chair offsets', 0x4b6b38, 'i', 44, '1fe4cf4a4d63e98dde60bd3118ae6232cb281d6ddefcb34426bcc1bd752f85c7'),

    ('Cafe directions', 0x4b6be8, 'i', 32, '20cca3d14d048eae4f9f1998d8b8ba71798415d27de577bbbbe593ea5baa55f8'),

    ('Cafe depth storage view', 0x4b6c68, 'i', 132, 'b6f82e249b6f5b8a02248873056ef7c78c176f59f20d1d6a6a4f2860b78750a1'),

    ('Boating physical queue offsets', 0x4b5290, 'i', 10, 'b4df82ead7830e1323794d74a904a220605cd28dc362a66733d8badb26c35ef3'),

    ('Copters sprite pointers', 0x4b4170, 'I', 10, '08934deb9894a6521759fe443ed20716ef0536ec48b1ac42ec26956530af9253'),

    ('Copters FX rows', 0x4b4140, 'I', 12, 'd5e7747a64cbca419547fa6f66ea72fb3f95f1caa77da4caa18587430b028b15'),

    ('Copters polyline A points', 0x4b4198, 'i', 12, 'fdb2ed21d27308adb97177c371f53a5ebb2ba6a5746607aeca293f14e5fc32ad'),

    ('Copters polyline A descriptor', 0x4b41c8, 'I', 2, '6e60d62938cbe6ecbad64ab25726fa7338bb8507ae8e8d2326435bee94e37b9f'),

    ('Copters polyline B points', 0x4b41d0, 'i', 14, 'eb879f787f1ba191b9d8f7e8814d31fe4bf7a8ba54c12bb7ca13b4602325c425'),

    ('Copters polyline B descriptor', 0x4b4208, 'I', 2, 'b4e16e0ce8f0a00334b24f0801c8c7a6f289795cc723b5130a2d4fb2f7942593'),

    ('Copters polyline C points', 0x4b4210, 'i', 12, '609e1474405af8accec37e0ac28b0229a07e11f90e12e7c7d0691248a87b8116'),

    ('Copters polyline C descriptor', 0x4b4240, 'I', 2, 'e5aa69adb9a25d2c59dbe531f9865287e24edf0b219eede18f2165d9dd92d3f1'),

    ('Copters polyline D points', 0x4b4248, 'i', 10, '62f5179c43d6a118350c0578f0cc6913555c85f1a287eca06ba10eef8200f713'),

    ('Copters polyline D descriptor', 0x4b4270, 'I', 2, '9dcf01e49a43e8556ab3798e23aabb6db627cd0260b9983c8a60892c84b2187a'),

    ('Copters polyline E points', 0x4b4278, 'i', 8, 'a8a63b32b3859df727a69d50ab7e75e287508bf969a6320d0c312e7024fb347f'),

    ('Copters polyline E descriptor', 0x4b4298, 'I', 2, '56f110c3082a4e87024af27ea46239e684b8fd06cd9a3f610a5e86041827b514'),

    ('Copters matrix selectors', 0x4b42a0, 'i', 12, '2a5635b7fe6620342095a70bc9faa1bc5729f3b693240ff7978d84936d1ea689'),

    ('Copter pose jump table', 0x404840, 'I', 5, '122d94df31082cd6ab82108c654ed42ee54f09ca9b7e6dc3efe5ed743b146e61'),

]
for name, va, fmt, count, digest in tables:
    raw = read(va, struct.calcsize("<"+fmt)*count)
    assert hashlib.sha256(raw).hexdigest() == digest, name
    print(name, hex(va), words(fmt, va, count))
for va, count, stride in ((0x4b40c8,4,12),(0x4b4140,4,12),
                           (0x4b4170,10,4),(0x4b4f08,4,4)):
    print("strings", hex(va), [cstring(words("I",va+i*stride,1)[0]) for i in range(count)])
for va in (0x4b76b8,0x4b7750):
    np, pp = words("I",va,2)
    assert np == 4
    for part in words("I",pp,np):
        nf, steps = words("I",part,2)
        print("Tower",hex(va),hex(part),[words("i",steps+i*16,4) for i in range(nf)])
# name, VA, exclusive end, instruction count, SHA-256
functions = [

    ('Copters_StepMachine', 0x404a90, 0x404bb8, 115, 'c4937d6c8dec9ca115ab828022dd4970d86fe46c13ef37188a57f514de0b17aa'),

    ('SpinningBarrels_StepMachine', 0x43c7f0, 0x43c92a, 113, '6edc81f512645077953b0791f0c3fa956f97e046d36a98461f9d762818e2c91f'),

    ('PlaneRide_StepMachine', 0x43e2b0, 0x43e3e5, 112, '876567972feb58a2f6840e77e1f6b424b5146ea03b6aa3b9e859e1f040ce9539'),

    ('SafariRide_StepMachine', 0x4150c0, 0x4151f3, 111, '0757277ff1af74d18d977ee9a9dde9881a1478a3cf94fde1122495005d36a805'),

    ('SpiderRide_StepMachine', 0x4161f0, 0x416310, 103, '72993a85af4d7ea0c313859058379e67f516a792f6224ffdda05d4aa16967c48'),

    ('SpaceTower_StepMachine', 0x43b990, 0x43ba9e, 94, '9161963f40af6a506963ff2577b6f16b27319228ef42f1d7b5d91c214c8aafb7'),

    ('EarthSlide_StepMachine', 0x42d560, 0x42d5ed, 53, 'dd3ef7bb439e5874edc07e3d89a4620b6e3a058dd641da2a1802547763d16cfb'),

    ('Copters_InitRecord', 0x403e90, 0x40403a, 118, 'b2d4093a0b671d996aa23e91f10259d78c92fe1b797210541f2c4616bec3652e'),

    ('SpaceTower_StopRide', 0x43aac0, 0x43ab6f, 56, '891f9dc7640e7783d957ee34e980ded0db8902a756759aaa713b1981442b2318'),

    ('PlaneRide_StopRide', 0x43d9f0, 0x43da5c, 37, 'cc243baf9d3179ca9ded82cfea28198f14c28cb6807606a4b8dad19da301fe97'),

    ('SafariRide_StopRide', 0x414b10, 0x414b7b, 36, '0c40b9c266c73a2ca41042aaedf70d4464039c060f08e0fb084e314be00cf280'),

    ('SpiderRide_StopRide', 0x415a90, 0x415ad6, 26, '66186cf09d73bbd981a50e51385f4f444f9ac5775dc567e7866aae9dae7264e1'),

    ('SpinningBarrels_StopRide', 0x43c2f0, 0x43c313, 11, 'e3e9db001f7f541f4aa4509ebfbd644a5e18da708aca29ef944ab595d8473cc1'),

    ('SpaceTower_PickSeat', 0x43acb0, 0x43acf6, 22, '8f5cbc051799c7d398bb530ba1ce75f9b96420d4d81d8226e58dc44652c6774e'),

    ('SpaceTower_StartSound', 0x43aa10, 0x43aa49, 18, 'baca11e73fd07dc9f58ee65982940f1e9cd9520ab35273f1d0ae6d058f1f8647'),

    ('SpiderRide_StartSound', 0x4159e0, 0x415a19, 18, '00d75a922e194982c41a580279ba36bfbb50997c4c8aa7b534b91be8c5751ecf'),

    ('GoldRush_HasFreePan', 0x406e90, 0x406ebd, 18, 'db8251172fd4276828e62e44b00cbb5f5cd7be764ad31facad4aa576f3ada6b7'),

    ('EarthSlide_AppendQueue', 0x42ce90, 0x42ceba, 16, '242d3e1ab0ad1f1d1d534e6f670bad09529cfcde1aaf4e2c0fda8260f1d2b1bf'),

    ('TowerCarStep', 0x43a940, 0x43a9ac, 37, '6d5f42565b03be1854b4645a059c884b085c8d77b01d908b778abb9a660f28a4'),

    ('TowerPlaceRiders', 0x43b810, 0x43b990, 116, '1ebc9c5c6f39094f1419207994fd036d12b0feed1379f96629fe5c249eaabe39'),

    ('CopterAdvanceFrame', 0x404860, 0x40489b, 23, '4ee86601f5d4ebaad7449e898b7560b2da260bc567419f50cfe1e09040c37aab'),

    ('CoptersReset', 0x4049a0, 0x404a84, 71, '8097b33d67dbb5ff84849c6917bf689f63f86e71035768bb6a224fbe2fd4526d'),

    ('CopterPlaceRider', 0x404630, 0x404840, 168, 'faf978eeeba63a5a232b12b6d47737589d0ae1169ad4a0c68e55228d658f2f3c'),

    ('Carousel_PickSeat', 0x42cd20, 0x42cd61, 25, 'b0a9305220a58cb58b4390f5ff47f8dfc2925d96c03a8006cfbe96f61c550ce8'),
]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True
all_starts = {}
for name, va, end, count, digest in functions:
    raw = read(va,end-va)
    assert hashlib.sha256(raw).hexdigest() == digest, name
    ins = list(md.disasm(raw,va))
    assert len(ins) == count and ins[-1].mnemonic == "ret", name
    assert ins[-1].address+ins[-1].size == end, name
    starts = {i.address for i in ins}
    all_starts[name] = starts
    for i in ins:
        if i.group(capstone.CS_GRP_JUMP) and i.operands[0].type == X86_OP_IMM:
            assert i.operands[0].imm in starts, (name,hex(i.address))
        if "--disasm" in sys.argv:
            print(f"{i.address:08x} {i.bytes.hex():20} {i.mnemonic:8} {i.op_str}")
    print("function OK",name,count,end-va)
assert all(x in all_starts["CopterPlaceRider"] for x in words("I",0x404840,5))
assert len(functions)==24 and sum(x[3] for x in functions)==1517
assert sum(x[2]-x[1] for x in functions)==4489
archive = Path(sys.argv[2]).read_bytes()
assert len(archive) == 16424086
assert hashlib.sha256(archive).hexdigest() == "b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2"
# Full directory paths were resolved from child/sibling RES directory nodes.
assets = [

    ('3DData/copters.pos', 4145344, 7688, '50f717bccf4f22b2f5f06b3cacf568acf9faae7f29bb8f1724dc1236a405dc2b'),

    ('3DData/earth.pos', 4143704, 776, '08092f3c9ef3bae53b99df362106671db2871290dff1bfa97113aae350cb29b1'),

    ('3DData/earthslide.rin', 4145256, 78, '3044f6298405949d55f9c368fd24bb10cd028e42fa6a885fb80c18892fd54900'),

    ('CompSprite/COPTERS SPRITE.CSP', 4532132, 296, 'db0ab647bf727d2031ea4715e9b5ab69ae7c34a0a4353948b9c26815ea01c54f'),

    ('Objdesc/CAROUSEL.ODF', 4769656, 467, '3b4c3c011ca6494d3bfcb3d7c134eae9f93bfcc44eeed606429faf417497bca7'),
]
for name, offset, size, digest in assets:
    assert 0 <= offset <= offset+size <= len(archive)
    raw = archive[offset:offset+size]
    assert hashlib.sha256(raw).hexdigest() == digest, name
    print("asset OK",name,offset,size)
    if name.endswith("CAROUSEL.ODF"):
        assert u("I",raw)[0]==120 and u("h",raw,0x2e)[0]==10
        assert list(range(0x20,0x20+10))==list(range(0x20,0x2a))
    if name.endswith(".pos"):
        frames, streams = u("2I",raw)
        assert size == 8+frames*streams*48
        print("POS frames,streams",frames,streams)
        if name.endswith("copters.pos"):
            assert (frames,streams)==(32,5)
            assert all(u("I",raw,8+i*48+8)[0]==4 for i in range(160))
    if name.endswith("earthslide.rin"):
        assert u("I",raw)[0]==1 and raw[4:10]==b"Box97\0"
        assert u("I",raw,10)[0]==16 and u("16I",raw,14)==(0,)*16
print("PASS: tables, strings, Tower programs, 24 functions, switch targets, 5 assets")

```
