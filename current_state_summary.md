# EVEmu Crucible — Current State Summary

## Part 1: Anomaly NPCs — Fixed (done)
- **Custom typeIDs (33001-33103) → SDE Entity typeIDs**: Switched from custom Ship-category types to real SDE Entity-category pirate NPC typeIDs (since client never caches custom types)
- **NPC::MakeSlimItem**: Override categoryID→6 (Ship) and map Entity groupIDs→Ship groups (25/26/27/419) so client renders models correctly
- **NPC::MakeSlimItem**: Added `charID=None`, `bounty=0`, `securityStatus=0.0`, `modules=[]` for Ship-category client expectations
- **DoSpawnForAnomaly**: Disabled warp-in for Entity-category NPCs, fixed warp speed range
- **DoSpawnForAnomaly**: GroupID-based faction fallback for `ownerID` when race-lookup fails
- **DungeonMgr**: Added `catID == Entity(11)` check for NPC spawns (alongside Ship/Drone)
- **Client cache**: `bulkDataChangeID` unchanged (reverted after accidental increment); `ObjCacheService` always regenerates `config.BulkData.types` from DB
- **published=1** migration for SDE types to ensure client loads them
- **Migrations consolidated**: Final SDE typeIDs used in `dunRoomObjects` (Angel/Blood/Guristas/Sansha/Serpentis/Rogue Drone)
- **TYPEID FORCE hack**: Removed from DungeonMgr.cpp
- **Debug logging**: Removed from DungeonMgr.cpp, reduced in SpawnMgr.cpp

## Part 2: Bills (done)
- `CharPayBill(billID)` — balance check, TransferFunds, mark paid
- `CharGetBills()` / `CharGetBillsReceivable()` — DB queries
- `PayCorporationBill(billID, fromAccountKey)` — corp wallet, TransferFunds
- `SendAutomaticPaySettings(dict)` — 6 bill types, REPLACE INTO crpAutoPay
- `CorporationDB::SetAutoPay()` — implemented
- `CorporationDB::GetCorporationBills()` — fixed SQL bug

## Part 3: Alliance Wars (done)
- `DeclareWarAgainst`, `RetractWar`, `ChangeMutualWarFlag`
- `GetCostOfWarAgainst` — configurable via `eve-server.xml <rates><warCost>50000000</warCost></rates>`
- `GetWars` — returns all wars (active + historical)
- War bills auto-created (25% of declaration cost/week)
- War decay via unpaid bills system

## Part 4: War-aware CONCORD/Sentry (done)
- `CrimeWatch::OnAggression()` — war check before CONCORD/criminal
- `warRegistry WHERE retracted=0 AND timeFinished=0` = active war
- No criminal flag, no CONCORD, no sec loss for war targets

## Part 5: AllianceDB (done)
- `DeleteLabel`, `EditLabel` — implemented

## Part 6: Corporation Mail (done)
- `MailDB::SendMail` — corp/alliance delivery via `toCorpOrAllianceID`
- Online-first delivery, offline fallback
- corp=4 / alliance=8 label masks

## Part 7: Clones / Jump Clones (done)
- **Per-clone implants**: Created `chrJumpCloneImplants` table, `entity.isActive` flag
- `GetCloneState` / `GetStationCloneState` — reads per-clone implants from `chrJumpCloneImplants`
- `GetPriceForClone` — real prices: Alpha=free, Beta=100k, Gamma=500k, Delta=5M, Epsilon=50M, Zeta=100M
- `InstallCloneInStation` — creates jump clone (max 5), checks duplicates, isActive=0
- `DestroyInstalledClone` — deletes clone + its implant entries
- `CloneJump` — uses `SetCloneActive()` to switch active clone (no longer moves ALL clones)
- `SetCloneActive` — deactivates all, activates target
- `GetShipCloneState` — returns empty list (ship clone bay not implemented yet)

## Part 8: Modules — Bugfixes (done)
- `RepairModule()` — fixed inverted nullptr check (was crashing)
- `ModuleRepair()` — now checks actual module damage, allows repair in space
- `StopModuleRepair()` — logging
- `CharacterLeavingShip()` — calls OfflineAll()
- `LaunchSnowBall()` — still stub (low priority)

## Part 9: Corporation Market — Enabled (done)
- **Removed block**: "Corporation Market transactions are not available" → now allowed
- **Office check**: Corp must have office in target station
- **NPC corp check**: Can't trade for NPC corps
- **Wallet permissions**: `AccountCanTake*` role check for corp wallet divisions
- `CancelCharOrder` for corp orders: escrow refunded to corp wallet, items returned to `flagCorpMarket`

## Part 10: Alliance Wars Config (done)
- `rates.warCost` in `eve-server.xml` (default 50M ISK)
- `GetCostOfWarAgainst` / `DeclareWarAgainst` read from config

## Part 11: Contracts — Stubs Implemented (done)
- `GetMyExpiredContractList` — returns expired contracts from DB
- `NumOutstandingContracts` — counts per character/corp
- `GetLoginInfo` — queries `needsAttention`, `inProgress`, `assignedToMe` from `ctrContracts`

## Part 12: Incursions — Base System (done)
- **SQL tables**: `incursions`, `incursionSystems`, `incursionRewards`
- **IncursionService** (registered as "incursion"): `GetDelayedRewardsByGroupIDs` returns rewards from DB
- **IncursionMgr** (singleton): 60s timer for state machine + influence updates
- **State machine**: established (5d) → mobilized (2d) → withdrawal (24h) → auto-end
- **Influence**: site completion reduces (VG=1%, AS=2%, HQ=4%), natural regen +1%/20min
- **Mothership**: spawns when all systems at 0% influence (hasBoss=1)
- **MapService**: `GetSystemsInIncursions`, `GetSystemsInIncursionsGM`, `GetIncursionGlobalReport` — all query real DB
- **HoloscreenMgrService**: `incursionReport` returns active incursions from DB
- **Incursion dungeons**: Vanguard (2100-2103), Assault (2110-2112), Headquarters (2120-2122) in migration
- **DoSpawnForIncursion**: spawns Sansha NPCs (10025/10030/11913/23383)
- **SpawnKilled incursion**: rewards ISK, updates influence via `OnSiteCompleted`
- **Rewards seeded**: VG=10.4M+1400LP, AS=18.2M+3500LP, HQ=31.5M+7000LP, Mothership=63M+14000LP

