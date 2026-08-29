#!/usr/bin/env python3
"""Extractor for the LEGOLAND disc's InstallShield 5.x Z archives.

The disc ships the game program, level data, DirectMusic content and the
character-animation AVIs inside `main.z` (with a companion `_setup.lib`).  Both
carry signature 0x8C655D13 and a directory of members at the tail of the file;
each member is compressed with PKWARE DCL implode (see blast.py).

Directory layout (reverse-engineered from the disc, verified by the members
packing contiguously and by every extracted member's magic matching its
extension):

    [42-byte meta for the first file]
    u8 name_len; char name[name_len];   <- described by the PRECEDING meta
    [42-byte meta for the NEXT file]
    u8 name_len; char name[name_len];
    ...

Each 42-byte metadata block describes the file whose NAME FOLLOWS it, so a
name at position P is described by the 42 bytes ending at P.  Within a meta
block:
        +16  u32  uncompressed (expanded) size
        +20  u32  stored (compressed) size
        +24  u32  offset of the DCL stream within the archive
        +28  u32  DOS date/time

(Getting this backwards pairs `legoland.exe`'s name with the next file's
data -- the tell is the 802 KB PE turning up under an AVI's name.)

Usage:
    iscab.py list   main.z
    iscab.py extract main.z -o outdir
"""
import argparse
import os
import struct
import sys

from blast import explode

SIG = 0x8C655D13


def parse_directory(data):
    """Return [(name, uncompressed, stored, offset), ...] for an IS Z archive."""
    if struct.unpack_from("<I", data, 0)[0] != SIG:
        raise ValueError("not an InstallShield Z archive (bad signature)")
    # The file/component directory lives near the tail.  Members store their
    # data from low offsets upward; the directory begins after the last stream.
    # Walk length-prefixed name records and keep those whose 42-byte metadata
    # yields an in-range (offset, stored) pair -- self-synchronising because
    # names are length-prefixed and the metadata size is fixed.
    entries = []
    n = len(data)
    pos = _find_dir_start(data)
    while pos < n - 1:
        L = data[pos]
        if not (1 <= L <= 60):
            break
        name = data[pos + 1:pos + 1 + L]
        if not all(32 <= b < 127 for b in name):
            break
        # This name is described by the 42-byte meta block ending at `pos`.
        meta_at = pos - 42
        if meta_at >= 0:
            usize, csize, off = struct.unpack_from("<III", data, meta_at + 16)
            if 0 < off < n and 0 < csize and off + csize <= n:
                entries.append((name.decode("latin1"), usize, csize, off))
        pos += 1 + L + 42
    return entries


def _find_dir_start(data):
    """Locate the first directory entry (the last member's stream ends here)."""
    # The directory is a run of [len][name][42] records.  Scan for the first
    # plausible one: a byte L in 1..40 followed by L printable chars and a '.'
    # whose metadata offset+size stays in range, appearing late in the file.
    n = len(data)
    scan = max(0, n - 200000)
    for pos in range(scan, n - 60):
        L = data[pos]
        if 5 <= L <= 40:
            name = data[pos + 1:pos + 1 + L]
            if all(32 <= b < 127 for b in name) and b"." in name and \
               (65 <= name[0] <= 90 or 97 <= name[0] <= 122 or name[0] in b"_0123456789"):
                usize, csize, off = struct.unpack_from(
                    "<III", data, pos + 1 + L + 16)
                if 0 < off < n and 0 < csize and off + csize <= n:
                    # confirm the NEXT record also parses -> real directory
                    p2 = pos + 1 + L + 42
                    if p2 < n and 1 <= data[p2] <= 40:
                        return pos
    raise ValueError("could not locate directory")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cmd", choices=["list", "extract"])
    ap.add_argument("archive")
    ap.add_argument("-o", "--out", default="extracted")
    args = ap.parse_args()

    data = open(args.archive, "rb").read()
    entries = parse_directory(data)

    if args.cmd == "list":
        print(f"{len(entries)} members in {args.archive}")
        total = 0
        for name, usize, csize, off in entries:
            total += usize
            print(f"  {name:28s} {usize:10d} <- {csize:10d}  @0x{off:08x}")
        print(f"total uncompressed: {total} bytes")
        return

    os.makedirs(args.out, exist_ok=True)
    ok = 0
    for name, usize, csize, off in entries:
        try:
            raw = explode(data, off, out_size=usize)
        except Exception as e:  # noqa: BLE001
            print(f"  FAIL {name}: {e}", file=sys.stderr)
            continue
        if len(raw) != usize:
            print(f"  WARN {name}: got {len(raw)} bytes, expected {usize}",
                  file=sys.stderr)
        safe = name.replace("\\", "_").replace("/", "_")
        with open(os.path.join(args.out, safe), "wb") as f:
            f.write(raw)
        ok += 1
    print(f"extracted {ok}/{len(entries)} members to {args.out}/")


if __name__ == "__main__":
    main()
