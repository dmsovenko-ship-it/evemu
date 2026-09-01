#!/bin/bash
# Parallel market seeder.
# Spawns one MySQL connection per region concurrently (up to MAX_PARALLEL at a time).
# Replaces the sequential evedbtool seed command.
#
# Strategy to eliminate lock contention:
#   1. Drop all secondary indexes on mktOrders before seeding.
#   2. Each region gets a pre-assigned orderID range — no auto-increment races.
#   3. Parallel inserts only maintain the PRIMARY KEY; no shared index page locks.
#   4. Each session sets innodb_lock_wait_timeout=600 and retries up to 3 times.
#   5. Rebuild all secondary indexes in one bulk operation after seeding.
#
# Environment variables (all inherited from docker-compose):
#   SEED_REGIONS     — comma-separated region names
#   SEED_SATURATION  — integer 0-100 (default 100)
#   SEED_PARALLEL    — max concurrent connections (default 4)
#   MARIADB_*        — connection parameters

MARIADB_HOST="${MARIADB_HOST:-db}"
MARIADB_DATABASE="${MARIADB_DATABASE:-evemu}"
MARIADB_USER="${MARIADB_USER:-evemu}"
MARIADB_PORT="${MARIADB_PORT:-3306}"
SATURATION="${SEED_SATURATION:-100}"
MAX_PARALLEL="${SEED_PARALLEL:-4}"

# IDs per region slot — must be >= max possible rows per region.
# Actual worst case: Lonetrek 461 stations × 11,635 items = 5.4M at 100% saturation.
# Use 8M to cover all regions at any saturation level.
IDS_PER_REGION=8000000

# Use MYSQL_PWD to avoid the "password on command line" warning
export MYSQL_PWD="${MARIADB_PASSWORD:-evemu}"

MYSQL_BASE="mysql -h ${MARIADB_HOST} -P ${MARIADB_PORT} -u ${MARIADB_USER} ${MARIADB_DATABASE}"

# Convert saturation percentage to decimal (e.g. 100 -> 1.0000)
SAT_DECIMAL=$(awk "BEGIN { printf \"%.4f\", ${SATURATION} / 100 }")

# ── Skip if already seeded ────────────────────────────────────────────────────
existing=$(${MYSQL_BASE} -sN -e "SELECT COUNT(*) FROM mktOrders LIMIT 1;" 2>/dev/null)
if [ "${existing:-0}" -gt 0 ]; then
    echo "[SEED] Market already seeded (${existing} orders present), skipping."
    exit 0
fi

# ── Parse regions ─────────────────────────────────────────────────────────────
IFS=',' read -r -a REGION_ARRAY <<< "${SEED_REGIONS:-}"

