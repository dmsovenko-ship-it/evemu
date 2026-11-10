#!/usr/bin/env python3
"""
Build the COSMOS site seed migration (flat model: beacon + first-pocket NPC pack
+ agent placement). Data sources:
  %TEMP%/opencode/cosmos_sites/*.json   - site pages (pockets / NPC tables)
  %TEMP%/opencode/cosmos2/*.json        - missions (agent locations/factions)
  %TEMP%/opencode/agents_cosmos.json    - agentID -> home system
  %TEMP%/opencode/q_systems.tsv         - solarSystemName -> solarSystemID
  %TEMP%/opencode/q_npcs.tsv            - NPC typeName -> typeID
  %TEMP%/opencode/q_faccorp.tsv         - factionID -> corporationID
Output:
  sql/migrations/20261106000024-cosmos_sites.sql
"""
import io
import json
import os
import random
import re

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
OUT = r"C:\opencode-projects\evemu\sql\migrations\20261106000024-cosmos_sites.sql"
BEACON_TYPE = 19706   # Celestial Beacon II (group 310 Beacon, client-known)

FACTIONS = {
    "caldari": 500001, "minmatar": 500002, "amarr": 500003, "gallente": 500004,
    "concord": 500006, "ammatar": 500007, "khanid": 500008, "syndicate": 500009,
    "guristas": 500010, "angel": 500011, "blood raider": 500012, "sansha": 500019,
    "serpentis": 500020, "rogue drone": 500022, "thukker": 500002,
}


def esc(s):
    s = (s or "").replace("\\", "\\\\").replace("'", "''")
    return "'" + s + "'"


def load_tsv(name, key_col=1, val_col=0):
    d = {}
    p = os.path.join(B, name)
    if not os.path.exists(p):
        return d
    for line in io.open(p, encoding="utf-8", errors="replace"):
        f = line.rstrip().split("\t")
        if len(f) > max(key_col, val_col) and f[val_col].strip():
            key = f[key_col].strip().lstrip("\ufeff").lower()
            val = f[val_col].strip().lstrip("\ufeff")
            d[key] = int(val) if val.isdigit() else val
    return d


FACTION_PREFIX = {
    500001: "Caldari State", 500002: "Minmatar Republic", 500003: "Amarr Empire",
    500004: "Gallente Federation", 500006: "CONCORD", 500007: "Amarr Empire",
    500008: "Khanid", 500009: "Serpentis", 500010: "Guristas", 500011: "Angel Cartel",
    500012: "Blood Raiders", 500019: "Sansha's Nation", 500020: "Serpentis",
    500022: "Rogue Drone",
}

CLASS_MAP = {
    "frigate": "Frigate", "destroyer": "Destroyer", "cruiser": "Cruiser",
    "battlecruiser": "BattleCruiser", "battleship": "Battleship",
    "elite frigate": "Commander Frigate", "elite cruiser": "Commander Cruiser",
    "elite battleship": "Commander Battleship",
    "commander battleship": "Commander Battleship",
    "overseer cruiser": "Cruiser", "industrial": "Hauler",
    "drone": "Drone", "sentry": "Sentry", "sentries": "Sentry",
}

# groupName -> representative typeID (from the live DB)
_groups = {}
_gp = os.path.join(B, "q_groups.tsv")
if os.path.exists(_gp):
    for line in io.open(_gp, encoding="utf-8", errors="replace"):
        f = line.rstrip("\n").split("\t")
        if len(f) == 2 and f[1].strip().isdigit():
            _groups[f[0].strip()] = int(f[1])


def resolve_npc(name, cls, faction, types):
    """exact type name, else a faction+class representative NPC type"""
    tid = types.get((name or "").strip().lower(), 0)
    if tid:
        return tid
    suffix = CLASS_MAP.get((cls or "").strip().lower())
    prefix = FACTION_PREFIX.get(faction, "")
    if not suffix or not prefix:
        return 0
    return _groups.get("Asteroid %s %s" % (prefix, suffix), 0)


def clean_item_name(n):
    n = (n or "").replace("\ufffd", "'").replace("\u2019", "'").replace("?", "'")
    n = re.sub(r"<[^>]+>", "", n)
    n = re.sub(r"^\s*(?:drops?|you to have|requires?)\s+", "", n, flags=re.I)
    n = re.sub(r"\s+", " ", n).strip(" .,;:")
    return n


