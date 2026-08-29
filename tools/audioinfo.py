#!/usr/bin/env python3
"""audioinfo.py -- catalog LEGOLAND audio/video/music assets.

Parses RIFF headers of WAV, AVI, and DirectMusic (.sty/.sgt/.bnd) files and
prints a per-file line plus summary tables. Pure header parsing; no decode.

Usage:
    python3 tools/audioinfo.py [DIR ...]

With no args, scans gamedata/main/ plus the disc Speech/ and root .avi if the
disc is mounted at /Volumes/LEGOLAND.
"""
import sys, os, glob, struct
from collections import Counter, defaultdict

WAVE_FMT = {0x0001: "PCM", 0x0002: "MS-ADPCM", 0x0011: "IMA-ADPCM",
            0x0055: "MP3", 0x0050: "MPEG"}


def u16(b, o): return struct.unpack_from("<H", b, o)[0]
def u32(b, o): return struct.unpack_from("<I", b, o)[0]


def riff_chunks(data, start, end):
    """Yield (fourcc, size, payload_off) for chunks in [start,end)."""
    o = start
    while o + 8 <= end:
        cid = data[o:o+4]
        sz = u32(data, o+4)
        yield cid, sz, o+8
        o += 8 + sz + (sz & 1)


def parse_wav(data):
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        return None
    info = {}
    for cid, sz, po in riff_chunks(data, 12, min(len(data), u32(data, 4)+8)):
        if cid == b"fmt ":
            info["tag"] = u16(data, po)
            info["ch"] = u16(data, po+2)
            info["rate"] = u32(data, po+4)
            info["byterate"] = u32(data, po+8)
            info["align"] = u16(data, po+12)
            info["bits"] = u16(data, po+14)
        elif cid == b"data":
            info["data_bytes"] = sz
        if len(info) >= 6 and "data_bytes" in info:
            break
    return info


def parse_avi(data):
    if data[:4] != b"RIFF" or data[8:12] != b"AVI ":
        return None
    out = {"streams": []}
    # hdrl LIST is first
    for cid, sz, po in riff_chunks(data, 12, min(len(data), 0x20000)):
        if cid == b"LIST" and data[po:po+4] == b"hdrl":
            # avih is first chunk inside
            for c2, s2, p2 in riff_chunks(data, po+4, po+sz):
                if c2 == b"avih":
                    out["us_per_frame"] = u32(data, p2)
                    out["total_frames"] = u32(data, p2+16)
                    out["width"] = u32(data, p2+32)
                    out["height"] = u32(data, p2+36)
                elif c2 == b"LIST" and data[p2:p2+4] == b"strl":
                    st = {}
                    for c3, s3, p3 in riff_chunks(data, p2+4, p2+s2):
                        if c3 == b"strh":
                            st["type"] = data[p3:p3+4].decode("latin1")
                            st["handler"] = data[p3+4:p3+8].decode("latin1")
                        elif c3 == b"strf":
                            if st.get("type") == "vids":
                                st["codec"] = data[p3+16:p3+20].decode("latin1")
                                st["bpp"] = u16(data, p3+14)
                            elif st.get("type") == "auds":
                                st["atag"] = u16(data, p3)
                                st["ach"] = u16(data, p3+2)
                                st["arate"] = u32(data, p3+4)
                                st["abits"] = u16(data, p3+14)
                    out["streams"].append(st)
            break
    return out


def parse_dmus(data):
    """DirectMusic RIFF form: RIFF <size> <FORM> ..."""
    if data[:4] != b"RIFF":
        return None
    form = data[8:12].decode("latin1")
    name = None
    end = min(len(data), u32(data, 4)+8)
    for cid, sz, po in riff_chunks(data, 12, end):
        if cid == b"LIST" and data[po:po+4] == b"UNFO":
            for c2, s2, p2 in riff_chunks(data, po+4, po+sz):
                if c2 == b"UNAM":
                    name = data[p2:p2+s2].split(b"\x00\x00")[0].decode("utf-16-le", "ignore")
    return {"form": form, "name": name}


