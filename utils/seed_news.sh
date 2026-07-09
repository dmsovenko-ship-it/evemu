#!/bin/bash
# Seed news ticker from git changelog
# Usage: bash utils/seed_news.sh [db_host] [db_user] [db_pass] [db_name]
#
# Populates srvNewsItems with the last 20 git commits as news items.

DB_HOST="${1:-db}"
DB_USER="${2:-evemu}"
DB_PASS="${3:-evemu}"
DB_NAME="${4:-evemu}"

MYSQL="mysql -h $DB_HOST -u $DB_USER -p$DB_PASS $DB_NAME"

# Clear old news
$MYSQL -e "TRUNCATE srvNewsItems;"

# Insert last 20 git commits as news items
COUNT=0
git log --oneline --no-color -20 --format="%H|%s|%ai" 2>/dev/null | while IFS='|' read hash subject date; do
    # Truncate hash for display
    short="${hash:0:7}"
    # Get full commit body for description
    body=$(git log --format="%b" -1 "$hash" 2>/dev/null | head -5 | tr '\n' ' ')
    $MYSQL -e "INSERT INTO srvNewsItems (title, description, date) VALUES ('$short: $subject', '$body', '$date');"
    COUNT=$((COUNT + 1))
done

echo "Seeded $COUNT news items from git log."
