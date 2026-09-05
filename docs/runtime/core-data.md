# Core executable and asset evidence

This recovery pass reads the original executable and resource bytes without executing the game or compiling reconstructed C. Evidence baseline: `legoland.exe`,802,857 bytes, SHA-256 `c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9`. Image base is`0x400000`. Addresses below are original virtual addresses. The adjacent original checkout supplies the input; no executable or game assets are committed.

The PE `.text` begins at VA`0x401000`/file`0x1000`; `.rdata` at`0x4ab000`/file`0xab000`; `.data` at`0x4b4000`/file`0xb4000`. `.data` has only`0xe000` file bytes but virtual size`0x37ff74`. Its remaining initial memory is zero-fill. In particular, the empty-name sentinel`0x4d8bb0` has no stored file byte; treating that VA as a raw file offset is invalid.

## Power table

VA`0x004b9340`, 528 file bytes; SHA-256`910f016b26436d4aa7bbaf943eab8d9773e3cc32c1aaed346f6287f53bca2069`.

| Index | Class name | Power |
| --- | --- | --- |
| 0 | Small Power Station | 800 |
| 1 | Crystal Power Station | 2500 |
| 2 | Dino Big | -100 |
| 3 | Dino Small | -30 |
| 4 | Dino Mini | -30 |
| 5 | T-Rex | -130 |
| 6 | Fountain 1 | -10 |
| 7 | Fountain 2 | -10 |
| 8 | Fountain 3 | -10 |
| 9 | Foodcart Drink | -2 |
| 10 | FoodCart Food | -2 |
| 11 | Foodcart Icecream | -2 |
| 12 | LEGO Shop 1 | -3 |
| 13 | LEGO Shop 2 | -3 |
| 14 | LEGO Media Shop | -3 |
| 15 | Octopus Cafe | -5 |
| 16 | Restaurant 1 | -10 |
| 17 | Restaurant 2 | -60 |
| 18 | Shark Cafe | -10 |
| 19 | Boating School | -200 |
| 20 | Boating School Mermaid | -30 |
| 21 | Copters | -100 |
| 22 | Driving School | -60 |
| 23 | Space Tower Ride | -40 |
| 24 | Spider Ride | -100 |
| 25 | Water Works Entrance | -30 |
| 26 | Water Works Crocodile Fountain | -6 |
| 27 | Water Works Elephant Fountain | -6 |
| 28 | Water Works Water Block | -4 |
| 29 | Water Pump | -15 |
| 30 | Bank | -5 |
| 31 | Chuck Wagon | -6 |
| 32 | General Store | -4 |
| 33 | Jail Cell | -2 |
| 34 | Saloon | -4 |
| 35 | Sheriff | -3 |
| 36 | Carousel | -60 |
| 37 | Fort | -8 |
| 38 | Gold Rush | -30 |
| 39 | Log Flume Entrance | -100 |
| 40 | Spinning Barrels Ride | -90 |
| 41 | Temple | -3 |
| 42 | Explorers Institute | -20 |
| 43 | Balloonz | -140 |
| 44 | Earth Slide Ride | -40 |
| 45 | Jungle Cruise | -100 |
| 46 | Plane Ride | -160 |
| 47 | Safari Ride | -80 |
| 48 | Temple Slide | -50 |
| 49 | Castle BBQ | -20 |
| 50 | Castle Level 1 | -8 |
| 51 | Castle Obj | -400 |
| 52 | Catapult | -2 |
| 53 | Joust | -75 |
| 54 | Miniland San Francisco | -40 |
| 55 | Miniland France | -40 |
| 56 | Miniland Washington | -40 |
| 57 | Miniland New York | -40 |
| 58 | Miniland India | -40 |
| 59 | Miniland Holland | -40 |
| 60 | Miniland London | -40 |
| 61 | Miniland Italy | -40 |
| 62 | Miniland Australia | -40 |
| 63 | Miniland Egypt | -40 |
| 64 | Miniland Denmark | -40 |
| 65 | empty-name sentinel at`0x4d8bb0` | 0 |

Names are compared case-insensitively; spelling/case above preserves the file. Positive values generate supply, negative values consume it. [Consumer](../../LEGOLAND/power.c)

## Male first names

VA`0x004bcecc`, 332 file bytes; SHA-256`2c73f7c41f4c19323d6bfd6aac77abca38c89ea1fa149f249182430f55530e4c`.

| Index | Name |
| --- | --- |
| 0 | Aaron |
| 1 | Alistaire |
| 2 | Anthony |
| 3 | Aron |
| 4 | Alex |
| 5 | Alexander |
| 6 | Andy |
| 7 | Brian |
| 8 | Bill |
| 9 | Barry |
| 10 | Chris |
| 11 | Christopher |
| 12 | Colin |
| 13 | Damian |
| 14 | Daniel |
| 15 | Darren |
| 16 | Dave |
| 17 | David |
| 18 | Dean |
| 19 | Doug |
| 20 | Dwayne |
| 21 | Edward |
| 22 | Eddy |
| 23 | Frank |
| 24 | Fred |
| 25 | Gary |
| 26 | Garth |
| 27 | Gareth |
| 28 | Glen |
| 29 | Graeme |
| 30 | Graham |
| 31 | Harry |
| 32 | Henry |
| 33 | Howard |
| 34 | Ian |
| 35 | Jason |
| 36 | Jay |
| 37 | Jeremy |
| 38 | Joe |
| 39 | John |
| 40 | Jonny |
| 41 | Ken |
| 42 | Kevin |
| 43 | Laurence |
| 44 | Larry |
| 45 | Malcolm |
| 46 | Mark |
| 47 | Michael |
| 48 | Mick |
| 49 | Micky |
| 50 | Mike |
| 51 | Narinder |
| 52 | Neil |
| 53 | Nicolas |
| 54 | Nigel |
| 55 | Paul |
| 56 | Pete |
| 57 | Peter |
| 58 | Ray |
| 59 | Rich |
| 60 | Richard |
| 61 | Rob |
| 62 | Robert |
| 63 | Robin |
| 64 | Ross |
| 65 | Russ |
| 66 | Russel |
| 67 | Shaun |
| 68 | Simeon |
| 69 | Simon |
| 70 | Stephen |
| 71 | Steve |
| 72 | Steven |
| 73 | Stewart |
| 74 | Stuart |
| 75 | Thomas |
| 76 | Tim |
| 77 | Timothy |
| 78 | Tom |
| 79 | Tony |
| 80 | Vernon |
| 81 | William |
| 82 | Walt |

## Female first names

VA`0x004bd018`, 360 file bytes; SHA-256`b6d05ed3bd06663f9d5a24e9c307f6f2fe41c3d7383fda1f66fb2db0bbcb488c`.