def parse_drop_items(txt):
    """'10 Jedon Hekkiren's Belongings (500 m3) and X (1 m3)' -> [(name, qty)]"""
    out = []
    for part in re.split(r"\s+and\s+|,\s*", txt or ""):
        m = re.match(r"\s*(\d[\d,]*)\s*[x\u00d7]?\s+([^(]+?)\s*(?:\(|$)", part)
        if m:
            out.append((clean_item_name(m.group(2)), int(m.group(1).replace(",", ""))))
            continue
        m2 = re.match(r"\s*([A-Za-z][^(]{3,})?\s*(?:\(|$)", part)
        if m2 and m2.group(1):
            out.append((clean_item_name(m2.group(1)), 1))
    return out


def faction_id(name):
    n = (name or "").lower()
    for k, v in FACTIONS.items():
        if k in n:
            return v
    return 0


def main():
    systems = load_tsv("q_systems.tsv", 1, 0)              # name -> id
    npc_types = load_tsv("q_npcs.tsv", 1, 0)               # name -> typeID
    item_types = load_tsv("q_pd2.tsv", 1, 0)               # item name -> typeID
    fac_corp = load_tsv("q_faccorp.tsv", 0, 1)             # factionID(str) -> corp

    agents = json.load(io.open(os.path.join(B, "agents_cosmos.json"), encoding="utf-8"))
    agent_by_id = {a["id"]: a for a in agents}

    # ---- combat sites from the constellation "COSMOS sites" tables ----
    sites = {}
    raw_sites = 0
    for fn in os.listdir(os.path.join(B, "cosmos_sites")):
        for s in json.load(io.open(os.path.join(B, "cosmos_sites", fn), encoding="utf-8")):
            raw_sites += 1
            sysname = (s.get("system") or "").split("(")[0].strip()
            sysid = systems.get(sysname.lower(), 0)
            if not sysid:
                continue
            name = s["name"].strip()
            key = (sysid, name.lower())
            if key in sites:
                continue
            fid = faction_id(s.get("faction"))
            packs = []
            drops = []
            gate_keys = []
            for pi, pocket in enumerate(s.get("pockets", []), 1):
                rows = {}
                # loot/cargo items dropped by the pocket's NPCs & structures
                for r in pocket.get("npcs", []) + pocket.get("structures", []):
                    fld = r.get("fields") or {}
                    for k in ("drop", "cargo"):
                        for nm, q in parse_drop_items(fld.get(k, "")):
                            tid = item_types.get(nm.lower(), 0)
                            if tid:
                                drops.append((pi, tid, min(q, 50)))
                # passkey needed by the gate leaving this pocket
                pkey = (0, 0)
                for r in pocket.get("gates", []) + pocket.get("structures", []) + pocket.get("npcs", []):
                    fld = r.get("fields") or {}
                    for k in ("note", "cargo", "drop"):
                        for nm, q in parse_drop_items(fld.get(k, "")):
                            low = nm.lower()
                            if "pass" in low or "key" in low:
                                tid = item_types.get(nm.lower(), 0)
                                if tid:
                                    pkey = (tid, q)
                                    break
                        if pkey[0]:
                            break
                    if pkey[0]:
                        break
                if not pkey[0]:
                    for note in pocket.get("notes", []):
                        for nm, q in parse_drop_items(note):
                            low = nm.lower()
                            if "pass" in low or "key" in low:
                                tid = item_types.get(nm.lower(), 0)
                                if tid:
                                    pkey = (tid, q)
                                    break
                        if pkey[0]:
                            break
                if pkey[0]:
                    gate_keys.append((pi, pkey[0], max(1, pkey[1])))
                for r in pocket.get("npcs", []):
                    nm = r.get("names", "").split("/")[0]
                    nm = re.sub(r"<[^>]+>", "", nm).strip()
                    qty = re.sub(r"[^0-9]", "", r.get("count", "") or "")
                    if not qty:
                        continue
                    tid = resolve_npc(nm, r.get("class"), fid, npc_types)
                    if tid:
                        rows[tid] = rows.get(tid, 0) + min(int(qty), 20)
                if rows:
                    packs.append((pi, rows))
            sites[key] = {"name": name, "systemID": sysid, "factionID": fid,
                          "type": s.get("type", ""), "rating": s.get("rating", ""),
                          "shipLimit": s.get("shipLimit", ""), "packs": packs,
                          "drops": drops, "gateKeys": gate_keys}

    # ---- agent sites (from the agents table "Location" column) ----
    agent_sites = {}
    ajson = []
    for fn in os.listdir(os.path.join(B, "cosmos2")):
        ajson.extend(json.load(io.open(os.path.join(B, "cosmos2", fn), encoding="utf-8")))
    seen_agents = set()
    for m in ajson:
        a = agent_by_id.get(0)
        aname = m.get("agent")
        match = [x for x in agents if x["name"] == aname]
        if not match:
            continue
        ag = match[0]
        if ag["id"] in seen_agents:
            continue
        seen_agents.add(ag["id"])
        loc = (m.get("agentLocation") or "").strip()
        if not loc or "#" in loc or "|" in loc or len(loc) > 80:
            continue
        site_name = loc
        if " - " in loc:
            prefix = loc.split(" - ")[0].strip()
            sysid = systems.get(prefix.lower(), 0)
            if sysid and sysid == ag["system"]:
                site_name = loc.split(" - ", 1)[1].strip()
        fid = faction_id(m.get("agentFaction") or m.get("faction"))
        key = (ag["system"], site_name.lower())
        if key not in sites:
            sites[key] = {"name": site_name, "systemID": ag["system"], "factionID": fid,
                          "type": "Agent Site", "rating": "", "shipLimit": "", "packs": []}
        agent_sites[ag["id"]] = key

    # ---- positional offsets (deterministic per site) ----
    ordered = sorted(sites.items(), key=lambda kv: (kv[1]["systemID"], kv[1]["name"]))
    site_rows = []
    npc_rows = []
    drop_rows = []
    gate_rows = []
    site_ids = {}
    for idx, (key, s) in enumerate(ordered, 1):
        site_ids[key] = idx
        rnd = random.Random(1000 + idx)
        radius = rnd.uniform(2.0e11, 7.0e11)       # 1.3 .. 4.7 AU from the star
        theta = rnd.uniform(0.0, 6.28318)
        phi = rnd.uniform(-1.5, 1.5)
        ox = int(radius * __import__("math").sin(theta) * __import__("math").cos(phi))
        oy = int(radius * __import__("math").sin(theta) * __import__("math").sin(phi))
        oz = int(radius * __import__("math").cos(theta))
        corp = fac_corp.get(str(s["factionID"]), 1) if s["factionID"] else 1
        site_rows.append((idx, s["name"], s["systemID"], s["factionID"], corp,
                          s["type"], s["rating"], s["shipLimit"], BEACON_TYPE, ox, oy, oz))
        for pi, rows in s["packs"]:
            for tid, qty in rows.items():
                npc_rows.append((idx, pi, int(tid), int(qty)))
        for pi, tid, qty in s.get("drops", []):
            drop_rows.append((idx, pi, int(tid), int(qty)))
        for pi, tid, qty in s.get("gateKeys", []):
            gate_rows.append((idx, pi, int(tid), int(qty)))

    agent_rows = []
    for aid, key in agent_sites.items():
        if key in site_ids:
            agent_rows.append((aid, site_ids[key]))

    # dedupe loot rows (same item can drop from several NPCs in one pocket)
    dd = {}
    for sid, pkt, tid, qty in drop_rows:
        dd[(sid, pkt, tid)] = dd.get((sid, pkt, tid), 0) + qty
    drop_rows = [(k[0], k[1], k[2], min(50, v)) for k, v in sorted(dd.items())]

    sql = []
    sql.append("-- COSMOS sites from the EVE University wiki (flat model: beacon +")
    sql.append("-- first-pocket NPC pack; agents stand at their own site beacon).")
    sql.append("-- +migrate Up")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosSites (")
    sql.append("  siteID int(10) unsigned NOT NULL,")
    sql.append("  siteName varchar(120) NOT NULL,")
    sql.append("  systemID int(10) unsigned NOT NULL,")
    sql.append("  factionID int(10) unsigned NOT NULL DEFAULT 0,")
    sql.append("  corporationID int(10) unsigned NOT NULL DEFAULT 0,")
    sql.append("  siteType varchar(20) NOT NULL DEFAULT '',")
    sql.append("  rating varchar(10) NOT NULL DEFAULT '',")
    sql.append("  shipLimit varchar(60) NOT NULL DEFAULT '',")
    sql.append("  beaconTypeID int(10) unsigned NOT NULL DEFAULT 19706,")
    sql.append("  ox bigint NOT NULL DEFAULT 0, oy bigint NOT NULL DEFAULT 0, oz bigint NOT NULL DEFAULT 0,")
    sql.append("  PRIMARY KEY (siteID), KEY systemID (systemID)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosSiteNpcs (")
    sql.append("  siteID int(10) unsigned NOT NULL,")
    sql.append("  pocket tinyint(3) unsigned NOT NULL,")
    sql.append("  typeID int(10) unsigned NOT NULL,")
    sql.append("  qty smallint(5) unsigned NOT NULL DEFAULT 1,")
    sql.append("  PRIMARY KEY (siteID, pocket, typeID)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosAgentSites (")
    sql.append("  agentID int(10) unsigned NOT NULL,")
    sql.append("  siteID int(10) unsigned NOT NULL,")
    sql.append("  PRIMARY KEY (agentID), KEY siteID (siteID)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosSiteDrops (")
    sql.append("  siteID int(10) unsigned NOT NULL,")
    sql.append("  pocket tinyint(3) unsigned NOT NULL,")
    sql.append("  typeID int(10) unsigned NOT NULL,")
    sql.append("  qty smallint(5) unsigned NOT NULL DEFAULT 1,")
    sql.append("  PRIMARY KEY (siteID, pocket, typeID)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("CREATE TABLE IF NOT EXISTS cosmosSiteGates (")
    sql.append("  siteID int(10) unsigned NOT NULL,")
    sql.append("  pocket tinyint(3) unsigned NOT NULL,")
    sql.append("  passkeyTypeID int(10) unsigned NOT NULL,")
    sql.append("  passkeyQty smallint(5) unsigned NOT NULL DEFAULT 1,")
    sql.append("  PRIMARY KEY (siteID, pocket)")
    sql.append(") ENGINE=InnoDB;")
    sql.append("DELETE FROM cosmosSiteDrops;")
    sql.append("DELETE FROM cosmosSiteGates;")
    sql.append("DELETE FROM cosmosSiteNpcs;")
    sql.append("DELETE FROM cosmosAgentSites;")
    sql.append("DELETE FROM cosmosSites;")
    sql.append("")

    def chunks(rows, n):
        for i in range(0, len(rows), n):
            yield rows[i:i + n]

    for part in chunks(site_rows, 200):
        vals = ",\n".join("(%d,%s,%d,%d,%d,%s,%s,%s,%d,%d,%d,%d)" % (
            r[0], esc(r[1]), r[2], r[3], r[4], esc(r[5]), esc(r[6]), esc(r[7]), r[8], r[9], r[10], r[11])
            for r in part)
        sql.append("INSERT INTO cosmosSites (siteID, siteName, systemID, factionID, corporationID,")
        sql.append("  siteType, rating, shipLimit, beaconTypeID, ox, oy, oz) VALUES")
        sql.append(vals + ";")
    if npc_rows:
        for part in chunks(npc_rows, 400):
            vals = ",\n".join("(%d,%d,%d,%d)" % r for r in part)
            sql.append("INSERT INTO cosmosSiteNpcs (siteID, pocket, typeID, qty) VALUES")
            sql.append(vals + ";")
    if agent_rows:
        for part in chunks(agent_rows, 400):
            vals = ",\n".join("(%d,%d)" % r for r in part)
            sql.append("INSERT INTO cosmosAgentSites (agentID, siteID) VALUES")
            sql.append(vals + ";")
    if drop_rows:
        for part in chunks(drop_rows, 400):
            vals = ",\n".join("(%d,%d,%d,%d)" % r for r in part)
            sql.append("INSERT INTO cosmosSiteDrops (siteID, pocket, typeID, qty) VALUES")
            sql.append(vals + ";")
    if gate_rows:
        for part in chunks(gate_rows, 400):
            vals = ",\n".join("(%d,%d,%d,%d)" % r for r in part)
            sql.append("INSERT INTO cosmosSiteGates (siteID, pocket, passkeyTypeID, passkeyQty) VALUES")
            sql.append(vals + ";")
    sql.append("")
    sql.append("-- +migrate Down")
    sql.append("DELETE FROM cosmosAgentSites;")
    sql.append("DELETE FROM cosmosSiteDrops;")
    sql.append("DELETE FROM cosmosSiteGates;")
    sql.append("DELETE FROM cosmosSiteNpcs;")
    sql.append("DELETE FROM cosmosSites;")
    sql.append("")

    io.open(OUT, "w", encoding="utf-8").write("\n".join(sql))
    print("raw sites", raw_sites, "seeded sites", len(site_rows),
          "npc rows", len(npc_rows), "agent links", len(agent_rows),
          "bytes", os.path.getsize(OUT))


if __name__ == "__main__":
    main()
