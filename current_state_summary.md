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

# TODO (next session)

## High Priority
1. **Jump clone ship bay** — full AcceptShipCloneInstallation/OfferShipCloneInstallation workflow
2. **Per-clone implants through UI** — assign implants to specific jump clones from clone window
3. **Ship clone bay** — full ship-to-ship clone transfer (Rorqual etc.)

## Medium Priority
4. **Contest system** — player contest mechanics for incursions
5. **Incursion penalties** — resistance/damage/bounty/cyno penalties in incursion systems
6. **LaunchSnowBall** — implement snowball launcher module
7. **War decay timer** — auto-check unpaid war bills and end wars
8. **Contract PlaceBid / FinishAuction** — auction support
9. **Corp Mail groups** — send mail to corp role groups (Directors, Officers, etc.)

## Low Priority
10. **Fresh DB rebuild** — `docker-compose down -v && docker-compose up --build -d` to verify all migrations
11. **Various header stub cleanup** — `PassiveModule()`, `RigModule()`, `SubSystemModule()` empty constructors

## Code Review Needed
- Verify incursion dungeon spawning works across multiple systems simultaneously
- Verify wave progression doesn't stall on incursion sites
- Verify mothership encounter triggers correctly when hasBoss=1