## Part 13: Crash Fixes (done)
- **FlushPendingDestinyUpdates** — null-check in loop to prevent nullptr crash in `DoDestinyUpdate`
- **NPC::EncodeDestiny** — works for SDE Entity types with categoryID=6 override
- **Entity-category NPC warp-in** disabled (was causing `Unknown packet type` with negative warp speed)

## Part 14: Implant Management (done)
- **CharAddImplant** — moves implant from hangar to character's `flagImplant` (89), re-processes effects
- **RemoveImplantFromCharacter** — moves implant back to station hangar, re-processes effects
- **GetCharacterAttributeModifiers** — returns real implant itemID/typeID/operation/value tuples from equipped implants
- **Implant slot conflict** — client-side error messages `OnlyOneImplantActiveBody`/`OnlyOneBoosterActiveBody` handled by client

## Part 15: Clone Death (done)
- **Implant destruction** — all equipped implants deleted on pod death in `ResetAfterPodded`
- **SP check** — compares trained SP vs clone grade max SP (Alpha=5M, Beta=5M, Gamma=50M, Delta=200M, Epsilon=500M, Zeta=unlimited)
- **SP overflow detection** — logs warning when SP exceeds clone grade capacity

## Part 16: Ship Clone Bay Stubs (done)
- **GetShipCloneState** — returns empty list (ship clone bay not implemented)
- **OfferShipCloneInstallation** — sends `OnShipJumpCloneInstallationOffered` notification to target character
- **AcceptShipCloneInstallation** — stub, returns false (not implemented)
- **CancelShipCloneInstallation** — sends `OnShipJumpCloneInstallationCanceled` notification

## Part 17: Incursion Wave Spawning / Mothership / SceneTypes (done)
- **SpawnSites()** — больше не пустышка: спавнит данжи вторжений (2100-2122) в системах где есть игроки
- **SceneType-зависимый спавн** — VG (фрегаты), AS (крейсера+линкоры), HQ (тяжелые линкоры)
- **Волновой спавн** — `SpawnKilled` для инкурсий теперь переключает волны через `MakeSpawn()`
- **Mothership** — `SpawnMothership()` спавнит эскорт + босс при `hasBoss=1`
- **Фикс hardcoded sceneType** — `SpawnKilled` читает реальный `sceneType` из `incursionSystems`
- **Staging defence** — сайты появляются и в staging-системе

## Part 18: Incursion Penalties / NPC Damage / LP Store (done)
- **NPC damage boost** — VG 1.2x, AS 1.4x, HQ 1.6x, Mothership 2.0x (resistances penalty simulation)
- **Remote repair penalty** — −70% эффективности ремоут-репа в инкурсионных системах
- **Cyno jamming** — цино не активируется в системах вторжения
- **Bounty penalty** — −25% к баунти выплатам
- **CONCORD LP Store** — миграция с LP-офферами для CONCORD (импланты, скины)

## Part 19: SDE vs Live API Verification (done)
- **Sansha NPC graphicIDs** verified against live ESI: 1236, 1238, 2295 all present in both `eveGraphics` and `graphics` tables ✅
- **Entity NPC types** (10025, 10030, 11913, 23383) have correct groupIDs (565-567, 582), all catID=11 (Entity) ✅
- **Entity effects** verified: 2192-2197 (shield/armor rep), 1871-1879 (EWAR), all present in `dgmEffects` ✅
- **All pirate NPCs** (Angel/Blood/Guristas/Serpentis/RogueDrone) verified: graphicIDs, effects, groups all match live API ✅
- **soundID 31** (pirate chatter) exists in `sounds` table ✅
- **Old custom type flaw**: 33083 Sansha Baron used graphicID 2289 (Dual80mmAutoCannon turret, not a ship model) — fixed by migration to SDE typeIDs
- **soundID 20073** used by custom types — does NOT exist in `sounds` table — fixed by migration to SDE typeIDs (now using soundID=31)

## Part 20: Entity NPC Client Rendering Fix (done)
- **Problem**: Entity-category NPCs (catID=11) showed targeting brackets but no crosshair reticle because client cached them as Entity, not Ship
- **Fix**: Added `Ball::Flag::IsInteractive` (0x08) to `NPC::EncodeDestiny()` flags — without this flag the client treats the ball as non-interactive and skips crosshair rendering
- **Also**: `Generate_invTypes()` overrides groupIDs for Entity NPC groups (550-584, 755-761) to Ship groups (25/26/27/419/420/28) for correct overview filtering
- **Groups mapped**: Frigate→25, Cruiser→26, Battleship→27, BattleCruiser→419, Destroyer→420, Industrial→28 (same mapping as `NPC::MakeSlimItem`)
- **Result**: NPCs now render with targeting crosshairs + proper selection effects ✅

## Part 21: Live API Verification Session (done)
- **CONCORD LP store**: Fixed corpID 1000125 (was 1000131), seeded with real live offers (implants + CONCORD BPCs) ✅
- **Incursion rewards**: Values (10.4M/18.2M/31.5M/63M) reasonable for Crucible era, no change needed ✅
- **NPC stats**: All Sansha NPC attributes verified against live ESI, match ✅
- **damageControl effect (2302)**: Found bug! effect_category was 1 (passive) instead of 4 (active) — fixed ✅
- **All pirate NPC effects**: 2192-2197 (shield/armor rep), 1871-1879 (EWAR) verified, match live ✅

## Part 22: Feature Implementation — Snowballs, Clones, War Decay (done)
- **LaunchSnowBall**: Implemented — snowball missile entity creation and launch (follows LaunchMissile pattern) ✅
- **Per-clone implants**: Added `AdjustCloneImplant` service method + `StationDB::AddCloneImplant`/`RemoveCloneImplant` for per-clone implant management ✅
- **War decay timer**: `CheckWarDecay()` called every minute — auto-ends wars where the weekly bill is overdue and unpaid ✅

## Part 23: Ship Clone Bay, Auction Contracts, Corp Mail Roles (done)
- **Jump clone ship bay**: `AcceptShipCloneInstallation` — creates a jump clone in the offering ship's clone bay (entity flagClone=30) ✅
- **Ship clone state**: `GetShipCloneState` — returns clones stored in current ship via DB query ✅
- **Contract auctions**: `PlaceBid` + `FinishAuction` — `ctrBids` table, escrow check, bid tracking, winner notification ✅
- **Corp mail role groups**: `roleMask` param in `SendMail` — filters corp members by role when sending mail to corp/alliance ✅
- **NPC crosshair fix**: Added `Ball::Flag::IsInteractive` (0x08) to `NPC::EncodeDestiny` + bumped `bulkDataChangeID` ✅

