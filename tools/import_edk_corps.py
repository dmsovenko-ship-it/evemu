#!/usr/bin/env python3
"""
Import real EVE corporation names (Crucible-era) from an EDK killboard
(kb.sotzone.ru) into botCorpNames, so chelobot-founded corporations get REAL
names instead of procedural ones (BotMgr::MakeCorpName picks an unused row).

It scans the monthly kill listings, fetches each kill's detail page and stores the
victim corporation's name (deduplicated, idempotent). A short ticker is derived
from the name when the board does not expose one.

Usage:
    python tools/import_edk_corps.py --db-host 127.0.0.1 --db-user evemu \
        --db-pass evemu --db-name evemu [--year 2011] [--month 12] [--sleep 0.25]
    (omit --year/--month to scan all months from --start-year..--end-year)
"""

import argparse
import re
import sys
import time
import urllib.request

try:
    import pymysql
except ImportError:
    sys.exit("pip install pymysql")

BASE = "https://kb.sotzone.ru/"
UA = {"User-Agent": "Mozilla/5.0 (EVEmu corp importer)"}


def http_get(url):
    req = urllib.request.Request(url, headers=UA)
    return urllib.request.urlopen(req, timeout=30).read().decode("utf-8", "replace")


def month_kill_ids(year, month, sleep=0.2):
    """Yield every kll_id listed for a year/month across its pages."""
    page = 1
    while True:
        html = http_get("%s?a=kills&y=%d&m=%d&page=%d" % (BASE, year, month, page))
        ids = sorted(set(int(x) for x in re.findall(r"kll_id=(\d+)", html)))
        next_link = "&page=%d" % (page + 1) in html.replace("&amp;", "&")
        if not ids:
            return
        for k in ids:
            yield k
        if not next_link:
            return
        page += 1
        time.sleep(sleep)


def parse_corp_name(detail):
    """Victim corporation name from a kill detail page ('Corp: ... <a>NAME</a>')."""
    m = re.search(r"Corp:</b></td>\s*<td class=kb-table-cell><b><a[^>]*>([^<]+)</a></b>",
                  detail, re.S)
    return m.group(1).strip() if m else ""


def make_ticker(name):
    """Short uppercase ticker derived from a corp name (the board has no ticker)."""
    words = re.findall(r"[A-Za-z0-9]+", name)
    if not words:
        return "CORP"
    if len(words) >= 2:
        t = "".join(w[0] for w in words[:4]).upper()
    else:
        t = words[0][:4].upper()
    return (t or "CORP")[:8]


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
    cur.execute("""CREATE TABLE IF NOT EXISTS botCorpNames (
        id INT UNSIGNED NOT NULL AUTO_INCREMENT,
        corpName VARCHAR(128) NOT NULL,
        ticker VARCHAR(16) NOT NULL DEFAULT '',
        used TINYINT UNSIGNED NOT NULL DEFAULT 0,
        PRIMARY KEY (id),
        UNIQUE KEY uq_corpName (corpName)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""")

    cur.execute("""CREATE TABLE IF NOT EXISTS edkProcessedCorps (
        killmail_id BIGINT UNSIGNED NOT NULL PRIMARY KEY) ENGINE=InnoDB""")
    cur.execute("SELECT killmail_id FROM edkProcessedCorps")
    done = set(r[0] for r in cur.fetchall())
    print("Resume: %d kills already scanned." % len(done))

    months = []
    if args.year and args.month:
        months = [(args.year, args.month)]
    else:
        for y in range(args.start_year, args.end_year + 1):
            for m in range(1, 13):
                months.append((y, m))

    total_new = 0
    scanned = 0
    for y, m in months:
        ids = [k for k in month_kill_ids(y, m, sleep=args.sleep) if k not in done]
        print("[%d-%02d] %d new kills" % (y, m, len(ids)), flush=True)
        for kll in ids:
            scanned += 1
            try:
                detail = http_get("%s?a=kill_detail&kll_id=%d" % (BASE, kll))
            except Exception as e:
                print("  kll %d fetch err %s" % (kll, e), file=sys.stderr)
                time.sleep(args.sleep)
                continue
            try:
                cur.execute("INSERT IGNORE INTO edkProcessedCorps (killmail_id) VALUES (%s)", (kll,))
                done.add(kll)
            except Exception:
                pass
            name = parse_corp_name(detail)
            if name and len(name) <= 128:
                try:
                    cur.execute("INSERT IGNORE INTO botCorpNames (corpName, ticker) VALUES (%s, %s)",
                                (name, make_ticker(name)))
                    if cur.rowcount == 1:
                        total_new += 1
                except Exception as e:
                    print("  insert err %s: %s" % (kll, e), file=sys.stderr)
            if scanned % 50 == 0:
                print("  ... %d scanned, %d corps" % (scanned, total_new), flush=True)
            time.sleep(args.sleep)
    db.close()
    print("Done. %d new corp names (scanned %d)." % (total_new, scanned))


if __name__ == "__main__":
    main()
