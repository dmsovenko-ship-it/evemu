# EVEmu Crucible — Progress / Прогресс

> **Our fork: `████████████████████` ~99%** · **Upstream: `████████░░░░░░░░░░░░` ~60%**
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

---

## Overview / Обзор

| System | % | Bar | Δ up | System | % | Bar | Δ up |
|--------|---|-----|------|--------|---|-----|------|
| Account & Character | 97% | `██████████████████░` | +2% | Skills & Certificates | 99% | `███████████████████` | +9% |
| Ship Navigation | 97% | `███████████████████` | +27% | Combat | 99% | `███████████████████` | +9% |
| Modules & Overheating | 97% | `███████████████████` | +12% | Drones | 92% | `██████████████████` | +17% |
| NPC AI & Spawning | 92% | `███████████████████` | +32% | Agents & Missions | 95% | `███████████████████` | +25% |
| **POS** | 97% | `███████████████████` | +27% | Market | 92% | `██████████████████` | +32% |
| **Incursions** | 85% | `█████████████████░` | +85% | Fleet | 100% | `████████████████████` | +25% |
| **Wormholes** | 90% | `██████████████████` | +30% | Scanning | 99% | `███████████████████` | +19% |
| **Notifications** | 97% | `██████████████████` | +37% | **Standings** | 92% | `██████████████████` | +32% |
| **Faction Warfare** | 99% | `████████████████████` | +49% | Calendar | 93% | `███████████████████` | +33% |
| Mail & LSC | 93% | `███████████████████` | +33% | Contracts | 95% | `██████████████████` | +35% |
| Corporation | 93% | `███████████████████` | +28% | **Alliance** | 92% | `██████████████████` | +37% |
| **Sovereignty** | 95% | `███████████████████` | +35% | Science & Industry | 90% | `██████████████████` | +45% |
| Bookmark System | 95% | `██████████████████` | +25% | **Effects System** | 96% | `██████████████████` | +31% |
| Memory Mgmt | 20% | `████░░░░░░░░░░░░░░` | — | Deployables (MWD) | 97% | `███████████████████` | — |

---

## Details / Детали

> ✅ done · 🟡 partial · ❌ not implemented · **bold** = significantly improved vs upstream

### 1. Account / Character `██████████████████░` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Login (CRAM-MC), character creation, attributes | ✅ | ✅ |
| Neural remap, implants, boosters | ✅ | ✅ |
| Jump clones + per-clone implants | 🟡 | ✅ |
| Clone jump, install/destroy clones | 🟡 | ✅ |
| KillMail, image server (portraits/logos) | ✅ | ✅ |

### 2. Skills / Certificates `███████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Queue, train, certificates, implants | ✅ | ✅ |
| SP loss on T3 pod kill | ❌ | ✅ |

### 3. Ship Navigation `███████████████████` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Stargate jump, orbit, follow, approach | ✅ | ✅ |
| Warp-to-0, fleet warp, login warp | ✅ | ✅ |
| **Autopilot** — CmdWarpToStuffAutopilot, auto-jump via `.tr` teleport, multi-hop via CmdStop → gate follow | ❌ | ✅ |
| **AP follow** — hysteresis smoothing, gradual accel/decel, no abrupt Stop() | ❌ | ✅ |
| **AP post-jump** — jump cloak (60s) + invul (15s) + gate effects (effects.JumpOut/GateActivity) | ❌ | ✅ |
| Orbit desync fix, periodic position sync | ❌ | ✅ |
| Destiny crash fixes — bubble guard, use-after-free | ❌ | ✅ |
| **Warp scramble** blocks logoff/emergency warp | 🟡 | ✅ |

### 4. Combat & Crimewatch `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Lock, activate modules, damage, crimewatch | ✅ | ✅ |
| **CONCORD** — ×25 HP, delay by sec, −0.2 penalty, respawn, despawn | 🟡 | ✅ |
| **Security status formula** — `−2.5% × sysSec × (1 + (v−a)/100)` | ❌ | ✅ |
| **Faction police** — secStatus threshold spawn | ❌ | ✅ |
| Sentry guns vs NPC, kill rights | 🟡 | ✅ |
| Combat logoff, outlaw docking | 🟡 | ✅ |

