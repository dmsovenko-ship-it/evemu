<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://img.shields.io/badge/Crucible-1.0-4f9eff?style=for-the-badge&logo=eveonline&logoColor=white"/>
    <img src="https://img.shields.io/badge/Crucible-1.0-1364d2?style=for-the-badge&logo=eveonline&logoColor=white"/>
  </picture>
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://img.shields.io/github/actions/workflow/status/dmsovenko-ship-it/evemu/opencode.yml?style=for-the-badge&label=build&logo=docker"/>
    <img src="https://img.shields.io/github/actions/workflow/status/dmsovenko-ship-it/evemu/opencode.yml?style=for-the-badge&label=build&logo=docker"/>
  </picture>
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://img.shields.io/badge/license-LGPLv3-8b5cf6?style=for-the-badge"/>
    <img src="https://img.shields.io/badge/license-LGPLv3-7c3aed?style-for-the-badge"/>
  </picture>
</p>

<h1 align="center">EVEmu Crucible</h1>

<p align="center">
  <b>EVE Online (Crucible era) server emulator</b> · <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> fork with extended NPC, drone, warp, and infrastructure systems.
</p>

<br>

> **⚠️ Educational project only** — built for studying C++, network protocols, and game server architecture.  
> **Not intended for running public or private gaming servers.**  
> *Только образовательный проект* — создан для изучения C++, сетевых протоколов и архитектуры игровых серверов.  
> *Не предназначен для запуска игровых серверов.*

---

---

## System Architecture

| Component | Stack |
|-----------|-------|
| **Build** | Docker Compose, ccache, MariaDB→MySQL (—400 MB image) |
| **Auth** | CRAM-MC (EVE native) |
| **Client** | EVE Online Crucible 1.x |

---

## Core Systems

### 🚀 Warp & Movement
> Client-server desync, velocity resets, autopilot chains

- **Desync fix** — ship arrives at `targetPoint` **before** sending GOTO update; client no longer shows 473 km after landing
- **NaN velocity guard** — `Halt()` after warp terminates rogue speed
- **Autopilot chain** — state preserved across gate jumps; `Follow` resumes for next gate approach
- **Gate warp** — 5 km offset spawn; 12 s align timeout
- **Velocity reset** — full stop on session change / wake-up

### 🧩 Drone System
> Full drone lifecycle: AI, skills, combat, reload, control range

- **AI states** — `Idle` → `Approach` → `Engage` → `Return` → cycle. Chases out-of-range targets, orbits at weapon range
- **Skill tree** — Navigation, Sharpshooting, racial specialisation (Caldari/Minmatar/Amarr/Gallente), Interfacing (×10% damage/level), Durability, Scout Drone Operation
- **Subtypes** — Combat, EWAR (ECM/web/scramble), Logistics (shield/armor), Cap Drain, Mining, Fighter, Fighter Bomber
- **Fighters & Bombers** — Ammo consumption (10 / 5 shots), auto-return on empty, **reload + re-engage** last target via `SetApproaching`; uses **cruise speed** (not max) for orbit, matching NPC AI pattern
- **Control range** — Computed from ship attribute + **Scout Drone Operation** (+5 km/lvl) + **Electronic Warfare Drone Interfacing** (+3 km/lvl); skill bonuses added explicitly (dogma/Fx bypass)
- **DCU fix** — Drone Control Unit module bonus summed at launch check (bypasses broken dogma → character channel)
- **Defensive** — Null-guard kill path (`m_destiny`, `m_bubble`, `m_self`); `ClearFromTargets()` before `SafeDelete(m_targMgr)` in `Killed()`; no `ClearTarget()` after `ApplyDamage()` to prevent use-after-free
- **Scoop fix** — Orphaned drones removed via `RemoveDroneFromFlight`; ghost drone elimination

### 🛡️ CONCORD & Security