| Index | Name |
| --- | --- |
| 0 | Abbie |
| 1 | Allison |
| 2 | Amanda |
| 3 | Anna |
| 4 | Anne |
| 5 | Annette |
| 6 | Beckie |
| 7 | Brenda |
| 8 | Bernadette |
| 9 | Charlotte |
| 10 | Chloe |
| 11 | Christine |
| 12 | Claire |
| 13 | Danielle |
| 14 | Davina |
| 15 | Deborah |
| 16 | Denise |
| 17 | Ellie |
| 18 | Elizabeth |
| 19 | Faith |
| 20 | Fiona |
| 21 | Francis |
| 22 | Gillian |
| 23 | Gill |
| 24 | Gerri |
| 25 | Glenda |
| 26 | Gloria |
| 27 | Jade |
| 28 | Jackie |
| 29 | Jane |
| 30 | Janet |
| 31 | Jasmine |
| 32 | Jessica |
| 33 | Jill |
| 34 | Joanne |
| 35 | Jodie |
| 36 | Joy |
| 37 | Judy |
| 38 | Julia |
| 39 | Julie |
| 40 | Kate |
| 41 | Kathy |
| 42 | Katie |
| 43 | Kerry |
| 44 | Laura |
| 45 | Lauren |
| 46 | Lilly |
| 47 | Lisa |
| 48 | Madeline |
| 49 | Maggie |
| 50 | Mandy |
| 51 | Marcie |
| 52 | Margaret |
| 53 | Marianne |
| 54 | Mary |
| 55 | Michelle |
| 56 | Natalie |
| 57 | Natasha |
| 58 | Nicola |
| 59 | Norma |
| 60 | Paige |
| 61 | Patricia |
| 62 | Paula |
| 63 | Penelope |
| 64 | Penny |
| 65 | Petra |
| 66 | Rebecca |
| 67 | Rhian |
| 68 | Rose |
| 69 | Rosie |
| 70 | Sadie |
| 71 | Safron |
| 72 | Sally |
| 73 | Samantha |
| 74 | Sandra |
| 75 | Sarah |
| 76 | Sheila |
| 77 | Shelly |
| 78 | Sissy |
| 79 | Sonia |
| 80 | Sophie |
| 81 | Stella |
| 82 | Sue |
| 83 | Susan |
| 84 | Terri |
| 85 | Tracy |
| 86 | Trisha |
| 87 | Val |
| 88 | Veronica |
| 89 | Wendy |

## Surnames

VA`0x004bd180`, 428 file bytes; SHA-256`45eb5cfc7371b4daa60604eeda546bf663351eceaa5b3fb5803d682e77ded161`.

| Index | Name |
| --- | --- |
| 0 | Adams |
| 1 | Adamson |
| 2 | Ammar |
| 3 | Anderson |
| 4 | Andrews |
| 5 | Armstrong |
| 6 | Basran |
| 7 | Black |
| 8 | Blair |
| 9 | Blaine |
| 10 | Blakemore |
| 11 | Blake |
| 12 | Brown |
| 13 | Brownsmith |
| 14 | Buck |
| 15 | Buzer |
| 16 | Butler |
| 17 | Close |
| 18 | Colledge |
| 19 | Cooper |
| 20 | Copperfield |
| 21 | Darroch |
| 22 | Davis |
| 23 | Dinsdale |
| 24 | Donnelly |
| 25 | Doucet |
| 26 | Edwards |
| 27 | Fletcher |
| 28 | Giarmati |
| 29 | Gillo |
| 30 | Goodley |
| 31 | Gratton |
| 32 | Greene |
| 33 | Hay |
| 34 | Harrap |
| 35 | Hebden |
| 36 | Henning |
| 37 | Hollingworth |
| 38 | Holmes |
| 39 | Hudson |
| 40 | Hugard |
| 41 | Incley |
| 42 | Jackson |
| 43 | James |
| 44 | Johnson |
| 45 | Jones |
| 46 | Kavanagh |
| 47 | Keates |
| 48 | Kirk |
| 49 | Kermode |
| 50 | Lane |
| 51 | Livingstone |
| 52 | Mackintosh |
| 53 | MacIntyre |
| 54 | Marsh |
| 55 | Mason |
| 56 | McBride |
| 57 | McDonald |
| 58 | Mckenna |
| 59 | Miller |
| 60 | Mundy |
| 61 | Norris |
| 62 | Ogilvy |
| 63 | Palmer |
| 64 | Parks |
| 65 | Pashley |
| 66 | Perks |
| 67 | Phelan |
| 68 | Phillips |
| 69 | Potente |
| 70 | Price |
| 71 | Pugh |
| 72 | Quigley |
| 73 | Rabjohn |
| 74 | Ray |
| 75 | Richardson |
| 76 | Richmond |
| 77 | Scarne |
| 78 | Scotford |
| 79 | Sharpe |
| 80 | Shields |
| 81 | Shuttleworth |
| 82 | Simmons |
| 83 | Smith |
| 84 | Smith |
| 85 | Stephens |
| 86 | Stead |
| 87 | Sumner |
| 88 | Swinhoe |
| 89 | Tarbell |
| 90 | Taylor |
| 91 | Teather |
| 92 | Thacker |
| 93 | Thorley |
| 94 | Tillson |
| 95 | Tredoux |
| 96 | Turner |
| 97 | Upchurch |
| 98 | Vernon |
| 99 | Ward |
| 100 | Ware |
| 101 | West |
| 102 | White |
| 103 | Williams |
| 104 | Wright |
| 105 | Yates |
| 106 | Zennon |

Bloke bytes`+83/+84` select first name/surname; Person3D sex`+84` selects83 male or90 female names. Surname duplicates retain their separate indices. [Name lookup](../../LEGOLAND/loaders.c), [initialization](../../LEGOLAND/lfmisc.c)

## Message cooldowns

VA`0x004ba8e0`, 204 file bytes; SHA-256`ab13018bb2cbcd0e5ba649cf11cf948ce9b515ae388a48b521ab39b432148c06`.

| Index | Topic | Delay ms | Initial last-time |
| --- | --- | --- | --- |
| 0 | 203 | 1000 | 0 |
| 1 | 2100 | 20000 | 0 |
| 2 | 2101 | 20000 | 0 |
| 3 | 2102 | 1000 | 0 |
| 4 | 2103 | 1000 | 0 |
| 5 | 2104 | 1000 | 0 |
| 6 | 2105 | 1000 | 0 |
| 7 | 2106 | 1000 | 0 |
| 8 | 2107 | 1000 | 0 |
| 9 | 2108 | 1000 | 0 |
| 10 | 2109 | 1000 | 0 |
| 11 | 2111 | 1000 | 0 |
| 12 | 2110 | 1000 | 0 |
| 13 | 2112 | 1000 | 0 |
| 14 | 2113 | 1000 | 0 |
| 15 | 2114 | 1000 | 0 |
| 16 | 2115 | 1000 | 0 |

## Nearby-cell scan

VA`0x004bff28`, 96 file bytes; SHA-256`8cb65625c9ad5238b3fcf923935a8a32dbb9532f5181bd5e45677a8d987d357a`.

