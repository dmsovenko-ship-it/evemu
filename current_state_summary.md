# Session: Corps/Alliances/Wars/Bills/Mail + Anomaly NPCs

## Part 1: Anomaly NPCs (done)
- **Root cause of empty anomalies**: NPC typeIDs (33001+) assigned Entity-category groups (catID=11), but `MakeDungeon` checks for Ship (catID=6) / Drone (catID=18)
- **Migration consolidation**: Single file `20260705000037-anomaly_npcs_final.sql` — ALTER TABLE, INSERT types (33001+), DELETE old objects, INSERT correct typeIDs
- **groupID fix**: Changed Entity groups (562,566,567,565...) → Ship groups (25=Frigate, 26=Cruiser, 27=Battleship, 419=Battlecruiser), Drone group (100=CombatDrone)
- **graphicID/radius fix**: Added to all 24 NPC types to prevent client `RecordNotFound` KeyError and server marshal crash
- **Temp TYPEID FORCE** in DungeonMgr.cpp: force-spawn all 33000+ room objects as NPCs (removable after fresh DB install)
- **DB issue on existing install**: `dunRoomObjects` empty for rooms 2000-2093, `dunDungeons`/`dunRooms` may be missing → need fresh DB (`docker-compose down -v` + `docker-compose up --build -d`)

## Part 2: Bills (done)
- `CharPayBill(billID)` — checks balance, deducts via `AccountService::TransferFunds`, marks bill paid
- `CharGetBills()` / `CharGetBillsReceivable()` — queries from `billsPayable`/`billsReceivable` for character
- `PayCorporationBill(billID, fromAccountKey)` — checks corp wallet via `AccountDB::GetCorpBalance`, transfers via TransferFunds
- `SendAutomaticPaySettings(dict)` — parses 6 bill type settings, saves to `crpAutoPay` via REPLACE INTO
- `CorporationDB::SetAutoPay()` — was empty, now implemented
- `CorporationDB::GetCorporationBills()` — fixed SQL bug: `paid externalID2` → `externalID2, paid`

## Part 3: Alliance Wars (done)
- `DeclareWarAgainst(againstID)` — checks war registry for existing war, deducts ISK (50M default), creates `warRegistry` record, creates `billsPayable` entry (weekly bill = 25% of declaration cost)
- `RetractWar(againstID)` — marks war as retracted with timestamp
- `ChangeMutualWarFlag(warID, mutual)` — updates mutual flag
- `GetCostOfWarAgainst(ownerID)` — returns 50M ISK
- `GetWars(ownerID)` — now reads from `warRegistry` table (was returning empty rowset)
- Table `warRegistry` auto-created via `CREATE TABLE IF NOT EXISTS`

## Part 4: War-aware CONCORD/Sentry (done)
- **CrimeWatch::OnAggression()** — added war check before setting criminal flag, spawning CONCORD, or granting kill rights
- Checks corporation-level war first, then alliance-level war
- `warRegistry WHERE retracted=0 AND timeFinished=0` = active war
- War targets: no criminal flag, no CONCORD, no sec status loss, sentries don't aggro

## Part 5: AllianceDB (done)
- `DeleteLabel(allyID, labelID)` — was empty, now deletes from `alnLabels`
- `EditLabel(allyID, labelID, color, name)` — was empty, now updates `alnLabels`

## Part 6: Corporation Mail (done)
- `MailDB::SendMail()` — `toCorpOrAllianceID` now delivers to corp/alliance members
- Queries `chrCharacters` by `corporationID` or `allianceID`
- Sets `labelMask = 4` (Corp) or `8` (Alliance) in `mailStatus`
- Checks online members first, falls back to all members

## Part 7: PROGRESS.md updated
- Alliance Bulletins: 20% → 40%
- Alliance Wars: 10% → 40%
- Corp Mail: 0% → 30%

## TODO (next session)

### High Priority
1. **Clones / Jump Clones** — ALL 10 methods in `JumpCloneService` are stubs:
   - `CloneJump(locationID)` — implement jump clone travel
   - `InstallCloneInStation()` — install jump clone at station
   - `DestroyInstalledClone(cloneID)` — remove jump clone
   - `GetCloneState()` / `GetStationCloneState()` — return real DB data
   - `GetShipCloneState()` — ship clone bay support
   - `GetPriceForClone()` — real price calculation
   - `OfferShipCloneInstallation` / `AcceptShipCloneInstallation` / `CancelShipCloneInstallation` — ship clone transfer workflow
   - Implant management: `CharAddImplant`, `RemoveImplantFromCharacter`, `GetCharacterAttributeModifiers`
   - Clone death: destroy implants, check SP vs clone grade, adjust skills
   - Jump clone cooldown, max clone limit, facility checks
   - Send notification events: `OnJumpCloneCacheInvalidated`, etc.

### Medium Priority
2. **Corporation Mail** — implement missing mail features (send to corp group from corp UI)
3. **Alliance Wars** — add war cost config option, war decay, war history

### Low Priority
4. **Fresh DB rebuild** — `docker-compose down -v && docker-compose up --build -d` to verify all migrations work on clean DB
5. **Remove TYPEID FORCE hack** from DungeonMgr.cpp after fresh DB confirms groupIDs are correct
6. **Remove debug logging** from DungeonMgr.cpp and SpawnMgr.cpp

### Code Review Needed
- Verify `WarRegistryService.cpp` compiles and runs (uses `RunQueryLID` instead of `GetLastInsertID`)
- Verify `AccountService::TransferFunds` works for corp wallet deductions in `PayCorporationBill`
- Verify corp member lookup in `MailDB::SendMail` works correctly
