# EVEmu Crucible — Custom Fork [![build](https://github.com/dmsovenko-ship-it/evemu/actions/workflows/opencode.yml/badge.svg)](https://github.com/dmsovenko-ship-it/evemu/actions)

> **Fork of [EVEmu Crucible](https://github.com/EvEmu-Project/evemu_Crucible)** — an educational server emulator for EVE Online (Crucible era).

---

## ✨ Features vs Upstream

| Category | Feature | Status | Details |
|----------|---------|--------|---------|
| **CONCORD** | Instant destruction with NPC ships | ✅ | `CrimeWatch` — Concord Police BS (typeID 1912-1918) warp in, apply 10,000% HP damage, self-destruct |
| | Response delay by system sec | ✅ | 1.0→6s, 0.9→6s, 0.8→7s, 0.7→10s, 0.6→14s, 0.5→19s |
| | Killmail via MailDB | ✅ | From EVE System (senderID=1) with ship/system/damage details |
| | Security penalty | ✅ | -0.2 per aggression + formula in Killed(), clamped [-10,+10] |
| **Sentry Guns** | Static spawn at gates & stations | ✅ | Via `SystemManager::SpawnSentryGuns()`, loaded at system boot |
| | Zone-based counts | ✅ | Highsec: gates 3, stations 5 / Lowsec: gates 4, stations 7 / Nullsec: gates 0, stations 8 / WH: none |
| | SentryAI targeting | ✅ | Targets criminals, aggressors, outlaws in range (custom `SentryAI`) |
| **CrimeWatch** | Weapon timer (60s on any fire) | ✅ | Blocks dock/jump |
| | Aggression timer (15min on PvP) | ✅ | Blocks dock/jump |
| | Criminal flag (15min highsec PvP) | ✅ | Triggers CONCORD |
| | Suspect flag on loot | ✅ | Weapon + aggression timer on looting foreign containers |
| | Outlaw status (SS ≤ -5.0) | ✅ | No dock/jump, CONCORD ignores attacks on outlaws, sentry guns engage |
| **Security** | Gate entry check | ✅ | `minSec = -(2.0 + (1.0 - systemSec) * 5.0)`, blocks jump if too criminal |
| | Personal security range | ✅ | -10 .. +10, clamped in `secStatusChange()` |
| | Highsec | ✅ | 0.5-1.0 (CONCORD active) |
| | Lowsec | ✅ | 0.01-0.49 (no CONCORD, sentry guns only) |
| **Warp** | Gate warp fix (large radius) | ✅ | Cap at 5km offset for gates with radius > 90km |
| | Align timeout extended | ✅ | 12s for players (was 10.3s) |
| | Velocity reset on catch-all | ✅ | Prevents off-angle forced warps |
| **NPC** | Belt rat spawn fix (PR #327) | ✅ | Correct security class, off-grid warp-in |
| | Move flag fix | ✅ | `UpdateVelocity()` early return sets flags, `>=` check in `MoveObject()` |
| | Damage messages | ✅ | Fixed missing `source`/`weapon` fields for drones/NPCs |
| **Mail** | GetMailHeaders | ✅ | Was stub returning nullptr — now queries DB |
| | CONCORD mail | ✅ | Via `MailDB::SendMail` into `mailMessage` table |
| **Build** | Docker without `.git` | ✅ | Removed `ADD /.git/` from Dockerfile |
| | `.dockerignore` | ✅ | Prevents build context pollution |
| | EVEDBTool download | ✅ | Direct URL, no GitHub API rate limits |
| | `eve-server.xml` config | ✅ | `PositionHack=true`, `crime.Enabled=true` |
| **Other** | `.claude/instructions.md` | ✅ | AI development guidelines |

---

## 🚀 Quick Start

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
docker compose up -d --build
```

First boot takes 10-20 min (market seeding). Wait for `Server started` in logs:

```bash
docker logs -f server
```

## 🎮 Management

```bash
docker compose stop              # stop
docker compose up -d             # start
docker compose down -v           # full wipe (DB reset)
docker compose logs -f server    # live logs
```

## 👤 Accounts & GM

Accounts are auto-created on first login. To grant admin:

```bash
bash utils/grant-admin.sh "YourAccountName"
```

Verify:

```bash
docker exec db mysql -u evemu -pevemu evemu -e "SELECT accountName, role FROM account;"
```

`role = 2013265920` = full admin.

### GM Commands

| Command | Description |
|---------|-------------|
| `/giveallskills me` | Max all skills |
| `/spawn <typeID>` | Spawn NPC |
| `/online me` | Online all modules |
| `/search <name>` | Search items |

Full list: [doc/admin_reference.md](doc/admin_reference.md)

## 🐞 Known Issues

- `Destiny::MoveObject()` "move checks not set right" — upstream bug, harmless log spam
- `eveMail` table mails not visible to client — use `mailMessage` table (fixed in this fork)
- Killmail popup in combat log — data saved to `chrKillTable` but no client notification
- Sentry gun damage shows as "self" when no sentry entity found in system

---

## 📜 Changelog (custom commits)

| Commit | Description |
|--------|-------------|
| `42ba735f` | Cleanup: sentry code removed from CrimeWatch |
| `a0426111` | SentryAI targets criminals/outlaws |
| `653baf7f` | Sentry damage source from real sentry entity |
| `62edcadd` | Sentry guns attack outlaws automatically |
| `4d5c13ad` | Sentry spawn by zone (nullsec no gates, wormholes skip) |
| `e217fd66` | Outlaw status (SS ≤ -5) — no dock, no CONCORD |
| `55ffe76f` | Security gate check — block jump if SS too low |
| `758a3265` | Suspect flag on loot, security gate |
| `a9dee857` | Security ranges: highsec ≥0.5, personal -10..+10 |
| `ab6c6940` | Sentry guns + damage timer |
| `c77fdfb1` | CONCORD NPC ships + mail + damage |
| `9fae0eeb` | Velocity reset in warp catch-all |
| `8dfafcdb` | Weapon timer on any fire, aggression only on PvP |
| `d082316e` | Killmail notification for all PvP kills |
| `983e44e0` | Fix missing brace in Killed() |
| `8cdfaecb` | System-level sentry guns at gates/stations |
| `cd75945c` | Crime config enabled, shipSE damage source |
| `5ab9cc61` | Negate security loss in Killed() |
| `b5bbb617` | BeyonceService: min distance for all objects |
| `21fa1e88` | Initial warp-to-0 fix, .dockerignore, .claude |
| `1ee3b5fb` | EVEDBTool direct URL (no API rate limit) |

---

## 🔗 Links

- **Upstream:** https://github.com/EvEmu-Project/evemu_Crucible
- **PR #327** (included): https://github.com/EvEmu-Project/evemu_Crucible/pull/327
- **Discord:** https://discord.gg/fTfAREYxbz

---

## ⚖️ License & Disclaimer

**Educational project.** LGPL v3. Not intended for public servers.

---

---

# EVEmu Crucible — Кастомный форк

Форк [EVEmu Crucible](https://github.com/EvEmu-Project/evemu_Crucible) — эмулятор сервера EVE Online (Crucible) с расширенной системой CONCORD, CrimeWatch, статическими sentry guns и исправлениями движения/варпа.

## ✨ Отличия от upstream

### CrimeWatch & CONCORD

| Механика | Статус |
|----------|--------|
| **CONCORD** — NPC корабли (Police BS), задержка по sec, mail, damage, cleanup | ✅ |
| **Sentry Guns** — статический спавн у гейтов/станций (через `SystemManager`) | ✅ |
| Количество: хайсек 3/5, лоусек 4/7, нули 0/8, вормхолы 0 | ✅ |
| SentryAI — автоматический поиск и атака криминалов/outlaw | ✅ |
| Outlaw (SS ≤ -5.0) — блок дока, CONCORD не реагирует, сентри атакуют | ✅ |
| Weapon timer 60s на любой выстрел | ✅ |
| Aggression 15min + Criminal 15min (PvP highsec) | ✅ |
| Suspect flag на лут | ✅ |
| Security penalty -0.2 за агрессию + формула в Killed(), клиппинг [-10,+10] | ✅ |

### Безопасность

| Механика | Статус |
|----------|--------|
| Gate security check — блок джампа если SS ниже порога | ✅ |
| Формула: `minSec = -(2.0 + (1.0 - systemSec) * 5.0)` | ✅ |
| Хайсек: 0.5-1.0 | ✅ |
| Лоусек: 0.01-0.49 | ✅ |
| Killmail в БД (`chrKillTable`) + письмо от EVE System | ✅ |
| GetMailHeaders — реализован (была заглушка) | ✅ |

### Варп / Движение

| Механика | Статус |
|----------|--------|
| Gate warp — отступ 5км для больших моделей (>90км) | ✅ |
| Таймаут выравнивания: 12с (было 10.3с) | ✅ |
| Сброс скорости в catch-all (предотвращает кривой варп) | ✅ |
| NPC move checks fix — `UpdateVelocity()` устанавливает флаги | ✅ |

### NPC / Бои

| Механика | Статус |
|----------|--------|
| Belt rat spawn (PR #327) — корректный класс + warp-in | ✅ |
| Damage messages — добавлены `source`/`weapon` для дронов/NPC | ✅ |
| Drone damage message — `Given` вместо `Taken` | ✅ |

### Инфраструктура

| Улучшение | Статус |
|-----------|--------|
| Docker без `.git` в образе | ✅ |
| `.dockerignore` | ✅ |
| EVEDBTool — прямой URL (нет rate limit GitHub API) | ✅ |
| `PositionHack=true` в `eve-server.xml` | ✅ |
| `crime.Enabled=true` по умолчанию | ✅ |
| `.claude/instructions.md` | ✅ |

## 📋 Известные проблемы

- `Destiny::MoveObject()` "move checks not set right" — upstream, не влияет на геймплей
- Почта в `eveMail` не видна клиенту — используем `mailMessage` (работает)
- Killmail combat log — данные в БД, но без push-уведомления

## 🚀 Установка

```bash
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu
docker compose up -d --build
```

Первый запуск ~10-20 мин (заливка рынка). Ждать `Server started`:

```bash
docker logs -f server
```

### Управление

```bash
docker compose stop                           # остановить
docker compose up -d                          # запустить
docker compose down -v                        # сброс БД
docker compose logs -f server                 # логи в реальном времени
docker exec db mysql -u evemu -pevemu evemu   # SQL консоль
```

### Учётные записи и GM

```bash
bash utils/grant-admin.sh "ИмяАккаунта"
```

Проверка:
```bash
docker exec db mysql -u evemu -pevemu evemu -e "SELECT accountName, role FROM account;"
```

`role = 2013265920` — полный админ.

### GM-команды

| Команда | Описание |
|---------|----------|
| `/giveallskills me` | Все навыки на 5 |
| `/spawn <typeID>` | Заспавнить NPC |
| `/online me` | Включить все модули |
| `/search <name>` | Поиск по имени |

Полный список: [doc/admin_reference.md](doc/admin_reference.md)

## 📜 Changelog (наши коммиты)

| Коммит | Описание |
|--------|----------|
| `42ba735f` | Очистка: sentry code удалён из CrimeWatch (теперь в SentryAI) |
| `a0426111` | SentryAI теперь сам ищет и атакует криминалов/outlaw |
| `e217fd66` | Outlaw (SS ≤ -5) — блок дока, CONCORD не вмешивается |
| `55ffe76f` | Проверка security при джампе через гейт |
| `a9dee857` | Хайсек ≥0.5, личный security -10..+10 |
| `ab6c6940` | Sentry guns + дамаг таймер |
| `c77fdfb1` | CONCORD NPC + mail + damage |
| `8dafcdb` | Weapon timer на любой выстрел, aggression только PvP |
| `cd75945c` | Crime config включён, источник урона — корабль |
| `5ab9cc61` | Штраф security: отрицательный (был положительный) |
| `b5bbb617` | BeyonceService: минимальная дистанция для всех объектов |
| `21fa1e88` | Первый фикс warp-to-0, .dockerignore, .claude |
| `1ee3b5fb` | EVEDBTool прямой URL |

## 🔗 Ссылки

- **Upstream:** https://github.com/EvEmu-Project/evemu_Crucible
- **PR #327** (включён): https://github.com/EvEmu-Project/evemu_Crucible/pull/327
- **Discord:** https://discord.gg/fTfAREYxbz

## ⚖️ Лицензия

LGPL v3. **Educational project.** Не предназначен для публичных серверов.
