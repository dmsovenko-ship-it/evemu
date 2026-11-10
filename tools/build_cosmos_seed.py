#!/usr/bin/env python3
"""
Build the COSMOS seed migration from data produced by import_cosmos_missions.py
plus live-DB dumps (agent list, item typeID map).

Inputs (all in %TEMP%\\opencode):
  agents_cosmos.json   - in-space type-8 agents (agentID, name, level, system, corp)
  cosmos2/*.json       - wiki missions per constellation page
  items_res3.txt       - "typeID<TAB>typeName" rows from the live invTypes
Output:
  sql/migrations/20261106000021-cosmos_real_missions.sql
"""
import glob
import io
import json
import os
import re

BASE = r"C:\Users\Dima\AppData\Local\Temp\opencode"
OUT = r"C:\opencode-projects\evemu\sql\migrations\20261106000021-cosmos_real_missions.sql"
FIRST_ID = 93000

FACTIONS = {
    "caldari": 500001, "minmatar": 500002, "amarr": 500003, "gallente": 500004,
    "concord": 500006, "ammatar": 500007, "khanid": 500008, "syndicate": 500009,
    "guristas": 500010, "angel": 500011, "blood raider": 500012, "sansha": 500019,
    "serpentis": 500020, "rogue drone": 500022, "thukker": 500002,
}

# generic faction structure spawned at the mission site for hunt objectives
STRUCTURES = {
    500001: 16796,   # Guristas/State bunker style
    500002: 17138,   # Angel/Minmatar battlestation
    500003: 16727,   # Amarr cathedral
    500004: 11076,   # stronghold
    500006: 11076,
    500007: 16727,
    500009: 11076,
    500010: 16796,
    500011: 17138,
    500012: 16727,
    500019: 26249,
    500020: 11076,
    500022: 16732,
}

MANUAL_ITEMS = {
    "1mn afterburner i": 439,
    "strange construction blocks": 22217,
    "large sealed cargo containers": 16045,
}


def esc(s):
    if s is None:
        return "''"
    s = s.replace("\\", "\\\\").replace("'", "''")
    return "'" + s + "'"


def clean_item(n):
    n = (n or "").strip()
    n = re.sub(r"\s*\d+(\.\d+)?\s*\{\{.*$", "", n)
    n = n.split("{{")[0]
    n = re.sub(r"\s+(to|for)\s+the\s+agent.*$", "", n, flags=re.I)
    n = re.sub(r"\s+\d+(\.\d+)?\s*m?3?$", "", n)
    return n.strip(" .,;:'\"")


def sanitize_level(v, fallback=1):
    d = re.sub(r"[^0-9]", "", str(v or ""))
    if not d:
        return max(1, min(4, fallback))
    return max(1, min(4, int(d)))


def faction_id(name):
    n = (name or "").lower()
    for k, v in FACTIONS.items():
        if k in n:
            return v
    return 0


def is_hunt(m):
    t = (m.get("type") or "").lower()
    o = (m.get("objective") or "").lower()
    if any(k in t for k in ("encounter", "security", "combat", "mining")):
        return True
    return any(k in o for k in ("destroy", "kill", "clear out", "eliminate"))


def step_of(name):
    m = re.search(r"\((\d+)\s+of\s+(\d+)\)", name or "")
    return int(m.group(1)) if m else 0