## Part 24: T3 Ships — Subsystem Implementation (done)
- **MakeSlimItem**: Uncommented `GetItemsByFlagRange(flagSubSystem0, flagSubSystem7)` — client now receives fitted subsystems in slim item ✅
- **Subsystem destruction**: Added `IsSubSystem()` check alongside `IsRigSlot()` — subsystems no longer survive ship destruction (0% drop like rigs) ✅
- **SP loss on T3 pod kill**: When a Strategic Cruiser is destroyed, 5% SP is removed from the pilot's Strategic Cruiser skill (minimum 500 SP retained) ✅
- **Existing infrastructure verified**: SubSystemModule class, InstallSubSystem, ModuleFactory dispatch, all 5 subsystem groups (954-958), flags (125-132), T3 hulls (29984/29986/29988/29990), all SDE data present ✅

## Part 25: Mega-Session (done)
- **Server cache fix**: `alwaysRegen` before `HaveCached()` — server served stale types ✅
- **NPC Ship-category typeIDs**: 33500-33523 for all 6 pirate factions ✅
- **Wreck mappings**: `invTypesToWrecks` for 33500+ types ✅
- **`/killallnpcs` fix**: `m_killed=true` + `RemoveNPC()` ✅
- **IsInteractive flag removed**: from `NPC::EncodeDestiny` — fix `Unknown packet type` ✅
- **Incursion waves**: `DoSpawnForAnomaly` accepts `isIncursion` flag ✅
- **Incursion contest**: damage tracking per player per bubble, proportional rewards ✅
- **Ship Fittings Manager**: full CRUD ✅
- **CorpFittingMgr**: full CRUD ✅
- **Civilian Manager**: basic NPC traffic using ConvoyAI ✅
- **GetStandingCompositions**: direct + corp + faction standings ✅
- **Invention**: chance calculation + T2 BPC creation ✅
- **Mailing lists**: Join/Leave/Delete/Kick/EntityAccess + PK fix ✅
- **LaunchSnowBall, Per-clone implants, War decay, Ship clone bay** ✅
- **Contract auctions + ISK transfer** ✅
- **CONCORD LP store fix, damageControl fix** ✅
- **Contraband: 90% base + freight skills** ✅
- **PyInt cache (-10..255) + bulk replace in 33 files** ✅
- **Agent portrait fix (images.evetech.net)** ✅
- **Login warp: removed UpdateChargeQty hack** ✅
- **GM commands: dogma, tr, status, siglist** ✅

## Part 26: POS Fuel & Reinforced (done)
- **Fuel consumption**: tower burns fuel blocks from cargo every ~60s based on `invControlTowerResources` or size-based defaults (10/20/40 per hour) ✅
- **Reinforced mode**: fuel runs out → force field drops, stront consumed, reinforced timer set (1-48h), timer expiry → auto-return to Online ✅
- **CPU/PG tracking**: `RecalcResources()`, `OnlineModule/OfflineModule` update PG/CPU load from `AttrPower`/`AttrCpu` ✅
- **Assume/Relinquish control**: `m_controllerID` on StructureSE, slim item includes `controllerID`, notifications sent to both old and new controller ✅
- **Orbitals**: `AnchorOrbital`/`UnanchorOrbital`/`OnlineOrbital`/`CompleteOrbitalStateChange`/`GMUpgradeOrbital` — all methods target `CustomsSE` or `StructureSE` ✅
- **Reactor linking**: `LinkResourceForTower` maps connections to `ReactorData`, `RunMoonProcessCycleforTower` toggles process cycle on all reactors/moon miners ✅
- **POS Weapon AI**: `POS_AI` class — bubble scan, corp validation, falloff-based to-hit, `Damage` application with effects ✅
- **Fuel notifications**: percentage tracking with thresholds (50/25/10/5%), calendar event stub ✅
- **Skill checks**: `CheckStructureSkills()` validates `AttrRequiredSkill1-6` before anchor/online operations ✅

## Part 27: Overheating & Nanite Repair (done)
- **Thermodynamics skill check**: `DogmaIMBound::Overload()` requires Thermodynamics level 1 ✅
- **HeatDamageCheck**: slot-based damage spread — self 100%, adjacent 25%, distance 2: 10%, further 5%; scaled by rack heat and reduced by Thermodynamics skill (5%/level) ✅
- **Nanite Paste repair**: `ModuleManager::ModuleRepair()` consumes typeID 24694 from cargo, heals 10% + 5%/level of Nanite Engineering per paste ✅

## Part 28: Warp & Client Crash Fixes (done)
- **Warp alignment**: `SetSpeedFraction` no longer converts WARP→GOTO during pre-warp acceleration — ship actually enters warp ✅
- **Warp exit crash**: `SetPosition(false)` in `WarpUpdate` — no `SetBallPosition` sent while client WarpLoop is active (fixes `ValueError: Unknown packet type`) ✅
- **Jump clone implants**: returned as `PyList` of `KeyVal(jumpCloneID, typeID)` instead of `dict` — client `.Filter()` works ✅

## Part 29: Standings Decay + PnP + Market History + Trade Skills (done)
- **Standings decay**: `StandingMgr::ProcessDecay()` runs every hour from main loop, `new = old * (1-0.0000278)^hours` (~2%/30d toward 0), skips inactive players (>60d), logs to `repStandingChanges` ✅
- **PnP standing**: `Standing::SetStanding()` RPC — clamp [-10,10], max ±2 per tx, UPSERT + `lastModified`, `OnStandingSet` + `OnStandingsModified` notifications ✅
- **SQL migration**: `lastModified` column on `repStandings` ✅
- **Market history**: `UpdatePriceHistory` `GROUP BY` uncommented, `Process()` wired to call it on `NeedsUpdate()`, seed from `CruciblePriceHistory` on empty table ✅
- **Trade skills**: `PlaceCharOrder` — order count (`Trade*4+Retail*8+Wholesale*16+Tycoon*32`), range validation (`Marketing`/`Procurement`), `MarginTrading` escrow reduction (`escrow = total * 0.75^level`) ✅
- **ModifyCharOrder**: fixed for corp orders — uses `oInfo.isCorp` + `accountKey` instead of hardcoded `charID`/`Cash` ✅

