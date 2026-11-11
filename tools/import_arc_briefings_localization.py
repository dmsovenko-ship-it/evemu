#!/usr/bin/env python3
"""Fill the last missing epic-arc briefings from the client localization.

Some epic-arc "report to" missions have no wiki {{MissionBriefing}}; the client
localization still ships their mission text (messageIDs around 1413xx/1421xx).
"""
import io
import json
import os
import re

B = r"C:\Users\Dima\AppData\Local\Temp\opencode"
OUT = r"C:\opencode-projects\evemu\sql\migrations\20261106000040-arc_briefings_from_localization.sql"

d = json.load(io.open(os.path.join(B, "loc_en.json"), encoding="utf-8"))

# missionID -> localization messageID (verified against the mission objective)
picks = {
    80101: 141410,   # Aralin Jick  - "talk to ... Aralin Jick"
    80124: 141355,   # An Honorable Betrayal - meeting Arikio Kuretsu
    80140: 141351,   # Slipping Away - "head back to Arikio Kuretsu"
    80143: 141319,   # Learning by Doing - earn Katsen Isha's trust
    80168: 142111,   # Everybody Has a Price - dropping Ralie off
}


def clean(text):
    text = re.sub(r"<br\s*/?>", " ", text, flags=re.I)
    text = re.sub(r"<[^>]+>", "", text)
    text = re.sub(r"\{\[[^}]*\]\}", "", text)
    text = text.replace("{[character]player.name}", "<Character>")
    text = re.sub(r"\s+", " ", text)
    return text.strip()


def esc(s):
    return "'" + s.replace("\\", "\\\\").replace("'", "''") + "'"


sql = []
sql.append("-- Final epic-arc briefings taken from the client localization")
sql.append("-- (missions whose wiki page has no MissionBriefing template).")
sql.append("-- +migrate Up")
count = 0
for mid, locid in sorted(picks.items()):
    v = d.get(str(locid))
    if not v:
        continue
    text = clean(v)
    if len(text) < 40:
        continue
    sql.append("UPDATE missionObjectives SET briefingText=%s WHERE missionID=%d;" % (esc(text), mid))
    count += 1
sql.append("-- +migrate Down")
sql.append("-- (briefings stay)")
sql.append("")
io.open(OUT, "w", encoding="utf-8").write("\n".join(sql))
print("written", OUT, "rows", count)
