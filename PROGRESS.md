# EVEmu Crucible — Fork Progress

> **Overall: ~82%** (upstream: 59.5%)  
> Last updated: 2026-07-08  
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

---

## 1. Account & Character Management — 97%

| Feature | Status |
|---------|--------|
| Account login (CRAM-MC) | ✅ |
| Character creation | ✅ |
| Character window, skills, certs, attributes | ✅ |
| Neural remap, implants, boosters | ✅ |
| Jump clones + per-clone implants | ✅ |
| Clone jump, install/destroy clones | ✅ |
| KillMail (XML blob, push notification) | ✅ |
| Image server (portrait/logo serving) | ✅ |

## 2. Skills & Certificates — 99%

| Feature | Status |
|---------|--------|
| Browse, queue, train skills | ✅ |
| Certificate awarding | ✅ |
| Implant/booster processing | ✅ |
| SP loss on T3 pod kill | ✅ |

## 3. Ship Navigation (Destiny) — 95%

| Feature | Status |
|---------|--------|
| Stargate jump, orbit, follow, approach | ✅ |
| Warp-to-0, fleet warp (WarpToMember/WarpFleetToMember) | ✅ |
| Autopilot chain (preserved across jumps) | ✅ |
| Login warp-in, combat logoff, emergency warp | ✅ |
| Warp scramble blocks logoff/emergency warp | ✅ |

## 4. Combat — 99%

| Feature | Status |
|---------|--------|
| Lock target, activate modules, damage | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | ✅ |
| Sentry guns, kill rights | ✅ |
| Combat logoff (15min ghost, 60s emergency) | ✅ |

## 5. Modules & Module Management — 80-92%

| Feature | Status |
|---------|--------|
| Module groups: engineering, electronic, weapons, shields, armor, EWAR, propulsion, rigs | ✅ |
| Cyno, covert cyno, cloak, jump portal, titan bridge | ✅ |
| Heat/overload systems | 🟡 |
| Module repair (nanite paste) | 🟡 |
| Drone modules (DDA, nav, tracking, range) | ✅ |

## 6. Drones — 90%

| Feature | Status |
|---------|--------|
| Launch, scoop, return, abandon | ✅ |
| AI states: Attack, Assist, Guard, Mine, Focus Fire | ✅ |
| Subtypes: Combat, Sentry, EWAR (ECM/web/scan), Logistics, Cap Drain, Mining, Fighter | ✅ |
| Skills: all drone skills implemented | ✅ |
| Control range, bandwidth, damage bonuses | ✅ |

## 7. NPC AI & Spawning — 80%

| Feature | Status |
|---------|--------|
| NPC AI: orbit, target, engage, call-for-help, retreat | ✅ |
| Belt rats, gate rats (dynamic/static spawns) | ✅ |
| Sentry guns, CONCORD, customs police | ✅ |
| Convoys (station-to-station with escort) | ✅ |
| NPC repair visuals (shield/armor beams) | ✅ |
| Anomaly NPCs (Ship-category typeIDs) | ✅ |

## 8. Agents & Missions — 70-85%

| Feature | Status |
|---------|--------|
| Agent conversation, mission offer/accept/complete | ✅ |
| Courier, mining, encounter missions | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | ✅ |
| Career agents (29, 4 factions), COSMOS agents | ✅ |
| Research agents (21, tech fields) | ✅ |
| LP store (faction + CONCORD) | ✅ |

## 9. Market — 55%

| Feature | Status |
|---------|--------|
| Buy/sell orders, immediate transactions | ✅ |
| Corporation market (office/permission checks) | ✅ |
| Market bots (Trader Joe) | ✅ |
| Multi-region seeding (empire + NPC null-sec) | ✅ |
| Price history | 🟡 |
| Escrow handling | 🟡 |

## 10. Contracts — 50%

| Feature | Status |
|---------|--------|
| Item exchange, courier contracts | ✅ |
| Contract search, create, accept, complete | ✅ |
| Auction contracts (PlaceBid/FinishAuction + ISK) | ✅ |
| Full auction lifecycle (item transfer, bid refunds) | 🟡 |

## 11. Corporation & Alliance — 30-70%

| Feature | Status |
|---------|--------|
| Corporation creation, management, roles | ✅ |
| Office rental, bills, wallet divisions | ✅ |
| Alliance creation, wars, bulletins, labels, contacts | ✅ |
| Alliance war decay timer | ✅ |

## 12. Science & Industry — 40%