| Feature | Detail |
|---------|--------|
| **CONCORD** | Police Battleships (×25 HP), delay by sec rating, −0.2 penalty, killmail |
| **Sentry Guns** | Corp inherited from station, standing-based in nullsec (< −2.0), CrimeWatch in high/low |
| **CrimeWatch** | Weapon (60 s), aggression (15 min), criminal (15 min highsec), suspect on loot |
| **Outlaw** | SS ≤ −5.0 — no dock/jump, sentries engage |
| **Kill Rights** | Grant on criminal aggression / pod kill, 30-day expiry, auto-activate → Limited Engagement |
| **Contraband** | Gate scan (highsec), per-stack detection × Smuggling skill, 60s jettison timer, standing loss + fine + confiscation |
| **Customs Police** | Faction-specific NPC (group 446) spawned on timer expiry, auto-engages contraband runner (not instant kill — faction police, not CONCORD) |

### 🧬 Clone & Jump Clone

| Feature | Detail |
|---------|--------|
| **Jump Clones** | Install, destroy, clone jump — all 10 `JumpCloneBound` methods implemented |
| **Per-clone implants** | `chrJumpCloneImplants` table — each clone has its own implant set |
| **Active clone** | `entity.isActive` flag tracks which clone is active (respawn point) |
| **Clone pricing** | Alpha=free, Beta=100k, Gamma=500k, Delta=5M, Epsilon=50M, Zeta=100M ISK |
| **Clone jump** | `SetCloneActive` toggles active clone instead of moving all clones |

### 🤖 NPC Systems

| System | Detail |
|--------|--------|
| **Convoys** | Guard + hauler between stations (sec 0.5–0.7), phased movement, wake-up on attack, sentry defense |
| **Killmails** | XML blob (dropped + destroyed), push via mail + combat log |
| **Agent Missions** | Distribution, kill, courier — with skill/standing requirements |
| **Dungeons** | Cosmic anomalies, combat sites, gate spawns, JSON-defined rooms |
| **Encounter Spawner** | `encounterSpawnServer` service spawns pirate NPCs (Guristas/Angel/Serpentis/Blood/Sansha/Rogue Drone) on activation |
| **Storyline Missions** | 144 missions across all levels — Courier + Encounter types |
| **Epic Arc** | The Blood-Stained Stars (SoE) — 58 missions, 7 chapters, branching, standing reward |

### 📬 Infrastructure

- **Mail** — Dual-write `mailMessage`+`mailStatus` (visible in client) + legacy `eveMail`; Deflate compression
- **LiveUpdate** — News ticker shows latest commit instead of SSL error
- **Kill Rights DB** — `srvKillRights` table with mask, status, timestamps

---

