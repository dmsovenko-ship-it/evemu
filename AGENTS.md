# EVEmu Session Context

## Current State
Session saved. Latest commit: `865a5b64`. Server on remote host `172.20.1.47`, SSH user: `dmitry` (password `gbnjy78`), path: `/opt/evemu`.

## Главное достижение (3 августа): AP gate-jump ПОЧИНЕН
Автопилот теперь проходит много-гейтовый маршрут без зависаний и рывков:
- корабль подлетает к гейту, прыгает, сразу продолжает (варп к следующему гейту), прыгает дальше, докидывает до станции.

### Три бага, которые были найдены и исправлены
1. **AP-стагнация после прыжка** (~2 мин молчания, потом сам очнулся; док/андок «лечил»). Причина: клиентский `starmap.UpdateRoute(fakeUpdate=True)` в `OnSessionChanged` мог отработать слишком рано во время перехода (stale маршрут → `destID None` → AP молча ждёт). **Фикс (`960d6715`)**: сервер через 5 сек после прыжка шлёт повторную «доброкачественную» смену сессии (`nextSessionChange`) → клиент заново вызывает `OnSessionChanged → UpdateRoute`, маршрут продвигается, AP продолжает. Код: `Client.cpp` `StargateJump` (m_apSessionRetry=true + m_stateTimer.Start(5000)) и `ProcessClient` Idle-кейс.
2. **Первый варп после прыжка срывался Halt()**. Причина: после прыжка корабль в битом состоянии (`USF=0, m_stop=true, но TF/ASF=1.0` — наследие от pre-jump follow/warp), MoveObject при `USF==0` вызывал `Halt()` → варп отменялся, потом re-warp. **Фикс (`cc5ffaa2`)**: в выравнивании WARP-режима `else if (m_userSpeedFraction < 0.7499) SetSpeedFraction(1.0f, true)` — убрано условие `m_timeFraction < 0.749` (теперь перевооружает корабль независимо от TF).
3. **Посадка AP-варпа на больших гейтах** (была вплотную ~950м от поверхности). Клиентский WarpLoop всегда сажает warp-to-gate ~15км от **ЦЕНТРА** гейта независимо от stop-distance → на большом гейте (радиус 14км) корабль на гейте; увеличение stop-distance → пролёт сквозь гейт + рывок назад. **Фикс (`29ecf6ce`)**: AP варпит к **ТОЧКЕ** `radius + apWarptoDistance` от центра гейта (warp-to-point, сажается точно). Итог: ~11км от поверхности малого гейта, ~14км от большого, без рывков. Код: `BeyonceService.cpp` `CmdWarpToStuffAutopilot` (GPoint landPoint = gatePos - toGate * (radius + apWarptoDistance); WarpTo(landPoint, 0)).

## Карта для отладки (системы/гейты)
- 30000197 = Uemon, 30000198 = Paala, 30002355 = LXQ2-T, 30000196 = Otosela.
- Гейты: 50011022 «Stargate (Paala)» в Uemon→Paala (radius ~19km); 50011530 «Stargate (Uemon)» в Paala→Uemon (radius 3532); 50014211 «Stargate (LXQ2-T)» в Paala→LXQ2-T (radius 14051); 50014212 «Stargate (Paala)» в LXQ2-T→Paala.
- Rattlesnake warpSpeedMultiplier=3 (3 AU/s).

