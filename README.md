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
  <b>EVE Online (Crucible era) server emulator</b> · <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> fork — ~99% complete
</p>

<br>

> **⚠️ Educational project only** — for studying C++, network protocols, game server architecture.  
> *Только образовательный проект* — для изучения C++, сетевых протоколов и архитектуры игровых серверов.

---

## Features / Возможности

| EN | RU |
|----|----|
| **Warp & Movement** — smooth warp-to-0, fleet warp, **autopilot (auto-jump, multi-hop)**, **early warp start (<30°+half align)**, **snap stop (no drift)** | **Варп и движение** — плавный варп-ту-0, флот-варп, **автопилот (авто-прыжок, мультихоп)** |
| **Drones** — full AI (combat/EWAR/logistics/mining), skills, subtypes, control range, UserError messages, Pursuit/Fleeing AI, Drone Control Unit bonus | **Дроны** — полный AI (бой/EWAR/логистика/майнинг), навыки, подтипы, UserError-сообщения, Pursuit/Fleeing AI, Drone Control Unit |
| **NPC systems** — anomalies, incursions, belt rats, gate rats, convoys, customs police, module fitting system, per-weapon effects GUIDs | **NPC системы** — аномалии, инкурсии, бельтраты, гейтраты, конвои, таможня, модульная система фита, GUID эффектов по типу оружия |
| **Crimewatch** — weapon/aggression/criminal timers, CONCORD, sentry guns, kill rights, **probe aggression (15min)** | **Crimewatch** — таймеры, CONCORD, сентри, киллрайты, **агрессия пробок (15мин)** |
| **Warp Disruption Probes** — Interdiction Sphere Launcher, bubble 20km, smartbomb destruction, **scramble cleanup on range exit** | **Пробки варп-дисрапта** — лаунчер, баббл 20км, уничтожение смартбомбами, **очистка скрембла при выходе из радиуса** |
| **Mobile Warp Disruptor** — anchor/online via DogmaIM, SDE timers per type, **WarpDisruptFieldGenerating** visual, StructureOnlined effect, **transient (deleted on restart)** | **MWD** — anchor/online через DogmaIM, таймеры из SDE по типу, **WarpDisruptFieldGenerating** визуал, StructureOnlined эффект, **транзиент (удаляется при ребуте)** |
| **Clones & Implants** — jump clones, per-clone implants, ship clone bay, SP loss on T3 pod | **Клоны и импланты** — джамп-клоны, импланты на клон, шип-клон-бей, SP loss |
| **Contracts** — item exchange, courier, auctions with bidding + ISK transfer | **Контракты** — обмен, курьер, аукционы со ставками и переводом ISK |
| **Corporation & Alliance** — corp/ally contacts with **role checks**, **OnContactLoggedOn/Off**, PyFloat* standing | **Корпорации и альянсы** — контакты с **проверкой ролей**, **OnContactLoggedOn/Off**, PyFloat* standing |
| **Market** — buy/sell orders, corp market, **market bot spin-lock fix (1000→1)** | **Маркет** — ордера, корп-маркет, **фикс spin-lock бота (1000→1)** |
| **Science & Industry** — manufacturing, copying, invention, reverse engineering | **Наука и промышленность** — производство, копирование, инвеншен, РЕ |
| **POS** — towers, fuel/reinforced, CPU/PG, weapon AI, orbitals, reactors, skill checks | **POS** — тауэры, топливо/reinforced, CPU/PG, оружие AI, орбиталки, реакторы, контроль, скиллы |
| **Overheating** — heat dmg per slot, OverloadRack, Thermodynamics, Nanite Paste | **Перегрев** — урон по слотам, OverloadRack, Thermodynamics, Nanite Paste |
| **Notifications** — persistent DB + live push, bill/tower/agent/corp sources | **Нотификации** — БД + live push, счета/POS/агенты/корп |
| **LSC Chat** — private conversations, channels, mailing lists, contact online notifications | **LSC Чат** — разговоры, каналы, списки рассылки, нотификации онлайна контактов |
| **Faction Warfare** — join/leave, plex spawn, militia stats, corp/alliance | **ФВ** — вступление/выход, плексы, статистика, корп/альянс |
| **Planetary Interaction** — colonies, customs offices, resource extraction | **Планетарка** — колонии, таможня, добыча |
| **Sovereignty** — TCU claim, IHub reinforcement, outpost capture, sov levels, upgrades | **Суверенность** — TCU захват, IHub reinforce, аутпосты, уровни, апгрейды |
| **Wormholes** — full lifecycle, mass/lifetime tracking, K162 generation | **Варпхолы** — полный цикл, масса/время жизни, K162 |
| **Missions** — courier, mining, encounter, storyline, epic arcs (Blood-Stained Stars) | **Миссии** — курьер, майнинг, encounter, storyline, эпик арки |
| **PvE Expeditions** — escalation chains (3/10→10/10), faction-specific DED sites, Journal tracking | **Экспедиции** — эскалации, фракционные DED-сайты, трекинг в журнале |
| **W-space / Sleepers** — SleeperAI (remote rep, energy neut, capital escalation), combat sites by WH class | **W-space / Слиперы** — SleeperAI, боевые сайты по классу ВХ |
| **Scanning** — probes, cosmic signatures, anomalies, directional scan | **Сканирование** — пробы, сигнатуры, аномалии, D-scan |
| **Fleet** — fleet warp, boosts, warfare links, specialist skills | **Флот** — флот-варп, бусты, warfare-линки, скиллы |
| **Incursions** — state machine, wave spawning, contest rewards | **Инкурсии** — стейт-машина, волновой спавн, contest награды |
| **GM commands** — spawn, dogma, giveallskills, kick, ban, teleport | **GM команды** — спавн, догма, скиллы, кик, бан, телепорт |
| **Orbit** — smooth circular motion (no 40km snap), **Approach** — no oscillation, **snap stop (no drift)** | **Орбита** — плавное движение (без 40км скачка), **Approach** — без осцилляции, **мгновенная остановка без дрифта** |
| **Bubble hopping fix** — player/NPC bubble stability, empty bubble cleanup 5s | **Bubble hopping fix** — стабильность бабблов, очистка пустых за 5с |
| **Jump cloak** — 60s cloak works, enemies don't see you | **Клок прыжка** — 60с клок работает, враг не видит |
| **Missile fix** — use-after-free guard on target | **Ракеты** — защита use-after-free цели |

