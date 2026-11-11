#!/usr/bin/env python3
"""
Extract COSMOS mission briefings from the community COSMOS guide PDFs
(static.adam4eve.eu/Guides/Cosmos/, text already extracted to
%TEMP%/opencode/cosmos_pdf_txt) and emit the seed migration.

The jowen_datloran guides use blocks:
  Mission name: <name> Mission type: <type> Mission briefing: <text> Objective: <text> Notes/hints: ...
"""
import io
import json
import os
import re

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
ROOT = r"C:\opencode-projects\evemu"
OUT = os.path.join(ROOT, "sql", "migrations", "20261106000041-cosmos_briefings_guides.sql")


def norm(s):
    s = (s or "").replace("\u2019", "'").replace("\ufffd", "'").replace("?", "'")
    return re.sub(r"[^A-Za-z0-9]+", " ", s).lower().strip()


def clean(t):
    t = t.replace("?name?", "<Character>").replace("?Name?", "<Character>")
    t = re.sub(r"\s*-\s*", "-", t)
    return re.sub(r"\s+", " ", t).strip()


def esc(s):
    return "'" + s.replace("\\", "\\\\").replace("'", "''") + "'"


def main():
    txtdir = os.path.join(B, "cosmos_pdf_txt")
    pat = re.compile(
        r"Mission name:\s*(.*?)\s*Mission type:\s*(.*?)\s*Mission briefing:\s*(.*?)\s*Objective:\s*(.*?)\s*"
        r"(?:Notes/hints:|Mission name:|$)", re.S | re.I)
    found = {}
    for fn in sorted(os.listdir(txtdir)):
        if not fn.endswith(".txt"):
            continue
        c = re.sub(r"\s+", " ", io.open(os.path.join(txtdir, fn), encoding="utf-8", errors="replace").read())
        for m in pat.finditer(c):
            name, typ, brief, obj = m.group(1).strip(), m.group(2).strip(), m.group(3).strip(), m.group(4).strip()
            if len(name) < 4 or len(brief) < 60:
                continue
            key = norm(name)
            if key and key not in found:
                found[key] = {"name": name, "briefing": brief, "objective": obj, "type": typ}

    db = {}
    for line in io.open(os.path.join(B, "q_cosmos_missions.tsv"), encoding="utf-8", errors="replace"):
        f = line.rstrip("\n").split("\t")
        if len(f) >= 2 and f[0].strip().isdigit():
            db[norm(f[1])] = int(f[0])

    sql = []
    sql.append("-- COSMOS mission briefings from the community COSMOS guides (adam4eve archive).")
    sql.append("-- These fill the missions whose client/wiki pages carry no briefing text.")
    sql.append("-- +migrate Up")
    rows = 0
    for key, v in sorted(found.items(), key=lambda kv: kv[1]["name"]):
        mid = db.get(key)
        if not mid:
            continue
        brief = clean(v["briefing"])
        obj = clean(v["objective"])
        if len(brief) < 60:
            continue
        q = "UPDATE missionObjectives SET briefingText=%s" % esc(brief)
        if obj:
            q += ", objectiveText=IF(objectiveText='', %s, objectiveText)" % esc(obj)
        q += " WHERE missionID=%d AND (briefingText IS NULL OR briefingText='');" % mid
        sql.append(q)
        rows += 1
    sql.append("-- +migrate Down")
    sql.append("-- (briefings stay)")
    sql.append("")
    io.open(OUT, "w", encoding="utf-8").write("\n".join(sql))
    print("extracted", len(found), "matched+updated SQL rows", rows, "->", OUT)


if __name__ == "__main__":
    main()
