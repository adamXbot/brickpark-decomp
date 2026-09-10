# Lane PORT-B2 — the first front-end frame and the first click

Brief: `docs/SCOPE_PORT_WAVE.md` "PORT-B2". Branch `scope/PORT-B2`, cut from
main `8e02a675`. Predecessor: `docs/lanes/scope-port-b.md` (read that first —
the main-loop decision in its §1 and the vtable slot table in its §3 are still
the contract).

Owned files: `portable/cmake/browser.cmake`, `portable/src/browser/**`,
`portable/src/hostwin/{ddraw,user32,gdi32,dinput,winmm,dsound}.c`,
`portable/src/hostwin/ll_font.c` (new), additions to
`portable/hostwin/include/ll_host.h`, this note, the PORT-B2 part of
`portable/README.md`. `LEGOLAND/*.c` is read-only for this lane; `gen_link.py`,
`kernel32.c` and `tests.cmake` belong to PORT-A2 and PORT-C and are untouched.

Where the page was when this lane opened: `legoland.html` runs the game's own
`WinMain` through `DirectDrawCreate`, the window and the CD check, then fails in
`RES_OpenVolume` because `g_volume_names` points into unnamed `.rdata` that
`gen_link.py --ilp32` does not re-point. That is PORT-A2's. This lane is
everything *after* the loader, written and tested ahead of it.

---

## 1. The host calls between "resources mounted" and the first front-end flip

(filled in below as each is checked against the shim)

## 2. Status

| deliverable | state |
| --- | --- |
| 1. front-end host-call census + fix every gap | |
| 2. node-safe JS library | |
| 3. DirectInput shapes verified against input.c/input2.c | |
| 4. page ergonomics | |
| 5. merge PORT-A2 and look at a frame | |
