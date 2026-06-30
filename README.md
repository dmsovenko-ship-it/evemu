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
    <img src="https://img.shields.io/badge/license-LGPLv3-7c3aed?style=for-the-badge"/>
  </picture>
</p>

<h1 align="center">EVEmu Crucible</h1>

<p align="center">
  <b>EVE Online (Crucible era) server emulator</b> · <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> fork with extended NPC, drone, warp, and infrastructure systems.
</p>

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

### 🤖 NPC Systems

| System | Detail |
|--------|--------|
| **Convoys** | Guard + hauler between stations (sec 0.5–0.7), phased movement, wake-up on attack, sentry defense |
| **Killmails** | XML blob (dropped + destroyed), push via mail + combat log |
| **Agent Missions** | Distribution, kill, courier — with skill/standing requirements |
| **Dungeons** | Cosmic anomalies, combat sites, gate spawns |

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

---

## Changelog

| Area | Change |
|------|--------|
| **Warp** | Desync fix (position before GOTO), NaN halt, autopilot chain, gate align timeout, velocity reset |
| **Drones** | Re-engage after reload, cruise orbit speed, control range with skill bonuses, DCU fix, null-guard kill path, scoop cleanup, full skill tree, EWAR/logistics/mining subtypes |
| **CONCORD** | Police ×25 HP, delay by sec, −0.2 penalty |
| **Sentry** | Corp inheritance, nullsec standing check |
| **CrimeWatch** | Timers, outlaw, suspect, criminal |
| **Kill Rights** | Grant, auto-activation, Limited Engagement |
| **Convoys** | Phased movement, sentry defense |
| **Killmails** | XML blob, push notification |
| **Mail** | Dual-write, Deflate |
| **Build** | Docker Compose, ccache, 400 MB reduction |

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="https://github.com/EvEmu-Project/evemu_Crucible/pull/327">PR #327</a>
</p>

<p align="center"><b>LGPL v3</b> — educational project. Not for public servers.</p>

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

### 🤖 NPC системы

| Система | Детали |
|---------|--------|
| **Конвои** | Охрана + грузовоз между станциями, фазы, пробуждение при атаке, защита сентри |
| **Киллимейлы** | XML (дроп + уничтожено), push в почту + combat log |
| **Миссии** | Distribution, kill, courier — скиллы/рейтинг |
| **Данжи** | Аномалии, combat sites, спавн у гейтов |

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

---

## Список изменений

| Область | Изменение |
|---------|-----------|
| **Варп** | Фикс десинхра (позиция до GOTO), NaN halt, цепочка автопилота, таймаут выравнивания, сброс скорости |
| **Дроны** | Ре-энгадж после перезарядки, крейсерская орбита, дальность с бонусами навыков, DCU, нулл-гарды, подтипы AI |
| **CONCORD** | Police ×25 HP, задержка по sec |
| **Сентри** | Корпорация, рейтинг в нулях |
| **CrimeWatch** | Таймеры, outlaw, suspect, criminal |
| **Kill Rights** | Выдача, Limited Engagement |
| **Конвои** | Фазы, защита сентри |
| **Киллимейлы** | XML, push |
| **Почта** | Двойная запись, Deflate |
| **Сборка** | Docker Compose, ccache, −400 МБ |

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="https://github.com/EvEmu-Project/evemu_Crucible/pull/327">PR #327</a>
</p>

<p align="center"><b>LGPL v3</b> — образовательный проект. Не предназначен для публичных серверов.</p>