## Клиент (важно)
- Реальный клиент: **`C:\Program Files (x86)\CCP\EVE`** (НЕ `C:\EVE`). Crucible 2012, `start.ini` server=router.iks-online.net:26000.
- Декомпил: `C:\opencode-projects\other\all\` (autopilot.py, starMapSvc_py.py, sessions_py.py, pathfinder_py.py).
- Dev-консоль клиента (`~`) НЕ работает — инспектировать клиент нельзя.
- Клиентские log-каналы (`svc.autoPilot` в log.ini/start.ini [log]) НЕ пишут в gamelog — попытки не сработали.

## Сервер: docker-compose сломан
`docker-compose up` падает `KeyError: ContainerConfig` (compose 1.29.2 vs docker 29.1.3). Контейнеры вручную:
```bash
docker stop server db && docker rm server db
docker run -d -t -i --name server --network evemu_default -v /opt/evemu/config:/app/etc -v evemu_server_cache:/app/server_cache -v evemu_image_cache:/app/image_cache -v evemu_ccache:/ccache -p 26000:26000 -p 26001:26001 -e SEED_MARKET=TRUE -e SEED_SATURATION=75 -e 'SEED_REGIONS=...' -e RUN_WITH_GDB=FALSE evemu_server
docker run -d --name db --network evemu_default -v evemu_db:/var/lib/mysql -e MARIADB_RANDOM_ROOT_PASSWORD=true -e MARIADB_USER=evemu -e MARIADB_PASSWORD=evemu -e MARIADB_DATABASE=evemu mariadb:11.8 --innodb-buffer-pool-size=1G --innodb-log-file-size=256M --innodb-flush-log-at-trx-commit=2 --max-allowed-packet=64M --bulk-insert-buffer-size=64M
```
**ОБЯЗАТЕЛЬНО `-t -i`** для server (иначе спам `Command not recognized:`). Включён `DESTINY__WARP_TRACE=1` и `AUTOPILOT__MESSAGE=1` в /opt/evemu/config/log.ini.

## Осталось
1. **Дроны** — два бага (частично диагностированы, см. ниже).
2. **Bracket crash** — `'NoneType' object has no attribute 'lower'` при наведении.
3. **Мобилка после анчоринга не скрамблит** — проверить.
4. **Incursion rewards / ISK на сайте Uroborus** — проверить.

## Дроны — диагностика (4 августа)
- **«1 пропадает при запуске»**: лимит дронов = 5 (AttrMaxActiveDrones, DCU нет). Клиент шлёт 6 LaunchDrone → 6-й отклоняется лимитом (SE не создаётся) → «1 потерялся». Это корректное поведение лимита, но клиентский drone-window не синхронизируется с отказом. Задеплоен фикс `f86e7191`: `ShipSE::GetDroneLimit()` (char AttrMaxActiveDrones + online DCU-бонусы) — раньше Drop и LaunchDrone считали лимит по-разному (LaunchDrone не учитывал DCU), из-за чего последний дрон партии отклонялся.
- **«Дроны улетают в дальний космос при scoop / не забрать»**: первый клик scoop РАБОТАЕТ (итем уходит в дрон-бэй, SE удаляются, flight-лист пуст — подтверждено DIAG: itemFlag=87, flightCount=0). Второй клик не находит SE (`Unable to find droneSE`) — дроны уже в бэе. Проблема в клиенте: drone-ball'ы не удаляются/не останавливаются корректно → визуально «улетают в космос», юзер думает что дроны в космосе и жмёт scoop снова. Это клиентский FOLLOW/ORBIT десинхрон (клиент ведёт шары сам, сервер не шлёт позиции в FOLLOW/ORBIT-режимах — DestinyManager.cpp:1016). TODO: разобраться с удалением шара дрона при scoop (RemoveBall при null-bubble пропускается: DestinyManager.cpp:641, SystemManager.cpp:1174) и/или с возвратом дрона (Follow(ship,0) в DroneAI.cpp:413 — возможный overshoot).
- **NEW: дроны улетают сами по себе (idle, без цели)** — воспроизведено 4 авг (дрон 140006149):
  - Запуск → IdleOrbit (ORBIT mode, usf=0.6) → дрон корректно орбитит 88.9 м/с ~4 сек (centers≈1300м).
  - В 14:10:57 SetSpeedFraction(0.6, start:true), затем 0.75 → 0.80 (+0.15/тик) — это **accel-логика FOLLOW()** в DestinyManager.cpp:1327 (`newSpeed = usf + 0.15`, cap maxApproach=0.8). Дрон перешёл в **FOLLOW mode** и улетел по прямой на 1824 м/с (0.8×2280). Через ~2 мин был на 210-600 км.
  - КТО переводит дрона в FOLLOW — НЕ найден. CmdFollowBall/CmdSetSpeedFraction клиента действуют на `GetShipSE()` (корабль), не на дрон. DroneAI не вызывает SetSpeedFraction/Follow при idle. Гипотеза: клиентский шар дрона шлёт CmdFollowBall/CmdOrbit при создании балла (мишель), сервер почему-то применяет к дрону. НУЖНО: залогировать все команды destiny для дрона (CmdFollowBall/CmdOrbit/CmdGotoPoint/CmdSetSpeedFraction на drone destiny) при воспроизведении.
  - **Подсказка юзера**: на коммитах, когда допиливали цетрей/майнинг-дронов, дроны летали НОРМАЛЬНО. Значит сломано одним из более поздних коммитов по орбиту/follow: `cf7a1fbd` (MoveObject SetPosition+тангенс), `3b0fb5bb` (Follow min decel 500м), `fec219f0` (skip SetBallPosition GOTO), `2b096795` (skip SetBallPosition Orbit), `b56b859b` (advance pos FOLLOW). Стоит откатить по очереди и сравнить.
  - **Побочно**: дрон с 210 км «достреливает» NPC в 11 км — проверка дальности атаки использует рассинхронённую позицию (нужно чинить вместе с позицией).
  - maxVelocity Infiltrator II в БД = 2280 м/с (реальная ~625) — возможно завышена, но не причина прямолинейного полёта.
- **Крэш при отзыве дронов** (SEGV, исправлен коммитом `60ed19f4`): ScoopDrone → Offline → `AssignShip(nullptr)`, а Departing-хендлер DroneAI (line ~251) разыменовывал `m_assignedShip->GetPosition()` без null-проверки → NULL deref. Фикс: guard `if (m_assignedShip == nullptr or m_assignedShip->DestinyMgr() == nullptr) { SetIdle(); break; }` в начале Departing-кейса.
- Лог-каналы: DRONE__ERROR=1, DRONE__AI_TRACE=1, NPC__AI_TRACE=1, DESTINY__ORBIT_TRACE=1, DESTINY__MOVE_TRACE=1 (для диагностики дронов включены ORBIT/MOVE_TRACE — после фикса убрать).

## Имена предметов — приведены к клиенту (4 августа)
- **Проблема**: `/create 'Inferno Precision Light Missile'` → «Unable to find valid type to create», хотя итем продаётся на рынке. Причина: серверная `invTypes` содержит старые (Crucible-era) имена (`Flameburst Precision Light Missile`, `Bloodclaw Light Missile`, `Standard Missile Launcher I`), а клиент (SDE новее) показывает современные (`Inferno Precision Light Missile`, `Scourge Light Missile`, `Light Missile Launcher I`). Рынок работает (типы по typeID), но поиск по имени в `/create` падал. До ресета базы эта проблема не была — старая база уже имела client-like имена.
- **Источник клиентских имён**: `C:\Program Files (x86)\CCP\EVE\bulkdata\600004.cache2` — это клиентский кэш CRowset'а `invTypes` (blue.marshal, PyPackedRow). Распарсен скриптом `C:\Users\Dima\AppData\Local\Temp\opencode\dump_cache2.py` → `client_names.tsv` (18710 строк). Также есть современный SDE `C:\opencode-projects\misc\sde\fsd\types.yaml` (153MB, имена `en:`), который совпадает с cache2.
- **Фикс**: сравнены клиентские vs серверные имена → **1405 расхождений** (17272 уже совпадали). Сгенерирован `rename_invtypes.sql` (`UPDATE invTypes SET typeName=...` + `UPDATE entity SET itemName=...`) и выполнен в db-контейнере. Проверка: 2647 = «Inferno Precision Light Missile», 202 = «Mjolnir Cruise Missile», 499 = «Light Missile Launcher I».
- **Несовпавшие**: 33 типа есть только в клиенте (новые), 1041 только в сервере (непубличные/внутренние) — им имена не менялись.
- ВАЖНО: имена в `invTypes` — мастер-данные; при ресете базы (evedbtool install) они откатятся к старым — нужно повторить скрипт.

## Git Log (верх)
```
29ecf6ce fix: AP warp-to-gate targets a point radius+apWarptoDistance from gate center (warp-to-point)
8b674e50 fix: AP warp-to-gate lands apWarptoDistance from gate surface (add gate radius), capped at 25km
865a5b64 revert: AP warp stop distance back to apWarptoDistance (15000 from gate center)
95fe7f34 fix: AP warp-to-gate lands apWarptoDistance from gate surface (REVERTED)
cc5ffaa2 fix: warp alignment re-arms ship when USF<0.75 regardless of TF
960d6715 fix: re-send benign session change (nextSessionChange) ~5s after gate jump
f8bca947 fix: StargateJump range check surface-to-surface distance
92918a2d fix: wrong table name crpCorporations -> crpCorporation
27afe41d feat: ECM bomb jams targets + Energy bomb drains 1800GJ
eea108d8 feat: add Bomb_ECM/Bomb_Energy group enum + AoE detonation
88b4f45e feat: bomb launcher mechanics — bombs fly straight, detonate AoE
4ee3d397 fix: missile launcher cycle not displayed — ShowEffect charge effectID
011a2b98 fix: TCU/anchored structures with AttrAnchoringDelay
46a3ef07 fix: reject CmdStargateJump beyond maxStargateJumpingDistance
ab1d6677 feat: gate activation effects (GateActivity)
c5df3622 fix: add jump cloak + invul after gate jump
a02fca16 fix: StargateJump mirrors Command_tr exactly
7aa86e89 fix: AP gate jump uses .tr-style MoveToLocation flow
b6e77ec3 fix: remove double-wrap in AddBalls XML
fe8ece5b fix: remove double-wrapping in AddBalls2 XML
```

## Key Decisions
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
