<p align="center">
  <img src="https://img.shields.io/badge/Crucible-1.0-blue?style=flat-square"/>
  <img src="https://github.com/dmsovenko-ship-it/evemu/actions/workflows/opencode.yml/badge.svg"/>
  <img src="https://img.shields.io/badge/license-LGPLv3-green?style=flat-square"/>
</p>

<h1 align="center">EVEmu Crucible — Custom Fork</h1>

<p align="center">
  Educational server emulator for <b>EVE Online (Crucible era)</b>.<br>
  Fork of <a href="https://github.com/EvEmu-Project/evemu_Crucible">EvEmu-Project/evemu_Crucible</a>.
</p>

---

## ✨ Features

### Security & CONCORD

| | |
|---|---|
| **CONCORD** | NPC Police Battleships warp in, apply ×25 HP damage, self-destruct. Delay scales by sec: 1.0→6s, 0.5→19s |
| **Sentry Guns** | Static spawn at gates & stations per zone: highsec 3/5, lowsec 4/7, nullsec 0/8 |
| | Highsec/lowsec: target by crime watch (criminal/aggressor/outlaw) |
| | **Nullsec**: target by standing with owner corporation (< -2.0). Corps inherited from station |
| **CrimeWatch** | Weapon timer (60s), aggression timer (15min), criminal flag (15min highsec), suspect on loot |
| **Outlaw** | SS ≤ -5.0: no dock/jump, CONCORD ignores, sentries engage |
| **Kill Rights** | Granted to victim on criminal aggression (highsec) or pod kill (lowsec). 30-day expiry. |
| | Activation: 15-min **Limited Engagement** — target is legal for everyone, no CONCORD |
| | Price & access mask supported |

### Combat & NPC

| | |
|---|---|
| **Convoy system** | Guard + hauler convoys between stations (sec 0.5–0.7). Phased movement, wake-up on attack, sentry defense |
| **Killmails** | Full XML blob with dropped/destroyed items, saved to `chrKillTable`. Push notification via `SelfEveMail` + `SendNotifyMsg` |
| **Loot** | Minerals + 30% module drop on convoy kills |
| **Standings** | Faction penalty on kill, enemy faction reward |
| **NPC defaults** | `AttrInertia=70`, `AttrCruiseSpeed=150`, HP/shield defaults for guards & haulers |

### Movement

| | |
|---|---|
| **Warp** | Gate warp offset capped at 5km for large models (>90km radius) |
| **Align** | Timeout 12s (upstream 10.3s) |
| **Velocity** | Reset in catch-all prevents off-angle forced warps |

### Infrastructure

| | |
|---|---|
| **Build** | Docker Compose, ccache with persistent volume, MariaDB→MySQL symlinks (saves 400MB) |
| **Mail** | Dual-write: `mailMessage` + `mailStatus` (visible in client) + `eveMail` (legacy). Killmail push via `SelfEveMail` |
| **Kill Rights** | DB-backed, UI via `standing2.GetMyKillRights` / `ActivateKillRight` / `UpdateKillRight` / `DeleteKillRight` |

---

## 🚀 Quick Start

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
docker compose up -d --build

# wait for "Server started" in logs (~10-20 min on first boot, market seeding):
docker logs -f server
```

### Management

```bash
docker compose stop              # stop
docker compose up -d             # start
docker compose down -v           # full wipe (DB reset)
docker compose logs -f server    # live logs
```

### Accounts & GM

```bash
# grant admin rights:
bash utils/grant-admin.sh "YourAccountName"

