# Presentation executable and asset evidence

This supplement closes the five presentation source boundaries recorded at baseline `f8f5854481b2a87fb456a37b02ce581e9206b400`. The specification combines the audited C with read-only extraction and bounded x86 disassembly of the original PE32 executable; no game or compiler was run. Symbol names are reconstruction labels, while virtual addresses and bytes identify the original behavior. [Presentation](presentation.md), [source audit](presentation-audit.md), [original executable](../../../legoland/original/legoland.exe)

The executable SHA-256 is `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`, image base `0x400000`. All ranges below use **exclusive ends**, little-endian fields and SHA-256 over exact file-backed bytes. Zero-filled virtual `.data` is identified separately; it is never read by extrapolating a file offset. The extraction/check blocks in this page run from the Scope J worktree root and make no output files. [Original executable](../../../legoland/original/legoland.exe)

## 1. Complete static UI tables

The manifest identifies storage, types and counts independently of the displayed decoded values. String-pointer tables also require the pointed-to strings; the extraction prints these and checks the concatenated price-name digest. Empty padding and initially null sprite references are initialization data, not missing behavior. [Original executable](../../../legoland/original/legoland.exe), [fpui5.c](../../LEGOLAND/fpui5.c), [screens3.c](../../LEGOLAND/screens3.c)

| Table | VA | Element format | Count | Bytes | SHA-256 |
| --- | --- | --- | --- | --- | --- |
| Free-play prices | `0x004bdeb8` | `<B3xIii` | 134 | 2144 | `3ebc0a05629a7ef3a834cbe7dc03510065927b59b7491798fde6dca812e4eb64` |
| Character map | `0x004bad58` | `<Bb` | 59 | 118 | `89a4a10961bb6f43ecfd8f27e156f95ba3fa9c7d5d6518a96d1bd28b516a4595` |
| Theme names | `0x004bafa8` | `<20s` | 4 | 80 | `ef8316aef14f5b9a8765e55f35ac8c148ce2fb51e8af3ef1566c358562919bf9` |
| Submenu names | `0x004baffc` | `<20s` | 4 | 80 | `67d94d83f2d69d79dacb001f8666ad0f345b727dddd10649534c941c4604cf70` |
| Control positions | `0x004bb04c` | `<ii` | 9 | 72 | `34650dfa0a4fc8ddab5a3ee3ecdcde02e9d52f1ce1b1dfccc2581f051e1b0801` |
| Initial theme-closed flags | `0x004bb094` | `<i` | 4 | 16 | `1b897dddd4c151e2a2e6e3e91b7ea0f7fc4fd5ed00ef1c9669e8566393a02586` |
| World markers | `0x004beb80` | `<IIiiiII` | 10 | 280 | `cb5c533ff5314901a5a9e5a77714fb25a77f9482a2b77197d0553b611b6c698b` |
| Tutorial markers | `0x004beca0` | `<IIiiiII` | 5 | 140 | `2d35e66bf646030df48f69d91335ea71e9a23cc01abcc3d8e1f5ffeee883961f` |
| Cursor palettes B/A/C/D | `0x004b95c4` | `<8B` | 4 | 32 | `d478b2eb440a981a72a2843ddeae0a9445c52cfcf46f27dc7fbe6654af9c644e` |

### Free-play prices

Each 16-byte row is `{u8 id; u8 padding[3]; char *name; s32 price; s32 chosen}`. Padding and chosen are initially zero in every row. IDs 131 and 132 are absent: physical rows 0..130 have matching IDs, row 131 has ID 133 and row 132 ID 134. The final row 133 is `{0,0x004d8bb0,0,0}`; its name points into zero-filled virtual `.data`, so the sentinel is an **empty string at a nonnull address**. The `.data` raw bytes end at `0x004c2000`, before that address; the section's virtual extent includes it. Lookup is case-insensitive and terminates on empty name. The 133 names with a NUL after each, concatenated in row order, hash to `1410a83bb878f5523716911792cbb13c632a1b3da61e0c40bc2c22728ee7a08d`. [Original executable](../../../legoland/original/legoland.exe), [fpui2.c](../../LEGOLAND/fpui2.c), [fpui5.c](../../LEGOLAND/fpui5.c)

