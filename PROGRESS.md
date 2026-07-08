# EVEmu Crucible — Fork Progress

> **Overall: `██████████░░░░░░░░` 96%** (upstream: 59.5%)  
> Last updated: 2026-07-09  
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

---

## Progress Overview

| System | % | Bar | System | % | Bar |
|--------|---|-----|--------|---|-----|
| Account & Character | 97% | `██████████████████░` | Skills & Certificates | 99% | `███████████████████` |
| Ship Navigation | 95% | `██████████████████` | Combat | 99% | `███████████████████` |
| Modules & Overheating | 95% | `██████████████████` | Drones | 90% | `█████████████████░` |
| NPC AI & Spawning | 80% | `████████████████░░` | Agents & Missions | 85% | `████████████████░` |
| **POS** | 95% | `██████████████████` | Market | 85% | `████████████████░` |
| **Incursions** | 75% | `██████████████░░░░` | Fleet | 98% | `██████████████████` |
| **Wormholes** | 80% | `████████████████░░` | Scanning | 99% | `███████████████████` |
| **Notifications** | 95% | `██████████████████` | **Standings** | 85% | `████████████████░` |
| **Faction Warfare** | 80% | `████████████████░░` | Calendar | 90% | `█████████████████░` |
| Mail & LSC | 85% | `████████████████░` | Contracts | 85% | `████████████████░` |
| Corporation | 80% | `████████████████░░` | Alliance | 60% | `████████████░░░░░░` |
| Sovereignty | 80% | `████████████████░░` | Science & Industry | 50% | `██████████░░░░░░░░` |
| Bookmark System | 95% | `██████████████████` | Effects System | 88% | `█████████████████░` |

---

## Detailed Breakdown

### 1. Account & Character Management `██████████████████░` 97%

| Feature | |
|---------|-|
| Account login (CRAM-MC) | ✅ |
| Character creation | ✅ |
| Character window, skills, certs, attributes | ✅ |
| Neural remap, implants, boosters | ✅ |
| Jump clones + per-clone implants | ✅ |
| Clone jump, install/destroy clones | ✅ |
| KillMail (XML blob, push notification) | ✅ |
| Image server (portrait/logo serving) | ✅ |

### 2. Skills & Certificates `███████████████████` 99%

| Feature | |
|---------|-|
| Browse, queue, train skills | ✅ |
| Certificate awarding | ✅ |
| Implant/booster processing | ✅ |
| SP loss on T3 pod kill | ✅ |

### 3. Ship Navigation `██████████████████` 95%

| Feature | |
|---------|-|
| Stargate jump, orbit, follow, approach | ✅ |
| Warp-to-0, fleet warp | ✅ |
| Autopilot chain (preserved across jumps) | ✅ |
| Login warp-in, combat logoff, emergency warp | ✅ |
| Warp scramble blocks logoff/emergency warp | ✅ |
| Warp alignment fix (no WARP→GOTO cancel) | ✅ |
| Warp exit crash fix (no SetBallPosition during WarpLoop) | ✅ |

### 4. Combat `███████████████████` 99%

| Feature | |
|---------|-|
| Lock target, activate modules, damage | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | ✅ |
| Sentry guns, kill rights | ✅ |
| Combat logoff (15min ghost, 60s emergency) | ✅ |

### 5. Modules & Overheating `██████████████████` 95%

| Feature | |
|---------|-|
| Module groups: engineering, electronic, weapons, shields, armor, EWAR, propulsion, rigs | ✅ |
| Cyno, covert cyno, cloak, jump portal, titan bridge | ✅ |
| Overload/DeOverload, OverloadRack/StopOverloadRack | ✅ |
| Thermodynamics skill check before overload | ✅ |
| HeatDamageCheck — slot-based damage spread | ✅ |
| Nanite Paste repair (consumes typeID 24694 from cargo) | ✅ |
| Module repair (nanite paste) | ✅ |
| Drone modules (DDA, nav, tracking, range) | ✅ |

### 6. Drones `█████████████████░` 90%

| Feature | |
|---------|-|
| Launch, scoop, return, abandon | ✅ |
| AI states: Attack, Assist, Guard, Mine, Focus Fire | ✅ |
| Subtypes: Combat, Sentry, EWAR, Logistics, Cap Drain, Mining, Fighter | ✅ |
| Skills: all drone skills implemented | ✅ |
| Control range, bandwidth, damage bonuses | ✅ |

### 7. NPC AI & Spawning `████████████████░░` 80%

| Feature | |
|---------|-|
| NPC AI: orbit, target, engage, call-for-help, retreat | ✅ |
| Belt rats, gate rats (dynamic/static spawns) | ✅ |
| Sentry guns, CONCORD, customs police | ✅ |
| Convoys (station-to-station with escort) | ✅ |
| NPC repair visuals (shield/armor beams) | ✅ |
| Anomaly NPCs (Ship-category typeIDs) | ✅ |

### 8. Agents & Missions `████████████████░` 85%

