#!/usr/bin/env python3
"""Decode a LEGOLAND `COMP` sprite block to RGBA pixels and write a PNG.

A COMP block is an .lls member of Graphics1.res / Graphics2.res: the LLS
animation record that __BMPLoader @ 0x0044e010 (LEGOLAND/screen.c) reads whole
and uses in place.  Its FORMAT word picks one of two frame encodings, and each
is reimplemented here from the game's own software blitters.

COMP block header (little-endian):
  +0x00  char[4] "COMP"
  +0x04  u32 width
  +0x08  u32 height
  +0x0c  u32 format     the LLS format word, not a bit depth:
                          0x10 = 16-bpp RLE frames      (ImageRec type 3)
                          8    = 8-bit paletted frames  (ImageRec type 2)
  +0x10  u16 count      animation frames
  +0x12  u16            0 in the file (the loader's frame timer)
  +0x14  u32 flags      bit 0: a BASE image comes first, so the frame table
                        holds count+1 records and the painters draw record 0
                        under record frame+1.  No other bit changes decoding.
  +0x18  frame table    length-linked records, each starting with its i32 size
                        (add it to reach the next record)

decode(data, off, frame) decodes ONE record, `frame` indexing the table; it
does not composite a base image.

Format 0x10: 16-bpp RLE (`_decode16`, record 0 only)
----------------------------------------------------
The type-3 software decode/blit is dispatched from 0x00466770 into the per-row
decoders 0x004673f0 / 0x00467d10 (identical opcode logic).  Record 0, with its
fields at their offsets from the block start:
  +0x18  u32 s0         = record size (block_size - 24 for a one-record block)
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

Format 8: 8-bit paletted frames (`_decode8`, any record)
--------------------------------------------------------
Read from SoftBlitAnimPlain @ 0x00464480 (LEGOLAND/softblit2.c) and its
recolouring sibling SoftBlitAnim @ 0x00465240 (LEGOLAND/softblit.c).  A record:
  +0x00  i32 size       add it to reach the next record
  +0x04  i32 npixels    length of the 8-bit INDEX block
  +0x08  u8[npixels]    palette indices
         RECORD 0 ONLY: a 0x200-byte palette, 256 u16 RGB555 entries that the
         later records reuse (LLS555To565 @ 0x0047d6a0 repacks it in place
         for a 565 display)
         then the u32 control stream, to the end of the record

Control stream: 2-bit codes, 16 per u32, LSB first.  The reader shifts the
word it holds right by 2 before each code and loads the next word once all 16
are used (`_Ctl8`, after ll_anim_code / ll_anim_count in
portable/hostwin/include/ll_portable.h).  Unlike format 0x10 there is no
separate length stream:
  bit1 = 0        -> one literal pixel (next index byte)   [codes 00 and 01]
  10              -> one transparent pixel
  11              -> an 8-bit COUNT follows in the control stream.  It fills 4
                     code slots: the low byte of the word just shifted when at
                     least 4 slots are left in it, else the low byte of a
                     freshly loaded word (the old word's remainder is dropped)
        count == 0    -> end of row
        count  > 0    -> a sub-code follows:
              bit1 = 1        -> skip `count` pixels
              00              -> copy `count` index bytes
              01              -> repeat ONE index byte `count` times
A record is `height` rows, each ended by a zero count, and it consumes exactly
its npixels index bytes and every control byte (`comp.py --check`).

Transparency is opcode-driven in both formats: pixels never written are
transparent (alpha 0).  Neither codec has a per-pixel colour key, so value 0
(or an index whose palette entry is 0) is a real black pixel.

    comp.py <block.comp|archive.res> <out.png> [name|index] [--frame N]
    comp.py --check <archive.res> [...]
"""
import argparse
import os
import struct
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


class _Ctl8:
    """The format-8 control reader: ll_anim_code / ll_anim_count, the C form
    of the reader both type-2 painters spell in asm (`ebx` the word being
    consumed, `ebp` the next word, g_zb_bits the codes left in `ebx`).  `end`
    bounds the stream, so a bad walk raises rather than reading the next
    record."""
    __slots__ = ("d", "ptr", "end", "cur", "bits")

    def __init__(self, data, ptr, end):
        self.d = data
        self.ptr = ptr
        self.end = end
        self.cur = 0
        self.bits = 0

    def _load(self):
        if self.ptr + 4 > self.end:
            raise ValueError("control stream overrun at 0x%x" % self.ptr)
        self.cur = struct.unpack_from("<I", self.d, self.ptr)[0]
        self.ptr += 4

    def code(self):
        self.cur >>= 2                  # the shift happens even on a reload
        self.bits -= 1
        if self.bits < 0:
            self.bits = 15
            self._load()
        return self.cur & 3

    def count(self):
        self.cur >>= 2
        self.bits -= 4                  # the count fills four code slots
        if self.bits < 0:               # fewer were left: take a fresh word
            self.bits = 12
            self._load()
        n = self.cur & 0xFF
        self.cur >>= 6
        return n


