#!/usr/bin/env python3
"""
Import COSMOS site layouts from the EVE University wiki.

Each empire COSMOS page has a "COSMOS sites" table (site name / type / system).
Every site page describes its pockets: NPC tables ({{NPCTableRow|Class|Count|Names}}),
structures (acceleration gates, passkeys) and site info (faction, ship limit).

Usage:
  python import_cosmos_sites.py --all --outdir <dir>
"""
import argparse
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

PAGES = [
    "Minmatar COSMOS", "Amarr COSMOS", "Caldari COSMOS", "Gallente COSMOS",
    "Angel COSMOS", "Guristas COSMOS", "Blood Raider COSMOS", "Serpentis COSMOS",
]


def raw(title, sleep=0.2):
    url = BASE + "?title=" + urllib.parse.quote(title.replace(" ", "_")) + "&action=raw"
    req = urllib.request.Request(url, headers={"User-Agent": "evemu-content/1.0"})
    with urllib.request.urlopen(req, timeout=40, context=CTX) as r:
        data = r.read().decode("utf-8", "replace")
    time.sleep(sleep)
    return data


def clean(s):
    if not s:
        return ""
    s = re.sub(r"<[^>]+>", "", s)
    s = s.replace("[[", "").replace("]]", "").replace("{{", "").replace("}}", "")
    return re.sub(r"\s+", " ", s).strip()


NPC_CLASSES = {
    "frigate", "destroyer", "cruiser", "battlecruiser", "battleship",
    "elite frigate", "elite cruiser", "elite battleship",
    "commander battleship", "overseer cruiser", "drone", "industrial",
    "sentry", "sentries", "friendly frigate", "friendly cruiser",
    "friendly battleship", "friendly elite frigate", "storyline mission cruiser",
}


def parse_site_list(page):
    """Parse the 'COSMOS sites' table into [{name, system, type}]."""
    i = page.find("COSMOS sites")
    if i < 0:
        return []
    # keep only the sites section: from its heading until the next heading of the
    # same or higher level (subsections like "==== Combat ====" stay included)
    head = page.rfind("\n", 0, i) + 1
    line_end = page.find("\n", head)
    head_line = page[head:line_end]
    level = len(head_line) - len(head_line.lstrip("="))
    stop = len(page)
    for m in re.finditer(r"^=+([^=\n]+)=+\s*$", page[line_end:], re.M):
        lvl = len(m.group(1)) # not used
        hlvl = len(m.group(0)) - len(m.group(0).lstrip("="))
        if hlvl <= level:
            stop = line_end + m.start()
            break
    body = page[i:stop]
    sites = []
    for chunk in re.split(r"^\s*\|-\s*$", body, flags=re.M):
        m = re.search(r"\[\[([^\]|]+)\]\]", chunk)
        if not m:
            continue
        name = m.group(1).strip()
        if name.lower().startswith(("cosmos", "category:")):
            continue
        lines = [l.strip() for l in chunk.split("\n") if l.strip().startswith("|")]
        vals = [clean(l[1:]) for l in lines]
        # vals = [[[name]], ded, type, system, id]
        if len(vals) >= 3:
            sites.append({"name": name, "rating": vals[1], "type": vals[2],
                          "system": vals[3] if len(vals) > 3 else "",
                          "sig": vals[4] if len(vals) > 4 else ""})
    return sites


def field(text, name):
    m = re.search(r"\|\s*%s\s*=\s*([^\n|]*)" % name, text)
    return m.group(1).strip() if m else ""


def parse_site_page(text):
    info = {
        "faction": field(text, "faction"),
        "shipLimit": field(text, "ship limit"),
        "location": field(text, "location"),
        "type": field(text, "type"),
        "rating": field(text, "rating"),
        "pockets": [],
    }
    parts = re.split(r"^==+\s*([^=\n]+?)\s*==+\s*$", text, flags=re.M)
    sections = [(None, parts[0])]
    for i in range(1, len(parts), 2):
        sections.append((parts[i], parts[i + 1]))
    for heading, body in sections:
        if heading is None:
            continue
        npcs = []
        gates = []
        for row in re.findall(r"\{\{NPCTableRow\|([^}]*)\}\}", body):
            f = [x.strip() for x in row.split("|")]
            if len(f) < 3:
                continue
            cls = f[0].lower()
            if "acceleration gate" in cls or cls in ("accl gate",):
                gates.append(row)
                continue
            if cls not in NPC_CLASSES:
                continue
            if not re.match(r"^\d+$", re.sub(r"[^0-9]", "", f[1]) or ""):
                continue
            npcs.append({"class": f[0], "count": f[1], "names": f[2],
                         "drop": f[3] if len(f) > 3 else ""})
        passkeys = []
        for line in body.split("\n"):
            if "passcard" in line.lower() or "passkey" in line.lower() or "requires" in line.lower():
                passkeys.append(clean(line))
        if npcs or gates or "Pocket" in heading or "pocket" in heading:
            info["pockets"].append({"name": clean(heading), "npcs": npcs,
                                    "gates": gates, "notes": passkeys})
    return info


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--outdir")
    args = ap.parse_args()
    assert args.outdir

    os.makedirs(args.outdir, exist_ok=True)
    for page in PAGES:
        try:
            cpage = raw(page)
            sites = parse_site_list(cpage)
            out = []
            for s in sites:
                try:
                    stext = raw(s["name"])
                except Exception as ex:
                    sys.stderr.write("  site %s failed: %s\n" % (s["name"], ex))
                    continue
                d = parse_site_page(stext)
                d.update(s)
                d["page"] = page
                out.append(d)
            fn = os.path.join(args.outdir, page.replace(" ", "_") + ".json")
            with open(fn, "w", encoding="utf-8") as f:
                json.dump(out, f, ensure_ascii=False, indent=1)
            print("%s: sites=%d pockets=%d" % (page, len(out), sum(len(x["pockets"]) for x in out)), flush=True)
        except Exception as ex:
            print("%s: FAILED %s" % (page, ex), flush=True)


if __name__ == "__main__":
    main()
