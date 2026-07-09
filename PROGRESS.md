# EVEmu Crucible — Fork Progress

> **Our fork: `███████████████████░` 95%** · **Upstream: `████████░░░░░░░░░░░░` 59.5%**
> Last updated: 2026-07-10
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

---

## Progress Overview

### Upstream Base (EvEmu-Project/evemu_Crucible)

| System | % | Bar | System | % | Bar |
|--------|---|-----|--------|---|-----|
| Account & Character | 95% | `██████████████████` | Skills & Certificates | 90% | `██████████████████` |
| Ship Navigation | 70% | `████████████████░░` | Combat | 90% | `██████████████████` |
| Modules & Overheating | 85% | `█████████████████░` | Drones | 75% | `████████████████░░` |
| NPC AI & Spawning | 60% | `████████████░░░░░░` | Agents & Missions | 70% | `██████████████░░░░` |
| POS | 70% | `██████████████░░░░` | Market | 60% | `████████████░░░░░░` |
| Incursions | 0% | `░░░░░░░░░░░░░░░░░░` | Fleet | 75% | `██████████████░░░░` |
| Wormholes | 60% | `████████████░░░░░░` | Scanning | 80% | `████████████████░░` |
| Notifications | 60% | `████████████░░░░░░` | Standings | 60% | `████████████░░░░░░` |
| Faction Warfare | 50% | `██████████░░░░░░░░` | Calendar | 60% | `████████████░░░░░░` |
| Mail & LSC | 60% | `████████████░░░░░░` | Contracts | 60% | `████████████░░░░░░` |
| Corporation | 65% | `██████████████░░░░` | Alliance | 55% | `████████████░░░░░░` |
| Sovereignty | 60% | `████████████░░░░░░` | Science & Industry | 45% | `██████████░░░░░░░░` |
| Bookmark System | 70% | `██████████████░░░░` | Effects System | 65% | `██████████████░░░░` |

### Our Fork Enhancements

| System | % | Bar | Δ up | System | % | Bar | Δ up |
|--------|---|-----|------|--------|---|-----|------|
| Account & Character | 97% | `██████████████████░` | +2% | Skills & Certificates | 99% | `███████████████████` | +9% |
| Ship Navigation | 75% | `████████████████░░` | +5% | Combat | 99% | `███████████████████` | +9% |
| Modules & Overheating | 95% | `██████████████████` | +10% | Drones | 90% | `█████████████████░` | +15% |
| NPC AI & Spawning | 80% | `████████████████░░` | +20% | Agents & Missions | 85% | `█████████████████░` | +15% |
| **POS** | 97% | `███████████████████` | +27% | Market | 85% | `█████████████████░` | +25% |
| **Incursions** | 75% | `█████████████████░` | +75% | Fleet | 98% | `██████████████████` | +23% |
| **Wormholes** | 90% | `██████████████████` | +30% | Scanning | 99% | `███████████████████` | +19% |
| **Notifications** | 95% | `██████████████████` | +35% | **Standings** | 85% | `█████████████████░` | +25% |
| **Faction Warfare** | 90% | `█████████████████░` | +40% | Calendar | 90% | `█████████████████░` | +30% |
| Mail & LSC | 85% | `█████████████████░` | +25% | Contracts | 95% | `██████████████████` | +35% |
| Corporation | 87% | `█████████████████░` | +22% | **Alliance** | 85% | `█████████████████░` | +30% |
| **Sovereignty** | 90% | `█████████████████░` | +30% | Science & Industry | 55% | `████████████░░░░░░` | +10% |
| Bookmark System | 95% | `██████████████████` | +25% | **Effects System** | 95% | `██████████████████` | +30% |

---

## Detailed Breakdown

> _Legend:_ ✅ done · 🟡 partial · ❌ not done  
> _Системы с **жирным** названием — существенно улучшены относительно upstream_

---

