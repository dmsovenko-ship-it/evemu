# EVEmu Session Context

## Current State
Session saved. Latest commit: `1edb6957`. Server on remote host `172.20.1.47`, SSH user: `dmitry` (password `gbnjy78`), path: `/opt/evemu`. Всё ниже задеплоено (юзер собирает сам).

## 10 августа (день): фиксы орбиты/дронов/декора + подготовка к сборке
- **Орбита вокруг NPC раскручивалась** (`d52f47a2`): TooFar-ветка Orbit() имела ранний return при `m_orbiting==TooFar` — heading вычислялся один раз, корабль летел к устаревшей точке, дистанция росла 5→30км («орбитил NPC», дроны за кораблём = «телепорт в другой конец экрана»). Фикс: TooFar пересчитывает heading каждый тик.
- **Телепорт дрона при смене chase→orbit** (`3103b69a`): SetEngaged не слал SetPosition перед CmdOrbit (в отличие от SetApproaching/IdleOrbit) — клиентский Ballpark пересоздавал шар на новой орбите. Добавлен sync.
- **Дрон улетал за NPC** (`f9945956`): leash был 2x control range (40км); NPC с большим flyRange (Eradicator 27км) утаскивали дронов. Теперь 1x control range.
- **Орбита вокруг невидимой декорации** (`cc1cebb3`): IsTargetInvalid останавливает ORBIT/FOLLOW если у цели нет DestinyMgr (декорации CelestialSE) — фикс варпа «You are already warping».
- **import_prices.py**: порт 3306 контейнера db проброшен на хост (`-p 3306:3306`, данные в volume evemu_db). Запуск: `/opt/conda/bin/python tools/import_prices.py --db-host 127.0.0.1 --db-user evemu --db-pass evemu --db-name evemu`.
- **Контейнеры остановлены** для сборки юзером. Сеть evemu_default осталась (db не удалялся).


## 10 августа: подтверждено юзером — убитый дрон пропадает навсегда, врек остаётся (`2ebb22df` ClearController при DroneSE::Killed).
## Следующая проверка (cc1cebb3): варп после орбиты вокруг невидимой декорации.

## 9 августа (поздний вечер): SEGV при убийстве NPC — DOUBLE-REMOVE найден и исправлен
- **Симптом**: сервер упал в Segfault в 08:42:45 при бое в аномалии. Перед крашем `AnomalyMgr::RemoveSignal() - removing 750000078` **дважды** за 08:42:10 (двойное удаление NPC) + корабль юзера `140001543` удалён (варп) + `RemoveSignal(1000000288)` (ракета). Краш через 35с после двойного удаления — классический delayed heap-corruption.
- **Причина** (`5d086488`): `Damage::ApplyDamage` (Damage.cpp:373-374) вызывает `Killed(d)` (виртуальный → `NPC::Killed`) а затем `SystemEntity::Killed(d)`. Но `NPC::Killed` (конец метода) сам вызывал `m_system->RemoveNPC(this)`, который делает `RemoveEntity` + `pNPC->RemoveNPC()` (`m_self->Delete()`). Потом `SystemEntity::Killed` → `Delete()` снова `RemoveEntity` + `m_self->Delete()` → двойной RemoveEntity + двойной item-delete → heap corruption.
- **Фикс**: `NPC::Killed` теперь вызывает только `m_system->RemoveNPCFromList(this)` (новый метод: m_npcs.erase + sEntityList.RemoveNPC(), БЕЗ RemoveEntity/item-delete). Полное удаление (RemoveEntity + m_self->Delete) делает `SystemEntity::Killed`→`Delete()` сразу после.
- Также `/kill` и `/killallnpcs` (SystemCommands.cpp) вызывают `NPC::Killed` напрямую (не через Damage) — они теперь цепляют `npcEntity->SystemEntity::Killed(damage)` для полного удаления; `/kill` для NPC больше не делает отдельный `RemoveEntity` (он внутри SystemEntity::Killed).
- Комментарий в Damage.cpp:373 «Killed must NOT remove dead SE» — контракт восстановлен.

