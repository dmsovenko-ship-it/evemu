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

# TODO (next session)

## High Priority
1. **Jump clone ship bay** — full AcceptShipCloneInstallation/OfferShipCloneInstallation workflow
2. **Ship clone bay** — full ship-to-ship clone transfer (Rorqual etc.)
3. **Contract PlaceBid / FinishAuction** — auction support

## Medium Priority
4. **Corp Mail groups** — send mail to corp role groups (Directors, Officers, etc.)

## Low Priority
5. **Fresh DB rebuild** — `docker-compose down -v && docker-compose up --build -d` to verify all migrations
6. **Various header stub cleanup** — `PassiveModule()`, `RigModule()`, `SubSystemModule()` empty constructors