### 1. Account & Character Management `██████████████████░` 97%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Account login (CRAM-MC) | ✅ | ✅ |
| Character creation | ✅ | ✅ |
| Character window, skills, certs, attributes | ✅ | ✅ |
| Neural remap, implants, boosters | ✅ | ✅ |
| Jump clones + per-clone implants | 🟡 | ✅ |
| Clone jump, install/destroy clones | 🟡 | ✅ |
| KillMail (XML blob, push notification) | ✅ | ✅ |
| Image server (portrait/logo serving) | ✅ | ✅ |

---

### 2. Skills & Certificates `███████████████████` 99%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Browse, queue, train skills | ✅ | ✅ |
| Certificate awarding | ✅ | ✅ |
| Implant/booster processing | ✅ | ✅ |
| SP loss on T3 pod kill | ❌ | ✅ |

---

### 3. Ship Navigation `████████████████░░` 75%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Stargate jump, orbit, follow, approach | ✅ | ✅ |
| Warp-to-0, fleet warp | ✅ | ✅ |
| Login warp-in, combat logoff, emergency warp | ✅ | ✅ |
| Warp scramble blocks logoff/emergency warp | 🟡 | ✅ |
| Autopilot chain (preserved across jumps) | ❌ | ❌ не работает |
| Orbit desync fix (distance fallback /6 → distance) | ❌ | ✅ |
| Periodic position sync (SetBallPosition every 5 tics) | ❌ | ✅ |
| Login warp overview fix (no SendSetState during warp) | ❌ | ✅ |

---

### 4. Combat `███████████████████` 99%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Lock target, activate modules, damage | ✅ | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | 🟡 | ✅ |
| Sentry guns, kill rights | 🟡 | ✅ |
| Combat logoff (15min ghost, 60s emergency) | 🟡 | ✅ |

---

### 5. Modules & Overheating `██████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Module groups: engineering, electronic, weapons, etc | ✅ | ✅ |
| Cyno, covert cyno, cloak, jump portal, titan bridge | 🟡 | ✅ |
| Overload/DeOverload, OverloadRack/StopOverloadRack | ❌ | ✅ |
| Thermodynamics skill check before overload | ❌ | ✅ |
| HeatDamageCheck — slot-based damage spread | ❌ | ✅ |
| Nanite Paste repair (consumes typeID 24694) | ❌ | ✅ |
| Module repair (nanite paste) | ❌ | ✅ |
| Drone modules (DDA, nav, tracking, range) | ✅ | ✅ |

---

### 6. Drones `█████████████████░` 90%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Launch, scoop, return, abandon | ✅ | ✅ |
| AI states: Attack, Assist, Guard, Mine, Focus Fire | 🟡 | ✅ |
| Subtypes: Combat, Sentry, EWAR, Logistics, etc | 🟡 | ✅ |
| Skills: all drone skills implemented | ✅ | ✅ |
| Control range, bandwidth, damage bonuses | ✅ | ✅ |

---

### 7. NPC AI & Spawning `████████████████░░` 80%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| NPC AI: orbit, target, engage, call-for-help, retreat | ✅ | ✅ |
| Belt rats, gate rats (dynamic/static spawns) | ✅ | ✅ |
| Sentry guns, CONCORD, customs police | 🟡 | ✅ |
| Convoys (station-to-station with escort) | ❌ | ✅ |
| NPC repair visuals (shield/armor beams) | 🟡 | ✅ |
| Anomaly NPCs (Ship-category typeIDs 33500-33523) | ❌ | ✅ |
| Civilian traffic with multi-system routes | ❌ | ✅ |
| NPC crosshairs (categoryID override in type cache) | ❌ | ✅ |

---

### 8. Agents & Missions `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Agent conversation, mission offer/accept/complete | ✅ | ✅ |
| Courier, mining, encounter missions | ✅ | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | 🟡 | ✅ |
| Career agents (29, 4 factions), COSMOS agents | 🟡 | 🟡 |
| Research agents (21, tech fields) | ✅ | ✅ |
| LP store (faction + CONCORD) | 🟡 | ✅ |

---

### 9. Market `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Buy/sell orders, immediate transactions | ✅ | ✅ |
| Corporation market (office/permission checks) | 🟡 | ✅ |
| Market bots (Trader Joe — config-driven) | ❌ | ✅ |
| Multi-region seeding (empire + NPC null-sec) | ❌ | ✅ |
| Price history (GROUP BY fix, timer, seed data) | ❌ | ✅ |
| Escrow handling (MarginTrading reduction) | ❌ | ✅ |
| Trade skills — order count, range validation | ❌ | ✅ |
| Expired auctions auto-finish (1m tick) | ❌ | ✅ |

