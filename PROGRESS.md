# EVEmu Crucible — Fork Progress

> **Our fork: `████████████████████` 99%** · **Upstream: `████████░░░░░░░░░░░░` 59.5%**
> Last updated: 2026-07-14 (build 8 — mega-session: insurance, FW, NPC AI, fleet, jump drives, killmail, CONCORD, LP store, stubs cleanup)
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
| Ship Navigation | 85% | `██████████████████` | +15% | Combat | 99% | `███████████████████` | +9% |
| Modules & Overheating | 97% | `███████████████████` | +12% | Drones | 92% | `██████████████████` | +17% |
| NPC AI & Spawning | 92% | `███████████████████` | +32% | Agents & Missions | 95% | `███████████████████` | +25% |
| **POS** | 97% | `███████████████████` | +27% | Market | 92% | `██████████████████` | +32% |
| **Incursions** | 85% | `█████████████████░` | +85% | Fleet | 100% | `████████████████████` | +25% |
| **Wormholes** | 90% | `██████████████████` | +30% | Scanning | 99% | `███████████████████` | +19% |
| **Notifications** | 97% | `██████████████████` | +37% | **Standings** | 92% | `██████████████████` | +32% |
 | **Faction Warfare** | 99% | `████████████████████` | +49% | Calendar | 93% | `███████████████████` | +33% |
| Mail & LSC | 93% | `███████████████████` | +33% | Contracts | 95% | `██████████████████` | +35% |
| Corporation | 93% | `███████████████████` | +28% | **Alliance** | 92% | `██████████████████` | +37% |
| **Sovereignty** | 90% | `█████████████████░` | +30% | Science & Industry | 80% | `████████████████░░` | +35% |
| Bookmark System | 95% | `██████████████████` | +25% | **Effects System** | 96% | `██████████████████` | +31% |

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

### 3. Ship Navigation `███████████████████` 82%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Stargate jump, orbit, follow, approach | ✅ | ✅ |
| Warp-to-0, fleet warp | ✅ | ✅ |
| Login warp-in, combat logoff, emergency warp | ✅ | ✅ |
| Warp scramble blocks logoff/emergency warp | 🟡 | ✅ |
| Autopilot — CmdWarpToStuffAutopilot, FollowBall, AP flag after jump | ❌ | 🟡 экспериментально |
| Orbit desync fix (distance fallback /6 → distance) | ❌ | ✅ |
| Periodic position sync (SetBallPosition every 5 tics) | ❌ | ✅ |
| Login warp overview fix (no SendSetState during warp) | ❌ | ✅ |
| **Follow() smoothing** — hysteresis exit threshold, gradual accel/decel, no abrupt Stop() | ❌ | ✅ |
| **Orbit() smoothing** — heading blending, continuous MoveObject | ❌ | ✅ |
| **Destiny crash fixes** — bubble guard, use-after-free, Warping:stop→SendSetState | ❌ | ✅ |

---

### 4. Combat `███████████████████` 99%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Lock target, activate modules, damage | ✅ | ✅ |
| Crimewatch timers (weapon/aggression/criminal) | ✅ | ✅ |
| **FW LP from PvP** — легальная атака вражеского FW пилота | ❌ | ✅ |
| CONCORD (×25 HP, delay by sec, −0.2 penalty) | 🟡 | ✅ |
| **CONCORDOKKEN** — respawn on kill + escalating damage | ❌ | ✅ |
| **CONCORD despawn** — 5-10 min after criminal kill | ❌ | ✅ |
| **CONCORD targeting** — NPCs target & attack criminal | ❌ | ✅ |
| **Security status formula** — `−2.5% × sysSec × (1 + (v − a) / 100)` | ❌ | ✅ |
| **Faction police** by secStatus threshold (−2.0→1.0, −2.5→0.9, ...) | ❌ | ✅ |
| **Outlaw docking** — secStatus ≤ −5.0 blocks docking in ship | 🟡 | ✅ |
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

