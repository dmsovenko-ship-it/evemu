# Session: Full System — Clones/Wars/Mail/Anomaly NPCs/Corp Market/Contracts/Incursions/Modules

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
- **Alliance member info**: `GetMembers()` now returns `ceoID, memberCount, tickerName, taxRate, stationID, allianceMemberStartDate` alongside existing fields ✅
- **Alliance details pane**: `GetAllianceMembers()` enriched with the same additional corp info ✅
- **Role groups fix**: Added computed `columns` field (`1,2,3,4,5,6,7`) to `GetCorpRoleGroups()` — fixes client `KeyError: 1` crash when opening corporation members tab ✅

# TODO

## 🟡 If desired
1. **Civilian Manager** — multi-system routes (currently basic ConvoyAI, needs full multi-system traffic)
2. **Wormhole effects** — pulsar, magnetar, cataclysmic variable effects on ships/modules

## 🟢 Eventually
3. **RefPtr → shared_ptr** — major refactoring across ~400 files
4. **PyRep memory management** — valgrind leak fixes
