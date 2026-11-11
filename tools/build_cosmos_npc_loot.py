#!/usr/bin/env python3
"""
Build the named-NPC loot seed (migration 000037) for COSMOS pockets.

Loot rows from the wiki are attached to the NPC type that actually drops them
(when that name resolves to a real invTypes entry); otherwise they are attached
to the pocket's lead NPC (npcTypeID = 0).
"""
import importlib.util
import io
import json
import os
import re

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
ROOT = r"C:\opencode-projects\evemu"
spec = importlib.util.spec_from_file_location("bs", os.path.join(ROOT, "tools", "build_cosmos_sites_seed.py"))
bs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bs)

npc_types = bs.load_tsv("q_npcs.tsv", 1, 0)
item_types = bs.load_tsv("q_pd2.tsv", 1, 0)
systems = bs.load_tsv("q_systems.tsv", 1, 0)

seed = io.open(os.path.join(ROOT, "sql", "migrations", "20261106000024-cosmos_sites.sql"), encoding="utf-8").read()
site_ids = {}
for m in re.finditer(r"INSERT INTO cosmosSites .*?VALUES(.*?);", seed, re.S):
    for row in re.finditer(r"\((\d+),'((?:[^'\\]|\\.)*)',(\d+),", m.group(1)):
        sid = int(row.group(1))
        name = row.group(2).replace("''", "'")
        sysid = int(row.group(3))
        site_ids[(sysid, name.lower())] = sid
print("site ids", len(site_ids))

drop_rows = []
for fn in os.listdir(os.path.join(B, "cosmos_sites")):
    for s in json.load(io.open(os.path.join(B, "cosmos_sites", fn), encoding="utf-8")):
        sysname = (s.get("system") or "").split("(")[0].strip()
        sysid = systems.get(sysname.lower(), 0)
        if not sysid:
            continue
        sid = site_ids.get((sysid, s["name"].strip().lower()))
        if not sid:
            continue
        for pi, pocket in enumerate(s.get("pockets", []), 1):
            by_npc = {}
            for r in pocket.get("npcs", []):
                fld = r.get("fields") or {}
                items = []
                for k in ("drop", "cargo"):
                    for nm, q in bs.parse_drop_items(fld.get(k, "")):
                        tid = item_types.get(nm.lower(), 0)
                        if tid:
                            items.append((tid, min(int(q), 50)))
                if not items:
                    continue
                nm = r.get("names", "").split("/")[0]
                nm = re.sub(r"<[^>]+>", "", nm).strip()
                exact = npc_types.get(nm.lower(), 0)
                key = int(exact) if exact else 0
                bucket = by_npc.setdefault(key, {})
                for tid, q in items:
                    bucket[tid] = min(50, bucket.get(tid, 0) + q)
            for r in pocket.get("structures", []):
                fld = r.get("fields") or {}
                for k in ("drop", "cargo"):
                    for nm, q in bs.parse_drop_items(fld.get(k, "")):
                        tid = item_types.get(nm.lower(), 0)
                        if tid:
                            bucket = by_npc.setdefault(0, {})
                            bucket[tid] = min(50, bucket.get(tid, 0) + min(int(q), 50))
            for npcKey, items in by_npc.items():
                for tid, q in items.items():
                    drop_rows.append((sid, pi, npcKey, tid, q))

dd = {}
for sid, pkt, npcKey, tid, q in drop_rows:
    key = (sid, pkt, npcKey, tid)
    dd[key] = dd.get(key, 0) + q
drop_rows = [(k[0], k[1], k[2], k[3], min(50, v)) for k, v in sorted(dd.items())]
print("drop rows", len(drop_rows), "named", sum(1 for r in drop_rows if r[2]))

sql = []
sql.append("-- COSMOS pocket loot keyed by the NPC type that drops it (0 = lead NPC).")
sql.append("-- +migrate Up")
sql.append("ALTER TABLE cosmosSiteDrops ADD COLUMN IF NOT EXISTS npcTypeID int(10) unsigned NOT NULL DEFAULT 0;")
sql.append("DELETE FROM cosmosSiteDrops;")
for part in [drop_rows[i:i + 400] for i in range(0, len(drop_rows), 400)]:
    vals = ",\n".join("(%d,%d,%d,%d,%d)" % r for r in part)
    sql.append("INSERT INTO cosmosSiteDrops (siteID, pocket, npcTypeID, typeID, qty) VALUES")
    sql.append(vals + ";")
sql.append("-- +migrate Down")
sql.append("-- (loot stays)")
sql.append("")
out = os.path.join(ROOT, "sql", "migrations", "20261106000037-cosmos_named_npc_drops.sql")
io.open(out, "w", encoding="utf-8").write("\n".join(sql))
print("written", out)