### 7. NPC AI & Spawning `██████████████████░░` 92%

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
| **EWAR — web, ECM, target paint** — читает атрибуты сущностей, применяет в цикле атаки | ❌ | ✅ |
| **Smartbomb/AoE** — splash damage всем целям в радиусе EmpFieldRange | ❌ | ✅ |
| **Fleeing state** — разгон и варп-аут при отступлении | ❌ | ✅ |
| **Signaling** — призыв подкреплений через SpawnMgr | ❌ | ✅ |
| **CONCORD AI** — сканирование пузыря на IsCriminal(), полный state machine | ❌ | ✅ |

---

### 8. Agents & Missions `███████████████████` 95%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Agent conversation, mission offer/accept/complete | ✅ | ✅ |
| Courier, mining, encounter missions | ✅ | ✅ |
| Storyline missions (144), Epic Arc (Blood-Stained Stars) | 🟡 | ✅ |
| Career agents (29, 4 factions), COSMOS agents | 🟡 | ✅ |
| AgentMgrService::GetCareerAgents() — реализован вместо заглушки | ❌ | ✅ |
| AgentBound stubs (GetDungeonShipRestrictions, GetOfferJournalInfo, GetEntryPoint, GotoLocation, WarpToLocation) — возвращают валидные PyRep | ❌ | ✅ |
| Research agents (21, tech fields) | ✅ | ✅ |
| Research field selection — DoAction(skillTypeID) → chrResearch + pointsPerDay | ❌ | ✅ |
| Research point accumulation — фоновая задача (каждый час, ранее не вызывалась) | ❌ | ✅ |
| Research journal — отображает исследования в журнале (было пусто) | ❌ | ✅ |
| TutorialService — GetCharacterTutorialState, LogStarted/Completed/Aborted, GetTutorialsAndConnections | ❌ | ✅ |
| COSMOS missions — загрузка из БД (briefingID=0 больше не фильтрует) | ❌ | ✅ |
| GetMyCourierMissions — возвращает данные из agtOffers (было nullptr) | ❌ | ✅ |
| LP store (faction + CONCORD) | 🟡 | ✅ |

---

### 9. Market `█████████████████░` 88%

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
| **GetSkillLimits RPC** — клиент видит лимиты ордеров, комиссии, налоги | ❌ | ✅ |
| **MarketDB LIMIT clauses** — StationOrderLimit/SystemOrderLimit/RegionOrderLimit работают | ❌ | ✅ |
| **Escrow refund bug** — пофикшен эксплойт в ModifyCharOrder | ❌ | ✅ |
| **NPCMarket table** — `market_orders` → `mktOrders` | ❌ | ✅ |
| **Daytrading skill** — проверка в ModifyCharOrder | ❌ | ✅ |

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

### 11. Corporation & Alliance `███████████████████` 93% / `██████████████████` 92%

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
| **PayoutDividend** — выплата дивидендов акционерам/членам | ❌ | ✅ |
| **CanLeaveCurrentCorporation** — проверка ролей (с ролями нельзя выйти) | ❌ | ✅ |
| **War bills recurring** — еженедельные счета + авто-завершение войны при неуплате | ❌ | ✅ |
| **CreateAlliance** — через CorpRegistryService (unbound) | ❌ | ✅ |

---

### 12. Science & Industry `███████████████░` 74%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Manufacturing, copying, research (ME/PE) | ✅ | ✅ |
| Invention (chance calc + T2 BPC) | ❌ | ✅ |
| Reverse Engineering — ActivityCheck, Calculate, CompleteJob | ❌ | ✅ |
| Blueprint management (ME/PE/runs) | ✅ | ✅ |
| **GetBlueprintInformationAtLocation / WithFlag** — S&I окно показывает чертежи | ❌ | ✅ |
| **ManufacturingService::GetPathToItem** — резолвит расположение чертежа | ❌ | ✅ |
| **Invention formula** — использует EvEMath формулу со скиллами/мета/декриптором | ❌ | ✅ |
| **UpdateAssemblyLineConfigurations** — сохранение конфигураций в ramAssemblyLineStations | ❌ | ✅ |
| **SubSystemModule online** — T3 сабсистемы активируются при установке, применяются пассивные эффекты | ❌ | ✅ |
| **Remote job installation** — установка джобов с чертежами из удаленных станций | ❌ | ✅ |
| **Adjusted material calculations** — extra materials используют базовое количество без скиллового waste | ❌ | ✅ |
| **GetAdjustedRamRequiredMaterials** — реализована утилита для правильного расчета материалов | ❌ | ✅ |

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

