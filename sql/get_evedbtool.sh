#!/bin/bash

echo "Downloading EVEDBTool..."

if ! command -v curl &> /dev/null; then
    echo "curl not found, please install it."
    exit 1
fi

arch=$(arch)

# Direct download URLs for v0.0.6 (avoids GitHub API rate limits)
if [[ $arch == aarch64* ]]; then
    echo "Using aarch64 build..."
    URL="https://github.com/EvEmu-Project/EVEDBTool/releases/download/0.0.6/evedb_aarch64"
elif [[ $arch == x86_64* ]]; then
    echo "Using x86_64 build..."
    URL="https://github.com/EvEmu-Project/EVEDBTool/releases/download/0.0.6/evedbtool"
else
    echo "Unsupported architecture: $arch"
    exit 1
fi

echo "Downloading from: $URL"
if curl --output evedbtool -s -L -f "$URL"; then
    chmod +x evedbtool
    echo "EVEDBTool downloaded successfully."
else
    echo "Failed to download EVEDBTool."
    rm -f evedbtool
    exit 1
fi