---

### 10. Contracts `██████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Item exchange, courier contracts | 🟡 | ✅ |
| Contract search, create, accept, complete | 🟡 | ✅ |
| Auction contracts (PlaceBid/FinishAuction) | ❌ | ✅ |
| Auction item transfer — items to winner, refund losers | ❌ | ✅ |
| Auction notifications — OnAuctionWon, OnAuctionCompleted | ❌ | ✅ |
| Auto-finish expired auctions — 1m scheduled task | ❌ | ✅ |
| Nested containers in crate validation | ❌ | ✅ |
| forCorp in DeleteContract | ❌ | ✅ |
| Courier crateID, delivery flow, GetCourierContractFromItemID | ❌ | ✅ |

---

### 11. Corporation & Alliance `█████████████████░` 87% / `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Corporation creation, management, roles | ✅ | ✅ |
| Office rental, bills, wallet divisions | ✅ | ✅ |
| Alliance creation, wars, bulletins, labels, contacts | 🟡 | ✅ |
| Alliance war decay timer | ❌ | ✅ |
| Corp mail role filtering (crpRoles table + seeding) | ❌ | ✅ |
| Faction Warfare corp/alliance join/leave | 🟡 | ✅ |
| Corp voting (CreateVoteCase, CastVote, GetVotes) | ❌ | ✅ |
| Vote expiry processing (CheckVoteExpiry) | ❌ | ✅ |
| Alliance tax rate (SetTaxRate + alnAlliance.taxRate) | ❌ | ✅ |
| Alliance executor change (DeclareExecutorSupport) | ❌ | ✅ |
| Alliance member info — ceoID, memberCount, ticker, joinDate | ❌ | ✅ |
| **Alliance executor voting** (alnVoteItems/Options/Votes) | ❌ | ✅ |

---

### 12. Science & Industry `████████████░░░░░░` 55%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Manufacturing, copying, research (ME/PE) | ✅ | ✅ |
| Invention (chance calc + T2 BPC) | ❌ | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ❌ | ✅ |
| Blueprint management (ME/PE/runs) | ✅ | ✅ |

---

### 13. POS `███████████████████` 97%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Tower anchoring/unanchor/online/offline | ✅ | ✅ |
| Force field management, password | ✅ | ✅ |
| POS modules (weapons, shields, jump bridges) | ✅ | ✅ |
| Fuel consumption (invControlTowerResources) | 🟡 | ✅ |
| Reinforced mode (fuel empty → stront timer → auto-Online) | ❌ | ✅ |
| CPU/PG tracking (real-time usage) | ❌ | ✅ |
| Orbitals (Anchor/Online/Complete/Unanchor) | ❌ | ✅ |
| Reactors (LinkResource/RunMoonProcessCycle) | ❌ | ✅ |
| **Reaction cycle engine** (invTypeReactions input/output) | ❌ | ✅ |
| Weapon AI (POS_AI — bubble scan, corp validation, damage) | ❌ | ✅ |
| Assume/Relinquish control | ❌ | ✅ |
| Skill checks (RequiredSkill1-6) | ❌ | ✅ |
| Fuel notifications (threshold tracking) | ❌ | ✅ |
| **posReactorData DB persistence** | ❌ | ✅ |

---

### 14. Overheating `██████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Module overload/deoverload | 🟡 | ✅ |
| Ship heat generation/dissipation | ❌ | 🟡 |
| HeatDamageCheck — slot-based damage spread | ❌ | ✅ |
| Thermodynamics skill check | ❌ | ✅ |
| Nanite Paste repair | ❌ | ✅ |
| OverloadRack/StopOverloadRack | ❌ | ✅ |

---

