#!/bin/bash
# evedbtool binary is now vendored in the repo at /src/sql/evedbtool
# No need to download — just ensure it's executable
if [ -f "/src/sql/evedbtool" ]; then
    chmod +x /src/sql/evedbtool
    echo "EVEDBTool found locally."
else
    echo "EVEDBTool not found at /src/sql/evedbtool"
    exit 1
fi
