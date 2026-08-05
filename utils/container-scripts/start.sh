#!/bin/bash

echo "Initializing EVEmu..."

#Initialize the database
/src/utils/container-scripts/db_init.sh

#Initialize configuration files
if [ ! -f "/app/etc/eve-server.xml" ]; then
    echo "eve-server.xml not found, installing..."
    cp /src/utils/config/eve-server.xml /app/etc/
fi
if [ ! -f "/app/etc/log.ini" ]; then
    echo "log.ini not found, installing..."
    cp /src/utils/config/log.ini /app/etc/
fi
if [ ! -f "/app/etc/MarketBot.xml" ]; then
    echo "MarketBot.xml not found, installing..."
    cp /src/utils/config/MarketBot.xml /app/etc/
fi
if [ ! -f "/app/etc/devtools.raw" ]; then
    echo "devtools.raw not found, installing..."
    cp /src/utils/config/devtools.raw /app/etc/
fi

# Clean up stale wrecks
mysql -h "${MARIADB_HOST:-db}" -u "${MARIADB_USER:-evemu}" -p"${MARIADB_PASSWORD:-evemu}" "${MARIADB_DATABASE:-evemu}" \
    -e "DELETE FROM entity_attributes WHERE itemID IN (SELECT itemID FROM entity WHERE itemName LIKE '%Wreck%'); DELETE FROM entity_attributes WHERE itemID IN (SELECT itemID FROM entity WHERE locationID IN (SELECT itemID FROM entity WHERE itemName LIKE '%Wreck%')); DELETE FROM entity WHERE locationID IN (SELECT itemID FROM entity WHERE itemName LIKE '%Wreck%'); DELETE FROM entity WHERE itemName LIKE '%Wreck%';" \
     2>/dev/null; true

# Clean up old decorations and orphaned dynamic entities (containers, beacons, wrecks)
mysql -h "${MARIADB_HOST:-db}" -u "${MARIADB_USER:-evemu}" -p"${MARIADB_PASSWORD:-evemu}" "${MARIADB_DATABASE:-evemu}" \
    -e "DELETE FROM entity WHERE itemID > 14000000 AND typeID IN (23,3293,3296,3298,3465,3467,24445,24545,17366,19373,10645,10124,10753,10754,10758,10756,1225,1226,1227,1228,1229,1230,1231,1232,26468,26483,26505,26527,26549,29033,29034,29035,29036);" \
    2>/dev/null; true

#Start eve-server
echo "Starting eve-server..."
cd /app/bin/
if [ "$RUN_WITH_GDB" == "TRUE" ]; then
    echo "=== Running EVEmu with gdb (batch, backtrace on crash) ==="
    gdb -batch \
        -ex "handle SIGPIPE nostop noprint pass" \
        -ex "handle SIGCHLD nostop noprint pass" \
        -ex "handle SIGALRM nostop noprint pass" \
        -ex run \
        -ex "bt 40" \
        ./eve-server
else
    echo "=== Running EVEmu normally ==="
    ulimit -c unlimited
    ./eve-server
fi