def main():
    agents = json.load(io.open(os.path.join(BASE, "agents_cosmos.json"), encoding="utf-8"))
    by_name = {}
    for a in agents:
        by_name.setdefault(a["name"], []).append(a)

    rows = []
    seen = set()
    for fn in sorted(glob.glob(os.path.join(BASE, "cosmos2", "*.json"))):
        page = os.path.basename(fn)[:-5].replace("_", " ")
        for m in json.load(io.open(fn, encoding="utf-8")):
            alist = by_name.get(m["agent"])
            if not alist:
                continue
            agent = alist[0]
            key = (agent["id"], m["mission"])
            if key in seen:
                continue
            seen.add(key)
            m["_page"] = page
            m["_agent"] = agent
            rows.append(m)

    # assign mission ids + chain steps per agent
    steps = {}
    for m in rows:
        a = m["_agent"]
        s = step_of(m["mission"])
        steps.setdefault(a["id"], []).append((s, m))
    order = []
    for aid, lst in steps.items():
        lst.sort(key=lambda x: (x[0] if x[0] else 10 ** 6, x[1]["mission"]))
        for i, (_, m) in enumerate(lst, 1):
            order.append((aid, i, m))

    # item map
    have = {}
    p = os.path.join(BASE, "items_res3.txt")
    for l in io.open(p, encoding="utf-8", errors="replace"):
        f = l.rstrip("\n").split("\t")
        if len(f) >= 2 and f[0].isdigit():
            have[f[1].lower()] = int(f[0])
    have.update(MANUAL_ITEMS)

    mid = FIRST_ID
    mission_sql = []
    obj_sql = []
    chain_sql = []
    stats = {"missions": 0, "hunt": 0, "travel": 0, "items": 0}

    for aid, step, m in order:
        mname = m["mission"]
        lvl = sanitize_level(m.get("level"), m["_agent"]["level"])
        fid = faction_id(m.get("faction") or m["_agent"].get("faction"))
        hunt = is_hunt(m)
        otype = 3 if hunt else 1
        item = clean_item(m.get("itemName"))
        itype = have.get(item.lower(), 0) if item else 0
        iqty = int(m.get("itemQty") or 0) if itype else 0
        if itype:
            stats["items"] += 1
        structure = STRUCTURES.get(fid, 0) if hunt else 0
        npc_count = 5 if hunt else 0
        brief = re.sub(r"\s+", " ", m.get("briefing") or "").strip()
        objtxt = re.sub(r"\s+", " ", m.get("objective") or "").strip()[:500]
        note = ("COSMOS %s / %s" % (m["_page"], m.get("type") or "?"))[:255]

        mission_sql.append(
            "(%d,0,%s,%d,9,0,0,0,0,0,0,0,0,0,0,0)" % (mid, esc(mname), lvl))
        obj_sql.append(
            "(%d,0,%d,%d,%d,%d,%s,%d,%d,%d,%s,%s)" % (
                mid, otype, itype, iqty, npc_count, esc(note), fid, structure,
                (5 if structure else 0), esc(objtxt), esc(brief)))
        chain_sql.append("(%d,%d,%d)" % (aid, mid, step))
        stats["missions"] += 1
        stats["hunt" if hunt else "travel"] += 1
        mid += 1

    # agent rows (names + bloodline typeID 1376 default; existing rows keep theirs)
    agent_sql = ",\n".join("(%d,%s,'',1376,0,%d,0,0,0,0)" % (a["id"], esc(a["name"]), a["corp"])
                           for a in agents)

    sql = []
    sql.append("-- COSMOS content from the EVE University wiki (agents + missions).")
    sql.append("-- The client does not ship COSMOS mission texts (resCOSMOS holds only models),")
    sql.append("-- and the previous 92xxx rows were a synthetic skeleton.")
    sql.append("-- +migrate Up")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosAgentMissions (")
    sql.append("  agentID int(10) unsigned NOT NULL,")
    sql.append("  missionID int(10) unsigned NOT NULL,")
    sql.append("  step tinyint(3) unsigned NOT NULL,")
    sql.append("  PRIMARY KEY (agentID, step),")
    sql.append("  UNIQUE KEY (missionID)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("")
    sql.append("-- drop the synthetic COSMOS set")
    sql.append("DELETE FROM cosmosAgentMissions;")
    sql.append("DELETE FROM agtOffers WHERE missionID BETWEEN 92000 AND 92207;")
    sql.append("DELETE FROM missionObjectives WHERE missionID BETWEEN 92000 AND 92207;")
    sql.append("DELETE FROM agtMissions WHERE id BETWEEN 92000 AND 92207;")
    sql.append("DELETE FROM agtAgents WHERE agentTypeID=12;")
    sql.append("")
    sql.append("-- COSMOS agents live in space in the SDE; our client talks to agents at stations,")
    sql.append("-- so dock them at a station in their system (fallback: constellation, then region)")
    sql.append("UPDATE agtAgents SET level=4 WHERE agentTypeID=8 AND level>4")
    sql.append("  AND locationID BETWEEN 30000000 AND 30999999;")
    sql.append("UPDATE agtAgents a")
    sql.append("  JOIN (SELECT solarSystemID, MIN(stationID) AS sid FROM staStations GROUP BY solarSystemID) s")
    sql.append("    ON s.solarSystemID = a.locationID")
    sql.append("  SET a.locationID = s.sid")
    sql.append("  WHERE a.agentTypeID=8 AND a.locationID BETWEEN 30000000 AND 30999999;")
    sql.append("UPDATE agtAgents a")
    sql.append("  JOIN mapSolarSystems ms ON ms.solarSystemID = a.locationID")
    sql.append("  JOIN (SELECT ms2.constellationID, MIN(st.stationID) AS sid FROM staStations st")
    sql.append("          JOIN mapSolarSystems ms2 ON ms2.solarSystemID = st.solarSystemID")
    sql.append("        GROUP BY ms2.constellationID) c ON c.constellationID = ms.constellationID")
    sql.append("  SET a.locationID = c.sid")
    sql.append("  WHERE a.agentTypeID=8 AND a.locationID BETWEEN 30000000 AND 30999999;")
    sql.append("UPDATE agtAgents a")
    sql.append("  JOIN mapSolarSystems ms ON ms.solarSystemID = a.locationID")
    sql.append("  JOIN (SELECT ms2.regionID, MIN(st.stationID) AS sid FROM staStations st")
    sql.append("          JOIN mapSolarSystems ms2 ON ms2.solarSystemID = st.solarSystemID")
    sql.append("        GROUP BY ms2.regionID) r ON r.regionID = ms.regionID")
    sql.append("  SET a.locationID = r.sid")
    sql.append("  WHERE a.agentTypeID=8 AND a.locationID BETWEEN 30000000 AND 30999999;")
    sql.append("")
    sql.append("-- ensure every COSMOS agent has a name row (LoadAgentData needs a bloodline typeID)")
    sql.append("INSERT IGNORE INTO chrNPCCharacters")
    sql.append("  (characterID, characterName, description, typeID, gender, corporationID, stationID, solarSystemID, constellationID, regionID)")
    sql.append("VALUES")
    sql.append(agent_sql + ";")
    sql.append("UPDATE chrNPCCharacters c")
    sql.append("  JOIN agtAgents a ON a.agentID = c.characterID")
    sql.append("  LEFT JOIN staStations s ON s.stationID = a.locationID")
    sql.append("  SET c.corporationID = a.corporationID, c.stationID = a.locationID,")
    sql.append("      c.solarSystemID = IFNULL(s.solarSystemID, c.solarSystemID),")
    sql.append("      c.constellationID = IFNULL(s.constellationID, c.constellationID),")
    sql.append("      c.regionID = IFNULL(s.regionID, c.regionID)")
    sql.append("  WHERE a.agentTypeID=8;")
    sql.append("")
    sql.append("-- real missions (typeID 9 = Cosmos)")
    sql.append("DELETE FROM agtMissions WHERE id BETWEEN %d AND %d;" % (FIRST_ID, mid - 1))
    sql.append("INSERT INTO agtMissions")
    sql.append("  (id, briefingID, name, level, typeID, important, storyline, raceID, constellationID,")
    sql.append("   corporationID, dungeonID, rewardISK, rewardItemID, rewardItemQty, bonusISK, bonusTime)")
    sql.append("VALUES")
    sql.append(",\n".join(mission_sql) + ";")
    sql.append("")
    sql.append("REPLACE INTO missionObjectives")
    sql.append("  (missionID, briefingID, objectiveType, itemTypeID, itemQty, npcCount, note,")
    sql.append("   npcFactionID, structureTypeID, structureQty, objectiveText, briefingText)")
    sql.append("VALUES")
    sql.append(",\n".join(obj_sql) + ";")
    sql.append("")
    sql.append("REPLACE INTO cosmosAgentMissions (agentID, missionID, step) VALUES")
    sql.append(",\n".join(chain_sql) + ";")
    sql.append("")
    sql.append("-- +migrate Down")
    sql.append("DELETE FROM cosmosAgentMissions;")
    sql.append("DROP TABLE IF EXISTS cosmosAgentMissions;")
    sql.append("DELETE FROM agtMissions WHERE id BETWEEN %d AND %d;" % (FIRST_ID, mid - 1))
    sql.append("DELETE FROM missionObjectives WHERE missionID BETWEEN %d AND %d;" % (FIRST_ID, mid - 1))
    sql.append("")

    io.open(OUT, "w", encoding="utf-8").write("\n".join(sql))
    print("agents", len(agents), "missions", stats, "ids", FIRST_ID, mid - 1)
    print("written", OUT, os.path.getsize(OUT), "bytes")


if __name__ == "__main__":
    main()