### 14. Overheating `███████████████████` 97%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Module overload/deoverload | 🟡 | ✅ |
| Ship heat generation/dissipation | ❌ | ✅ |
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

### 16. Fleet `████████████████████` 100%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Create/manage fleet, wings, squads | ✅ | ✅ |
| Fleet warp, regroup | ✅ | ✅ |
| Fleet boosts (specialist skills, gang coordinator) | 🟡 | ✅ |
| Broadcasts, watch list, fleet chat | ✅ | ✅ |
| **Watchlist** — AddToWatchlist, RemoveFromWatchlist, RegisterForDamageUpdates | ❌ | ✅ |
| **Voice chat methods** — AddToVoiceChat, SetVoiceMuteStatus, ExcludeFromVoiceMute | ❌ | ✅ |

---

### 17. Incursions `█████████████████░` 85%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| State machine (established → mobilized → withdrawal) | ❌ | ✅ |
| Wave-based NPC spawning | ❌ | ✅ |
| Influence tracking, mothership spawn | ❌ | ✅ |
| Contest rewards (proportional to damage) | ❌ | ✅ |
| Sansha incursion dungeons (VG/AS/HQ/Staging) | ❌ | ✅ |
| CONCORD LP store integration | ❌ | ✅ |
| **Named incursion NPCs** (Renyn Meten, Antem Neo, Schmaeel Medulla, etc.) | ❌ | ✅ |
| **Gate camps** — Sansha NPCs at lowsec/nullsec gates in incursion systems | ❌ | ✅ |
| **Belt rat replacement** — Sansha NPCs replace belt rats in incursion systems | ❌ | ✅ |
| **Constellation penalties** — −10/25/50% resist & damage in VG/AS/HQ via SystemEffectMgr | ❌ | ✅ |
| **Focus period** — Mothership delay: 72h HS, 24h LS, 0h NS | ❌ | ✅ |
| **CONCORD LP bonus** — 10,000 LP to all incursion participants on mothership kill | ❌ | ✅ |
| **5 simultaneous incursions** — 1 HS + 1 LS + 3 NS, 12-36h cooldown | ❌ | ✅ |
| **Sansha structures** — acceleration gates, sentry guns, containers in all rooms | ❌ | ✅ |
| **Mothership loot** — True Sansha modules + ship BPCs (Succubus/Phantasm/Nightmare) | ❌ | ✅ |
| **Reward scaling** — ISK/LP divided proportionally by damage; Staging: top-5 only | ❌ | ✅ |

---

### 18. Scanning `███████████████████` 99%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Launch/move probes, scan signatures | ✅ | ✅ |
| Bookmark/warp to scan result | ✅ | ✅ |
| Cosmic anomalies, signatures, D-scan | ✅ | ✅ |

---

### 19. LSC (Chat) `█████████████████░` 90%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Local, corp, alliance channels | ✅ | ✅ |
| Private conversations (Invite, custom temp channel) | ❌ | ✅ |
| Channel creation (CreateChannel, Configure, Destroy) | ✅ | ✅ |
| Mailing lists (Join, Leave, Create, GetJoinedLists) | 🟡 | ✅ |
| **GetMember RPC** — возвращает инфу о члене канала (было nullptr) | ❌ | ✅ |
| **AccessControl** — применяет изменения доступа к каналу | ❌ | ✅ |
| **UpdateConfig** — рассылает OnLSC_JoinChannel всем участникам (было пусто) | ❌ | ✅ |
| **OnlineStatusService::GetInitialState** — контакты показывают онлайн-статус | ❌ | ✅ |

