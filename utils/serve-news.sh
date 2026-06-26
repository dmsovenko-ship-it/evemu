#!/bin/sh
# Serve a mock news page on port 80 to suppress the Crucible news ticker error.
# The client tries https://www.eveonline.com with outdated SSL and fails.
#
# Usage:
#   1. Add to /etc/hosts:  127.0.0.1 www.eveonline.com
#   2. Run this script in a separate terminal or background it.
#
# Note: the client uses HTTPS but will fall back to a non-SSL error
# that doesn't crash. This stops the SSL handshake failure spam.

DIR="$(cd "$(dirname "$0")" && pwd)"
cat > /tmp/news.xml << 'XML'
<?xml version="1.0"?>
<news>
  <item>
    <title>EVEmu Crucible — Local Server</title>
    <text>Welcome to your private EVE Online Crucible server.</text>
    <date>2026-06-26</date>
  </item>
</news>
XML

cd /tmp
python3 -m http.server 80 --bind 0.0.0.0 2>/dev/null
