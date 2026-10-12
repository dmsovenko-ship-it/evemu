#!/bin/bash
# Market housekeeping: prune orders that can never fill and old history.
#
#  * mktOrders: rows with volRemaining = 0 (fully filled / cancelled escrow
#    leftovers) are dead weight in the 38M-row table.
#  * mktHistory: daily price history is only needed for a rolling window; keep
#    N days (default 90) and drop the rest.
#
# Timestamps are FILETIME (100ns since 1601): unix*1e7 + 11644473600*1e7.
# Safe on a live server (pure DB, no in-memory refs). Idempotent.
#
# Usage: bash tools/prune_market.sh [history_days=90]
set -e
DAYS="${1:-90}"
# FILETIME cutoffs
DEAD_CUT=$(( ($(date +%s) + 11644473600 - 86400) * 10000000 ))
HIST_CUT=$(( ($(date +%s) + 11644473600 - DAYS*86400) * 10000000 ))

DB() { sudo docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu "$@"; }

echo "[market] dead orders (volRemaining=0, older than 1 day):"
DB -e "SELECT COUNT(*) AS dead_orders FROM mktOrders WHERE volRemaining = 0 AND issued < $DEAD_CUT;"

echo "[market] pruning dead orders..."
while :; do
    N=$(DB -N -e "DELETE FROM mktOrders WHERE volRemaining = 0 AND issued < $DEAD_CUT LIMIT 20000; SELECT ROW_COUNT();")
    echo "  deleted ${N:-0}"
    if [ -z "$N" ] || [ "$N" = "0" ]; then break; fi
done

echo "[market] history older than $DAYS days:"
DB -e "SELECT COUNT(*) AS old_history FROM mktHistory WHERE historyDate < $HIST_CUT;"

echo "[market] pruning old history..."
while :; do
    N=$(DB -N -e "DELETE FROM mktHistory WHERE historyDate < $HIST_CUT LIMIT 20000; SELECT ROW_COUNT();")
    echo "  deleted ${N:-0}"
    if [ -z "$N" ] || [ "$N" = "0" ]; then break; fi
done

echo "[market] done."
