#!/usr/bin/env python3
"""
Capture the COSMOS agent infobox (level / system / beacon / corporation) from the
EVE University wiki. COSMOS agents live in space next to a named beacon
("Natura Seminary", "The Ebony Tower", ...) - this gives every agent a concrete
agent-site, not just the ones whose wiki location column parsed cleanly.

Usage: python import_cosmos_agent_meta.py --outdir <dir>
"""
import argparse
import io
import json
import os
import re
import ssl
import sys
import time
import urllib.parse
import urllib.request

BASE = "https://wiki.eveuniversity.org/index.php"
CTX = ssl.create_default_context()
CTX.check_hostname = False
CTX.verify_mode = ssl.CERT_NONE

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"


def raw(title, sleep=0.2):
    url = BASE + "?title=" + urllib.parse.quote(title.replace(" ", "_")) + "&action=raw"
    req = urllib.request.Request(url, headers={"User-Agent": "evemu-content/1.0"})
    with urllib.request.urlopen(req, timeout=40, context=CTX) as r:
        data = r.read().decode("utf-8", "replace")
    time.sleep(sleep)
    return data


def field(text, name):
    m = re.search(r"\|\s*%s\s*=\s*([^\n|]*)" % name, text)
    return m.group(1).strip() if m else ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", required=True)
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)

    agents = json.load(io.open(os.path.join(B, "agents_cosmos.json"), encoding="utf-8"))
    out = []
    for a in agents:
        try:
            page = raw(a["name"])
        except Exception as ex:
            sys.stderr.write("agent %s failed: %s\n" % (a["name"], ex))
            continue
        beacon = field(page, "beacon")
        system = field(page, "system")
        beacon = re.sub(r"<[^>]+>", "", beacon)
        beacon = re.sub(r"\[\[([^\]|]+)\]\]", r"\1", beacon)
        beacon = re.sub(r"\s+", " ", beacon).strip()
        out.append({"agentID": a["id"], "name": a["name"], "systemID": a["system"],
                    "beacon": beacon, "system": system,
                    "corporation": field(page, "corporation"),
                    "level": field(page, "level")})
        if beacon:
            sys.stderr.write("%s -> %s\n" % (a["name"], beacon))
    io.open(os.path.join(args.outdir, "agents_meta.json"), "w", encoding="utf-8").write(
        json.dumps(out, ensure_ascii=False, indent=1))
    print("agents", len(out), "with beacon", sum(1 for a in out if a["beacon"]))


if __name__ == "__main__":
    main()
