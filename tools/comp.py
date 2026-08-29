#!/usr/bin/env python3
"""Decode a LEGOLAND `COMP` sprite block to RGBA pixels and write a PNG.

Reverse-engineered from the game's own software blitter in legoland.exe:
  __BMPLoader @ 0x0044e010 loads the raw COMP block and tags the image type
  (16 -> type 3).  The type-3 software decode/blit is dispatched from 0x00466770
  into the per-row decoders 0x004673f0 / 0x00467d10 (identical opcode logic).
  This module reimplements that opcode loop exactly.

COMP block header (little-endian):
  +0x00  char[4] "COMP"
  +0x04  u32 width
  +0x08  u32 height
  +0x0c  u32 bpp        (16 = RGB555; 8 = paletted, see below)
  +0x10  u32 count/flags (1)
  +0x14  u32 reserved   (0; bit0 selects blitter group, same pixel result)
  +0x18  u32 s0         = block_size - 24  (payload length from +0x18)
  +0x1c  u32 s1         = pixel-word count
  +0x20  u32 s2         = length-stream byte count
  +0x24  u32 s3         = control-opcode count
  +0x28  ...            payload

Payload sub-streams (in order, from +0x28):
  pixels : 2*s1 bytes  -> RGB555 literal pixels (16-bit LE words)
  length : s2   bytes  -> run-length bytes
  control: remainder   -> 2-bit opcodes, 16 per u32, LSB-first; s3 opcodes total
                          (control byte length == ceil(s3/16)*4)

Control opcode (2 bits, lo = even bit, hi = odd bit of the current u32):
  hi=0            -> emit 1 literal pixel  (next word)         [codes 00 and 01]
  hi=1, lo=0      -> emit 1 transparent pixel                  [code 10]
  hi=1, lo=1      -> escape: read length byte L                [code 11]
        L == 0        -> end of scanline (advance to next row)
        L  > 0        -> read a sub-opcode (2 bits):
              sub hi=1        -> L transparent pixels
              sub 00          -> L literal pixels (copy run)
              sub 01          -> L copies of one word (fill run)

Transparency is opcode-driven: pixels never written are transparent (alpha 0).
The COMP codec has no per-pixel colour key, so value 0 is a real black pixel.
"""
import struct
import sys
import zlib


# ---------------------------------------------------------------------------
# RGB555 -> RGB888.  555 layout: bit15 unused, R=14..10, G=9..5, B=4..0.
def rgb555_to_rgb(v):
    r = (v >> 10) & 0x1f
    g = (v >> 5) & 0x1f
    b = v & 0x1f
    return ((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2))


class _Bits2:
    """2-bit opcode reader: 16 codes per u32, LSB-first, matching the asm's
    rotating mask (mask starts at 3, `rol 2` each step, advance ptr on wrap)."""
    __slots__ = ("d", "ptr", "mask")

    def __init__(self, data, ptr):
        self.d = data
        self.ptr = ptr
        self.mask = 3

    def next(self):
        word = struct.unpack_from("<I", self.d, self.ptr)[0]
        code = word & self.mask
        hi = 1 if (code & 0xAAAAAAAA) else 0
        lo = 1 if (code & 0x55555555) else 0
        m = ((self.mask << 2) | (self.mask >> 30)) & 0xFFFFFFFF
        self.mask = m
        if m & 1:
            self.ptr += 4
        return hi, lo


def parse_header(data, off=0):
    if data[off:off + 4] != b"COMP":
        raise ValueError("not a COMP block at offset %d" % off)
    w, h, bpp, cnt, res, s0, s1, s2, s3 = struct.unpack_from("<9I", data, off + 4)
    return {
        "width": w, "height": h, "bpp": bpp, "count": cnt, "reserved": res,
        "s0": s0, "s1": s1, "s2": s2, "s3": s3,
        "block_end": off + 24 + s0,
    }


def decode(data, off=0):
    """Decode a COMP block -> dict(width, height, rgba(bytes RGBA), header)."""
    hd = parse_header(data, off)
    w, h, bpp = hd["width"], hd["height"], hd["bpp"]
    if bpp == 16:
        return _decode16(data, off, hd)
    if bpp == 8:
        return _decode8(data, off, hd)
    raise ValueError("unsupported bpp %d" % bpp)


