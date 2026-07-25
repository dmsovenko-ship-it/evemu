# EVEmu

[![Build Status](https://github.com/dmsovenko-ship-it/evemu/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/dmsovenko-ship-it/evemu/actions/workflows/cmake-multi-platform.yml)

**English** — EVEmu is an open-source server emulator for **EVE Online: Crucible** (December 2011, build 7.31.406079). It allows you to run your own EVE server for development, testing, or private play.

**Русский** — EVEmu — это эмулятор сервера для **EVE Online: Crucible** (Декабрь 2011, сборка 7.31.406079) с открытым исходным кодом. Позволяет запустить собственный сервер EVE для разработки, тестирования или приватной игры.

---

## Features

- Login, character creation, and station services
- Ship navigation: warp, orbit, follow, align, dock/undock
- Combat: missiles, turrets, drones, smartbombs, warp disrupt probes
- Fitting, cargo, market, contracts
- Agent missions, anomalies, wormholes
- Corporation management, sovereignty
- Mail and mailing lists
- POS (Player-Owned Structures)
- NPC AI with roaming spawns
- Multi-client support

## Quick Start

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

See `doc/admin_reference.md` for configuration and database setup.

## Requirements

- Linux (tested on Ubuntu 22.04+) or macOS
- C++17 compiler (GCC 11+, Clang 14+)
- MySQL 8.0+ / MariaDB 10.6+
- CMake 3.20+
- OpenSSL, libcurl, libmysqlcppconn, zlib

## Documentation

- `doc/admin_reference.md` — server administration
- `doc/ChangeLog.md` — version history
- `doc/TODO.md` — planned work
- `doc/TESTING.md` — test scenarios
- `doc/code_and_design_notes/` — architecture notes
- `doc/decompiled_client_scripts/` — Crucible client Python (1082 scripts)

## License

GNU Lesser General Public License v2.1. See `LICENSE.txt`.