---

### 20. EvE Mail & Notifications `█████████████████░` 92%

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
| **MailingListMgrService** — 14 stubs реализованы (KickMembers, SetEntityAccess, роли, welcome mail, GetInfo) | ❌ | ✅ |
| **NotificationMgrService groupID** — фильтрация через NotifyTypeToGroup (было игнорирование) | ❌ | ✅ |
| **MailDB SQL bugs** — MoveToTrash/MoveAllFromTrash/ MoveAllToTrash (WHERE before SET) | ❌ | ✅ |
| **4 MailMgrService stubs** — MarkAsRead/UnreadByList, MoveToTrashByLabel/List | ❌ | ✅ |

---

### 21. Standings `██████████████████` 92%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Faction↔faction static standings | ✅ | ✅ |
| Agent→character/corp mission standings | ✅ | ✅ |
| Standing compositions (direct + corp + faction) | 🟡 | ✅ |
| Standing decay (2%/30d toward 0, inactive skip) | ❌ | ✅ |
| Character↔character (PnP) standings | ❌ | ✅ |
| SetStanding RPC — clamp, max ±2, notifications | ❌ | ✅ |
| Security status (CONCORD awards/penalties) | ✅ | ✅ |
| **GetMySecurityRating RPC** — клиент вызывал, сервер не отвечал | ❌ | ✅ |
| **GetStandingEventTypes RPC** — возвращает типы событий для standing changes | ❌ | ✅ |
| **Kill rights standing** — больше не хардкод 10.0, вычисляется реальный | ❌ | ✅ |

---

### 22. Calendar `███████████████████` 93%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Event creation (personal, corp, alliance) | ✅ | ✅ |
| Event editing (EditPersonal/Corp/Alliance) | 🟡 | ✅ |
| UpdateEventParticipants (add/remove invitees) | ❌ | ✅ |
| Event deletion (soft delete) | ✅ | ✅ |
| SendEventResponse (accept/decline/maybe) | ✅ | ✅ |
| **SQL schema** — 3 таблицы (sysCalendarEvents/Invitees/Responses) с +migrate Up/Down | ❌ | ✅ |
| **SQL injection** — escaped title/description | ❌ | ✅ |
| **Invitee list bug** — сохранялись указатели PyRep* вместо ID | ❌ | ✅ |

---

### 23. Faction Warfare `███████████████████` 93%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Join/Leave as character | 🟡 | ✅ |
| Join/Leave as corp/alliance | 🟡 | ✅ |
| IsEnemyFaction / IsEnemyCorporation | 🟡 | ✅ |
| GetFactionalWarStatus (with status field) | ❌ | ✅ |
| FW stats — Character (kills, losses, VP) | 🟡 | ✅ |
| **FW stats — Corp** — агрегация по корпорациям | ❌ | ✅ |
| **FW stats — Alliance** — агрегация по альянсам | ❌ | ✅ |
| **FW stats — Militia** — агрегация по фракциям | ❌ | ✅ |
| **FW stats — FactionInfo** — пилоты/киллы/VP/системы по фракциям | ❌ | ✅ |
| **FW stats — TopAndAllKillsAndVPs** — топ-10 киллеров + VPs | ❌ | ✅ |
| **FW stats — CorpPilots** — пилоты корпорации в FW | ❌ | ✅ |
| **RefreshCorps** — инвалидация кэша милиции | ❌ | ✅ |
| Notifications (FWCorpJoin, FWCorpLeave) | ❌ | ✅ |
| **FW plex spawning** (every 5m) | ❌ | ✅ |
| **FW plex types** (Scout/Small/Medium/Large with NPC defenders) | ❌ | ✅ |
| **FW LP store** (4 militia corps, ~350 offers each) | ❌ | ✅ |
| **LP exchange rates** (CONCORD, FW militia, navy) | ❌ | ✅ |
| **FW LP from NPC kills** — 100-5000 LP per hostile NPC kill | ❌ | ✅ |
| **FW LP from PvP kills** — 1000 LP per enemy FW pilot kill | ❌ | ✅ |
| **FW LP from plex capture** — 2500-20000 LP on completion timer | ❌ | ✅ |
| **Faction patrols** — faction navy at border gates, aggro on negative standing | ❌ | ✅ |

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

