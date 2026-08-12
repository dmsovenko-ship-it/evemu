#!/usr/bin/env python3
"""
Seed botChatLearned with realistic EVE local-chat dialogue pairs so bots have
conversational 'experience' from the start (a pseudo-intellect grown from real
EVE forums/chat style) instead of only learning from live play.

Usage:
    python tools/seed_bot_chat.py --api-key <DeepSeekKey> \
        --db-host 127.0.0.1 --db-port 3306 --db-user evemu --db-pass evemu --db-name evemu

Writes (trigger, reply) pairs into botChatLearned for every known bot (charID in
botMemory). Re-running updates uses instead of duplicating.
"""

import argparse
import json
import sys
import time
import urllib.request
import urllib.error

try:
    import pymysql
except ImportError:
    pymysql = None
    try:
        import mysql.connector as mysql_connector
    except ImportError:
        mysql_connector = None

DEEPSEEK_URL = "https://api.deepseek.com/chat/completions"


def ask_deepseek(api_key: str, system: str, user: str) -> str:
    body = json.dumps({
        "model": "deepseek-chat",
        "messages": [
            {"role": "system", "content": system},
            {"role": "user", "content": user},
        ],
        "max_tokens": 400,
        "temperature": 0.9,
    }).encode("utf-8")
    req = urllib.request.Request(
        DEEPSEEK_URL, data=body,
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {api_key}"})
    for attempt in range(3):
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                data = json.loads(resp.read())
            return data["choices"][0]["message"]["content"].strip()
        except (urllib.error.HTTPError, urllib.error.URLError, OSError, KeyError, IndexError) as e:
            print(f"  DeepSeek error (attempt {attempt+1}/3): {e}", file=sys.stderr)
            if attempt < 2:
                time.sleep(2 ** attempt)
    return ""


def parse_pairs(text: str):
    """Parse 'trigger|reply' lines from the model output."""
    pairs = []
    for line in text.splitlines():
        line = line.strip()
        if not line or "|" not in line:
            continue
        t, _, r = line.partition("|")
        t = t.strip().strip('"')
        r = r.strip().strip('"')
        if t and r and len(t) <= 250 and len(r) <= 250:
            pairs.append((t, r))
    return pairs


def main():
    ap = argparse.ArgumentParser(description="Seed botChatLearned with EVE dialogue pairs")
    ap.add_argument("--api-key", required=True)
    ap.add_argument("--db-host", default="localhost")
    ap.add_argument("--db-port", type=int, default=3306)
    ap.add_argument("--db-user", default="root")
    ap.add_argument("--db-pass", default="")
    ap.add_argument("--db-name", default="evemu")
    ap.add_argument("--batches", type=int, default=3, help="how many generation batches (each ~35 pairs)")
    args = ap.parse_args()

    if not pymysql and not mysql_connector:
        print("ERROR: need 'pymysql' or 'mysql-connector-python'", file=sys.stderr)
        sys.exit(1)

    system = (
        "You are generating sample EVE Online local-chat dialogue. Write casual, "
        "natural EVE player chatter — slang (isk, rat, gate, warp, fit, lowsec, "
        "nullsec, dock, hauler, drone, anom, belt). Mix English and Russian lines "
        "(some players chat in Russian)."
    )

    all_pairs = []
    for b in range(args.batches):
        print(f"Batch {b+1}/{args.batches}: asking DeepSeek...")
        text = ask_deepseek(
            args.api_key, system,
            "Output exactly 35 lines, one per line, format: TRIGGER|REPLY\n"
            "TRIGGER is a common local-chat line someone might say (a question or "
            "statement), REPLY is a realistic EVE player's answer to it. Both short, "
            "1 sentence. Do not number them, no extra text.")
        pairs = parse_pairs(text or "")
        print(f"  got {len(pairs)} pairs")
        all_pairs.extend(pairs)
        if b + 1 < args.batches:
            time.sleep(2)

    # De-dupe by (trigger, reply)
    seen = set()
    dedup = []
    for t, r in all_pairs:
        if (t.lower(), r.lower()) not in seen:
            seen.add((t.lower(), r.lower()))
            dedup.append((t, r))
    print(f"Total unique pairs: {len(dedup)}")

    if not dedup:
        print("Nothing generated, exiting.", file=sys.stderr)
        sys.exit(1)

    # Write to DB for every known bot.
    conn = (pymysql.connect(host=args.db_host, port=args.db_port, user=args.db_user,
                            password=args.db_pass, database=args.db_name, charset="utf8mb4")
            if pymysql else mysql_connector.connect(
                host=args.db_host, port=args.db_port, user=args.db_user,
                password=args.db_pass, database=args.db_name))
    cur = conn.cursor()
    cur.execute("SELECT charID FROM botMemory")
    bots = [row[0] for row in cur.fetchall()]
    if not bots:
        print("No bots in botMemory yet — seeding for bot #0 (any future bot reuses by name? no).", file=sys.stderr)
        bots = [0]
    print(f"Seeding {len(dedup)} pairs to {len(bots)} bots...")
    n = 0
    for cid in bots:
        for t, r in dedup:
            cur.execute(
                "INSERT INTO botChatLearned (charID, `trigger`, reply, uses, lastUse)"
                " VALUES (%s, %s, %s, 1, NOW())"
                " ON DUPLICATE KEY UPDATE uses = uses + 1, lastUse = NOW()",
                (cid, t, r))
            n += 1
    conn.commit()
    cur.close()
    conn.close()
    print(f"Done. {n} rows written.")
    print("Tip: restart the server so bots load the seeded phrases (they query the table live, "
          "no restart strictly needed).")


if __name__ == "__main__":
    main()
