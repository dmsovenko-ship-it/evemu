#!/usr/bin/env python3
"""
Seed the COSMOS site structures (wiki 'Structures' rows of every pocket) as
in-world objects: cosmosSiteStructures(siteID, pocket, typeID, qty).

Only names that resolve to a real invTypes row are used.
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

struct_types = bs.load_tsv("q_struct.tsv", 1, 0)
systems = bs.load_tsv("q_systems.tsv", 1, 0)

seed = io.open(os.path.join(ROOT, "sql", "migrations", "20261106000024-cosmos_sites.sql"), encoding="utf-8").read()
site_ids = {}
for m in re.finditer(r"INSERT INTO cosmosSites .*?VALUES(.*?);", seed, re.S):
    for row in re.finditer(r"\((\d+),'((?:[^'\\]|\\.)*)',(\d+),", m.group(1)):
        site_ids[(int(row.group(3)), row.group(2).replace("''", "'").lower())] = int(row.group(1))

rows = []
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
            counts = {}
            for r in pocket.get("structures", []):
                nm = r.get("names", "").split("/")[0]
                nm = re.sub(r"\[\[([^\]|]+)(?:\|[^\]]*)?\]\]", r"\1", nm)
                nm = re.sub(r"<[^>]+>", "", nm).strip()
                tid = struct_types.get(nm.lower(), 0)
                if tid:
                    counts[tid] = counts.get(tid, 0) + 1
            for tid, qty in counts.items():
                rows.append((sid, pi, int(tid), min(6, int(qty))))

dd = {}
for sid, pkt, tid, q in rows:
    dd[(sid, pkt, tid)] = min(8, dd.get((sid, pkt, tid), 0) + q)
rows = [(k[0], k[1], k[2], v) for k, v in sorted(dd.items())]
print("structure rows", len(rows), "sites", len(set(r[0] for r in rows)))

sql = []
sql.append("-- COSMOS site structures (wiki Structures tables) - visual/landmark objects.")
sql.append("-- +migrate Up")
sql.append("CREATE TABLE IF NOT EXISTS cosmosSiteStructures (")
sql.append("  siteID int(10) unsigned NOT NULL,")
sql.append("  pocket tinyint(3) unsigned NOT NULL,")
sql.append("  typeID int(10) unsigned NOT NULL,")
sql.append("  qty tinyint(3) unsigned NOT NULL DEFAULT 1,")
sql.append("  PRIMARY KEY (siteID, pocket, typeID)")
sql.append(") ENGINE=InnoDB;")
sql.append("DELETE FROM cosmosSiteStructures;")
for part in [rows[i:i + 400] for i in range(0, len(rows), 400)]:
    vals = ",\n".join("(%d,%d,%d,%d)" % r for r in part)
    sql.append("INSERT INTO cosmosSiteStructures (siteID, pocket, typeID, qty) VALUES")
    sql.append(vals + ";")
sql.append("-- +migrate Down")
sql.append("DROP TABLE IF EXISTS cosmosSiteStructures;")
sql.append("")
out = os.path.join(ROOT, "sql", "migrations", "20261106000038-cosmos_site_structures.sql")
io.open(out, "w", encoding="utf-8").write("\n".join(sql))
print("written", out)
