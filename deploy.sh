#!/bin/bash
# EVEmu VDS Deployment Script
# Usage: bash deploy.sh [--no-seed] [--build-args "..."]
#
# Prerequisites:
#   - Docker + docker compose plugin (v2)
#   - Git
#   - 10 GB+ free disk, 4 GB+ RAM
#
# Steps:
#   1. Clone/update repo
#   2. Copy .env.example → .env (edit if needed)
#   3. Build + start with production overrides
#   4. Tail logs until initialization finishes

set -euo pipefail

cd "$(dirname "$0")"

# ── Colors ──────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; NC='\033[0m'
info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
err()   { echo -e "${RED}[ERR]${NC}   $*" >&2; }

# ── Flags ───────────────────────────────────────────────────────
SEED_MARKET=true
BUILD_ARGS=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-seed) SEED_MARKET=false; shift ;;
        --build-args) BUILD_ARGS="$2"; shift 2 ;;
        *) err "Unknown flag: $1"; exit 1 ;;
    esac
done

# ── Pre-flight checks ───────────────────────────────────────────
info "Pre-flight checks..."

command -v docker >/dev/null 2>&1 || { err "Docker not found"; exit 1; }
docker compose version >/dev/null 2>&1 || { err "docker compose plugin not found"; exit 1; }

# ── .env ─────────────────────────────────────────────────────────
if [[ ! -f .env ]]; then
    warn ".env not found — copying from .env.example"
    cp .env.example .env
    info "Edit .env to set IMAGE_SERVER_HOST and change DB passwords before proceeding."
    info "Then re-run this script."
    exit 0
fi

# shellcheck source=.env
source .env

# ── Stop existing containers ────────────────────────────────────
info "Stopping any running EVEmu containers..."
docker compose -f docker-compose.yml -f docker-compose.prod.yml down --remove-orphans 2>/dev/null || true

# ── Pull latest images ──────────────────────────────────────────
info "Pulling MariaDB image..."
docker pull mariadb:11.8

# ── Build + Start ────────────────────────────────────────────────
COMPOSE_FILES="-f docker-compose.yml -f docker-compose.prod.yml"
SEED_ENV="SEED_MARKET=${SEED_MARKET}"

info "Building and starting EVEmu (seed=${SEED_MARKET})..."
SEED_MARKET="${SEED_MARKET}" \
docker compose ${COMPOSE_FILES} up -d --build

# ── Watch initialization ────────────────────────────────────────
info "Watching server logs (Ctrl+C to stop watching, server keeps running)..."
echo ""
docker compose ${COMPOSE_FILES} logs -f server
