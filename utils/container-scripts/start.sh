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

# Start mock news server to suppress "GetNewsTickerData" SSL error
# Generates a self-signed cert on first run and serves via HTTPS on port 443.
if [ "${DISABLE_NEWS:-FALSE}" != "TRUE" ]; then
    CERT=/app/etc/news_cert.pem
    KEY=/app/etc/news_key.pem
    if [ ! -f "$CERT" ]; then
        openssl req -x509 -newkey rsa:2048 -keyout "$KEY" -out "$CERT" -days 3650 -nodes \
            -subj "/CN=www.eveonline.com" 2>/dev/null
        cat "$CERT" "$KEY" > /app/etc/news_combined.pem
    fi
    python3 -c "
import ssl, http.server, threading

HTML = b'''<?xml version=\"1.0\"?>
<news>
  <item>
    <title>EVEmu Crucible</title>
    <text>Welcome to your private server. o7</text>
    <date>$(date +%Y-%m-%d)</date>
  </item>
</news>'''

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/xml')
        self.end_headers()
        self.wfile.write(HTML)
    def log_message(self, *a): pass

httpd = http.server.HTTPServer(('0.0.0.0', 443), Handler)
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain('/app/etc/news_combined.pem')
httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
threading.Thread(target=httpd.serve_forever, daemon=True).start()
print('News server running on port 443 (HTTPS)')
" 2>/dev/null &
fi

#Start eve-server
echo "Starting eve-server..."
cd /app/bin/
if [ "$RUN_WITH_GDB" == "TRUE" ]; then
    echo "=== Running EVEmu with gdb ==="
    gdb -ex run ./eve-server
else
    echo "=== Running EVEmu normally ==="
    ./eve-server
fi