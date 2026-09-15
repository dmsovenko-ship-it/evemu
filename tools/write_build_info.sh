#!/bin/sh
# Write the current build's git info to the server config dir, so the EVEmu
# server can report it in the evening admin Telegram digest (commit count is
# used to compute "code changes since the last report").
#
# Run this on the HOST after `git pull`, before/after rebuilding the server
# container:
#
#     sh tools/write_build_info.sh
#
# Defaults assume the standard deploy layout: repo at /opt/evemu and its config
# dir (mounted into the container as /app/etc) at /opt/evemu/config.
# Override with:  sh tools/write_build_info.sh <repo-dir> <output-file>

repo="${1:-/opt/evemu}"
out="${2:-/opt/evemu/config/build_info.txt}"

commit="$(git -C "$repo" rev-parse --short HEAD 2>/dev/null)"
count="$(git -C "$repo" rev-list --count HEAD 2>/dev/null)"
built="$(date '+%Y-%m-%d %H:%M:%S')"

{
    echo "commit=$commit"
    echo "count=$count"
    echo "built=$built"
} > "$out"

echo "wrote $out: commit=$commit count=$count built=$built"
