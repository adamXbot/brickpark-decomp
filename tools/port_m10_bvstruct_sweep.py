#!/usr/bin/env python3
"""Moved: PORT-M10's by-value-struct sweep is now `portable/tools/bvstruct_sweep.py`.

PORT-A9 promoted it into the portable tree with a `--selftest` (including the
emcc-vs-clang repro as a positive control) and two ctests, `bvstruct_sweep` and
`bvstruct_sweep_selftest`, so the class is gated instead of re-derived. The
promoted version also differs in two ways that change its answers:

* it reads the sources the way the PORTABLE BUILD does, so a fix (a scalar
  declaration in a `#else` arm of `#ifndef LEGOLAND_PORTABLE`) is not reported
  as a fresh hit -- this version reports all three of M10's own repairs;
* it sizes a typedef tree-wide, so `union BPosW { unsigned short w; BPos b; }`
  is the 2-byte, 2-member union it is. This version could not size it and filed
  all thirteen of those sites under "wasm-ld already warns", which it does not.

This alias forwards so an old command line still works; it will go when the
round checklist stops naming it.
"""
import os, runpy, sys

HERE = os.path.dirname(os.path.abspath(__file__))
NEW = os.path.join(HERE, '..', 'portable', 'tools', 'bvstruct_sweep.py')
print(f'port_m10_bvstruct_sweep.py has moved to portable/tools/bvstruct_sweep.py'
      f' -- running it', file=sys.stderr)
sys.argv[0] = NEW
runpy.run_path(NEW, run_name='__main__')
