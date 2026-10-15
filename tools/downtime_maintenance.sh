#!/usr/bin/env bash
# ============================================================================
# Daily downtime maintenance (Crucible-style).
#
# Runs while ONLY the game server container is down; the DB stays UP.
# Install on the game host (172.20.1.47):
#   cron:  */5 * * * * /opt/evemu/tools/downtime_maintenance.sh >> /opt/evemu/backups/downtime.log 2>&1
#
# Window self-check: the script does anything only between DowntimeHourUTC and
# DowntimeHourUTC + TASK_DEADLINE_MIN, so an every-5-min cron is safe.
#
# EXTENDING THE DOWNTIME (updates / rebuild):
#   touch /opt/evemu/downtime.hold      # DB tasks still run, server NOT auto-started
#   ... git pull; docker-compose build --no-cache server ...
#   rm /opt/evemu/downtime.hold && docker start server
#
# Manual stop any time: docker stop -t 120 server  (SIGTERM = clean shutdown path)
# ============================================================================
set -u

DT_HOUR=11                 # must match <DowntimeHourUTC> in eve-server.xml
DT_MIN=0                   # must match <DowntimeMinuteUTC>
TASK_DEADLINE_MIN=28       # minutes after DT start when the server must be back
HOLD_FILE=/opt/evemu/downtime.hold
LOCK=/tmp/downtime_maintenance.lock
LOGTAG="[downtime]"

# 10# forces base-10: %H/%M yield leading zeros (08/09) which bash reads as octal
now_min=$(( 10#$(date -u +%H) * 60 + 10#$(date -u +%M) ))
start_min=$(( DT_HOUR * 60 + DT_MIN ))
delta=$(( (now_min - start_min + 1440) % 1440 ))
[ "$delta" -gt "$TASK_DEADLINE_MIN" ] && exit 0          # outside the window

[ -f "$LOCK" ] && exit 0                                  # already running
echo $$ > "$LOCK"
trap 'rm -f "$LOCK"' EXIT

M() { echo "$(date -u +%FT%TZ) $LOGTAG $*"; }

DB() { docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu -e "$1" 2>&1 | grep -v "Using a password"; }

# --- 0) wait for the game server to exit (announced shutdown); fallback stop ---
for i in $(seq 1 60); do
    st=$(docker inspect -f '{{.State.Running}}' server 2>/dev/null || echo "missing")
    [ "$st" != "true" ] && break
    if [ "$i" -eq 60 ]; then
        M "server still running - forcing stop (SIGTERM = clean shutdown path)"
        docker stop -t 120 server >/dev/null
    fi
    sleep 10
done
M "server is down, DB stays up - starting maintenance"

# --- 1) quick SQL hygiene ---
n=$(DB "DELETE s FROM sysSignatures s LEFT JOIN entity e ON e.itemID = s.sigItemID WHERE e.itemID IS NULL; SELECT ROW_COUNT();" | tail -1)
M "orphan signatures removed: ${n:-0}"

DB "SET GLOBAL binlog_expire_logs_seconds = 259200;" >/dev/null
DB "PURGE BINARY LOGS BEFORE NOW() - INTERVAL 3 DAY;" >/dev/null
M "binlog retention set to 3 days + purged"

# --- 2) price sync from EVE (ESI -> invTypes.basePrice; used by market/TopValuables/Resolve fallbacks) ---
if [ -x /opt/conda/bin/python3 ] && [ -f /opt/evemu/tools/import_prices.py ]; then
    M "price sync: start"
    out=$(timeout 900 /opt/conda/bin/python3 /opt/evemu/tools/import_prices.py \
        --db-host 127.0.0.1 --db-user evemu --db-pass evemu --db-name evemu 2>&1 | tail -2 | tr '\n' ' ')
    M "price sync: ${out:-done}"
else
    M "price sync: skipped (tools not found)"
fi

# --- 3) ANALYZE hot tables (fast; refreshes optimizer statistics) ---
DB "ANALYZE TABLE entity, mktOrders, mktTransactions, chrKillTable, jnlCharacters;" >/dev/null \
    && M "ANALYZE ok"

# --- 4) OPTIMIZE rotation (one heavy table per day; guarded by fragmentation and free disk) ---
# MariaDB InnoDB OPTIMIZE rebuilds the table: needs free disk ~= 1x table size.
dow=$(date -u +%u)   # 1=Mon .. 7=Sun
case $dow in
    1) TABS="chrKillTable jnlCharacters" ;;
    2) TABS="mktOrders" ;;
    3) TABS="mktTransactions" ;;
    4) TABS="entity" ;;
    5) TABS="mktOrders" ;;
    6) TABS="mktTransactions" ;;
    7) TABS="entity" ;;
esac
free_kb=$(df -Pk /var/lib/docker | tail -1 | awk '{print $4}')
for T in $TABS; do
    row=$(DB "SELECT data_length, data_free FROM information_schema.tables WHERE table_schema = 'evemu' AND table_name = '$T';" | tail -1)
    dl=$(echo "$row" | awk '{print $1}'); dfree=$(echo "$row" | awk '{print $2}')
    [ -z "${dl:-}" ] && continue
    dl=${dl:-0}; dfree=${dfree:-0}
    frag=0; [ "$dl" -gt 0 ] && frag=$(( dfree * 100 / dl ))
    if [ "$frag" -lt 15 ]; then M "OPTIMIZE $T skipped (fragmentation ${frag}% < 15%)"; continue; fi
    need_kb=$(( (dl + dfree) * 2 / 1024 ))
    if [ "$need_kb" -gt "$free_kb" ]; then M "OPTIMIZE $T skipped (disk: need ~${need_kb}K, free ${free_kb}K)"; continue; fi
    M "OPTIMIZE $T start (size $(( dl / 1048576 ))MB, frag ${frag}%)"
    DB "OPTIMIZE TABLE $T;" >/dev/null && M "OPTIMIZE $T done"
done

# --- 5) disk hygiene ---
M "docker image prune: $(docker image prune -af 2>/dev/null | tail -1 | tr -d '\n')"
# old core dumps: keep the 2 newest (gdb triage may need the latest)
CORE_DIR=/var/lib/docker/volumes/evemu_server_cache/_data
if [ -d "$CORE_DIR" ]; then
    ls -t "$CORE_DIR"/core.* 2>/dev/null | tail -n +3 | xargs -r rm -f
    M "old core dumps cleaned"
fi

# --- 6) start the server back (unless the operator holds it for updates) ---
if [ -f "$HOLD_FILE" ]; then
    M "HOLD active ($HOLD_FILE) - server NOT started. When done: rm $HOLD_FILE && docker start server"
    exit 0
fi
M "starting server"
docker start server >/dev/null && M "server started"