## Сессия 9 августа: декор видимость, призраки дронов, орбита/отталкивание
- **Декор невидим — ИСТИННАЯ причина** (`3a8490bd`): CelestialSE наследует `IsStaticEntity=true` → декор/контейнеры/облака попадали в static-map bubble (`m_entities`), а `GetEntities()`/`SendAddBalls()` шлют только `m_dynamicEntities`. `AddBallExclusive` при спавне не помогал (игрок заходит позже). Фикс: `CelestialSE::IsStaticEntity=false` — декор теперь dynamic и доходит до клиента. Гейты/планеты/луны — отдельные StaticSystemEntity классы, не затронуты.
- **Отталкивание корабля/NPC/дронов** (`0ca4a008`): collision check в `DestinyManager::ProcessState` выталкивал движущихся из КАЖДОЙ static-сущности с радиусом ≥500м — декорации (контейнеры 1174м) и врата ускорения (2341м) толкали всех в стороны каждый тик. Фикс: выталкивание только из IsGateSE/IsStationSE/IsPlanetSE/IsMoonSE.
- **Орбита раскручивалась** (`0ca4a008`): `mPos.y = radius * phi`, где phi уже радианы (~0.785) → вертикальный разлёт ±0.785×radius. Фикс: орбита в горизонтальной плоскости + малая Y-качка (0.05).
- **Призраки дронов в БД** (`2a8b3921` re-applied `7ec8e576`): `InventoryItem::Delete` для дронов (cat 18) удалял предмет полностью вместо junkyard-Move. Подтверждено: убитые дроны исчезают из entity. Клиентские «призраки» уходят по TerminalExplosion/RemoveBall.
- **Декор видимый** (`42d8e954` re-applied `c8e04d05`): AddBallExclusive после спавна (дополнительно к CelestialSE dynamic).

## Осталось (TODO)
1. Деплой `3a8490bd` + `0ca4a008` на сервер — юзер собирает.
2. Проверить: декор виден в Guristas-аномалии, врата ускорения в DED.
3. Проверить: убитый дрон исчезает, орбита не раскручивается, нет отталкивания.
4. FxError fxID=0 Rocket Launcher II (цикл ракет) — проверить 4ee3d397.
5. DB Error #1366 notificationText — бинарные данные в utf8.
6. Выключить диагностику в log.ini после финальной проверки.

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
1. **Дроны** — 4 фикса задеплоены (5 авг): scoop→бэй корабля (`m_shipRef->itemID()`, было `GetLocationID()`=система → дроны «пропадали»), мягкий sync позиций откачен (рывки орбиты), возврат дрона за 2x контроль-дистанции в бою (цель сбежала/варпнула), сообщения запуска (`MaxBandwidthExceeded2` правильное имя+аргументы droneName/bandwidthNeeded/droneBandwidthUsed, LaunchDrone→enum различает лимит/банду).
2. **Bracket crash** — `'NoneType' object has no attribute 'lower'` при наведении.
3. **Мобилка после анчоринга не скрамблит** — проверить.
4. **Incursion rewards / ISK на сайте Uroborus** — проверить.
5. **NPC AI** — ОСНОВНОЕ СДЕЛАНО (5 авг): NPC больше не «вклозе», орбитят по дальности, атакуют, скрамлят. Юзер подтвердил: «атака идет, скрамблят, орбитят». См. секцию «NPC AI (5 августа)» ниже. TODO: проверить ремонты на NPC с атрибутами (Plunderer: щит+25/5000мс); мелочи.
- ВАЖНО про диск: повторные `docker build` копят старые слои образов → диск 100% (237G/249G) → сервер падает на старте в `CachedObjectMgr::SaveCachedToFile` (fwrite assert). Лечится `docker image prune -f` (освободил 193GB). Периодически чистить.