| ID | Exact name | Price |
| --- | --- | --- |
| 0 | HEDGE | 32 |
| 1 | PALM TREE | 27 |
| 2 | BARREL | 14 |
| 3 | TREE 1 | 29 |
| 4 | MINI PINE TREE | 23 |
| 5 | FLOWERS | 19 |
| 6 | SHRUB | 13 |
| 7 | WATERPUMP | 59 |
| 8 | CASTLE TREE 1 | 28 |
| 9 | CASTLE TREE 2 | 33 |
| 10 | CASTLE TREE 3 | 24 |
| 11 | CASTLE WELL | 43 |
| 12 | CROCTREE | 264 |
| 13 | MONKTREE | 167 |
| 14 | PALM TREES 2 | 85 |
| 15 | PLANTS 1 | 17 |
| 16 | PLANTS 2 | 35 |
| 17 | PLANTS 3 | 17 |
| 18 | PLANTS 4 | 40 |
| 19 | FLOWER BED 1 | 23 |
| 20 | FLOWER BED 2 | 25 |
| 21 | FLOWER BED 3 | 23 |
| 22 | FLOWER BED 4 | 26 |
| 23 | BUSH 1 | 30 |
| 24 | BUSH 2 | 45 |
| 25 | WEST BUSH 1 | 16 |
| 26 | WEST BUSH 2 | 17 |
| 27 | WEST BUSH 3 | 16 |
| 28 | CACTI 1 | 20 |
| 29 | CACTI 2 | 129 |
| 30 | CACTI 3 | 85 |
| 31 | CACTUS 1 | 18 |
| 32 | CACTUS 2 | 17 |
| 33 | CACTUS 3 | 18 |
| 34 | KEEP | 23 |
| 35 | BARROW | 15 |
| 36 | FOUNTAIN 1 | 583 |
| 37 | FOUNTAIN 2 | 210 |
| 38 | FOUNTAIN 3 | 150 |
| 39 | XXCASTLE_DUMMY | 0 |
| 40 | ANUBIS | 18 |
| 41 | SPHINX | 105 |
| 42 | EGYPTIAN STATUE | 44 |
| 43 | OBLISK SMALL | 15 |
| 44 | BIG OBLISK | 22 |
| 45 | ABU SIM | 489 |
| 46 | PYRAMID 1 | 616 |
| 47 | PYRAMID 2 | 174 |
| 48 | DINO BIG | 383 |
| 49 | DINO MINI | 765 |
| 50 | DINO SMALL | 329 |
| 51 | T-REX | 1277 |
| 52 | SMALL POWER STATION | 586 |
| 53 | CRYSTAL POWER STATION | 886 |
| 54 | LEGO SHOP 1 | 247 |
| 55 | LEGO SHOP 2 | 244 |
| 56 | LEGO MEDIA SHOP | 225 |
| 57 | RESTAURANT 1 | 522 |
| 58 | RESTAURANT 2 | 894 |
| 59 | EXPLORERS INSTITUTE | 471 |
| 60 | SHARK CAFE | 126 |
| 61 | SHARK CAFE BROLLY | 24 |
| 62 | CASTLE BBQ | 456 |
| 63 | FOODCART DRINK | 34 |
| 64 | FOODCART FOOD | 43 |
| 65 | FOODCART ICECREAM | 32 |
| 66 | CHUCK WAGON | 33 |
| 67 | SHERIFF | 88 |
| 68 | BANK | 84 |
| 69 | GENERAL STORE | 79 |
| 70 | JAIL CELL | 30 |
| 71 | GOLD RUSH | 5130 |
| 72 | FORT | 1756 |
| 73 | SALOON | 181 |
| 74 | TEMPLE | 1279 |
| 75 | CATAPULT | 1303 |
| 76 | OCTOPUS CAFE | 373 |
| 77 | SPACE TOWER RIDE | 408 |
| 78 | COPTERS | 2117 |
| 79 | CAROUSEL | 7958 |
| 80 | PLANE RIDE | 7116 |
| 81 | BALLOONZ | 3403 |
| 82 | SAFARI RIDE | 4544 |
| 83 | ROPE CLIMB | 0 |
| 84 | EARTH SLIDE RIDE | 242 |
| 85 | SPIDER RIDE | 3183 |
| 86 | SPINNING BARRELS RIDE | 3213 |
| 87 | JOUST | 4422 |
| 88 | TEMPLE SLIDE | 785 |
| 89 | DRIVING SCHOOL | 706 |
| 90 | DRIVING SCHOOL ROADS | 19 |
| 91 | ZEBRA CROSSING | 12 |
| 92 | DRIVING SCHOOL PUMPS | 22 |
| 93 | JUNGLE CRUISE | 2111 |
| 94 | JUNGLE CRUISE MONKEY FISH | 228 |
| 95 | JUNGLE CRUISE MONKEY TREE | 387 |
| 96 | JUNGLE CRUISE WATER | 12 |
| 97 | BOATING SCHOOL | 605 |
| 98 | BOATING SCHOOL MERMAID | 43 |
| 99 | BOATING SCHOOL WATER | 12 |
| 100 | CASTLE LEVEL 1 | 1770 |
| 101 | CASTLE OBJ | 1770 |
| 102 | SQUARE_TRACK_HEIGHT_PATH | 12 |
| 103 | SQUARE_TRACK | 12 |
| 104 | SQUARE_TRACK_HEIGHT | 12 |
| 105 | SQUARE_TRACK_HEIGHT_0 | 12 |
| 106 | XXROLLER COASTER TRACK | 0 |
| 107 | LOG FLUME ENTRANCE | 2881 |
| 108 | LOG FLUME CSAW | 997 |
| 109 | LOG FLUME TUNNEL | 716 |
| 110 | LOG FLUME SPECIAL CORNER 1 | 411 |
| 111 | LOG FLUME SPECIAL CORNER 2 | 394 |
| 112 | LOG FLUME SPECIAL CORNER 3 | 493 |
| 113 | LOG FLUME SPECIAL CORNER 4 | 470 |
| 114 | LOG FLUME HOLD UP | 1854 |
| 115 | LOG FLUME DROP | 1162 |
| 116 | LOG FLUME TRACK | 503 |
| 117 | MINILAND SAN FRANCISCO | 3172 |
| 118 | MINILAND LONDON | 2426 |
| 119 | MINILAND FRANCE | 1981 |
| 120 | MINILAND HOLLAND | 2198 |
| 121 | MINILAND ITALY | 1079 |
| 122 | MINILAND WASHINGTON | 2863 |
| 123 | MINILAND INDIA | 1136 |
| 124 | MINILAND AUSTRALIA | 3185 |
| 125 | MINILAND NEW YORK | 3632 |
| 126 | MINILAND EGYPT | 3086 |
| 127 | xxMINILAND DENMARK | 1500 |
| 128 | WATER WORKS ENTRANCE | 756 |
| 129 | WATER WORKS CROCODILE FOUNTAIN | 1113 |
| 130 | WATER WORKS ELEPHANT FOUNTAIN | 1000 |
| 133 | WATER WORKS SHOWER | 54 |
| 134 | WATER WORKS WATER BLOCK | 171 |

### Character map and cheat matching

The character map is 59 **two-byte** records `{u8 DIK; s8 character}` at `0x004bad58`, including its final inert `(0,0)` record. The following rows preserve original iteration order; DIK values are hexadecimal. Capitals in the table are lowercased by the existing Caps Lock XOR Shift rule. A newly pressed later row supersedes an earlier one. [Original executable](../../../legoland/original/legoland.exe), [input.c](../../LEGOLAND/input.c), [input2.c](../../LEGOLAND/input2.c)

| Row | DIK | Signed value | Character / operation |
| --- | --- | --- | --- |
| 0 | `c8` | -11 | Up |
| 1 | `cb` | -12 | Left |
| 2 | `cd` | -13 | Right |
| 3 | `d0` | -14 | Down |
| 4 | `1e` | 65 | A |
| 5 | `30` | 66 | B |
| 6 | `2e` | 67 | C |
| 7 | `20` | 68 | D |
| 8 | `12` | 69 | E |
| 9 | `21` | 70 | F |
| 10 | `22` | 71 | G |
| 11 | `23` | 72 | H |
| 12 | `17` | 73 | I |
| 13 | `24` | 74 | J |
| 14 | `25` | 75 | K |
| 15 | `26` | 76 | L |
| 16 | `32` | 77 | M |
| 17 | `31` | 78 | N |
| 18 | `18` | 79 | O |
| 19 | `19` | 80 | P |
| 20 | `10` | 81 | Q |
| 21 | `13` | 82 | R |
| 22 | `1f` | 83 | S |
| 23 | `14` | 84 | T |
| 24 | `16` | 85 | U |
| 25 | `2f` | 86 | V |
| 26 | `11` | 87 | W |
| 27 | `2d` | 88 | X |
| 28 | `15` | 89 | Y |
| 29 | `2c` | 90 | Z |
| 30 | `0b` | 48 | 0 |
| 31 | `02` | 49 | 1 |
| 32 | `03` | 50 | 2 |
| 33 | `04` | 51 | 3 |
| 34 | `05` | 52 | 4 |
| 35 | `06` | 53 | 5 |
| 36 | `07` | 54 | 6 |
| 37 | `08` | 55 | 7 |
| 38 | `09` | 56 | 8 |
| 39 | `0a` | 57 | 9 |
| 40 | `52` | 48 | 0 |
| 41 | `4f` | 49 | 1 |
| 42 | `50` | 50 | 2 |
| 43 | `51` | 51 | 3 |
| 44 | `4b` | 52 | 4 |
| 45 | `4c` | 53 | 5 |
| 46 | `4d` | 54 | 6 |
| 47 | `47` | 55 | 7 |
| 48 | `48` | 56 | 8 |
| 49 | `49` | 57 | 9 |
| 50 | `0e` | -1 | Backspace |
| 51 | `01` | -2 | Escape |
| 52 | `1c` | -3 | Enter |
| 53 | `9c` | -3 | Enter |
| 54 | `39` | 32 | Space |
| 55 | `3a` | -10 | Case modifier |
| 56 | `36` | -10 | Case modifier |
| 57 | `2a` | -10 | Case modifier |
| 58 | `00` | 0 | Inert zero |

The cheat portion of `UpdateControllerFromKeyboardData` at `0x00473c10` shifts 19 bytes of a 20-byte ring and appends every **nonzero** typed result, including negative control values. Most checks use case-insensitive fixed-length suffix comparison; the two `memcmp` exceptions are explicitly marked below. Checks form one ordered always-enabled chain followed by one ordered game-mode-3 chain; processing then folds the held keyboard bits into the controller. The C body already contained these strings and effects; the previous “external cheat strings” coverage gap was an audit omission. [input.c](../../LEGOLAND/input.c), [original executable](../../../legoland/original/legoland.exe)

