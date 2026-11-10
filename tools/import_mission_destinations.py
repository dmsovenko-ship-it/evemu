#!/usr/bin/env python3
"""
Extract mission destinations (system/station) from the EVE University wiki and
emit the SQL that seeds missionObjectives.destinationSystemID/StationID.

Sources (in %TEMP%\\opencode): q_systems.tsv, q_stations.tsv,
q_arc_missions.tsv (missionID/name from epicArcMission),
q_cosmos_missions.tsv (agtMissions typeID=9).

Usage:
  python import_mission_destinations.py --outdir <dir>
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

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
BASE = "https://wiki.eveuniversity.org/index.php"
CTX = ssl.create_default_context()
CTX.check_hostname = False
CTX.verify_mode = ssl.CERT_NONE


def norm(s):
    s = s.replace("\u2019", "'").replace("\ufffd", "'")
    s = re.sub(r"<[^>]+>", " ", s or "")
    s = re.sub(r"\s+", " ", s)
    return s


def raw(title, cache_dir):
    fn = os.path.join(cache_dir, re.sub(r"[^A-Za-z0-9._-]", "_", title) + ".wiki")
    if os.path.exists(fn):
        return io.open(fn, encoding="utf-8", errors="replace").read()
    url = BASE + "?title=" + urllib.parse.quote(title.replace(" ", "_")) + "&action=raw"
    req = urllib.request.Request(url, headers={"User-Agent": "evemu-content/1.0"})
    with urllib.request.urlopen(req, timeout=40, context=CTX) as r:
        data = r.read().decode("utf-8", "replace")
    time.sleep(0.15)
    io.open(fn, "w", encoding="utf-8").write(data)
    return data


def field(text, name):
    m = re.search(r"\|\s*%s\s*=\s*([^\n|]*)" % name, text)
    return m.group(1).strip() if m else ""


def load_tsv(name, cols):
    out = []
    p = os.path.join(B, name)
    if not os.path.exists(p):
        return out
    for line in io.open(p, encoding="utf-8", errors="replace"):
        f = line.rstrip().split("\t")
        if len(f) >= cols and f[0].strip().isdigit():
            out.append(f)
    return out


def boundaries_ok(text, pos, needle):
    a = text[pos - 1] if pos > 0 else " "
    b = text[pos + len(needle)] if pos + len(needle) < len(text) else " "
    return not a.isalnum() and not b.isalnum()


AGENT_CTX = re.compile(r"\b(report|talk|speak|contact|meet|bring|deliver|return|visit|find|go to)\b[^.]{0,40}$")


def find_agent(text, agents, station_sys):
    """'Report to <Agent Name>' -> (systemID, stationID, agentID)."""
    best = None
    for name, loc, agent_id in agents:
        pos = text.find(name)
        while pos != -1:
            if boundaries_ok(text, pos, name):
                ctx = text[max(0, pos - 40):pos]
                if AGENT_CTX.search(ctx):
                    if loc >= 60000000:
                        sysid = station_sys.get(loc, 0)
                        if sysid:
                            return (sysid, loc, agent_id)
                    elif 30000000 <= loc <= 30999999:
                        return (loc, 0, agent_id)
                break
            pos = text.find(name, pos + 1)
    return best


def find_station(text, stations):
    best = None
    for st in stations:            # [stationID, stationName, solarSystemID]
        name = norm(st[1]).lower()
        if len(name) < 8:
            continue
        pos = text.find(name)
        while pos != -1:
            if boundaries_ok(text, pos, name):
                if best is None or len(name) > len(best[1]):
                    best = (int(st[0]), name, int(st[2]))
                break
            pos = text.find(name, pos + 1)
    return best


def find_system(text, systems):
    best = None
    for sy in systems:             # [solarSystemID, solarSystemName]
        name = norm(sy[1]).lower()
        if len(name) < 3:
            continue
        pos = text.find(name)
        while pos != -1:
            if boundaries_ok(text, pos, name):
                ctx = text[max(0, pos - 14):pos]
                score = 1
                if re.search(r"\b(in|to|at|near|system|station)\b", ctx):
                    score = 3
                if best is None or score > best[0]:
                    best = (score, int(sy[0]), name)
                break
            pos = text.find(name, pos + 1)
        if best is not None and best[0] == 3:
            break
    return best


def mission_text(text):
    obj = field(text, "Objective")
    mb = re.search(r"\{\{MissionBriefing\s*\|(.*?)\}\}\s*(?:\n|$)", text, re.S)
    brief = mb.group(1) if mb else ""
    desc = ""
    m = re.search(r"==\s*Description\s*==(.*?)(?:\n==|\Z)", text, re.S)
    if m:
        desc = m.group(1)
    return norm(obj), norm(brief), norm(desc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", required=True)
    ap.add_argument("--limit", type=int, default=0)
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)
    cache = os.path.join(args.outdir, "cache")
    os.makedirs(cache, exist_ok=True)

    systems = load_tsv("q_systems.tsv", 2)
    stations = load_tsv("q_stations.tsv", 3)
    station_sys = {}
    for st in stations:
        station_sys[int(st[0])] = int(st[2])
    agents = []
    for f in load_tsv("q_agents.tsv", 3):
        nm = norm(f[1]).lower()
        if len(nm) >= 5:
            agents.append((nm, int(f[2]), int(f[0])))
    missions = []
    for f in load_tsv("q_arc_missions.tsv", 2):
        missions.append((int(f[0]), f[1]))
    for f in load_tsv("q_cosmos_missions.tsv", 2):
        missions.append((int(f[0]), f[1]))
    seen = set()
    uniq = []
    for m in missions:
        if m[0] not in seen:
            seen.add(m[0])
            uniq.append(m)
    missions = uniq
    if args.limit:
        missions = missions[:args.limit]
    sys.stderr.write("missions: %d\n" % len(missions))

    out = []
    for mid, name in missions:
        try:
            page = raw(name, cache)
        except Exception as ex:
            sys.stderr.write("skip %u %s: %s\n" % (mid, name, ex))
            continue
        texts = mission_text(page)
        found = None
        for txt in texts:
            if not txt:
                continue
            low = txt.lower()
            ag = find_agent(low, agents, station_sys)
            if ag:
                found = ag
                break
            st = find_station(low, stations)
            if st:
                found = (st[2], st[0])
                break
            sy = find_system(low, systems)
            if sy:
                found = (sy[1], 0)
                break
        if found:
            ag = found[2] if len(found) > 2 else 0
            out.append((mid, found[0], found[1], ag))
            sys.stderr.write("%u %s -> sys %u station %u agent %u\n" % (mid, name, found[0], found[1], ag))
    json.dump(out, io.open(os.path.join(args.outdir, "destinations.json"), "w", encoding="utf-8"))

    sql = []
    sql.append("-- Mission destinations parsed from the EVE University wiki")
    sql.append("-- (systems/stations mentioned in Objective/Briefing/Description).")
    sql.append("-- +migrate Up")
    sql.append("ALTER TABLE missionObjectives ADD COLUMN IF NOT EXISTS destinationSystemID int(10) unsigned NOT NULL DEFAULT 0;")
    sql.append("ALTER TABLE missionObjectives ADD COLUMN IF NOT EXISTS destinationStationID int(10) unsigned NOT NULL DEFAULT 0;")
    for row in out:
        mid, dsys, st = row[0], row[1], row[2]
        sql.append("UPDATE missionObjectives SET destinationSystemID=%d, destinationStationID=%d WHERE missionID=%d;"
                   % (dsys, st, mid))
    sql.append("-- +migrate Down")
    sql.append("-- (columns stay; the offer logic only reads them)")
    sql.append("")
    out_sql = r"C:\opencode-projects\evemu\sql\migrations\20261106000026-mission_destinations.sql"
    io.open(out_sql, "w", encoding="utf-8").write("\n".join(sql))

    # destination agents ("Report to <Agent>") -> journal 'agent' objective
    asql = []
    asql.append("-- Mission destination agents (journal 'report to agent' objective).")
    asql.append("-- +migrate Up")
    asql.append("ALTER TABLE missionObjectives ADD COLUMN IF NOT EXISTS destinationAgentID int(10) unsigned NOT NULL DEFAULT 0;")
    agent_count = 0
    for row in out:
        mid, ag = row[0], row[3]
        if ag:
            asql.append("UPDATE missionObjectives SET destinationAgentID=%d WHERE missionID=%d;" % (ag, mid))
            agent_count += 1
    asql.append("-- +migrate Down")
    asql.append("-- (column stays)")
    asql.append("")
    out_agent_sql = r"C:\opencode-projects\evemu\sql\migrations\20261106000028-mission_destination_agents.sql"
    io.open(out_agent_sql, "w", encoding="utf-8").write("\n".join(asql))
    print("resolved", len(out), "of", len(missions), "agent targets", agent_count, "sql", out_sql)


if __name__ == "__main__":
    main()