## Quick Start

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git
cd evemu
docker compose up -d --build
docker logs -f server          # wait for "Server started"
```

| Command | Purpose |
|---------|---------|
| `docker compose stop` | Stop server |
| `docker compose up -d` | Start |
| `docker compose down -v` | Full DB reset |
| `bash utils/grant-admin.sh "Name"` | Grant GM |

| GM Command | Purpose |
|------------|---------|
| `/giveallskills me` | Max skills |
| `/spawn <typeID>` | Spawn NPC |
| `/online me` | Online modules |

Full reference: [doc/admin_reference.md](doc/admin_reference.md)

---

## Known Issues

- `Destiny::MoveObject()` log noise — upstream, harmless
- `MailService::SendMail()` — delivery incomplete
- Killmail push notifications — not implemented
- Fighter/bomber aggro — only on last target or when fired upon (no idle scan)
- **Same-system beacon jump** — cyno/covert-cyno field in the same system as the jumping ship does not appear in the fleet right-click jump menu (inter-system hyperjump works). Client-side limitation in `menusvc.py` / fleet service beacon filtering.
- **Customs NPC AI** — spawned customs police NPC targets but may not use faction-specific EWAR/weapon attributes from SDE (uses default NPC combat fallbacks).
- **NPC rendering** — Entity-category NPC models override categoryID to Ship in slim items; graphics render correctly but some ship groupID mappings may affect UI behavior (right-click menu, info window).
- **Destiny crash with Entity NPCs** — some Entity-category NPCs may cause `Unknown packet type` if warp-in is triggered with invalid speed (mitigated: warp-in disabled for non-Ship category).

---

## Changelog

| Area | Change |
|------|--------|
| **Warp** | Desync fix (position before GOTO), NaN halt, autopilot chain, gate align timeout, velocity reset |
| **Drones** | Re-engage after reload, cruise orbit speed, control range with skill bonuses, DCU fix, null-guard kill path, scoop cleanup, full skill tree, EWAR/logistics/mining subtypes |
| **CONCORD** | Police ×25 HP, delay by sec, −0.2 penalty |
| **Sentry** | Corp inheritance, nullsec standing check |
| **CrimeWatch** | Timers, outlaw, suspect, criminal, CanDock/CanJump, combat logoff 15min, emergency warp 60s |
| **Warp Scramble** | Player modules, NPC AI scram, emergency warp block, CmdSafeLogoff check, scramble cleanup on NPC/drone target clear |
| **Kill Rights** | Grant, auto-activation, Limited Engagement |
| **Convoys** | Phased movement, sentry defense |
| **Killmails** | XML blob, push notification |
| **Mail** | Dual-write, Deflate |
| **Fleet** | WarpToMember, WarpFleetToMember, FleetRegroup, Specialist skills, MiningDirector, Gang Coordinator modules. Fixed null ShipSE crash in ApplyBoost on undock |
| **Image Server** | Auto-resolve URL, imageServerURL config, portrait gender fix |
| **Cyno** | Fixed jammer check (verify entity exists), fixed security check (inverted m_secValue → `>= 0.5`), fixed fleet requirement bypass |
| **Covert Cyno** | `CovertCynoModule` — no fleet required, high-sec allowed, creates CovertCynosuralFieldI (not regular). Can activate while cloaked |
| **Covert Cloak** | `cpuNeed=0` migration bypasses client cache; `Cloak()`/`UnCloak()` via `DestinyMgr`; full activation/deactivation cycle with `ShowEffect` |
| **Titan Bridge** | `JumpPortalModule` — portal effect, fleet/alliance jump, `CmdJumpThroughFleet`/`CmdJumpThroughAlliance`/`CmdBridgeToMember` |
| **Jump Drive** | Range check with JDC skill (+25%/lvl Crucible), distance-based fuel, minimum fuel 1, quantityLeft consumption |
| **Jump Bridge** | Enhanced fuel calc (distance-based), range check on bridge jumps |
| **Portal broadcast** | `SetOnline()` now broadcasts `OnMultiEvent` to all clients in bubble — fleet members see module state changes |
| **Customs Scan** | `ContrabandScan()` on gate jump — highsec only, system-wide broadcast, 60s timer, faction standing loss, item confiscation, ISK fine |
| **Customs Police** | `SpawnCustomsPolice()` spawns faction-specific NPC (group 446 Customs_Official) near player. NPCAI auto-engages contraband runners |
| **Epic Arc (SoE)** | The Blood-Stained Stars — 58 `agtMissions` (80000-80057), 7 chapters, branching, +0.7 faction standing final reward |
| **Epic Arc Mgr** | `EpicArcMgr` singleton — arc/chapter/mission data, 90-day cooldown, `GetMyEpicJournalDetails` returns real state |
| **Agent dialog** | `DoAction(EpicArcStart)` — epic arc button for arc agents, mission chain progression on Complete |
| **Storyline Missions** | 144 missions via `qstEncounter` table + `qstCourier` typeID=8; encounter/courier storylines |
| **Encounter Server** | `encounterSpawnServer` registered — `RequestActivateEncounters` spawns pirate NPCs with NPCAI; `RequestDeactivateEncounters` cleans up |
| **MissionDataMgr** | `m_encounter`/`m_storyline` maps loaded at startup; `CreateMissionOffer` handles Storyline type |
| **Implant/Booster effects** | `Character::ProcessEffects()` now processes `flagImplant` and `flagBooster` items |
| **Warp speed bonuses** | `AttrWarpSpeedBonus` (601) applied as multiplier; `WarpDriveOperation` removed from warp speed (Crucible-accurate) |
| **Charge compatibility** | `IsChargeCompatible()` fallback when SDE `chargeGroup1-5` missing — supports T2 ammo for all weapon types |
| **Build** | Docker Compose, ccache, 400 MB reduction. Added CovertCynoModule + JumpPortalModule + CustomsNPCManager + EpicArcMgr + EncounterServer to CMakeLists |
| **Jump Clones** | All 10 `JumpCloneBound` methods implemented: `GetCloneState`, `GetStationCloneState`, `GetShipCloneState`, `GetPriceForClone` (real prices), `InstallCloneInStation`, `DestroyInstalledClone`, `CloneJump` (uses `SetCloneActive` instead of moving all clones). Per-clone implants via `chrJumpCloneImplants` table. Active clone tracking via `entity.isActive` |
| **Anomaly NPCs** | Switched from custom typeIDs (33001-33103) to SDE Entity pirate typeIDs. `NPC::MakeSlimItem` overrides `categoryID=6` (Ship) and maps Entity groups to Ship groups (25/26/27/419). `MakeDungeon` handles Entity catID(11) alongside Ship(6)/Drone(18). Fixed warp-in for Entity NPCs. GroupID-based faction fallback for ownerID |
| **Corp Market** | Enabled — removed blocking error. Office check, NPC corp check, wallet division permissions. `CancelCharOrder` handles corp orders (escrow to corp wallet, items to `flagCorpMarket`) |
| **Contracts** | `GetMyExpiredContractList`, `NumOutstandingContracts`, `GetLoginInfo` now query real DB data instead of returning empty/null |
| **Incursions** | Base system: `IncursionService` registered, `IncursionMgr` singleton with 60s timer. State machine (established→mobilized→withdrawal). Influence tracking (site completion reduces, +1%/20min regen). Mothership spawn on 0% influence. DoSpawnForIncursion spawns Sansha NPCs. Rewards seeded (VG=10.4M / AS=18.2M / HQ=31.5M / MS=63M ISK + LP). DB tables: `incursions`, `incursionSystems`, `incursionRewards` |
| **Module Management** | Fixed `RepairModule()` inverted nullptr check. `ModuleRepair()` now checks actual module damage. `StopModuleRepair()` logs. `CharacterLeavingShip()` calls `OfflineAll()` |
| **War Registry** | `rates.warCost` config option (eve-server.xml), `GetWars` returns historical wars |
| **Destiny** | Null-check in `FlushPendingDestinyUpdates` prevents crash from null update |

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="https://github.com/EvEmu-Project/evemu_Crucible/pull/327">PR #327</a>
</p>

<p align="center"><b>LGPL v3</b> — educational project. Not for public servers.</p>
<p align="center">
  📊 <a href="PROGRESS.md">Project Status</a> — detailed feature breakdown across all systems
</p>

---

<br>

<h1 align="center">EVEmu Crucible</h1>

<p align="center">
  <b>Эмулятор сервера EVE Online (Crucible)</b> · форк <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> с расширенными NPC, дронами, варпом и инфраструктурой.
</p>

---

## Системная архитектура

| Компонент | Стек |
|-----------|------|
| **Сборка** | Docker Compose, ccache, MariaDB→MySQL (−400 МБ) |
| **Авторизация** | CRAM-MC (родной EVE) |
| **Клиент** | EVE Online Crucible 1.x |

---

## Ключевые системы

### 🚀 Варп и движение
> Десинх клиент-сервер, сброс скорости, цепочки автопилота

- **Десинх** — корабль фиксирует позицию в `targetPoint` **до** отправки GOTO; клиент больше не видит 473 км после прилёта
- **NaN velocity** — `Halt()` после варпа гасит невалидную скорость
- **Автопилот** — состояние сохраняется после прыжка через гейт; `Follow` возобновляется для подлёта
- **Варп к гейту** — спавн в 5 км; таймаут выравнивания 12 с
- **Сброс скорости** — полная остановка при смене сессии / пробуждении

### 🧩 Система дронов
> Полный цикл: AI, навыки, бой, перезарядка, дальность управления

- **Состояния AI** — `Idle` → `Approach` → `Engage` → `Return` → цикл. Преследует цели за пределами дальности, выходит на орбиту в зоне поражения
- **Навыки** — Drone Navigation, Sharpshooting, расовая специализация, Interfacing (×10%/лвл), Durability, Scout Drone Operation
- **Подтипы** — Combat, EWAR (ECM/web/scramble), Logistics (щит/броня), Cap Drain, Mining, Fighter, Fighter Bomber
- **Файтеры и бомберы** — Расход патронов (10 / 5 выстрелов), авто-возврат при пустом магазине, **перезарядка + атака последней цели** через `SetApproaching`; орбита на **крейсерской** скорости (не максимальной)
- **Дальность управления** — Атрибут корабля + **Scout Drone Operation** (+5 км/лвл) + **Electronic Warfare Drone Interfacing** (+3 км/лвл); бонусы навыков добавлены вручную (догма-система не применяет)
- **DCU** — Бонус модуля Drone Control Unit суммируется явно при проверке лимита дронов
- **Защита** — Нулл-гарды в `Killed()`, `AwardSecurityStatus()`, `ApplyDamage()`; `ClearFromTargets()` до `SafeDelete(m_targMgr)`; убран `ClearTarget()` после `ApplyDamage()`
- **Скуп** — Мёртвые дроны удаляются через `RemoveDroneFromFlight`

### 🛡️ CONCORD и безопасность

| Система | Детали |
|---------|--------|
| **CONCORD** | Police Battleships (×25 HP), задержка по sec, штраф −0.2 |
| **Sentry Guns** | Корпорация от станции, рейтинг в нулях (< −2.0), CrimeWatch |
| **CrimeWatch** | Weapon (60 с), aggression (15 мин), criminal (15 мин хайсек), suspect на луте |
| **Outlaw** | SS ≤ −5.0 — нет дока/джампа, сентри атакуют |
| **Kill Rights** | Выдача при агрессии / подкилле, 30 дней, авто → Limited Engagement |
| **Контрабанда** | Скан на гейте (хайсек), шанс обнаружения × Smuggling, 60s на выброс груза, штраф + конфискация + стояние |
| **Таможня** | NPC фракции (группа 446) спавнится по таймеру, атакует нарушителя (не CONCORD — обычный NPC) |

### 🧬 Клоны и Jump Clones

| Система | Детали |
|---------|--------|
| **Jump Clones** | Все 10 методов: установка, удаление, прыжок между клонами |
| **Импланты на клон** | Таблица `chrJumpCloneImplants` — у каждого клона свой набор |
| **Активный клон** | `entity.isActive` — точка респавна |
| **Цены** | Alpha=0, Beta=100k, Gamma=500k, Delta=5M, Epsilon=50M, Zeta=100M |
| **Прыжок** | `SetCloneActive` переключает активный клон, не двигая все |

### 🤖 NPC системы

| Система | Детали |
|---------|--------|
| **Конвои** | Охрана + грузовоз между станциями, фазы, пробуждение при атаке, защита сентри |
| **Киллимейлы** | XML (дроп + уничтожено), push в почту + combat log |
| **Миссии** | Distribution, kill, courier — скиллы/рейтинг |
| **Данжи** | Аномалии, combat sites, спавн у гейтов, JSON-комнаты |
| **Encounter Server** | Сервис `encounterSpawnServer` — спавн пиратских NPC (Guristas/Angel/Serpentis/Blood/Sansha/Rogue Drone) при активации |
| **Storyline миссии** | 144 миссии всех уровней — Courier + Encounter типы |
| **Epic Arc (SoE)** | The Blood-Stained Stars — 58 миссий, 7 глав, бранчинг, награда +0.7 faction standing |

### 📬 Инфраструктура

- **Почта** — Двойная запись `mailMessage`+`mailStatus` (видна клиенту) + `eveMail`; Deflate
- **LiveUpdate** — Новостная строка с последним коммитом вместо SSL ошибки
- **Kill Rights** — Таблица `srvKillRights` (маска, статус, таймеры)

---

## Быстрый старт

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git
cd evemu
docker compose up -d --build
docker logs -f server          # ждать "Server started"
```

