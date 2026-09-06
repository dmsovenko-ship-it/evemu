#!/usr/bin/env python3
"""
Import real EVE fittings (Crucible-era) from an EDK killboard (kb.sotzone.ru)
into botKillmailLegends so chelobots fly real fits.

The EDK renders each kill's fitted modules as EFT text via
`?a=eft_fitting&kll_id=N`. We scrape monthly kill listings, pull the EFT fit of
the victim, resolve module names to typeIDs against the local invTypes, and
store them (reusing the existing botKillmailLegends schema). Modules that don't
exist in our (Crucible) SDE are skipped.

Usage:
    python tools/import_edk_fits.py --db-host 127.0.0.1 --db-user evemu \
        --db-pass evemu --db-name evemu [--year 2011] [--month 12] [--sleep 0.3]
    (omit --year/--month to scan all months from --start-year..--end-year)
"""

import argparse
import json
import re
import sys
import time
import urllib.parse
import urllib.request

try:
    import pymysql
except ImportError:
    sys.exit("pip install pymysql")

BASE = "https://kb.sotzone.ru/"
UA = {"User-Agent": "Mozilla/5.0 (EVEmu fit importer)"}


def http_get(url):
    req = urllib.request.Request(url, headers=UA)
    return urllib.request.urlopen(req, timeout=30).read().decode("utf-8", "replace")


def month_kill_ids(year, month, sleep=0.2):
    """Yield all kll_id values listed for a year/month across its pages."""
    page = 1
    while True:
        url = "%s?a=kills&y=%d&m=%d&page=%d" % (BASE, year, month, page)
        html = http_get(url)
        ids = sorted(set(int(x) for x in re.findall(r"kll_id=(\d+)", html)))
        # Last page if no rows or no 'next page' link to page+1
        next_link = "&page=%d" % (page + 1) in html.replace("&amp;", "&")
        if not ids:
            return
        for k in ids:
            yield k
        if not next_link:
            return
        page += 1
        time.sleep(sleep)