| Match, in check order | Gate | Effect |
| --- | --- | --- |
| `:THEME` | Always | SetTheme(0) |
| `:EGYPT` | Always | SetTheme(1) |
| `:INCA` | Always | SetTheme(2) |
| `:CASTLE` | Always | SetTheme(3) |
| `:WEST` | Always | SetTheme(4) |
| `:STOP` | Always | StopMusic() |
| `::DIE` | Always | exit(1) |
| `:ILIKETOTRAVEL` followed by two bytes | Game mode 3; case-insensitive 14-byte prefix | `10` selects level 15; `01`..`09` select levels 6..14; uppercase `T1`..`T5` select levels 1..5. Successful selection sets pending state 2; other suffixes do nothing |
| `:COLDHARDCASH` | Game mode 3 | AddBricks(5000) |
| `:HARDASNAILS` | Game mode 3 | Set ride wear to 0 |
| `:PRAISEME` | Game mode 3 | Set instant-appraisal flag 1 |
| `:WELOVELEGOLAND` | Game mode 3 | EndLevel(1) |
| `:IMPROVISE` | Game mode 3; **case-sensitive** | StopScript(1) |
| `:DIGGER` | Game mode 3 | TriggerSwitch(0), (1), (2), (3) |
| `:SHOWCAPACITY` | Game mode 3; **case-sensitive** | Set capacity display 1 |

### Menus, toolbar and markers

The four theme-name slots are 20 bytes each, in menu-index order; the four closed flags start as `(1,1,1,1)` but use LEGOLAND/CASTLE/WESTERN/ADVENTURERS order. Toolbar position indices 0..3 follow menu order, then Path, Query, Eraser, Map, Options. This preserves the existing theme-index mismatch instead of exchanging the WESTERN/CASTLE button positions. [Original executable](../../../legoland/original/legoland.exe), [bigscreens.c](../../LEGOLAND/bigscreens.c), [screens3.c](../../LEGOLAND/screens3.c), [fpui4.c](../../LEGOLAND/fpui4.c)

| Index | Theme name | Submenu name | Toolbar position |
| --- | --- | --- | --- |
| 0 | LEGOLAND THEME | SCENERY MENU | (8,379) |
| 1 | WESTERN THEME | FOOD STORES MENU | (105,379) |
| 2 | CASTLE THEME | SHOPS MENU | (202,379) |
| 3 | ADVENTURERS THEME | ATTRACTIONS MENU | (299,379) |
| 4 | Path tool | — | (12,418) |
| 5 | Query tool | — | (92,418) |
| 6 | Eraser tool | — | (172,418) |
| 7 | Map tool | — | (252,418) |
| 8 | Options tool | — | (332,418) |

Both marker tables have 28-byte rows `{lit_name,dim_name,help_id,x,y,lit_sprite,dim_sprite}` with the two runtime sprite pointers initially null. World rows begin at `0x004beb80`; the `bigscreens.c` declaration at `0x004beb88` is the help-ID view eight bytes into each row. Tutorial rows begin at `0x004beca0`. Sprite names below are exact archive names; coordinates and help IDs are decimal. [Original executable](../../../legoland/original/legoland.exe), [screens3.c](../../LEGOLAND/screens3.c), [bigscreens.c](../../LEGOLAND/bigscreens.c)

| Level | Lit name | Dim name | Help | x | y |
| --- | --- | --- | --- | --- | --- |
| 1 | Appraisal_Yes.lls | Appraisal_No.lls | 20 | 30 | 130 |
| 2 | Appraisal_Yes.lls | Appraisal_No.lls | 21 | 30 | 154 |
| 3 | Appraisal_Yes.lls | Appraisal_No.lls | 22 | 30 | 178 |
| 4 | Appraisal_Yes.lls | Appraisal_No.lls | 23 | 30 | 202 |
| 5 | Appraisal_Yes.lls | Appraisal_No.lls | 24 | 30 | 226 |
| 6 | Pro_SanFran_Lit.lls | Pro_SanFran_Unlit.lls | 25 | 5 | 26 |
| 7 | Pro_Belgium_Lit.lls | Pro_Belgium_Unlit.lls | 26 | 194 | 5 |
| 8 | Pro_Washington_Lit.lls | Pro_Washington_Unlit.lls | 27 | 131 | 102 |
| 9 | Pro_NY_Lit.lls | Pro_NY_Unlit.lls | 28 | 5 | 141 |
| 10 | Pro_India_Lit.lls | Pro_India_Unlit.lls | 29 | 340 | 83 |
| 11 | Pro_Holland_Lit.lls | Pro_Holland_Unlit.lls | 30 | 140 | 198 |
| 12 | Pro_London_Lit.lls | Pro_London_Unlit.lls | 31 | 282 | 162 |
| 13 | Pro_Italy_Lit.lls | Pro_Italy_Unlit.lls | 32 | 479 | 128 |
| 14 | Pro_Syd_Lit.lls | Pro_Syd_Unlit.lls | 33 | 216 | 276 |
| 15 | Pro_Egypt_Lit.lls | Pro_Egypt_Unlit.lls | 34 | 410 | 239 |

### Exact unmet-goal format strings

