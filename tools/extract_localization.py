#!/usr/bin/env python3
"""
Extract EVE client localization (messageID -> text) from resLocalization.stuff.

The .stuff files are "EmbedFs 1.0" archives:
    u32 entryCount
    repeat { u32 dataSize; u32 nameLen; char name[nameLen]; u8 0 }   (no padding)
    <data blocks in the same order as the directory>
    <manifest text + "EmbedFs 1.0" footer>

Each localization_*.pickle is a plain pickle: (localeName, {messageID: (text, ..)}).

Usage:
  python extract_localization.py --stuff "<client>/resLocalization.stuff" --locale en-us \
      --out loc_en.json [--id 130400] [--grep "Dagan"] [--minlen 200]

The JSON is a flat {messageID: text} map (UTF-8). Only entries with text are kept.
"""
import argparse
import json
import os
import pickle
import struct
import sys
import zlib


def read_embedfs(path):
    data = open(path, "rb").read()
    count = struct.unpack_from("<I", data, 0)[0]
    off = 4
    entries = []
    for _ in range(count):
        size = struct.unpack_from("<I", data, off)[0]
        off += 4
        nlen = struct.unpack_from("<I", data, off)[0]
        off += 4
        name = data[off:off + nlen].decode("ascii", "replace")
        off += nlen + 1          # single NUL terminator, no 4-byte padding
        entries.append((name, size))
    blobs = {}
    for name, size in entries:
        blobs[name] = data[off:off + size]
        off += size
    return blobs


def load_locale(stuff_path, locale):
    blobs = read_embedfs(stuff_path)
    key = None
    for name in blobs:
        if name.lower().endswith("localization_%s.pickle" % locale.lower()):
            key = name
            break
    if key is None:
        raise SystemExit("locale %s not found; available: %s" %
                         (locale, ", ".join(sorted(k for k in blobs if "localization_" in k))))
    blob = blobs[key]
    if blob[:1] == b"\x78":
        blob = zlib.decompress(blob)
    obj = pickle.loads(blob, encoding="utf-8", errors="replace")
    table = obj[1] if isinstance(obj, tuple) and len(obj) > 1 else obj
    out = {}
    for mid, val in table.items():
        text = val[0] if isinstance(val, tuple) and val else val
        if isinstance(text, str) and text:
            try:
                mid = int(mid)
            except (TypeError, ValueError):
                continue
            out[mid] = text
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stuff", required=True, help="path to resLocalization.stuff")
    ap.add_argument("--locale", default="en-us", help="en-us | de | ru | ja | main")
    ap.add_argument("--out", help="write {messageID: text} JSON here")
    ap.add_argument("--id", type=int, action="append", default=[], help="print these messageIDs")
    ap.add_argument("--grep", action="append", default=[], help="print entries containing this text")
    ap.add_argument("--minlen", type=int, default=0, help="with --grep: only texts at least this long")
    ap.add_argument("--limit", type=int, default=10)
    args = ap.parse_args()

    if not os.path.exists(args.stuff):
        raise SystemExit("not found: %s" % args.stuff)
    loc = load_locale(args.stuff, args.locale)
    sys.stderr.write("loaded %d localization entries (%s)\n" % (len(loc), args.locale))

    if args.out:
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump({str(k): v for k, v in sorted(loc.items())}, f, ensure_ascii=False)
        sys.stderr.write("wrote %s\n" % args.out)

    for mid in args.id:
        print("%d\t%s" % (mid, loc.get(mid, "<missing>")))

    for term in args.grep:
        shown = 0
        for mid in sorted(loc):
            t = loc[mid]
            if term.lower() in t.lower() and len(t) >= args.minlen:
                print("%d\t%s" % (mid, t[:400].replace("\n", " ")))
                shown += 1
                if shown >= args.limit:
                    break


if __name__ == "__main__":
    main()
