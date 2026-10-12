#!/bin/bash
# One-time consolidation of stackable entity rows.
#
# Bots deposit cargo as a NEW entity row per run, so `entity` accumulated
# ~2.9M mergeable mineral rows (500k+ single Tritanium stacks, etc.) across
# only ~2000 owners / ~1400 locations. Merging same (typeID, ownerID,
# locationID, flag) stacks into the oldest row cuts the table from ~4.15M to
# ~1.2M rows and speeds up every hangar/owner query.
#
# RUN WITH THE SERVER STOPPED (in-memory InventoryItemRefs would otherwise go
# stale for merged-away rows). The server's DepositCargoAtStation now merges
# on deposit, so this is a one-off cleanup - re-running it is safe/idempotent
# (nothing to merge -> no-op).
#
# Usage (on the host):  bash tools/merge_entity_stacks.sh
set -e

DB() { sudo docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu "$@"; }

echo "[stacks] building keeper map (categories 4/16/17/43, stackables)..."
DB -e "
DROP TABLE IF EXISTS _stack_keep;
CREATE TABLE _stack_keep (
  keepID INT UNSIGNED PRIMARY KEY,
  typeID SMALLINT UNSIGNED,
  ownerID INT UNSIGNED,
  locationID INT UNSIGNED,
  flag TINYINT UNSIGNED,
  totalQty INT UNSIGNED,
  KEY k_group (typeID, ownerID, locationID, flag)
) ENGINE=InnoDB;
INSERT INTO _stack_keep (keepID, typeID, ownerID, locationID, flag, totalQty)
SELECT MIN(e.itemID), e.typeID, e.ownerID, e.locationID, e.flag, SUM(e.quantity)
FROM entity e
JOIN invTypes  t ON t.typeID  = e.typeID
JOIN invGroups g ON g.groupID = t.groupID
WHERE g.categoryID IN (4,16,17,43)
  AND e.singleton = 0
  AND e.quantity  > 0
GROUP BY e.typeID, e.ownerID, e.locationID, e.flag
HAVING COUNT(*) > 1;
SELECT CONCAT('[stacks] groups to merge: ', COUNT(*)) AS info FROM _stack_keep;
"

echo "[stacks] bumping keeper quantities..."
DB -e "
UPDATE entity e JOIN _stack_keep k ON e.itemID = k.keepID
SET e.quantity = k.totalQty;
"

echo "[stacks] collecting duplicate row ids..."
DB -e "
DROP TABLE IF EXISTS _stack_del;
CREATE TABLE _stack_del (itemID INT UNSIGNED PRIMARY KEY) ENGINE=InnoDB;
INSERT IGNORE INTO _stack_del (itemID)
SELECT e.itemID
FROM entity e
JOIN invTypes  t ON t.typeID  = e.typeID
JOIN invGroups g ON g.groupID = t.groupID
JOIN _stack_keep k
  ON k.typeID = e.typeID AND k.ownerID = e.ownerID
 AND k.locationID = e.locationID AND k.flag = e.flag
WHERE g.categoryID IN (4,16,17,43)
  AND e.singleton = 0
  AND e.quantity  > 0
  AND e.itemID <> k.keepID;
SELECT CONCAT('[stacks] duplicate rows to delete: ', COUNT(*)) AS info FROM _stack_del;
"

echo "[stacks] deleting duplicates in batches..."
TOTAL=0
while :; do
    IDS=$(DB -N -e "SELECT GROUP_CONCAT(itemID) FROM (SELECT itemID FROM _stack_del LIMIT 10000) x;")
    if [ -z "$IDS" ] || [ "$IDS" = "NULL" ]; then break; fi
    DB -e "
      DELETE FROM entity_attributes WHERE itemID IN ($IDS);
      DELETE FROM entity            WHERE itemID IN ($IDS);
      DELETE FROM _stack_del        WHERE itemID IN ($IDS);
    "
    N=$(echo "$IDS" | tr ',' '\n' | wc -l)
    TOTAL=$((TOTAL + N))
    echo "[stacks] deleted $TOTAL..."
done

echo "[stacks] cleanup temp tables..."
DB -e "DROP TABLE IF EXISTS _stack_keep; DROP TABLE IF EXISTS _stack_del; OPTIMIZE TABLE entity;"
echo "[stacks] done. total deleted: $TOTAL"
