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
  <b>EVE Online (Crucible era) server emulator</b> · <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> fork — ~82% complete
</p>

<br>

> **⚠️ Educational project only** — for studying C++, network protocols, game server architecture.  
> *Только образовательный проект* — для изучения C++, сетевых протоколов и архитектуры игровых серверов.

---

## Features / Возможности

| EN | RU |
|----|----|
| **Warp & Movement** — smooth warp-to-0, fleet warp, autopilot chains, login warp-in | **Варп и движение** — плавный варп-ту-0, флот-варп, автопилот, логин-варп |
| **Drones** — full AI (combat/EWAR/logistics/mining), skills, subtypes, control range | **Дроны** — полный AI (бой/EWAR/логистика/майнинг), навыки, подтипы |
| **NPC systems** — anomalies, incursions, belt rats, gate rats, convoys, customs police | **NPC системы** — аномалии, инкурсии, бельтраты, гейтраты, конвои, таможня |
| **Crimewatch** — weapon/aggression/criminal timers, CONCORD, sentry guns, kill rights | **Crimewatch** — таймеры, CONCORD, сентри, киллрайты |
| **Clones & Implants** — jump clones, per-clone implants, ship clone bay, SP loss on T3 pod | **Клоны и импланты** — джамп-клоны, импланты на клон, шип-клон-бей, SP loss |
| **Contracts** — item exchange, courier, auctions with bidding + ISK transfer | **Контракты** — обмен, курьер, аукционы со ставками и переводом ISK |
| **Corporation & Alliance** — corp management, alliance wars, bulletins, bills, LP store | **Корпорации и альянсы** — управление, войны, бюллетени, счета, LP магазин |
| **Market** — buy/sell orders, corp market, market bots (Trader Joe) | **Маркет** — ордера, корп-маркет, маркет-боты |
| **Science & Industry** — manufacturing, copying, invention, reverse engineering | **Наука и промышленность** — производство, копирование, инвеншен, РЕ |
| **POS** — towers, fuel/reinforced, CPU/PG, weapon AI, orbitals, reactors, assume/relinquish control, skill checks | **POS** — тауэры, топливо/reinforced, CPU/PG, оружие AI, орбиталки, реакторы, контроль, скиллы |
| **Overheating** — heat dmg per slot, OverloadRack, Thermodynamics, Nanite Paste | **Перегрев** — урон по слотам, OverloadRack, Thermodynamics, Nanite Paste |
| **Notifications** — persistent DB + live push, bill/tower/agent/corp sources | **Нотификации** — БД + live push, счета/POS/агенты/корп |
| **LSC Chat** — private conversations, channel creation, mailing lists | **LSC Чат** — приватные разговоры, каналы, списки рассылки |
| **Faction Warfare** — join/leave, membership, enemy checks, stats, corp/alliance, notifications | **ФВ** — вступление/выход, членство, враги, статистика, корп/альянс, нотификации |
| **Planetary Interaction** — colonies, customs offices, resource extraction | **Планетарка** — колонии, таможня, добыча |
| **Wormholes** — full lifecycle, mass/lifetime tracking, K162 generation | **Варпхолы** — полный цикл, масса/время жизни, K162 |
| **Missions** — courier, mining, encounter, storyline, epic arcs (Blood-Stained Stars) | **Миссии** — курьер, майнинг,encounter, storyline, эпик арки |
| **Scanning** — probes, cosmic signatures, anomalies, directional scan | **Сканирование** — пробы, сигнатуры, аномалии, D-scan |
| **Fleet** — fleet warp, boosts, warfare links, specialist skills | **Флот** — флот-варп, бусты, warfare-линки, специалист скиллы |
| **Mail & LSC** — corp/alliance mail, mailing lists, chat channels | **Почта и чат** — корп/альянс почта, списки рассылки, каналы |
| **Incursions** — state machine, wave spawning, contest rewards | **Инкурсии** — стейт-машина, волновой спавн, contest награды |
| **GM commands** — spawn, dogma, giveallskills, kick, ban, teleport | **GM команды** — спавн, догма, скиллы, кик, бан, телепорт |

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

**Estimated overall: ~96%** (upstream: 59.5%)

| System                |  %  | System                |  %  |
|-----------------------|:---:|-----------------------|:---:|
| Account & Character   | 97% | Skills & Certificates | 99% |
| Ship Navigation       | 95% | Combat                | 99% |
| Ship Navigation       | 95% | Combat                | 99% |
| Modules & Overheating | 95% | Drones                | 90% |
| NPC AI & Spawning     | 80% | Agents & Missions     | 85% |
| **POS**               | 95% | Market                | 85% |
| **Incursions**        | 75% | Fleet                 | 98% |
| **Wormholes**         | 80% | Scanning              | 99% |
| **Notifications**     | 95% | **Standings**         | 85% |
| **Faction Warfare**   | 80% | Calendar              | 90% |
| Mail & LSC            | 85% | Contracts             | 85% |
| Corporation           | 80% | Alliance              | 40% |
| Sovereignty           | 80% | Science & Industry    | 50% |
| Bookmark System       | 95% | Effects System        | 88% |
| Anomaly Manager       | 85% | Spawn Manager         | 75% |
| Dungeon Manager       | 70% | Civilian Manager      | 10% |

See [`PROGRESS.md`](PROGRESS.md) for full breakdown.  
Полная раскладка — в [`PROGRESS.md`](PROGRESS.md).

---

<p align="center">
  <a href="https://github.com/EvEmu-Project/evemu_Crucible">Upstream</a> ·
  <a href="PROGRESS.md">Progress</a> ·
  <a href="current_state_summary.md">Session log</a>
</p>

<p align="center"><b>LGPL v3</b> — educational project. Not for public servers.</p>