| Команда | Описание |
|---------|----------|
| `docker compose stop` | Остановить |
| `docker compose up -d` | Запустить |
| `docker compose down -v` | Сброс БД |
| `bash utils/grant-admin.sh "Имя"` | Права GM |

| Команда GM | Описание |
|------------|----------|
| `/giveallskills me` | Навыки на 5 |
| `/spawn <typeID>` | Спавн NPC |
| `/online me` | Модули онлайн |

Полный список: [doc/admin_reference.md](doc/admin_reference.md)

---

## Известные проблемы

- `Destiny::MoveObject()` — upstream, безвредно
- `MailService::SendMail()` — неполная доставка
- Push-уведомления киллимейлов — не реализованы
- Аггро файтеров — только на последнюю цель или при атаке (нет сканирования в Idle)
- **Прыжок к маяку в той же системе** — цино/коверт-цино в одной системе с кораблём не появляется в меню прыжка флота (межсистемный гиперпрыжок работает). Ограничение на стороне клиента — `menusvc.py` / fleet service.
- **AI таможни** — спавн NPC работает, но атрибуты оружия/EWAR могут быть дефолтными (не из SDE) — использует стандартные NPC-заглушки.
- **Рендеринг NPC** — Entity-категория NPC рендерится с categoryID=6 (Ship); модели отображаются корректно, но UI может неверно определять группу корабля.
- **Destiny краш** — Entity NPC могут вызывать `Unknown packet type` при варп-ине с невалидной скоростью (фикс: варп-ин отключён для non-Ship категорий).