### 15. Wormholes `██████████████████` 90%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Lifecycle: creation, tracking, mass/lifetime | ✅ | ✅ |
| K162 generation, visual states | ✅ | ✅ |
| Jump through (mass restriction, position sync) | ✅ | ✅ |
| Collapse (mass/time depletion) | ✅ | ✅ |
| **System effects** (Pulsar/Magnetar/Cataclysmic/etc) | ❌ | ✅ |

---

### 16. Fleet `██████████████████` 98%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Create/manage fleet, wings, squads | ✅ | ✅ |
| Fleet warp, regroup | ✅ | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | 🟡 | ✅ |
| Broadcasts, watch list, fleet chat | ✅ | ✅ |

---

### 17. Incursions `█████████████████░` 75%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| State machine (established → mobilized → withdrawal) | ❌ | ✅ |
| Wave-based NPC spawning | ❌ | ✅ |
| Influence tracking, mothership spawn | ❌ | ✅ |
| Contest rewards (proportional to damage) | ❌ | ✅ |
| Sansha incursion dungeons (VG/AS/HQ) | ❌ | ✅ |
| CONCORD LP store integration | ❌ | ✅ |

---

### 18. Scanning `███████████████████` 99%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Launch/move probes, scan signatures | ✅ | ✅ |
| Bookmark/warp to scan result | ✅ | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ | ✅ |

---

### 19. LSC (Chat) `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Local, corp, alliance channels | ✅ | ✅ |
| Private conversations (Invite, custom temp channel) | ❌ | ✅ |
| Channel creation (CreateChannel, Configure, Destroy) | ✅ | ✅ |
| Mailing lists (Join, Leave, Create, GetJoinedLists) | 🟡 | ✅ |

---

### 20. EvE Mail & Notifications `██████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Send/receive mail (character, corp, alliance) | ✅ | ✅ |
| Mailing lists (create/join/leave/delete) | 🟡 | ✅ |
| NotificationMgrService — full impl (8 methods) | 🟡 | ✅ |
| CreateNotification — DB persist + live push | ❌ | ✅ |
| Rate limiting — 5/min, 50 recipients | ❌ | ✅ |
| Spam filter — blocked contacts check | ✅ | ✅ |
| Corp mail role filtering — crpRoles + roleMask | ❌ | ✅ |
| Notification sources: bills, towers, agents, corps, structures | ❌ | ✅ |

---

### 21. Standings `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Faction↔faction static standings | ✅ | ✅ |
| Agent→character/corp mission standings | ✅ | ✅ |
| Standing compositions (direct + corp + faction) | 🟡 | ✅ |
| Standing decay (2%/30d toward 0, inactive skip) | ❌ | ✅ |
| Character↔character (PnP) standings | ❌ | ✅ |
| SetStanding RPC — clamp, max ±2, notifications | ❌ | ✅ |
| Security status (CONCORD awards/penalties) | ✅ | ✅ |

---

### 22. Calendar `█████████████████░` 90%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Event creation (personal, corp, alliance) | ✅ | ✅ |
| Event editing (EditPersonal/Corp/Alliance) | 🟡 | ✅ |
| UpdateEventParticipants (add/remove invitees) | ❌ | ✅ |
| Event deletion (soft delete) | ✅ | ✅ |
| SendEventResponse (accept/decline/maybe) | ✅ | ✅ |

---

### 23. Faction Warfare `█████████████████░` 90%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Join/Leave as character | 🟡 | ✅ |
| Join/Leave as corp/alliance | 🟡 | ✅ |
| IsEnemyFaction / IsEnemyCorporation | 🟡 | ✅ |
| GetFactionalWarStatus (with status field) | ❌ | ✅ |
| FW stats (kills, losses, VP) | 🟡 | ✅ |
| Notifications (FWCorpJoin, FWCorpLeave) | ❌ | ✅ |
| **FW plex spawning** (every 5m) | ❌ | ✅ |
| **FW LP store** (4 militia corps, ~350 offers each) | ❌ | ✅ |
| **LP exchange rates** (CONCORD, FW militia, navy) | ❌ | ✅ |

---

