# EVEmu Session Context

## Current State
Session saved. Latest commit: `977ff5d9`. Server on remote host `172.20.1.47`, SSH user: `dmitry` (password `gbnjy78`), path: `/opt/evemu`.

## Что сделано сегодня (31 июля)
1. **Destiny.xmlp AddBalls/AddBalls2** — восстановлен правильный формат (двойная обёртка), клиент `AddBalls2(self, chunk)` ждёт 1 аргумент.
2. **Дублирующиеся .sql файлы** — удалены 6 файлов (`crpVoteItems.sql` и др.), которые существовали и как `.sql` и как `.sql.gz`. `evedbtool` пытался читать `.sql` как gzip → `gzip: invalid header`.
3. **cacheLocations** — заполнена из SDE (планеты, луны, гейты, станции) — чинит овервью и меню варпа.
4. **career_agents migration** — добавлена колонка `description` в INSERT (strict mode MariaDB падал).
5. **docker-compose.yml** — сервер использует `evemu_server` тег (не `eve-server`).

## КРИТИЧНО для следующей сессии
**Сервер отстал от remote** — на `b6c94a50`, нужно обновить:
```bash
cd /opt/evemu && git fetch origin && git reset --hard origin/master
```

Потом сбросить БД полностью (фикс career_agents подтянется и EVEDBTool пройдёт дальше):
```bash
docker-compose down -v && docker-compose up -d
```

## Ошибка миграций
EVEDBTool остановился на `20260705000025-lp_store.sql` — `20260705000026-career_agents.sql` падал с `description doesn't have default value`. Фикс запушен (`66ee9bbc`), но сервер его не подтянул. После обновления миграции пройдут.

## Нерешённые проблемы
1. **Овервью** — показывала только станцию/сентри (cacheLocations была пуста). Фикс запушен, но не проверен на сервере.
2. **Меню варпа** — зависело от cacheLocations, тоже ждёт проверки.
3. **Дроны при scoop** — улетают в дальний космос.
4. **Дроны при запуске** — 1 пропадает.
5. **Bracket crash** — `'NoneType' object has no attribute 'lower'` при наведении.
6. **Incurions** — таблицы созданы, но EVEDBTool не накатил (нужен полный сброс БД).
7. **minSecurity/reinforceHour** — тоже ждут перезапуска миграций.

## Git Log
```
b6e77ec3 fix: remove double-wrap in AddBalls XML — same pattern as AddBalls2
fe8ece5b fix: remove double-wrapping in AddBalls2 XML — client expects (state, extraBallData)
339357f8 fix: add PyIncRef before act.update = *update — prevents use-after-free
2ac27b78 fix: add targeted action raw-type logging in _SendQueuedUpdates
b00af344 fix: reject destiny updates with non-scalar first item (e.g., list)
e5e76d7a fix: remove last stateStamp ref in SendStaticBall
8d99f0b6 fix: remove stateStamp ref in ShipService.cpp
c901541f fix: restructure AddBalls2 to start with string funcName
d5fd2877 fix: PyIncRef before push to m_pendingUpdates
d1b34cc8 fix: remove stateStamp refs in SystemManager.cpp
d24189e7 chore: comment SIGSEGV guard in MakeSetState
182a8cce fix: auto-cleanup orphaned decorations on server restart
df680a34 fix: add RemoveEntity in DroneSE destructor
a7e30938 fix: cleanup corrupt decoration entities from DB
48d15e07 fix: disable DoPackage path except SetState
c52e7993 fix: add RemoveEntity before delete this in ContainerSE::Process
a37a3a6a fix: remove DestinyMgr null check from MakeSetState
94954991 fix: make SystemBubble::GetID() const
41462a87 fix: build error — GetID() not const in MakeSetState
db232d41 debug: log MakeSetState entry
7519ad51 fix: skip entities not in m_ticEntities during MakeSetState
e4c7209e fix: set velocity toward target before InitWarp on alignment timeout
e9e5e202 fix: drone AI use-after-free + MakeSetState null check
9f822aa4 fix: BubblecastDestinyUpdate clones before broadcast
8d13e0c5 fix: TargetManager::QueueUpdate clones before broadcast
d4fee97c fix: SendDamageStateChanged crash
ffc681ae fix: incursionRewards PK fix
331413a9 feat: procedural faction-based dungeon decorations
```

## Key Decisions

### Warp Disrupt / MWD Bubble
- **No distance check in destiny.dll** — `OnActivatingWarp` only checks `m_activations[0].size() > 0` via `PyDict_GetItem`, no range check
- **Dynamic bubble toggle removed** — WarpDisruptFieldGenerating always stays visible; `AttrWarpScrambleStatus` handles range-based scramble per-ship
- **Bubble::Add** skips `SendAddBalls` for warping ships (WarpLoop crash prevention)
- `warpScrambleTimer` runs every 1000ms, sets `AttrWarpScrambleStatus` based on actual distance to bubble center