# verify (role = 2013265920 = full admin):
docker exec db mysql -u evemu -pevemu evemu -e "SELECT accountName, role FROM account;"
```

| Command | Description |
|---------|-------------|
| `/giveallskills me` | Max all skills |
| `/spawn <typeID>` | Spawn NPC |
| `/online me` | Online all modules |
| `/search <name>` | Search items |

Full list: [doc/admin_reference.md](doc/admin_reference.md)

---

## 🐞 Known Issues

- `Destiny::MoveObject()` "move checks not set right" — upstream bug, harmless spam
- In-game mail to client inbox — incomplete `MailService`, root cause not yet found
- Full killmail push notification (combat log live updates) — not implemented
- Missing dataset entries (`eveOwners 500022`, `dgmEffects` 0) — SQL migration applied

---

## 🔗 Links

- **Upstream:** https://github.com/EvEmu-Project/evemu_Crucible
- **PR #327** (included): https://github.com/EvEmu-Project/evemu_Crucible/pull/327
- **Discord:** https://discord.gg/fTfAREYxbz

---

## ⚖️ License

**LGPL v3.** Educational project. Not intended for public servers.

---

<br>

<h1 align="center">EVEmu Crucible — Кастомный форк</h1>

<p align="center">
  Образовательный эмулятор сервера <b>EVE Online (Crucible)</b>.
</p>

---

## ✨ Возможности

### Безопасность и CONCORD

| | |
|---|---|
| **CONCORD** | NPC Police Battleships, задержка по sec, ×25 HP, письмо в почту |
| **Сентри** | Статический спавн у гейтов и станций. Количество по зонам: хайсек 3/5, лоусек 4/7, нули 0/8 |
| | Хайсек/лоусек: атака по CrimeWatch (criminal/aggressor/outlaw) |
| | **Нули**: атака по рейтингу с корпорацией-владельцем станции (< -2.0). Корпорация наследуется от станции |
| **CrimeWatch** | Weapon timer (60s), aggression (15min), criminal (15min хайсек), suspect на лут |
| **Outlaw** | SS ≤ -5.0: нет дока/джампа, CONCORD игнорирует, сентри атакуют |
| **Kill Rights** | Жертве при криминальной агрессии (хайсек) или подкилле (лоусек). Срок 30 дней |
| | Активация: Limited Engagement 15 мин — цель легальна для всех, CONCORD не вмешивается |
| | Поддержка цены и маски доступа |

### Бои и NPC

| | |
|---|---|
| **Конвои** | Охрана + грузовозы между станциями (sec 0.5–0.7). Фазы движения, пробуждение при атаке, защита сентри |
| **Киллимейлы** | Полный XML blob с выпавшими/уничтоженными предметами. Push-уведомление через `SelfEveMail` + `SendNotifyMsg` |
| **Лут** | Минералы + 30% шанс модуля при убийстве конвоя |
| **Репутация** | Штраф фракции за убийство, бонус за вражескую фракцию |

### Движение

| | |
|---|---|
| **Варп** | Отступ 5км для больших моделей гейтов (>90км) |
| **Выравнивание** | Таймаут 12с (в upstream 10.3с) |
| **Скорость** | Сброс при catch-all, предотвращает кривой варп |

### Инфраструктура

| | |
|---|---|
| **Сборка** | Docker Compose, ccache, MariaDB→MySQL symlinks (экономит 400МБ) |
| **Почта** | Двойная запись: `mailMessage` + `mailStatus` (видна клиенту) + `eveMail` (legacy) |
| **Kill Rights** | БД + UI через `standing2.GetMyKillRights`, `ActivateKillRight`, `UpdateKillRight`, `DeleteKillRight` |

---

## 🚀 Быстрый старт

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
docker compose up -d --build

# ждать "Server started" в логах (~10-20 мин при первом запуске):
docker logs -f server
```

### Управление

```bash
docker compose stop              # остановить
docker compose up -d             # запустить
docker compose down -v           # сброс БД
docker compose logs -f server    # логи
```

### Учётные записи и GM

```bash
bash utils/grant-admin.sh "ИмяАккаунта"

# проверить (role = 2013265920 = полный админ):
docker exec db mysql -u evemu -pevemu evemu -e "SELECT accountName, role FROM account;"
```

| Команда | Описание |
|---------|----------|
| `/giveallskills me` | Все навыки на 5 |
| `/spawn <typeID>` | Заспавнить NPC |
| `/online me` | Включить все модули |
| `/search <name>` | Поиск по имени |

Полный список: [doc/admin_reference.md](doc/admin_reference.md)

---

## 🐞 Известные проблемы

- `Destiny::MoveObject()` — upstream, не влияет на геймплей
- Доставка почты в клиентский инбокс — неполный `MailService`
- Push-уведомления киллимейлов (combat log) — не реализованы
- Kill Right активация — оплата владельцу через прямой SQL (без транзакций)

---

## 🔗 Ссылки

- **Upstream:** https://github.com/EvEmu-Project/evemu_Crucible
- **PR #327:** https://github.com/EvEmu-Project/evemu_Crucible/pull/327
- **Discord:** https://discord.gg/fTfAREYxbz

---

## ⚖️ Лицензия

**LGPL v3.** Образовательный проект. Не предназначен для публичных серверов.
