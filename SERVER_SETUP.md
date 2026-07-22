# Server Setup & Debug Notes

## Что сделать на сервере (после git pull)

```bash
# 1. Подтянуть последние изменения
cd /src && git pull

# 2. Применить новые миграции БД
/sql/evedbtool migrate

# 3. Включить логи пакетов в /src/utils/config/log.ini:
#    DESTINY__BALL_DUMP=1
#    DESTINY__BALL_DECODE=1

# 4. Сбросить server_cache (чтоб клиент перезапросил config.BulkData.types с новым bulkDataChangeID)
rm -rf /app/server_cache/*

# 5. Собрать
make -j$(nproc)

# 6. Запустить
./eve-server

# 7. В игре заспавнить тестовых NPC:
#    /spawn 2372    # Entity Frigate — проверить крестик
#    /spawn 10017   # Entity Cruiser
#    /spawn 24692   # Ship NPC (контрольный — крестик должен быть)
```

## Debug Logs (для отладки)

Создать `config/log.ini` (монтируется в `/app/etc/log.ini`):

```ini
# Категории для кросс-крестов NPC (DESTINY)
DESTINY__BALL_DUMP=1       # dump структуры AddBalls пакетов
DESTINY__BALL_DECODE=1     # декодированный destiny-бинарник
DESTINY__ORBIT_TRACE=1     # трассировка орбиты (расчёт позиции)
DESTINY__MESSAGE=1         # сообщения Destiny

# Автопилот
AUTOPILOT__MESSAGE=1       # CmdWarpToStuffAutopilot/CmdStop/CmdJump

# Клиентские запросы (Collector)
COLLECT__DESTINY=1         # destiny-апдейты от клиента
COLLECT__CALL_SUMMARY=1    # сводка RPC вызовов (уже включён по умолчанию)

# Отключить когда не нужно (шумят)
# DESTINY__BALL_DUMP=0
# DESTINY__ORBIT_TRACE=0
# COLLECT__DESTINY=0
```

Совет: не включайте `DESTINY__ORBIT_TRACE` на продакшене — генерирует много лога каждый тик.

## Анализ пакетов (deprecated)

Дампы пакетов в `/tmp/evemu_addball_*.bin` больше не нужны — кросс-кресты починены. Если нужно для дальнейшего анализа:

```bash
pip install evemu
python3 /src/tools/parse_packet.py /tmp/evemu_addball_0.bin
```

Дамп пишется в `SystemBubble::AddBallExclusive()` только для первых 10 спавнов.

## Cache (bulkDataChangeID)

Добавлена пустая миграция `20260711000003-force_cache_refresh.sql`. При следующем запуске сервера `bulkDataChangeID` изменится, клиент перезапросит `config.BulkData.types` с исправленными groupID.

Если не помогло — сбросить принудительно:
```bash
rm -rf /app/server_cache/*
```

## Известные проблемы

### 1. Crosshair (красный крест) на Entity NPC
**Симптом:** Entity-пираты (Angel, Blood, Guristas и т.д.) не имеют красного креста и иконки типа корабля. Можно взять в таргет, рамки есть, но креста нет.
**ФИКС (2026-07-12):** `NPC::MakeSlimItem` теперь делегирует `DynamicSystemEntity::MakeSlimItem()` — использует реальные `categoryID(11)/groupID(550-552)` из БД вместо хардкода `categoryID=6/groupID=25`. Клиенту нужны настоящие Entity-группы для отображения крестика.
**Дополнительно:** `NPC::EncodeDestiny` — flags изменены с `IsInteractive|IsFree|IsMassive (13)` на `IsFree (1)` (как у ShipSE без пилота).

### 2. Autopilot chain
**Симптом:** После прыжка через гейт автопилот не продолжает цепочку
**Текущее состояние:** CmdWarpToStuffAutopilot работает, Follow после варпа работает, полная остановка у врат. Проблема — клиент не продолжает маршрут после session change.
**Причина:** Маршрут автопилота хранится только на клиенте и сбрасывается при session change. Сервер корректно сохраняет `m_autoPilot=true`, но клиент не шлёт `CmdWarpToStuffAutopilot` в новой системе.
**Решение:** Требует серверного API для хранения/восстановления маршрута.

### 3. Orbit дёрганье
**Симптом:** При орбите на высокой скорости (>3000 м/с) корабль дёргается
**Причина:** Orbit() использует позиционный расчёт (SetPosition каждый тик) вместо velocity-based движения. Нужен рефакторинг Destiny.

## Миграции БД (деплой)

```bash
# Установить миграции
/sql/evedbtool migrate

# Если миграция уже есть в БД но не применялась:
mysql -u evemu -p evemu -e "INSERT INTO migrations (id, applied_at) VALUES ('20260711000001-calendar_tables', NOW());"
```

## Тестовые спавн ID

```bash
# Ship-type (работают с крестиком):
/spawn 24692  # Incursion Sansha Battlecruiser

# Entity-type (проверка крестиков после фикса):
/spawn 2372   # Angel Frigate → g550→25
/spawn 10017  # Angel Cruiser → g551→26
/spawn 11898  # Angel Battleship → g552→27
/spawn 10025  # Sansha Frigate → g567→25
```