def parse_eft_fit(text):
    """Parse EFT fitting block into ordered list of module names (fitted slots).
    EFT groups modules by slot separated by blank lines; we keep every non-blank
    module line, skipping the [Ship, Pilot] header."""
    names = []
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            continue
        # strip a possible trailing " xN" from EDK qty? eft has plain names
        names.append(line)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db-host", default="127.0.0.1")
    ap.add_argument("--db-port", type=int, default=3306)
    ap.add_argument("--db-user", required=True)
    ap.add_argument("--db-pass", required=True)
    ap.add_argument("--db-name", required=True)
    ap.add_argument("--year", type=int, default=0)
    ap.add_argument("--month", type=int, default=0)
    ap.add_argument("--start-year", type=int, default=2007)
    ap.add_argument("--end-year", type=int, default=2011)
    ap.add_argument("--sleep", type=float, default=0.25)
    args = ap.parse_args()

    db = pymysql.connect(host=args.db_host, port=args.db_port,
                         user=args.db_user, password=args.db_pass,
                         database=args.db_name, charset="utf8mb4",
                         autocommit=True)
    cur = db.cursor()
    cur.execute("""CREATE TABLE IF NOT EXISTS botKillmailLegends (
        id INT UNSIGNED NOT NULL AUTO_INCREMENT,
        killmail_id BIGINT UNSIGNED NOT NULL,
        character_id INT UNSIGNED NOT NULL,
        character_name VARCHAR(128) NOT NULL DEFAULT '',
        corporation_id INT UNSIGNED NOT NULL DEFAULT 0,
        alliance_id INT UNSIGNED NOT NULL DEFAULT 0,
        ship_type_id INT UNSIGNED NOT NULL DEFAULT 0,
        fitted_item_ids TEXT NULL,
        security_status FLOAT NOT NULL DEFAULT 0,
        kill_time DATETIME NOT NULL,
        points INT UNSIGNED NOT NULL DEFAULT 0,
        PRIMARY KEY (id),
        UNIQUE KEY uqm_killmail_char (killmail_id, character_id),
        KEY idx_character (character_id),
        KEY idx_ship (ship_type_id)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""")

    # typeName -> typeID from our local (Crucible) SDE
    cur.execute("SELECT typeName, typeID FROM invTypes WHERE published=1")
    typeid_by_name = {}
    for name, tid in cur.fetchall():
        typeid_by_name.setdefault(str(name).strip(), tid)
    print("Loaded %d published types." % len(typeid_by_name))

    # Resume support: remember killmail_ids we already processed so a restarted
    # run skips them (both detail fetch and DB insert are idempotent).
    cur.execute("""CREATE TABLE IF NOT EXISTS edkProcessedKills (
        killmail_id BIGINT UNSIGNED NOT NULL PRIMARY KEY) ENGINE=InnoDB""")
    cur.execute("SELECT killmail_id FROM edkProcessedKills")
    done = set(r[0] for r in cur.fetchall())
    print("Resume: %d killmails already processed." % len(done))

    # Decide which months to scan
    months = []
    if args.year and args.month:
        months = [(args.year, args.month)]
    else:
        for y in range(args.start_year, args.end_year + 1):
            for m in range(1, 13):
                months.append((y, m))

    insert_sql = """
        INSERT INTO botKillmailLegends
        (killmail_id, character_id, character_name, corporation_id,
         alliance_id, ship_type_id, fitted_item_ids, security_status,
         kill_time, points)
        VALUES (%s,%s,%s,%s,%s,%s,%s,0,NOW(),0)
        ON DUPLICATE KEY UPDATE
          character_name = VALUES(character_name),
          ship_type_id = VALUES(ship_type_id),
          fitted_item_ids = VALUES(fitted_item_ids)
    """

    total_new = 0
    total_upd = 0
    seen = 0
    for y, m in months:
        ids = list(month_kill_ids(y, m, sleep=args.sleep))
        todo = [k for k in ids if k not in done]
        print("[%d-%02d] %d kills (%d new)" % (y, m, len(ids), len(todo)), flush=True)
        for kll in todo:
            seen += 1
            try:
                detail = http_get("%s?a=kill_detail&kll_id=%d" % (BASE, kll))
                eft = http_get("%s?a=eft_fitting&kll_id=%d" % (BASE, kll))
            except Exception as e:
                print("  kll %d fetch err %s" % (kll, e), file=sys.stderr)
                time.sleep(args.sleep)
                continue
            # Mark processed even on HTTP errors so we don't retry forever.
            try:
                cur.execute("INSERT IGNORE INTO edkProcessedKills (killmail_id) VALUES (%s)", (kll,))
                done.add(kll)
            except Exception:
                pass
            mods = parse_eft_fit(eft)
            if not mods:
                time.sleep(args.sleep)
                continue
            # victim identity from the detail page (href uses plain '&')
            vm = re.search(r"pilot_detail&plt_id=(\d+)", detail)
            char_id = int(vm.group(1)) if vm else 0
            nm = re.search(r"Victim:</b></td>\s*<td class=kb-table-cell><b><a[^>]*>([^<]+)</a></b>", detail, re.S)
            vname = nm.group(1).strip() if nm else ""
            ship_m = re.search(r"<b>Ship:</b></td>\s*<td class=kb-table-cell><b>([^<]*)</b>", detail, re.S)
            ship_name = ship_m.group(1).strip() if ship_m else ""
            corp_m = re.search(r"Corp:</b></td>\s*<td class=kb-table-cell><b><a[^>]*>([^<]+)</a></b>", detail, re.S)
            corp_name = corp_m.group(1).strip() if corp_m else ""
            ally_m = re.search(r"Alliance:</b></td>\s*<td class=kb-table-cell><b><a[^>]*>([^<]+)</a></b>", detail, re.S)
            ally_name = ally_m.group(1).strip() if ally_m else ""
            if not char_id or not vname or not ship_name:
                time.sleep(args.sleep)
                continue
            ship_type = typeid_by_name.get(ship_name, 0)
            # map module names -> typeIDs (skip unknown / ammo/cargo noise we can't map)
            fit = []
            for mn in mods:
                tid = typeid_by_name.get(mn, 0)
                if tid:
                    fit.append(tid)
            if not fit:
                time.sleep(args.sleep)
                continue
            try:
                cur.execute(insert_sql,
                    (kll, char_id, vname, 0, 0, ship_type, json.dumps(fit)))
                if cur.rowcount == 1:
                    total_new += 1
                elif cur.rowcount == 2:
                    total_upd += 1
            except Exception as e:
                print("  insert err %s: %s" % (kll, e), file=sys.stderr)
            if seen % 25 == 0:
                print("  ... %d processed, new=%d upd=%d" % (seen, total_new, total_upd), flush=True)
            time.sleep(args.sleep)
    db.close()
    print("Done. new=%d updated=%d (processed %d)." % (total_new, total_upd, seen))


if __name__ == "__main__":
    main()
