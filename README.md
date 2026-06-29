<p align="center">
  <img src="https://img.shields.io/badge/Crucible-1.0-blue?style=flat-square"/>
  <img src="https://github.com/dmsovenko-ship-it/evemu/actions/workflows/opencode.yml/badge.svg"/>
  <img src="https://img.shields.io/badge/license-LGPLv3-green?style=flat-square"/>
</p>

<h1 align="center">EVEmu Crucible — Custom Fork</h1>

<p align="center">
  <b>EVE Online (Crucible era)</b> server emulator.<br>
  Fork of <a href="https://github.com/EvEmu-Project/evemu_Crucible">EvEmu-Project/evemu_Crucible</a>.
</p>

---

## ✨ Features

**CONCORD** — NPC Police Battleships (×25 HP), delay scales by sec (1.0→6s, 0.5→19s). Sec penalty on aggression.

**Sentry Guns** — Static spawn at gates & stations. Highsec/lowsec: CrimeWatch targets. **Nullsec:** standing with station corp (< -2.0). Corp inherited from station.

**CrimeWatch** — Weapon timer (60s), aggression (15min), criminal flag (15min highsec), suspect on loot. Outlaw (SS ≤ −5.0) blocks dock/jump, sentries engage.

**Kill Rights** — Granted on criminal aggression (highsec) or pod kill (lowsec). 30-day expiry. Auto-activation on attack → 15min **Limited Engagement** (legal target, no CONCORD). Price & access mask supported.

**Convoys** — Guards + haulers between stations (sec 0.5–0.7). Phased movement, wake-up on attack, sentry defense. Loot drops (minerals + 30% module).

**Killmails** — XML blob with dropped/destroyed items. Push notification via mail + combat log.

**Movement** — Gate warp 5km offset, 12s align timeout, velocity reset fix.

**Infrastructure** — Docker Compose, ccache, MariaDB→MySQL symlinks (−400MB build).

**Mail** — Dual-write to `mailMessage`+`mailStatus` (visible in client) + `eveMail` (legacy).

---

## 🚀 Quick Start

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
docker compose up -d --build
docker logs -f server          # wait for "Server started"
```

| Command | Description |
|---------|-------------|
| `docker compose stop` | Stop server |
| `docker compose up -d` | Start server |
| `docker compose down -v` | Full DB reset |
| `bash utils/grant-admin.sh "Name"` | Grant GM rights |

| GM Command | Description |
|------------|-------------|
| `/giveallskills me` | Max skills |
| `/spawn <typeID>` | Spawn NPC |
| `/online me` | Online modules |
| `/search <name>` | Search items |

Full GM list: [doc/admin_reference.md](doc/admin_reference.md)

---

## 🐞 Known Issues

- `Destiny::MoveObject()` — upstream noise, harmless
- Mail delivery — incomplete `MailService`
- Killmail push notifications — not implemented

---

## 📜 Changelog (custom)

| Area | Change |
|------|--------|
| **CONCORD** | Police ships, delay by sec, killmail, −0.2 penalty |
| **Sentry Guns** | Corp from station, standing-based in nullsec, CrimeWatch in high/low |
| **CrimeWatch** | Weapon/aggression/criminal timers, outlaw, suspect on loot |
| **Kill Rights** | Grant on attack, auto-activate, Limited Engagement, price + DB |
| **Convoys** | Guards + haulers, phased movement, wake-up on attack, defense |
| **Killmails** | XML blob, push via mail + combat log, NPC corpID fix |
| **Warp** | Stop desync fix, Halt after warp, position sync to client |
| **Drones** | Full skill bonuses, EWAR/logistics/mining, AI rewrite |
| **Mail** | Dual-write fix (visible in client), Deflate compression |
| **LiveUpdate** | News ticker shows latest commit instead of SSL error |
| **Build** | Docker Compose, ccache, MariaDB→MySQL symlinks |

---

## 🔗 Links

[Upstream](https://github.com/EvEmu-Project/evemu_Crucible) · [PR #327](https://github.com/EvEmu-Project/evemu_Crucible/pull/327) · [Discord](https://discord.gg/fTfAREYxbz)

## ⚖️ License

**LGPL v3** — educational project. Not for public servers.

---

<br>

<h1 align="center">EVEmu Crucible — Кастомный форк</h1>

<p align="center">
  Образовательный эмулятор сервера <b>EVE Online (Crucible)</b>.
</p>

---

## ✨ Возможности

**CONCORD** — Police Battleships, ×25 HP, задержка по sec, штраф −0.2 за агрессию.

**Сентри** — Спавн у гейтов и станций. Хайсек/лоусек: CrimeWatch. **Нули:** рейтинг с корпорацией станции (< −2.0). Корпорация наследуется от станции.

**CrimeWatch** — Weapon timer (60s), aggression (15min), criminal (15min хайсек), suspect на лут. Outlaw (SS ≤ −5.0): нет дока/джампа, сентри атакуют.

**Kill Rights** — Выдача при агрессии (хайсек) или подкилле (лоусек). 30 дней. Авто-активация → Limited Engagement 15 мин (легальная цель). Цена и маска доступа.

**Конвои** — Охрана + грузовозы между станциями. Фазы, пробуждение при атаке, лут (минералы + 30% модуль).

**Киллимейлы** — XML с дропом. Push в почту + combat log.

**Инфраструктура** — Docker Compose, ccache, MariaDB→MySQL (−400MB). Почта: двойная запись (видна клиенту).

---

## 🚀 Быстрый старт

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
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
| `/search <name>` | Поиск |

Полный список: [doc/admin_reference.md](doc/admin_reference.md)

---

## 🐞 Известные проблемы

- `Destiny::MoveObject()` — upstream, безвредно
- Доставка почты — неполный `MailService`
- Push-уведомления киллимейлов — не реализованы

---

## 📜 Список изменений

| Область | Изменение |
|---------|-----------|
| **CONCORD** | Police, задержка по sec, киллимейл, штраф −0.2 |
| **Сентри** | Корпорация от станции, рейтинг в нулях |
| **CrimeWatch** | Таймеры оружия/аггрессии, криминал, outlaw |
| **Kill Rights** | Выдача, авто-активация, Limited Engagement, цена |
| **Конвои** | Охрана + грузовозы, фазы, защита сентри |
| **Киллимейлы** | XML, push в почту, фикс killerID для NPC |
| **Варп** | Фикс десинхра, Halt после варпа |
| **Дроны** | Скиллы, EWAR/логистика/майнинг, AI |
| **Почта** | Двойная запись (видна клиенту) |
| **LiveUpdate** | Новости — последний коммит вместо SSL ошибки |

---

## 🔗 Ссылки

[Upstream](https://github.com/EvEmu-Project/evemu_Crucible) · [PR #327](https://github.com/EvEmu-Project/evemu_Crucible/pull/327) · [Discord](https://discord.gg/fTfAREYxbz)

## ⚖️ Лицензия

**LGPL v3** — образовательный проект. Не предназначен для публичных серверов.
