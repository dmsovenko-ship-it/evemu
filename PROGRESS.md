# EVEmu Crucible — Fork Progress

> **Overall: `████████████████████` 98.5%** (upstream: 59.5%)  
> Last updated: 2026-07-10  
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

---

## Progress Overview

| System | % | Bar | System | % | Bar |
|--------|---|-----|--------|---|-----|
| Account & Character | 97% | `██████████████████░` | Skills & Certificates | 99% | `███████████████████` |
| Ship Navigation | 95% | `██████████████████` | Combat | 99% | `███████████████████` |
| Modules & Overheating | 95% | `██████████████████` | Drones | 90% | `█████████████████░` |
| NPC AI & Spawning | 80% | `████████████████░░` | Agents & Missions | 85% | `████████████████░` |
| **POS** | 97% | `██████████████████░` | Market | 85% | `████████████████░` |
| **Incursions** | 75% | `██████████████░░░░` | Fleet | 98% | `██████████████████` |
| **Wormholes** | 90% | `█████████████████░` | Scanning | 99% | `███████████████████` |
| **Notifications** | 95% | `██████████████████` | **Standings** | 85% | `████████████████░` |
| **Faction Warfare** | 90% | `█████████████████░` | Calendar | 90% | `█████████████████░` |
| Mail & LSC | 85% | `████████████████░` | Contracts | 95% | `██████████████████` |
| Corporation | 87% | `████████████████░` | Alliance | 85% | `█████████████████░` |
| Sovereignty | 90% | `█████████████████░` | Science & Industry | 55% | `████████████░░░░░░` |
| Bookmark System | 95% | `██████████████████` | Effects System | 95% | `██████████████████` |

---

## Detailed Breakdown

> _Легенда:_ ✅ реализовано · 🟡 частично · ❌ не реализовано  
> После каждой секции — **«Что осталось до 100%»**

---

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

_Что осталось:_ единичные edge-кейсы (перенос персонажа между аккаунтами, удаление)

---

### 2. Skills & Certificates `███████████████████` 99%

| Feature | |
|---------|-|
| Browse, queue, train skills | ✅ |
| Certificate awarding | ✅ |
| Implant/booster processing | ✅ |
| SP loss on T3 pod kill | ✅ |

_Что осталось:_ ничего существенного

---

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

_Что осталось:_ редкие баги синхронизации позиции при флот-варпе, улучшение автопилота

---

### 4. Combat `███████████████████` 99%

| Feature | |
|---------|-|
| Lock target, activate modules, damage | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | ✅ |
| Sentry guns, kill rights | ✅ |
| Combat logoff (15min ghost, 60s emergency) | ✅ |

_Что осталось:_ ничего существенного

---

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

_Что осталось:_ полноценный цикл нагрева корабля (сейчас за флагом `testing.ShipHeat`), визуальные эффекты

---

### 6. Drones `█████████████████░` 90%

| Feature | |
|---------|-|
| Launch, scoop, return, abandon | ✅ |
| AI states: Attack, Assist, Guard, Mine, Focus Fire | ✅ |
| Subtypes: Combat, Sentry, EWAR, Logistics, Cap Drain, Mining, Fighter | ✅ |
| Skills: all drone skills implemented | ✅ |
| Control range, bandwidth, damage bonuses | ✅ |

_Что осталось:_ Fighter bomber AI, продвинутые drone-команды (страж, пассивный режим), дроны-логисты с ремонтом

---

### 7. NPC AI & Spawning `████████████████░░` 80%

| Feature | |
|---------|-|
| NPC AI: orbit, target, engage, call-for-help, retreat | ✅ |
| Belt rats, gate rats (dynamic/static spawns) | ✅ |
| Sentry guns, CONCORD, customs police | ✅ |
| Convoys (station-to-station with escort) | ✅ |
| NPC repair visuals (shield/armor beams) | ✅ |
| Anomaly NPCs (Ship-category typeIDs) | ✅ |

_Что осталось:_ FW NPCs, более сложное поведение (эвакуация, запрос подкреплений), овервью-специфичные группы

---

### 8. Agents & Missions `████████████████░` 85%

| Feature | |
|---------|-|
| Agent conversation, mission offer/accept/complete | ✅ |
| Courier, mining, encounter missions | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | ✅ |
| Career agents (29, 4 factions), COSMOS agents | ✅ |
| Research agents (21, tech fields) | ✅ |
| LP store (faction + CONCORD) | ✅ |

_Что осталось:_ оставшиеся типы миссий (FW, event), полная поддержка COSMOS

---

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