### Warp Physics
Based on decompiled `destiny.dll` (stored at `C:\opencode\projects\other\`):
- **Accel**: `distance = e^(3t)`, `speed = 3e^(3t)` ✓ (already implemented)
- **Decel**: `distance = total - e^(-t)*decelDist`, `speed = warpSpeed * e^(-t)` ✓ (already implemented)
- **Exit condition**: `distance < ball->radius` (changed from hardcoded 100m to `m_radius`)
- **Catch-all/30° warp**: no longer zeroes `m_velocity` — ship enters warp with momentum
- **PyMethodDef table**: only 4 exports — `FindShortestPath`, `Test`, `GetBoxCenter`, `SetConstant`

### Bump Physics
- **Fixed collision formula**: `distance -= (r1 + r2)` instead of `(r1 - r2)` — was computing surface distance incorrectly

### Client Crashes (all confirmed via decompiled `.py` files)
1. **graphicInfo=None** → `effects.SmartBomb`/`MicroWarpDrive` skipped in `ShowEffect`; `SendSpecialEffect` routes to `OnSpecialFX10` when graphicInfo=0
2. **bracket None.lower()** → `"name"` field added to `MakeSlimItem()` for all entity types
3. **AddBalls2 KeyError** → `EncodeDestiny()` for DeployableSE includes `DataSector` for `IsFree`
4. **WarpLoop crash** → `WarpStop()` sends no packets; `Bubble::Add()` skips `SendAddBalls` for warping ships

### Mail System
- `SelfEveMail()` in LSCService — inserts `mailMessage` + `mailStatus`
- `GetMailBody()` returns raw compressed data (client decompresses)
- `MailingListGetInfo/GetSettings` — fixed leading spaces in list names
- `OnMailSent` notifications

### Sovereignty
- `militaryPoints`/`industrialPoints` default changed from 5 to 0 (client expects raw values, no fallback)

### Ghidra Analysis
- JDK 21 + Ghidra 12.1.2 installed at `C:\Users\User\AppData\Local\Temp\ghidra_extract\`
- Decompiled `destiny.dll` output: `C:\Users\User\AppData\Local\Temp\opencode\destiny_decompile.txt` (48KB)
- Method table dump: `C:\Users\User\AppData\Local\Temp\opencode\destiny_methods.txt`
- Key functions: `OnActivatingWarp @ 0x1001b3c0`, `EntityWarpIn @ 0x10020ed0`, `OnDeactivatingWarp @ 0x100209f0`, `OnExitWarp @ 0x10004b00`
- Scripts: `ghidra_script.java`, `ghidra_full.java`, `ghidra_methods.java` in `C:\Users\User\AppData\Local\Temp\opencode\`

### Decompiled Client Python
- 1082 files decrypted from compiled.code → `C:\opencode\projects\other\all\`
- Key files: `michelle.py`, `fxSequencer.py`, `bracketMgr.py`, `godma.py`, `evemail.py`, `sovSvc.py`
- Doc string shows Python 2.4 compatibility, Crucible branch

## Progress

### Done
- **Mail**: SelfEveMail inserts mailMessage+mailStatus; GetMailBody raw compressed; mailing list leading-space fix; all 8 SendMail overloads (PyInt/PyBool/PyString/PyWString); MarkAsUnreadByList signature fix; MarkAsReadByList/TrashByList listID→messageID bug; SyncMail range filter via second param; OnMailSent notification
- **MWD deployables**: bubble toggle removed (always visible); server-side AttrWarpScrambleStatus check in WarpTo(); Bubble::Add sends OnSpecialFX14 with graphicInfo(range); warpScrambleTimer periodic 1000ms range checks; EncodeDestiny DataSector for IsFree; range hardcoded per SDE typeID (5k–48k); scramble cleanup on bubble exit (Remove) + per-player range check
- **Warp physics**: accel formula divide-by-3 fix (was 1/3 dist); capacitor mass unit fix (kg→Mkg, was ×1000); decel exit m_radius instead of hardcoded 100m; catch-all/30° no longer zeroes velocity; warp-to-0 surface landing; collision detection (bump off gates/stations); warp intercept via HasWarpBubble; JumpIn broadcast on gate jump
- **Defender missiles**: ShipSE::MissileLaunched auto-fires defenders; Missile::HitTarget intercepts missiles; public Destroy() method; Countermeasure_Launcher enabled in ModuleFactory
- **Client crash fixes**: graphicInfo=None→skip SmartBomb/MicroWarpDrive; bracket "name" field in all MakeSlimItem; AddBalls2 DataSector; WarpLoop SendAddBalls skip; Bump null-check pilots; sourceShipID in Missile MakeSlimItem
- **Destiny update use-after-free**: PyIncRef before act.update assignment in all paths; PyIncRef before push to m_pendingUpdates; reject empty tuples and non-scalar first items
- **AddBalls/AddBalls2 double-wrap**: removed extra tuple wrapping ((data),) → (data); restructured to start with string funcName; removed stateStamp field
- **OnModuleAttributeChange size**: restored oldValue (7-item tuple); events sent separately via OnMultiEvent to avoid RealFlushState unpack mismatch
- **Sovereignty**: militaryPoints/industrialPoints default 5→0

### To Test
- **Warp scramble**: ship inside MWD bubble gets "Warp drive is disrupted."; outside bubble (>range) can warp
- **Gate arrival**: JumpIn + GateActivity effects play when warping to a gate
- **Capacitor**: frigate ~3 GJ / 5 AU; battleship ~300 GJ / 5 AU; Warp Drive Operation skill reduces drain
- **Mail**: Cyrillic; reply/forward flags; labels; mailing lists; blocked-contact filter; rate limit
- **Defender**: active launcher + defender charge intercepts incoming missiles
- **Bump**: correct surface distance (r1+r2+BUMP_DISTANCE); notification messages
- **Brackets**: all entity types display name/type/corp/alliance without AttributeError
- **Sovereignty dashboard**: loads without error; index values display correctly
- **Gate jump**: JumpOut→JumpIn→GateActivity sequence; no SceneManager `NoneType.vx`

### Remaining Issues
- Full `WarpDisruptFieldGenerating` effect classification missing in Crucible
- SceneManager crash (`NoneType.vx`) is a secondary effect — primary warp crash now fixed
- `tabgroup UnicodeDecodeError` (CP1252) — client-side, needs `errors='replace'` in editplaintext.py