---

## Quick Start / Быстрый старт

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git
cd evemu
docker compose up -d --build
docker logs -f server
```

| Command | Purpose |
|---------|---------|
| `docker compose stop` | Stop server |
| `docker compose up -d` | Start |
| `docker compose down -v` | Full DB reset |
| `bash utils/grant-admin.sh "Name"` | Grant GM |

| GM Command | Purpose |
|------------|---------|
| `/giveallskills me` | Max all skills |
| `/spawn <typeID>` | Spawn item/NPC |
| `/online me` | Online all modules |
| `/dogma me agility = 0.5` | Set attribute |
| `/dogma me list` | List attributes |
| `.tr <locationID>` | Teleport |

---

## Changelog / Изменения

See [`current_state_summary.md`](current_state_summary.md) for full session-by-session progress.  
Подробно по сессиям — в [`current_state_summary.md`](current_state_summary.md).

---

## Progress / Прогресс

**Our fork: ~99%** · **Upstream: ~59.5%**

| System | Upstream | Our Fork | Δ | System | Upstream | Our Fork | Δ |
|--------|:--------:|:--------:|:-:|--------|:--------:|:--------:|:-:|
| Account & Character | 95% | 97% | +2% | Skills & Certificates | 90% | 99% | +9% |
| Ship Navigation | 70% | **99%** | +29% | Combat | 90% | 99% | +9% |
| Modules & Overheating | 85% | 97% | +12% | Drones | 75% | **96%** | +21% |
| NPC AI & Spawning | 60% | **97%** | +37% | Agents & Missions | 70% | 95% | +25% |
| **POS** | 70% | **97%** | +27% | Market | 60% | 95% | +35% |
| **Incursions** | 0% | **93%** | +93% | Fleet | 75% | **100%** | +25% |
| **Wormholes** | 60% | **92%** | +32% | Scanning | 80% | 99% | +19% |
| **Notifications** | 60% | **97%** | +37% | **Standings** | 60% | 92% | +32% |
| **Faction Warfare** | 50% | **99%** | +49% | Calendar | 60% | 93% | +33% |
| Mail & LSC | 60% | **95%** | +35% | Contracts | 60% | 95% | +35% |
| Corporation | 65% | 93% | +28% | **Alliance** | 55% | **92%** | +37% |
| **Sovereignty** | 60% | **95%** | +35% | Science & Industry | 45% | **90%** | +45% |
| Bookmark System | 70% | 95% | +25% | **Effects System** | 65% | **96%** | +31% |

See [`PROGRESS.md`](PROGRESS.md) for full breakdown.  
Полная раскладка — в [`PROGRESS.md`](PROGRESS.md).

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="PROGRESS.md">Progress</a> ·
  <a href="current_state_summary.md">Session log</a>
</p>

<p align="center"><b>LGPL v3</b> — educational project. Not for public servers.</p>