## NPC AI (5 августа) — «не вклозе, а орбита по оптималам»
- **Диагностика**: NPC в бою орбитили вплотную (Plunderer dist 500-600м, Mortifier ~950м). Причины: (1) `AttrMaxRange` у части NPC = 0/223м (Dire Guristas Murderer/Plunderer, Guristas Plunderer) → команда орбиты = точка-в-точку; (2) `Attack()` вызывается только из `CheckDistance`, а `m_mainAttackTimer` запускался ТОЛЬКО в `Target()` — если NPC агрил через `Targeted()` (игрок/дрон атакует первым), таймер не стартовал → NPC орбитил молча, вообще не стрелял; (3) `AttrSpeed` (цикл оружия в ms) у Guristas Plunderer/Mortifier = **30000 (30 сек!)** → казалось что NPC мёртвые.
- **Фиксы**:
  - `672afd73`: минимальная дистанция орбиты — `if (m_optimalRange < 1000) m_optimalRange = max(1500, min(5000, m_maxAttackRange/2))` в конструкторе NPCAI.cpp.
  - `04ee6348`: в `Targeted()` добавлен запуск `m_mainAttackTimer.Start(m_attackSpeed)` + `m_missileTimer` (раньше только в `Target()`); кламп `m_attackSpeed` — `if (m_attackSpeed < 500 or > 15000) m_attackSpeed = MakeRandomInt(3000, 8000)`.
- **Данные БД (атрибуты NPC, dgmTypeAttributes)**: `speed`(51)=цикл оружия ms (норма 2500-3500; битые 30000), `maxRange`(54)=оптимальная дистанция (у многих 0), `entityAttackRange`(247)=дальность атаки, `entityFlyRange`(416)=радиус орбиты, `entityAttackDelayMin/Max`(475/476)=задержка первого выстрела (НЕ используется кодом — при желании подключить), `entityChaseMaxDistance`(665)=boostRange, `entityShieldBoostAmount/Duration`(637/636)=ремонт. Guristas Arrogator=2382 (speed 2500, maxRange 500, dmg 0.625), Plunderer=2386 (speed 30000, dmg 0), Dire Mortifier=23307 (speed 30000), Dire Plunderer=23332 (speed 2750, maxRange 11250, dmg 3).
- **Результат юзера (тест 14:15 UTC)**: «Идет бой», «атака идет, скрамблят, орбитят». Орбиты: Sunder/Decimator Drone (мили) ~1.4-1.75км, Atomizer Drone (дальний) 10-15км — по своим оптималам.
- **Запросы к БД**: клиент mariadb в db-контейнере = `/usr/bin/mariadb` (НЕ mysql, нет в PATH). sudo-скрипт: `echo gbnjy78 | sudo -S -p '' -v` (кэш креденшелов) потом `sudo docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu < file.sql`. PowerShell манглит кавычки в plink — писать SQL в файл и передавать base64 (сообщение `base64: invalid input` — НЕ ошибка). entity НЕ имеет solarSystemID — позиции по `locationID=<system>` + x/y/z (устаревшие для движущихся объектов).

