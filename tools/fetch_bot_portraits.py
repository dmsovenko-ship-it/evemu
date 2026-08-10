#!/usr/bin/env python3
"""
Fetch real EVE character portraits (from ESI) for bot characters and store them
under the server character id in the image cache, so the game client shows a
real avatar instead of a blank one.

image_server reads:  <imageDir>/Character/{id}_512.jpg
(this script writes  image_cache/Character/{serverCharID}_512.jpg)

Usage:
    python tools/fetch_bot_portraits.py --db-host 127.0.0.1 --db-user evemu \
        --db-pass evemu --db-name evemu --image-dir /opt/evemu/image_cache

Requires: requests, pymysql
"""

import argparse
import os
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

USER_AGENT = "EVEmuBotPortraitFetcher/1.0 (contact: dev@example.com)"
ESI = "https://esi.evetech.net/v4/characters/%u/portrait/?datasource=tranquility"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db-host", default="127.0.0.1")
    ap.add_argument("--db-port", type=int, default=3306)
    ap.add_argument("--db-user", required=True)
    ap.add_argument("--db-pass", required=True)
    ap.add_argument("--db-name", required=True)
    ap.add_argument("--image-dir", required=True, help="server image cache dir (image_cache)")
    ap.add_argument("--limit", type=int, default=500)
    args = ap.parse_args()

    charDir = os.path.join(args.image_dir, "Character")
    os.makedirs(charDir, exist_ok=True)

    db = pymysql.connect(host=args.db_host, port=args.db_port,
                         user=args.db_user, password=args.db_pass,
                         database=args.db_name, charset="utf8mb4",
                         autocommit=True)
    cur = db.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS botPortraits (
            serverCharID INT UNSIGNED NOT NULL,
            eveCharID INT UNSIGNED NOT NULL,
            fetched TINYINT(1) NOT NULL DEFAULT 0,
            PRIMARY KEY (serverCharID),
            KEY idx_eve (eveCharID)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """)
    cur.execute(
        "SELECT serverCharID, eveCharID FROM botPortraits WHERE fetched = 0 LIMIT %s",
        (args.limit,))
    rows = cur.fetchall()
    if not rows:
        print("No portraits to fetch.")
        return

    sess = requests.Session()
    done = 0
    for serverID, eveID in rows:
        path = os.path.join(charDir, "%u_512.jpg" % serverID)
        if os.path.exists(path) and os.path.getsize(path) > 0:
            cur.execute("UPDATE botPortraits SET fetched = 1 WHERE serverCharID = %s", (serverID,))
            done += 1
            continue
        try:
            r = sess.get(ESI % eveID, headers={"User-Agent": USER_AGENT}, timeout=30)
            r.raise_for_status()
            data = r.json()
            url = data.get("px512x512")
            if not url:
                cur.execute("UPDATE botPortraits SET fetched = 1 WHERE serverCharID = %s", (serverID,))
                continue
            img = sess.get(url, headers={"User-Agent": USER_AGENT}, timeout=30)
            img.raise_for_status()
            with open(path, "wb") as f:
                f.write(img.content)
            cur.execute("UPDATE botPortraits SET fetched = 1 WHERE serverCharID = %s", (serverID,))
            done += 1
            print("[ok] server %u <- eve %u (%u bytes)" % (serverID, eveID, len(img.content)))
        except Exception as e:
            print("[err] server %u eve %u: %s" % (serverID, eveID, e))
        time.sleep(0.3)

    db.close()
    print("Done. %u portraits fetched." % done)


if __name__ == "__main__":
    main()