| Index | dx | dy |
| --- | --- | --- |
| 0 | 0 | 1 |
| 1 | 0 | -1 |
| 2 | 1 | 0 |
| 3 | -1 | 0 |
| 4 | 0 | 2 |
| 5 | 0 | -2 |
| 6 | 2 | 0 |
| 7 | -2 | 0 |
| 8 | 1 | 1 |
| 9 | 1 | -1 |
| 10 | -1 | 1 |
| 11 | -1 | -1 |

The ordered scan and cooldown consumers are in [workorder2.c](../../LEGOLAND/workorder2.c). Last-time values are mutable; zero is the original image initialization, not a reset on every call.

## High-level AI dispatch

VA`0x004b8368`, 104 file bytes; SHA-256`bc260d183c96d76ce61ec1cdd006779462f3dd6921a9afdf324bdc43bf8e94de`.

| State | Target |
| --- | --- |
| 0 | `0x00484910` |
| 1 | `0x0044f170` |
| 2 | `0x0044ebf0` |
| 3 | `0x0044ed70` |
| 4 | `0x00484910` |
| 5 | `0x00000000` |
| 6 | `0x0044f610` |
| 7 | `0x00484910` |
| 8 | `0x00484910` |
| 9 | `0x00484910` |
| 10 | `0x00484910` |
| 11 | `0x00484910` |
| 12 | `0x00484910` |
| 13 | `0x0044fe80` |
| 14 | `0x00450250` |
| 15 | `0x00450450` |
| 16 | `0x0049a480` |
| 17 | `0x0049a4b0` |
| 18 | `0x0049a4e0` |
| 19 | `0x0049a7f0` |
| 20 | `0x00450330` |
| 21 | `0x0049ba10` |
| 22 | `0x0049bd20` |
| 23 | `0x0044fe10` |
| 24 | `0x0049a4a0` |
| 25 | `0x0049a4d0` |

## Low-level AI dispatch

VA`0x004bd34c`, 64 file bytes; SHA-256`03820b747995544c325d2932a04b1153062a532f51f8f181e7aad0f8460ae46e`.

| State | Target |
| --- | --- |
| 0 | `0x004838a0` |
| 1 | `0x004838c0` |
| 2 | `0x00483ef0` |
| 3 | `0x00484090` |
| 4 | `0x00483d10` |
| 5 | `0x004838e0` |
| 6 | `0x00484220` |
| 7 | `0x004845d0` |
| 8 | `0x00484630` |
| 9 | `0x00484790` |
| 10 | `0x00483e20` |
| 11 | `0x00484470` |
| 12 | `0x00484520` |
| 13 | `0x004848e0` |
| 14 | `0x00483d90` |
| 15 | `0x00484350` |

The low-level dispatcher indexes the16-entry table with the unsigned state word without a bounds check. The next bytes after this table are a surname string, not additional handlers. High-level slot5 is null and the shared target`0x484910` is a bare return. [Dispatch callers](../../LEGOLAND/blokeai.c)

## Movement arrival and coordinate units

`0x4845d0..0x48462d` handles low-level state7. Before advancing, it tests squared distance from Bloke world`+68/+6c` to target`+24/+28` against `(2*speed)^2`, where speed is byte`+7f`. The helper`0x4841a0..0x4841d2` uses signed32-bit multiplication/addition/comparison. On arrival,`0x483240..0x483255` restores state`+0e` from saved-step`+10`, clears saved-step and returns the restored state. It writes neither position nor action. Otherwise state7 advances the MoveLine at`+98`, copies its output to world position and updates the walk animation.

Consequently state7 does **not** snap to its target. Gold-panning exit is bounded by the arrival radius; neither exact cancellation nor a fixed cumulative48-unit drift follows. `CalcMoveLine` at`0x480740` adds eight fractional bits to its caller coordinates (`input<<8 + 0x80`); `NavigMoveLine` at`0x4807f0` outputs the accumulator shifted right8. Ride inputs are already24.8 world units, making the internal accumulator effectively16.16 map coordinates. The movement helper’s older whole-tile wording does not change this caller/return scale. [bnvmove.c](../../LEGOLAND/bnvmove.c), [goldrush2.c](../../LEGOLAND/goldrush2.c)

## Script-string writer