These 36 English formats are NUL-terminated CP1252 strings embedded in the executable, not localization IDs or an external message file. Rows follow the `m_*` declaration order in `softblit.c`; the goal-code dispatch and argument order are specified in the presentation page. Byte counts and SHA-256 include the terminating NUL, exclude inter-string alignment bytes and retain every spelling, punctuation and doubled space. The concatenated rows contain 1811 bytes and hash to `f28e873a3665aedac747e3412aca3f22c68ee89d06f3f014083d3aabbb5cc8b7`. [softblit.c](../../LEGOLAND/softblit.c), [goal dispatch](presentation.md#text-advisor-and-mission-help), [original executable](../../../legoland/original/legoland.exe)

| Source symbol | VA | Bytes including NUL | Exact format | SHA-256 |
| --- | --- | --- | --- | --- |
| `m_build_more_of` | `0x004ba6bc` | 39 | `You need to build %d more of object %s` | `6014fc43c20628098a4262fe27e93ec746f8886a620c84287025d142f27d013d` |
| `m_research` | `0x004ba69c` | 31 | `You need to research object %s` | `b005043e74785c5619ca7196595e649ed687997c30acd374b77ee71096606507` |
| `m_connect_one` | `0x004ba674` | 38 | `You need to connect your %s to a path` | `e09296b7e6d8b2dbdcb03efdc2e27333095ab8c31a7c06de8bac4f7de46cfe4d` |
| `m_connect_all` | `0x004ba648` | 42 | `You need to connect all objects to a path` | `d097c4e13867906013f9463c68f7f26468ecc40af6cca6857085b5a9c8a306ca` |
| `m_link_one` | `0x004ba60c` | 60 | `You need to link the path from your %s to the park entrance` | `e8c7dcd92708b453f41750dd13d6c6037dffa000f6383e006f8f155d8c28d7c9` |
| `m_link_all` | `0x004ba5c8` | 66 | `You need to link the paths from all objects to the park entrance.` | `3b3826cf2ce31b4c722538629fe5f6bda59f39c6d91a8439e1e528b132e06fcd` |
| `m_build_new_range` | `0x004ba590` | 55 | `You need to build %d new attractions from the %s range` | `5383f6e092fe05ad3c74bea15d8c13dd055d19dab49be887ec096ffd412d5cbd` |
| `m_build_more_range` | `0x004ba558` | 56 | `You need to build %d more attractions from the %s range` | `aeed70a2ae59334ce94ce42d5acf0df6ebaddc64b83880a935dc7933b56dfa4a` |
| `m_delete_all_range` | `0x004ba510` | 69 | `You need to delete all instances of %d attractions from the %s range` | `73e2ea1dc86d1eead8bb4bd0a11b91f582b39b07ff655362ff89ff5061f38411` |
| `m_delete_range` | `0x004ba4dc` | 49 | `You need delete %d attractions from the %s range` | `3e8cc1d78dd0ec44ed289cc6dcfa7e1704095657dc67b601a0f511ab640159a7` |
| `m_remove_items` | `0x004ba4b0` | 42 | `You need to remove %d items from the area` | `717b4978316b75526022f6263fa23d0e2d029ba1c5b29c5383b9a62a3ceb0e58` |
| `m_delete_obj` | `0x004ba48c` | 35 | `You need to delete %d of object %s` | `b107d2bc26af579430081f8a84b9248c162420e209dbd83ab94a27e48431571b` |
| `m_attract_people` | `0x004ba45c` | 48 | `You need to attract %d more people to your park` | `b95fac71f98cc80497daebdd0cec8419e23acac88b38001c5a5f5b8f19a71937` |
| `m_more_gardeners` | `0x004ba428` | 51 | `You need %d more gardeners to look after your park` | `bd6d7eeceae41a1d4417724ba11cd5f45151ab366145c7a09d4810afd29ee9c5` |
| `m_fewer_gardeners` | `0x004ba3fc` | 41 | `You need %d fewer gardeners in your park` | `d9c2b16eaaa866577241e53a43a085ed0cd44b32caf50dd7ad953c8e0a94f25d` |
| `m_more_mechanics` | `0x004ba3cc` | 47 | `You need %d more mechanics to help in the park` | `bdb88be029d7fa1d66e2a21f7f42cca0750ae18a24d25cdee827fc7a0f0f8b1d` |
| `m_fewer_mechanics` | `0x004ba3a0` | 41 | `You need %d fewer mechanics in your park` | `eba7d1ffd94fbdf51add154e1f785d3ba76332a0d86be98b283eb64376c15b76` |
| `m_cover_objects` | `0x004ba370` | 47 | `You need to cover %d more squares with objects` | `d4551e9112b766f16073621a6ae87c0bfeb83bdd480ec0240be1584f413057c2` |
| `m_cover_rides` | `0x004ba340` | 45 | `You need to cover %d more squares with rides` | `9cb03cd9b3b5ad5fdad0298510fecdc2b2f074fc2db91e8a2047545b36df8ef8` |
| `m_cover_shops` | `0x004ba310` | 45 | `You need to cover %d more squares with shops` | `3531d5d00d92b9046cd37256862cb26852f2fa35a451353a59d3839a70cc9945` |
| `m_cover_food` | `0x004ba2dc` | 52 | `You need to cover %d more squares with food outlets` | `d0ffdee0c117b8a8a9faa9dc880188b12d0c1e5465a5367df7bed16d8808684c` |
| `m_cover_scenery` | `0x004ba2ac` | 47 | `You need to cover %d more squares with scenery` | `c41faeca175da252e74fd7f8e07dfc3957920f6e934ea5b00fa596fcedb1d38e` |
| `m_cover_wonder` | `0x004ba26c` | 63 | `You need to cover %d more squares with stop 'n' wonder objects` | `6df3dc4243bff430286835a089e12344836ed21f3478d5b8f3e703ea858d1af1` |
| `m_path_scenery` | `0x004ba234` | 53 | `You need to line %d%% more of your path with scenery` | `2c896cf4034ecd70a0d5b91b89cc5843d08efa1d9295e84c51ce7e252515ad9c` |
| `m_save_coins` | `0x004ba210` | 35 | `You need to save up %d more coins.` | `d382f0f2c464ea7ac0573caad5fb3b13bc99669295d13f526e5745be396c54fc` |
| `m_happiness` | `0x004ba1dc` | 49 | `You need make %d people up to happiness level %d` | `1ca206a779fa96d55ee6e565016a8045fd2eb7535d1f007bf3c978c227b79d09` |
| `m_hunger_fewer` | `0x004ba198` | 67 | `You need to get %d fewer people with hunger levels greater than %d` | `cf9f9e452e4b8381f94437a87b19d45687df8c45641e8841794abe21664f8d92` |
| `m_hunger_more` | `0x004ba15c` | 58 | `You need to get %d more people with hunger level below %d` | `5f9e3f5cf162dec351bd9c95119447bd8e03bd562a8569a116ead2c1ca8cea9d` |
| `m_repairs` | `0x004ba110` | 74 | `You need make repairs to %d objects to bring them to above %d%% of health` | `11a700cf4d0958a00dff1ec95e70f254b746beb0cee3d538369e0bd270dc9a61` |
| `m_ride_people` | `0x004ba0e4` | 41 | `You need to get %d more people on the %s` | `3a5814bdf0654247362af46703bb1d24378612f817d0b285904d3f88c8c3c648` |
| `m_parts_diff` | `0x004ba0b4` | 45 | `You need to add %d different parts to the %s` | `66dec4a37d32ae6fc9b82a51bcddd7e5e94532ca11c354eeb9186736d577962f` |
| `m_parts_more` | `0x004ba08c` | 40 | `You need to add %d more parts to the %s` | `a5173604d703219072b68cbbcffbf8763c3341e89ad91df3f0d51ece7c3b6ce9` |
| `m_zone_legoland` | `0x004ba050` | 60 | `You need to improve the LEGOLAND zoning (from %d%% to %d%%)` | `ccfc498faeb2da29874fba61819627c6abaff96aaece802b189acbe8a18d4819` |
| `m_zone_adventurer` | `0x004ba010` | 62 | `You need to improve the ADVENTURER zoning (from %d%% to %d%%)` | `e2cbe7e23ed101f1660d180c6b50804cd308e1a4e958be00fb83dbef4398946f` |
| `m_zone_castle` | `0x004b9fd4` | 59 | `You need to  inprove the CASTLE zoning (from %d%% to %d%%)` | `ee26ea91d4e8b063a70cdec3fdc3b7c95f028a37d3bc298340f91c732b1ceecf` |
| `m_zone_western` | `0x004b9f98` | 59 | `You need to improve the WESTERN zoning (from %d%% to %d%%)` | `10bdb204625e5965d72140f4ad4ea98e4edb1419ed8c43b079ace8c6c8924d61` |

Code 0 has a different provenance: the executable format at `0x004b8bbc` is `%s` plus NUL, while its argument is script-string table `0x007fe120[strid]`. `strid` is a zero-based unchecked index, not a `GetString` localization ID. `LoadGame` block 3 calls `LoadScripts`, which reads count `0x00668720` and loads each entry using `LoadScriptString`: a 32-bit byte length, −1 for a null entry, or that many payload bytes followed by an appended runtime NUL. The loader and selection behavior are specified even though individual hint wording belongs to the level/save payload. [softblit.c](../../LEGOLAND/softblit.c), [savegame.c](../../LEGOLAND/savegame.c), [savechunks.c](../../LEGOLAND/savechunks.c), [savegame2.c](../../LEGOLAND/savegame2.c), [script framing](persistence.md)

This independent read-only check validates all 36 displayed strings, addresses, lengths and digests, plus the code-0 wrapper. It uses raw-backed PE sections and creates no files. [Original executable](../../../legoland/original/legoland.exe)

```sh
python3 - <<'PY_HINTS'
from pathlib import Path
import hashlib, re, struct
raw = Path('../legoland/original/legoland.exe').read_bytes()
assert hashlib.sha256(raw).hexdigest() == 'c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9'
def u(fmt, offset): return struct.unpack_from('<'+fmt, raw, offset)
pe = u('I', 60)[0]
assert raw[pe:pe+4] == b'PE\0\0'
assert u('H', pe+24)[0] == 0x10b
base = u('I', pe+52)[0]
sections = []
for i in range(u('H', pe+6)[0]):
    at = pe+24+u('H', pe+20)[0]+i*40
    virtual, rva, size, offset = u('4I', at+8)
    assert offset+size <= len(raw)
    sections.append((base+rva, size, offset))
def read(va, size):
    assert size >= 0
    for start, count, offset in sections:
        if start <= va and va+size <= start+count:
            return raw[offset+va-start:offset+va-start+size]
    raise ValueError((hex(va), size, 'not file-backed'))
page = Path('docs/runtime/presentation-data.md').read_text()
rows = re.findall(r'^\| `(m_\w+)` \| `0x([0-9a-f]{8})` \| (\d+) \| `([^`]+)` \| `([0-9a-f]{64})` \|$', page, re.M)
assert len(rows) == len({r[0] for r in rows}) == 36
parts = []
for name, va, count, value, digest in rows:
    data = read(int(va, 16), int(count))
    assert data == value.encode('cp1252')+b'\0', name
    assert hashlib.sha256(data).hexdigest() == digest, name
    parts.append(data)
assert sum(map(len, parts)) == 1811
assert hashlib.sha256(b''.join(parts)).hexdigest() == 'f28e873a3665aedac747e3412aca3f22c68ee89d06f3f014083d3aabbb5cc8b7'
assert read(0x4b8bbc, 3) == b'%s\0'
print('PASS: 36 exact goal formats, 1811 NUL-inclusive bytes, code-0 %s wrapper')
PY_HINTS
```

## 2. Original type-3 painter contracts

The ten leaf ranges below were disassembled through their last `ret`, excluding alignment padding. Eight are the hit/no-hit and left/right clipping combinations selected in the main specification; the other two are recoloring and highlighting. Each range decoded to its exclusive end. The manifest and read-only check reproduce the exact bytes and instruction counts, so the absence of reconstructed C bodies no longer leaves their behavior unspecified. [Original executable](../../../legoland/original/legoland.exe), [softblit2.c](../../LEGOLAND/softblit2.c), [bigrender.c](../../LEGOLAND/bigrender.c), [dispatch contract](presentation.md)

| Routine | Start | End | Bytes | Instructions | SHA-256 |
| --- | --- | --- | --- | --- | --- |
| HitLR | `0x00466d80` | `0x00467173` | 1011 | 325 | `e5ad55a0d7abc9901d3d1c6f8152a921ee951128f7356f3fe6cb272cf1ffd506` |
| HitL | `0x00467180` | `0x004673e3` | 611 | 202 | `57d711f21c102202ea9ec9ac6c14ab0454430673bc3e44cbe3d0273c98f925f0` |
| HitR | `0x004673f0` | `0x0046763c` | 588 | 197 | `6f47c78073eb06d2070fc3821954717b45775fc34f5a6d570172856c32b5eccd` |
| HitFull | `0x00467640` | `0x004677a6` | 358 | 122 | `4d024f92e872749ed5a9d119d177676e556cfc2faa6ce292c06ed33537b0155d` |
| NoHitLR | `0x004677b0` | `0x00467af1` | 833 | 271 | `279ca21b071b85bf6943a9e88ea8778f385e1ec626e35b9f7d4d3d962aa1ed3a` |
| NoHitL | `0x00467b00` | `0x00467d0e` | 526 | 180 | `120e8a909d2ebabe8335da6110e9551e195e65ffeb2694fed2f451f295e9c172` |
| NoHitR | `0x00467d10` | `0x00467ef9` | 489 | 167 | `6096d358539a0094f09a2ec5bf9980260b27f20a49128c8e4a2b078e0785bcb4` |
| Fast | `0x00467f00` | `0x00468037` | 311 | 114 | `04c7d4023f330ad0836d0efa2c3022efe6dfc5f9a8e19242db7216821c11af97` |
| Recolor | `0x00468040` | `0x00468405` | 965 | 303 | `7e8240aacddcec43d2d49a3b308d8c1838f6f06c92e5a5169ffb55146d67f51e` |
| Highlight | `0x00468410` | `0x004687f0` | 992 | 312 | `7275bdec7ad9b37cd0b6738ff5bc506af0cfd66c74d6f11bdaf4f53002c02d67` |
| CursorB | `0x0045fad0` | `0x0045fc9a` | 458 | 160 | `bcbad2e42c5550cef71149d3f71936c0c1fe0fed726b11d085cc15e08d36fbe1` |
| CursorA | `0x0045fca0` | `0x0045fefe` | 606 | 195 | `adac8b37f88d82ea0b4d9aca263846a3f3d702a63e1e7654285e93193648c342` |
| PopupSetup | `0x00471950` | `0x00471be8` | 664 | 192 | `0c01d4230286647c9bbdc6021740686a3f6cfb8606c0a6f0e1049165851c06af` |

### Frame layout and operation grammar

For frame start F, four dwords are total frame byte size at `+0`, **pixel-word count** at `+4`, padded **run-length byte count** at `+8`, and padded two-bit opcode count at `+0xc`. A=`F+0x10` contains u16 pixels; B=`A+2*pixel_count` contains u8 lengths; C=`B+length_count` contains packed two-bit controls. The leaf prologues load A as the word source, B as the byte source and C as the rotating-mask source. Thus both conflicting header descriptions of B, and the “u16 control entries” name for `+4`, are superseded. The dispatcher ignores `+0xc`; the shipped assets pad B to four bytes and C to 16 opcodes, with zero padding. [Original executable](../../../legoland/original/legoland.exe), [FORMATS COMP definition](../FORMATS.md), [softblit2.c](../../LEGOLAND/softblit2.c), [bigrender.c](../../LEGOLAND/bigrender.c)

The mask starts at 3 and rotates left by two after each code, advancing C by four bytes on wrap; codes are LSB-first and are not realigned per row. Pixel color zero is opaque black, never a transparency key. The same grammar is used by every leaf except the five top-skip defects below. [Original executable](../../../legoland/original/legoland.exe), [FORMATS COMP definition](../FORMATS.md)

| Primary code | Operation |
| --- | --- |
| 0 or 1 | Read one u16 from A and emit one pixel |
| 2 | Advance one pixel without writing |
| 3, next B byte=0 | End row; consumes no secondary code |
| 3, next B byte=L>0, secondary 0 | Read and emit L u16 pixels from A |
| 3, L>0, secondary 1 | Read one u16 from A and emit it L times |
| 3, L>0, secondary 2 or 3 | Advance L pixels without writing |

The leaf ABI begins `(dst,A,B,C,height,pitch,source_top)`; clipped forms also receive source-left and visible width, and hit forms receive a spare zero and mouse pixel address. Recolor/highlight use the same stream/clip values and the global mask. Top rows consume source only: destination is already positioned at the first visible row. Each visible row skips source-left, emits its visible span and consumes any right tail through the end-row marker so all three streams align for the following row. End-row advances the saved destination row by byte pitch, decrements height and restores the left/width counters. Left-only leaves paint the rest of the source row and require positive left skip, as ensured by their dispatcher; right-only leaves use width. Full leaves have neither side clip. No leaf validates stream bounds or total dimensions; the drawing loop presumes positive height. [Original executable](../../../legoland/original/legoland.exe), [softblit2.c](../../LEGOLAND/softblit2.c)

Plain hit leaves OR 1 into `0x007feb14` when a written singleton equals the mouse address, or when `uint32((mouse-destination)>>1)` is less than the **drawn** opaque run length. The shift is arithmetic before unsigned comparison. Clipped-away and transparent pixels do not hit; no-hit leaves never update the flag. Recolor stores `pixel & (mask & 0xffff)`; highlight stores `(pixel & (mask & 0xffff)) >> 1`, using a logical word shift. Their different hit behavior below must be retained separately from the plain rules. [Original executable](../../../legoland/original/legoland.exe), [bigrender.c](../../LEGOLAND/bigrender.c)

### Original leaf quirks

Five top-row skippers mishandle primary code 1: HitL, HitR, HitFull, Recolor and Highlight consume its literal word, then fall through into escape processing instead of continuing to the next primary code. Code 0 continues correctly. HitLR and all four no-hit leaves handle both literal codes correctly. The exact distinguishing flow is visible immediately after each top-skip test of `0xaaaaaaaa`, where those five implementations advance the pixel pointer then test `0x55555555` without an intervening unconditional jump. The archive scan below finds no primary code 1, so this defect is dormant for the checked 16-bit assets; it remains part of the executable behavior for other streams. [Original executable](../../../legoland/original/legoland.exe), [asset checks below](#5-read-only-reproduction-and-checks)

Recolor/highlight never test a singleton against the mouse and do not hit-test an opaque remainder emitted while crossing the left clip. They test only opaque runs that start in the normal visible loop. For those runs the mouse comparison uses the original run length **before** reducing it at the right clip; consequently a mouse in the unwritten right tail can still set the hit flag. Their pixel stores are correctly clipped. This differs from all plain hit leaves, which test the actual written span. [Original executable](../../../legoland/original/legoland.exe), [softblit.c](../../LEGOLAND/softblit.c)

## 3. Cursor segment pixels and phase

`DrawCursorSegmentB=0x0045fad0` and `A=0x0045fca0` take `(video_surface,kind,x,y,palette,height)`. The surface has byte pitch `+0`, width/height `+4/+8`, bits `+0xc` and pixel-format ID `+0x14`; `+0x10` is unused. Both map two RGB triplets through GetNearestColour before checking format, then paint only format 2 (16 bpp). Their indirect call through IAT `0x004ab2c0` resolves to **USER32.dll PtInRect**. It tests only the primary pixel against global clip `0x004bdea0`, so its right/bottom boundaries are exclusive. [Original executable](../../../legoland/original/legoland.exe), [bigrender.c](../../LEGOLAND/bigrender.c), [surface.c](../../LEGOLAND/surface.c)

The four eight-byte palettes contain `{r0,g0,b0,0,r1,g1,b1,0}`. Caller flag 4 chooses A, flag 2 chooses B, otherwise point-kind bits `0xc` choose C and other points D. Segment flag `0x20` independently chooses the dashed A painter; it does not select palette A. [Original executable](../../../legoland/original/legoland.exe), [bigrender.c](../../LEGOLAND/bigrender.c)

| Palette | VA | First RGB | Second RGB |
| --- | --- | --- | --- |
| B | `0x004b95c4` | (0, 128, 255) | (0, 255, 255) |
| A | `0x004b95cc` | (0, 0, 192) | (128, 255, 128) |
| C | `0x004b95d4` | (0, 192, 0) | (128, 255, 128) |
| D | `0x004b95dc` | (255, 0, 0) | (255, 128, 128) |

Each segment starts its local phase at global `0x007cacd4`, increments phase for every attempted primary pixel including clipped or masked pixels, and does not modify the global. The global is cleared at `0x0046389b` and incremented once in presentation paths at `0x00466165` and `0x00466307`. B uses phase modulo 16: indices 0..11 are color 0, 12..15 color 1, and every accepted pixel is stored, including black. A uses modulo 32: indices 0..11 and 16..27 are zero, 12..15 color 0, 28..31 color 1; it skips a zero table word. Therefore A also skips a legitimate palette color that maps to black. [Original executable](../../../legoland/original/legoland.exe), [bigrender.c](../../LEGOLAND/bigrender.c), [render2.c](../../LEGOLAND/render2.c)

| Kind | Primary samples | Companion thickness pixel |
| --- | --- | --- |
| 0 | `i=0..height-1`: `(x,y-i)` | `(x-1,y-i)` only when x>0 |
| 1 | `i=0..height`: `(x-i,y+floor(i/2))` | Same x, one row above, only when current y>0 |
| 2 | `i=0..height`: `(x+i,y+floor(i/2))` | Same x, one row above, only when current y>0 |
| Other | No samples | None |

The companion pixel is written only when the primary passes clipping (and A's nonzero test), but it has **no separate clip test**. Origin guards prevent negative x/y for that companion, not crossing a positive clip-left/clip-top boundary. B's diagonal loop is `0x0045fbdc..0x0045fc3d`, with phase mask at `0x0045fbf1` and companion stores at `0x0045fbfe/0x0045fc00`; A's mask and zero tests are `0x0045fe1c/0x0045fe24`. These are thick isometric edge samples, not arbitrary raster lines. [Original executable](../../../legoland/original/legoland.exe)

## 4. Popup reachability and profile editor overlap

The payload used by compiled popup cases `0x10b/0x10c` is a work order, as established by its element `+4`, assigned flag `+0x18`, worker `+0x1c` and cancellation consumers. Executable control flow closes the missing-producer question: **normal shipped popup setup never selects these display cases**. `PopUpInfoSetUp` at `0x00471950` resets first, publishes request metadata, and accepts only request kinds `0x103`, `0x306`, `0x307`, `0x308`; other kinds branch to reset at `0x00471bdf`. The displayed-kind stores produce only 0, `0xa`, `0x14`, `0x103`, `0x306`. Worker pickup requests `0x307/0x308` reset after dispatch. This is a dormant compiled display branch, not evidence for an invented order-pointer writer. [Original executable](../../../legoland/original/legoland.exe), [popup.c](../../LEGOLAND/popup.c), [fpui5.c](../../LEGOLAND/fpui5.c), [workorder2.c](../../LEGOLAND/workorder2.c)

A bytewise `.text` scan for direct references to payload `0x007fdf80` has exactly five operands, at `0x004729ed`, `0x00472a62`, `0x004734fd`, `0x00473511`, `0x0047353b`; all belong to `mov eax,[0x007fdf80]` reads starting one byte earlier. The sole address-taking use of popup base `0x007fdec0` is initializer `0x00470bb8`, followed by `rep stosd` at `0x00470bc3` with EAX=0, ECX=`0x40`: it clears the entire 256-byte block, including payload and kind. Other direct base references write the request kind. This bounded reference/producer audit establishes normal initialized-state reachability; it does not assume arbitrary memory corruption or external writes. [Original executable](../../../legoland/original/legoland.exe), [popup.c](../../LEGOLAND/popup.c)

| Displayed-kind instruction | Value / operation |
| --- | --- |
| `0x00471521` | Reset stores EAX=0 |
| `0x00471a0e` | Store request EAX, only on the `0x306` arm |
| `0x00471a7b` | Store `0xa` (garden path) |
| `0x00471ac5` | Store `0x14` (mechanic path) |
| `0x00471b01` | Store `0x103` (ordinary object) |
| `0x004700c0/0x00472574/0x00472924/0x00472bcf/0x004734f0` | Remaining references are tests/reads |

Work-order hit types **are active**, in the separate hit record `0x004bdd00`: calls at `0x00457b33/0x00457b5e` look up gardener/mechanic orders through `0x0049b130/0x0049b180`; nonnull results assign hit kinds `0x10b` at `0x00457b49` and `0x10c` at `0x00457b74`, with object stored at `0x004bdd04`. These support selection, footprint and cancellation; they are not writes to popup payload or displayed kind. The distinction resolves the earlier label and producer conflict while retaining the dormant DrawPopUpInfo/Delete2 behavior in the specification. [Original executable](../../../legoland/original/legoland.exe), [misc3.c](../../LEGOLAND/misc3.c), [fpui5.c](../../LEGOLAND/fpui5.c), [popup.c](../../LEGOLAND/popup.c)

The temporary editor overlays length on byte `+0x1e` of a 32-byte name region at `0x007cad60`; this is not a reserved length byte in every saved/live profile. Original EnterNewProfile `0x00491bd0` loads length at `0x00491bda`, compares it with 31 at `0x00491c21`, checks the **old** measured width against 123 at `0x00491c26`, appends a character and NUL, then finally stores the new length at `0x00491d4c`. Thus reaching length 30 overwrites that newly written NUL at index 30 with byte `0x1e`; reaching 31 overwrites the character at index 30 with byte `0x1f`, while index 31 contains NUL. Byte `+0x1f` has no editor flag semantics. The save-name editor uses the same overlapping byte and width threshold 203; its positive-character arm also makes the leading-space rejection unreachable. These are original overlap/width-check defects, not a 30-byte serialization format. [Original executable](../../../legoland/original/legoland.exe), [screens2.c](../../LEGOLAND/screens2.c), [profiles.c](../../LEGOLAND/profiles.c), [persistence](persistence.md)

## 5. Read-only reproduction and checks

The following command reads the executable, checks every range manifest above, decodes all nine tables and their pointed strings, and reproduces the exact popup operand lists. Run from the Scope J worktree root with Python 3 and Capstone. Its range checks reject BSS; the sentinel is separately validated against `.data` virtual extent. No game initialization, load, save or execution is involved. [Original executable](../../../legoland/original/legoland.exe), [source audit](presentation-audit.md)

```sh
python3 - <<'PY_CHECK'
from pathlib import Path
import struct, hashlib, re
import capstone
exe = Path('../legoland/original/legoland.exe').read_bytes()
assert hashlib.sha256(exe).hexdigest() == 'c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9'
pe = struct.unpack_from('<I', exe, 60)[0]
base = struct.unpack_from('<I', exe, pe+52)[0]
count = struct.unpack_from('<H', exe, pe+6)[0]
opt = struct.unpack_from('<H', exe, pe+20)[0]
sections = []
for i in range(count):
    p = pe+24+opt+40*i
    vsize, rva, rawsize, raw = struct.unpack_from('<4I', exe, p+8)
    sections.append((exe[p:p+8].rstrip(b'\0'), base+rva, vsize, raw, rawsize))
def read(va, n):
    assert n >= 0
    for name, start, virtual, raw, size in sections:
        if start <= va and va+n <= start+size:
            return exe[raw+va-start:raw+va-start+n]
    raise ValueError(('not file-backed', hex(va), n))
def string(va):
    out = bytearray()
    while (byte := read(va, 1)) != b'\0':
        out.extend(byte); va += 1
    return bytes(out).decode('cp1252')
page = Path('docs/runtime/presentation-data.md').read_text()
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
ranges = re.findall(r'^\| ([^|]+) \| `0x([0-9a-f]{8})` \| `0x([0-9a-f]{8})` \| (\d+) \| (\d+) \| `([0-9a-f]{64})` \|$', page, re.M)
assert len(ranges) == 13
for row in ranges:
    name, a, z, size, count, digest = row
    a, z = int(a,16), int(z,16)
    data = read(a,z-a); ins = list(cs.disasm(data,a))
    assert len(data) == int(size) and hashlib.sha256(data).hexdigest() == digest
    assert len(ins) == int(count) and ins[-1].address+ins[-1].size == z
    assert ins[-1].mnemonic == 'ret'
    print(name, hex(a), hex(z), len(ins))
rows = re.findall(r'^\| ([^|]+) \| `0x([0-9a-f]{8})` \| `<([^`]+)` \| (\d+) \| (\d+) \| `([0-9a-f]{64})` \|$', page, re.M)
assert len(rows) == 9
for name, a, fmt, count, size, digest in rows:
    a, count, size = int(a,16), int(count), int(size)
    data = read(a,size)
    assert len(data) == struct.calcsize('<'+fmt)*count
    assert hashlib.sha256(data).hexdigest() == digest
    print(name, list(struct.iter_unpack('<'+fmt,data)))
price = list(struct.iter_unpack('<B3xIii',read(0x4bdeb8,134*16)))
assert [v[0] for v in price[:-1]] == list(range(131))+[133,134]
assert all(v[3] == 0 for v in price) and price[-1] == (0,0x4d8bb0,0,0)
assert all(read(0x4bdeb8+16*i+1,3) == b'\0'*3 for i in range(134))
assert any(n == b'.data' and st+rawsize <= 0x4d8bb0 < st+vs for n,st,vs,raw,rawsize in sections)
names = [string(row[1]) for row in price[:-1]]
assert hashlib.sha256(('\0'.join(names)+'\0').encode('cp1252')).hexdigest() == '1410a83bb878f5523716911792cbb13c632a1b3da61e0c40bc2c22728ee7a08d'
print('PRICE',list(zip([v[0] for v in price[:-1]],names,[v[2] for v in price[:-1]])))
for start,count in [(0x4beb80,10),(0x4beca0,5)]:
    print('MARKERS',[(string(v[0]),string(v[1]),*v[2:]) for v in struct.iter_unpack('<IIiiiII',read(start,count*28))])
for start in (0x4bafa8,0x4baffc):
    print('MENU',[read(start+20*i,20).split(b'\0')[0].decode() for i in range(4)])
text = read(0x401000,695622)
def refs(value):
    needle = struct.pack('<I',value)
    return [0x401000+i for i in range(len(text)-3) if text[i:i+4] == needle]
assert refs(0x7fdf80) == [0x4729ed,0x472a62,0x4734fd,0x473511,0x47353b]
assert refs(0x7fdf9c) == [0x4700c2,0x471522,0x471a0f,0x471a7d,0x471ac7,0x471b03,0x472575,0x472926,0x472bd1,0x4734f1]
for address in [0x4729ec,0x472a61,0x4734fc,0x473510,0x47353a]:
    ins = next(cs.disasm(read(address,8),address))
    assert ins.mnemonic == 'mov' and ins.op_str == 'eax, dword ptr [0x7fdf80]'
print('popup direct-reference checks passed')
PY_CHECK
```

### Shipped COMP validation

An independent stream-consumption walk checked every 16-bpp COMP frame in the unique physical members found in both shipped graphics archives. The reader deduplicates member offsets, so these counts describe physical payloads rather than every named directory alias. Its full directory traversal agrees exactly with the heuristic reader’s unique `(offset,size)` set (Graphics1: 574 paths/581 nodes/570 payloads; Graphics2: 905 paths/907 nodes/898 payloads). It checked member/frame boundaries, exact pixel-word consumption, per-row width, length padding to four bytes, opcode padding to 16 codes and zero-valued padding. No clipping or color conversion was needed for these structural checks. The 365 unique eight-bpp COMP payloads in Graphics2 use the separate type-2 path and are outside this type-3 check. [Graphics1.res](../../../legoland/gamedata/disc/Graphics1.res), [Graphics2.res](../../../legoland/gamedata/disc/Graphics2.res), [archive reader](../../tools/resfile.py), [FORMATS](../FORMATS.md)

| Archive | Bytes | SHA-256 | Checked 16-bpp sprites | Checked frames |
| --- | --- | --- | --- | --- |
| Graphics1.res | 19962774 | `a0c5d7001dddac254ff23da798ba12fb0b0cc1b8f91028443909e7889555b80e` | 429 | 706 |
| Graphics2.res | 120357911 | `34c828ce12300b5b8bf87f5bbbafded4c218bc268bd33b6d31adf992ca065ecc` | 530 | 4579 |

All 959 sprites / 5,285 frames passed. Maximum unused B padding was three bytes and unused C padding 15 codes. Observed primary codes were 0:1,157,524; 2:484,872; 3:7,742,691; observed secondary codes were 0:2,664,246; 1:3,012,748; 2:1,365,047. Neither primary 1 nor secondary 3 appeared. This is structural asset validation, not execution of the original renderer or a rendered-image comparison. [Graphics1.res](../../../legoland/gamedata/disc/Graphics1.res), [Graphics2.res](../../../legoland/gamedata/disc/Graphics2.res)

The following check traverses the actual directory tree, cross-checks the existing archive reader, independently consumes the frame grammar, and asserts those counts. It takes approximately tens of seconds, reads the two archives, and writes nothing. [Archive reader](../../tools/resfile.py), [Graphics1.res](../../../legoland/gamedata/disc/Graphics1.res), [Graphics2.res](../../../legoland/gamedata/disc/Graphics2.res)

```sh
python3 - <<'PY_ASSETS'
from pathlib import Path
import runpy, struct, hashlib
from collections import Counter
parse_leaves = runpy.run_path('tools/resfile.py')['parse_leaves']
hist = Counter(); results = []; padding = Counter()
expected = [
 ('Graphics1.res','a0c5d7001dddac254ff23da798ba12fb0b0cc1b8f91028443909e7889555b80e',429,706),
 ('Graphics2.res','34c828ce12300b5b8bf87f5bbbafded4c218bc268bd33b6d31adf992ca065ecc',530,4579)]
for name,digest,sprites,frames in expected:
    data = Path('../legoland/gamedata/disc',name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest
    base = struct.unpack_from('<I',data)[0]
    seen = set(); files = {}; paths = set()
    def walk(rel,parent=''):
        while rel != 0xffffffff:
            assert rel not in seen and base+rel+20 <= len(data)
            seen.add(rel); p = base+rel
            child,nxt,folder,size,off = struct.unpack_from('<5I',data,p)
            end = data.index(b'\0',p+20)
            name = data[p+20:end].decode('ascii')
            full = parent+'/'+name if parent else name
            assert folder in (0,1) and name
            if folder: walk(child,full)
            else:
                assert child == 0xffffffff and 4 <= off and off+size <= base
                assert full not in paths; paths.add(full)
                files[(off,size)] = full
            rel = nxt
    walk(0)
    expected_directory = (574,581,570) if name == 'Graphics1.res' else (905,907,898)
    assert (len(paths),len(seen),len(files)) == expected_directory
    assert set(files) == {(v['offset'],v['size']) for v in parse_leaves(data)}
    ns = nf = 0
    for off,size in sorted(files):
        leaf = {'offset':off,'size':size}
        start,end = leaf['offset'],leaf['offset']+leaf['size']
        if data[start:start+4] != b'COMP': continue
        width,height,bpp,count,flags = struct.unpack_from('<5I',data,start+4)
        if bpp != 16: continue
        ns += 1; frame = start+24
        for index in range(count+(flags&1)):
            size,np,nl,nc = struct.unpack_from('<4I',data,frame)
            a = frame+16; b = a+2*np; c = b+nl
            assert nc%16 == 0 and c+nc//4 == frame+size <= end
            ai = bi = ci = x = y = 0
            def opcode():
                global ci
                assert ci < nc
                value = (data[c+ci//4] >> (2*(ci%4))) & 3
                ci += 1
                return value
            while y < height:
                op = opcode(); hist['primary'+str(op)] += 1
                if op < 2: ai += 1; x += 1
                elif op == 2: x += 1
                else:
                    assert bi < nl
                    length = data[b+bi]; bi += 1
                    if not length: y += 1; x = 0; continue
                    op = opcode(); hist['sub'+str(op)] += 1
                    if op == 0: ai += length
                    elif op == 1: ai += 1
                    x += length
                assert x <= width and ai <= np
            assert ai == np and nl == ((bi+3)&~3) and nc == ((ci+15)&~15)
            assert not any(data[b+bi:b+nl])
            assert not any((data[c+i//4] >> (2*(i%4)))&3 for i in range(ci,nc))
            padding[(nl-bi,nc-ci)] += 1
            nf += 1; frame += size
        assert frame == end
    assert (ns,nf) == (sprites,frames)
    results.append((name,ns,nf))
assert hist == Counter(primary0=1157524,primary2=484872,primary3=7742691,sub0=2664246,sub1=3012748,sub2=1365047)
assert max(a for a,b in padding) == 3 and max(b for a,b in padding) == 15
print(results,dict(hist),'all structural checks passed')
PY_ASSETS
```


### Copters animation payloads

The five `cop_*m.lls` layers used by the Copters CSP have 8-bpp COMP headers with count32 and flags2: exactly32 physical frames and no base-image frame. A size-word walk over all32 frames in each payload ends exactly at its directory member boundary. These are additional header/frame-boundary checks, separate from the16-bpp opcode validation above; they justify the byte+0x10 frame count cached by Copters_InitRecord. The SHA values cover each entire physical member. [Graphics2.res](../../../legoland/gamedata/disc/Graphics2.res), [Copters](attractions-data.md), [ridemisc4.c](../../LEGOLAND/ridemisc4.c)

| Member | Absolute archive offset | Bytes | Width×height | bpp/count/flags | Member SHA-256 |
| --- | --- | --- | --- | --- | --- |
| cop_b2m.lls | 29319208 | 118755 | 81×152 | 8/32/2 | `7b01b56510642ffe9e578c24f2b87e21b76c6319a1b5adb5b73958950298925e` |
| cop_ym.lls | 29437964 | 111638 | 89×145 | 8/32/2 | `e85639f0a9816031ac36742414992505eb3b22b300a12a4c62497a1d51d75891` |
| cop_rm.lls | 29549604 | 117890 | 83×148 | 8/32/2 | `0cbc8b5c5b6c9814dfffe1288f8aa169d92d13c524ba05a03733bc744110db3d` |
| cop_gm.lls | 29667496 | 110343 | 83×151 | 8/32/2 | `eaae15d91005115e25d78a4442a51c9b00891a0a9fcd0955dd2a9e931f072577` |
| cop_b1m.lls | 29777840 | 106091 | 88×146 | 8/32/2 | `55b7562ec12423197f344569af95fc9f226abcf2e205952f657426ee013ac387` |

Run the following read-only header and frame-size check from the worktree root. It also verifies each full-member digest in the table. [Graphics2.res](../../../legoland/gamedata/disc/Graphics2.res)

```sh
python3 - <<'PY_COPTERS'
from pathlib import Path
import struct,hashlib,re
data=Path('../legoland/gamedata/disc/Graphics2.res').read_bytes()
assert hashlib.sha256(data).hexdigest()=='34c828ce12300b5b8bf87f5bbbafded4c218bc268bd33b6d31adf992ca065ecc'
page=Path('docs/runtime/presentation-data.md').read_text()
rows=re.findall(r'^\| (cop_[^|]+) \| (\d+) \| (\d+) \| (\d+)×(\d+) \| 8/32/2 \| `([0-9a-f]{64})` \|$',page,re.M)
assert len(rows)==5
for name,off,size,w,h,digest in rows:
    off,size,w,h=map(int,(off,size,w,h))
    member=data[off:off+size]
    assert hashlib.sha256(member).hexdigest()==digest and member[:4]==b'COMP'
    assert struct.unpack_from('<5I',member,4)==(w,h,8,32,2)
    cursor=24
    for i in range(32):
        length=struct.unpack_from('<I',member,cursor)[0]
        assert length>0
        cursor+=length
        assert cursor<=size
    assert cursor==size
print('PASS: five Copters layers,32 frames each')
PY_COPTERS
```

These binary and asset checks close the table, cursor, painter and popup-reachability boundaries without claiming exact GDI text metrics, a portable renderer implementation, original executable matching of reconstructed C, or whole-game runtime testing. Those are separate deliverables from Scope J's recovered runtime specification. [Scope J](../SCOPE_J_runtime_spec.md), [source audit and WIP ledger](presentation-audit.md)