## Дроны — диагностика (4 августа)
- **«1 пропадает при запуске»**: лимит дронов = 5 (AttrMaxActiveDrones, DCU нет). Клиент шлёт 6 LaunchDrone → 6-й отклоняется лимитом (SE не создаётся) → «1 потерялся». Это корректное поведение лимита, но клиентский drone-window не синхронизируется с отказом. Задеплоен фикс `f86e7191`: `ShipSE::GetDroneLimit()` (char AttrMaxActiveDrones + online DCU-бонусы) — раньше Drop и LaunchDrone считали лимит по-разному (LaunchDrone не учитывал DCU), из-за чего последний дрон партии отклонялся.
- **«Дроны улетают в дальний космос при scoop / не забрать»**: первый клик scoop РАБОТАЕТ (итем уходит в дрон-бэй, SE удаляются, flight-лист пуст — подтверждено DIAG: itemFlag=87, flightCount=0). Второй клик не находит SE (`Unable to find droneSE`) — дроны уже в бэе. Проблема в клиенте: drone-ball'ы не удаляются/не останавливаются корректно → визуально «улетают в космос», юзер думает что дроны в космосе и жмёт scoop снова. Это клиентский FOLLOW/ORBIT десинхрон. TODO: разобраться с удалением шара дрона при scoop и/или с возвратом дрона.
- **NEW: дроны улетают сами (idle) — НАЙДЕНА ПРИЧИНА (5 авг)**:
  - Запуск → IdleOrbit (ORBIT, usf=0.6) → орбит работает. Через ~6-11 сек клиент шлёт `CmdReturnHome` («Return and Orbit» — это НОРМАЛЬНОЕ поведение клиента при отправке орбиты дронам) → `EntityBound::CmdReturnHome` → `DroneAIMgr::Return()` → `Follow(ship, 0)` + Departing.
  - Дрон в FOLLOW стартует со СТАРОГО тангенса орбиты (не успевает довернуть: `m_degPerTic=(60-agility)/10` давал ~6°/тик даже для agility 0.005) → летит по прямой от корабля. Ускорение FOLLOW (`newSpeed=usf+0.15`, cap 0.8) → 1824 м/с → 210-600 км.
  - **Фиксы (9a59bc49, деплой проверен юзером — «дроны и нпс ожили, орбитят»)**:
    - `Follow()` мгновенно пере-прицеливает heading дрона на цель (нет дрейфа по тангенсу).
    - `m_degPerTic = 60/(agility+1)` вместо `(60-agility)/10` — поворот реально работает.
    - drone chase-speed floor 2000 → 100.
  - maxVelocity Infiltrator II в БД = 2280 (в совр. SDE typeDogma = 2760) — НЕ завышена, дроны реально быстрые.
- **Крэш при отзыве дронов** (SEGV, фикс `60ed19f4`): ScoopDrone → Offline → `AssignShip(nullptr)`, а Departing-хендлер разыменовывал `m_assignedShip->GetPosition()` без null-проверки → NULL deref. Фикс: guard в начале Departing-кейса.
- **Ещё крэш (SIGABRT refcount underflow)** — не воспроизведён повторно; анализ через GDB-режим (`start.sh` теперь `gdb -batch -ex run -ex "bt 40"` при RUN_WITH_GDB=TRUE).
- **Мягкий sync позиций** (`fedd39cb`, деплой ожидает проверки): каждые ~5с шлётся `SetBallPosition` для FOLLOW/ORBIT/GOTO шаров (дрон/NPC/корабль) — клиентский Ballpark плавно lerp'ит, дрейф ограничен метрами. Включён DESTINY__TRACE для логирования sync.
- **Варп: телепорт+стоп в конце** — серверное торможение фикс. 21с, клиентское другое (масса/agility). Сервер заканчивает варп раньше клиента → WarpStop шлёт CmdStop+SetPosition(цель) пока клиент ещё тормозит → рывок. Константы декеля клиента из destiny.dll (OnDeactivatingWarp) извлечены: DAT_10063f20=0.1, DAT_1005f948=1.496e11 (1 AU), DAT_10064028=1.5, DAT_10063fe0=3, DAT_10064018=0.3333, DAT_10064020=-3, DAT_10064088=1.496e12, DAT_10064098=0.7, DAT_100640a0=0.01, DAT_1005f718=1, DAT_1005f710=0.5 (accel tanh), DAT_1005f720=2, DAT_10063f10=1e-5 (мин. скорость).
  - **Сделано (68c3ef81, деплой 10:17 UTC)**: decelDist = `min(mass*0.1*AU, warpSpeed*1.5)/3` (проверено по логу: для 24AU long-warp decelDist=2.24e11 совпал), время = ln(decelDist); холд 5с у выхода (клиентский декел ~10% дольше, gap растёт с дальностью: 334м на 0.5AU, 6км на 24AU); WarpStop НЕ шлёт SetBallPosition (клиент уже на цели, снап = стоп-рывок).
  - **Осталось**: микро-рывок самого стопа (клиентский WARP→STOP переход обнуляет остаточную скорость резко). Тайминги сошлись (торможение успевает), но плавной остановки нет — вероятно, врождённый артефакт клиентского Ballpark без точного совпадения формул (accel клиента — tanh-кривая `(tanh+1)*0.5*warpSpeed`, у сервера exp-accel). Если юзер настаивает — нужно реализовать клиентский tanh-accel + двухфазный декел (OnDeactivatingWarp branch 2 линейный, потом branch 1 exp).
  - **ИТОГ (11:43 UTC, деплой 76f62478)**: двухфазный декел (линейная 3с фаза → exp-затухание, формульный остаток — не накапливаемый), холд 3с у выхода, WarpStop НЕ шлёт CmdStop (клиентский Ballpark сам завершает варп при достижении цели — наблюдалось в накопительной сборке: клиент останавливался плавно без CmdStop). Результат юзера: «Варп работает, немного дёргает на стопе, но именно немного и не каждый раз». Дальнейшая доводка — только точная копия клиентского tanh-accel, высокий риск. Можно считать закрытым, если юзер согласен.
  - ВАЖНО про декел: формульный остаток (не накопление) — иначе кламп скорости 1e-5 застрелял корабль на нескольких сотнях метров → «You are already warping» после каждого варпа.
