# EVEmu Crucible — Custom Fork

Форк [EVEmu Crucible](https://github.com/EvEmu-Project/evemu_Crucible) — эмулятор сервера EVE Online (Crucible).  
Сборка и запуск через Docker.

## Отличия от upstream

- **Warp-to-0** — исправлено приземление внутри объектов (добавлен отступ 2200м)
- **Docker** — удалён `.git` из образа, добавлен `.dockerignore`
- **`.claude/instructions.md`** — правила для AI-assisted разработки

## Установка на сервере

```bash
# 1. Клонировать
git clone https://github.com/dmsovenko-ship-it/evemu.git /opt/evemu
cd /opt/evemu

# 2. Собрать и запустить
docker compose up -d --build

# 3. Следить за прогрессом первого запуска
docker logs -f db     # инициализация БД
docker logs -f server # загрузка сервера
```

Первый запуск ~10-20 минут — заливается рыночная база.  
После `Server started` можно подключаться клиентом EVE (Crucible).

## Управление

```bash
docker compose stop              # остановить
docker compose up -d             # запустить
docker compose down -v           # сброс + удаление БД
docker compose logs -f server    # логи в реальном времени
```

## Учётные записи

Создаются автоматически при первом входе в игру.  
Выдача GM-прав:

```bash
bash utils/grant-admin.sh "AccountName"
```

Проверка:

```bash
docker exec db mysql -u evemu -pevemu evemu -e "SELECT accountName, role FROM account;"
```

`role = 2013265920` — полный админ.

## GM-команды

| Команда | Описание |
|---------|----------|
| `/giveallskills me` | Все скиллы на 5 |
| `/spawn <typeID>` | Заспавнить NPC |
| `/online me` | Включить все модули |
| `/search <name>` | Поиск предметов |

Полный список: [doc/admin_reference.md](doc/admin_reference.md)

## Ссылки

- Upstream: https://github.com/EvEmu-Project/evemu_Crucible  
- Ченджлог: [doc/ChangeLog.md](doc/ChangeLog.md)  
- Discord: https://discord.gg/fTfAREYxbz

## Disclaimer

**Educational project.** Не предназначен для публичных серверов.

## License

LGPL v3 — [LICENSE](https://www.gnu.org/licenses/lgpl-3.0.html)