### 5. Modules / Overheating `██████████████████` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Module groups, cyno, cloak, jump portal, titan bridge | ✅ | ✅ |
| **Overload** — Thermo check, heat damage, Nanite Paste, OverloadRack | ❌ | ✅ |

### 6. Drones `█████████████████░` 90%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Launch/scoop/return, AI states (10 subtypes) | 🟡 | ✅ |
| Skills, control range, bandwidth, damage bonuses | ✅ | ✅ |

### 7. NPC AI & Spawning `███████████████████` 92%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| NPC AI: orbit, target, engage, flee, call-for-help | ✅ | ✅ |
| Belt/gate rats, anomalies, incursions, convoys | ✅ | ✅ |
| **EWAR** — web, ECM, target paint from attributes | ❌ | ✅ |
| **Smartbomb/AoE** — splash in EmpFieldRange | ❌ | ✅ |
| **CONCORD AI** — criminal scan, full state machine | ❌ | ✅ |
| **Sentry AI vs NPC** — aggro on NPCs attacking players in highsec | ❌ | ✅ |
| NPC spawn position fix (no gate-bubble offset) | ❌ | ✅ |

### 8. Agents & Missions `███████████████████` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Missions (courier/mining/encounter/storyline), agents | ✅ | ✅ |
| Career agents, COSMOS, research, tutorial | 🟡 | ✅ |
| **Courier fixup** — destinationID, agentDB COALESCE | ❌ | ✅ |
| **Mission dungeon spawn** — on accept | ❌ | ✅ |
| LP store (faction + CONCORD) | 🟡 | ✅ |

### 9. Market `█████████████████░` 88%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Buy/sell orders, corp market, bots, price history | ✅ | ✅ |
| Trade skills, MarginTrading, escrow, expired auctions | ❌ | ✅ |

### 10. Contracts `██████████████████` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Item exchange, courier, auctions with bidding | 🟡 | ✅ |
| Auction item transfer, refund, notifications, auto-finish | ❌ | ✅ |

### 11. Corporation / Alliance `███████████████████` 93% / `██████████████████` 92%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Corp management, roles, offices, bills, wallets | ✅ | ✅ |
| Alliance creation, wars, voting, dividends | 🟡 | ✅ |
| Corp mail role filtering, war bills recurring | ❌ | ✅ |

### 12. Science & Industry `██████████████████` 90%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Manufacturing, copying, research (ME/PE) | ✅ | ✅ |
| **Invention** — formula with skills/meta/decryptor, T2 BPC | ❌ | ✅ |
| **Reverse Engineering** — chance calc + T2 BPC | ❌ | ✅ |
| **Remote job install** — with blueprints from remote stations | ❌ | ✅ |
| **Adjusted materials** — correct extra/waste/base | ❌ | ✅ |
| **POS assembly lines** — auto-create for POS structures | ❌ | ✅ |

### 13. POS `███████████████████` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Towers, modules, force field, fuel | ✅ | ✅ |
| Reinforced mode (fuel → stront → auto-online), CPU/PG | ❌ | ✅ |
| Reactions, weapon AI, skill checks, fuel notifications | ❌ | ✅ |

### 14. Wormholes `██████████████████` 90%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Lifecycle, K162, mass/lifetime tracking | ✅ | ✅ |
| System effects (Pulsar/Magnetar/etc) | ❌ | ✅ |

### 15. Fleet `████████████████████` 100%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Create/manage, wings, squads, boosts, broadcasts | ✅ | ✅ |
| Watchlist, voice chat methods | ❌ | ✅ |

### 16. Incursions `█████████████████░` 85%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| State machine, wave NPCs, influence, rewards | ❌ | ✅ |
| Gate camps, belt replacement, focus period, 5 simultaneous | ❌ | ✅ |
| **Constellation penalties** (−10/25/50%), **CONCORD LP bonus** | ❌ | ✅ |