### 24. Reverse Engineering `████████████░░░░░░` 60%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| ActivityCheck (RE allowed at POS labs) | ❌ | ✅ |
| Time calculation (researchTechTime + AdvLabOps) | ❌ | ✅ |
| CompleteJob (chanceOfRE + RE skill +1%/lvl) | ❌ | ✅ |
| T2 BPC output via parentBlueprintTypeID | ❌ | ✅ |
| Datacore skill modifier | ❌ | ✅ |

---

### 25. Effects System `██████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Passive, online, active effect processing | ✅ | ✅ |
| Implant/booster processing | ✅ | ✅ |
| Subsystem effect processing | 🟡 | 🟡 |
| **Wormhole system effects** (SystemEffectMgr) | ❌ | ✅ |
| **Sovereignty upgrade effects** | ❌ | ✅ |

---

### 26. Cosmic Managers

| Manager | Upstream | Our Fork | Notes |
|---------|:--------:|:--------:|-------|
| Anomaly Manager | 40% | 85% | All site types, FW anomalies, QueueRespawn |
| Dungeon Manager | 40% | 75% | Anomaly/mission/unrated/incursion |
| Spawn Manager | 40% | 75% | Dynamic/static, wave progression |
| Wormhole Manager | 60% | 90% | Full lifecycle + effects |
| Belt Manager | 50% | 85% | Asteroid distribution |
| Civilian Manager | 0% | 90% | ConvoyAI + multi-system routes |
| Incursion Manager | 0% | 75% | State machine + contest rewards |

---

### 27. Memory Management `████░░░░░░░░░░░░░░` 20%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| PyInt cache (−10..255) | ❌ | ✅ |
| `new PyInt` → `PyStatic.NewInt()` in hot paths | ❌ | ✅ |
| RefPtr→shared_ptr migration (planning) | ❌ | 🟡 |
| Valgrind leak tracking | ❌ | 🟡 |

---

## Key Enhancements Over Upstream

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| POS — full overhaul (fuel, reinforced, CPU/PG, orbitals, reactors, weapon AI, control, skills, reaction cycle) | ❌ | ✅ |
| Overheating — Thermodynamics, slot heat damage, Nanite Paste, OverloadRack | ❌ | ✅ |
| Notifications — CreateNotification + 6 sources | ❌ | ✅ |
| Standings decay + PnP SetStanding RPC | ❌ | ✅ |
| Market — price history, trade skills, bot config, MarginTrading, expired auctions | ❌ | ✅ |
| Calendar — EditEvent, UpdateEventParticipants, PyWString fix | ❌ | ✅ |
| Contracts — FinishAuction item transfer + bid refund + auto-finish + nested containers + forCorp | ❌ | ✅ |
| Corp mail roles — crpRoles table + roleMask filtering | ❌ | ✅ |
| Warp alignment + warp exit crash fix + periodic position sync | ❌ | ✅ |
| Faction Warfare — full implementation + plex spawning + LP store | ❌ | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ❌ | ✅ |
| LSC — private conversations (Invite, OnLSC notify) | ❌ | ✅ |
| NPC Ship-category typeIDs (33500-33523) | ❌ | ✅ |
| Crimewatch full implementation | ❌ | ✅ |
| CONCORD + sentry + sec loss | ❌ | ✅ |
| Drones: full AI + 10 subtypes + skills | ❌ | ✅ |
| Incursions — state machine + contest rewards | ❌ | ✅ |
| Jump clones + per-clone implants | ❌ | ✅ |
| War decay timer | ❌ | ✅ |
| Ship fittings CRUD | ❌ | ✅ |
| Invention chance + T2 BPC | ❌ | ✅ |
| Mailing lists (full implementation) | ❌ | ✅ |
| SDE vs live ESI verification | ❌ | ✅ |
| **Alliance executor voting** | ❌ | ✅ |
| **Wormhole system effects** (6 types) | ❌ | ✅ |
| **Sovereignty upgrade effects** | ❌ | ✅ |
| **News ticker** (server RPC + LiveUpdate patch) | ❌ | ✅ |
| **Auto bulkDataChangeID** | ❌ | ✅ |
| **Corp window stability fixes** (KeyError:1, roles, columns) | ❌ | ✅ |
| **Orbit desync fix** (m_followDistance /6 → distance) | ❌ | ✅ |