## Part 30: Notifications — DB + Live Push (done)
- **CreateNotification helper**: `EntityList::CreateNotification()` — inserts into `notification` + `notificationText` tables, sends `OnNotificationReceived` live push ✅
- **Sources connected**: `CharPayBill` → `BillPaidChar`, `CheckFuel` → `TowerAlert`, `OfferMission` → `ResearchMissionAvailable`, `CorpRegistryBound` → `CorpAppNew`, `Structure::Killed` → `CorpStructLost` ✅
- **NotificationText**: marshal PyDict via `Marshal()` for DB storage ✅

## Part 31: Calendar + Contracts + Market Bot (done)
- **Calendar**: `EditPersonalEvent`/`EditCorporationEvent`/`EditAllianceEvent` — UPDATE + `OnReloadCalendar`, `UpdateEventParticipants` — add/remove invitees, `SendEventResponse` — now sends reload notification ✅
- **Calendar DB**: `UpdateEventParticipants()` + `UpdateEvent()` methods on `CalendarDB` ✅
- **Contracts**: `FinishAuction` — item transfer to winner (`ChangeOwner` + `Move`), refund non-winning bidders, notify issuer, return items on no-bids ✅
- **Contracts**: `PlaceBid` — refund previous highest bidder, `FillBidData` column fix (`bidDateTime` → `timeBid`) ✅
- **Market bot**: `buy.OrdersPerSystem`/`sell.OrdersPerSystem`, `buy.DupeOrdersPerSystem`/`sell.DupeOrdersPerSystem`, `buy.MinBuyAmount`/`sell.MinSellAmount` from config ✅

## Part 32: Corp Mail Roles + More Notifications (done)
- **crpRoles table**: SQL migration creates `crpRoles(characterID, roleID)` for role-based mail filtering ✅
- **MailDB roleMask**: `SendMail` now queries `crpRoles` when `roleMask > 0` to filter recipients by role ✅
- **Structure kill notification**: `CorpStructLost` via `CreateNotification` on `StructureSE::Killed` ✅

## Part 33: POS Full Overhaul + Overheating + Warp Fixes (done)
- **POS fuel**: tower burns fuel from cargo, reinforced at empty, stront timer, auto-return to Online ✅
- **POS CPU/PG**: `RecalcResources()`, `OnlineModule`/`OfflineModule` track real usage ✅
- **POS orbitals**: `AnchorOrbital`/`UnanchorOrbital`/`OnlineOrbital`/`CompleteOrbitalStateChange`/`GMUpgradeOrbital` ✅
- **POS reactors**: `LinkResourceForTower` + `RunMoonProcessCycleforTower` ✅
- **POS weapon AI**: `POS_AI` class — bubble scan, corp validation, falloff to-hit ✅
- **POS assume/relinquish control**: `m_controllerID`, slim item, notifications ✅
- **POS skill checks**: `CheckStructureSkills()` validates `RequiredSkill1-6` ✅
- **Overheating**: Thermodynamics skill check, `HeatDamageCheck` slot-based spread, Nanite Paste consumption ✅
- **Warp fixes**: alignment no longer cancels warp, `SetPosition(false)` during WarpUpdate prevents client crash ✅

## Part 34: Faction Warfare + Sovereignty Upgrades (done)
- **FW membership**: `facWarCharacters` + `facWarStats` tables, `JoinFactionAsCharacter` with validation + notification ✅
- **FW enemy checks**: `IsEnemyFaction` via `StandingDB`, `IsEnemyCorporation` via `warFactionID` → standing ✅
- **FW status**: `GetFactionalWarStatus` (KeyVal), `GetCharacterRankInfo` (Rowset), `GetCorporationWarFactionID` ✅
- **FW corp/alliance**: `Join/Leave/Withdraw` for corp (Director role) and alliance, cascading `warFactionID` update ✅
- **FW queries**: `GetFactionCorporations`, `GetSystemsConqueredThisRun` (contested), `GetStats_Character` ✅
- **FW notifications**: `FWCorpJoin`, `FWCorpLeave` via `CreateNotification` ✅
- **SOV upgrades**: `sovUpgrades` table + `GetUpgradeData/System`, `Add/RemoveSystemUpgrade` ✅
- **crpRoles seeding**: startup seed via `EntityList::Initialize`, auto-update via `CharacterDB::SetCorpRole` ✅

## Part 35: Market Bot Config + Expired Auctions + Mailing Lists + Calendar Fix (done)
- **Market bot config**: `VALID_GROUPS` via `<groups>` XML section, `PriceMultiplierMin/Max` for buy/sell, `QuantityMin/Max` configurable, `QuantityLarge/SmallMin/Max` for main ✅
- **Expired auctions**: `CheckExpiredAuctions()` runs every minute — auto-finishes with winner (ISK+items) or returns items on no-bids ✅
- **Mailing Lists**: `Join`/`Leave`/`Delete` stubs now call existing MailDB methods; `Join` returns `KeyVal` with `id` ✅
- **Calendar fix**: all `PyWString*` params changed to `PyRep*` — client sends `PyString` not `PyWString` ✅
- **Migration annotations**: All new migrations now have proper `-- +migrate Up/Down` annotations ✅

## Part 36: Reverse Engineering + OverloadRack + LSC Private Conversations (done)
- **Reverse Engineering**: `ActivityCheck` allows RE at POS labs, `Calculate` uses `researchTechTime` + `AdvancedLaboratoryOperation`, `CompleteJob` uses `chanceOfRE` + `ReverseEngineering` skill (+1%/lvl), produces T2 BPC ✅
- **OverloadRack/StopOverloadRack**: finds rack from module flag, overloads/deoverloads all modules via `GetModulesInBank` ✅
- **LSC Private Conversations**: `Invite` creates temp channel with negative deterministic ID (`-(min<<32|max)`), adds inviter, sends `OnLSC` notification to invitee ✅

## Part 37: Datacore Skills for RE + Alliance Tax + LP Store Fixes (done)
- **Datacore skill modifier**: `FactoryDB::GetRequiredItems` returns datacore typeIDs, `CompleteJob` multiplies success chance by `1 + 0.1 * datacoreSkillLevel`, RE formula now uses both `ReverseEngineering` (+1%/lvl) and datacore skills ✅
- **Alliance tax**: `SetTaxRate` implemented, `UpdateAlliance` SQL bug fixed (wrong column reference) ✅
- **LP store**: `GetSystemStatus` returns system state, `TakeOffer` correctly checks required items before completing offer ✅

