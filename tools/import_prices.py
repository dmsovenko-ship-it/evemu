#!/usr/bin/env python3
"""
Import EVE Online market prices from ESI into EVEmu database.

Usage:
    python tools/import_prices.py [--region 10000002] [--db-host localhost] [--db-port 3306]
                                   [--db-user root] [--db-pass ""] [--db-name evemu]

By default fetches sell orders from The Forge (Jita, region 10000002).
Updates basePrice in invTypes with the median sell price per typeID.
Types without market orders keep their existing basePrice.
"""

import json
import sys
import time
import argparse
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


ESI_BASE = "https://esi.evetech.net/latest"


def fetch_esi(endpoint: str, params: dict = None):
    """Fetch JSON from ESI with retry logic."""
    url = f"{ESI_BASE}{endpoint}"
    if params:
        qs = "&".join(f"{k}={v}" for k, v in params.items() if v is not None)
        url += f"?{qs}"
    for attempt in range(3):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "EVEmuPriceImport/1.0"})
            with urllib.request.urlopen(req, timeout=30) as resp:
                return json.loads(resp.read())
        except (urllib.error.HTTPError, urllib.error.URLError, OSError) as e:
            print(f"  ESI error (attempt {attempt+1}/3): {e}", file=sys.stderr)
            if attempt < 2:
                time.sleep(2 ** attempt)
    return []


def fetch_all_orders(region_id: int, order_type: str = "sell") -> dict:
    """Fetch all orders of given type from ESI. Returns {typeID: [prices]}."""
    print(f"Fetching {order_type} orders for region {region_id}...")
    type_prices: dict[int, list[float]] = {}

    page = 1
    total_pages = 1
    while page <= total_pages:
        data = fetch_esi(f"/markets/{region_id}/orders/", {
            "order_type": order_type,
            "page": page,
            "datasource": "tranquility",
        })
        if not data:
            break
        # Extract total page count from response headers via a second query
        if page == 1:
            req = urllib.request.Request(
                f"{ESI_BASE}/markets/{region_id}/orders/?order_type={order_type}&page=1&datasource=tranquility",
                headers={"User-Agent": "EVEmuPriceImport/1.0"}
            )
            try:
                with urllib.request.urlopen(req) as resp:
                    h = resp.headers.get("X-Pages", "1")
                    total_pages = int(h)
            except Exception:
                total_pages = 1
            print(f"  Total pages: {total_pages}")

        for order in data:
            tid = order.get("type_id")
            price = order.get("price", 0)
            if tid and price > 0:
                type_prices.setdefault(tid, []).append(price)
        page += 1
        if page % 10 == 0:
            print(f"  Page {page-1}/{total_pages} done ({len(type_prices)} types so far)")

    print(f"  Done. Found prices for {len(type_prices)} typeIDs across {total_pages} pages.")
    return type_prices


def median(prices: list[float]) -> float:
    """Calculate median price."""
    if not prices:
        return 0.0
    sp = sorted(prices)
    n = len(sp)
    if n % 2 == 1:
        return sp[n // 2]
    return (sp[n // 2 - 1] + sp[n // 2]) / 2.0


def update_database(conn, type_prices: dict[int, float]):
    """Update basePrice in invTypes table."""
    cursor = conn.cursor()
    updated = 0
    skipped = 0

    print("Updating invTypes.basePrice...")
    for tid, price in type_prices.items():
        cursor.execute("SELECT typeID FROM invTypes WHERE typeID = %s", (tid,))
        if cursor.fetchone() is None:
            skipped += 1
            continue
        cursor.execute(
            "UPDATE invTypes SET basePrice = %s WHERE typeID = %s",
            (price, tid)
        )
        updated += 1
        if updated % 1000 == 0:
            conn.commit()
            print(f"  {updated} updated ({skipped} skipped)")

    conn.commit()
    cursor.close()
    print(f"  Done. {updated} types updated, {skipped} typeIDs not found in DB.")


def get_db_connection(args):
    """Connect to MariaDB/MySQL."""
    if pymysql:
        return pymysql.connect(
            host=args.db_host, port=args.db_port,
            user=args.db_user, password=args.db_pass,
            database=args.db_name, charset="utf8mb4"
        )
    elif mysql_connector:
        return mysql_connector.connect(
            host=args.db_host, port=args.db_port,
            user=args.db_user, password=args.db_pass,
            database=args.db_name
        )
    else:
        print("ERROR: need 'pymysql' or 'mysql-connector-python'", file=sys.stderr)
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description="Import EVE market prices into EVEmu DB")
    ap.add_argument("--region", type=int, default=10000002, help="ESI region ID (default: 10000002 = The Forge/Jita)")
    ap.add_argument("--db-host", default="localhost")
    ap.add_argument("--db-port", type=int, default=3306)
    ap.add_argument("--db-user", default="root")
    ap.add_argument("--db-pass", default="")
    ap.add_argument("--db-name", default="evemu")
    ap.add_argument("--order-type", choices=["sell", "buy"], default="sell",
                    help="Order type to use for pricing (default: sell)")
    ap.add_argument("--median", action="store_true", help="Use median (default)")
    ap.add_argument("--mean", action="store_true", help="Use mean instead of median")
    args = ap.parse_args()

    # Fetch orders
    type_prices = fetch_all_orders(args.region, args.order_type)

    if not type_prices:
        print("No prices fetched. Exiting.", file=sys.stderr)
        sys.exit(1)

    # Calculate per-type price
    calc_prices = {}
    for tid, prices in type_prices.items():
        if args.mean:
            calc_prices[tid] = sum(prices) / len(prices)
        else:
            calc_prices[tid] = median(prices)

    # Update database
    conn = get_db_connection(args)
    try:
        update_database(conn, calc_prices)
    finally:
        conn.close()

    print("Price import complete.")


if __name__ == "__main__":
    main()