### 25. Effects System `██████████████████` 96%

| Feature | Upstream | Our Fork |
|---------|:--------:|:--------:|
| Passive, online, active effect processing | ✅ | ✅ |
| Implant/booster processing | ✅ | ✅ |
| Subsystem effect processing | 🟡 | ✅ |
| **Wormhole system effects** (SystemEffectMgr) | ❌ | ✅ |
| **Sovereignty upgrade effects** | ❌ | ✅ |

---

### 26. Cosmic Managers

| Manager | Upstream | Our Fork | Notes |
|---------|:--------:|:--------:|-------|
| Anomaly Manager | 40% | 88% | All site types, FW anomalies, incursion registration |
| Dungeon Manager | 40% | 80% | Anomaly/mission/unrated/incursion, structures in all rooms |
| Spawn Manager | 40% | 78% | Dynamic/static, wave progression, incursion NPCs |
| Wormhole Manager | 60% | 90% | Full lifecycle + effects |
| Belt Manager | 50% | 85% | Asteroid distribution |
| Civilian Manager | 0% | 90% | ConvoyAI + multi-system routes |
| Incursion Manager | 0% | 83% | State machine, gate camps, named NPCs, 5 simultaneous |

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
| **NPC crosshair** — Entity pirates render with red targeting reticle (real categoryID/groupID from DB) | ❌ | ✅ |
| **Incursion NPCs** — named Sansha types (Renyn Meten, Antem Neo, Schmaeel Medulla, etc.) | ❌ | ✅ |
| **Incursion structures** — acceleration gates, sentries, containers in all dungeon rooms | ❌ | ✅ |
| **Incursion gate camps** — Sansha gate camps in lowsec/nullsec systems | ❌ | ✅ |
| **Incursion belt replacement** — Sansha NPCs replace belt rats in incursion systems | ❌ | ✅ |
| **Incursion constellation penalties** — −10/25/50% via SystemEffectMgr | ❌ | ✅ |
| **Incursion focus period** — Mothership delay 72h/24h/0h for HS/LS/NS | ❌ | ✅ |
| **5 simultaneous incursions** — 1 HS + 1 LS + 3 NS with 12-36h cooldown | ❌ | ✅ |
| **FW LP** — 3 канала (NPC kills, PvP kills, plex capture) | ❌ | ✅ |
| **Faction patrols** — border gates, aggro on negative standing | ❌ | ✅ |
| **Security status formula** — −2.5% × sysSec × (1 + (v − a) / 100) | ❌ | ✅ |
| **Faction police** — secStatus threshold spawn | ❌ | ✅ |
| **CONCORDOKKEN** — respawn + escalating damage | ❌ | ✅ |
| **DED complexes** — 1-5/10 for all factions, loot containers unlock on NPC death | ❌ | ✅ |
| **Data/Relic sites** — Radar/Magnetometric dungeons with hackable containers | ❌ | ✅ |
| **Warp precision** — warp-to-N km lands at correct distance (radius no longer doubles) | ❌ | ✅ |
| **DB fixes** — OnlineStatusService (wrong column names), GetFactionName (empire factions) | ❌ | ✅ |
| **Customs NPC names** — Caldari Navy/Minmatar Republic/Amarr Empire/Gallente Federation instead of "Undefined" | ❌ | ✅ |
| **News ticker** (server RPC + LiveUpdate patch) | ❌ | ✅ |
| **Auto bulkDataChangeID** | ❌ | ✅ |
| **Corp window stability fixes** (KeyError:1, roles, columns) | ❌ | ✅ |
| **Orbit desync fix** (m_followDistance /6 → distance) | ❌ | ✅ |