## Part 38: Corp Voting + SOV Upgrades (done)
- **Corp voting**: `CanVote` checks corp membership, `CorpVote` broadcasts to all members, `GetVotes` returns active votes ✅
- **Vote expiry**: `CheckVoteExpiry` runs every 60s — auto-processes expired corp votes ✅
- **SOV upgrades**: `GetAllDevelopmentIndices` returns all systems with dev indices, `AddSystemUpgrade`/`RemoveSystemUpgrade` with `SovDataMgr` helper ✅
- **SovereigntyMgrService**: full CRUD for SOV upgrades via `SovereigntyDataMgr` ✅

## Part 39: Courier Contracts + ForCorp (done)
- **Courier contracts**: `GetCourierContractFromItemID` finds courier contract by item ID, `CompleteContract` handles courier delivery ✅
- **ForCorp contracts**: `AcceptContract` and `CompleteContract` accept `forCorp` param, use `Account::KeyType::Cash` for corp wallet operations ✅
- **SearchContracts**: type 10 now includes courier contracts in results ✅

## Part 40: FW Plex Spawning (done)
- **FW anomalies**: `SpawnFWAnomalies()` runs every 5 minutes — spawns FW plex sites in loaded FW systems via `DoSpawnForAnomaly` ✅
- **Faction validation**: FW anomalies assigned to correct faction based on system's `warFactionID` ✅

## Part 41: Dungeon Structures + NPC Crosshairs (done)
- **Dungeon structures**: acceleration gates, sentry guns, containers now spawn in anomaly/incursion rooms alongside NPCs ✅
- **NPC crosshairs in dungeons**: added missing groupIDs to `NPC::EncodeDestiny` crosshair check, DSE guard fix, bubble creation on demand ✅
- **Dungeon AddObject**: accepts `PyNone` for rotation params (client sends `None` when not set) ✅
- **Dungeon search**: fixed `dungeonNameID IS NULL` — client was skipping entries because nameID was not populated ✅
- **Structure slim items**: `ItemSystemEntity::MakeSlimItem()` now includes `categoryID`/`groupID` — structures visible in overview ✅

## Part 42: Stability & Bug Fixes (done)
- **cspa TINYINT range**: reduced from 2950 to 250, then to 100 (DB `TINYINT` signed max 127) ✅
- **factionID mismatches**: Guristas 500014→500010, Rogue Drones 500020→500022, Serpentis 500013→500020 in `AnomalyMgr` ✅
- **Login warp crash**: removed `SendSetState` in `SetLoginWarpComplete` — conflicts with client `WarpLoop`, prevents jerking ✅
- **Client ZeroDivisionError**: fixed `shipui UpdateGauges` crash when ship lacks `capacitorCapacity` ✅
- **PyVisitor stack overflow**: added depth limit to `PyVisitor`/`MarshalStream` to prevent stack overflow on deeply nested packets ✅
- **Bubble desync**: `SetPosition` before `AddNPC` in bubble creation + safety net in `BubbleCast` ✅
- **Compilation fixes**: `uint16` walletKey, renamed shadowed `forCorp`, added missing `Station.h`/`Inventory.h` includes ✅
- **LSC Invite fixes**: `convID` cast to `int32`, `GetCharName().c_str()` for string params, removed dead code in `Configure` ✅
- **FactionWar fix**: `util_Rowset::line` → `lines` (member is `PyList*`) ✅
- **DSE guard fix**: restored missing `switch (gID)` block that was accidentally removed ✅
- **NPC crosshair groupIDs**: added 550-584, 755-761 group ranges for Entity-category NPCs in dungeons ✅

## Part 43: Alliance Member Info + Role Groups Fix (done)
- **Alliance member info**: `GetMembers()` now returns `ceoID, memberCount, tickerName, taxRate, stationID, allianceMemberStartDate` ✅
- **Role groups fix**: added computed `columns` to `GetCorpRoleGroups()` ✅

## Part 44: VDS Deployment + Desync Fixes (done)
- **Prod deployment**: `prod/docker-compose.yml`, `prod/deploy.sh`, `prod/.env.example` ✅
- **Orbit desync fix**: `m_followDistance` fallback divided by 6 instead of using `distance` (ship pulled into gate center) ✅
- **Periodic position sync**: `m_moveSyncCounter` in `MoveObject()` — sends `SetBallPosition` every 5 tics to prevent drift ✅

## Part 45: Civilian Manager — Multi-System Routes + Faction Variety (done)
- **Cross-system routes**: stargate-to-stargate via `mapJumps`, `TransferCrossSystem`/`ResumeCrossSystem` in `CivilianMgr` ✅
- **Phase 4 GateJump**: `ConvoyAI` transfers NPCs between systems when reaching stargate ✅
- **Faction-aware ships**: Caldari/Minmatar/Amarr/Gallente industrial types per region ✅
- **StargateLinks**: helper queries connected systems via `mapJumps` ✅

## Part 46: Wormhole System Effects — Pulsar/Magnetar/BlackHole etc (done)
- **SystemEffectMgr**: applies attribute modifiers on system enter, removes on leave ✅
- **6 effect types**: Pulsar, Magnetar, Cataclysmic, Black Hole, Red Giant, Wolf-Rayet ✅
- **WH class mapping**: C4-C6 systems get effects based on systemID hash ✅
- **Client hook**: `OnEnterSystem`/`OnLeaveSystem` in `Client::MoveToLocation()` ✅

## Part 47: Alliance Executor Voting (done)
- **SQL tables**: `alnVoteItems`, `alnVoteOptions`, `alnVotes` (mirroring corp voting) ✅
- **AllianceDB**: `AddVoteCase`, `GetVoteItems`, `GetVoteOptions`, `GetVotes`, `CastVote`, `ResolveVote`, `ProcessVoteExpiry` ✅
- **AllianceBound**: 7 vote methods registered — `CanViewVotes`, `CanVote`, `InsertVoteCase`, `GetVoteCasesByCorporation`, `GetVoteCaseOptions`, `GetVotes`, `InsertVote` ✅
- **Auto-resolve**: executor vote winner → `alnAlliance.executorCorpID` updated ✅

## Part 48: FW LP Store + Exchange Rates (done)
- **LP exchange rates**: `GetLPExchangeRates` returns static rates for CONCORD, FW militia, navy corps ✅
- **FW militia LP stores**: ~350 offers each (1000179-1000182) already in base `lpStore.sql.gz` ✅