def parse_header(data, off=0):
    if data[off:off + 4] != b"COMP":
        raise ValueError("not a COMP block at offset %d" % off)
    w, h, fmt, cnt, _timer, flags, s0, s1, s2, s3 = struct.unpack_from(
        "<3IHHI4I", data, off + 4)
    return {
        "width": w, "height": h,
        "format": fmt, "bpp": fmt,      # "bpp": the format word's old name
        "count": cnt, "flags": flags,
        "records": cnt + (flags & 1),   # bit 0: a base image record first
        # record 0's header as format 0x10 lays it out
        "s0": s0, "s1": s1, "s2": s2, "s3": s3,
        "block_end": off + 24 + s0,     # the end of record 0
    }


def decode(data, off=0, frame=0):
    """Decode frame record `frame` of a COMP block -> dict(width, height,
    rgba (bytes RGBA), header, consumed).  The 16-bpp reader decodes record 0
    only."""
    hd = parse_header(data, off)
    fmt = hd["format"]
    if fmt == 16:
        if frame != 0:
            raise ValueError("the 16-bpp reader decodes record 0 only")
        return _decode16(data, off, hd)
    if fmt == 8:
        return _decode8(data, off, hd, frame)
    raise ValueError("unsupported format %d" % fmt)


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


def frames8(data, off=0, hd=None):
    """The frame table of a format-8 block: one dict per record, offsets
    absolute: {offset, size, npixels, index, palette, ctrl, end}.  `palette`
    is None except on record 0, whose palette sits between its index block
    and its control stream."""
    if hd is None:
        hd = parse_header(data, off)
    if hd["format"] != 8:
        raise ValueError("not a format-8 COMP block")
    recs = []
    p = off + 0x18
    for i in range(hd["records"]):
        if p + 8 > len(data):
            raise ValueError("frame record %d at 0x%x is past the data" % (i, p))
        size, npixels = struct.unpack_from("<ii", data, p)
        index = p + 8
        palette = index + npixels if i == 0 else None
        ctrl = index + npixels + (0x200 if i == 0 else 0)
        end = p + size
        if npixels < 0 or end < ctrl or (end - ctrl) % 4 or end > len(data):
            raise ValueError("frame record %d at 0x%x is malformed "
                             "(size %d, npixels %d)" % (i, p, size, npixels))
        recs.append({"offset": p, "size": size, "npixels": npixels,
                     "index": index, "palette": palette,
                     "ctrl": ctrl, "end": end})
        p = end
    return recs


def _decode8(data, off, hd, frame=0):
    """Format 8: frame record `frame`, coloured through record 0's palette."""
    w, h = hd["width"], hd["height"]
    recs = frames8(data, off, hd)
    if not 0 <= frame < len(recs):
        raise ValueError("frame %d is out of range: %d frame records"
                         % (frame, len(recs)))
    pal = struct.unpack_from("<256H", data, recs[0]["palette"])
    ink = [bytes(rgb555_to_rgb(v)) + b"\xff" for v in pal]   # index -> RGBA
    rec = recs[frame]
    ip = rec["index"]
    iend = ip + rec["npixels"]
    bits = _Ctl8(data, rec["ctrl"], rec["end"])
    rgba = bytearray(w * h * 4)     # zero-filled => transparent
    x = y = 0
    row = 0                         # rgba offset of row y

    while y < h:
        code = bits.code()
        if not code & 2:                    # one literal pixel (00 / 01)
            if ip >= iend:
                raise ValueError("index block overrun in row %d" % y)
            if x < w:
                o = row + 4 * x
                rgba[o:o + 4] = ink[data[ip]]
            ip += 1
            x += 1
        elif not code & 1:                  # 10: one transparent pixel
            x += 1
        else:                               # 11: a count follows
            n = bits.count()
            if n == 0:                      # end of row
                y += 1
                x = 0
                row += 4 * w
                continue
            sub = bits.code()
            if sub & 2:                     # skip n pixels
                x += n
                continue
            take = 1 if sub & 1 else n      # 01 repeats one byte, 00 copies n
            if ip + take > iend:
                raise ValueError("index block overrun in row %d" % y)
            vis = min(n, w - x)             # clipped at the right edge
            if vis > 0:
                o = row + 4 * x
                if sub & 1:
                    rgba[o:o + 4 * vis] = ink[data[ip]] * vis
                else:
                    rgba[o:o + 4 * vis] = b"".join(
                        [ink[c] for c in data[ip:ip + vis]])
            ip += take
            x += n

    return {"width": w, "height": h, "rgba": bytes(rgba), "header": hd,
            "frame": frame, "record": rec,
            "consumed": {"index_bytes": ip - rec["index"],
                         "control_bytes": bits.ptr - rec["ctrl"]}}


