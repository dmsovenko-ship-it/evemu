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

# EVEmu Crucible

<p align="center">
  <b>EVE Online (Crucible) server emulator</b> · <a href="https://github.com/EvEmu-Project/evemu_Crucible">upstream</a> fork · `████████████████████` ~99%
</p>

> **EN:** Educational project — study of C++, networking, game server architecture.  
> **RU:** Образовательный проект — изучение C++, сетевых протоколов и архитектуры игровых серверов.

---

## Features / Возможности

| **EN** | **RU** |
|--------|--------|
| **Warp & Autopilot** — smooth warp-to-0, fleet warp, **auto-jump, multi-hop AP** | **Варп и автопилот** — плавный варп, флот-варп, **авто-прыжок, мультихоп** |
| **Drones** — full AI, 10 subtypes, skills, control range | **Дроны** — полный AI, 10 подтипов, навыки |
| **NPC systems** — anomalies, incursions, belt/gate rats, convoys, customs | **NPC системы** — аномалии, инкурсии, бельт/гейт-раты, конвои, таможня |
| **Clones & Implants** — jump clones, per-clone implants, SP loss on T3 pod | **Клоны и импланты** — джамп-клоны, импланты на клон, SP loss |
| **Science & Industry** — manufacturing, copying, invention, reverse engineering | **Наука и производство** — крафт, копирование, изобретения, реверс |
| **POS** — towers, modules, fuel, reinforced mode, reactions, weapon AI | **POS** — башни, модули, топливо, реинфорс, реакции, оружие |
| **Sovereignty** — TCU (8h claim), IHub (reinforcement), upgrades, outposts | **Суверенитет** — TCU (8ч захват), IHub (реинфорс), апгрейды |
| **Wormholes** — K162, mass/lifetime, system effects, collapse | **Вармхолы** — K162, масса/время, системные эффекты, коллапс |
| **Incursions** — state machine, 5 simultaneous, named NPCs, gate camps | **Инкурсии** — конечный автомат, 5 шт., именные NPC |
| **Faction Warfare** — plex spawn, LP (NPC/PvP/plex), patrols, militia stats | **ФВ** — плесы, LP (NPC/PvP/плекс), патрули, статистика |
| **Crimewatch** — CONCORD, sentry guns, aggro/criminal timers, kill rights | **Crimewatch** — CONCORD, сентри, таймеры, киллрайты |
| **Contracts** — exchange, courier, auctions with bidding | **Контракты** — обмен, курьер, аукционы со ставками |
| **Market** — buy/sell orders, corp market, bots, price history | **Маркет** — ордера, корп-маркет, боты, история цен |
| **Corporation & Alliance** — wars, voting, dividends, bills, roles | **Корпорации и альянсы** — войны, голосования, дивиденды |
| **Agents & Missions** — storyline, career, COSMOS, research, LP store | **Агенты и миссии** — storyline, карьерные, COSMOS, LP магазин |
| **Fleet** — boosts, broadcasts, watchlist, voice chat | **Флот** — бусты, броадкасты, watchlist, голосовой чат |
| **Standings & Security** — decay, PnP, kill rights, status formula | **Стояния и секьюрити** — декай, PnP, киллрайты |

---

## Quick Start / Быстрый старт

```bash
git clone https://github.com/dmsovenko-ship-it/evemu
docker compose up -d
```

Connect client to `http://your-server:26000` — see `SERVER_SETUP.md`.

---

> **⚠️ Educational project only — NOT for gaming use.**  
> *Только образовательный — НЕ предназначен для игры.*

---

## License / Лицензия

LGPLv3 — see [LICENSE](LICENSE).

---

## Documentation / Документация

- `PROGRESS.md` — feature-by-feature progress (EN/RU)
- `SERVER_SETUP.md` — installation guide
- `doc/` — technical notes