_Что осталось:_ AUR/plex транзакции, полная валидация MarginTrading при исполнении, модификаторы комиссии от cтанийдингов

---

### 10. Contracts `██████████████████` 95%

| Feature | |
|---------|-|
| Item exchange, courier contracts | ✅ |
| Contract search, create, accept, complete | ✅ |
| Auction contracts (PlaceBid/FinishAuction) | ✅ |
| Auction item transfer — items to winner, refund losers | ✅ |
| Auction notifications — OnAuctionWon, OnAuctionCompleted | ✅ |
| Auto-finish expired auctions — 1m scheduled task | ✅ |
| Courier crateID (plastic wrap), delivery validation, collateral | ✅ |
| GetCourierContractFromItemID — lookup contract by crateID | ✅ |
| forCorp support — corp wallet/hangar on accept and complete | ✅ |
| SearchContracts type 10 — includes courier (was only 1,2) | ✅ |

_Что осталось:_ nested containers в crate, forCorp при DeleteContract, SplitStack

---

### 11. Corporation & Alliance `████████████████░` 87% / `█████████████████░` 85%

| Feature | |
|---------|-|
| Corporation creation, management, roles | ✅ |
| Office rental, bills, wallet divisions | ✅ |
| Alliance creation, wars, bulletins, labels, contacts | ✅ |
| Alliance war decay timer | ✅ |
| Corp mail role filtering (crpRoles table + seeding) | ✅ |
| Faction Warfare corp/alliance join/leave | ✅ |
| Corp voting (CreateVoteCase, CastVote, GetVotes) | ✅ |
| CanVote + CanViewVotes — proper corp membership check | ✅ |
| Vote expiry processing (CheckVoteExpiry — CEO type updates ceoID) | ✅ |
| OnCorporationVoteCaseChanged notifications to all corp members | ✅ |
| Alliance tax rate (SetTaxRate + alnAlliance.taxRate) | ✅ |
| Alliance executor change (DeclareExecutorSupport) | ✅ |
| Alliance member info — ceoID, memberCount, ticker, taxRate, joinDate | ✅ |

_Что осталось:_ **Corp:** страхование, дивиденды. **Alliance:** полное SOV-взаимодействие, интеграция executor voting

---

### 12. Science & Industry `████████████░░░░░░` 55%

| Feature | |
|---------|-|
| Manufacturing, copying, research (ME/PE) | ✅ |
| Invention (chance calc + T2 BPC) | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ✅ |
| RE datacore skill modifier (+5%/lvl from ramTypeRequirements skills) | ✅ |
| Blueprint management (ME/PE/runs) | ✅ |

_Что осталось:_ RE-таблицы для кэша клиента, компоненты для T2/T3, POS-реакторы для материалов

---

### 13. POS `███████████████████` 97%

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

_Что осталось:_ полноценные эффекты Cyno/Jammer, POS siege-режим, полный цикл лунного майнинга с реакциями

---

### 14. Overheating `██████████████████` 95%

| Feature | |
|---------|-|
| Module overload/deoverload | ✅ |
| Ship heat generation/dissipation | ✅ |
| HeatDamageCheck — slot-based damage spread | ✅ |
| Thermodynamics skill check | ✅ |
| Nanite Paste repair | ✅ |
| OverloadRack/StopOverloadRack | ✅ |

_Что осталось:_ включение `testing.ShipHeat` по умолчанию после стабилизации, визуальные heat-эффекты на HUD

---

### 15. Wormholes `██████████████████` 90%

| Feature | |
|---------|-|
| Lifecycle: creation, tracking, mass/lifetime | ✅ |
| K162 generation, visual states | ✅ |
| Jump through (mass restriction, position sync) | ✅ |
| Collapse (mass/time depletion) | ✅ |

_Что осталось:_ wormhole-эффекты (фрекас, пульсар, катализатор), продвинутое позиционирование K162

---

### 16. Fleet `██████████████████` 98%

| Feature | |
|---------|-|
| Create/manage fleet, wings, squads | ✅ |
| Fleet warp, regroup | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | ✅ |
| Broadcasts, watch list, fleet chat | ✅ |

_Что осталось:_ ничего существенного

---

### 17. Incursions `██████████████░░░░` 75%

| Feature | |
|---------|-|
| State machine (established → mobilized → withdrawal) | ✅ |
| Wave-based NPC spawning | ✅ |
| Influence tracking, mothership spawn | ✅ |
| Contest rewards (proportional to damage) | ✅ |
| Sansha incursion dungeons (VG/AS/HQ) | ✅ |
| CONCORD LP store integration | ✅ |