def _decode16(data, off, hd):
    w, h = hd["width"], hd["height"]
    s1, s2 = hd["s1"], hd["s2"]
    base = off + 0x28
    pix = base                 # 2*s1 bytes
    lenp = base + 2 * s1       # s2 bytes
    ctrl = base + 2 * s1 + s2  # remainder

    bits = _Bits2(data, ctrl)
    rgba = bytearray(w * h * 4)     # zero-filled => transparent
    x = 0
    y = 0
    npx = w * h
    guard = 0
    limit = npx * 4 + 4096

    def put(word):
        idx = (y * w + x) * 4
        r, g, b = rgb555_to_rgb(word)
        rgba[idx] = r
        rgba[idx + 1] = g
        rgba[idx + 2] = b
        rgba[idx + 3] = 255

    while y < h:
        guard += 1
        if guard > limit:
            break
        hi, lo = bits.next()
        if hi == 0:                         # literal 1 (codes 00 / 01)
            if 0 <= x < w and y < h:
                put(struct.unpack_from("<H", data, pix)[0])
            pix += 2
            x += 1
        elif lo == 0:                       # code 10: transparent 1
            x += 1
        else:                               # code 11: escape
            L = data[lenp]
            lenp += 1
            if L == 0:                      # end of scanline
                y += 1
                x = 0
            else:
                shi, slo = bits.next()
                if shi == 1:                # transparent run
                    x += L
                elif slo == 0:              # literal run
                    for _ in range(L):
                        if 0 <= x < w and y < h:
                            put(struct.unpack_from("<H", data, pix)[0])
                        pix += 2
                        x += 1
                else:                       # fill run
                    word = struct.unpack_from("<H", data, pix)[0]
                    pix += 2
                    for _ in range(L):
                        if 0 <= x < w and y < h:
                            put(word)
                        x += 1

    return {"width": w, "height": h, "rgba": bytes(rgba), "header": hd,
            "consumed": {"pixel_bytes": pix - base,
                         "length_bytes": lenp - (base + 2 * s1),
                         "control_bytes": bits.ptr - ctrl}}


def _decode8(data, off, hd):
    """bpp==8 paletted COMP.  Same opcode stream, but the pixel stream is a
    byte index stream and a 256-entry RGB555 palette (from an 888 768-byte
    table, 888->555) precedes it.

    NB: no bpp==8 COMP blocks exist in the shipped Graphics1.res, so this path
    is written from the loader disassembly (0x0044e28c builds the palette,
    888->555) but is not exercised by real data.  It mirrors the 16-bit loop
    with a palette indirection on the pixel stream.
    """
    w, h = hd["width"], hd["height"]
    s1, s2 = hd["s1"], hd["s2"]
    base = off + 0x28
    # 256 * 4-byte RGBQUAD palette (B,G,R,0) then index stream.
    pal_bytes = 256 * 4
    pal = []
    for i in range(256):
        b, g, r, _ = data[base + i * 4: base + i * 4 + 4]
        pal.append((r, g, b))
    pix = base + pal_bytes
    lenp = pix + s1            # s1 index bytes
    ctrl = lenp + s2
    bits = _Bits2(data, ctrl)
    rgba = bytearray(w * h * 4)
    x = y = 0

    def put(idx):
        o = (y * w + x) * 4
        r, g, b = pal[idx]
        rgba[o] = r
        rgba[o + 1] = g
        rgba[o + 2] = b
        rgba[o + 3] = 255

    while y < h:
        hi, lo = bits.next()
        if hi == 0:
            if 0 <= x < w and y < h:
                put(data[pix])
            pix += 1
            x += 1
        elif lo == 0:
            x += 1
        else:
            L = data[lenp]
            lenp += 1
            if L == 0:
                y += 1
                x = 0
            else:
                shi, slo = bits.next()
                if shi == 1:
                    x += L
                elif slo == 0:
                    for _ in range(L):
                        if 0 <= x < w and y < h:
                            put(data[pix])
                        pix += 1
                        x += 1
                else:
                    idx = data[pix]
                    pix += 1
                    for _ in range(L):
                        if 0 <= x < w and y < h:
                            put(idx)
                        x += 1
    return {"width": w, "height": h, "rgba": bytes(rgba), "header": hd}


# ---------------------------------------------------------------------------
def write_png(path, width, height, rgba):
    """Minimal RGBA PNG writer (stdlib zlib only)."""
    def chunk(typ, body):
        c = typ + body
        return (struct.pack(">I", len(body)) + c
                + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF))

    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)  # filter: none
        raw += rgba[y * stride:(y + 1) * stride]
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    open(path, "wb").write(png)


def main():
    if len(sys.argv) < 3:
        print("usage: comp.py <block.comp|archive.res> <out.png> [name|index]")
        raise SystemExit(1)
    src, out = sys.argv[1], sys.argv[2]
    data = open(src, "rb").read()
    off = 0
    if data[:4] != b"COMP":
        # treat as .res archive; need a name/index
        from resfile import parse_leaves
        leaves = parse_leaves(data)
        key = sys.argv[3]
        comp = [r for r in leaves if r["is_comp"]]
        r = (comp[int(key)] if key.isdigit()
             else next(x for x in leaves if x["name"] == key))
        off = r["offset"]
    res = decode(data, off)
    write_png(out, res["width"], res["height"], res["rgba"])
    print(f"wrote {out}  {res['width']}x{res['height']}")


if __name__ == "__main__":
    main()
