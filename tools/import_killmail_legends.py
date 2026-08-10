#!/usr/bin/env python3
"""
Collect real EVE player legends from zKillboard for simulated players ("bots").

Fetches killmails via the zKillboard API, extracts every pilot involved
(victim + attackers) with their ship hull and fitted modules, and stores them
in the `botKillmailLegends` table. BotMgr then builds believable player legends
(names, corps, ships, fits, skill tiers) from real EVE data.

Usage:
    python tools/import_killmail_legends.py --db-host 127.0.0.1 --db-user evemu \
        --db-pass evemu --db-name evemu [--pages N] [--max-age-days 30]

Requires: requests (pip install requests)
"""

import argparse
import json
import sys
import time

try:
    import requests
except ImportError:
    sys.exit("pip install requests")

try:
    import pymysql
except ImportError:
    sys.exit("pip install pymysql")

USER_AGENT = "EVEmuBotDataCollector/1.0 (contact: dev@example.com)"
API = "https://zkillboard.com/api/"
# Which slots are part of a fit (not cargo/ammo/drones). flag values from EVE:
#   11-15 high slots, 19-24 mid slots, 27-33 low slots, 92-94 rigs, 87 drones
FIT_FLAGS = set(range(11, 16)) | set(range(19, 25)) | set(range(27, 34)) | {87, 92, 93, 94}


def fetch_kills(requests_session, params=None):
    """GET /api/kills with pagination-by-time. Returns list of killmail dicts."""
    url = API + "kills/"
    headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
    resp = requests_session.get(url, headers=headers, params=params, timeout=60)
    resp.raise_for_status()
    return resp.json()


def extract_legend(kill):
    """Return list of (char_id, corp_id, alliance_id, ship_type, fit_json, sec, time, points)."""
    out = []
    tm = kill.get("killmail_time", "")
    points = kill.get("zkb", {}).get("points", 0)

    victim = kill.get("victim", {})
    if victim.get("character_id"):
        fit = [i["item_type_id"] for i in victim.get("items", [])
               if i.get("flag") in FIT_FLAGS]
        out.append((
            victim.get("character_id"),
            victim.get("corporation_id", 0),
            victim.get("alliance_id", 0),
            victim.get("ship_type_id", 0),
            json.dumps(fit),
            victim.get("security_status", 0),
            tm, points,
        ))

    for atk in kill.get("attackers", []):
        if atk.get("character_id"):
            out.append((
                atk.get("character_id"),
                atk.get("corporation_id", 0),
                atk.get("alliance_id", 0),
                atk.get("ship_type_id", 0),
                "[]",
                atk.get("security_status", 0),
                tm, points,
            ))
    return out


ESI_URL = "https://esi.evetech.net/latest/universe/names/?datasource=tranquility"


def fetch_names(char_ids):
    """Resolve character IDs to names via ESI universe/names (max 1000/batch)."""
    if not char_ids:
        return {}
    names = {}
    ids = list(dict.fromkeys(char_ids))   # unique, preserve order
    for i in range(0, len(ids), 1000):
        batch = ids[i:i + 1000]
        try:
            r = requests.post(ESI_URL, json=batch, timeout=60,
                              headers={"User-Agent": USER_AGENT})
            r.raise_for_status()
            for ent in r.json():
                if ent.get("category") == "character":
                    names[ent["id"]] = ent["name"]
        except Exception as e:
            print(f"[names] batch error: {e}", file=sys.stderr)
        time.sleep(0.5)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db-host", default="127.0.0.1")
    ap.add_argument("--db-port", type=int, default=3306)
    ap.add_argument("--db-user", required=True)
    ap.add_argument("--db-pass", required=True)
    ap.add_argument("--db-name", required=True)
    ap.add_argument("--pages", type=int, default=10, help="number of API pages (each ~200 kills)")
    ap.add_argument("--sleep", type=float, default=1.0, help="seconds between API pages")
    args = ap.parse_args()

    db = pymysql.connect(host=args.db_host, port=args.db_port,
                         user=args.db_user, password=args.db_pass,
                         database=args.db_name, charset="utf8mb4",
                         autocommit=True)
    cur = db.cursor()

    # Make sure table exists (migrations should have created it, but be safe).
    cur.execute("""
        CREATE TABLE IF NOT EXISTS botKillmailLegends (
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
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """)

    sess = requests.Session()
    insert_sql = """
        INSERT IGNORE INTO botKillmailLegends
        (killmail_id, character_id, character_name, corporation_id, alliance_id,
         ship_type_id, fitted_item_ids, security_status, kill_time, points)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
    """

    total = 0
    all_recs = []
    for page in range(args.pages):
        # Walk backwards in time: zKillboard paginates by max killmail id.
        # First page (no params) = newest. Subsequent pages use beforeKillID.
        try:
            if page == 0:
                kills = fetch_kills(sess)
            else:
                last_id = max(k["killmail_id"] for k in kills)
                kills = fetch_kills(sess, {"beforeKillID": last_id})
        except Exception as e:
            print(f"[page {page}] fetch error: {e}", file=sys.stderr)
            break

        for k in kills:
            all_recs.extend(extract_legend(k))
        print(f"[page {page}] fetched {len(kills)} kills (pilots so far {len(all_recs)})")

        time.sleep(args.sleep)

    # Resolve names for all collected character IDs in one pass.
    char_ids = [r[0] for r in all_recs]
    print(f"Resolving {len(set(char_ids))} unique pilot names via ESI...")
    names = fetch_names(char_ids)

    rows = []
    for kmid, cid, corp, ally, ship, fit, sec, tm, pts in all_recs:
        rows.append((kmid, cid, names.get(cid, ""), corp, ally, ship, fit, sec, tm, pts))
    if rows:
        cur.executemany(insert_sql, rows)
        total = len(rows)

    db.close()
    print(f"Done. {total} pilot legends collected into botKillmailLegends.")


if __name__ == "__main__":
    main()