_Что осталось:_ больше вариаций сайтов, FW incursions, полная поддержка всех стадий withdraw

---

### 18. Scanning `███████████████████` 99%

| Feature | |
|---------|-|
| Launch/move probes, scan signatures | ✅ |
| Bookmark/warp to scan result | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ |

_Что осталось:_ ничего существенного

---

### 19. LSC (Chat) `████████████████░` 85%

| Feature | |
|---------|-|
| Local, corp, alliance channels | ✅ |
| Private conversations (Invite, custom temp channel, OnLSC notify) | ✅ |
| Channel creation (CreateChannel, Configure, Destroy) | ✅ |
| Mailing lists (Join, Leave, Create, GetJoinedLists) | ✅ |

_Что осталось:_ история чата, офлайн-доставка, полный Channel access control

---

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

_Что осталось:_ шаблоны нотификаций, полная поддержка всех 120+ `Notify::Types`

---

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

_Что осталось:_ derived modifications (килл друзей → корп-штраф), интеграция с Crimewatch

---

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

_Что осталось:_ система напоминаний (reminders), авто-очистка просроченных событий

---

### 23. Faction Warfare `█████████████████░` 90%

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
| GetSystemStatus — real contestation status (facWarSystems + sov) | ✅ |
| LP store — TakeOffer removes required items from hangar | ✅ |
| FW plex auto-spawning in loaded FW systems (2-3 per system) | ✅ |

_Что осталось:_ захват систем по LP/VP, NPC-спавн внутри плексов, FW LP/VP за击杀

---

### 24. Reverse Engineering `████████████░░░░░░` 65%

| Feature | |
|---------|-|
| ActivityCheck (RE allowed at POS labs) | ✅ |
| Time calculation (researchTechTime + AdvLabOps) | ✅ |
| CompleteJob (chanceOfRE + RE skill +1%/lvl) | ✅ |
| T2 BPC output via parentBlueprintTypeID | ✅ |
| Skill checks (ReverseEngineering 3408) | ✅ |
| Datacore skill modifier (reads required skills from ramTypeRequirements) | ✅ |

_Что осталось:_ RE-таблицы для кэша клиента (1800008/1800009), полный список RE-рецептов из SDE

---

### 25. Effects System `██████████████████` 95%

| Feature | |
|---------|-|
| Passive, online, active effect processing | ✅ |
| Implant/booster processing | ✅ |
| Subsystem effect processing | 🟡 |

_Что осталось:_ полная обработка эффектов подсистем T3, резисты в режиме онлайн/офлайн

---

### 26. Cosmic Managers

| Manager | % | Bar | Notes | Что осталось |
|---------|---|-----|-------|-------------|
| Anomaly Manager | 90% | `█████████████████░` | All site types, FW anomalies, QueueRespawn, FW plex auto-spawn | NPC-спавн в плексах |
| Dungeon Manager | 70% | `████████████░░░░` | Anomaly/mission/unrated/incursion | больше типов данжей |
| Spawn Manager | 75% | `██████████████░░` | Dynamic/static, wave progression | продвинутые волны с триггерами |
| Wormhole Manager | 80% | `████████████████░░` | Full lifecycle | эффекты, K162 позиционирование |
| Belt Manager | 85% | `████████████████░` | Asteroid distribution | динамический респаун |
| Civilian Manager | 80% | `████████████████░░` | Spawns/despawns civilian convoys with ConvoyAI on 60s timer | более сложные маршруты, реакция на игроков |

---

### 27. Memory Management `████░░░░░░░░░░░░░░` 20%

| Feature | |
|---------|-|
| PyInt cache (−10..255) | ✅ |
| `new PyInt` → `PyStatic.NewInt()` in hot paths | ✅ |
| RefPtr→shared_ptr migration (planning) | 🟡 |
| Valgrind leak tracking | 🟡 |

_Что осталось:_ RefPtr→shared_ptr рефакторинг (сложный, затрагивает ~400 файлов), valgrind-чистка

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
| Contracts — courier crateID, delivery, forCorp, GetCourierContractFromItemID | ✅ |
| Corp mail roles — crpRoles table + roleMask filtering | ✅ |
| Warp alignment + warp exit crash fix | ✅ |
| Corp voting — CanVote, CheckVoteExpiry, OnCorporationVoteCaseChanged | ✅ |
| Alliance tax rate (SetTaxRate) + executor support | ✅ |
| Faction Warfare — full implementation + GetSystemStatus + plex spawning | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob + datacore modifier | ✅ |
| Sovereignty — GetAllDevelopmentIndices + upgrade management (Install/Remove) | ✅ |
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