- Лог-каналы: DRONE__ERROR=1, DRONE__AI_TRACE=1, NPC__AI_TRACE=1, DESTINY__ORBIT_TRACE=1, DESTINY__MOVE_TRACE=1, DESTINY__WARP_TRACE=1 (диагностика дронов/варпа — после фиксов убрать).

## Имена предметов — приведены к клиенту (4 августа)
- **Проблема**: `/create 'Inferno Precision Light Missile'` → «Unable to find valid type to create», хотя итем продаётся на рынке. Причина: серверная `invTypes` содержит старые (Crucible-era) имена (`Flameburst Precision Light Missile`, `Bloodclaw Light Missile`, `Standard Missile Launcher I`), а клиент (SDE новее) показывает современные (`Inferno Precision Light Missile`, `Scourge Light Missile`, `Light Missile Launcher I`). Рынок работает (типы по typeID), но поиск по имени в `/create` падал. До ресета базы эта проблема не была — старая база уже имела client-like имена.
- **Источник клиентских имён**: `C:\Program Files (x86)\CCP\EVE\bulkdata\600004.cache2` — это клиентский кэш CRowset'а `invTypes` (blue.marshal, PyPackedRow). Распарсен скриптом `C:\Users\Dima\AppData\Local\Temp\opencode\dump_cache2.py` → `client_names.tsv` (18710 строк). Также есть современный SDE `C:\opencode-projects\misc\sde\fsd\types.yaml` (153MB, имена `en:`), который совпадает с cache2.
- **Фикс**: сравнены клиентские vs серверные имена → **1405 расхождений** (17272 уже совпадали). Сгенерирован `rename_invtypes.sql` (`UPDATE invTypes SET typeName=...` + `UPDATE entity SET itemName=...`) и выполнен в db-контейнере. Проверка: 2647 = «Inferno Precision Light Missile», 202 = «Mjolnir Cruise Missile», 499 = «Light Missile Launcher I».
- **Несовпавшие**: 33 типа есть только в клиенте (новые), 1041 только в сервере (непубличные/внутренние) — им имена не менялись.
- ВАЖНО: имена в `invTypes` — мастер-данные; при ресете базы (evedbtool install) они откатятся к старым — нужно повторить скрипт.