if [ ${#REGION_ARRAY[@]} -eq 0 ]; then
    echo "[SEED] SEED_REGIONS is empty, nothing to seed."
    exit 0
fi

echo "[SEED] Seeding ${#REGION_ARRAY[@]} regions with up to ${MAX_PARALLEL} parallel connections..."
echo "[SEED] Saturation: ${SATURATION}% (factor ${SAT_DECIMAL})"
START=$(date +%s)

# ── Drop secondary indexes to eliminate B-tree lock contention ───────────────
# Parallel bulk inserts compete heavily for shared index pages. Dropping indexes
# first means each session only maintains the PRIMARY KEY (non-overlapping ranges
# = zero contention). We rebuild all indexes in one pass after seeding.
echo "[SEED] Dropping secondary indexes for bulk load..."
${MYSQL_BASE} <<'SQL'
ALTER TABLE mktOrders
    DROP INDEX IF EXISTS typeID,
    DROP INDEX IF EXISTS regionID,
    DROP INDEX IF EXISTS stationID,
    DROP INDEX IF EXISTS orderID,
    DROP INDEX IF EXISTS idx_solar_system,
    DROP INDEX IF EXISTS idx_region_type_bid,
    DROP INDEX IF EXISTS idx_region_bid;
SQL

# ── Clear the entire table once before parallel seeding ─────────────────────
# Single DELETE avoids gap-lock deadlocks between concurrent per-region DELETEs.
echo "[SEED] Clearing mktOrders..."
${MYSQL_BASE} -e "DELETE FROM mktOrders;" 2>/dev/null

# ── Per-region seed function (runs in a subshell via xargs) ──────────────────
seed_region() {
    local region_name="$1"
    local start_id="$2"
    local end_id=$(( start_id + IDS_PER_REGION - 1 ))
    local attempt=0

    local safe_name="${region_name//\'/\'\'}"

    local regionid
    regionid=$(mysql -h "${MARIADB_HOST}" -P "${MARIADB_PORT}" -u "${MARIADB_USER}" "${MARIADB_DATABASE}" \
        -sN -e "SELECT regionID FROM mapRegions WHERE regionName = '${safe_name}' LIMIT 1;" 2>/dev/null)

    if [ -z "$regionid" ]; then
        echo "[SEED] WARNING: '${region_name}' not found in mapRegions, skipping."
        return 0
    fi

    echo "[SEED] Starting : ${region_name} (regionID=${regionid}, orderID base=${start_id})"

    while [ $attempt -lt 3 ]; do
        attempt=$(( attempt + 1 ))
        if [ $attempt -gt 1 ]; then
            local wait=$(( attempt * 15 ))
            echo "[SEED] Retry ${attempt}/3  : ${region_name} (waiting ${wait}s...)"
            sleep $wait
        fi

        mysql -h "${MARIADB_HOST}" -P "${MARIADB_PORT}" -u "${MARIADB_USER}" "${MARIADB_DATABASE}" \
            <<SQL
SET innodb_lock_wait_timeout = 600;
SET @lim = (SELECT ROUND(COUNT(stationID) * ${SAT_DECIMAL}) FROM staStations WHERE regionID = ${regionid});
SET @i = 0;
-- Current time as Windows FILETIME (100-ns intervals since 1601-01-01), matching evedbtool behaviour.
SET @issued = (UNIX_TIMESTAMP() + 11644473600) * 10000000;

CREATE TEMPORARY TABLE tStations (
    stationID     INT,
    solarSystemID INT,
    regionID      INT,
    corporationID INT,
    security      FLOAT
);

INSERT INTO tStations
    SELECT stationID, solarSystemID, regionID, corporationID, security
    FROM   staStations
    WHERE  (@i := @i + 1) <= @lim
      AND  regionID = ${regionid}
    ORDER BY RAND();

SET @oid = ${start_id} - 1;

INSERT INTO mktOrders
    (orderID, typeID, ownerID, regionID, stationID, price,
     volEntered, volRemaining, issued, minVolume, duration, solarSystemID, jumps)
SELECT
    (@oid := @oid + 1),
    t.typeID,
    s.corporationID,
    s.regionID,
    s.stationID,
    IF(t.basePrice = 0 OR s.security <= 0, 100, t.basePrice / s.security),
    CASE g.categoryID
        WHEN 8  THEN 10000                                    -- Charge: ammo/charges/missiles/fuel — consumables
        WHEN 4  THEN 10000                                    -- Material: minerals/gas isotopes/ice/fuel blocks
        WHEN 24 THEN 5000                                     -- Reaction
        WHEN 25 THEN 5000                                     -- Asteroid (ore)
        WHEN 17 THEN 5000                                     -- Commodity
        WHEN 42 THEN 10000                                    -- Planetary Resources
        WHEN 43 THEN 5000                                     -- Planetary Commodities
        WHEN 18 THEN 100                                      -- Drone
        WHEN 22 THEN 50                                       -- Deployable
        WHEN 5  THEN 20                                       -- Accessories (implants/boosters)
        WHEN 7  THEN 10                                       -- Module
        WHEN 6  THEN 1                                        -- Ship
        WHEN 9  THEN 1                                        -- Blueprint
        ELSE 50                                               -- everything else (rare/collectibles)
    END,
    CASE g.categoryID
        WHEN 8  THEN 10000
        WHEN 4  THEN 10000
        WHEN 24 THEN 5000
        WHEN 25 THEN 5000
        WHEN 17 THEN 5000
        WHEN 42 THEN 10000
        WHEN 43 THEN 5000
        WHEN 18 THEN 100
        WHEN 22 THEN 50
        WHEN 5  THEN 20
        WHEN 7  THEN 10
        WHEN 6  THEN 1
        WHEN 9  THEN 1
        ELSE 50
    END,
    @issued, 1, 250,
    s.solarSystemID, 1
FROM   tStations s
CROSS  JOIN invTypes t
INNER  JOIN invGroups g ON t.groupID = g.groupID
WHERE  t.published = 1
  AND  g.categoryID IN (4, 5, 6, 7, 8, 9, 16, 17, 18, 22, 23, 24, 25, 32, 34, 35, 39, 40, 41, 42, 43, 46);
SQL

        if [ $? -eq 0 ]; then
            echo "[SEED] Done     : ${region_name}"
            return 0
        fi
        echo "[SEED] Attempt ${attempt} failed: ${region_name}"
    done

    echo "[SEED] FAILED   : ${region_name} (all 3 attempts exhausted)"
    return 1
}

export -f seed_region
export MARIADB_HOST MARIADB_DATABASE MARIADB_USER MARIADB_PORT MYSQL_PWD SAT_DECIMAL IDS_PER_REGION

# ── Launch with pre-assigned ID ranges ────────────────────────────────────────
REGION_INDEX=0
{
    for region in "${REGION_ARRAY[@]}"; do
        region="${region#"${region%%[![:space:]]*}"}"
        region="${region%"${region##*[![:space:]]}"}"
        [ -z "$region" ] && continue
        START_ID=$(( REGION_INDEX * IDS_PER_REGION + 1 ))
        echo "${region}|${START_ID}"
        REGION_INDEX=$(( REGION_INDEX + 1 ))
    done
} | xargs -d '\n' -n 1 -P"${MAX_PARALLEL}" bash -c '
    IFS="|" read -r region start_id <<< "$1"
    seed_region "$region" "$start_id"
' _

# ── Rebuild indexes and reset AUTO_INCREMENT ──────────────────────────────────
echo "[SEED] Rebuilding indexes..."
${MYSQL_BASE} <<'SQL'
ALTER TABLE mktOrders
    ADD INDEX typeID              (typeID),
    ADD INDEX regionID            (regionID),
    ADD INDEX stationID           (stationID),
    ADD INDEX idx_solar_system    (solarSystemID),
    ADD INDEX idx_region_type_bid (regionID, typeID, bid),
    ADD INDEX idx_region_bid      (regionID, bid);
SQL

echo "[SEED] Resetting AUTO_INCREMENT..."
${MYSQL_BASE} -e "
    SET @max_id = (SELECT COALESCE(MAX(orderID), 0) FROM mktOrders);
    SET @sql = CONCAT('ALTER TABLE mktOrders AUTO_INCREMENT = ', @max_id + 1);
    PREPARE stmt FROM @sql;
    EXECUTE stmt;
    DEALLOCATE PREPARE stmt;
" 2>/dev/null

ELAPSED=$(( $(date +%s) - START ))
echo "[SEED] All regions seeded in ${ELAPSED}s."