def check(paths):
    """Decode every frame record of every format-8 member of each archive and
    require it to consume exactly its npixels index bytes and its whole
    control stream over `height` rows, with the records filling the member
    exactly.  Prints counts only, never asset content.  Returns the number
    of failures."""
    import resfile
    failures = 0
    total = 0
    for path in paths:
        data = resfile.load(path)
        arc = os.path.basename(path)
        members = records = 0
        for leaf in resfile.parse_leaves(data):
            if not leaf["is_comp"]:
                continue
            off, name = leaf["offset"], leaf["name"]
            hd = parse_header(data, off)
            if hd["format"] != 8:
                continue
            members += 1
            try:
                recs = frames8(data, off, hd)
            except ValueError as e:
                print("FAIL %s %s: %s" % (arc, name, e))
                failures += 1
                continue
            end = recs[-1]["end"] if recs else off + 0x18
            if end != off + leaf["size"]:
                print("FAIL %s %s: the frame records end at +0x%x, the member "
                      "at +0x%x" % (arc, name, end - off, leaf["size"]))
                failures += 1
            for k, rec in enumerate(recs):
                records += 1
                try:
                    got = decode(data, off, k)["consumed"]
                except (ValueError, struct.error) as e:
                    print("FAIL %s %s frame %d: %s" % (arc, name, k, e))
                    failures += 1
                    continue
                want = (rec["npixels"], rec["end"] - rec["ctrl"])
                if (got["index_bytes"], got["control_bytes"]) != want:
                    print("FAIL %s %s frame %d: index %d of %d bytes, control "
                          "%d of %d bytes" % (arc, name, k, got["index_bytes"],
                                              want[0], got["control_bytes"],
                                              want[1]))
                    failures += 1
        print("comp check: %s: %d format-8 members, %d frame records"
              % (arc, members, records))
        total += members
    if not total:
        print("FAIL comp check: no format-8 member to check")
        failures += 1
    print("comp check: %d failure(s)" % failures)
    return failures


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
    ap = argparse.ArgumentParser(
        description="Decode one frame of a COMP block (an .lls member) to a "
                    "PNG, or check the format-8 reader against archives.")
    ap.add_argument("src", nargs="?", help="a raw COMP block or a .res archive")
    ap.add_argument("out", nargs="?", help="the PNG to write")
    ap.add_argument("member", nargs="?",
                    help="archive member: its name, or an index among the "
                         "COMP members")
    ap.add_argument("--frame", type=int, default=0,
                    help="frame record to decode (default 0; with flags bit 0 "
                         "set, record 0 is the base image)")
    ap.add_argument("--check", nargs="+", metavar="ARCHIVE",
                    help="decode every frame record of every format-8 member "
                         "and require exact stream consumption; prints counts "
                         "only")
    args = ap.parse_args()
    if args.check:
        raise SystemExit(1 if check(args.check) else 0)
    if args.out is None:
        ap.print_usage()
        raise SystemExit(1)
    data = open(args.src, "rb").read()
    off = 0
    if data[:4] != b"COMP":
        # treat as .res archive; need a name/index
        from resfile import parse_leaves
        if args.member is None:
            ap.error("an archive needs a member name or index")
        leaves = parse_leaves(data)
        key = args.member
        comp = [r for r in leaves if r["is_comp"]]
        r = (comp[int(key)] if key.isdigit()
             else next(x for x in leaves if x["name"] == key))
        off = r["offset"]
    try:
        res = decode(data, off, args.frame)
    except ValueError as e:
        ap.error(str(e))
    write_png(args.out, res["width"], res["height"], res["rgba"])
    hd = res["header"]
    note = ("  frame %d of %d" % (args.frame, hd["records"])
            if hd["format"] == 8 else "")
    print(f"wrote {args.out}  {res['width']}x{res['height']}{note}")


if __name__ == "__main__":
    main()