## Part 49: POS Reactions Full Cycle (done)
- **Reaction cycle engine**: `ReactorSE::ProcessReactionCycle()` with configurable timer ✅
- **invTypeReactions**: lookup reaction formula from SDE data, consume inputs, produce outputs ✅
- **DB persistence**: `posReactorData` table, `GetReactorData`/`SaveReactorData`/`UpdateReactorData` ✅
- **GetProcessInfo**: returns real active/reaction/connections/demands/supplies ✅

## Part 50: Sovereignty Upgrade Effects (done)
- **Effect system extended**: `SystemEffectMgr` queries `sovUpgrades` and applies upgrade modifiers ✅
- **OnEnterSystem/OnLeaveSystem**: sovereignty effects applied alongside wormhole effects ✅
- **Upgrade type definitions**: Cynosural Navigation, Suppression, Logistics Network, Supercapital Facilities ✅

## Part 51: Courier Contracts Full Implementation (done)
- **Nested containers**: `GetCrateContentsRecursive` validates items inside containers within crates ✅
- **forCorp in DeleteContract**: returns items to corp hangar when contract was issued for corp ✅
- **Recursive completion validation**: `CompleteContract` checks both top-level and nested items ✅
- **Petition stub**: `GetMyPetitionsEx` returns empty list ✅
- **FW GetFactionalWarStatus**: added `status` field to returned KeyVal ✅

## Part 52: Corp Window & Role Groups Fixes (done)
- **GetCorpRoleGroups**: manual CRowSet with `columns` as `PyList` instead of SQL string ✅
- **GetCorpRoles**: empty CRowSet with correct header (crpRoles table overwritten by migration) ✅
- **GetRoles/GetRoleGroups**: return empty CRowSet instead of nullptr on SQL failure ✅
- **COALESCE division names**: `GetCorporation` returns default names when `crpWalletDivisons` row missing ✅

## Part 53: Orbit Desync Fix + Stability (done)
- **Orbit distance fallback**: `m_followDistance = distance` instead of `(distance+radius+Tr)/6` ✅
- **Distance thresholds**: use `max(m_followDistance, m_targetDistance)` as reference ✅
- **Periodic position sync**: `SetBallPosition` every 5 tics in `MoveObject` to prevent drift ✅
- **Login warp overview**: skip `SendSetState` during login (conflicts with WarpLoop), send after warp complete ✅
- **Orbit crash fix**: null target check in `Orbit()` and `SetSpeedFraction` ✅
- **NaN guards**: velocity and newPos NaN checks in `MoveObject` ✅

## Part 54: NPC Crosshair Fixes (done)
- **CategoryID in type cache**: `Generate_invTypes` now includes `categoryID=6` for Entity NPC types ✅
- **BulkDataChangeID auto**: computed from migration timestamps, no manual bumps ✅
- **AddBallExclusive packet_type=0**: full state instead of incremental, forces client re-init ✅
- **Civilian AddNPC order**: `SetPosition` before `AddNPC` for correct bubble assignment ✅
- **Civilian unique names**: per-ship names + customInfo "CivilianTraffic" ✅

## Part 55: News Ticker + LiveUpdate (done)
- **srvNewsItems table**: stores news items with title/description/date ✅
- **GetNewsTickerData**: returns XML from DB via `holoscreenMgr` RPC ✅
- **LiveUpdate injection**: patches client `holoscreenSvc.GetNewsTickerData` to call server RPC ✅
- **seed_news.sh**: populates news from git log (`bash utils/seed_news.sh`) ✅

## Part 56: Network Crash Fix (done)
- **__cxa_pure_virtual**: `ProcessReceivedData` made non-pure virtual with default no-op ✅

## Part 57: Corp Window & Service Fixes (done)
- **UpdateCorporation**: accept 4th param `allowAccess` (client sends PyInt for tax) ✅
- **GetRecentKillsAndLosses**: `offset` optional (client sends PyNone) ✅
- **CreateRecruitmentAd**: `PyWString*` → `PyRep*` (client sends PyString) ✅
- **GetRecruitmentAdsByCriteria**: `PyBool*` → `PyInt*` ✅
- **PyVisitor kMaxDepth**: 4096→128 (stack overflow prevention) ✅

## Part 58: Civilian NPC Fixes (done)
- **GetStationPosition**: search both `m_entities` and `m_staticEntities` (stations/gates) ✅
- **ConvoyAI timer**: `Check(false)` instead of `Check(true)` (timer was never one-shot) ✅
- **ResetAfterTransit**: reset phase/timers after cross-system jump ✅
- **Entity-category Convoy types**: use group 297 instead of Ship-category 582-594 (avoids dogma crash) ✅
- **Random civilian names**: from agent names + bloodline surnames ✅

## Part 59: Anomaly NPC Crosshairs + Difficulty (done)
- **Sleeper groups**: 959-961,982-987 added to `Generate_invTypes` and `MakeSlimItem` ✅
- **IsMassive flag**: added to NPC and DSE `EncodeDestiny` (flags 0x07) ✅
- **SendSetState after spawn**: forces full ball re-init for all players in bubble ✅
- **Warp-to-0 distance**: reduced from 1000m to 0m ✅
- **NPC stat scaling by level**: `0.2 + level * 0.36` (L1=0.56x, L5=2.0x) ✅
- **Anomaly tier selection**: `minSecurity`/`maxSecurity`/`difficulty` columns on `dunDungeons` ✅
- **GetRandomDungeon**: filters by security, weighted by difficulty ✅
- **GetRandLevel**: weighted by system security (highsec→L1, nullsec→L3-5) ✅
- **Periodic sync**: `SetBallPosition` every 20 tics for player ships only ✅
- **Player pushback fix**: removed periodic sync for NPCs ✅

# TODO

