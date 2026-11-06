#!/usr/bin/env python3
"""
Build mission objective candidates from the client localization texts.

The Crucible client ships every mission briefing as a messageID -> text entry in
resLocalization.stuff (see tools/extract_localization.py). Epic arc / COSMOS
missions in our DB have no briefingID, so this tool:
  1. scores localization texts against a mission's name (distinctive words),
  2. classifies the best matching text (travel / courier / hunt),
  3. emits candidate rows for `missionObjectives` (review before applying).

Input missions file: TSV with `missionID<TAB>name` (e.g. produced from the DB:
  SELECT CONCAT(missionID, '\t', missionName) FROM epicArcMission WHERE arcID=1;
  SELECT CONCAT(id, '\t', name) FROM agtMissions WHERE typeID=9;).

Usage:
  python build_mission_objectives.py --loc loc_en.json --missions arc1.tsv \
      [--minlen 120] [--top 3] [--sql out.sql] [--csv review.tsv]
"""
import argparse
import json
import re

STOP = set("""the and for from with your you have this that will into when what mission agent pilot ship
space find their them they there here about would could should been being was were are his her its our
your all any some more most other than then these those such only also just very much many""".split())

COURIER_RE = re.compile(r"\b(deliver|bring|transport|haul|shipment|courier|pick up|pickup|cargo)\b", re.I)
HUNT_RE = re.compile(r"\b(destroy|eliminate|kill|clear out|wipe out|assault|engage|attack|hunt down)\b", re.I)
TRAVEL_RE = re.compile(r"\b(travel to|go to|head to|visit|meet|speak with|talk to|find|locate|scan|investigate|report to)\b", re.I)
QTY_RE = re.compile(r"\b(\d{1,6})\s+(?:units? of\s+)?([A-Z][A-Za-z0-9' \-]{2,40})")


def words(text):
    return [w.lower() for w in re.findall(r"[A-Za-z]{4,}", text) if w.lower() not in STOP]


def classify(text):
    if QTY_RE.search(text) and COURIER_RE.search(text):
        return 2
    if COURIER_RE.search(text):
        return 2
    if HUNT_RE.search(text):
        return 3
    if TRAVEL_RE.search(text):
        return 1
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--loc", required=True, help="localization JSON from extract_localization.py")
    ap.add_argument("--missions", required=True, help="TSV: missionID<TAB>name")
    ap.add_argument("--minlen", type=int, default=120)
    ap.add_argument("--top", type=int, default=3)
    ap.add_argument("--sql", help="write candidate INSERTs here")
    ap.add_argument("--csv", help="write a review table here")
    args = ap.parse_args()

    with open(args.loc, encoding="utf-8") as f:
        loc = {int(k): v for k, v in json.load(f).items()}
    long_texts = {mid: t for mid, t in loc.items() if len(t) >= args.minlen}

    missions = []
    with open(args.missions, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            if "\t" in line:
                mid, name = line.split("\t", 1)
            elif r"\t" in line:
                mid, name = line.split(r"\t", 1)
            else:
                continue
            missions.append((int(mid), name.strip()))

    sql = []
    review = ["missionID\tname\tbestID\tscore\tobjective\ttext"]
    for mid, name in missions:
        ws = words(name)
        if not ws:
            continue
        scored = []
        for lid, text in long_texts.items():
            lt = text.lower()
            score = sum(1 for w in ws if w in lt)
            if score >= max(1, (len(ws) + 1) // 2):
                scored.append((score, len(text), lid, text))
        scored.sort(key=lambda x: (-x[0], -x[1]))
        for score, ln, lid, text in scored[:args.top]:
            obj = classify(text)
            review.append("%d\t%s\t%d\t%d\t%d\t%s" % (mid, name, lid, score, obj,
                                                       text[:200].replace("\n", " ").replace("\t", " ")))
            if len(sql) < 100000:
                sql.append("INSERT INTO missionObjectives (missionID, briefingID, objectiveType, note) "
                           "VALUES (%d, %d, %d, 'auto %s score=%d');" %
                           (mid, lid, obj, name.replace("'", "''"), score))

    if args.csv:
        with open(args.csv, "w", encoding="utf-8") as f:
            f.write("\n".join(review) + "\n")
    if args.sql:
        with open(args.sql, "w", encoding="utf-8") as f:
            f.write("\n".join(sql) + "\n")
    print("missions:", len(missions), "candidates:", len(sql))


if __name__ == "__main__":
    main()
