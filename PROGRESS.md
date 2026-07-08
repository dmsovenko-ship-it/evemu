# EVEmu Crucible — Fork Progress

> **Overall: ~95%** (upstream: 59.5%)  
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
| Warp alignment fix (no WARP→GOTO cancel) | ✅ |
| Warp exit crash fix (no SetBallPosition during WarpLoop) | ✅ |

## 4. Combat — 99%

| Feature | Status |
|---------|--------|
| Lock target, activate modules, damage | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | ✅ |
| Sentry guns, kill rights | ✅ |
| Combat logoff (15min ghost, 60s emergency) | ✅ |

## 5. Modules & Module Management — 92%

| Feature | Status |
|---------|--------|
| Module groups: engineering, electronic, weapons, shields, armor, EWAR, propulsion, rigs | ✅ |
| Cyno, covert cyno, cloak, jump portal, titan bridge | ✅ |
| **Overheating** — Thermodynamics skill check | ✅ |
| **Overheating** — HeatDamageCheck slot-based damage spread | ✅ |
| **Overheating** — Nanite Paste repair (consumes typeID 24694) | ✅ |
| Module repair (nanite paste) | ✅ |
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

## 8. Agents & Missions — 85%

| Feature | Status |
|---------|--------|
| Agent conversation, mission offer/accept/complete | ✅ |
| Courier, mining, encounter missions | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | ✅ |
| Career agents (29, 4 factions), COSMOS agents | ✅ |
| Research agents (21, tech fields) | ✅ |
| LP store (faction + CONCORD) | ✅ |

## 9. Market — 75%

| Feature | Status |
|---------|--------|
| Buy/sell orders, immediate transactions | ✅ |
| Corporation market (office/permission checks) | ✅ |
| Market bots (Trader Joe — config-driven) | ✅ |
| Multi-region seeding (empire + NPC null-sec) | ✅ |
| Price history (GROUP BY fix, timer, seed data) | ✅ |
| Escrow handling (MarginTrading reduction) | ✅ |
| **Trade skills** — order count, range validation | ✅ |
| **ModifyCharOrder** — corp-aware escrow handling | ✅ |

## 10. Contracts — 70%

| Feature | Status |
|---------|--------|
| Item exchange, courier contracts | ✅ |
| Contract search, create, accept, complete | ✅ |
| Auction contracts (PlaceBid/FinishAuction) | ✅ |
| **Auction item transfer** — items to winner, refund losers | ✅ |
| **Auction notifications** — OnAuctionWon, OnAuctionCompleted | ✅ |

## 11. Corporation & Alliance — 70%

| Feature | Status |
|---------|--------|
| Corporation creation, management, roles | ✅ |
| Office rental, bills, wallet divisions | ✅ |
| Alliance creation, wars, bulletins, labels, contacts | ✅ |
| Alliance war decay timer | ✅ |
| Corp mail role filtering (crpRoles table) | ✅ |

## 12. Science & Industry — 40%

| Feature | Status |
|---------|--------|
| Manufacturing, copying, research (ME/PE) | ✅ |
| Invention (chance calc + T2 BPC) | ✅ |
| Reverse engineering | 🟡 |
| Blueprint management (ME/PE/runs) | ✅ |

## 13. POS — 90%

| Feature | Status |
|---------|--------|
| Tower anchoring/unanchor/online/offline | ✅ |
| Force field management, password | ✅ |
| POS modules (weapons, shields, jump bridges) | ✅ |
| **Fuel consumption** (invControlTowerResources, burn from cargo) | ✅ |
| **Reinforced mode** (fuel empty → stront timer → auto-Online) | ✅ |
| **CPU/PG tracking** (Real-time usage counting) | ✅ |
| **Orbitals** (Anchor/Online/Complete/Unanchor) | ✅ |
| **Reactors** (LinkResource/RunMoonProcessCycle) | ✅ |
| **Weapon AI** (POS_AI — bubble scan, corp validation, damage) | ✅ |
| **Assume/Relinquish control** (controllerID, notifications) | ✅ |
| **Skill checks** (RequiredSkill1-6 for anchor/online) | ✅ |
| **Fuel notifications** (threshold tracking, calendar stub) | ✅ |

## 14. Overheating — 80%

| Feature | Status |
|---------|--------|
| Module overload/deoverload (via GenericModule) | ✅ |
| Ship heat generation (ProcessHeat, GenerateHeat) | ✅ |
| Ship heat dissipation (DissipateHeat) | ✅ |
| **HeatDamageCheck** — slot-based damage spread | ✅ |
| **Thermodynamics skill check** before overload | ✅ |
| **Nanite Paste repair** (consumes typeID 24694 from cargo) | ✅ |
| OverloadRack/StopOverloadRack | 🟡 |

## 15. Wormholes — 80%

| Feature | Status |
|---------|--------|
| Lifecycle: creation, tracking, mass/lifetime | ✅ |
| K162 generation, visual states | ✅ |
| Jump through (mass restriction, position sync) | ✅ |
| Collapse (mass/time depletion) | ✅ |