| Feature | |
|---------|-|
| Agent conversation, mission offer/accept/complete | ✅ |
| Courier, mining, encounter missions | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | ✅ |
| Career agents (29, 4 factions), COSMOS agents | ✅ |
| Research agents (21, tech fields) | ✅ |
| LP store (faction + CONCORD) | ✅ |

### 9. Market `████████████████░` 85%

| Feature | |
|---------|-|
| Buy/sell orders, immediate transactions | ✅ |
| Corporation market (office/permission checks) | ✅ |
| Market bots (Trader Joe — config-driven: groups, prices, quantities) | ✅ |
| Multi-region seeding (empire + NPC null-sec) | ✅ |
| Price history (GROUP BY fix, timer, seed data) | ✅ |
| Escrow handling (MarginTrading reduction) | ✅ |
| Trade skills — order count, range validation | ✅ |
| ModifyCharOrder — corp-aware escrow handling | ✅ |
| Expired auctions auto-finish (1m tick) | ✅ |

### 10. Contracts `████████████████░` 85%

| Feature | |
|---------|-|
| Item exchange, courier contracts | ✅ |
| Contract search, create, accept, complete | ✅ |
| Auction contracts (PlaceBid/FinishAuction) | ✅ |
| Auction item transfer — items to winner, refund losers | ✅ |
| Auction notifications — OnAuctionWon, OnAuctionCompleted | ✅ |
| Auto-finish expired auctions — 1m scheduled task | ✅ |

### 11. Corporation & Alliance `████████████████░░` 80%

| Feature | |
|---------|-|
| Corporation creation, management, roles | ✅ |
| Office rental, bills, wallet divisions | ✅ |
| Alliance creation, wars, bulletins, labels, contacts | ✅ |
| Alliance war decay timer | ✅ |
| Corp mail role filtering (crpRoles table + seeding) | ✅ |
| Faction Warfare corp/alliance join/leave | ✅ |

### 12. Science & Industry `██████████░░░░░░░░` 50%

| Feature | |
|---------|-|
| Manufacturing, copying, research (ME/PE) | ✅ |
| Invention (chance calc + T2 BPC) | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ✅ |
| Blueprint management (ME/PE/runs) | ✅ |

### 13. POS `██████████████████` 95%

| Feature | |
|---------|-|
| Tower anchoring/unanchor/online/offline | ✅ |
| Force field management, password | ✅ |
| POS modules (weapons, shields, jump bridges) | ✅ |
| Fuel consumption (invControlTowerResources, burn from cargo) | ✅ |
| Reinforced mode (fuel empty → stront timer → auto-Online) | ✅ |
| CPU/PG tracking (real-time usage counting) | ✅ |
| Orbitals (Anchor/Online/Complete/Unanchor) | ✅ |
| Reactors (LinkResource/RunMoonProcessCycle) | ✅ |
| Weapon AI (POS_AI — bubble scan, corp validation, damage) | ✅ |
| Assume/Relinquish control (controllerID, notifications) | ✅ |
| Skill checks (RequiredSkill1-6 for anchor/online) | ✅ |
| Fuel notifications (threshold tracking) | ✅ |
| FieldSE crash fix (IsGlobal flag, shield charge init) | ✅ |

### 14. Overheating `██████████████████` 95%

| Feature | |
|---------|-|
| Module overload/deoverload | ✅ |
| Ship heat generation/dissipation | ✅ |
| HeatDamageCheck — slot-based damage spread | ✅ |
| Thermodynamics skill check | ✅ |
| Nanite Paste repair | ✅ |
| OverloadRack/StopOverloadRack | ✅ |

### 15. Wormholes `████████████████░░` 80%

| Feature | |
|---------|-|
| Lifecycle: creation, tracking, mass/lifetime | ✅ |
| K162 generation, visual states | ✅ |
| Jump through (mass restriction, position sync) | ✅ |
| Collapse (mass/time depletion) | ✅ |

### 16. Fleet `██████████████████` 98%

| Feature | |
|---------|-|
| Create/manage fleet, wings, squads | ✅ |
| Fleet warp, regroup | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | ✅ |
| Broadcasts, watch list, fleet chat | ✅ |

### 17. Incursions `██████████████░░░░` 75%

| Feature | |
|---------|-|
| State machine (established → mobilized → withdrawal) | ✅ |
| Wave-based NPC spawning | ✅ |
| Influence tracking, mothership spawn | ✅ |
| Contest rewards (proportional to damage) | ✅ |
| Sansha incursion dungeons (VG/AS/HQ) | ✅ |
| CONCORD LP store integration | ✅ |

### 18. Scanning `███████████████████` 99%

| Feature | |
|---------|-|
| Launch/move probes, scan signatures | ✅ |
| Bookmark/warp to scan result | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ |

### 19. LSC (Chat) `████████████████░` 85%

| Feature | |
|---------|-|
| Local, corp, alliance channels | ✅ |
| Private conversations (Invite, custom temp channel, OnLSC notify) | ✅ |
| Channel creation (CreateChannel, Configure, Destroy) | ✅ |
| Mailing lists (Join, Leave, Create, GetJoinedLists) | ✅ |

### 20. EvE Mail & Notifications `██████████████████` 95%

