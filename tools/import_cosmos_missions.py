#!/usr/bin/env python3
"""
Import COSMOS mission content from the EVE University wiki.

COSMOS missions are NOT in the client (resCOSMOS.stuff holds only models/textures;
the localization has only site lore), and the 92xxx rows in our DB are a synthetic
skeleton. The wiki has one page per COSMOS agent; missions are either transcluded
({{:Mission Name}}) or written inline as a level-2 heading followed by
{{Missiondetails ...}} and {{MissionBriefing|...}} blocks.

Output JSON per constellation page, enriched with agent standing/faction/location,
mission level, classified objective type and extracted item/quantity.

Usage:
  python import_cosmos_missions.py --page "Minmatar COSMOS" --out minmatar.json
  python import_cosmos_missions.py --all --outdir <dir>
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


def brief_of(text):
    m = re.search(r"\{\{MissionBriefing\s*\|(.*?)\}\}\s*(?:\n|$)", text, re.S)
    if not m:
        return ""
    return re.sub(r"\s+", " ", m.group(1).replace("'''", "")).strip()


def parse_details(block):
    return {
        "objective": field(block, "Objective"),
        "faction": field(block, "Faction1") or field(block, "Faction"),
        "level": field(block, "Level"),
        "type": field(block, "Type"),
        "rewards": field(block, "Rewards"),
        "standingLoss": field(block, "StandingLoss"),
    }


def clean(s):
    if not s:
        return ""
    s = re.sub(r"<[^>]+>", "", s)
    s = s.replace("&nbsp;", " ").replace("[[", "").replace("]]", "")
    s = re.sub(r"\s+", " ", s)
    return s.strip()


def standing_to_level(standing):
    try:
        v = float(re.sub(r"[^0-9.]", "", standing or ""))
    except Exception:
        return 1
    if v <= 0:
        return 1
    if v < 4.0:
        return 2
    if v < 6.0:
        return 3
    return 4


def classify(mtype, obj):
    t = (mtype or "").lower()
    o = (obj or "").lower()
    if "encounter" in t or "security" in t or "combat" in t:
        return 3
    if "courier" in t or "distribution" in t or "haul" in t:
        return 2
    if "trade" in t:
        if any(k in o for k in ("deliver", "transport", "provide")):
            return 2
        return 1
    if "mining" in t:
        return 3
    if any(k in o for k in ("destroy", "kill", "retrieve %s" % "", "salvage")):
        return 3
    return 1


QTY_ITEM = re.compile(r"(\d[\d,]*)\s*[x\u00d7]\s*([A-Za-z0-9][^,()\[\]]{2,60}?)\s*(?:\(|,|$| from | in | and )")


def extract_item(obj):
    """Return (name, qty) from an objective like 'Provide 1 x Jakon's Head (0.1 m3)'."""
    s = clean(obj)
    m = QTY_ITEM.search(s)
    if m:
        qty = int(m.group(1).replace(",", ""))
        name = m.group(2).strip(" .;:")
        if name and name.lower() not in ("m3", "units"):
            return name, qty
    return "", 0


def parse_agent_meta(page):
    """Parse the agents table: rows separated by '|-', fields on following lines."""
    meta = {}
    for chunk in re.split(r"^\s*\|-\s*$", page, flags=re.M):
        m = re.search(r"\|\s*\[\[([^\]|]+)\]\]", chunk)
        if not m:
            continue
        name = m.group(1).strip()
        lines = [l for l in chunk.split("\n") if l.strip()]
        i = next((k for k, l in enumerate(lines) if "[[" + name in l.replace(" ", "") or (name in l and "[[" in l)), None)
        rest = lines[i + 1:] if i is not None else []
        vals = []
        for l in rest:
            l = l.strip()
            if l.startswith("|"):
                vals.append(clean(l[1:]))
        meta[name] = {
            "standing": vals[0] if len(vals) > 0 else "",
            "faction": vals[1] if len(vals) > 1 else "",
            "location": vals[2] if len(vals) > 2 else "",
        }
    return meta


def parse_agent_page(page, agent):
    out = []
    parts = re.split(r"^==+\s*([^=\n]+?)\s*==+\s*$", page, flags=re.M)
    sections = [(None, parts[0])]
    for i in range(1, len(parts), 2):
        sections.append((parts[i], parts[i + 1]))
    for heading, body in sections:
        for blk in re.findall(r"\{\{Missiondetails(.*?)\}\}", body, re.S):
            d = parse_details(blk)
            name = field(blk, "Name") or (heading or "").strip()
            if not name:
                continue
            d.update({"agent": agent, "mission": name, "briefing": brief_of(body)})
            out.append(d)
    for name in re.findall(r"\{\{:([^}]+)\}\}", page):
        name = name.strip()
        if any(m["mission"] == name for m in out):
            continue
        try:
            mtext = raw(name)
        except Exception:
            continue
        if "Missiondetails" not in mtext and "MissionBriefing" not in mtext:
            continue
        mb = re.search(r"\{\{Missiondetails(.*?)\}\}", mtext, re.S)
        d = parse_details(mb.group(1)) if mb else {}
        d.update({"agent": agent, "mission": name, "briefing": brief_of(mtext)})
        out.append(d)
    return out


def import_page(pagename):
    page = raw(pagename)
    agents = []
    for a in re.findall(r"\|\s*\[\[([^\]|]+)\]\]", page):
        if a not in agents:
            agents.append(a)
    meta = parse_agent_meta(page)
    missions = []
    for agent in agents:
        try:
            apage = raw(agent)
        except Exception as ex:
            sys.stderr.write("  agent %s failed: %s\n" % (agent, ex))
            continue
        am = meta.get(agent, {})
        for d in parse_agent_page(apage, agent):
            if not d.get("faction"):
                d["faction"] = am.get("faction", "")
            if not d.get("level"):
                d["level"] = str(standing_to_level(am.get("standing", "")))
            d["agentStanding"] = am.get("standing", "")
            d["agentFaction"] = am.get("faction", "")
            d["agentLocation"] = am.get("location", "")
            d["objectiveType"] = classify(d.get("type"), d.get("objective"))
            iname, iqty = extract_item(d.get("objective"))
            d["itemName"], d["itemQty"] = iname, iqty
            missions.append(d)
    return agents, missions


PAGES = [
    "Minmatar COSMOS", "Amarr COSMOS", "Caldari COSMOS", "Gallente COSMOS",
    "Angel COSMOS", "Guristas COSMOS", "Blood Raider COSMOS", "Serpentis COSMOS",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--page")
    ap.add_argument("--out")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--outdir")
    args = ap.parse_args()

    if args.all:
        assert args.outdir, "--outdir required with --all"
        os.makedirs(args.outdir, exist_ok=True)
        total = 0
        for p in PAGES:
            try:
                agents, missions = import_page(p)
                fn = os.path.join(args.outdir, p.replace(" ", "_") + ".json")
                with open(fn, "w", encoding="utf-8") as f:
                    json.dump(missions, f, ensure_ascii=False, indent=1)
                total += len(missions)
                print("%s: agents=%d missions=%d" % (p, len(agents), len(missions)), flush=True)
            except Exception as ex:
                print("%s: FAILED %s" % (p, ex), flush=True)
        print("TOTAL missions=%d" % total)
        return

    assert args.page and args.out
    agents, missions = import_page(args.page)
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(missions, f, ensure_ascii=False, indent=1)
    print("%s: agents=%d missions=%d -> %s" % (args.page, len(agents), len(missions), args.out))


if __name__ == "__main__":
    main()