## 16. Fleet — 98%

| Feature | Status |
|---------|--------|
| Create/manage fleet, wings, squads | ✅ |
| Fleet warp, regroup | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | ✅ |
| Broadcasts, watch list, fleet chat | ✅ |

## 17. Incursions — 75%

| Feature | Status |
|---------|--------|
| State machine (established → mobilized → withdrawal) | ✅ |
| Wave-based NPC spawning | ✅ |
| Influence tracking, mothership spawn | ✅ |
| Contest rewards (proportional to damage) | ✅ |
| Sansha incursion dungeons (VG/AS/HQ) | ✅ |
| CONCORD LP store integration | ✅ |

## 18. Scanning & Probing — 99%

| Feature | Status |
|---------|--------|
| Launch/move probes, scan signatures | ✅ |
| bookmark/warp to scan result | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ |

## 19. LSC (Chat) — 70%

| Feature | Status |
|---------|--------|
| Local, corp, alliance channels | ✅ |
| Private conversations, channel creation | 🟡 |

## 20. EvE Mail & Notifications — 90%

| Feature | Status |
|---------|--------|
| Send/receive mail (character, corp, alliance) | ✅ |
| Mailing lists (create/join/leave/delete) | ✅ |
| **NotificationMgrService** — full impl (8 methods) | ✅ |
| **CreateNotification** — DB persist + live push | ✅ |
| **Rate limiting** — 5/min, 50 recipients | ✅ |
| **Spam filter** — blocked contacts check | ✅ |
| **OnMessage push** — live notification on new mail | ✅ |
| **Corp mail role filtering** — crpRoles + roleMask | ✅ |
| Notification sources: bills, towers, agents, corps, structures | ✅ |

## 21. Standings — 75%

| Feature | Status |
|---------|--------|
| Faction↔faction static standings | ✅ |
| Agent→character/corp mission standings | ✅ |
| Standing compositions (direct + corp + faction) | ✅ |
| **Standing decay** (2%/30d toward 0, inactive skip) | ✅ |
| **Character↔character (PnP) standings** | ✅ |
| **SetStanding RPC** — clamp, max ±2, notifications | ✅ |
| Security status (CONCORD awards/penalties) | ✅ |

## 22. Calendar — 85%

| Feature | Status |
|---------|--------|
| Event creation (personal, corp, alliance) | ✅ |
| Event editing (EditPersonal/Corp/Alliance) | ✅ |
| UpdateEventParticipants (add/remove invitees) | ✅ |
| Event deletion (soft delete) | ✅ |
| SendEventResponse (accept/decline/maybe) | ✅ |
| Event list + details via CalendarProxy | ✅ |
| Reminder system | 🟡 |

## 23. Effects System — 88%

| Feature | Status |
|---------|--------|
| Passive, online, active effect processing | ✅ |
| Implant/booster processing | ✅ |
| Subsystem effect processing | 🟡 |

## 24. Cosmic Managers — 85%

| Manager | % | Notes |
|---------|---|-------|
| Anomaly Manager | 85% | All site types, FW anomalies, QueueRespawn |
| Dungeon Manager | 70% | Anomaly/mission/unrated/incursion dungeons |
| Spawn Manager | 75% | Dynamic/static spawning, wave progression |
| Wormhole Manager | 80% | Full lifecycle |
| Belt Manager | 85% | Asteroid distribution |
| Civilian Manager | 10% | Basic NPC traffic (ConvoyAI) |

## 25. Memory Management — 20%

| Feature | Status |
|---------|--------|
| PyInt cache (−10..255) | ✅ |
| `new PyInt` → `PyStatic.NewInt()` in hot paths | ✅ |
| RefPtr→shared_ptr migration (planning) | 🟡 |
| Valgrind leak tracking | 🟡 |

---

## Key Enhancements Over Upstream

| Feature | Added |
|---------|-------|
| POS — fuel, reinforced, CPU/PG, orbitals, reactors, weapon AI, assume control, skill checks | ✅ |
| Overheating — Thermodynamics check, slot heat damage, Nanite Paste repair | ✅ |
| Notifications — CreateNotification, NotificationMgrService, bill/tower/agent/corp/struct sources | ✅ |
| Standings decay + PnP SetStanding RPC | ✅ |
| Market — price history seed+GROUP BY+timer, trade skills (count/range/MarginTrading), ModifyCharOrder corp fix | ✅ |
| Calendar — EditEvent, UpdateEventParticipants, SendEventResponse | ✅ |
| Contracts — FinishAuction item transfer + bid refund + notifications | ✅ |
| Market bot — config-driven OrdersPerSystem, DupeOrdersPerSystem, MinBuy/SellAmount | ✅ |
| Corp mail roles — crpRoles table + roleMask filtering | ✅ |
| Warp alignment + warp exit crash fix | ✅ |
| NPC Ship-category typeIDs (33500-33523) | ✅ |
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