## Git Log (верх)
```
3ec64963 fix(dungeon): anomaly decorations purely visual — clouds (grp 227/312) + LCO ship Wreckage (grp 226, non-lootable), removed interactive containers/wrecks that rendered as 'gates don't activate, wrecks can't loot, empty containers'
d52f47a2 fix(destiny): orbit 'TooFar' recomputes approach heading every tick — stale-point early return let ships/NPCs fly toward where the target WAS, distance grew 5->30km ('orbited an NPC', drones followed into space = 'teleport across screen'). Same bug as NPCAI::SetChasing stale point (9d4e8f8c).
3103b69a fix(drone): no teleport when drone switches chase→orbit (SetEngaged now syncs SetPosition before CmdOrbit, same as SetApproaching/IdleOrbit — client Ballpark re-anchored ball to new orbit point from desynced position)
f9945956 fix(drone): control-range leash — drone stops chasing targets beyond GetControlRange() (was 2x=40km; NPCs with huge flyRange like Eradicator 27km pulled drones into deep space)
cc1cebb3 fix(ship): stop orbiting invisible decorations — IsTargetInvalid now stops ORBIT/FOLLOW when target has no DestinyMgr (decorations CelestialSE/ItemSystemEntity have m_destiny=nullptr + IsDynamicEntity()==false, so orbit persisted forever around invisible point -> 'circles invisible object', warp align never settled -> 'warp align/speed incorrect' -> stuck WARP -> 'You are already warping')
a43b2bab fix(drone): no teleport when drone returns to carrier after target dies — IdleOrbit now syncs position (SetBallPosition) before CmdOrbit, same as SetApproaching; client Ballpark re-anchored ball to new orbit point from desynced position -> visible teleport at moment target dies
2ebb22df fix(drone+ship): killed drone phantom 'distant space' (ClearController before SetIdle/StateChange so OnDroneStateChange prunes dead drone from client stateByDroneID — live controllerID made client think drone still in space + issue Return commands, yanking other drones) + removed legacy every-50-tic SetBallPosition to pilot's ship (~12.5s jerk in non-ORBIT modes)
5d086488 fix(npc): NPC killed twice — NPC::Killed called RemoveNPC (RemoveEntity + m_self->Delete) and Damage::ApplyDamage then called SystemEntity::Killed -> Delete() again. Double removal corrupted heap; segfault ~35s later on missile removal. NPC::Killed now only unregisters from lists (RemoveNPCFromList); full removal via SystemEntity::Killed. /kill and /killallnpcs chain SystemEntity::Killed.
42d8e954 fix(dungeon): decor/gate entities are static (IsStaticEntity=true) so SendAddBalls never delivered them — AddBallExclusive after spawn
2a8b3921 fix(drone): InventoryItem::Delete no longer junkyard-moves drones (cat 18) — dead drones left ghost entity rows at (0,0,0)
2b54fa15 fix(drone): drop stale target when destroyed structure (no TargetMgr) is removed — drone chased dead target into deep space
40b94991 feat(dungeon): transient decorations (Sansha LCO structures), acceleration gates between multi-room dungeons, cleanup script
e74345ac fix(dungeon): decorations spawn as transient items (SpawnTemp, no SaveItem)
f502d7ce fix: incursion ISK rewards use rewardTypeID=2 (const.rewardTypeISK) not 0
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
143c28e3 fix(orbit): point heading at orbit circle point instead of tangent — orbit no longer unwinds
04ee6348 fix(npc): NPCs aggroed via Targeted() never fired — attack/missile timers only started in Target(); clamp absurdly long AttrSpeed cycles (30000ms) to 3-8s
672afd73 fix(npc): minimum orbit range 1500-5000m when AttrMaxRange is tiny/0 — NPCs no longer clinch the player
670b8635 docs: drone fixes (scoop/orbit/return/messages) + disk-full warning (docker image prune)
29ecf6ce fix: AP warp-to-gate targets a point radius+apWarptoDistance from gate center (warp-to-point)
8b674e50 fix: AP warp-to-gate lands apWarptoDistance from gate surface (add gate radius), capped at 25km
865a5b64 revert: AP warp stop distance back to apWarptoDistance (15000 from gate center)
95fe7f34 fix: AP warp-to-gate lands apWarptoDistance from gate surface (REVERTED)
cc5ffaa2 fix: warp alignment re-arms ship when USF<0.75 regardless of TF
960d6715 fix: re-send benign session change (nextSessionChange) ~5s after gate jump
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