`SaveScriptString` at`0x46c620..0x46c67f` writes a signed32-bit length, followed by exactly that many bytes. Null writes−1; empty writes0; neither has a payload. Nonempty strings use `strlen`, excluding the terminator. A failed prefix/payload write returns0 and a complete write returns1. The writer does not increment the script error counter. The new [savemisc2.c](../../LEGOLAND/savemisc2.c) body independently agrees with this instruction recovery. The reader adds its own NUL after reading length bytes, so both sides agree and the old length+1-on-disk header was incorrect. [Readers and script callers](persistence.md#script-state)

## ODF finalization and power-station add callbacks

`ObjDefFinalize` at`0x480aa0..0x480b34` makes three case-insensitive name patches. Water Works Shower and Water Works Water Block set entrance ints`+0c/+10` to1/0 and exit bytes`+24/+25` to1/0; the Water Block also sets word`+2e` to10. Water Works Elephant Fountain sets the corresponding pairs to6/2. There is no callback invocation in this function.

The ODF loader’s ordinary branch calls `SetCustomCallbacks` and then finalization; it does not call`+a4`. The DLL-failure branch explicitly calls that slot after custom registration, and successful library registration calls library-table entry7. These are distinct real paths, not a hidden initialization responsibility of the finalizer. The built-in custom dispatcher assigns callbacks and invokes its listed providers; its immediate sound initialization calls are separate. [Loader](../../LEGOLAND/llidb_odf.c), [custom dispatcher](../../LEGOLAND/screen.c), [callback matrix](callbacks.md)

`SmallPowerStation_Add` (`0x452ad0..0x452b14`) and `CrystalPowerStation_Add` (`0x452b20..0x452b64`) first call `AddBasicObject` (`0x45efe0`) with their original arguments, then start the station’s sound (`0x496d20`) with arguments1,1 and a local SoundSource kind2 carrying the placement x/y. The sample pointer comes from`0x4b8764`/`0x4b8758` respectively. The unused source-object dword is uninitialized. These bodies were recovered from the original image and are now independently provided by the newly merged [lfmisc2.c](../../LEGOLAND/lfmisc2.c). [SoundSource](../../LEGOLAND/audio3.c), [registration](../../LEGOLAND/screen.c)

## Mood, hunger and departure defaults

`ResetMapAI` at`0x462dd0` initializes five signed thresholds at`0x832928..0x832938` to`[-8000,-1000,100,1000,5000]`. Departure buckets use−8000,100,1000,5000; the mood display uses−1000 and1000. The25 departure-history bytes initialize to2 and the ring index to0. Script handler`0x478cd0..0x478d2a` accepts five integer strings and, during its execution phase, writes all five thresholds, so these are reset defaults rather than immutable constants. [objmap.c](../../LEGOLAND/objmap.c), [workers.c](../../LEGOLAND/workers.c), [simcore2.c](../../LEGOLAND/simcore2.c)

Bloke word`+7c` is the hunger accumulator operationally: initialization samples0..2400; enabled periodic updates add byte`+80` every32 ticks up through7000; the tier thresholds are1000/2400/4000/7000; hunger at least7000 makes a visitor leave when flags`0x28` are clear; leaving a type5 food venue clears it. The UI displays the tier as hunger, and the original goal strings at`0x4ba198`/`0x4ba15c` explicitly call the quantity hunger. Thus the “tiredness” and “age group” names in older reconstructions are misleading aliases, not separate physiology fields. [workers3.c](../../LEGOLAND/workers3.c), [rides.c](../../LEGOLAND/rides.c), [popup.c](../../LEGOLAND/popup.c), [softblit.c](../../LEGOLAND/softblit.c)

## Instruction and table fingerprints

These byte ranges bind the recovered claims to the original image; hashing alone does not replace the instruction/consumer analysis above.

| Evidence | VA | Byte count | SHA-256 |
| --- | --- | --- | --- |
| Power table | `0x004b9340` | 528 | `910f016b26436d4aa7bbaf943eab8d9773e3cc32c1aaed346f6287f53bca2069` |
| Male first names | `0x004bcecc` | 332 | `2c73f7c41f4c19323d6bfd6aac77abca38c89ea1fa149f249182430f55530e4c` |
| Female first names | `0x004bd018` | 360 | `b6d05ed3bd06663f9d5a24e9c307f6f2fe41c3d7383fda1f66fb2db0bbcb488c` |
| Surnames | `0x004bd180` | 428 | `45eb5cfc7371b4daa60604eeda546bf663351eceaa5b3fb5803d682e77ded161` |
| Message cooldowns | `0x004ba8e0` | 204 | `ab13018bb2cbcd0e5ba649cf11cf948ce9b515ae388a48b521ab39b432148c06` |
| Nearby-cell scan | `0x004bff28` | 96 | `8cb65625c9ad5238b3fcf923935a8a32dbb9532f5181bd5e45677a8d987d357a` |
| High-level AI dispatch | `0x004b8368` | 104 | `bc260d183c96d76ce61ec1cdd006779462f3dd6921a9afdf324bdc43bf8e94de` |
| Low-level AI dispatch | `0x004bd34c` | 64 | `03820b747995544c325d2932a04b1153062a532f51f8f181e7aad0f8460ae46e` |
| State7 | `0x004845d0` | 94 | `2b07778f4364372c9a4975297e72d4faa57f2569144693139bc5e0e9275ecbb3` |
| Arrival radius | `0x004841a0` | 51 | `39efaafbf13f73b39ea241148a5fc2900687dc85b80c03a7c12b7b77093420d7` |
| Restore low-level state | `0x00483240` | 22 | `086138c9c9e1fd4f4d18ede510c318fe4631c7ae86da7ef63b3410f97fc66c9d` |
| Script-string writer | `0x0046c620` | 96 | `c8811eceab1ef0aa031661e74ae72fb8737aca7e9ca572f29ef8e8b14e3c0169` |
| ODF finalizer | `0x00480aa0` | 149 | `fd07646800da52a62b982958c37d3f891c92a8c2e1266fc3de9c9073a7cbc07c` |
| Small station add | `0x00452ad0` | 69 | `ae42595e6b2ce335949386e0ddaf0a36d73ec9c40656aa507a09d30223e69ea9` |
| Crystal station add | `0x00452b20` | 69 | `a320cf317df71df82486a1355b02c61aaa78f43113d5b019d016b0ff2b78257f` |
| Mood reset | `0x00462dd0` | 115 | `c8f64c9a5bb99a15c9fd054d6015eedb6c7371d039f1330500516ba46552d689` |
| Mood script override | `0x00478cd0` | 91 | `9d7a851dc5b1036a3f556c7ee1b6c5cba311141fa9280df4e1cf34e3a5533952` |
| BNV vertex accessor | `0x0044ddf0` | 34 | `a40a0e0c0995e542818d89d910257640199b4cb2fdbc1244749625e526e7dc83` |
| BNV projection skew | `0x0044de20` | 40 | `a7d60d17fb6f7b4646a49560b4f7d92c498b4c1aaf4ee3eac423e5153c1de116` |

## Reproduce the byte checks

Run from the repository root, giving the original executable path as the sole argument. The script accepts only the stated image and rejects virtual-only data rather than accidentally reading another file region. It checks the exact published table and instruction ranges.

~~~python
from pathlib import Path
import sys, struct, hashlib
d = Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(d).hexdigest() == 'c50865b60bfcb26c0a7329a75fb772b10ae234af324f669901906e5abb0e2bd9'
pe = struct.unpack_from("<I", d, 60)[0]
base = struct.unpack_from("<I", d, pe+52)[0]
count = struct.unpack_from("<H", d, pe+6)[0]
opt = struct.unpack_from("<H", d, pe+20)[0]
sections = [struct.unpack_from("<4I", d, pe+24+opt+40*i+8) for i in range(count)]
def read(va, n):
    for virtual_size, rva, raw_size, raw_offset in sections:
        if base+rva <= va and va+n <= base+rva+raw_size:
            off = raw_offset+va-base-rva
            return d[off:off+n]
    raise AssertionError((va, n, "not file-backed"))
ranges = [('Power table', 4952896, 528, '910f016b26436d4aa7bbaf943eab8d9773e3cc32c1aaed346f6287f53bca2069'),
 ('Male first names', 4968140, 332, '2c73f7c41f4c19323d6bfd6aac77abca38c89ea1fa149f249182430f55530e4c'),
 ('Female first names', 4968472, 360, 'b6d05ed3bd06663f9d5a24e9c307f6f2fe41c3d7383fda1f66fb2db0bbcb488c'),
 ('Surnames', 4968832, 428, '45eb5cfc7371b4daa60604eeda546bf663351eceaa5b3fb5803d682e77ded161'),
 ('Message cooldowns', 4958432, 204, 'ab13018bb2cbcd0e5ba649cf11cf948ce9b515ae388a48b521ab39b432148c06'),
 ('Nearby-cell scan', 4980520, 96, '8cb65625c9ad5238b3fcf923935a8a32dbb9532f5181bd5e45677a8d987d357a'),
 ('High-level AI dispatch', 4948840, 104, 'bc260d183c96d76ce61ec1cdd006779462f3dd6921a9afdf324bdc43bf8e94de'),
 ('Low-level AI dispatch', 4969292, 64, '03820b747995544c325d2932a04b1153062a532f51f8f181e7aad0f8460ae46e'),
 ('State7', 4736464, 94, '2b07778f4364372c9a4975297e72d4faa57f2569144693139bc5e0e9275ecbb3'),
 ('Arrival radius', 4735392, 51, '39efaafbf13f73b39ea241148a5fc2900687dc85b80c03a7c12b7b77093420d7'),
 ('Restore low-level state', 4731456, 22, '086138c9c9e1fd4f4d18ede510c318fe4631c7ae86da7ef63b3410f97fc66c9d'),
 ('Script-string writer', 4638240, 96, 'c8811eceab1ef0aa031661e74ae72fb8737aca7e9ca572f29ef8e8b14e3c0169'),
 ('ODF finalizer', 4721312, 149, 'fd07646800da52a62b982958c37d3f891c92a8c2e1266fc3de9c9073a7cbc07c'),
 ('Small station add', 4532944, 69, 'ae42595e6b2ce335949386e0ddaf0a36d73ec9c40656aa507a09d30223e69ea9'),
 ('Crystal station add', 4533024, 69, 'a320cf317df71df82486a1355b02c61aaa78f43113d5b019d016b0ff2b78257f'),
 ('Mood reset', 4599248, 115, 'c8f64c9a5bb99a15c9fd054d6015eedb6c7371d039f1330500516ba46552d689'),
 ('Mood script override', 4689104, 91, '9d7a851dc5b1036a3f556c7ee1b6c5cba311141fa9280df4e1cf34e3a5533952')]
ranges += [('BNV vertex accessor', 4513264, 34, 'a40a0e0c0995e542818d89d910257640199b4cb2fdbc1244749625e526e7dc83'), ('BNV projection skew', 4513312, 40, 'a7d60d17fb6f7b4646a49560b4f7d92c498b4c1aaf4ee3eac423e5153c1de116')]
for name, va, n, expected in ranges:
    assert hashlib.sha256(read(va,n)).hexdigest() == expected, name

# Compare the literal published table values with their original bytes.
doc = Path('docs/runtime/core-data.md').read_text()
def table(title):
    section = doc.split('## '+title+'\n',1)[1].split('\n## ',1)[0]
    return [[cell.strip().strip('`') for cell in line.split('|')[1:-1]]
            for line in section.splitlines() if line.startswith('| ')][2:]
def u32(va):
    return struct.unpack('<I',read(va,4))[0]
def cstring(va):
    data = bytearray()
    while read(va+len(data),1) != b'\0':
        data += read(va+len(data),1)
    return data.decode('cp1252')
for title, va, n in [('Male first names',0x4bcecc,83),('Female first names',0x4bd018,90),('Surnames',0x4bd180,107)]:
    assert table(title) == [[str(i),cstring(u32(va+4*i))] for i in range(n)], title
power = table('Power table')
assert len(power) == 66
for i,row in enumerate(power[:65]):
    va = 0x4b9340+8*i
    assert row == [str(i),cstring(u32(va)),str(struct.unpack('<i',read(va+4,4))[0])], row
assert u32(0x4b9340+65*8) == 0x4d8bb0 and u32(0x4b9340+65*8+4) == 0
for title,va,n,width in [('Message cooldowns',0x4ba8e0,17,3),('Nearby-cell scan',0x4bff28,12,2)]:
    assert table(title) == [[str(i),*map(str,struct.unpack('<'+str(width)+'i',read(va+i*width*4,width*4)))] for i in range(n)], title
for title,va,n in [('High-level AI dispatch',0x4b8368,26),('Low-level AI dispatch',0x4bd34c,16)]:
    assert table(title) == [[str(i),f'0x{u32(va+i*4):08x}'] for i in range(n)], title
print(f"PASS: {len(ranges)} original ranges and all published core tables")
~~~

## Character asset manifest

Archive `Legoland.res`:16,424,086 bytes; SHA-256`b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2`. Members below are taken from its directory with the full-path tree traversal below. The older [leaf parser](../../tools/leveldata.py) is useful for unique filenames but collapses repeated leaf names. They are external input data, with the loader contract and exact shipped variants documented separately from executable constants.

| Kind / sex | Animation index | Member | Frames | Vertices/frame | Normals/frame | Faces / Gouraud | File offset | Size | SHA-256 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1/male | 0 | `3DData/New/visitor/ManSit.mansit.3d` | 1 | [56] | [202] | 74 / 64 | `0x434c80` | 6668 | `af02b45622a7374d2ac836d73629b00d52543c4c5b884f1408637a62774cb31e` |
| 1/male | 1 | `3DData/New/visitor/ManWalk.manwalk.3d` | 8 | [56] | [202] | 74 / 64 | `0x42c388` | 28396 | `64141a7253861b82aaf788fc934bf5b90c758eee52047c502bf7014d8ef44e5c` |
| 1/male | 2 | `3DData/New/visitor/ManWave.manwave.3d` | 8 | [56] | [202] | 74 / 64 | `0x42549c` | 28396 | `cd3db94440b50755c8027d41597174923677262a17b60f0d1825e5c9b43dcc66` |
| 1/male | 3 | `3DData/New/visitor/ManStand.manstand.3d` | 1 | [56] | [202] | 74 / 64 | `0x433274` | 6668 | `d3a403228663f03ad3e930f54a085040378ec6e73f7265fcd2c5baea7d6bd9c0` |
| 1/male | 4 | `3DData/New/visitor/ManPan.manpan.3d` | 8 | [66] | [268] | 100 / 84 | `0x43f770` | 36940 | `5c1965873e20f7c08b56be6321f6816e3c6277f920a8dcc049b4a14176d1cf35` |
| 1/male | 5 | `3DData/New/visitor/ManPanWalk.manpanwalk.3d` | 8 | [66] | [268] | 100 / 84 | `0x436724` | 36940 | `1d158131b99f829d5d1250ead20e046e38bced568868515332852783edc119d9` |
| 1/female | 0 | `3DData/New/visitor/WomanSit.womansit.3d` | 1 | [66] | [213] | 79 / 67 | `0x406e18` | 7160 | `7391ca2b0fb5984c8ffc76b9dc1367b3a62b9b4e41c0bd2c48f937acb3ea96e3` |
| 1/female | 1 | `3DData/New/visitor/WomanWalk.womanwalk.3d` | 8 | [66] | [213] | 79 / 67 | `0x3fda64` | 30652 | `ebcf14984d623ebabba13ec39ad82e28f9227130b768bd929b65bc7f3ee930c3` |
| 1/female | 2 | `3DData/New/visitor/WomanWave.womanwave.3d` | 8 | [66] | [213] | 79 / 67 | `0x3f62a8` | 30652 | `331c8e83359614a942c89629bac62f5845c45f90cfd49663744a0dd7258e4d26` |
| 1/female | 3 | `3DData/New/visitor/WomanStand.womanstand.3d` | 1 | [66] | [213] | 79 / 67 | `0x405220` | 7160 | `ad30857ff828da1813f803ecb7a7b7f8917ed022476d4d72bcbd57fc6974c085` |
| 1/female | 4 | `3DData/New/visitor/WomanPan.womanpan.3d` | 8 | [76] | [279] | 105 / 87 | `0x41232c` | 39196 | `ecfa6fdcb839c4951f4ed9028427e31837736281abce6ddf848cafc10f81ed60` |
| 1/female | 5 | `3DData/New/visitor/WomanPanWalk.wompanwalk.3d` | 8 | [76] | [279] | 105 / 87 | `0x408a10` | 39196 | `c8184d687eb9463d3d3c0752e5b1f51de007d6c4b70a561f803e843c696e8227` |
| 2/Geoff | 0 | `3DData/New/geoff/GeofWalk.geofWalk.3d` | 8 | [84] | [316] | 112 / 102 | `0x370558` | 43852 | `cb45dce5c45f909a0b557be6d773eb788ca05e940580a5cf24818bdde8a9f26c` |
| 2/Geoff | 1 | `3DData/New/geoff/GeofPour.geofpour.3d` | 48 | [84] | [316] | 112 / 102 | `0x37b0a4` | 236172 | `cc131e0ca4307dc3c6b8bafa06046f731096c38dbdc6a762e844b8cdb98ba7ab` |
| 3/Tracy | 0 | `3DData/New/Tracy/TracyWalk.TraceWalk.3d` | 8 | [90] | [249] | 91 / 79 | `0x448934` | 36988 | `02af75f5bd157ae2a7072bd45b0cc288e4a28de3583826a6f35531407de7d684` |

All15 initialized animations satisfy the complete morph layout: per-frame vertex/normal clouds, shared index triples and36-byte material records consume the member exactly, all vertex indices are in range, and normal counts equal`3*Gouraud + flat faces`. Global gaps do not add extra animations. Visitor slots are sit,walk,wave,stand,pan,pan-walk; Geoff slots are walk,pour and Tracy has walk. [Initialization](../../LEGOLAND/data2.c), [consumer](../../LEGOLAND/person3d.c)

## LOC records and outfit patches

The four shipped LOC members establish a52-byte fixed header: texture count`+0`, runtime texture-ID base`+4` (written by InitMan), patch count`+8`,32-byte NUL-padded stem`+c..2b`, offsets`+2c/+30` for texture records and patches. The first table has eight bytes per texture; all its shipped bytes arezero and this character-loader path does not use it after relocation. The second has six-byte `{i16 texture,u8 x,y,w,h}` patches; its count is`+8`. Both offsets relocate unconditionally, including zero. A format field of32 bytes does **not** imply the unchecked `%s` consumer enforces a32-byte limit.

| Member | Textures | Patches | Stem | Texture offset | Patch offset | File offset / size | SHA-256 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `3DData/New/visitor/NewProject.loc` | 4 | 8 | `NewProject` | 52 | 84 | `0x3f6224` / 132 | `13a22a69427619b532ffa4cf9d04c8b505133acc44b51d116424d5a4027a1c20` |
| `3DData/New/geoff/Geoff.loc` | 2 | 3 | `Geoff` | 52 | 68 | `0x3b4d2c` / 86 | `e6bb7e66c0e2847a9a9b3ba57e269c57e93c8d97a4d90f0153863a772ea2df4e` |
| `3DData/New/Tracy/Tracy.loc` | 2 | 3 | `Tracy` | 52 | 68 | `0x4487bc` / 86 | `b4923e991ba1184e83de887f82c6bb4987e22b03dd5ce041dcc8e53f571267e4` |
| `3DData/man.loc` | 9 | 28 | `man` | 52 | 124 | `0x356b88` / 292 | `9c253a57756dc1a992d69512b79dcb5a53628e16180fe8950af3208ba6aecee3` |

`NewProject.loc`,Geoff and Tracy are the active InitMan selections. `man.loc` is an additional shipped member; its presence does not make it the active visitor model. [Load/relocation](../../LEGOLAND/data3.c), [patch consumer](../../LEGOLAND/anim2.c)

| Member | Index | Texture | x | y | w | h |
| --- | --- | --- | --- | --- | --- | --- |
| `3DData/New/visitor/NewProject.loc` | 0 | 0 | 0 | 0 | 177 | 148 |
| `3DData/New/visitor/NewProject.loc` | 1 | 1 | 0 | 0 | 177 | 148 |
| `3DData/New/visitor/NewProject.loc` | 2 | 2 | 0 | 0 | 177 | 148 |
| `3DData/New/visitor/NewProject.loc` | 3 | 3 | 0 | 0 | 177 | 148 |
| `3DData/New/visitor/NewProject.loc` | 4 | 0 | 0 | 149 | 118 | 97 |
| `3DData/New/visitor/NewProject.loc` | 5 | 0 | 119 | 149 | 118 | 97 |
| `3DData/New/visitor/NewProject.loc` | 6 | 1 | 0 | 149 | 95 | 93 |
| `3DData/New/visitor/NewProject.loc` | 7 | 0 | 178 | 0 | 56 | 56 |
| `3DData/New/geoff/Geoff.loc` | 0 | 0 | 0 | 0 | 255 | 255 |
| `3DData/New/geoff/Geoff.loc` | 1 | 1 | 0 | 0 | 63 | 52 |
| `3DData/New/geoff/Geoff.loc` | 2 | 1 | 0 | 53 | 56 | 56 |
| `3DData/New/Tracy/Tracy.loc` | 0 | 0 | 0 | 0 | 177 | 148 |
| `3DData/New/Tracy/Tracy.loc` | 1 | 0 | 0 | 149 | 56 | 56 |
| `3DData/New/Tracy/Tracy.loc` | 2 | 1 | 0 | 0 | 127 | 127 |
| `3DData/man.loc` | 0 | 0 | 0 | 0 | 177 | 148 |
| `3DData/man.loc` | 1 | 1 | 0 | 0 | 177 | 148 |
| `3DData/man.loc` | 2 | 2 | 0 | 0 | 177 | 148 |
| `3DData/man.loc` | 3 | 3 | 0 | 0 | 177 | 148 |
| `3DData/man.loc` | 4 | 0 | 0 | 149 | 118 | 97 |
| `3DData/man.loc` | 5 | 0 | 119 | 149 | 118 | 97 |
| `3DData/man.loc` | 6 | 1 | 0 | 149 | 118 | 97 |
| `3DData/man.loc` | 7 | 1 | 119 | 149 | 118 | 97 |
| `3DData/man.loc` | 8 | 2 | 0 | 149 | 118 | 97 |
| `3DData/man.loc` | 9 | 2 | 119 | 149 | 118 | 97 |
| `3DData/man.loc` | 10 | 3 | 0 | 149 | 118 | 97 |
| `3DData/man.loc` | 11 | 3 | 119 | 149 | 118 | 97 |
| `3DData/man.loc` | 12 | 4 | 0 | 0 | 118 | 97 |
| `3DData/man.loc` | 13 | 4 | 0 | 98 | 118 | 97 |
| `3DData/man.loc` | 14 | 4 | 119 | 0 | 118 | 97 |
| `3DData/man.loc` | 15 | 4 | 119 | 98 | 118 | 97 |
| `3DData/man.loc` | 16 | 5 | 0 | 0 | 118 | 97 |
| `3DData/man.loc` | 17 | 5 | 0 | 98 | 118 | 97 |
| `3DData/man.loc` | 18 | 5 | 119 | 0 | 118 | 97 |
| `3DData/man.loc` | 19 | 5 | 119 | 98 | 118 | 97 |
| `3DData/man.loc` | 20 | 6 | 0 | 0 | 118 | 97 |
| `3DData/man.loc` | 21 | 6 | 0 | 98 | 118 | 97 |
| `3DData/man.loc` | 22 | 6 | 119 | 0 | 118 | 97 |
| `3DData/man.loc` | 23 | 6 | 119 | 98 | 118 | 97 |
| `3DData/man.loc` | 24 | 7 | 0 | 0 | 118 | 97 |
| `3DData/man.loc` | 25 | 7 | 0 | 98 | 118 | 97 |
| `3DData/man.loc` | 26 | 0 | 178 | 0 | 5 | 6 |
| `3DData/man.loc` | 27 | 8 | 0 | 0 | 5 | 6 |

## BNV vertex and person field closure

A BNV name node starts with a signed vertex count. `GetVertex` (`0x44ddf0..0x44de11`) rejects a null node or index>=count, then returns`vertices+20*index`; negative indices are unchecked. The20-byte vertex contains projected signed-short x/y at0/2, the float depth used for eight-corner averaging at4, and further authored scalars at8/c/10. The tail is not entirely unused: `GetZSkew` (`0x44de20..0x44de47`) reads floats c/10 together with bin float14, computing `v10² / ((2*bin14−1)*v10 − v0c*bin14)`. `NewBNVPath` and `SetBlokePositionFromBNV` pass vertex0 to this helper. The scalar at8 is retained as source data but has no read in these recovered path/projection consumers; no independent gameplay rule is assigned to it. [math3d.c](../../LEGOLAND/math3d.c), [bnvmove.c](../../LEGOLAND/bnvmove.c), [bnvpath.c](../../LEGOLAND/bnvpath.c)

Person3D scale is the float triple at`+10/+14/+18`;`+40/+44/+48` is the Euler rotation triple in radians. Earlier specification labels calling the latter scale and the former position were wrong. `+24/+28` are source-window x/y offsets passed with the optional sprite pointer`+2c` to `RenderZBufferObject`;`+30` is the ride-ownership flag written by ride positioning/reset. `+34` is the high-byte depth key,`+38` the vertical depth weight and`+3c` extra depth gain when the sprite is present. Frame3D allocates0x38 bytes but leaves its last12 unwritten; Anim3D allocates0x24 bytes and zeroes the unused24-byte tail. These bytes are explicitly reserved storage, not missing model transformations. [person3d.c](../../LEGOLAND/person3d.c), [tri3d.c](../../LEGOLAND/tri3d.c), [math3d.c](../../LEGOLAND/math3d.c), [mechrides.c](../../LEGOLAND/mechrides.c)

The save layouts preserve specified raw words even where a field is scratch, reserved, or a stale pointer. Reproducing their byte position and load-time reconstruction is the compatibility contract; inventing a physiological or model-space name for a scratch word would weaken it. The [persistence maps](persistence.md) enumerate every byte group and identify uninitialized padding.

## Position streams

`LoadPos` consumes two dwords: records per stream, then stream count; each stream contains that many48-byte records. Each record has three raw dwords followed by nine matrix floats. Copters explicitly loads the shipped`copters.pos` with32 records ×5 streams, and its placement consumer reads record`+4` as float y, while`+8` is raw DWORD4 in all160 records. Thus a universal float-XYZ declaration would be false. The indexed matrix and consumer-specific raw fields are the full generic contract. See [Copters evidence](attractions-data.md) for exact bytes and instructions, and [LMS/LFM](transport-data.md) for the separate geometry formats. [loaders.c](../../LEGOLAND/loaders.c)

## Reproduce character asset checks

Run this snippet from the repository root with the original `Legoland.res` path as its argument. It checks each published member digest, complete morph stream boundaries, vertex indices, normal cardinality and LOC record/patch bounds.

~~~python
from pathlib import Path
import sys, struct, hashlib
d = Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(d).hexdigest() == 'b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2'
members = [('3DData/New/visitor/ManSit.mansit.3d',
  4410496,
  6668,
  'af02b45622a7374d2ac836d73629b00d52543c4c5b884f1408637a62774cb31e'),
 ('3DData/New/visitor/ManWalk.manwalk.3d',
  4375432,
  28396,
  '64141a7253861b82aaf788fc934bf5b90c758eee52047c502bf7014d8ef44e5c'),
 ('3DData/New/visitor/ManWave.manwave.3d',
  4347036,
  28396,
  'cd3db94440b50755c8027d41597174923677262a17b60f0d1825e5c9b43dcc66'),
 ('3DData/New/visitor/ManStand.manstand.3d',
  4403828,
  6668,
  'd3a403228663f03ad3e930f54a085040378ec6e73f7265fcd2c5baea7d6bd9c0'),
 ('3DData/New/visitor/ManPan.manpan.3d',
  4454256,
  36940,
  '5c1965873e20f7c08b56be6321f6816e3c6277f920a8dcc049b4a14176d1cf35'),
 ('3DData/New/visitor/ManPanWalk.manpanwalk.3d',
  4417316,
  36940,
  '1d158131b99f829d5d1250ead20e046e38bced568868515332852783edc119d9'),
 ('3DData/New/visitor/WomanSit.womansit.3d',
  4222488,
  7160,
  '7391ca2b0fb5984c8ffc76b9dc1367b3a62b9b4e41c0bd2c48f937acb3ea96e3'),
 ('3DData/New/visitor/WomanWalk.womanwalk.3d',
  4184676,
  30652,
  'ebcf14984d623ebabba13ec39ad82e28f9227130b768bd929b65bc7f3ee930c3'),
 ('3DData/New/visitor/WomanWave.womanwave.3d',
  4154024,
  30652,
  '331c8e83359614a942c89629bac62f5845c45f90cfd49663744a0dd7258e4d26'),
 ('3DData/New/visitor/WomanStand.womanstand.3d',
  4215328,
  7160,
  'ad30857ff828da1813f803ecb7a7b7f8917ed022476d4d72bcbd57fc6974c085'),
 ('3DData/New/visitor/WomanPan.womanpan.3d',
  4268844,
  39196,
  'ecfa6fdcb839c4951f4ed9028427e31837736281abce6ddf848cafc10f81ed60'),
 ('3DData/New/visitor/WomanPanWalk.wompanwalk.3d',
  4229648,
  39196,
  'c8184d687eb9463d3d3c0752e5b1f51de007d6c4b70a561f803e843c696e8227'),
 ('3DData/New/geoff/GeofWalk.geofWalk.3d',
  3605848,
  43852,
  'cb45dce5c45f909a0b557be6d773eb788ca05e940580a5cf24818bdde8a9f26c'),
 ('3DData/New/geoff/GeofPour.geofpour.3d',
  3649700,
  236172,
  'cc131e0ca4307dc3c6b8bafa06046f731096c38dbdc6a762e844b8cdb98ba7ab'),
 ('3DData/New/Tracy/TracyWalk.TraceWalk.3d',
  4491572,
  36988,
  '02af75f5bd157ae2a7072bd45b0cc288e4a28de3583826a6f35531407de7d684'),
 ('3DData/New/visitor/NewProject.loc',
  4153892,
  132,
  '13a22a69427619b532ffa4cf9d04c8b505133acc44b51d116424d5a4027a1c20'),
 ('3DData/New/geoff/Geoff.loc',
  3886380,
  86,
  'e6bb7e66c0e2847a9a9b3ba57e269c57e93c8d97a4d90f0153863a772ea2df4e'),
 ('3DData/New/Tracy/Tracy.loc',
  4491196,
  86,
  'b4923e991ba1184e83de887f82c6bb4987e22b03dd5ce041dcc8e53f571267e4'),
 ('3DData/man.loc', 3500936, 292, '9c253a57756dc1a992d69512b79dcb5a53628e16180fe8950af3208ba6aecee3')]
for name, off, size, expected in members:
    b = d[off:off+size]
    assert len(b) == size and hashlib.sha256(b).hexdigest() == expected, name
    if name.lower().endswith(".loc"):
        nt, ctx, np = struct.unpack_from("<3I", b)
        a, p = struct.unpack_from("<2I", b, 44)
        assert ctx == 0 and a == 52 and p == a+nt*8 and size == p+np*6, name
        assert set(b[a:p]) == {0} and b[43] == 0, name
        assert all(0 <= struct.unpack_from("<h", b, p+i*6)[0] < nt for i in range(np)), name
    else:
        frames = struct.unpack_from("<I", b)[0]
        p = 4; verts = []; normals = []
        for _ in range(frames):
            nv = struct.unpack_from("<I", b, p)[0]; p += 4+nv*12; verts.append(nv)
            nn = struct.unpack_from("<I", b, p)[0]; p += 4+nn*12; normals.append(nn)
        faces, smooth = struct.unpack_from("<2I", b, p); p += 8
        indices = struct.unpack_from("<"+str(faces*3)+"I", b, p); p += faces*12+faces*36
        assert smooth <= faces and p == size, name
        assert all(max(indices) < n for n in verts), name
        assert all(n == faces+smooth*2 for n in normals), name
print(f"PASS: {len(members)} character assets")
~~~

## Resource directory paths

The original directory is a linked tree, not just a bag of leaf names. Each node begins with five little-endian dwords `{child offset,next sibling offset,folder flag,size,data offset}`, followed by a NUL name. Tree offsets are relative to the directory base and `0xffffffff` terminates child/sibling links. Folder nodes have flag1; file nodes flag0 and no children. File payload offsets are absolute within the archive. The shipped Legoland archive traverses610 nodes:15 folders and595 files; fourteen pairs of paths share offsets, giving581 distinct physical offsets. A dictionary keyed only by leaf filename reduces this to580 entries and loses path distinctions.

This matters for `3DData/manpan.manpan.3d` versus `3DData/New/visitor/ManPan.manpan.3d`: the obsolete root model is265,900 bytes, while the active InitMan model is36,940 bytes. The manifest above selects the explicit `3DData/New/<kind>/` path constructed by InitMan. [Path construction](../../LEGOLAND/data2.c), [resource lookup](../../LEGOLAND/res.c)

Run with the original `Legoland.res` path. Every link must be unique and in the directory, every member must fit before it, and every full path must be unique.

~~~python
from pathlib import Path
import struct, hashlib, sys
b = Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(b).hexdigest() == "b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2"
base = struct.unpack_from("<I", b)[0]
seen = set(); files = {}
def walk(off, parent):
    while off != 0xffffffff:
        assert off not in seen and base+off+20 <= len(b), (off,parent)
        seen.add(off)
        p = base+off
        child, nxt, folder, size, data = struct.unpack_from("<5I", b, p)
        end = b.index(b"\0", p+20)
        name = b[p+20:end].decode("ascii")
        full = parent+"/"+name if parent else name
        assert folder in (0,1) and name
        if folder:
            walk(child,full)
        else:
            assert child == 0xffffffff and 4 <= data and data+size <= base
            assert full not in files
            files[full] = (size,data)
        off = nxt
walk(0, "")
assert len(seen) == 610 and len(files) == 595
assert len({off for size,off in files.values()}) == 581
assert files["3DData/New/visitor/ManPan.manpan.3d"] == (36940,4454256)
assert files["3DData/manpan.manpan.3d"] == (265900,3041700)
print("PASS: full resource directory, 595 files / 610 nodes")
~~~

## Active outfit lookup tables

`LoadAltTextures` (`0x442980..0x442c6f`) supplies the remaining live character tables. It parses the packed face/chest name lists, allocates one ordinal array for each count, then scans `NewProject.txt` after its two header lines. Case-insensitive basename matches, stopping at the dot, map those names to the zero-based row ordinal used by the LOC patch table. The numeric rectangle text is read but does not determine that ordinal. All requested shipped names resolve. [Initializer](../../LEGOLAND/data2.c), [ordinal consumer](../../LEGOLAND/savechunks.c), [patch consumer](../../LEGOLAND/anim2.c)

| Sex | Face ordinals | Chest ordinals | Default face / chest |
| --- | --- | --- | --- |
| Male | 4,5 | 0,2,3 | 4 / 0 |
| Female | 4 | 1,2,3 | 4 / 1 |

The arrays are allocated without zeroing; malformed lists or absent names do not acquire a safe default. The three pinned resource members below are sufficient to reconstruct all shipped mappings, including the source texture names.

~~~python
from pathlib import Path
import hashlib, struct, sys
b = Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(b).hexdigest() == 'b8cd7ee4a98c7da31e0f8aeb7495717a8ea320a73aa98a515b62ab13055627e2'
members = [('3DData/New/visitor/NewProject.txt', 4153032, 598, '9899906581b8a1b18f6e7d193313b4983f0536e7298e4e79ebeaa30b387477dd'), ('3DData/New/visitor/altman.txt', 4417236, 80, '2f955f89f023bb374e20029cb305a663f4e9e4a0a60add529491a5a2a3857bb9'), ('3DData/New/visitor/altwoman.txt', 4417164, 69, 'b3f5c1b4625788024f169bb96b79fc8f441565e923f3230c7f14700d53e44198')]
raw = {}
for name,off,n,h in members:
    raw[name] = b[off:off+n]
    assert hashlib.sha256(raw[name]).hexdigest() == h
base = members[0][0]
names = [line.split(b'.',1)[0].strip().decode().casefold()
         for line in raw[base].splitlines()[2:] if line.strip()]
assert len(names) == 8
for name,expected in [(members[1][0],([4,5],[0,2,3],4,0)),
                      (members[2][0],([4],[1,2,3],4,1))]:
    data = raw[name]; p = 0; groups = []; defaults = []
    for _ in range(2):
        e = data.index(b'\0',p); title = data[p:e].decode().casefold(); p = e+1
        n = struct.unpack_from('<I',data,p)[0]; p += 4
        group = []
        for i in range(n):
            e = data.index(b'\0',p); label = data[p:e].decode().casefold(); p = e+1
            group.append(names.index(label))
        assert data[p] == 0; p += 1
        groups.append(group); defaults.append(names.index(title))
    assert p == len(data) and (*groups,*defaults) == expected
print('PASS: three outfit-list assets and every active face/chest ordinal')
~~~