---

## Список изменений

| Область | Изменение |
|---------|-----------|
| **Варп** | Фикс десинхра (позиция до GOTO), NaN halt, цепочка автопилота, таймаут выравнивания, сброс скорости |
| **Дроны** | Ре-энгадж после перезарядки, крейсерская орбита, дальность с бонусами навыков, DCU, нулл-гарды, подтипы AI |
| **CONCORD** | Police ×25 HP, задержка по sec |
| **Сентри** | Корпорация, рейтинг в нулях |
| **CrimeWatch** | Таймеры, outlaw, suspect, criminal, CanDock/CanJump, combat logoff 15min, emergency warp 60s |
| **Kill Rights** | Выдача, Limited Engagement |
| **Конвои** | Фазы, защита сентри |
| **Киллимейлы** | XML, push |
| **Почта** | Двойная запись, Deflate |
| **Флот** | WarpToMember, WarpFleetToMember, FleetRegroup, Specialist скиллы, MiningDirector, Gang Coordinator. Пофикшен краш ApplyBoost при анлоке во флоте |
| **Изображения** | ImageServer URL авторезолв, imageServerURL конфиг, portrait gender в cacheOwners |
| **Варп скрамбл** | Модули игрока, NPC AI, блокировка emergency warp, CmdSafeLogoff, очистка статуса |
| **Цино** | Пофикшен джаммер, секьюрити (`>= 0.5` вместо `<= 0.6`), теперь блокирует только хайсек |
| **Коверт цино** | CovertCynoModule — без флота, хайсек, CovertCynosuralFieldI. Можно активировать под клакой |
| **Коверт клака** | `cpuNeed=0` в БД (клиентский кэш); `Cloak()`/`UnCloak()` через `DestinyMgr`; полный цикл с `ShowEffect` |
| **Титан бридж** | JumpPortalModule — портал, прыжки флота/альянса, CmdBridgeToMember |
| **Портал бридж** | `OpenBridge()` по команде флот-мембера; `SetOnline()` шлёт broadcast всем в бабле |
| **Jump Drive** | Range check с JDC (+25%/lvl), минимальное топливо 1, quantityLeft |
| **Таможня** | `ContrabandScan()` на прыжке — хайсек, глобальное уведомление, 60s таймер, штраф стояния, конфискация, штраф ISK |
| **Полиция фракции** | `SpawnCustomsNPCs()` — NPC таможни (группа 446) у гейтов в хайсеке; орбита 8-12 км |
| **Epic Arc (SoE)** | The Blood-Stained Stars — 58 `agtMissions` (80000-80057), 7 глав, бранчинг, финальная награда +0.7 standing |
| **Epic Arc Mgr** | Singleton — загрузка арки/глав/миссий, 90-day кулдаун, `GetMyEpicJournalDetails` |
| **Агентский диалог** | `DoAction(EpicArcStart)` — кнопка эпик арки, цепочка миссий, награда при Complete |
| **Storyline миссии** | 144 миссии через `qstEncounter` + `qstCourier` typeID=8 |
| **Encounter Server** | `encounterSpawnServer` — `RequestActivateEncounters` спавнит NPC; `RequestDeactivateEncounters` убирает |
| **MissionDataMgr** | `m_encounter`/`m_storyline` карты; `CreateMissionOffer` обрабатывает Storyline тип |
| **Импланты/бустеры** | `Character::ProcessEffects()` обрабатывает `flagImplant` и `flagBooster` |
| **Варп скорость** | `AttrWarpSpeedBonus` (601) как мультипликатор; WarpDriveOperation убран со скорости (как в Crucible) |
| **Заряды T2** | `IsChargeCompatible()` fallback при отсутствии `chargeGroup1-5` в SDE — T2 аммо для всех типов оружия |
| **Сборка** | Docker Compose, ccache, −400 МБ. Добавлены новые файлы в CMakeLists |
| **Jump Clones** | Все 10 методов `JumpCloneBound` реализованы: `GetCloneState`, `GetStationCloneState`, `GetShipCloneState`, `GetPriceForClone` (реальные цены), `InstallCloneInStation`, `DestroyInstalledClone`, `CloneJump` (через `SetCloneActive`). Импланты на клон через таблицу `chrJumpCloneImplants`. Флаг `entity.isActive` для отслеживания активного клона |
| **Аномалии NPC** | Переход с кастомных typeID (33001-33103) на SDE Entity пиратские typeID. `NPC::MakeSlimItem` шлёт `categoryID=6` (Ship). `MakeDungeon` обрабатывает Entity catID(11). Отключён варп-ин для Entity NPC. OwnerID через groupID→faction fallback |
| **Корп маркет** | Включён — убрана заглушка. Проверка офиса корпы, wallet division permissions. `CancelCharOrder` для корп-ордеров |
| **Контракты** | `GetMyExpiredContractList`, `NumOutstandingContracts`, `GetLoginInfo` — реальные данные из БД |
| **Инкурсии** | Базовая система: `IncursionService`, `IncursionMgr` с 60s таймером. Стейт-машина (established→mobilized→withdrawal). Влияние (сайты ↓, реген +1%/20мин). Мазершип при 0% влияния. `DoSpawnForIncursion` спавнит Sansha. Реварды (VG=10.4M / AS=18.2M / HQ=31.5M / MS=63M ISK + LP). Таблицы: `incursions`, `incursionSystems`, `incursionRewards` |
| **Модули** | Пофикшен `RepairModule()` (инвертированный nullptr check). `ModuleRepair()` проверяет повреждения. `CharacterLeavingShip()` вызывает `OfflineAll()` |
| **Войны** | `rates.warCost` в eve-server.xml. `GetWars` возвращает исторические войны |
| **Destiny** | Null-check в `FlushPendingDestinyUpdates` предотвращает краш |

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="https://github.com/EvEmu-Project/evemu_Crucible/pull/327">PR #327</a>
</p>

<p align="center"><b>LGPL v3</b> — образовательный проект. Не предназначен для публичных серверов.</p>
<p align="center">
  📊 <a href="PROGRESS.md">Состояние проекта</a> — детальный разбор фич по всем системам
</p>
