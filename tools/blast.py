"""Clean-room PKWARE DCL "implode" decompressor (a.k.a. explode / blast).

The InstallShield 5.x archive files on the LEGOLAND disc (main.z, _setup.lib)
store every member compressed with the PKWARE Data Compression Library's
"implode" method (stream header byte0 = literal mode, byte1 = dictionary-size
exponent 4/5/6).  This is a re-implementation of the well-documented DCL
format in pure Python -- no game code, purely a data-unpacking utility.

Reference for the format: the PKWARE Data Compression Library APPNOTE and the
public-domain "blast" algorithm by Mark Adler.
"""

# Length code base values and extra-bit counts (16 length symbols).
_LEN_BASE = (3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264)
_LEN_EXTRA = (0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8)

# Compact bit-length tables (high nibble+1 = run, low nibble = bit length).
_LIT_REP = bytes((
    11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8,
    9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5,
    7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
    8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27,
    44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45,
    44, 173))
_LEN_REP = bytes((2, 35, 36, 53, 38, 23))
_DIST_REP = bytes((2, 20, 53, 230, 247, 151, 248))


class _Huffman:
    """Canonical Huffman decoder built from a compact bit-length table."""

    def __init__(self, rep):
        lengths = []
        for b in rep:
            run = (b >> 4) + 1
            blen = b & 0x0F
            lengths.extend([blen] * run)
        self.count = [0] * (max(lengths) + 1)
        for l in lengths:
            if l:
                self.count[l] += 1
        # symbols ordered by (bit length, symbol index) -- canonical order
        self.symbol = [s for _, s in sorted(
            (l, s) for s, l in enumerate(lengths) if l)]


class _BitReader:
    __slots__ = ("data", "pos", "bitbuf", "bitcnt")

    def __init__(self, data, pos):
        self.data = data
        self.pos = pos
        self.bitbuf = 0
        self.bitcnt = 0

    def bits(self, need):
        val = self.bitbuf
        cnt = self.bitcnt
        data = self.data
        pos = self.pos
        while cnt < need:
            val |= data[pos] << cnt
            pos += 1
            cnt += 8
        self.pos = pos
        self.bitbuf = val >> need
        self.bitcnt = cnt - need
        return val & ((1 << need) - 1)

    def decode(self, h):
        # DCL/blast reads the code bits inverted relative to canonical order.
        code = 0
        first = 0
        index = 0
        count = h.count
        for length in range(1, len(count)):
            code |= self.bits(1) ^ 1        # invert each bit
            c = count[length]
            if code - c < first:
                return h.symbol[index + (code - first)]
            index += c
            first = (first + c) << 1
            code <<= 1
        raise ValueError("bad DCL code")


_LIT = _Huffman(_LIT_REP)
_LEN = _Huffman(_LEN_REP)
_DIST = _Huffman(_DIST_REP)


def explode(data, pos=0, out_size=None):
    """Decompress a DCL-imploded stream starting at `data[pos]`.

    Returns the decompressed bytes.  If `out_size` is given, stops once that
    many bytes have been produced (the InstallShield directory records the
    exact expanded size for each member)."""
    lit_mode = data[pos]
    dict_bits = data[pos + 1]
    if lit_mode not in (0, 1) or dict_bits not in (4, 5, 6):
        raise ValueError(f"not a DCL stream (mode={lit_mode}, dict={dict_bits})")
    br = _BitReader(data, pos + 2)
    out = bytearray()
    append = out.append
    while True:
        if br.bits(1):                      # 1 -> length/distance pair
            sym = br.decode(_LEN)
            length = _LEN_BASE[sym] + (br.bits(_LEN_EXTRA[sym]) if _LEN_EXTRA[sym] else 0)
            if length == 519:               # end-of-stream marker
                break
            dsym = br.decode(_DIST)
            if length == 2:
                dist = (dsym << 2) + br.bits(2) + 1
            else:
                dist = (dsym << dict_bits) + br.bits(dict_bits) + 1
            start = len(out) - dist
            if start < 0:
                raise ValueError("distance too far back")
            for i in range(length):         # overlapping copy, byte by byte
                append(out[start + i])
        else:                               # 0 -> literal
            append(br.bits(8) if lit_mode == 0 else br.decode(_LIT))
        if out_size is not None and len(out) >= out_size:
            break
    return bytes(out)