## Part 60: Ship-Category NPCs + Anomaly Scanner + DED Complexes (done)
- **Ship-category typeIDs (34000-34023)**: созданы в БД, но больше не используются для спавна (ревертнуты на SDE typeIDs) — client cache не находит кастомные типы ✅
- **Anomaly scanner fix**: `AddSignal()` — пропуск NPC через `IsNPCSE()`, Entity/Celestial больше не падают в `default` (не добавляются как аномалии) ✅
- **SystemManager::AddEntity()**: не вызывает `AddSignal()` для NPC ✅
- **SystemManager::GetAllEntities()**: фильтр NPC/Celestial/Entity из сканера ✅
- **NPCAI::Process()**: таможенники не агрятся автоматически в Idle ✅
- **Customs orbit**: 15% скорость (было 5%) ✅
- **NPC orbit fix**: `m_flyRange` по умолчанию = `radius * 5` (NPC больше не стоят на месте) ✅
- **Generate_invTypes()**: добавлены ВСЕ NPC группы (Deadspace, Commander, Officer, Incursion) в SQL remap ✅
- **DoSpawnForAnomaly faction override**: фикс для 565 (Sansha BS был пропущен), 562 (Guristas → Sansha) ✅
- **Jita anomaly ban**: пропуск systemID 30000142 в `CreateAnomaly()` ✅
- **Customs region faction**: при `factionID=0` — `GetRegionFaction()` ✅
- **CONCORDOKKEN**: респавн CONCORD при убийстве с наращиванием мощности (+50% дмг/HP за волну) ✅
- **CONCORD despawn**: 5-10 минут после убийства преступника ✅
- **CONCORD targeting**: NPC таргетят и атакуют преступника каждые 500ms ✅
- **Data/Relic site dungeons**: SQL миграции для Radar(4) и Magnetometric(3) archetypes ✅
- **DED complex dungeons**: 1-5/10 для всех 6 фракций (30+ подземелий) ✅
- **DED loot**: `PopulateDEDContainer()` — Overseer's Personal Effects, faction modules, ship BPCs ✅
- **DED container unlock**: при смерти NPC в баббле — контейнеры разблокируются ✅
- **Prospector::DropItems()**: лут data/relic контейнеров (фракционные материалы, декрипторы, salvage) ✅
- **Prospector::CanActivate()**: Data Miner работает на `CelestialSE` контейнеры ✅
- **IncursionMgr SpawnSites**: раз в 5 минут (было каждый тик) ✅
- **ESI price import script**: `tools/import_prices.py` ✅
- **WarpUpdate crash fix**: null-check `m_targBubble` ✅
- **Autopilot diagnostics**: частично — CmdWarpToStuffAutopilot работает, цепочка прыжков тестируется
- **FW LP earning**: реализован для PvP (CrimeWatch: 1000 LP) и NPC kills (NPC::Killed: 100-5000 LP) ✅  
- **FW Plex capture**: не реализован — требуется акселерационные гейты, захват-таймер, NPC-защитники (большая фича)
- **BubbleCast destiny update**: Err → fallback Add to BubbleMgr ✅

## Part 61: Stability & QoL Pass (done)
- **Marshal crash**: use-after-free в `FlushPendingDestinyUpdates` (double PyDecRef) ✅
- **PyVisitor null elements**: защита от null в Tuple/List Visit ✅
- **MarshalStream depth guard**: missing check в `VisitSubStream` ✅
- **Bubble guard**: `Stop()`/`ClearTurn()` — только при `SysBubble() != nullptr` ✅
- **Client AttributeError**: `Warping:stop` убран из `SendSetState` ✅
- **NPCMarket**: table name `market_orders` → `mktOrders` ✅
- **MarketDB LIMIT clauses**: StationOrderLimit/SystemOrderLimit/RegionOrderLimit включены ✅
- **Escrow refund exploit**: `ModifyCharOrder` — charge on raise, refund on lower (не negative = withdraw) ✅
- **Daytrading skill**: проверка в `ModifyCharOrder` при удалённой модификации ✅

## Part 62: Agents & Mail (done)
- **GetCareerAgents()**: реализован (была заглушка `return 0`) ✅
- **AgentBound stubs**: 6 методов — возвращают None/Dict вместо nullptr ✅
- **MailDB SQL errors**: MoveToTrash/MoveAllFromTrash/MoveAllToTrash (WHERE before SET) ✅
- **MailMgrService stubs**: MarkAsReadByList, MarkAsUnreadByList, MoveToTrashByLabel/List ✅
- **MailingListMgrService**: 14 stubs реализованы (KickMembers, SetEntityAccess, роли, welcome mail, GetInfo) ✅
- **NotificationMgrService groupID**: фильтрация через `NotifyTypeToGroup()` ✅
- **OnlineStatusService**: GetInitialState — контакты показывают реальный online статус ✅
- **LSC GetMember**: возвращает инфу о члене канала (было nullptr) ✅
- **LSC AccessControl/UpdateConfig**: применяет/сохраняет изменения доступа ✅

## Part 63: Market (done)
- **GetSkillLimits RPC**: broker fee, tax, order limits, skills ✅
- **NPCMarket table**: market_orders → mktOrders ✅
- **MarketDB LIMITs**: включены для station/system/region asks ✅
- **Escrow refund bug**: пофикшен (exploit — можно было вывести ISK) ✅
- **Daytrading skill check**: в ModifyCharOrder ✅

## Part 64: Science & Industry (done)
- **GetBlueprintInformationAtLocation**: S&I окно показывает чертежи ✅
- **ManufacturingService::GetPathToItem**: резолвит расположение чертежа ✅
- **Invention formula**: EvEMath со скиллами, мета-уровнем, декриптором ✅
- **Research field selection**: выбор поля через DoAction(skillTypeID) → chrResearch + pointsPerDay ✅
- **Research point accumulation**: фоновая задача каждый час ✅
- **POS assembly lines (Corp + Alliance)**: UNION + entity запрос для POS-модулей ✅
- **LocationRolesCheck**: проверка FactoryManager роли ✅
- **UpdateAssemblyLineConfigurations**: возвращает None (было nullptr) ✅
- **Job completion mail**: простое письмо при завершении джобы ✅
- **Invention decryptor modifier**: lookup из invMetaTypes ✅

## Part 65: Standings & Calendar (done)
- **GetMySecurityRating RPC**: новый (клиент вызывал, сервер не отвечал) ✅
- **GetStandingEventTypes RPC**: возвращает типы событий standings ✅
- **Kill rights standing**: не хардкод 10.0, вычисляется реальный ✅
- **Calendar SQL schema**: 3 таблицы (sysCalendarEvents/Invitees/Responses) с +migrate Up/Down ✅
- **Calendar SQL injection**: экранирование title/description ✅
- **Calendar invitee bug**: сохранялись указатели PyRep* вместо ID ✅

## Part 66: Destiny Smoothing & Crosshairs (done)
- **Follow() smoothing**: hysteresis exit threshold (×1.5), gradual accel/decel (+15%/−20% per tick) ✅
- **Orbit() smoothing**: heading blending (30% old + 70% new), непрерывный MoveObject ✅
- **Autopilot approach**: полная остановка у врат, distance 2500м + радиус корабля ✅
- **Generate_invTypes**: добавлен groupID 1052 (Incursion Sansha Capital) → Battleship(27) ✅
- **NPC::MakeSlimItem**: добавлен case 1052 ✅
- **SpawnMgr**: gID!=1055 в Sansha range check (группа не существует) ✅

