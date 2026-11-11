#!/usr/bin/env python3
"""
Fill the remaining epic-arc briefings/objectives from the jowen epic-arc guides
(eve-files.com/media/corp/jowen/*_epic_arc_guide_*.pdf, text pre-extracted to
%TEMP%/opencode/arc_pdf_txt).

The guides quote the in-game 'Mission briefing:' blocks. We only fill rows that
are empty in missionObjectives, so curated wiki/localization texts stay as-is.
"""
import io
import json
import os
import re

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
ROOT = r"C:\opencode-projects\evemu"
OUT = os.path.join(ROOT, "sql", "migrations", "20261106000043-arc_briefings_guides.sql")


def norm(s):
    s = (s or "").replace("\u2019", "'").replace("\ufffd", "'")
    return re.sub(r"[^A-Za-z0-9]+", " ", s).lower().strip()


def clean(t):
    t = t.replace("?name?", "<Character>").replace("?Name?", "<Character>")
    t = re.sub(r"\s*-\s*", "-", t)
    return re.sub(r"\s+", " ", t).strip()


def esc(s):
    return "'" + s.replace("\\", "\\\\").replace("'", "''") + "'"


PAT = re.compile(
    r"Mission name:\s*(.*?)\s*Agent name:\s*(.*?)\s*Mission type:\s*(.*?)\s*"
    r"(?:Acceleration gate restrictions[^:]*:[^:]*\s*)?(?:Accept remotely:[^:]*\s*)?"
    r"(?:Complete remotely:[^:]*\s*)?Mission briefing:\s*(.*?)\s*Objective:\s*(.*?)\s*"
    r"(?:Notes/hints:|Missions:|Warning:|Mission name:|$)", re.S | re.I)


def main():
    txtdir = os.path.join(B, "arc_pdf_txt")
    found = {}
    for fn in sorted(os.listdir(txtdir)):
        if not fn.endswith(".txt"):
            continue
        c = re.sub(r"\s+", " ", io.open(os.path.join(txtdir, fn), encoding="utf-8", errors="replace").read())
        for m in PAT.finditer(c):
            name, agent, typ, brief, obj = [x.strip() for x in m.groups()]
            if len(name) < 4 or len(brief) < 40:
                continue
            key = norm(name)
            if key and key not in found:
                found[key] = {"name": name, "type": typ, "briefing": brief, "objective": obj}

    db = {}
    for line in io.open(os.path.join(B, "q_arc_missions.tsv"), encoding="utf-8", errors="replace"):
        f = line.rstrip("\n").split("\t")
        if len(f) >= 2 and f[0].strip().isdigit():
            db[norm(f[1])] = int(f[0])

    sql = []
    sql.append("-- Epic-arc briefings/objectives from the jowen epic-arc guides")
    sql.append("-- (only rows that are still empty in missionObjectives).")
    sql.append("-- +migrate Up")
    rows = 0
    for key, v in sorted(found.items(), key=lambda kv: kv[1]["name"]):
        mid = db.get(key)
        if not mid:
            continue
        brief = clean(v["briefing"])
        obj = clean(v["objective"])
        if len(brief) >= 40:
            q = "UPDATE missionObjectives SET briefingText=%s WHERE missionID=%d AND (briefingText IS NULL OR briefingText='');" % (esc(brief), mid)
            sql.append(q)
        if obj:
            q = "UPDATE missionObjectives SET objectiveText=%s WHERE missionID=%d AND (objectiveText IS NULL OR objectiveText='');" % (esc(obj), mid)
            sql.append(q)
        rows += 1
    sql.append("-- +migrate Down")
    sql.append("-- (text stays)")
    sql.append("")
    io.open(OUT, "w", encoding="utf-8").write("\n".join(sql))
    print("guide missions", len(found), "matched SQL missions", rows, "->", OUT)


if __name__ == "__main__":
    main()