| Feature | Status |
|---------|--------|
| Manufacturing, copying, research (ME/PE) | ✅ |
| Invention (chance calc + T2 BPC) | ✅ |
| Reverse engineering | 🟡 |
| Blueprint management (ME/PE/runs) | ✅ |

## 13. POS — 65%

| Feature | Status |
|---------|--------|
| Tower anchoring/onanchor/online/offline | ✅ |
| Force field management, password | ✅ |
| POS modules (weapons, shields, jump bridges) | ✅ |
| Shield reinforcement (25% → invulnerable timer) | 🟡 |
| Fuel consumption, stront timer | 🟡 |

## 14. Wormholes — 80%

| Feature | Status |
|---------|--------|
| Lifecycle: creation, tracking, mass/lifetime | ✅ |
| K162 generation, visual states | ✅ |
| Jump through (mass restriction, position sync) | ✅ |
| Collapse (mass/time depletion) | ✅ |

## 15. Fleet — 98%

| Feature | Status |
|---------|--------|
| Create/manage fleet, wings, squads | ✅ |
| Fleet warp, regroup | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | ✅ |
| Broadcasts, watch list, fleet chat | ✅ |

## 16. Incursions — 75%

| Feature | Status |
|---------|--------|
| State machine (established → mobilized → withdrawal) | ✅ |
| Wave-based NPC spawning | ✅ |
| Influence tracking, mothership spawn | ✅ |
| Contest rewards (proportional to damage) | ✅ |
| Sansha incursion dungeons (VG/AS/HQ) | ✅ |
| CONCORD LP store integration | ✅ |

## 17. Scanning & Probing — 99%

| Feature | Status |
|---------|--------|
| Launch/move probes, scan signatures | ✅ |
| bookmark/warp to scan result | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ |

## 18. LSC (Chat) — 70%

| Feature | Status |
|---------|--------|
| Local, corp, alliance channels | ✅ |
| Private conversations, channel creation | 🟡 |

## 19. EvE Mail — 45%

| Feature | Status |
|---------|--------|
| Send/receive mail (character, corp, alliance) | ✅ |
| Mailing lists (create/join/leave/delete) | ✅ |
| Notification delivery | 🟡 |

## 20. Standings — 40%

| Feature | Status |
|---------|--------|
| Faction↔faction static standings | ✅ |
| Agent→character/corp mission standings | ✅ |
| Standing compositions (direct + corp + faction) | ✅ |
| Standing decay | 🟡 |
| Character↔character (PnP) standings | 🟡 |

## 21. Effects System — 88%

| Feature | Status |
|---------|--------|
| Passive, online, active effect processing | ✅ |
| Implant/booster processing | ✅ |
| Subsystem effect processing | 🟡 |

## 22. Cosmic Managers — 70-85%

| Manager | % | Notes |
|---------|---|-------|
| Anomaly Manager | 85% | All site types, FW anomalies, QueueRespawn |
| Dungeon Manager | 70% | Anomaly/mission/unrated/incursion dungeons |
| Spawn Manager | 75% | Dynamic/static spawning, wave progression |
| Wormhole Manager | 80% | Full lifecycle |
| Belt Manager | 85% | Asteroid distribution |
| Civilian Manager | 10% | Basic NPC traffic (ConvoyAI) |

## 23. Memory Management — 20%

| Feature | Status |
|---------|--------|
| PyInt cache (−10..255) | ✅ |
| `new PyInt` → `PyStatic.NewInt()` in hot paths | ✅ |
| RefPtr→shared_ptr migration (planning) | 🟡 |
| Valgrind leak tracking | 🟡 |

---

## Key Enhancements Over Upstream

| Feature | Status |
|---------|--------|
| NPC Ship-category typeIDs (33500-33523) | ✅ |
| Warp-to-0, fleet warp, autopilot chain | ✅ |
| Crimewatch full implementation | ✅ |
| CONCORD + sentry + sec loss | ✅ |
| Drones: full AI + 10 subtypes + skills | ✅ |
| Kill rights (grant, activate, Limited Engagement) | ✅ |
| Contraband / customs police | ✅ |
| Incursion state machine + contest rewards | ✅ |
| Jump clones + per-clone implants | ✅ |
| War decay timer | ✅ |
| Contract auctions + ISK transfer | ✅ |
| Ship fittings CRUD | ✅ |
| Invention chance + T2 BPC | ✅ |
| Mailing lists (full implementation) | ✅ |
| SDE vs live ESI verification | ✅ |
| Agent portrait fallback (images.evetech.net) | ✅ |
| GM commands with colored output | ✅ |