def classify(path):
    ext = os.path.splitext(path)[1].lower()
    with open(path, "rb") as h:
        head = h.read(0x20000)
    size = os.path.getsize(path)
    if ext == ".wav":
        return "wav", parse_wav(head), size
    if ext == ".avi":
        return "avi", parse_avi(head), size
    if ext in (".sty", ".sgt", ".bnd"):
        return "dmus", parse_dmus(head), size
    if ext == ".bnv":
        # not RIFF; engine-native band/visitor blob
        return "bnv", {"magic": head[:4].hex()}, size
    return None, None, size


def main():
    dirs = sys.argv[1:]
    if not dirs:
        dirs = ["gamedata/main"]
        if os.path.isdir("/Volumes/LEGOLAND/Speech"):
            dirs.append("/Volumes/LEGOLAND/Speech")
            dirs.append("/Volumes/LEGOLAND")
    files = []
    for d in dirs:
        for ext in ("wav", "avi", "sty", "sgt", "bnd", "bnv"):
            files += glob.glob(os.path.join(d, "*." + ext))
            files += glob.glob(os.path.join(d, "*." + ext.upper()))
    files = sorted(set(files))

    wav_fmts = Counter()
    avi_vid = Counter()
    avi_aud = Counter()
    avi_noaudio = 0
    dmus_forms = Counter()
    bnv_n = 0
    samples = defaultdict(list)

    for f in files:
        kind, info, size = classify(f)
        if kind == "wav" and info:
            key = (WAVE_FMT.get(info["tag"], hex(info["tag"])), info["ch"],
                   info["rate"], info["bits"])
            wav_fmts[key] += 1
            if len(samples["wav"]) < 3:
                samples["wav"].append((os.path.basename(f), key, size))
        elif kind == "avi" and info:
            vs = next((s for s in info["streams"] if s.get("type") == "vids"), None)
            a = next((s for s in info["streams"] if s.get("type") == "auds"), None)
            if vs:
                fps = round(1e6/info["us_per_frame"], 2) if info.get("us_per_frame") else 0
                avi_vid[(vs.get("codec"), info["width"], info["height"], fps)] += 1
            if a:
                avi_aud[(WAVE_FMT.get(a["atag"], hex(a["atag"])), a["ach"], a["arate"])] += 1
            else:
                avi_noaudio += 1
            samples["avi"].append((os.path.basename(f),
                                   info.get("width"), info.get("height"),
                                   info.get("total_frames"),
                                   vs.get("codec") if vs else "?",
                                   "audio" if a else "silent", size))
        elif kind == "dmus" and info:
            dmus_forms[info["form"]] += 1
        elif kind == "bnv":
            bnv_n += 1

    print("=" * 64)
    print("WAV (speech / SFX)")
    print("  fmt / ch / rate / bits            count")
    for k, v in wav_fmts.most_common():
        print(f"  {str(k):34s} {v}")
    for n, k, s in samples["wav"]:
        print(f"    e.g. {n}  {s} bytes")

    print("=" * 64)
    print(f"AVI  ({sum(avi_vid.values())} files, {avi_noaudio} silent)")
    print("  video codec / WxH / fps           count")
    for k, v in avi_vid.most_common():
        print(f"  {str(k):34s} {v}")
    print("  audio fmt / ch / rate             count")
    for k, v in avi_aud.most_common():
        print(f"  {str(k):34s} {v}")
    for row in samples["avi"]:
        print(f"    {row[0]:24s} {row[1]}x{row[2]} {row[3]}f {row[4]} {row[5]} {row[6]}B")

    print("=" * 64)
    print("DirectMusic RIFF forms (.sty/.sgt/.bnd)")
    for k, v in dmus_forms.most_common():
        print(f"  {k:8s} {v}")
    print(f"BNV (engine-native band/visitor, non-RIFF): {bnv_n}")
    print("=" * 64)
    print(f"TOTAL files scanned: {len(files)}")


if __name__ == "__main__":
    main()