| Feature | |
|---------|-|
| Send/receive mail (character, corp, alliance) | ✅ |
| Mailing lists (create/join/leave/delete) | ✅ |
| NotificationMgrService — full impl (8 methods) | ✅ |
| CreateNotification — DB persist + live push | ✅ |
| Rate limiting — 5/min, 50 recipients | ✅ |
| Spam filter — blocked contacts check | ✅ |
| OnMessage push — live notification on new mail | ✅ |
| Corp mail role filtering — crpRoles + roleMask | ✅ |
| Notification sources: bills, towers, agents, corps, structures | ✅ |

### 21. Standings `████████████████░` 85%

| Feature | |
|---------|-|
| Faction↔faction static standings | ✅ |
| Agent→character/corp mission standings | ✅ |
| Standing compositions (direct + corp + faction) | ✅ |
| Standing decay (2%/30d toward 0, inactive skip) | ✅ |
| Character↔character (PnP) standings | ✅ |
| SetStanding RPC — clamp, max ±2, notifications | ✅ |
| Security status (CONCORD awards/penalties) | ✅ |
| Used by FW enemy checks | ✅ |

### 22. Calendar `█████████████████░` 90%

| Feature | |
|---------|-|
| Event creation (personal, corp, alliance) | ✅ |
| Event editing (EditPersonal/Corp/Alliance) | ✅ |
| UpdateEventParticipants (add/remove invitees) | ✅ |
| Event deletion (soft delete) | ✅ |
| SendEventResponse (accept/decline/maybe) | ✅ |
| Event list + details via CalendarProxy | ✅ |
| PyWString→PyRep fix for client compatibility | ✅ |

### 23. Faction Warfare `████████████████░░` 80%

| Feature | |
|---------|-|
| Join/Leave as character (membership table + warFactionID) | ✅ |
| Join/Leave as corp (Director role check, cascading update) | ✅ |
| Join/Leave as alliance | ✅ |
| Withdraw join/leave (corp + alliance) | ✅ |
| IsEnemyFaction / IsEnemyCorporation | ✅ |
| GetFactionalWarStatus, GetCharacterRankInfo | ✅ |
| GetFactionCorporations, GetSystemsConqueredThisRun | ✅ |
| GetStats_Character (kills, losses, VP) | ✅ |
| Notifications (FWCorpJoin, FWCorpLeave) | ✅ |

### 24. Reverse Engineering `████████░░░░░░░░░░` 60%

| Feature | |
|---------|-|
| ActivityCheck (RE allowed at POS labs) | ✅ |
| Time calculation (researchTechTime + AdvLabOps) | ✅ |
| CompleteJob (chanceOfRE + RE skill +1%/lvl) | ✅ |
| T2 BPC output via parentBlueprintTypeID | ✅ |
| Skill checks (ReverseEngineering 3408) | ✅ |
| Datacore skill modifier | 🟡 |

### 25. Effects System `█████████████████░` 88%

| Feature | |
|---------|-|
| Passive, online, active effect processing | ✅ |
| Implant/booster processing | ✅ |
| Subsystem effect processing | 🟡 |

### 26. Cosmic Managers

| Manager | % | Bar | Notes |
|---------|---|-----|-------|
| Anomaly Manager | 85% | `████████████████░` | All site types, FW anomalies, QueueRespawn |
| Dungeon Manager | 70% | `████████████░░░░` | Anomaly/mission/unrated/incursion dungeons |
| Spawn Manager | 75% | `██████████████░░` | Dynamic/static spawning, wave progression |
| Wormhole Manager | 80% | `████████████████░░` | Full lifecycle |
| Belt Manager | 85% | `████████████████░` | Asteroid distribution |
| Civilian Manager | 10% | `██░░░░░░░░░░░░░░` | Basic NPC traffic (ConvoyAI) |

### 27. Memory Management `████░░░░░░░░░░░░░░` 20%

| Feature | |
|---------|-|
| PyInt cache (−10..255) | ✅ |
| `new PyInt` → `PyStatic.NewInt()` in hot paths | ✅ |
| RefPtr→shared_ptr migration (planning) | 🟡 |
| Valgrind leak tracking | 🟡 |

---

## Key Enhancements Over Upstream

| Feature | |
|---------|-|
| POS — full overhaul (fuel, reinforced, CPU/PG, orbitals, reactors, weapon AI, control, skills, FieldSE fix) | ✅ |
| Overheating — Thermodynamics, slot heat damage, Nanite Paste, OverloadRack | ✅ |
| Notifications — CreateNotification + 6 sources | ✅ |
| Standings decay + PnP SetStanding RPC | ✅ |
| Market — price history, trade skills, bot config, MarginTrading, expired auctions | ✅ |
| Calendar — EditEvent, UpdateEventParticipants, PyWString fix | ✅ |
| Contracts — FinishAuction item transfer + bid refund + auto-finish | ✅ |
| Corp mail roles — crpRoles table + roleMask filtering | ✅ |
| Warp alignment + warp exit crash fix | ✅ |
| Faction Warfare — full implementation | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ✅ |
| LSC — private conversations (Invite, OnLSC notify) | ✅ |
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
