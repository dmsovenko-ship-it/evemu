#!/usr/bin/env bash
# ============================================================================
# Deploys the Crucible-style daily downtime onto a game host. Idempotent -
# safe to re-run. Run on the host from the repo checkout:
#
#   sudo bash /opt/evemu/tools/deploy_downtime.sh
#
# What it does:
#   1) adds the 4 <DailyDowntime*> tags into config/eve-server.xml <world>
#      (backup made next to the file; tags apply on the NEXT server start)
#   2) installs the root cron entry (every 5 min; the maintenance script
#      self-checks the UTC window and its own lock, so the frequency is safe)
#   3) sanity checks (containers present, script syntax, self-test run)
# ============================================================================
set -u

EVEMU_DIR=${EVEMU_DIR:-/opt/evemu}
MAINT="$EVEMU_DIR/tools/downtime_maintenance.sh"
CONFIG="$EVEMU_DIR/config/eve-server.xml"
LOGDIR="$EVEMU_DIR/backups"
CRON_LINE="*/5 * * * * $MAINT >> $LOGDIR/downtime.log 2>&1"

[ "$(id -u)" -eq 0 ] || { echo "ERROR: run as root (sudo)"; exit 1; }
[ -f "$MAINT" ] || { echo "ERROR: $MAINT not found (git pull first)"; exit 1; }
[ -f "$CONFIG" ] || { echo "ERROR: $CONFIG not found"; exit 1; }

chmod +x "$MAINT"
if ! bash -n "$MAINT"; then echo "ERROR: syntax error in $MAINT"; exit 1; fi
mkdir -p "$LOGDIR"

# --- 1) config tags (idempotent) ---
if ! grep -a -q '<DailyDowntime>' "$CONFIG"; then
    BAK="$CONFIG.bak_downtime_$(date -u +%Y%m%d%H%M%S)"
    cp -a "$CONFIG" "$BAK"
    own=$(stat -c '%u:%g' "$CONFIG")
    sed -i 's|[[:space:]]*</world>|    <DailyDowntime>true</DailyDowntime>\n    <DowntimeHourUTC>11</DowntimeHourUTC>\n    <DowntimeMinuteUTC>0</DowntimeMinuteUTC>\n    <DowntimeLengthMin>30</DowntimeLengthMin>\n</world>|' "$CONFIG"
    chown "$own" "$CONFIG"
    if grep -a -q '<DailyDowntime>' "$CONFIG"; then
        echo "config: tags added (backup: $BAK). Applied on the NEXT server start."
    else
        echo "ERROR: could not add config tags (no </world> found?). Config restored from backup."
        cp -a "$BAK" "$CONFIG"; chown "$own" "$CONFIG"
        exit 1
    fi
else
    echo "config: tags already present"
fi

# --- 2) cron (root crontab, idempotent) ---
if crontab -l 2>/dev/null | grep -q 'downtime_maintenance.sh'; then
    echo "cron: entry already present"
else
    (crontab -l 2>/dev/null; echo "$CRON_LINE") | crontab -
    echo "cron: entry installed"
fi

# --- 3) sanity ---
docker ps --format '{{.Names}}' 2>/dev/null | grep -qx 'server' \
    && echo "sanity: container 'server' ok" || echo "sanity: WARN container 'server' not found"
docker ps --format '{{.Names}}' 2>/dev/null | grep -qx 'db' \
    && echo "sanity: container 'db' ok" || echo "sanity: WARN container 'db' not found"
if "$MAINT"; then echo "self-test: exited cleanly (running outside the window - expected)"; fi

echo "done. maintenance log: $LOGDIR/downtime.log"
echo "NOTE: announcements require a server rebuild (EntityList::ProcessDowntime)."
echo "      Until then the cron still enforces the window via 'docker stop' fallback (no announcements)."