### 17. Scanning `███████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Probes, scan signatures, anomalies, D-scan | ✅ | ✅ |

### 18. Faction Warfare `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Join/leave, militia stats (char/corp/alliance/faction) | 🟡 | ✅ |
| **Plex spawning** (Scout/Small/Medium/Large), LP from NPC/PvP/plex | ❌ | ✅ |
| **Faction patrols**, **LP exchange rates** | ❌ | ✅ |

### 19. Sovereignty `███████████████████` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| TCU 8h claim timer, vulnerable window | 🟡 | ✅ |
| IHub 2-cycle reinforcement, reinforce hour | ❌ | ✅ |
| Sov level (weeks), dev indices, upgrade effects | ❌ | ✅ |
| Outpost capture framework | ❌ | ✅ |
| **Alliance conflict zones** — SBU contested flag, ProcessSovStatusChanged, map display | ❌ | ✅ |

### 20. Deployables (Mobile Warp Disruptor + Probes) `███████████████████` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Deploy from cargo, space entity | ❌ | ✅ |
| Anchor/online via DogmaIM (effect dispatching) | ❌ | ✅ |
| Offlining timer (AttrAnchoringDelay fallback) | ❌ | ✅ |
| Anchor/online/unanchor timers from SDE attributes | ❌ | ✅ |
| Sec-level restriction (`AttrAnchoringSecurityLevelMax`) | ❌ | ✅ |
| Warp scramble bubble (Process()) when online | ❌ | ✅ |
| **Scramble cleanup** on range exit (no other sources) | ❌ | ✅ |
| **Scramble cleanup** on last entity removed from bubble | ❌ | ✅ |
| **Transient** — deleted from DB on server restart (Crucible behavior) | ❌ | ✅ |
| **Warp Disrupt Probe** — Interdiction Sphere Launcher, bubble, aggression (15min), highsec block | ❌ | ✅ |
| **Probe range** from `AttrWarpScrambleRange` (fallback 20km) | ❌ | ✅ |
| **Smartbombs (player)** — Smart_Bomb module, AoE splash, capacitor drain, effects.SmartBomb FX | ❌ | ✅ |
| **Warp intercept** — distance-aware pull-out via GOTO transition | ❌ | ✅ |
| **Invulnerability** — immune except to smartbombs/bombs | ❌ | ✅ |
| **Shuttle immunity** — group 31 hardcoded | ❌ | ✅ |
| **AttrWarpBubbleImmune** (Interdiction Nullifier) — all 6 check paths | ❌ | ✅ |

### 21. Ship Module Restrictions

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Block fitting in space (subcaps) | ❌ | ✅ |
| Allow capitals to fit in space | ❌ | ✅ |
| Allow T3 subsystem swap in space | ❌ | ✅ |

### 21. Effects System `██████████████████` 96%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Passive/online/active effects, implants/subsystems | ✅ | ✅ |
| Wormhole system effects, sov upgrade effects | ❌ | ✅ |

---

## Key Enhancements vs Upstream / Ключевые улучшения

- **Autopilot** — complete rewrite: auto-jump via `.tr` teleport, multi-hop via CmdStop → gate follow, 60s jump cloak, gate animation effects
- **POS** — reinforced mode, CPU/PG, reactions, weapon AI, skills, fuel notifications
- **Incursions** — full state machine, 5 simultaneous, named NPCs, gate camps, constellation penalties
- **Faction Warfare** — plex spawn, 3 LP channels, militia stats, patrols
- **Sovereignty** — TCU 8h claim + IHub 2-cycle reinforce + levels + upgrades + outpost capture
- **Science & Industry** — invention formula, reverse engineering, remote install, POS lines
- **Decompiled Crucible client** — 1082 Python scripts extracted, autopilot/session/starmap analyzed
- **Client cache analysis** — all 85 bulkdata cache files documented
- **SDE validation** — all NPC types verified against live API
- **~400 dungeon definitions** — anomaly, incursion, DED, data/relic, mission