## Part 67: Calendar migration fix (done)
- **+migrate Up/Down annotations**: добавлены в calendar_tables.sql ✅

## Part 68: Crosshair Fix + DB Fixes + Warp-2-N km (done)
- **NPC crosshair**: `NPC::MakeSlimItem` now delegates to `DynamicSystemEntity::MakeSlimItem()` — uses real `categoryID=11`/`groupID=550` from DB instead of hardcoded `categoryID=6/groupID=25` ✅
- **NPC EncodeDestiny**: flags changed from `IsInteractive|IsFree|IsMassive (13)` to `IsFree (1)` (matching ShipSE without pilot) ✅
- **DB OnlineStatusService**: `characterID` → `ownerID`, `inContacts` → `inWatchlist` (columns didn't exist) ✅
- **Warp-to-N km**: don't subtract target radius from `warpToPoint` when `distance > 0` (radius + minRange was doubling landing distance) ✅
- **Warp-to-anomaly**: skip planet formula (`radius>90km`) for `IsAnomalySE()` — warp goes to center + distance ✅
- **Orbit jerking reduced**: tangent-based heading + `refFollow` fallback to `m_targetDistance` when `m_followDistance=0` ✅
- **Customs NPC names**: `GetFactionName` now returns Caldari Navy/Minmatar Republic/Amarr Empire/Gallente Federation/CONCORD instead of "Undefined" ✅
- **Build fix**: `fwrite` raw pointer cast (`buf.begin<uint8>()` → `&buf[0]`) in `SystemBubble.cpp` ✅

# TODO

## 🔴 Высокий приоритет (играбельные баги)
1. **Autopilot chain** — после прыжка клиент не продолжает маршрут (клиентская проблема)

## 🟡 Средний приоритет (крупные фичи)
1. **Jump drives / Capital ships** — cyno, portal, bridge работают. Добавлены IsCovert, blops bridge restriction
2. **Full NPC AI** — web, ECM, target paint добавлены в NPCAIMgr (читает атрибуты сущностей)
3. **S&I — Manufacturing** — материал мультипликатор работает через Calculate(), GetAdjustedRamRequiredMaterials — мёртвая функция
4. **Full LP store** — LPService + storeServer (AcceptOffer, GetAvailableOffers) реализованы
5. **FW plex capture** — NPC защитники от system occupier, размерные ограничения, contested (пауза таймера)
6. **Killmail** — finalSecurityStatus берётся из реального secStatus атакующего (вместо 0)

## 🟢 Низкий приоритет (мелкие стабы)
1. **GetRecentSovActivity** ✅ — возвращает CRowset из mapSystemSovInfo
2. **GetDeadspaceAgentsMap / GetDeadspaceComplexMap** ✅ — возвращают пустой PyDict, лог почищен
3. **GetMyEscalatingPathDetails** ✅ — возвращает пустой PyList вместо None
4. **CopyBookmarks / MoveFoldersToDB** — уже возвращали PyStatic.NewNone() ✅
5. **GetApprenticeships** — метод не найден в коде (возможно удалён клиентом)
6. **AccruedTime / SetLanguageID** ✅ — возвращают PyStatic.NewNone() вместо nullptr
7. **GetRecentEpicArcCompletions** ✅ — возвращает пустой PyList вместо nullptr
8. **DamageModules** ✅ — уже возвращал PyStatic.NewNone()
9. **GMChangeSpaceObjectOwner** ✅ — уже возвращал PyStatic.NewNone()

## 🔵 Крупные системы (нужен фундаментальный подход)
1. **Planetary Interaction (PI)** — полная система (колонии, экстракторы, процессоры, линки, кастомс офисы). **0%**
2. **RefPtr → shared_ptr** — major refactoring ~400 files
3. **PyRep memory management** — valgrind leak fixes

## ✅ Done this session
- **Corporate offices** — CancelRentOfOffice: impound + OnOfficeRentalChanged уведомление гостям станции
- **Insurance** — исправлена: нестрахованный корабль получает 40% базовой цены (вместо 15K), дробные диапазоны вместо точного float сравнения, `InsurancePayout` уведомление, фоновый `ProcessInsuranceExpiry()` раз в минуту + `InsuranceExpiration` уведомление
- **FW GetVictoryPoints** (MapService) — возвращает состояние FW систем вместо None
- **ReconnectToLostProbes** — переподключает зонды после реконнекта (SendNewProbe для всех ProbeSE в системе)
- **Cloaking / Covert Ops** — aligned-before-cloak: Covert Ops cloak требует нулевой скорости перед активацией
- **Auto-bills** — `ProcessAutoPay()` раз в минуту оплачивает счета корпораций с включённым `crpAutoPay` (MarketFine, RentalBill, BrokerBill, AllianceMaintenance, Sovereignty)
- **FW GetStats** — 7 стабов реализованы: FactionInfo, TopAndAllKillsAndVPs, Corp, Alliance, Militia, CorpPilots, RefreshCorps
- **Fleet watchlist** — 3 метода FleetManager реализованы: AddToWatchlist, RemoveFromWatchlist, RegisterForDamageUpdates (с хранением в FleetService)
- **Fleet voice chat** — 3 метода FleetBound: AddToVoiceChat, SetVoiceMuteStatus, ExcludeFromVoiceMute (no-op, без ворнингов)
- **SubSystemModule** — T3 сабсистемы online/active при установке
- **UpdateAssemblyLineConfigurations** — сохранение в БД
- **TutorialService** — LogStarted/Completed/Aborted, GetCharacterTutorialState
- **COSMOS missions** — загрузка из БД
- **Research RP** — фоновое накопление (раньше не вызывалось)
- **Research journal** — отображение в журнале
- **War bills** — рекуррентные еженедельные счета
- **PayoutDividend** — выплата дивидендов
- **CreateAlliance** (unbound) — имплементирован
- **GetMyCourierMissions** — возвращает данные
- **5 AgentBound stubs** — реализованы
- **6 CorpRegistryBound stubs** — валидные PyRep

## 🟢 Eventually
1. **RefPtr → shared_ptr** — major refactoring ~400 files
2. **PyRep memory management** — valgrind leak fixes
