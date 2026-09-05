# EVEmu Session Context

## Current State
Session saved (итог 5 сент. вечер: фикс приёма курьерок `a34850f1` подтверждён, авто-удаление wrap `25da872f` запушен, wraps почищены, «кривое описание» = клиентское). Server on remote host `172.20.1.47`, SSH user: `dmitry` (password `gbnjy78`), path: `/opt/evemu`. Web-портал на `video.iks-online.net:26006` (другой хост, PHP+nginx, репо `https://github.com/dmsovenko-ship-it/evemu-portal` private). Сервер (origin/master) и портал — см. свежие коммиты; портал можно дёргать с сервера эмулятора `curl http://172.20.1.49/...`, SSH на портал `172.20.1.49` (dmitry/gbnjy78), сайт `/var/www/html`. ⚠️ Сервер работает на `a34850f1`, ещё НЕ пересобран на `25da872f`.

## 5 сентября: экономика челоботов + этап 2 «физический товар» — СДЕЛАНО И ПРОВЕРЕНО
Сервер пересобран на `57c13750` (22:17) и работает; игрок Vugl в игре.

### Этап 1 (рынок): коммиты `bb5a4935`, `0439027d`, `bdcd15b1`
- `MarketMgr::BotArbitrageFill(botCharID, stationID, typeID, askOrderID, bidOrderID, qty)` — без-клиентное исполнение пары resting sell+buy как одной сделки: бот покупает у ask-владельца (деньги оффлайн → chrCharacters.balance), товар минтуется ему в ангар и передаётся владельцу bid, escrow станции платит боту; оба ордера списываются/закрываются, пишутся настоящие mktTransactions. Skills через `CharacterDB::GetSkillLevel`, balance-check ДО сделки, никогда не торгует с собой.
- `BotMgr::ProcessDockedTraderEconomy(sysID, stationID, DockedBot)` — раз в 4-10 мин трейдер читает книгу своей станции: пересечение чужих ордеров (bestBid>bestAsk сверх confidence-маржа) → арбитраж; иначе маркет-мейкинг (квотит лучше best bid/ask) с якорем fair = midpoint книги или `invTypes.basePrice` (import_prices.py тянет медиану с ESI). Старые квоты бота на станции/типе снимаются перед ре-квотой. Buy-квота проверяет баланс.
- `BotMemory`: `tradeProfit/tradeLosses` + `GetTradeConfidence()` (-1..1); уверенность гейтит жадность. `DockedBot` хранит `stationID`. Миграция `20260905000002`.
- ⚠️ `PlaceBotOrderAt/PlaceBotBuyOrderAt` — только производители (Miner/Hacker/Explorer 20%) + legacy. Трейдеры рандомные ордера больше НЕ кидают.

### Этап 2 (физический товар целиком): коммиты `df9bca7a`, `2898722c`, `c53c92f6`, `7627f012`
Юзер выбрал максимальный масштаб: майнеры/раттеры реально производят товар, он едет курьерками в Джиту, продаётся там.
- `df9bca7a` **прокачка скиллов**: `botMemory.skillLevel` (0..5, 0xFF=unset) + `GetPractice()`/`PracticeForNextLevel` (4/10/22/46/95) + `PlayerBot::LevelUpFromPractice()` (на каждый profession run / выживший бой) → `CharacterDB::TrainBotToSkillLevel()` тренирует реальные skill-айтмы в БД. SpawnBot читает skillLevel из botMemory (раньше m_botSkill был захардкожен =3 в ctor и никогда не читался из БД; skillTier при респавне роллился заново). Миграция `20260905000003`. Также PlayerBot::AddCargo/DepositCargoAtStation + m_cargo.
- `2898722c` **минеры добывают реальную руду**: в Miner-кейсе бот летит к астероиду, в <4 км «копает» strip-цикл — тип руды астероида в m_cargo (объём цикла от m_botSkill, hold ~15k м³); при доке `DepositCargoAtStation` материализует трюм как реальные entity в ангар станции (owner=бот).
- `c53c92f6` **раттеры сальважат свои вреки**: после победы `SalvageMyWrecks()` — находит вреки owner=свой корпус, забирает реальный лут (который DropLoot уже положил) в m_cargo, добавляет salvage-материалы (группа 754, 25588-25605), удаляет врек. Hold≥10k м³ → док.
- `7627f012` **упаковка реального склада в курьерки**: `PlaceStockCourierContractAt(sysID, stationID, charID, corpID)` — читает реальный склад бота (ownerID=charID, flagHangar, не singleton), грузит до 15k м³/6 типов, назначение = хаб (Jita), создаёт courier-контракт, реальные предметы замораживаются (ChangeOwner(1)) + пишутся в `ctrItems`. `ProcessDockedEconomy`: производители (Miner/RatHunter/Hacker/Explorer) и трейдеры шипят банк в хаб; трейдер при отсутствии реального груза — fallback на виртуальную курьерку. `CompleteContract` (561e0ca5) доставляет ctrItems в ангар issuer'а на конечной станции → товар физически едет между станциями.
- Ранее: `561e0ca5` courier `CompleteContract` (был объявлен-не-реализован) + forward-declare DockedBot; `21878084` видимость бот-курьерок в публичном поиске (джойны по item только при фильтре по содержимому).

### После `7627f012` (закрывают ISK-цикл + портал): коммиты `1e8a3020`, `5f0d4638`, `57c13750`, `f62e408f`
- `1e8a3020` **продажа реального склада в Джите**: `MarketMgr::SellStockIntoBuyOrder(botCharID, orderID, iRef, qty, stationID, typeID)` — без-клиентное зеркало ExecuteBuyOrder (оффлайн-продавец): товар из ангара бота уходит владельцу buy-ордера, escrow/покупатель платит боту (offline wallet), налог по Accounting, 2 mktTransactions. `BotMgr::SellStockAtHub(sysID, stationID, charID)` — бот, docked в торговом хабе, продаёт каждый тип своего склада в лучший (по цене) resting buy-ордер. `ProcessDockedEconomy`: у хаба → продажа, не у хаба → упаковка в курьерку. **ISK-цикл замкнут**.
- `5f0d4638` **флот-майнинг (гайд mmocenter)**: опытный майнер (skillTier≥3, 20%) спавнится флагманом на **Orca 28606** (империя) / **Rorqual 28352** (нули); `PlayerBot::IsFleetBoss/SetFleetBoss`; `GetFleetMiningBoost()` — баржа своего корпа в ~80 км от флагмана копает ×1.3 (mining foreman / industrial core эффект). cycleVol *= boost.
- `57c13750` фикс сборки: GetFleetMiningBoost не const (звал неконстантные SystemMgr()/GetPosition()).
- `f62e408f` **API CourierContracts.xml.aspx**: публичные courier-контракты (contractType=3, status=0, isPrivate=0) — contractid, fromstation/fromsystem, tostation/tosystem, volume, reward, issuer, itemcount + units (реальный груз из ctrItems/entity). Параметры limit/fromsystem/tosystem. Все атрибуты lowercase.

### ПРОВЕРЕНО на сервере (22:17, `57c13750`)
- Сервер стартует чисто, Vugl логинится. Арбитраж работает: на хабе 60001624 боты реально сводят сделки (Megacyte 40: 101.11 → 709.36, Damage Control II 2048: 103.29 → 715.62; двухуровневый спред между 3+ ботами). mktTransactions=234+ растёт. 17 ботов уже имеют tradeProfit (+3.8M ISK суммарно). Физический товар копится в ангарах: у ботов на 60001624 минералы (Trit 550/Nocx 633/Zyd 279/Megacyte 799). Курьерка живая: бот wy yang выставил Uemon→Jita (3608 м³, 194k ISK) — видна через API/портал.
- ⚠️ Включил в log.ini (на сервере, `/opt/evemu/config/log.ini`): `BOT__ERROR=1`, `BOT__MESSAGE=1`, `MARKET__MESSAGE=1` — иначе новые `_log(BOT__MESSAGE/MARKET__MESSAGE)` не пишутся (был ERROR-only). Дубль MARKET__MESSAGE в конце файла безвреден. Применится после рестарта сервера.

### 🔴 ФИКС courier-принятия (`a34850f1`): AcceptContract/CompleteContract передавали `PyInt*` как `%u`
- **Симптом (юзер)**: «Контракты не принимаются, в трюме появляется курьерская посылка, можно хоть 100 заспавнить» + «привёз в Джиту, сдать не могу». Accept создавал plastic wrap на каждый клик, но контракт оставался `status=0`, `crateID=0` → кликнуть можно бесконечно, а `GetCourierContractFromItemID` не находил контракт для сдачи.
- **Корень**: в `AcceptContract` (case 1 и case 3) и `CompleteContract` вызовы `sDatabase.RunQuery(err, "UPDATE ... WHERE contractId = %u", ..., contractID)` передавали **сырой `PyInt*`** (указатель) вместо `contractID->value()` → varargs-формат подставлял адрес указателя как ID → UPDATE не находил строку, контракт никогда не помечался принятым. Тот же баг в финальном SELECT (возврат клиенту). Исправлено `->value()` во всех 4 местах.
- **Проверено юзером**: после пересборки исполнил все 3 курьерки — контракты `status=4`, acceptorID/crateID записаны, wraps доставлены в конечные станции (Jita 60000361, Murethand 60012067, ... 60008185).
- «Кривое описание» в окне (`{[numeric]numJumps}`/`{locationLink}` сырые) — клиентская локализация маршрута, отдельный косметический вопрос (не связан с фиксом).

### Вечер (после `a34850f1`): коммит `25da872f`, чистка wraps, «кривое описание» = клиентское
- `25da872f` **автоудаление wrap после сдачи**: CompleteContract (case 4) теперь `Delete()` courier crate (typeID 3468) — раньше пустой plastic wrap оставался в ангаре навсегда («self-destruct» был только комментарием). ⚠️ **Сервер ещё НЕ пересобран на `25da872f`** (работает на `a34850f1`) — при следующем рестарте подхватится.
- **Чистка БД**: удалены осиротевшие plastic wraps Mr Tort `140265776` (60012067) и `140265777` (60008185) + 16 `entity_attributes` (поле itemID обнулено). У Mr Tort wraps = 0. Это были плоды бага `a34850f1` (каждый клик Accept создавал wrap).
- **«Кривое описание» — подтверждено КЛИЕНТСКОЕ** (юзер видит его в окне деталей открытого контракта): contracts_py.py:2636 строит `NumJumpsAway` через `pathfinder.GetJumpCountFromCurrent`, для удалённых конечных станций (Murethand и пр.) numJumps/locationLink не резолвятся → остаются сырые плейсхолдеры. Серверное описание чистое (`title='Courier shipment'`, `description='Standard courier contract'`). Не лечится с сервера — косметика клиента, оставили.

### Дальше (по плану юзера, не сделано / на завтра днём — «доделать остальные профы»)
- Доделать остальные профессии (юзер: «Днём пройдём доделаем остальные профы») — хакеры/эксплореры пока не производят реальный лут с сайтов (в отличие от майнеров/раттеров); проверить Hacker/Explorer/Courier активность и замкнуть их товар-цепочку.
- Флот-майнинг доделать: охрана у флагмана (guards у Orca), проверка ×1.3 буста на живом сервере.
- Портал: `/haul` страница курьерок уже задеплоена (nginx :80 отдаёт 200) — проверить снаружи через video.iks-online.net:26006, добавить при желании фильтры/детали.
- Понаблюдать: курьеры довозят груз в Джиту и товар продаётся (SellStockAtHub), прокачка skillLevel у активных ботов.

### 🎯 ЗАДАЧА: сдача Encounter-миссий у игроков не работает (полный разбор, НЕ сделано)
**Симптом**: принял encounter-миссию → прилетел в destination → убил врагов → у агента нет кнопки «сдать» (Complete), миссия висит.
**Разбор (5 сент., по коду — подтверждено):**
- `Client::IsMissionComplete` (`Client.cpp:2440`) для `Mission::Type::Encounter` — пустой скелет (`return false`). Courier учитывает destination+груз, всё остальное — false.
- `AgentBound` case Accept (`AgentBound.cpp:341-370`): миссионный данж спавнится через `MakeDungeon(sig, dungeonID)` если `qstEncounter.dungeonID>0`, НО `offer.dungeonLocationID`/`dungeonSolarSystemID` НЕ заполняются (колонки в `agtOffers`/`MissionOffer` есть) → нет связи «миссия ↔ её данж».
- `SpawnMgr::DoSpawnForMission` (`SpawnMgr.cpp:808`) — заглушка (`SetMission()` без спавна/подсчёта). NPC миссионных данжей идут через `DoSpawnForAnomaly` (ставит `SetAnomaly`) → в `SpawnKilled` (`SpawnMgr.cpp:233`) попадают в anomaly-ветку, а `else if (pBubble->IsMission())` (`:309`) — placeholder «not coded yet» (список TODO включает «setting mission completion status»).
- Счётчик живых NPC есть ТОЛЬКО для инкурсий: `m_incursionAlive[bubbleID]` (`SpawnMgr.cpp:321`), декремент в SpawnKilled::Incursion при смерти.
- `EncounterSpawnServer` (`missions/EncounterServer.*`, Service «encounterSpawnServer», регистрируется `eve-server.cpp:731`) — спроектирован хранить `spawnedEntities`/charID, НО `AddEncounter` нигде не вызывается из игрового пути (мёртвый код), а клиентская активация (`RequestActivateEncounters`) вызывается только из debug-окна. Есть `qaTools/encounterSpawnServer.cpp` — дубль-заглушка.
- **Блокер контента**: `qstEncounter.dungeonID>0` только у ~8 миссий (мы создавали); у ~2970 миссий dungeonID=0 → после Accept в destination-системе НЕТ ни данжа, ни NPC-целей → выполнять нечего. Нужен спавн целей (см. ниже).
**План фикса (по юзеру «полный охват»: данж + destination):**
1. Миграция: добавить в `agtOffers` колонки счётчика целей (напр. `missionNPCs` total / `missionNPCsKilled`) — по аналогии `botMemory` миграций, формат `-- +migrate Up/Down`.
2. `AgentBound` case Accept (Encounter, `typeID==Mission::Type::Encounter`): заполнить `offer.dungeonLocationID`/`dungeonSolarSystemID` из `sig.sigItemID`/`offer.destinationSystemID` после `MakeDungeon`; задать total целей; `UpdateOffer`.
3. Спавн целей: если dungeonID>0 — переиспользовать NPC комнат данжа; если 0 — спавнить кластер NPC-целей в destinationSystem (по образцу `EncounterSpawnServer::RequestActivateEncounters`: фракция 500014 Guristas default, `GetNPCsForFaction`/`GetCorpIDForFaction`, 4-6 NPC кластером) + помечать NPC меткой миссии (`customInfo` `mission:<offerID>`), регистрировать в `EncounterSpawnServer::AddEncounter`.
4. Смерть NPC: в `NPC::Killed` (где уже есть `pClient` из `damage.srcSE`, `NPC.cpp:608`) — если у погибшего NPC customInfo начинается с `mission:` и убийца = владелец оффера → инкремент `missionNPCsKilled` (UPDATE agtOffers); при достижении total оффер готов.
5. `Client::IsMissionComplete` Encounter: `missionNPCsKilled >= missionNPCs` (SELECT из agtOffers по offerID) — кнопка Complete появится.
6. (Опц.) `SpawnKilled::Mission` вместо placeholder.
**Проверка**: нужна живая сборка; зайти на encounter-миссию с dungeonID (8 шт) и без него, убить цели, сдать у агента. После — бот-миссионер (Missioner) сможет переиспользовать тот же механизм.



## 5 сентября: экономика челоботов (шевеление рынка) — дизайн/этапы
Юзер хочет полноценную живую экономику: трейдеры-боты **двигают рынок** (не мёртвые случайные ордера), с **самообучением** на прибыли/убытке; движение товара порождает спрос на **перевозку** → боты-курьеры и люди возят грузы между станциями, **курьерки на общем рынке**.

### Проблема текущей модели
- `BotMgr::PlaceBotOrderAt/PlaceBotBuyOrderAt` вставляют случайный sell+buy ордер (цена 100-900 ISK, qty 10-2000, рандомный товар) — НЕ привязаны к рынку, никем не исполняются (кроме редкой покупки игрока).
- `MarketBotMgr` (Trader Joe) тоже только ставит/снимает ордера, сделок сам не сводит.
- Итог: в `mktTransactions` только сделки реального игрока (Mr Tort покупал у станц. NPC-корп 1000016/1000014). Рынок статичен.
- Ключ: `ExecuteBuyOrder/ExecuteSellOrder` требуют `Client*` — автономного исполнения ордеров без клиента НЕТ.

### Целевая модель (согласовано с юзером)
1. **Арбитраж**: трейдер-бот на своей станции ищет пересечение цен по товару (чужой sell < чужой buy) и сводит сделку: покупает по sell-ордеру, продаёт по buy-ордеру, спред → баланс бота; объёмы списываются с ордеров, пишутся `mktTransactions` (реальные сделки!). Двигает цены/объёмы.
2. **Маркет-мейкинг**: если спреда нет — бот подрезает рынок (sell чуть ниже лучшей sell, buy чуть выше лучшей buy), схлопывая спред и двигая цену.
3. **Самообучение**: бот помнит результат своих сделок (прибыль/убыток, заполнялся ли его ордер) — агрессивнее после успеха, осторожнее после убытка; корректирует спред/цену. (botMemory: добавить поля `tradeProfit`/`tradeLosses`/последний спред?).
4. **Живой товар → перевозка**: накопление товара на станции (из-за торговли/производства/добычи) порождает **публичные courier-контракты** (на общий рынок, как сейчас `PlaceBotCourierContractAt`) между станциями — их берут боты-курьеры И игроки; курьеры реально перемещают груз (сущности между станциями), получая награду.
5. **Портал**: страница маркета уже есть (топы куплено/продано, покупатели/продавцы из `mktTransactions`), добавить курьерки/контракты.

### Технические замечания
- `mktTransactions`: transactionType bit — 1 = buy (memberID/characterID = покупатель), 0 = sell; каждая сделка = 2 зеркальные записи. Схема: transactionID, date, typeID, quantity, price, clientID, characterID, stationID, regionID.
- Владельцы ордеров сейчас почти все = NPC-корпорации станций (10000xx, seed `basePrice/security`). Челоботы-трейдеры ставят ордера со своим charID.
- `MarketMgr::ExecuteBuyOrder(seller Client*...)` — нужен путь исполнения без клиента (бот-покупатель/продавец = charID из chrCharacters, без Session).
- Индексы для ActiveSystems добавлены (`20260905000001`): chrCharacters.solarSystemID, entity(locationID,flag) — портал /systems был пуст из-за 5с таймаута при ~7с запросе.



## 3-4 сентября: Killboard/портал (полный аналог zkillboard) + челоботы-имитация игроков

**Новая архитектура: портал PHP читает ТОЛЬКО через API-сервер (`:26002`), НЕ в БД напрямую. Отдельный приватный репо `evemu-portal` (настройки в `C:\opencode-projects\evemu-portal`). Портал развёрнут юзером на другом хосте (`video.iks-online.net:26006`), конфиг `config.php` там свой (API_BASE наружу).**

### Ключевые уроки/законы API+портала
- **SimpleXML в PHP 8 регистрозависим** → ВСЕ элементы/атрибуты API обязаны быть **lowercase** (`accountid`, `solarsystemid`, `onlineplayers`, `serveronline`, `perpage`, `topkillers`…). НЕ добавлять camelCase теги.
- **Текстовые/блоб-поля в XML атрибутах экранировать** `xmlEscape()` (`<`,`>`,`&`,`"`,`'`) — killBlob содержит `<items>`.
- `sDatabase.RunQuery(res, ...)` принимает **`const char*`** (не `std::string`) — конкатенацию строить в `std::string q; ... q.c_str()`.
- **POST body API сервер читает из оставшегося буфера** (не `read_until` после заголовков): парсинг Content-Length+`\r\n\r\n`+`url_decode`+`+`→пробел в `APIServer.cpp`. Формы портала — POST.
- CCP логин: `password` колонка у клиент-аккаунтов ПУСТА, пароль хранится в `hash` = **PasswordHash(user,pass)** = SHA1(UTF-16 bytes pass + salt), 1000 итераций (`PasswordModule::GeneratePassHash`). Сравнение через HEX(hash).
- Роли: CCP битмаска `Acct::Role` (`EVE_Roles.h`): ADMIN=`0x0100000000000000` (72057594037927936), GMH=`0x20000000000000`, GML=`0x40000000000000`, WORLDMOD=4096. BOSS=0x63f8000280c41000 у Vugl. Портал использует эти значения.
- Изображения: **портреты/лого с нашего image server `:26001`** (`/Character/{id}_128.jpg`), иконки кораблей — `images.evetech.net` (`/types/{id}/render?size=N`, `/icon`) через кеш-прокси `img.php` (curl → file_get_contents fallback; zkillboard CDN блокирует серверный curl, НЕ использовать). `img.php` кеширует в `cache/` (давать www-data права на запись).

### Коммиты сервера (последовательность, все в origin/master)
- killmail челоботов (`38667602`): PlayerBot — NPC SE, смерть идёт через `PlayerBot::Killed→NPC::Killed`, запись была только в `ShipSE::Killed` → добавил `PlayerBot::RecordBotKillMail()` (до NPC::Killed) → `SaveKillOrLoss` + уведомление убийцы.
- не-боевые халлы (`b8d75376`): в BotMgr `base=0` + `PlayerBot::GetShipClass`→0 для Industrial/Freighter/TransportShip/MiningBarge/Exhumer/Shuttle/Capsule/IndustrialCommandShip/JumpFreighter/CapitalIndustrialShip → грузовики/баржи НЕ дерутся (бегство в `OnAttacked` уже было).
- онлайн счётчик (`9c4ab952`): `onlineplayers` = клиенты + активные челоботы в космосе + докнутые. Безопасно: `BotMgr::RefreshOnlineCount()` 1 раз/тик на игровом потоке в `std::atomic` поля; API их читает. Также killmail челобота синтезирует фит (High=оружие AttrGfxTurretID, Mid=AB/SSE, Low=DC) — реальных модулей у ботов нет.
- финал-удар дроном (`d418af65`): `finalShipTypeID`/корпа/альянс = ПИЛОТ (Nyx), оружие = дрон (Cyclops) — в `ShipSE::Killed` и `RecordBotKillMail`.
- KillDetail+MapData (`ec46ce28`): `KillDetail.xml.aspx?killid=` (полные corp/alliance/регион жертвы+убийцы), `MapData.xml.aspx?systemid=` (системы констелляции+jumps, констелляции региона, координаты x/z).
- TopKills→solarsystemname (`85e9cc5d`).
- TopValuables (`4bb81307`): топ киллов по оценке ISK = корпус + фит по `AVG(price)` из mktOrders (общий price-запрос для всех typeID пула).
- Activity (`920ac542`): Current Activity (distinct chars/corps/alliances/ships/systems/regions) + топ-списки за период.
- фикс .c_str() (`d0c2e655`).

### Коммиты портала (origin/master evemu-portal)
- роли CCP (`55b965c`), lowercase+роутинг /kill/{id} и пр., image cache прокси.
- kill detail (`91b7a50`): через KillDetail (корпы/альянсы/регион), справа SVG-карты System/Constellation/Region (`render_minimap`, точки по x/z, sec-цвет), EFT + Original Killmail + Related Kills.
- home (`e2ec7c0`): zkillboard — 3 блока по 6 больших карточек (Most Valuable Ships / Structures / Sponsored) с рендером, именем жертвы и `isk_compact()` стоимостью (`57.86b`); сайдбар Current Activity + Top Characters/Corps/Alliances/Ships/Systems; Recent Kills.
- Админка: аккаунты/бан, петиции, таймкоды, выдача предметов, роли — сделано ранее (auth/admin API), требует донастройки ролей.
- `img.php` маппинг только evetech/eveonline/zkillboard; kость картинок — curl сначала.

### Челоботы — НЕРЕШЁНО/проверка после пересборки
- Пересобрать сервер на `d0c2e655`, портал `git pull e2ec7c0`; проверить: главную (карточки+ISK, сайдбар), детальный килл (корпы/альянсы/карты/related), онлайн с челоботами, логин (CCP PasswordHash), киллы челоботов (должны писаться + фит в слотах).
- "Баржи/грузовики бросаются в самоубийственные атаки" — зафикшено класс 0/урон 0; если всё ещё атакуют после пересборки → включить BOT__TRACE/NPC__AI_TRACE, найти какой путь (assist/флот-саппорт/НПСAI) толкает их в бой.
- Полная неотличимость челобота (реальные модули-предметы в вреке как лут, а не синтетика в killBlob) — НЕ сделано, по желанию: спавнить модули в корабль + перенос в врек при гибели + чистка при деспавне.
- Иконки/портреты: evetech и наш image server работают; проверить после ребилда.

## 2 сентября (вечер): siege fix, turret chgTypeID, LXQ2-T cleanup, character restore, KillMails API

### Siege module passive-effect fix (`675c3a85`)
- **Корень:** `OnModuleOnline()` сохранял `m_savedMaxVelocity` ПОСЛЕ `sFxProc.ApplyEffects()` уже обнулл AttrMaxVelocity через пассивный эффект `siegeModeEffect6` (effectCategory=1, speedFactor=-100%). При деактивации `OnModuleOffline()` восстанавливал из `m_savedMaxVelocity=0`.
- **Фикс:** Убрано ручное управление AttrMaxVelocity/warpScrambleStatus в OnModuleOnline/Offline. Эффект-система теперь полностью управляет через `siegeModeEffect6` (ApplyEffects/RemoveItemModifier). OnModuleOnline/Offline только отключают/включают propulsion модули и обновляют destiny manager.

### Turret chgTypeID fix (`8dc4372b`)
- **Корень:** `ShowEffect()` отправлял `chgTypeID = m_modRef->typeID()` когда `m_chargeRef = mModRef` (туррель без заряда). Клиент `StandardWeapon.Start()` крэшился при `SetAmmoColorByTypeID(turretTypeID)`.
- **Фикс:** `chgTypeID = 0` когда нет заряда. Клиент получает `None` для `otherTypeID`.

### LXQ2-T client crash
- 258 далёких кораблей ботов (~2.46 трлн м от центра) → float32 precision → краш клиента. Очищено.
- **ПРЕДУПРЕЖДЕНИЕ:** При чистке случайно удалена Revelation (140214608) и каспула (140152711).

### Character restore
- Revelation (140214608), каспула (140152711) восстановлены из бекапа `evemu_20260902_0400.sql.gz`
- chrCharacters: shipID=140214608, capsuleID=140152711, locationID=60001795
- **Важно:** character entity НЕ хранится в `entity` таблице — только в `chrCharacters`.

### Осталось (после пересборки)
- Пересобрать сервер на `675c3a85` и проверить осадный модуль на дредах
- Анимация лазерных турелей Revelation — `GetEffectGuid`/gfxID для X-Large Energy Turret
- LXQ2-T: 301 сущность, ~30 ботов, физические позиции нормальные
- **KillMails.xml.aspx API** — реализован в `APICharacterManager` + `APICorporationManager`. Запрашивает `chrKillTable` и отдаёт XML в формате, который понимает zKillboard (pheal). Нужна пересборка + настройка zKillboard.

## 29 августа (вечер): siege/triage/industrial core, effects, TCU crash, combat log, Revelation
**Коммиты: `df8e4dda`...`d4a4c87a`. Сервер пересобран с GDB=TRUE для ловли крашей.**

### Siege/Triage/Industrial Core — полная реализация
- **Siege Module I/II** (20280/4292): x7/x8.4 дамаг, неподвижность, +100% ремонт, блок remote reps
- **Triage Module I/II** (27951/4294): неподвижность, +100% ремонт, дроны off (логистические работают), ECM off, T2: -20% cap cost
- **Industrial Core I** (28583): неподвижность, x10 масса, +400% mining drones, compression
- **EnforceSiegeEffects()** в `ProcessModules()` — каждый тик проверяет и восстанавливает velocity=0 если effects-система перезаписала
- **OnModuleOnline()/OnModuleOffline()** — эффекты применяются при включении/выключении модуля
- **DestinyManager::Stop()** + **SetSpeedFraction(1.0f)** — корабль реально останавливается и восстанавливает скорость после деактивации

### Effects
- **Marshal table**: `effects.Laser` (index 171) НЕ менять — клиент ожидает `effects.Laser`
- **NPC код**: `effects.StandardWeapon` отправляется как literal string (не через marshal table) → клиент получает правильно
- **GetEffectGuid override**: turret effects → `effects.StandardWeapon` (безопасно для marshal)
- **TurretEffectID fallback**: если effectID=0 → inference по groupID (Energy_Weapon→targetAttack, Projectile_Weapon→projectileFired)

### TCU crash
- `StructureSE::Killed` — `m_moonSE->GetID()` на nullptr. Null-guard добавлен.

### Combat log
- `OnDamageMessage` отправляется сервером через `QueueDestinyEvent`
- Клиент偶尔 не показывает — marshal table `effects.Laser` НЕ менять (обратная совместимость)

### Revelation — РЕШЕНО (01 сент): "The Dreadnought cannot be fitted."
- **Симптом**: клиент Crucible отклоняет `ActivateShip` для typeID 19720 ("Невозможно установить Дредноут на корабль"). Только Revelation; Moros/Naglfar/Phoenix (дреды), Nyx/Thanatos работают.
- **КОРЕНЬ**: effect 1626 (`dreadnoughtShipBonusLaserCapNeedA1`) ссылался на `preExpression=6466`/`postExpression=6467`, которых **НЕТ в `dgmExpressions`** (и не было в publich Crucible SDE — Fuzzwork `dgmExpressions` не содержит). Клиент при Make Ship Active: `StartPassiveEffects` → `StartEffect(1626)` → exception (нет expression tree) → `FitItemToLocation` → `UserError('ModuleFitFailed')` → "The Dreadnought cannot be fitted." Дополнительно: серверный `FxProc::ParseExpression()` падал `opATTR called with no expressionAttributeID defined`.
- **✅ Фикс (миграция `20260901000000-revelation_dreadnought_bonus_expressions.sql`)**: созданы expressions 19279-19283 — дерево повторяет рабочий 1627 (`dreadnoughtShipBonusLaserRofA2`), но таргетирует `capacitorNeed` (attr 6) вместо `speed` (attr 51) на модулях X-Large Energy Turret:
  - 19279: константа `capacitorNeed` (operand 22, `expressionAttributeID=6` ← критично, без него FxProc падал)
  - 19280: ATT `CurrentShip[X-Large Energy Turret]->capacitorNeed` (operand 12, arg1=6007 LRS, arg2=19279)
  - 19281: EFF PostPercent (operand 31, arg1=1095, arg2=19280)
  - 19282/19283: ALRSM/RLRSM (operand 9/61, arg1=19281, arg2=6422 `dreadnoughtShipBonusA1`)
  - `dgmEffects`: 1626 → pre=19282, post=19283; 1627 → восстановлен (pre=6469, post=6470)
- **ВАЖНО про `expressionAttributeID`**: константы (operand 22) должны нести ID атрибута (651→51 speed, 6422→875, 6423→876, 6426→879). ATT/EFF/ALRSM/RLRSM выражения имеют `expressionAttributeID=0` — это норма. Без ID на константе серверный FxProc падает и атрибуты не считаются.
- **ВАЖНО**: публичный Crucible SDE (Fuzzwork `cru16`) НЕ включает `dgmExpressions` — только `dgmEffects` с ссылками pre/post. Часть выражений из нашей БД (6466/6467) отсутствовала из-за неполного импорта.

### Осталось
- Дождаться GDB backtrace при следующем TCU crash
- Проверить siege mode полностью (топливо, дамаг, неподвижность)
- Комбат лог — клиент偶尔 не показывает (marshal table compatibility)

## 2 сентября — задачи НА ЗАВТРА (записал юзер, проверено им)
- **🔴 Siege module — НЕРЕШЕНО (на ВСЕХ дредах, не только Revelation)**: с установленным осадным модулем корабль НЕ варпает и скорость 7 м/с. «Вылечивается» полным циклом вкл/выкл модуля, но НЕ всегда. Связано с эффектами осады (неподвижность/скорость) — вероятно `EnforceSiegeEffects()` или `OnModuleOffline()` не всегда сбрасывает WarpScramble/velocity. Копать в `ActiveModule` siege-кейсе + `DestinyManager`.
  - **⚠️ ПОДСКАЗКА (01 сент, от юзера)**: баг срабатывает **от ПРОСТО УСТАНОВЛЕННОГО модуля** (не обязательно включённого) — после телепорта в любую систему варп мёртв и скорость 7 м/с. Это НЕ только вкл/выкл: похоже siege-модуль в фите накладывает ПОСТОЯННЫЙ модификатор (speed-кап до 7 м/с + блок варпа), который не снимается при смене систем/снятии с экипажа. Искать пассивный эффект модуля (dgmTypeEffects на Siege_Module: `siegeMode`/speed-модификаторы) и как сервер применяет его к кораблю даже в offline-состоянии.
- **🔴 Анимация лазерных турелей на Revelation — ОТСУТСТВУЕТ**: корабль (Revelation, дредноут) стреляет/работает, но лазерные турели не анимируются. Связано с `GetEffectGuid`/gfxID для капитальных энергетических турелей (X-Large Energy Turret). Копать в turret-эффектах Revelation vs обычные корабли.

## 28 августа (день): суициды ботов на Никс, fighter-bomber пейнтеры, StructureSE::Killed, PyRep leak-аудит (dtor clear)
**Коммиты: `541fb0c9` (bots skip capital drones + missile formula), `63ca913d` (fighter-bomber ReturnBay + stale re-engage), `6e6a977b` (build fixes), `58f51f1f` (PyRep dtor clears), `eff057e6` (Multicast clear), `042158b1` (Encode null + revert), `6659d44d` (revert ~PyResult), `9041fe35` (revert ~PyPacket clear). Сборка прошла, сервер работает.**

### Боты vs Никс (суицидальные атаки фрегатов)
- **✅ `FindAggroTarget` skip capital drones (`541fb0c9`)**: когда файтеры Никса атакуют фрегат-бота, бот оценивает ФАЙТЕР (class 2), а не Никс (class 6). Бот с m_botSkill≥4 решает что赢 fight → летит к файтеру → умирает от главного оружия Никса. Фикс: `FindAggroTarget` проверяет владельца дрона — если это carrier/supercarrier/titan (groupID 547/659/30), цель пропускается. Используются прямые константы `EVEDB::invGroups` (PlayerBot::GetShipClass не доступен из DroneAIMgr).
- **✅ `OnAttacked` power assessment**: `CountEnemiesNearby` считает ownersNearShip как врагов, но НЕ считает владельца дрона-агрессора. С built-in +4 (supercarrier) +3 (fighters) +3 (enemies) power evaluation correctly rejects Nyx fights for most skill levels.

### Fighter-bomber: пейнтеры + deep-space teleport
- **✅ FighterBomberAttack missile formula (`541fb0c9`)**: бомбы файтер-бомберов наносили сырой урон БЕЗ формулы ракет (Sr/Er/Ev/V). Пейнтеры увеличивали sig radius, но сервер не использовал его в расчёте. Фикс: добавлена `Missile::HitTarget` формула в `FighterBomberAttack` с guard'ами на existence атрибутов (AttrAoeCloudSize и пр.).
- **✅ Deep-space teleport fix (`63ca913d`)**: `SetIdle()` перезаряжал bomber'ы в космосе (строки 540-544) + перенацеливал на устаревшую цель (строки 550-558, TargetMgr не очищался) → бомбер летел к далёкой цели. Фикс: (1) `FighterBomberAttack` вызывает `ReturnBay()` (return to bay) вместо `Return()` при патроне 0; (2) SetIdle убрана автоперезарядка бомберов (перезаряжаются только в трюме) + убрано перенацеливание для бомберов.

### StructureSE::Killed — краш при убийстве ТКУ
- **✅ `StructureSE::Killed` ClearFromTargets (`541fb0c9`)**: `StructureSE::Killed` OVERRIDES base `SystemEntity::Killed` и НЕ вызывал `ClearFromTargets()` → дроны/NPC хранили dangling pointer на мёртвую структуру → segfault в `DroneAI::Process` Engaged handler (line 283 `pTarget->SysBubble()`). Фикс: `m_targMgr->Destroyed()` + `m_targMgr->ClearFromTargets()` в начале `StructureSE::Killed`. Аналогичный guard в `ObjectSystemEntity::Killed` НЕ нужен ( structures removing via Delete() path).

### PyRep leak-аудит (итог)
- **✅ `EntityList::Multicast` clear (`eff057e6`)**: 2 перегрузки Multicast (character_set и MulticastTarget) вызывали `PyDecRef(payload)` БЕЗ `payload->clear()` → items leak. Фикс: добавлен `payload->clear()` перед DecRef (первая перегрузка уже имела).
- **✅ `Encode()` null pointers (`042158b1`)**: `PyPacket::Encode()` крал `payload`/`named_payload` в `arg_tuple->items[]` БЕЗ IncRef → dual ownership с `~PyPacket` dtor → double-free. Фикс: `payload = nullptr; named_payload = nullptr;` после кражи.
- **🔴 Деструкторы PyTuple/PyList/PyDict НЕ освобождают items**: `~PyPacket clear()` откачен — `PySubStream` хранит borrowed rep БЕЗ IncRef → clear() каскадно освобождает → `~PyResult`/`~PyObject` потом обращаются к freed memory. Подтверждено на логине: crash в `Handle_CallReq` → `_SendCallReturn` → `PySubStream(rsp.ssResult)` → `~PyPacket clear()` → `~PySubStream` → `PySafeDecRef(ssResult)` → `~PyResult` → access freed. **Системный фикс dtor'ов ОТКЛОНЁН** (ATMOS-AGENTS.md, корень — Encode() без IncRef). Оставлены: Encode null fix + Multicast clear. Утечки в dtor'ахACCEPTABLE (контейнеры маленькие, редкие path-ы).

## Нерешённое (28 авг)
- **🔴 Revelation/Dreadnought — модули пропадают после дока**: uniquely to Dreadnought group (485). Nyx (659) работает нормально. Слоты в БД ок (Hi=8/Mid=4/Low=4). Проверить `invTypes` для Revelation: `categoryID`, `groupID`, `radius`, `mass`. Вероятно船а не распознается как Ship (categoryID≠6) или отсутствуют критичные атрибуты. siege modules (Siege_Module group) registered as ActiveModule — empty case block (no activation effects, expected for mode-change modules). *(Активация/boarding Revelation — РЕШЕНО 01 сент, см. секцию 29 авг — expressions 19279-19283. Модули после дока — отдельный вопрос.)*
- **Диагностика**: `SELECT typeID, typeName, groupID, categoryID, radius, mass, capacity FROM invTypes WHERE typeName LIKE '%Revelation%';`
- **PyRep leak-аудит**: точечные fix'ы в конкретных call sites (clear() перед DecRef) вместо системных dtor'ов. Высокотрафичные пути: Handle_CallReq, _SendCallReturn, SendNotification, Multicast — проверены. Низкотрафичные: Agent/Contract/Map/Calendar — НЕ проверены.

## 28 августа (ночь): PyRep Leak-аудит — исходящий сетевой путь (QueuePacket/Handle_CallReq)
**Коммиты: `6295bccc` (CreateNotification leak), `d755f32f`+`deebe9cf` (clear() перед DecRef в CorpNotify/Broadcast/Multicast), `134457b5` (QueuePacket delete + _SendCallReturn refcount named_payload), `bdb7285e` (docs). Запушено в origin/master. Юзеру нужна ПЕРЕСБОРКА сервера.**
- **✅ `QueuePacket` не удалял packet (`134457b5`)**: каждый исходящий RPC/notify строил `PyPacket` (payload/named_payload), `QueuePacket` кодировал его в wrapper через `scn.Encode()`, ставил в очередь — но НЕ удалял `PyPacket` → утечка на каждом ответе/уведомлении. Фикс: `SafeDelete(packet)` в `EVEClientSession::QueuePacket` в ОБЕИХ ветках (после `mNet->QueueRep(res)` и при `res == nullptr`).
- **✅ `_SendCallReturn` — double-release named_payload (`134457b5`)**: `packet->named_payload = rsp.ssNamedResult` (borrowed из локального `PyResult`). С delete-packet dtor `PyPacket` делает `PySafeDecRef(named_payload)`, а dtor `PyResult` в `Handle_CallReq` тоже DecRef'ит ssNamedResult → use-after-free на каждом ответе. Фикс: `PySafeIncRef(rsp.ssNamedResult)` перед присваиванием. Для `ssResult` НЕ нужен: `packet->payload->SetItem(0, new PySubStream(rsp.ssResult))` уже IncRef'ит PySubStream внутри `PyTuple::SetItem` (PyRep.h:626-636) → delete-packet снимает только реф с обёртки, не с ssResult.
- **Модель владения (доказано по коду)**: dtor PyTuple/PyList НЕ освобождают items (PyRep.h:647, PyRep.cpp:698-702) → `delete packet` может снять рефы ТОЛЬКО с top-level payload/named_payload; для codec-сайтов `scn.Encode()` возвращает новый объект (реф владельца = packet), вложенные items (scn.changes и пр.) остаются за `scn` — двойного освобождения нет.
- **`QueueRep` синхронен** (EVETCPConnection.cpp:50-83: MarshalDeflate → Send → `PySafeDecRef(res)`) — удаление packet после возврата безопасно.
- **Все 8 call sites `QueuePacket` проверены** (Client.cpp 2731/2798/3018/3257/3282/3306/3371, EVESession.cpp:66): все передают fresh-объекты, совместимы с delete packet. В `SendSessionChange` (Client.cpp:2798) удалён мёртвый `//SafeDelete(packet);` (комментарий обновлён).
- **Продолжение аудита**: следующий кандидат — входящий путь `EVEClientSession::Process`/`Handle_CallReq` и `EVENetwork_StreamDecoder`-объекты; системный фикс dtor'ов PyRep-контейнеров ОСТАЁТСЯ опасен (см. ниже, «Системная проблема»).

## 27 августа (день): орбита от поверхности, турели по роли, аналитика ботов, чат, самооборона PvP
**Коммиты: `a12d4095` (орбита), `0c690a5c`+`84ef3e01` (турели), `7673c9e6`+`5ce0cb67` (аналитика силы), `be6c8571`+`734bcb90` (чат), `08b54e29`+`f9aefd79`+`ff78ea34` (самооборона), `30d8ac78` (docs). Образ собран, сервер перезапущен, работает.**
- **✅ Орбита вокруг гейтов/станций (`a12d4095`)**: EVE-орбита отсчитывается от ПОВЕРХНОСТИ структуры, а код трактовал как дистанцию от ЦЕНТРА. Гейт radius 3532-14051 м → команда 2500 (и 15км) ставила корабль ВНУТРИ гейта → push-out обнулял скорость. Фикс: `Orbit()` добавляет `pSE->GetRadius()` для гейтов/станций/планет/лун.
- **✅ Турели стационарные + тип атаки по роли (`0c690a5c`+`84ef3e01`)**: группы 99/180/336/383/417 → `m_isStationary` (не летают/орбитят). По роли: Tower Sentry = турельный урон; Stasis Tower = web (сила из AttrSpeedFactor, чанс нормализован: турели проценты 100/50, NPC float 0-1); Energy Neutralizer = cap drain (EnergyDestabilization); Missile Battery = реальные ракеты (chargeGroup→typeID). Чисто-EWAR урона не наносят.
- **✅ Файтер-бомберы всегда попадают (`74221cb1`)**: SDE-бомберы (группа 1023) не имеют tracking/falloff → `GetDroneToHit` делил на ноль → «misses (too far away)» каждый выстрел. Фикс: toHit=1.0 (бомбы взрываются при подлёте). Юзер подтвердил: файтеры работают отлично.
- **✅ Аналитическая оценка силы (`7673c9e6`+`5ce0cb67`)**: юзер «челоботы кидаются на никс с бомберами при шансе 0». (1) дроны/файтеры игрока атакуются ТОЛЬКО если HunterWouldEngage на корабль-владельца; (2) analytic (не точный) assessment: капы +4 (цино/флот), баттлшипы +1 (assume fitted), дроны/файтеры в космосе +3 (файтер-скрин). Во всех 3 местах (HunterWouldEngage/OnAttacked/HuntForTarget).
- **✅ Минимальный анализ чата (`be6c8571`+`734bcb90`)**: промпт DeepSeek несёт интент сообщения (вопрос/приветствие/помощь/флот/оскорбление по EN+RU эвристике) → бот отвечает по теме. + systemHint знает, если игрок в том же гриде (не раскрывает «я один в аномалии») и в бою ли бот.
- **✅ Самооборона (PvP и боты) (`08b54e29`+`f9aefd79`+`ff78ea34`)**: юзер «челобот атаковал первый, не вешается ли агрессия на меня». КОРЕНЬ: ЛЮБОЙ урон по атакующему флагал стреляющего — жертва за самооборону получала агрессию. Фикс: первый атакующий записывается (`RegisterAttackBy`), ответный огонь жертвы НЕ флагает (только инициатор). Единый механизм для игроков (charID) и челоботов (botCharID). Ответный огонь по ганкеру — без агрессии.
- **Проверено юзером**: таймеры агрессии (док не дало, сентри сагрились); MWD после анчоринга работает; файтеры бьют с уроном; бой на аномалиях (NPC+челоботы) виден.
- **⚠️ ASAN-сборка НЕ применима для ежедневной работы**: eve-xmlpktgen линкует ASAN-eve-core → libasan перехватывает malloc → генерация висит >15 мин. Сервер на ОБЫЧНОМ образе. ASAN-аудит уже дал 4 фикса (XMLParser vdtor, PyLong/PyFloat hash, AssignAt memcpy, InvBrokerBound static_cast). Возобновить при необходимости с не-ASAN eve-core для генератора.

## 27 августа (ночь/утро): docker-compose починен, Memory Mgmt аудит + Этап 0 (RefPtr укрепление)
**Коммиты: `2c5a71d7`+`efd4d09b`+`0936987a` (юзер: quiet logs, dockerignore, binlog), `b4f424b9` (RefPtr uint16→uint32 + диагностика). Образ собран, сервер перезапущен через compose, работает.**
- **✅ docker-compose работает**: причина — старые контейнеры `server`/`db` были созданы вручную (`docker run`), compose-проект про них не знал и конфликтовал по имени. Фикс: `docker-compose down` (удалил старый db, volume `evemu_db` external — данные целы) + `docker rm server` + `docker-compose up -d`. Теперь `down`/`up` работают штатно. НО: `docker-compose up -d --force-recreate` падает `KeyError: 'ContainerConfig'` (compose 1.29.2 vs docker 29.1.3 при чтении конфига старого контейнера) — паттерн: `docker rm -f server` затем `docker-compose up -d`.
- **🔴 КОРЕНЬ KeyError: ContainerConfig**: старый баг, упомянутый в AGENTS.md — воспроизводится только при `--force-recreate`/чтении image_config существующего контейнера. Обычный `up -d` работает (контейнеры под управлением compose).
- **✅ Memory Mgmt аудит (полный PyRep→shared_ptr НЕ реалистичен)**: RefObject/RefPtr — самописный счётчик (eve-core/memory/RefPtr.h). Масштаб перевода: `new Py*`=4281, сырых `Py*`=6443, `PyIncRef/PyDecRef`=552, `PyStatic`=1145, ручных delete в eve-common=199. Вывод: полный перевод = ~6000 мест в ~100 файлах, недели работы, высокий риск. ВМЕСТО него — **Этап 0 усиленный**: укрепить RefObject + ASAN-сборка для адресного поиска UAF, потом лечить найденные баги локально (как QueueDestinyUpdate/AddBalls2).
- **✅ Этап 0 выполнен (`b4f424b9`)**: (1) `mRefCount` uint16→uint32 — устранено переполнение счётчика на 65535 рефов (реальный класс багов на проде); (2) диагностика mDeleted-костыля разделена на IncRef/DecRef (раньше DecRef печатал «IncRef()»); (3) opt-in `REFPTR_HARD_FAIL` (abort при use-after-free) для ASAN/debug-сборок, default остаётся не-крэшащим. REFPTR__ERROR=1 в log.ini, за 13ч прогона 0 срабатываний — текущий код чист по этому костылю.
- **✅ Этап 0.2 (ASAN) — сборка готова + найден реальный баг**: Dockerfile получил build-arg `CMAKE_CXX_FLAGS_EXTRA` (`2d9a4acf`) для прокидывания `-fsanitize=address,undefined -DREFPTR_HARD_FAIL -fno-omit-frame-pointer -O1 -g`. Образ `evemu_server_asan` собран (бинарь ~770 МБ, libasan.so.8). ⚠️ Первая сборка падала DEADLYSIGNAL — это OOM от `make -j$(nproc)` (ASAN-компиляция ~1-2 ГБ/процесс). **🔴 Баг, найденный ASAN (фикс `9345edd1`)**: `XMLParser::ElementParser` имеет виртуальный `Parse()`, но НЕ виртуальный деструктор → `ClearParsers` удаляет производные парсеры через базовый указатель (`Generator` 32 байта как `ElementParser` 8 байт) → new-delete-type-mismatch, порча кучи при каждом запуске генератора пакетов (eve-xmlpktgen). Добавлен `virtual ~ElementParser()`. ASAN окупился на первом же запуске сборки.
- **✅ Этап 0.2 — UBSAN-баги найдены и разобраны**: UBSAN флагал misaligned-доступы в `Buffer::AssignAt`/`const_iterator::operator*` (каждый маршалинг/cache-сборка) и signed-shift overflow в `PyLong::hash`/`PyFloat::hash`. Разбор: (1) misaligned-доступы — **фундамент формата** (EVE marshal пакует значения вплотную без padding), на x86 безопасны → оставлены как есть, UBSAN-alignment выключен (`-fno-sanitize=alignment`); `AssignAt` переведён на memcpy (`85e6a83d`, безопасно для записи); (2) `PyLong::hash`/`PyFloat::hash` — **реальные баги** (signed left-shift overflow, UB) → переведены на unsigned-арифметику (`01f2d302`, по CPython-эталону). ⚠️ Урок: возврат `operator*` по значению НЕ сработал (Get<T>/operator[]/Deflate `&input[0]` требуют lvalue) — откачен (`0a360f1d`).
- **✅ Этап 0.2 — eve-xmlpktgen исключён из ASAN (`953f811e`)**: ASAN-инструментированный генератор пакетов работал >15 мин на один файл (вис сборки). Таргет получает флаги без санитайзеров через `SET_TARGET_PROPERTIES COMPILE_FLAGS` (строка из CMAKE_CXX_FLAGS с вырезанными `-fsanitize`). Runtime eve-server остаётся полностдью санитайзерным.
- **🔴 UBSAN-баг BoundService (фикс `c581b308`)**: «member call on address which does not point to an object of type BoundServiceParent» при `InvBrokerBound::Release`. КОРЕНЬ: `InvBrokerService` наследует `BindableService<InvBrokerService, InvBrokerBound>` (который наследует `BoundServiceParent<InvBrokerBound>`) **И** `BoundServiceParent<InventoryBound>` напрямую — ДВА разных базовых под-объекта. `InvBrokerBound::GetInventory` создавал `InventoryBound` через `reinterpret_cast<BoundServiceParent<InventoryBound>&>(this->GetParent())`, а `GetParent()` возвращает `BoundServiceParent<InvBrokerBound>&` → каст между разными под-объектами = UB. Фикс: `InvBrokerBound` хранит `InvBrokerService& m_service`, создание InventoryBound через `static_cast<BoundServiceParent<InventoryBound>&>(m_service)`. Проверено: 0 runtime errors на ASAN-сервере после фикса.
- **✅ Фикс промахов файтер-бомберов (`74221cb1`)**: юзер «лог боя пишет далекий промах, хотя я вплотную». КОРЕНЬ: файтер-бомберы (группа 1023, SDE-холлы Cyclops/Malleus/...) — AoE-боеприпасы: имеют `AttrEntityAttackRange`=20000, `thermalDamage`=3000, `aoeCloudSize`/`proximityRange`, но НЕ имеют `AttrTrackingSpeed`/`AttrFalloff`/`AttrOptimalSigRadius` (это не турели). `GetDroneToHit` делил на ноль → toHit=0 → combat log «misses completely (too far away)» на каждый выстрел бомбера, независимо от дистанции. Фикс: `FighterBomberAttack` применяет toHit=1.0 (бомбы взрываются при подлёте — always hit). Обычные файтеры/дроны (tracking/falloff есть) сохраняют турельную формулу. Юзер подтвердил: файтеры работают отлично.
- **✅ Аналитическая оценка силы челоботов (`7673c9e6`+`5ce0cb67`)**: юзер «челоботы кидаются на никс с бомберами при шансе выжить 0». ДВЕ причины: (1) `Target player drones` в NPCAI Idle-скане атаковал ЛЮБОЙ дрон/файтер игрока без оценки владельца — бот суицидился на бомберы Никса; (2) оценка силы была «точной» по классу корпуса (Nyx=6), а юзер требует «аналитической»: пилот не знает фит, но исходит из того, что Никс МОЖЕТ нести файтеры. Фиксы: (1) челобот атакует дрон/файтер игрока ТОЛЬКО если HunterWouldEngage на корабль владельца; (2) analytic assessment во всех 3 местах (HunterWouldEngage/OnAttacked/HuntForTarget): кап-классы (Carrier/Supercarrier/Titan) +4 power (может зажечь цино/дропнуть флот, редко один), баттлшипы/BlackOps/Marauders +1 (assume fitted), дроны/файтеры уже в космосе +3 (реальный файтер-скрин).
- **✅ Минимальный анализ чата (`be6c8571`)**: юзер «M'yara > solo PvP or nothing челоботы применяют к месту и без». КОРЕНЬ: промпт DeepSeek был `senderName says: message` без понимания интента → бот отвечал случайной фразой на любой реплике. Фикс: эвристика (вопрос/приветствие/помощь/флот/оскорбление/реплика по ключевым словам EN+RU) добавляет в промпт указание «это ВОПРОС — ответь по существу», «это ПРИВЕТСТВИЕ — поздоровайся» и т.п.
- **✅ Фикс сборки ASAN (`953f811e`+`bf52c743`)**: eve-xmlpktgen под ASAN работал >15 мин/файл (вис). `SET_TARGET_PROPERTIES COMPILE_FLAGS "-fno-sanitize=..."` — потому что COMPILE_FLAGS **дополняет** CMAKE_CXX_FLAGS, его нельзя вырезать из флагов, только отключить через -fno-sanitize. LINK_FLAGS оставлены с санитайзером (eve-xmlpktgen линкует ASAN-eve-core → нужен libasan).
- **✅ Орбита вокруг гейтов/станций (`a12d4095`)**: юзер «подлетаю к гейту на ибисе, орбита 2500 — гейт дергает, скорость 0; и 15 км тоже». КОРЕНЬ: EVE-орбита отсчитывается от ПОВЕРХНОСТИ структуры, а код трактовал команду как дистанцию от ЦЕНТРА. Гейт radius 3532-14051 м (Paala ~19 км) → команда 2500 (и даже 15000) ставила центр корабля ВНУТРИ гейта → collision push-out (ProcessState) обнулял скорость каждый тик («гейт дергает»). Фикс: `Orbit()` добавляет `pSE->GetRadius()` к командованной дистанции для гейтов/станций/планет/лун (орбита от поверхности, как в live EVE).
- **✅ Турели стационарные + тип атаки по роли (`0c690a5c`+`84ef3e01`)**: юзер «башни не летают, это статическое сооружение». КОРЕНЬ: NPC-турели спавнились как обычные `new NPC` с NPCAIMgr — двигались/орбитили/преследовали (flyRange fallback radius*5>0). Фикс: группы Sentry Gun (99), Protective (180), Mobile (336), Destructible Sentry Gun (383), Mobile Missile Sentry (417) → `m_isStationary` (не Wander/Chasing/Following/Orbit, только атака с места). Тип атаки по роли башни: Tower Sentry/Sentry Guns = прямой турельный урон; Stasis Towers = только web (сила из реального AttrSpeedFactor, -75% у Sansha; чанс нормализован: турели хранят проценты 100/50, NPC float 0-1); Energy Neutralizer Sentry = только cap drain (новый EnergyDestabilization-эффект, атрибуты entityCapacitorDrain*); Missile Battery (417) = реальные ракеты (`PickMissileForChargeGroup`: chargeGroup 384 Light→210, 385 Heavy→209, 386 Cruise→203, 89 Torpedo→267, 476 Citadel→2678). Чисто-EWAR башни урона не наносят.
- **⚠️ Запуск ASAN-контейнера БЕЗ `-t -i` даёт спам «Command not recognized» и DEADLYSIGNAL в логах** — не путать с крашем. Обязательно `docker run -d -t -i`.
- **⚠️ ASAN-сборка НЕ применима для ежедневной работы**: `-fno-sanitize` на eve-xmlpktgen не спасает — генератор линкует ASAN-инструментированный eve-core → libasan перехватывает malloc → генерация пакетов висит >15 мин. Сервер сейчас на ОБЫЧНОМ образе (`evemu_server`, собран быстро, все фиксы внутри). ASAN-аудит уже окупился (XMLParser vdtor `9345edd1`, PyLong/PyFloat hash `01f2d302`, AssignAt memcpy `85e6a83d`, InvBrokerBound static_cast `c581b308`) — при необходимости возобновить с отдельным не-ASAN-сбором eve-core для генератора.

## 26 августа (вечер, инфраструктура): stale-флаг аккаунта, binlog+бэкапы, compose починен, логи снова ERROR-only
**Коммиты: `7fa3c457` (stale account online), `0936987a` (binlog+бэкап), `efd4d09b` (диагностика, потом отключена). Сервер пересоздан юзером через `docker-compose up -d` (build+up), работает.**
- **✅ Stale «This account is currently online» (`7fa3c457`)**: при краше/зависании клиента `account.online` остаётся 1 (деструктор, сбрасывающий его, не отработал) → следующий логин отклоняется ДО выбора персонажа. Фикс в `_VerifyLogin`: если `online=1`, но живого соединения с аккаунтом нет (`EntityList::FindClientByAccountID`, новый метод) → stale-флаг сбрасывается и логин проходит. Только при реально живом клиенте отказывает. Аналогичен персонажному фиксу `1b9bec69`.
- **✅ Защита данных (binlog + бэкапы)**: db-контейнер пересоздан с `--log-bin=mysql-bin --binlog-format=ROW` (`log_bin=ON`, `binlog_format=ROW`); скрипт `/opt/evemu/backup_db.sh` (mariadb-dump → gzip в `/opt/evemu/backups/`, хранит 7, cron `0 4 * * *`); первый дамп `evemu_20260826_2126.sql.gz` (342 МБ). Сырой дамп `backup_evemu_20260826.sql` (5 ГБ) в корне `/opt/evemu` — НЕ удалён, но добавлен в `.dockerignore` (раздувал build context до 4.8 ГБ).
- **✅ docker-compose починен**: контейнеры были созданы вручную (`docker run`) из-за пересоздания db → `docker-compose down` падал «network has active endpoints». Теперь контейнеры под управлением compose (созданы через `docker-compose up -d`). docker-compose 1.29.2 не менялся. В `docker-compose.yml` добавлены binlog-флаги в db-сервис (принимается старым compose).
- **✅ .dockerignore расширен**: исключены `backup_*.sql`, `backups/`, `image_cache/`, `server_cache/`, `*.cache2`, `out.txt` — build context уменьшен с 4.8 ГБ до ~80 МБ.
- **Логи снова ERROR-only**: диагностика (DAMAGE__MESSAGE/SE__SLIMITEM/NPC__AI_TRACE/CLIENT__SESSION/COLLECT__DESTINY/DESTINY__UPDATES) включена временно для поиска клиент-кика при атаке ТКУ файтерами, потом отключена по просьбе юзера. log.ini в репо и на сервере (`/opt/evemu/config/log.ini`) — 70 ERROR-каналов, остальное 0.
- **НЕ СДЕЛАНО**: клиент-кик при атаке ТКУ файтерами так и не диагностирован (диагностику отключили до повтора проблемы). Если повторится — включить те же 6 каналов, воспроизвести, снять лог.
- **ВАЖНО**: юзер просил НЕ менять софт на сервере без спроса (замена docker-compose/скрипты были отменены). Команды перезапуска юзера: `docker-compose down` / `docker-compose up -d`.

## 26 августа (утро/день, юзер): ECM игрока, амбуши хантеров, self-preservation, джамп-драйвы, клоны, FW system flip, краш-фиксы
**Коммиты юзера за день (хронологически): `701a6991` (revert market defaults), `b74385d0` (destiny off-grid тихо), `b20223c4` (SBU у гейта), `6ab89f14` (боевые корпуса), `10009b5a` (урон дронов владельцу), `d9f25a04` (шар заскупленного дрона), `d40b5301` (tr same-system), `52bb400d` (боты у гейтов), `8d38cec7` (self-preservation), `35014694` (combat analysis + EWAR fit), `c64518a5` (build-fix TryAmbush), `005c342c` (амбуши хантеров), `7ae077e2` (ECM игрока), `4adb57d0` (docs verified), `6dd25d3d` (джамп-драйвы цино+топливо), `a5e5e26c` (docs), `b93bcee7` (краш дронов m_assignedShip), `015f5736` (ship clone bay), `ffc27824` (CloneJump двигает пилота), `2c99292e` (краш OnWeaponFired), `3c6aa809` (docs PI), `b62e1310` (FW system flip), `51371c8d` (docs FW). Образ собран; юзеру нужен РЕСТАРТ контейнера `server`.**
- **✅ ECM игрока (`7ae077e2`)**: `ActiveModule` ECM-кейс был пустой заглушкой. Теперь сравнивает jam strength модуля с сильнейшим сенсором цели, при успехе сбрасывает лок цели на игрока (ClearTarget) + шлёт `ElectronicAttributeModifyTarget`. EWAR-набор завершён: web/scram/ECM/paint для игроков, челоботов (role-атрибуты) и NPC (NPCAI).
- **✅ Амбуши хантеров (`005c342c`+`c64518a5`)**: опытные хантеры (skill≥3) с союзниками рядом ставят варп-бабл-ловушку: `TryAmbush` дропает Mobile Warp Disruptor (online сразу, пузырь не даёт цели варпнуть), зовёт флот, потом атакует. Хукнут в HuntForTarget (цели-боты) и NPCAI Idle scan (цели-игроки). Флот амбуша включает Commander бустеров (gang links) и Support EWAR.
- **✅ Combat situation analysis + EWAR fit (`35014694`)**: Support-боты получают полный web/scram/ECM/paint (NPCAI применяет в AttackTarget), Fighter'ы — tackle scram + лёгкий таргет-паинтер. `AnalyzeCombatSituation()` во время боя: ре-таргет Support/Commander на приоритетную цель (логисты/командиры/EWAR умирают первыми), дезенгаж при критически низком HP корпуса или сильном численном перевесе врага.
- **✅ Self-preservation (`8d38cec7`)**: (1) ошибка новичка теперь только заставляет ПАНИЧНО СБЕЖАТЬ из выигрышного боя, никогда не атаковать проигранный (хаулер/фрегат больше не суицидится на Nyx с бомберами); (2) не-боевые корпуса (industrial/barge/freighter/hauler) никогда не отбиваются — варпят, `CallFleetSupport` их больше не созывает в бой. Грузовики не атакуют игроков.
- **✅ Боты у гейтов (`52bb400d`)**: темп решений теперь сек-зависимый, первое действие через 2-5с после спавна вместо 15с простоя (low/null реагируют за 4-9с, хайсек 12-25с); боты, реально пересекающие гейт, бродкастят GateActivity flash на гейте прибытия (не материализуются из ниоткуда).
- **✅ Боевые корпуса (`6ab89f14`)**: боевые профы (Hunter/RatHunter) валидируют ship group легенды — майнинг-баржи/фригатеры/хаулеры из легенд заменяются на боевой крейсер/БК («пираты на Covetors» убраны).
- **✅ Урон дронов владельцу (`10009b5a`)**: `ApplyDamage` слал OnDamageMessage только пилоту цели (HasPilot) — урон по дронам/файтерам игрока не попадал в combat-log владельца. Добавлено owner-keyed Taken-сообщение + немедленный flush очереди.
- **✅ Шар заскупленного дрона (`d9f25a04`)**: `ScoopDrone` держал DroneSE живым (отложенная очистка), но шар не убирал мгновенно — если владелец варпил до 2-тиков pendingRemoval, шар файтера оставался у гейта (виден через систему). Теперь: ClearTarget дрона + `SysBubble()->Remove()` сразу в ScoopDrone (мгновенный RemoveBall).
- **✅ tr same-system (`d40b5301`)**: трансклокация в той же системе оставляла корабль в списке игроков старого баббла (`SetDestiny` только двигал позицию, AddEntity early-return для уже зарегистрированной сущности) — клиент продолжал видеть старый грид из-за всей системы. `UpdateBubble` теперь выбрасывает корабль из устаревшего баббла перед SendSetState.
- **✅ SBU у гейта (`b20223c4`)**: проверка «near a stargate» для развёртывания SBU использовала `GetOperationalStatics()` (только TCU/SBU/IHub/Outpost) — гейты не в нём, проверка всегда фейлилась. Теперь `GetGates()` (m_gateMap) + сравнение bubbleID.
- **✅ Destiny off-grid тихо (`b74385d0`)**: «Cannot BubbleCast» + traceStack для NPC/челоботов вне баббла — молча (легитимно при travel/warp-out), fallback re-add только для пилотов. (Дублирует мой `dda2b6de`, юзер переписал чище.)
- **✅ Джамп-драйвы (`6dd25d3d`+`a5e5e26c`)**: кап-прыжки и бриджи требуют АКТИВНОЕ цино — `CmdBeaconJumpFleet/Alliance` + `CmdJumpThroughFleet/Alliance` валидируют CynosuralFieldI/CovertCynosuralFieldI ещё online в системе назначения (деактивированный цино удаляется, буктмарк/корабль отклоняется). Миграция `20260826000000-jump_drive_fuel_type.sql`: `AttrJumpDriveConsumptionType` (866) на кап-корабли по расе (Caldari→Helium 16274, Minmatar→Hydrogen 17889, Amarr→Nitrogen 17888, Gallente→Oxygen 17887) — клиентское окно джамп-драйва показывает правильный изотоп и серверный fuel-check работает.
- **✅ Джамп-драйв ПРОВЕРЕН юзером (27 авг, `1313d4cb`)**: прыжок на Anshar (джамп-фрейтере) Тасти→Уемон сработал визуально и по доставке. НО: расход топлива был **занижен ~16 раз** — сожглось 198 изотопов вместо ~3157. КОРЕНЬ: координаты систем ~1e16 м, а `GVector::length()` на `GPoint` (GaFloat=float, ~7 значащих цифр) терял точность → дистанция считалась ~0.064 LY вместо 1.018. Фикс `1313d4cb`: `SystemDB::GetSolarSystemPositionDouble` + расчёт дистанции в double во всех 5 путях джампа (цино/флот/альянс/бридж ×2). После пересборки расход станет ~3157/LY (минус скиллы JumpFuelConservation/JumpFreighters). Бридж (CmdJumpThrough) пока НЕ тестирован.
- **✅ Клоны (`015f5736`+`ffc27824`)**: (1) `AcceptShipCloneInstallation` использовал несуществующие entity-колонки (name/positionX/...) — INSERT всегда падал, ship clone bay был мёртв; теперь клон создаётся в clone bay (flagClone) через item factory как остальные клоны; (2) `CloneJump` менял только БД-состояние (активный клон + home station), но не двигал пилота — PerformSessionChange('clonejump') не завершался. Теперь `MoveToLocation(destStation)` докует пилота на станции назначения (полная смена сессии: переход системы, корабль в ангар, ангар загружен).
- **🔴 Краш дронов при логофе (`b93bcee7`)**: SIGSEGV в `DroneAIMgr::Process` — use-after-free `m_assignedShip`. На логофе корабль становится ghost; когда ghost истекает, ShipSE освобождается, но дроны игрока в космосе держали raw-указатель → следующий тик дрона читал freed memory (GaVec3::distance). `SystemManager::ClearDronesAssignedTo()` обнуляет m_assignedShip всех дронов ДО удаления ghost-корабля.
- **🔴 Краш OnWeaponFired (`2c99292e`)**: SIGSEGV (this=0x0) от атак дронов — 8 методов атаки (Combat/Fighter/FighterBomber/Web/Scramble/ECM/Paint/CapDrain) звали `owner->GetCrimeWatch()->OnWeaponFired()` без проверки nullptr. После логофа владельца CrimeWatch null → дрон на след. тике разыменовывал. Все 8 гардируются.
- **✅ FW system flip (`b62e1310`+`51371c8d`)**: захват FW-плексов копит очки переворота фракции захватчика (60-200 за плекс по размеру); при достижении FW_FLIP_THRESHOLD (1000) occupierID системы переключается (SetSystemOccupier), пилотам нотификация. FW Plex capture завершён: спавн плексов, таймеры захвата, contested-пауза, LP-награды, переворот системы.
- **✅ PI (`3c6aa809`)**: документировано как реализованное (не заглушки) — CC deploy, пины, линки/маршруты, программы, launch.
- **Revert market (`701a6991`)**: дефолты Station/System/RegionOrderLimit возвращены на 10 — нужен только uint8→uint32 фикс (1547a65a), config явно ставит 20000.

## 26 августа (день): краш при атаке ТКУ (лог-флуд), урон/таргет челоботов с диких расстояний, режимы дронов (Focus Fire/Aggressive/Attack-Follow)
**Коммиты: `539cfdbc`+`adde225e` (режимы дронов), `1cc8cf04` (ТКУ-флуд), `24b48fb0` (дамаг по дистанции), `1be77a41` (таргет челоботов как у игрока). Образ собран на сервере; юзеру нужен РЕСТАРТ контейнера `server`.**
- **✅ Режимы дронов (`539cfdbc`+`adde225e`)**: юзер спросил «реализовано?» — `AttrDroneFocusFire`/`AttrFightersAttackAndFollow` сохранялись в Ship.cpp, но НЕ использовались (только `AttrDroneIsAgressive` читался при ответе на атаку). Теперь в DroneAI Idle-кейсе: (1) **Focus Fire** — все дроны бьют цель корабля (`m_assignedShip->TargetMgr()->GetFirstTarget(true)`); (2) **Aggressive** — дрон сам ищет ближайшего NPC-рата в `GetControlRange()`; (3) **Fighters Attack-and-Follow** — файтеры авто-агрят ратов (после ре-энгейджа по аммо). Новый хелпер `DroneAIMgr::FindAggroTarget()` (сканирует бабл, пропускает дронов/статику/челоботов `IsPlayerBot()`, берёт ближайшего NPC-рата). Build-fix: `#include "npc/NPC.h"` в DroneAI.cpp (IsPlayerBot — полный тип NPC).
- **🔴 Краш/вис при атаке ТКУ (`1cc8cf04`)**: юзер «начинается при атаке ТКУ» — трейс `CmdEngage → FighterBomberAttack → ApplyDamage → AttributeMap::SetAttribute → SendChanges → EntityList::Multicast(Corporation)`. КОРЕНЬ: `AttributeMap::SendChanges` для ЛЮБОГО корп-предмета (ТКУ/POS-башня под огнём файтеров: shield/armor/hull на каждый удар) строил корпоративный MulticastTarget → `EntityList::Multicast` печатал **Error + `traceStack()` (40 строк) на каждое изменение атрибута** → сотни traceStack/сек → сервер вис (тот же симптом, что 25 авг от логов). Сами корп-мультикаст НИЖЕ реализован (m_corpMembers заполняется в EntityList.cpp:181) — лог был мёртвым шумом. ФИКС: убраны Error+traceStack из корпоративной ветки Multicast. НЕ краш был, а лог-флуд.
- **✅ Челоботы стреляли/дамажили с 574 км (`24b48fb0`+`1be77a41`)**: юзер «мегатрон челобота стреляет и дамажит по мне с диких расстояний — 574 км». ДВЕ причины: (1) **урон** — `NPCAIMgr::AttackTarget` применял `ApplyDamage` БЕЗ проверки дистанции (`m_maxAttackRange` гейтил только эффекты через CycleModules); (2) **таргет** — дальность лока = `m_sightRange` по радиусу корабля (Мегатрон 150 км), лок НЕ дропался при выходе цели за дальность (`TargetManager::Process` не проверяет дистанцию; `CheckDistance` сбрасывал лок только `!IsTargetedBy`). Фиксы: (1) `AttackTarget` дамажит только при `dist <= m_maxAttackRange` (optimal+2*falloff, челобот-Мегатрон ~35 км); (2) челоботам `m_sightRange` = реальный `AttrMaxTargetRange` корабля ×1.25 (Long Range Targeting V): Мегатрон 72500→~90625 м, Raven 75000 (в БД атрибут 76 есть у реальных кораблей, челоботы летают на них); (3) `CheckDistance` — челобот дропает лок при выходе за дальность таргета НЕЗАВИСИМО от IsTargetedBy (как игрок), обычные NPC по-прежнему преследуют агрессора.
- **ВАЖНО про дальность таргета**: `AttrMaxTargetRange` (атрибут 76) у реальных баттлшипов в БД: Apocalypse 67500, Dominix 70000, Megathron 72500, Raven 75000, Thorax 55000, Rifter 22500. Используется в NPCAI constructor (первая ветка), TargetManager.cpp:175 (для игроков). НЕ путать с `AttrMaxRange` (54, оружие) и `AttrEntityAttackRange` (у NPC-ратов).
- **Осталось проверить после рестарта**: дроны в режимах Focus Fire/Aggressive/Attack-Follow бьют самостоятельно; ТКУ можно атаковать без виса; челобот-Мегатрон не дамажит и теряет лок с 574 км (затаргетится в пределах ~90 км, урон только ~35 км).

## 25 августа (ночь): тубы файтеров (20), агрессия PvP/бот↔игрок, логи приглушены (сервер вис от вывода), боты уходят без застревания
**Коммиты: `df0a1207`+`57940a22` (тубы файтеров), `dcaea8a8` (агрессия), `f5c95912` (логи — только ERROR), `dda2b6de` (NPC/боты не возвращаются в бабл при варпе). Сборка юзером ОБЯЗАТЕЛЬНА на `dda2b6de` (последний).**
- **✅ Тубы файтер-бомберов (`df0a1207`+`57940a22`)**: юзер на Nyx запускал 35 бомберов (первый раз глюк) и 20+6 (баг: лимит был GetDroneLimit=база+6 DCU, применялся к файтерам). КОРЕНЬ: `LaunchDrone` обходил лимит дронов И bandwidth для `isFighter` (`!isFighter`), тубы не проверялись; `GetFighterTubeCount()` был 3/6 (должно 10/20). Фикс: Carrier=10, Supercarrier=20 тубе; LaunchDrone проверяет `GetActiveFighterCount() >= GetFighterTubeCount()` (только файтеры, не обычные дроны); `ShipBound::Drop` для Fighter_Drone/Fighter_Bomber проверяет тубы отдельно; maxActiveDrones-проверка для обычных дронов вычитает активных файтеров (`DroneCount() - GetActiveFighterCount()`). Nyx с 6 DCU: 20 бомберов (тубы) + 6 обычных дронов (DCU) раздельно, как в EVE.
- **✅ Агрессия PvP + бот↔игрок (`dcaea8a8`)**: `CrimeWatch::OnAggression` был МЁРТВЫМ кодом (нигде не вызывался) — агрессия не ставилась вообще. Теперь: (1) `Damage::ApplyDamage` флагает атакующего (пилот или владелец дрона/файтера по `GetOwner()`) когда бьёт по пилотируемой цели или её дрону — работает игрок↔игрок и игрок→челобот; (2) новый `CrimeWatch::OnBotAggression(botCharID, sec)` — игрок, атакующий челобота, получает агрессию + криминал в хайсеке (челоботы не Client); (3) NPCAI: когда челобот бьёт дрон/файтер игрока — `StartAggressionTimer()` + `BroadcastAggression(владельцу)`. Челоботы-хантеры уже атакуют игроков в low/null (NPCAI Idle scan: только Hunter, 15% шанс, оценка сил `HunterWouldEngage`) — это задумано.
- **🔴 СЕРВЕР ВИС ОТ ЛОГОВ (`f5c95912`)**: включённые BALL_DECODE/BUBBLE_TRACE/MOVE_TRACE/WARP_TRACE/NPC__AI_TRACE/SE__SLIMITEM + MESSAGE/INFO каналы при 375 ботах писали сотни тысяч строк/мин → CPU 50%+ → сервер зависал (порт 26000 мёртв, лог замер на секунде, клиент не может зайти, юзер «всё умерло кроме чата»). ФИКС: log.ini приведён к ERROR-only (все *ERROR=1, всё остальное 0; также выключены SERVICE__CALLS, AUTOPILOT__MESSAGE, CLIENT__STACK_TRACE, WORMHOLE_MGR__INFO, DRONE__AI_TRACE, SOV__WARNING/INFO). После рестарта сервер idles <5% CPU. Новый log.ini залит на хост (`/opt/evemu/config/log.ini` = `/app/etc/log.ini`), закоммичен в utils/config/log.ini. Юзер подтвердил: зашёл нормально.
- **✅ Боты уходят без застревания (`dda2b6de`)**: `MarkForTravel`/`HeadTowardHub` → `WarpTo` когда бот уже вне баббла (удалён с грида при уходе) → `SendDestinyUpdate` падал в else «not in any bubble», traceStack (только test-server) и **`sBubbleMgr.Add` возвращал бота в бабл, блокируя уход**. Фикс: fallback re-add только для реальных пилотов (Client, `HasPilot() && !IsNPCSE()`); NPC/челоботы молча дропают off-grid warp update (никто не в радиусе). + null-guard `SysBubble()` в WarpTo NPC-логе (потенциальный segfault при включённом NPC__MESSAGE).
- **Клиентский краш в бою**: юзер «урон есть, источник не вижу» + клиент крашнулся + UpdateStateRequest. Причина — старый бинарь (без 4a06b77d/фиксов): Rogue Drone невидимы, дроны гоняются, тубы не ограничены. На новом бинаре должно уйти. Также: Nyx юзера ЖИВ (не потерян), юзер пересел и вернулся.
- **Диагностика log.ini**: СЕЙЧАС ТОЛЬКО ERROR-каналы (критичные). Включены: XMLP/REFPTR/NET/COLLECT/SERVICE/ACCOUNT/AGENT/ALLY/ATTRIBUTE/AUTOPILOT/BOOKMARK/BULKDATA/CACHE/CHARACTER/CLIENT/COLONY/COMMAND/COMMON/CONCORD/CORP/COSMIC_MGR/DAMAGE/DATA/DATABASE/DESTINY/DRONE/DUNG/EFFECTS/FACWAR/FLEET/INV/ITEM/LP/LSC/MAIL/MANUF/MARKET/MINING/MODULE/NPC/PHYSICS/PLANET/PLAYER/POS/QATOOLS/SCAN/SE/SHIP/SKILL/SOV/SPAWN/STANDING/TARGET/TCP_CLIENT/TCP_SERVER/THREAD — все `__ERROR=1`, остальное 0.
- **Осталось проверить после пересборки на `dda2b6de`**: агрессия при PvP и атаке челоботом (флаги с обеих сторон); боты не застревают при уходе; сервер не виснет от логов.
- **✅ ПРОВЕРЕНО юзером**: 20 бомберов у Nyx (тубы) — точно 20; Rogue Drone видны/красные и бьют с цифрами урона.

## 25 августа (вечер): челоботы везде активны, невидимые Rogue Drone (factionID=0), дроны не летят за целями; доки почищены
**Коммиты: `b304eaf5` (боты без станции: патруль/уход к хабу), `3b42fad8` (раттеры летят к NPC, хантеры дрейфуют/уходят), `bddf05e6` (build-fix: GetTradeHubSystem/IsTradeHub public, HasStationInSystem не-const), `936c975b` (доки: README/PROGRESS синк + удалены сессионные файлы), `b65226a6` (убрано упоминание декомпила из PROGRESS), `d492ef9f` (NPC не преследует цель вне баббла), `4a06b77d` (Rogue Drone factionID=500022 — красные/видимые). Сборка юзером ОБЯЗАТЕЛЬНА на `4a06b77d` (последний).**
- **✅ Поведение ботов во всех типах систем**: раттеры теперь варпят к дальней NPC-группе (аномалия/пояс) если в баббле нет крыс (`3b42fad8`); хантеры между зачистками дрейфуют (PatrolForIdle 55%) или уходят в другую систему (25%); станционные профы в системах без станции (ВХ/пустые) — опытные 60% к хаб-системе (`GetTradeHubSystem`, Jita из botTradeHubs), новички патрулируют затем уходят (`b304eaf5`). Покрыто: аномалии, ВХ, пустые системы, FW, инкурсии.
- **🔴 НЕВИДИМЫЕ ROGUE DRONE — РЕШЕНО (`4a06b77d`)**: юзер «у гейта» получал невидимый урон (логов урона нет — DAMAGE отключён; не видит источник). КОРЕНЬ: `DoSpawnForAnomaly` и `MakeSpawn` обнуляли `data.factionID` для `factionRogueDrones` (`should be "0" for client to use it right`) → `m_warID=0` → slim item: `warFactionID=None`, `securityStatus=0` (белые), клиент **не отрисовывал** шар. Фикс: обе строки теперь `data.factionID = factionID` (500022, в диапазоне [500000,999999) → IsFaction=true; corp=corpRogueDrones 1000001). Дроны красные/видимые/агрессивные. Юзер подтвердил «дроны были красными».
- **✅ Дроны не летят за целями (`d492ef9f`)**: дрон-повстанец преследовал юзера, ушедшего варпом, через всю систему — GotoPoint на позицию цели, цепочка бабблов (лог: 750000514 в бабблах 976-984 + 20), догонял и бил «невидимо». Фикс: в NPCAI Process (Chasing/Following/Engaged) если баббл цели != баббл NPC → ClearTarget (бой только в одном баббле). Аналогичный guard в CheckDistance.
- **Рogue Drone спавн**: дроны-повстанцы у бельтов приходят из «Rock - Infested by Rogue Drones» (110000002 и др.) — штатный бельтовый спавн factionRogueDrones. Sunder Drone (25698, group 759) / Dismantler Drone (25647, group 758) — клиент их ЗНАЕТ (есть в 600002.cache2 «Asteroid Rogue Drone *» и в 600004.cache2). Проблема была только в factionID.
- **Доки**: удалены сессионные файлы (`current_state_summary.md`, `ToTest.md`, `.github/ISSUE_TEMPLATE/todo-session-20260707.md`) — README ссылки убраны, Changelog → `git log`. README/PROGRESS синхронизированы (Market 95, Incursions 93, Wormholes 92, Drones 96, NPC AI 97; фичи: order-limit fix, декор/ворота, W-space сайты, EWAR cleanup, two-phase decel). Убрано упоминание декомпила из PROGRESS (по запросу юзера). Информация о челоботах — ТОЛЬКО в AGENTS.md (не в README/PROGRESS).
- **Клиентский десинхрон варпа (не фиксили)**: «подварпал на гейт, назад варпнуть не могу — вы ещё в варпе». Сервер варп завершил корректно (WarpStop 19:54:09), клиент не отрендерил второй варп (WARP не ушёл клиенту — в BallDecode только STOP), залип на 8KE-YS. Известный симптом «WarpStop не шлёт CmdStop». Юзер: «пока оставим» — разовое, не чинить.
- **Диагностика log.ini**: включены SE__SLIMITEM, NPC__AI_TRACE, DESTINY__BUBBLE_TRACE, DESTINY__BALL_DECODE; отключены MARKET__DUMP/TRACE/DB_TRACE, CACHE__DEBUG/INFO/TRACE/DUMP, MOVE/TURN/WARP_TRACE, SE__DESTINY, SCAN__TRACE, DAMAGE.
- **Осталось проверить после пересборки на `4a06b77d`**: Rogue Drone видны/красные у бельтов и бьют с лучами/цифрами урона; бой в одном баббле (дроны не разбегаются за юзером); поведение ботов в системах без станции и в аномалиях.

## 25 августа: МАРКЕТ — «нет в наличии» в списке товаров (РЕШЕНО `1547a65a`)
**Симптом**: в окне маркета (Browse → группа товаров) ВСЕ предметы показывают «нет в наличии» (NoneAvailable) вместо цены. В БД 38.5M ордеров, на станции юзера (60001795, Uemon, регион 10000002) 10390 типов с ордерами, ордера на продажу ЕСТЬ (юзер подтвердил).**
- **КОРЕНЬ (найден по warning компилятора)**: поля `StationOrderLimit`/`SystemOrderLimit`/`RegionOrderLimit` (и `FindBuyOrder`/`FindSellOrder`/`OldPriceLimit`/`NewPriceLimit`) в `EVEServerConfig::market` объявлены **`uint8`** (unsigned char). Присваивание `20000` в uint8 **оборачивается в 32** (20000 % 256 = 32). Поэтому каждый аск-запрос выполнялся с `LIMIT 32` — `GetStationAsks(60001795)` возвращал dict из 32 типов вместо ~10390. Клиент показывал «нет в наличии» для всех типов вне этих 32. Компилятор предупреждал: *'unsigned conversion from int to uint8 changes value from 20000 to 32'*.
- **Фикс `1547a65a`**: расширены все order-limit поля до `uint32` (str2<uint32> и LIMIT %u работают). Бонус: `OldPriceLimit`/`NewPriceLimit` (1000 → 232) тоже были сломаны — исправлены.
- История: `8040631b` (лимит 10→20000, НЕ помог — uint8 оборачивал), `70646181` (CIndexedRowset, клиент декодировал в 0 строк), `9b956859` (dict util.KeyVal), `f5e22ece` (дамп). Формат dict оказался правильным — проблема была только в uint8-лимите.
- **⚠️ Мои формат-эксперименты ОТКАЧЕНЫ `bf6b62d9`** (по запросу юзера «откати агента»): MarketDB.cpp снова `DBResultToIndexRowset`, хелпер `DBResultToTypeKeyValDict` удалён из EVEDBUtils, MARKET__TRACE/дампы убраны из MarketMgr. Настоящий фикс — только `1547a65a` (uint8→uint32).
- **Юзер подтвердил**: «Цены вижу». Маркет-логи (MARKET__TRACE/DUMP/DB_TRACE, CACHE__DEBUG/INFO/TRACE/DUMP) отключены в log.ini.
- **ВАЖНО**: тип колонки `price` = DBTYPE_R8 (5) — корректен. Ключи dict = typeID (UI2/18) — корректны.

## 21 августа (поздний вечер): инкурсия-сессия — сканер, камера ботов, защита реальных игроков; ПОТЕРЯН персонаж юзера
**Коммиты: `dc354446` (невидимые инкурсийные Sansha — remap групп 1051-1056), `1b9bec69` (сканер завис в инкурсии + stale «уже онлайн»), `ed62029d` (кнопка скана липла при варпе), `4debf188` (камера на челоботов — modules в slim как кортежи), `e3c7c0f0` (шар NPC не досылался после WarpStop → невидимые атакующие), `450892ca` (боты больше не трогают реальных игроков). Сервер пересобран юзером на `450892ca`? — проверка.**
- **🔴 ПОТЕРЯН персонаж юзера Mr Tort (90000000)**: между ~12:30 и 14:55 (пересборка) персонаж полностью удалён из БД (chrCharacters/entity/entity_attributes/навыки — 0 следов, binlog OFF, бэкапов нет → безвозвратно). Юзер создал нового Mr Tort (97233346, accountID=1). ВЕРОЯТНАЯ ПРИЧИНА: `CreateBotCharacter` дедуплицировал по имени БЕЗ фильтра accountID — легенда бота с именем «Mr Tort» находила реального игрока и отдавала его charID бот-системе (перезапись био/памяти/PlayerBot поверх). **Фикс `450892ca`**: дедуп только `accountID=0`.
- **✅ Невидимые инкурсийные Sansha (`dc354446`)**: группы 1051-1056 (Incursion Sansha) отсутствуют в клиентском repository.py GetGroupDict → клиент рендерил базовый SpaceObject (невидим). Remap groupID в slim item на известные клиенту (1053→567 Frigate, 1054→566 Cruiser, 1056→565 Battleship, 1051→568 Hauler) → EntityShip. Юзер подтвердил: «Саньшу у гейта увидел».
- **✅ Сканер зависал в системе инкурсии (`1b9bec69`)**: `IncursionMgr::SpawnSites` строил сигнатуру БЕЗ `sigID` → `ShipScanResult` слал клиенту результат с пустым id → клиентское окно замерло на таймере 0 (повторные RequestScans). Фикс: `sig.sigID = sEntityList.GetAnomalyID()` (как ExpeditionMgr). Юзер подтвердил: «баг со сканом пофикшен».
- **✅ Кнопка скана липла (`ed62029d`)**: клиент `scanSvc.RequestScans()` ставит `scanningProbes` ДО вызова сервера, сброс только по OnSystemScanStopped. При отказе «You can't scan while warping» сервер не слал событие → кнопка Disable навсегда. Фикс: сервер шлёт пустой OnSystemScanStopped при варп-отказе.
- **✅ Камера на челоботов (`4debf188`)**: клиент `turretSet.FitTurrets` читает `slimItem.modules` как кортежи `(moduleID, typeID)` (`module[0]`). Сервер слал `util.KeyVal{typeID,flag}` → TypeError в Ship.LookAtMe → камера молча не переключалась. Фикс: кортежи как у реального корабля. Юзер подтвердил: «камера заработала».
- **✅ Невидимые атакующие NPC после варпа (`e3c7c0f0`)**: NPC, варпящий в баббл игрока, получал AddBallExclusive с WARP-EncodeDestiny (клиент игнорирует); после WarpStop шар NPC не пересылался (только для пилотов) → NPC бьёт невидимо (нет метки/модели/логов урона, щит падает). Фикс: WarpStop досылает AddBallExclusive и для NPC.
- **Бот-идентификация по charID исправлена (`450892ca`)**: `HandleLocalMessage` считал ботом `(нет Client) && (charID>=90000000)` — реальные игроки (charID тоже в 90000000-97999999) ловились как боты в чате. Теперь бот = нет живого Client (игрок всегда имеет Client).
- **Диагностика log.ini**: включены SE__SLIMITEM, NPC__AI_TRACE, DESTINY__BUBBLE_TRACE, DESTINY__BALL_DECODE (для невидимых NPC); отключены MOVE_TRACE/TURN_TRACE/WARP_TRACE/SE__DESTINY/SCAN__TRACE/DAMAGE (шумные/протестированные). Применится при рестарте.
- **Осталось проверить**: пересборка на `450892ca`; невидимые NPC в инкурсии (всё ли видно после e3c7c0f0); «варп зависал при нажатии на планету» — юзер был внутри планеты? (гейт 50004103 и планета 40148131 в ~2700км, радиус планеты 2980км); новый Mr Tort (97233346) — лаги после фикса идентификации.

## 21 августа: био ботов стабильно, null-защита ship-state, фикс эффекта лазеров у игрока
**Коммиты: `7e8869a7` (био один раз на пилота), `6edff173` (GetShipState/GetChargeState/ShipGetModuleList не возвращают nullptr — восстановлен хвост потерянного `105c9103`), `7838e1b8` (build-fix: pool в UpdateBotBio — `const char**`), `70c55303` (эффект лазеров у игрока). Сервер пересобирается юзером.**
- **✅ Био стабильно (`7e8869a7`+`7838e1b8`)**: `UpdateBotBio` перезаписывал био при КАЖДОМ спавне случайным вариантом → текст «прыгал» между сессиями (прокол имитации). Теперь `botMemory.bioUpdated` (миграция `20260821000000`, все существующие помечены 1): био пишется ОДИН раз — при первом спавне пилота сразу после ролла профессии — и замораживается. Профессия и раньше была стабильна (хранится в botMemory, BotMgr.cpp:419). Build-fix: `pool` был `const char*` инициализированный `hunter[0]` → `pool[index]` давал char; теперь `const char**` + массив без `[0]`.
- **Null-защита ship-state (`6edff173`)**: `GetShipState`/`GetChargeState`/`ShipGetModuleList` возвращали nullptr при сбое `LoadContents()` → попадал в rsp/slim item → клиентский краш. Теперь пустой контейнер (PyDict/PyList). Это последний неперенесённый коммит из потерянного блока (104 шт).
- **✅ Эффект лазеров у игрока (`70c55303`)**: игрок не видел огня своих турелей (у ботов был). Боты шлют guid жёстко (`effects.Laser` в NPCAI), игрок — через `GetEffectGuid(m_effectID)` из серверного SDE; при отсутствии guid возвращал `""` → `ShowEffect` пропускал OnSpecialFX (`!guidStr.empty()`). Фикс: fallback в GetEffectGuid — `targetAttack(10)→effects.Laser`, `projectileFired(34)→effects.ProjectileFired`, `projectileFiredForEntities(1086)`. +EFFECTS__TRACE маппинга effectID→guid.
- **Маркет «60 jumps» и incursion ISK — закрыты юзером вручную**: `ee9613be` (16 авг, спец-диапазоны orderRange) и `f502d7ce`+миграции (rewardTypeID=2=ISK) работают — юзер подтвердил.

## 18 августа: скорость/аппроач/ворота, ВХ, анимация прыжка, экспедиции; сканер чинился откатом Scan
**Коммиты: `044b0926` (скорость/аппроач/облака/перевёрнутые ворота), `cd02460e` (ВХ: expiry в customInfo, K162 пересоздание, hangar reload), `34cb4ee9` (ZeroVelocity при прыжке), `ea2e7335` (ручная анимация прохода гейта + отложенный прыжок), `e160445c` (экспедиции: ExpeditionMgr), `c7dfd566` (экспедиции: Journal rowset + warp), `6b3325de`+`62648500` (build-фиксы), `1d1357cd` (откат фильтров сканера). Юзер пересобирал несколько раз; финальная сборка работает.**
- **✅ СКОРОСТЬ 93383 м/с (306×305) — КОРЕНЬ**: `NPCAI::ClearTarget` снимал веб безусловно при `m_webRange>0` (`WebbedMe(false)` = ÷0.4 = **×2.5** каждый вызов), хотя веб применялся только вероятностно. Слиперы (SpeedFactor=-60 в SDE) при каждом сбросе таргета раздували скорость. Фикс `044b0926`: `m_webApplied`/`m_webTargetID` трекинг, undo перед re-apply, веб снимается только с реально завебленной цели (+NPC::Killed guard). Аппроач к статичным объектам тоже чинился тут (`IsTargetInvalid(forOrbit)` — орбита вокруг static запрещена, FOLLOW/аппроач разрешён).
- **Дёргание корабля после прыжка (`34cb4ee9`)**: `Stop()` обнуляет speed-fraction, но оставлял `m_velocity` от pre-jump → `EncodeDestiny` слал `mode:STOP` с ненулевой Vel (`Vel:-59.3,169.6,337.7`) → клиентский Ballpark дёргал. Добавлен `DestinyManager::ZeroVelocity()`, вызывается в MoveToLocation (jump-ветка), StargateJump, ExecuteJump.
- **Анимация прохода гейта (`ea2e7335`)**: ручные прыжки двухфазные — JumpOut + ~4с (timer Jumping) → ExecuteJump (переход+JumpIn). AP — быстрый путь. Прыжок, посланный во время варпа к гейту, теперь откладывается (`RequestJumpAfterWarp`, WarpStop выполняет StargateJump) — раньше отклонялся («You can't do this while warping») и прыжок не срабатывал.
- **ВХ-фиксы (`cd02460e`)**: expiry ВХ теперь хранится в customInfo (`expiry:<filetime>`) и восстанавливается при перезагрузке системы (раньше сбрасывался на now+24h → ВХ не старели); повторный вход в ВХ с мёртвым K162 пересоздаёт exit вместо посадки на солнце; ангар станции перезагружается при ан-доке (иначе пустой после возврата через ВХ).
- **Экспедиции (PvE-эскалации) (`e160445c`+`c7dfd566`)**: убийство NPC в аномалии → 5% шанс частной экспедиции. `ExpeditionMgr` выбирает систему ниже по секу, спавнит фракционный DED-сайт (по стадии: 3/10→5/10→8/10→10/10) с сигнатурой dungeonType=9. `GetMyEscalatingPathDetails` возвращает util.Rowset (Journal→Expeditions), нотификация `OnEscalatingPathChange`, `CanWarpToPathPlex`+`CmdWarpToStuff('epinstance')` — варп к сайту. 50% шанс следующей стадии (до 4), цепочка 24ч. Конкретные имена/данжи по фракциям (`ExpeditionName`/`ExpeditionDungeon`). Эскалации НЕ в сканере (только журнал).
- **🔴 ВАЖНЫЙ УРОК — сканер**: фильтры `sExpMgr.IsHidden()` в `Scan::ShipScanResult`/`ProbeScanResult` (даже статическая функция через синглтон) ВЕШАЛИ сканер — «таймер доходит до 0 и замирает». ОТКАТ (`1d1357cd`): Scan.cpp байт-в-байт как до экспедиций. После пересборки сканер работает. НЕ добавлять вызовы sExpMgr/синглтонов в Scan. Эскалации и так не в ship-scan (они в m_sigByItemID, не m_anomByItemID) и не сканируются пробами (GetProbeDataForSig исключает Escalation).
- **Build-уроки**: `EVE_Classes.h` не существует (EVEDB в tables/invTypes.h + invGroups.h); `DungeonExplorationMgrService.cpp` нужен `#include "Client.h"` для GetCharacterID.

## 17 августа: декор виден, ворота рендерятся/ориентируются, краш при таргете и UnloadSystem
**Коммиты: `9b0a2bf9` (декор global, ворота только в гриде), `707f5b4e` (точный варп к комнате через customInfo), `8c0f35b9` (краш при таргете декора + ворота IsGlobal), `02bb65dc` (ворота через static-карту бабла), `866795cf` (краш UnloadSystem erase(end())), `4bb3d1cb` (ворота за декором + ориентация). Сервер ПЕРЕСОБРАН юзером на `8c0f35b9`; после `02bb65dc` упал крашем — фикс `866795cf` уже запушен, НУЖНА ещё одна пересборка.**
- **✅ ДЕКОР ВИДЕН (закрыт баг 16 авг)**: причина невидимости — CelestialSE наследует `IsStaticEntity=true` от ItemSystemEntity → декор попадал в static-map бабла, но клиент рендерил его только из static-карты при global-доставке. Финал: декор получает `AttrIsGlobal=1` → static-путь `SendStaticBall` (AddBalls2) → виден system-wide (юзер: «декор и виден только при подлёте, всё ок, не трогать»). Облакам задаётся реальный `AttrRadius` (4-10км, клиент 2×radius), HP 1e12 (неуничтожаем — стрельба не роняет сервер).
- **Краш при таргете (залочке) декора (`8c0f35b9`)**: `TargetManager::StartTargeting` (оба варианта — игрок и NPC) вызывал `tSE->TargetMgr()->TargetedAdd()` без проверки; у декора/облаков/ворот (CelestialSE) `TargetMgr()==nullptr` → SIGSEGV. Теперь такие объекты нельзя таргетить (return false + notify «You cannot target that»).
- **Ворота ускорения — видимость (`02bb65dc`)**: RIGID CelestialSE через dynamic-путь (SendAddBalls/AddBalls) клиент НЕ рисует, даже с флагом IsGlobal. Рендер работают только сущности из static-карты бабла (m_entities, как станции). Фикс: `CelestialSE` получил флаг `m_isStaticEntity` (default false — декор остаётся dynamic), `DungeonMgr` ставит воротам `SetIsStaticEntity(true)` → они в static-карте бабла и доставляются при входе в грид. НЕ global (нет AttrIsGlobal) → не видно за 1.3 св. года, только при подлёте (юзер подтвердил, что так и надо). `SendAddBalls` больше не выходит рано при пустых dynamic (если в бабле только static).
- **Краш UnloadSystem `free(): invalid pointer` / SIGABRT (`866795cf`)**: ворота стали IsStaticEntity=true, но НЕ global/COSE → `AddEntity` НЕ кладёт их в `m_staticEntities`. При выгрузке W-space системы `UnloadSystem` попадал в ветку IsStaticEntity и делал `m_staticEntities.erase(m_staticEntities.find(id))` — для отсутствующего ключа `find()` возвращал `end()`, а `erase(end())` = UB → glibc abort. Обе операции erase (m_staticEntities и m_opStaticEntities) теперь guard'ятся. КРАШ ВОСПРОИЗВЁЛСЯ именно так (лог 19:34:19 при UnloadSystem J163641/31001493).
- **Ворота — позиция и ориентация (`4bb3d1cb`)**: стоят на +x (вектор прыжка к следующей комнате) на **28-32 км** — ЗА декором (структуры до ~26 км, облака до 15 км), разгон проходит прямо над воротами. `ItemSystemEntity::MakeSlimItem` для ворот теперь считает `dunDirection` = единичный вектор от ворот к следующей комнате (парсит customInfo `gate_to:x:y:z`, fallback +x) вместо хардкода (5,-1,0), и добавляет `dunRotation = (yaw,0,0)` (yaw=atan2(dx,dz), pitch/roll=0 → строго горизонтально). ВАЖНО про float: вектор 50M м считаем в double (не GVector/GaVec3 — там GaFloat=float).
- **Точный варп к комнате (`707f5b4e`)**: `ActivateAccelerationGate` (KeeperService.cpp) варпит точно на позицию следующей комнаты из customInfo `gate_to:x:y:z`, а не `+NEXT_DUNGEON_ROOM_DIST` от корабля (то выкидывало в космос). `NEXT_DUNGEON_ROOM_DIST = 50000000000` (50M м).
- **НЕРЕШЕНО/на проверку**: после пересборки на `4bb3d1cb` проверить — ворота видны в аномалии с 2+ комнатами, стоят за декором, ориентированы по вектору; декор не таргетится (нет краша); нет краша при выгрузке систем с данжами; нет краша при залочке структур.

## 16 августа: контент всех фракций + W-space + миссии; НЕРЕШЕНО: невидимость декора
**Коммиты в origin/master: `abdeb8fe` (док-фикс+слипер-декор+облака), `9d822d2b`+`6a7c6aa0` (Саньша), `4cab6017` (Гуристас), `eee099ce` (Ангелы), `8c291766` (Рейдеры), `717f6529` (Серпентис), `42bbab25` (Дроны), `ae7b4835` (миссии), `3e9a66df` (боты в ВХ у планеты), `c2da9c8f` (боты не в local ВХ). Юзер пересобрал на `ae7b4835` (сервер 17:14, 217 данжей), потом я добавил `3e9a66df`+`c2da9c8f` — ТРЕБУЕТ ещё одной пересборки. Диагностика BALL_DECODE включена в `/app/etc/log.ini` (при пересборке подхватится).**
- **🔴 НЕРЕШЕНО: декорации в W-space не видны** («Декораций нет, только астероиды»). Декор СЛИПЕРОВ СПАВНИТСЯ (лог 17:26:31: 30278/30505/30903/30507/30807/30502/30280/10142/10756/30905/30514/10233/30903/30299/10812/30927/30300/30276/30273/30506 — все пулы, для 4013 whClass4). AllowNonPublished=true (все типы грузятся). Проблема в ДОСТАВКЕ/РЕНДЕРЕ. Текущий флаг ItemSystemEntity::EncodeDestiny = `IsMassive` (c5139949). История флагов: bf85aa8c IsMassive (видим) → 45bfb571 IsInteractive (коллизия) → 0b5b14f5 IsGlobal (warp-in) → c5139949 обратно IsMassive. Астероиды (ObjectSystemEntity, ЕСТЬ destiny, flags=0) видны; декор (CelestialSE, БЕЗ destiny, IsMassive) — нет. Гипотеза: клиент не создаёт шар без destiny, если не IsGlobal (гейты/планеты IsGlobal видны). СЛЕДУЮЩИЙ ШАГ: включён BALL_DECODE/SE__DESTINY/DESTINY__MESSAGE в log.ini; юзер пересобирает, заходит в ВХ сайт, читаем «Ball Decoded» — есть ли декор-типы в AddBall. Если шлются — менять флаг (пробовать IsGlobal как в 0b5b14f5). Декомпил: LargeCollidableObject.py рендерится стандартно, флаги обрабатываются в destiny.dll (C++).
- **W-space local канал — оставлен СКРЫТЫМ по лору**: юзер подтвердил «по мему в ВХ его быть не должно». LSCService::CreateSystemChannel: `IsWSpaceID → type=solarsystem, name="System"` (без списка/счётчика). НО Crucible-клиент ВСЁ РАВНО рендерил счётчик, т.к. боты добавлялись через `AddBotChar` → фикс `c2da9c8f`: в SpawnBot боты НЕ добавляются в local канал W-space систем (`!IsWSpaceID(pSystem->GetID())`).
- **Боты в W-space спавнились на (0,0,0) у солнца** (нет гейтов/станций) → `DestinyManager::SetPosition` traceStack-дамп (НЕ краш). Фикс `3e9a66df`: fallback — орбита первой планеты/луны (8-28км). PopulateSystem не исключает W-space (PlayerCount>=1), GetRandomAdjacentSystem для ВХ=0 → SpawnBot напрямую.
- **Док-фикс (`abdeb8fe`)**: CmdDock при IsWarping() больше не отклоняет — `RequestDockAfterWarp()` (m_dockRequested), `WarpStop()` в конце выполняет `AttemptDockOperation()` в try/catch (UserError DockingApproach вне RPC — проглотить); Stop/Halt сбрасывают флаг. Мини-баг «док раньше конца варпа» устранён.
- **Слипер-декор по классам ВХ (`abdeb8fe`)**: SpawnDecorations получила `whClass` (из dungeonID: combat 4001-4024 → (id-4001)/4+1, Data 4301-4312 и Relic 4401-4412 → /2+1). Пулы sleeperClouds (голубые 10068/10759/10810/10811/10812), sleeperDebris (10142/30514/10232/10233/10756), sleeperConverters (30300/30274/30905/30797/30798/30806/30807), sleeperOutposts (30299/30273/30301/30277/30513/30512/30901/30927/30902), sleeperCitadels (30293/30276/30302/30275/30502/30509/30503/30510/30504/30507/30505/30508/30506/30511/30903/30904/30278/30279/30280). C1-C2 debris+converters, C3 +outposts, C4-C6 +citadels; для слиперов НЕ мешаются lcoDeco/rockDeco (isSleeper).
- **Размер облаков исправлен (`abdeb8fe`)**: SDE radius облаков (группы 227/312) = 1м, клиент масштабирует `2*radius` (cloud.py SetRadiusDX8) → облака были 2-метровыми точками. В SpawnDecorations облакам задаётся `AttrRadius = 1500+rand*4500` (1.5-6км) ДО создания SE.
- **Саньша (`9d822d2b`+`6a7c6aa0`)**: sanshaDeco + LCS 319 (16813-16821, 17381, 28252 Battlestation, Freight Pad); турели во все аномалии (Tower Sentry Sansha I-III 17157/17156/16746, Point/Light/Heavy/Cruise Missile Battery 17580-17583, Stasis Tower 17607, Energy Neutralizer Sentry 28142/28147/28152); новые аномалии Sansha Lookout (2094, 2 комнаты, Sansha Control Center 26249 + True Sansha Misshape 23391) и Sansha Refuge (2096); DED Sansha Prison Camp 8/10 (2730, Central Bastion 3805), True Sansha Fleet Staging Point 9/10 (2830, Sansha Fleet Outpost 29023), Centus Assembly T.P. Co. 10/10 (2930, Station Ultima 19961); Dark-Blood-аналоги True Sansha командеры (807-810, 851); Forlorn/Forlorn Rally турели.
- **Гуристас (`4cab6017`)**: guristasDeco + Guristas Bunker 16796, LCO Shipyard 23741, Spaceshuttle Wreck 10138, Debris; турели (13068, 17596-17599, 17611, 24767, 28141/28146/28151); новые аномалии Guristas Rally Point (2097, Dread Guristas Saboteur 13598), Port (2098), Sanctum (2099); DED 4/10 переименован в «Guristas Scout Outpost» (2520); DED Pith's Penal Complex 7/10 (2740, Guristas Control Center 26248 + Dread Pith), The Maze 10/10 (2940, Mazed Karadom's Armageddon 24664); Hideaway/Burrow пересобраны (Pithi Wrecker/Plunderer 17006/17001), убраны Renegade Angel Goon.
- **Ангелы (`eee099ce`)**: angelDeco + Angel Battlestation 11077/17138/28247; турели (13114, 17572-17575, 17605, 27280, 28139/28144/28150); Angel Sanctum (2012); Domination спавны (Thug 13518, Crusher 13523, Breaker 13529, Commander 13535, General 13537, Saint 13540, War General 13539); DED 3/10 → «Angel Repurposed Outpost» (2300); Hideaway/Burrow пересобраны (Gistii 16901-16903, Gistior 24229/24230).
- **Кровавые Рейдеры (`8c291766`)**: bloodDeco + Cathedral 16727, Chapel 16731, Deadspace Tactical Unit 17380, Bloodsport Arena 17393, LCS Bhaalgorn 25552; турели (13116, 17145/17144/16741, 17592-17595, 17610, 27281, 28140/28145/28149); Blood Haven (2030), Blood Sanctum (2031); Dark Blood спавны (Visionary 23287, Bishop 23293, Exorcist 23298, Cardinal 23300, Patriarch 23301, Monsignor 23299, Pope 23302, Oracle 13559, Apostle 13562, Archbishop 13560); DED Blood Raider Temple Complex (2710, Blood Raider Control Center 26247 + Corpus Patriarch/Pope 24139/24140 + Dark Blood; нейтрализаторы/стазис по лору); Den/Hideaway пересобраны (Corpior 23970-23982, Corpum 16927-16949), убраны Renegade Guristas.
- **Серпентис (`717f6529`)**: ⚠️ dunDungeons хранит Серпентис как factionID **500013** (SDE), а не `factionSerpentis` (500020 в EVE_Corp.h) — SpawnDecorations/тир обрабатывают ОБА. serpentisDeco + Stronghold 11076, Battlestation 23949, Research Station 28258, Research Station Ruins 9879/9891/9897, Caldari Research Outpost 4100, LCS 16822-16830; турели (13115, 17568-17571, 17163, 28143/28148/28153); новые аномалии Rally Point (2087), Port (2088), Hub (2089), Haven (2050), Sanctum (2051); Shadow Serpentis спавны (Trooper 23457, Wing Leader 23463, Captain 23466, Admiral 23469, Grand Admiral 23471, Lord Admiral 23472); DED Serpentis Pharmalogical Plant (2720, нарко-платформы Crash/Exile/Mindflood 26856-26858, Serpentis Control Center 26250), Serpentis Research Complex (2920, Core Serpentis Operational HQ 24579); Hideaway пересобран (Coreli 17116-17121, Corelior 23973-23994), убраны Renegade Serpentis Assassin.
- **Восставшие Дроны (`42bbab25`)**: rogueDroneDeco + Drone Structure I/II 16732/16733, Infested station ruins 16736, Reinforced Drone Bunker 4011; турели (18023, 18031-18035, 27953-27955, 27956); полная лестница 10 типов: Cluster 2090, Collection 2091, Assembly 2092, Horde 2093 + новые Gathering 2140, Surveillance 2141, Menagerie 2142, Herd 2143, Squad 2144, Patrol 2145 (Alvi 805→Alvior 804→Alvum 803→Alvatis 801→Alvus 802); Sentient командеры (27738 Annihilator Alvum, 27728 Patriarch Alvus, 27748 Dismantler Alvior, 27722 Alvus Queen, 27726 Domination Alvus); DED 3/10 (2350) турели.
- **Миссии (`ae7b4835`)**: ВСЕ 2970 agtMissions имели dungeonID=0, qstEncounter пустая (70 записей, dungeonID=0) → ни одна миссия не спавнила карманы. Арки уже есть (epicArc: Blood-Stained Stars, Angel Sound, Smash and Grab, Right to Rule, Penumbra, Syndication, Wildfire, Vision of Greatness; epicArcChapter 26, epicArcMission 197). Созданы mission-данжи (archetypeID 1): Mission Vengeance (5100, 3 комнаты, Hoborak Moon 20439 + Angel Webifier 16562 + Angel Battlestation 11077 как декорация), Recon (1 of 3) (5110, Blood Raiders, ускорительные врата 24км), Ride to the Rescue (5120, Yukiro Demense 32391), Fear of Angels (5130). Привязка в `qstEncounter` (id=agtMissions.id): 54351/54352/54581→5100, 55369/55420→5110, 80064→5120, 80076→5130. AgentBound:341 спавнит данж через `SELECT dungeonID FROM qstEncounter WHERE id=missionID AND dungeonID>0`.
- **ВАЖНО про тип канала ВХ**: в `LSCService::CreateSystemChannel` W-space = `LSC::Type::solarsystem` + name «System» (скрытый local, как в реале). НЕ МЕНЯТЬ на solarsystem2 (юзер против).
- **Тиры декора в MakeDungeon**: whClass вычисляется для всех фракций по dungeonID (Sansha 2066-2072=2/2100-2133=3/2330-2630=3; Guristas; Angel; Blood; Serpentis; Drone). SpawnDecorations(roomPos, factionID, whClass).

## 14 августа: DroneAI EWAR-cleanup, полная очистка ВХ, тестовые линии, Reverse Engineering из реликтов
**Коммиты: `9b435733` (DroneAI fix), `8c009e9d` (RE из реликтов), `eeaa51b2` (RE случайный T2 BPC + не слать ложный успех), `fed99ac1` (маркет debug-logs). Все в origin/master. Сервер пересобран юзером, работает.**
- **DroneAI fix (`9b435733`)**: `DroneAIMgr::ClearTarget` теперь снимает web/paint/warp-scramble с цели перед сбросом лока — `CleanupTargetEwar(pSE)` (стоп лучей WarpScramble/ModifyTargetSpeed/TargetPaint start=0, `AttrWarpScrambleStatus=0`, `WebbedMe(false)` симметричный undo, восстановление оригинальной сигнатуры). Аналогично `SetIdle` (чистит все оставшиеся цели + painted) → EWAR снимается при смерти дрона (`DroneSE::Killed`→SetIdle) и scoop/abandon (`DroneSE::Offline`→SetIdle). `WebAttack`/`PaintAttack` идемпотентны (undo перед re-apply, трекинг `m_webApplied`/`m_webTargetID`) — веб больше не стакает скорость в ноль, paint не раздувает сигнатуру. Все прямые `TargetMgr()->ClearTarget()` в DroneAI.cpp заменены на `ClearTarget()` метод. Сборка подтверждена (100%). Юзер подтвердил: «дронов убил, ульта упала».
- **Полная очистка ВХ перед пересборкой** (по запросу юзера): скрипт `cleanup_wormholes.sql` удалил все 411 ВХ-сигнатур из `sysSignatures` (dungeonType=6) + 322 ВХ-сущности из `entity` (groupID 988) + `entity_attributes`. Выполнен при выключенном сервере. После старта сервер заспавнил нормальные ВХ (юзер: «ВХ стало потяжелее сканить, но это и хорошо»). Скрипт на сервере: `/tmp/cleanup_wormholes.sql`.
- **Линии индустрии — НЕ баг данных**: сравнена наша БД с ОФИЦИАЛЬНЫМ Crucible SDE (fuzzwork `https://www.fuzzwork.co.uk/dump/old/Crucible/cru16/`, файлы `ramAssemblyLineStations.sql.bz2`/`ramAssemblyLineTypes.sql.bz2`/`ramAssemblyLines.sql.bz2`). ПОЛНОЕ совпадение: 2252 станции с линиями, те же типы (5:546, 6:842, 7:546, 8:546, 35:1362, 36:2, 38:546), 546 research-станций, 24 в The Forge. Включая 120 станций с типом 35 в лоусеке — так и в SDE. «Инженерный анализ» (RE, activity 7) на NPC станциях в Crucible НЕ существовал (только POS-структуры тип 158/3). Перераспределение НЕ нужно.
- **Тестовые линии на станции 60001795 (Uemon VIII - Moon 10 - Zainou Biotech Production)**: по запросу юзера добавлены по ОДНОЙ линии каждого типа из `ramAssemblyLineTypes` (~145 типов, quantity=1) в `ramAssemblyLineStations` + по одной индивидуальной строке в `ramAssemblyLines` (assemblyLineID от 100005093, costInstall=1000/costPerHour=333/restrictionMask=0). Юзер оставил их для тестов. Откат: таблицы `ramAssemblyLineStations_bak_20260814` + `ramAssemblyLines_bak_20260814`.
- **Reverse Engineering из реликтов (`8c009e9d` + `eeaa51b2`)**: серверный `InstallJob` требовал categoryID==Blueprint для ЛЮБОЙ активности → правый клик по реликту (кат. 34) давал `RamActivityRequiresABlueprint` («просит чертёж»). Фикс: для activity RE разрешена категория AncientRelics (34) — `isRelicRE` (пропуск Blueprint-специфичных шагов: ActivityCheck/StaticCast/split/ChangeSingleton/Move/UpdateRuns, ручной расчёт времени 1ч через `GetAssemblyLineProperties`+`EvEMath::RAM::InventionTime`+`sConfig.ram.ReTime`, `ItemOwnerCheck` переведён на `InventoryItemRef`). Завершение RE (`CompleteJob`): для реликта выдаёт случайный T2 BPC. БАГ в `eeaa51b2`: `GetRandomBlueprint` выбирал из ВСЕХ published invTypes — а `invBlueprintTypes` содержит 18 РЕЛИКТОВ (кат. 34: 30187/30605 и т.д.), их typeID не является Blueprint → `Blueprint::Spawn` возвращал nullptr → BPC не создавался, но `CompleteJob` всё равно слал клиенту «успех» (юзер видел «Доставлен» без чертежа). Фикс: `GetRandomBlueprint` фильтрует `g.categoryID=9 AND bt.techLevel=2` (789 T2-чертежей); `CompleteJob` при `Spawn==nullptr` отвечает «провал» вместо ложного успеха.
- **Маркет sell-ордер «60 jumps» — ЗАКРЫТО (`ee9613be`, 16 авг)**: юзер на станции 60001795 выставлял sell-ордер → «Your Marketing skill only allows sell orders within 60 jumps», ордер не создавался. КОРЕНЬ: окно простой продажи (SellStuff) всегда шлёт `orderRange=32767` (rangeRegion), сервер трактовал его как «32767 прыжков» и резал по Marketing (5→60). Фикс: спец-диапазоны клиента `rangeStation(-1)/rangeSolarSystem(0)/rangeConstellation(4)/rangeRegion(32767)` больше НЕ лимитируются скиллом — только jump-based (1..60+). Аналогичный фикс для buy (Procurement). Диагностика `fed99ac1` (dump orderRange/maxSellRange/marketingLevel) осталась в коде под MARKET__DUMP.
- **Лут слиперов в трюме на станции (правый клик не работает)**: НЕ баг typeID — типы 30605/30618/30633 есть в клиентском SDE (`D:\Games\EveTest\bulkdata\600004.cache2`). Симптом «правый клик не работает в трюме корабля на станции, в ангаре работает, перетаскивание работает» — клиентская логика меню (`menusvc._InvItemMenu`). Сервер не при чём. Требует разбора декомпила `script__ui_services_menusvc_py.py`. НЕ решено.
- **ВАЖНО про клиент**: реальный клиент в `D:\Games\EveTest` (НЕ `C:\CCP\EVE`). `bulkdata\600004.cache2` = invTypes, `1800006.cache2` = ramAssemblyLineTypes (есть тип 38 STATION Invention!), `1800003.cache2` = ramActivities. Официальный современный SDE (112MB zip) НЕ содержит `ramAssemblyLineStations`.
- **ВАЖНО про сервер**: server контейнер запускается `-t -i` (иначе спам «Command not recognized»). Логи в `/app/logs/`. `docker-compose down` не удаляет контейнеры db/server (Conflict) — сначала `docker rm db server`. Ремарка: при пересборке после `git pull` ОБЯЗАТЕЛЬНО `docker-compose build --no-cache server` (кэш слоёв не подхватывает git-изменения). Проверка бинаря: `docker exec server grep -c GetRandomBlueprint /app/bin/eve-server` (должно быть >0).
- **Полезные команды диагностики**: grep по логам через base64 (`echo $b64 | base64 -d | sh` внутрь `docker exec server sh -c`), чтение БД через `docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu`.

## 13 августа (вечер): W-space сайты + СЛОМАННЫЙ ТРЮМ ИСПРАВЛЕН (кастомные typeID 34100+ = KeyError в клиенте)
**Коммиты: `507b12bd` (реальные typeID лута), `f178afc0`+`5e113ecd`+`421f3892` (полный набор W-space сайтов). Сервер пересобран, master синхронизирован (`421f3892`).**
- **ИЗВЕСТНЫЙ КРАШ (`IncRef() called on deleted object!` — SIGSEGV)**: сервер упал 19:03 (Exited 139) при активном бое ботов (`Zalimir-7`, `Xolirix II`, `MrStmnyy` commander fleet bonus). RefPtr use-after-free (count 4630) в бою. НЕ разобран — нужен анализ (вероятно флот-бонус командира или орбита при смерти цели). Сервер перезапущен юзером.
- **КОРЕНЬ «лут не виден + трюм ломается»**: кастомные typeID 34100-34111 НЕ существуют в клиентском SDE (Crucible). `cfg.invtypes.Get(34100)` → `KeyError('RecordNotFound')` в `uix.GetItemData/GetItemName/GetCategoryGroupTypeStringForItem` → uthread рендера окна врека/трюма умирает → пустые вреки + сломанный трюм. Сервер присылает строки корректно (фильтр длины invCache.py:993 не отсекает), проблема чисто клиентская.
- **Фикс (`507b12bd`)**: используются РЕАЛЬНЫЕ typeID из клиента (в БД уже были):
  - Propulsion Relics (971): Intact 30187 / Malf 30558 / Wrecked 30562
  - Electronics (990): 30599/30600/30605; Offensive (991): 30628/30632/30633
  - Engineering (992): 30582/30586/30588; Defensive (993): 30614/30615/30618
  - Hull (997): 30752/30753/30754; Sleeper Components (880): 30744-30747
  - Тир по классу Sleeper: Sleepless=Intact, Awakened=Malf, Emergent=Wrecked. Кастомные 34100+ удалены из БД и миграции.
- **W-space сайты по вики EVE University** (`f178afc0`, миграции `20260813000002`+`20260813000003`):
  - Боевые (Anomaly): 24 данжа 4001-4024, 4 варианта/класс — Perimeter (C1-2), Frontier (C3-4), Core (C5-6); accel-гейты + третий рум C3+.
  - Data (Magnetometric): 4301-4312 «Unsecured Perimeter/Frontier/Core *» — контейнеры 23 + Sleeper-стражи.
  - Relic (Radar): 4401-4412 «Forgotten Perimeter/Frontier/Core *» — контейнеры + стражи.
  - Оре (Gravimetric): процедурно — сигнатурный item + `SpawnMineableAsteroids` (SpawnMineableAsteroids сделан public).
  - Газ (Ladar): процедурно — сигнатурный item + реальные газовые облака grp 711 (Cytoserocin/Fullerite).
- **ВАЖНО про миграции**: при ручном применении выполнять ТОЛЬКО Up-часть (до `-- +migrate Down`) — иначе INSERT+DELETE=0. Узнать номер строки Down: `grep -n "migrate Down" file`. Применять `head -N file | mariadb`.
- **ВАЖНО про кастомные typeID**: любые новые типы, которых нет в клиентском `cfg.invtypes`, ломают интерфейс. Только реальные SDE-типы (уже в БД: 30187-30754, 25268+, 30370+).
- Трюм юзера очищен от фейковых предметов (140111724/140111723).

## 13 августа (день): ВХ-связки работают, найден и исправлен «мусор ВХ» в БД
**Коммиты: `93938f60` (Collapse чистит sysSignatures), `862fdfba` (LoadAnomalies удаляет осиротевшие сигнатуры), `51f15f5f` (лимит ВХ: k-space ≤1, w-space ≤2), `1847064f` (build fix DBerror). Юзер подтвердил: вход/выход через 4 разных ВХ (нули, лоу, 2×w-space) работает без проблем.**
- **Юзер проверил 4 ВХ**: в нули, лоу и 2 в w-space — вход, выход через дырки, всё ок. Связка (входной ВХ ↔ K162) стабильна.
- **Накопление «мусорных» ВХ** (проверено по БД): 491-509 сигнатур в `sysSignatures` (dungeonType=6), из них 141 в Uemon (30000197), 125 — сироты без живой пары. Причина: каждый вход юзера в ВХ создавал K162 + `SaveAnomaly` INSERT в `sysSignatures`, а `Collapse` НЕ удалял строку → `LoadAnomalies` грузила все навсегда → «много ВХ при сканировании».
- **Фиксы**:
  1. `93938f60`: `WormholeMgr::Collapse` удаляет сигнатуры обоих ВХ (entrance + exit) из `sysSignatures` через `sDatabase.RunQuery`.
  2. `862fdfba`: `AnomalyMgr::LoadAnomalies` пропускает и удаляет осиротевшие ВХ-сигнатуры (entity отсутствует) через `ManagerDB::RemoveAnomaly`.
  3. `51f15f5f`: `WormholeMgr::Create` кап ВХ на систему (k-space ≤1, w-space ≤2); K162 exit (`exitSystemID!=0`) не лимитируется (паруется с входным).
  4. `1847064f`: build fix — `RunQuery(DBerror&, ...)` не принимает rvalue `DBerror()`, нужна локальная переменная.
- **ВАЖНО про пересборку**: `docker-compose build server` кэширует слои — если git pull сделан после сборки, фиксы НЕ попадут в бинарь (бинарь собран 13:05, исходники обновлены 14:52-16:01). Лечится `docker-compose build --no-cache server`. Проверка: `strings /app/bin/eve-server | grep -c RemoveAnomaly` (должно быть >0).
- **ДИСК ПОЛНЫЙ (0 байт)**: лечится `docker image prune -af` + `docker container prune -f` (освободил 46GB). ⚠️ `container prune` удаляет остановленные контейнеры, включая `db` — данные в volume `evemu_db` не теряются, контейнер пересоздаётся: `docker run -d --name db --network evemu_default -v evemu_db:/var/lib/mysql -e MARIADB_RANDOM_ROOT_PASSWORD=true -e MARIADB_USER=evemu -e MARIADB_PASSWORD=evemu -e MARIADB_DATABASE=evemu mariadb:11.8 --innodb-buffer-pool-size=1G --innodb-log-file-size=256M --innodb-flush-log-at-trx-commit=2 --max-allowed-packet=64M --bulk-insert-buffer-size=64M`.
- **Текущее состояние БД**: 509 сигнатур ВХ, чистка произойдёт при заходе игрока в систему (LoadAnomalies на новом бинаре удалит сироты автоматически).
- Проверка связи: C391(140105413)↔K162(140105414) J151817↔Onsooh — рабочая пара.

## 13 августа (ночь): ВХ-прыжки работают, найден краш в w-space из-за ботов-майнеров
**Коммиты запушены: `08685d26` (guard maxJumpMass + CreateExit всегда при destItemID==0), `698e1f96` (краш GetRandBeltID в w-space). Юзер пересобрал; вход в ВХ и возврат через K162 работают.**
- **Вход в ВХ из J151817 → Onsooh (30000013) сработал**: `WormholeSvc mass check: ship=1270.0t maxJump=2000000t` → юзер перенесён, корабль (Buzzard 140102525) сохранён в 30000013, связка C391(140105413)↔K162(140105414) цела. Краш при этом входе — см. ниже.
- **Краш `std::out_of_range: vector::_M_range_check: __n (which is 0) >= this->size() (which is 0)`** (`698e1f96`): `NPCAI::WarpOut` → `SystemManager::GetRandBeltID()` использовал `m_beltVector.at(MakeRandomInt(0, m_beltCount))`. В w-space (31000000+) **0 поясов астероидов** (J151817=0, все w-space=0), а боты-майнеры/PvP в системе с игроком звали `GetRandBeltID` → `.at(0)` на пустом векторе → abort. Фикс: `GetRandBeltID` возвращает 0 при пустом векторе; NPCAI:721 проверяет `GetSE(0)==nullptr` → Idle.
- **`08685d26` (2 фикса ВХ)**:
  1. `WormholeSvc::WormholeJump` отклонял корабль когда `maxJumpMass=0` (у exit K162 до копирования атрибутов источника) — добавлен guard `maxJumpMass>0` (как в Client).
  2. `Client::WormholeJump` создавал exit K162 ТОЛЬКО если `m_movePoint==NULL_ORIGIN`, но `m_movePoint` мог нести stale-значение → CreateExit не вызывался, юзер попадал к солнцу. Теперь K162 создаётся всегда при `destItemID==0` (нет парного exit), независимо от m_movePoint.
- **Проверка данных**: K162 140105414 в 30000013 (`TargetSystem1=31002458` J151817, `TargetSystem2=140105413`, maxJump=2000000, maxStable=4998730); C391 140105413 в J151817 (`TargetSystem1=30000013`, `TargetSystem2=140105414`). 30000013=Onsooh, 30000028=Eshtah (НЕ перепутать: K162 ведёт в Onsooh).
- **Известный симптом до фиксов**: «unsupported location 20000028» (мусор в TargetSystem1) — устранено восстановлением миграции wormhole_classes + `f4a46ecc` (CreateExit(unloaded) → TargetSystem1=sourceItemID).

## 🔴 КРИТИЧНО ДЛЯ СЛЕДУЮЩЕЙ СЕССИИ: ПОТЕРЯННЫЙ БЛОК SLEEPERAI / W-SPACE (восстановить!)

**Юзер подтвердил: был написан целый блок «Слиперы и червоточины», потом при глобальном сбое всё сломалось и был откат на пару дней. 104 коммита ушли из master ресетом `e512bc1b` (28 июля, «moving to origin/master»). Файлы SleeperAI/Sleeper.cpp в текущем master ОТСУТСТВУЮТ — но коммиты достижимы через reflog и МОГУТ быть восстановлены.**

В текущем коде остались только следы: `Spawn::Group::Sleeper` (пустая заглушка в SpawnMgr.cpp, нигде не присваивается), `factionSleepers=500023` (EVE_Corp.h, hostile-статус в NPC.cpp:278), группы NPC 959-961,982-987 в БД (Deadspace Sleeper Sleepless/Awakened/Emergent Sentinel/Patroller/Defender), Sleeper Turret (группа 53), декор Sleeper Debris (306). Спавна слиперов в ВХ НЕТ.

**Потерянные коммиты (все достижимы, предки `e512bc1b`/`64718231`):**
- `6748ee96` feat: SleeperAI — remote armor/shield repair, energy neutralizer, advanced target switching (**SleeperAI.cpp 179 строк + SleeperAI.h 38 строк + CMakeLists + NPC.cpp**)
- `01094494` fix: SleeperAI use m_npc->TargetMgr() instead of m_pDrone
- `0be18f5d` fix: SleeperAI — public accessors, remove override
- `85add6d4` fix: SleeperAI — add Client.h include
- `c269bbe8` feat: Sleeper capital escalation — CheckCapitalEscalation spawns 6/8 Guardians when capital enters bubble
- `a999b56b` feat: W-space dungeons — acceleration gates between rooms, third wave for C3+ sites
- `3cabe90b` feat: Sleeper NPC system — typeIDs, W-space dungeons (C1-C6), salvage/relic loot drops (**sql/migrations/20260730000000-sleeper_system.sql 158 строк + NPC.cpp**)
- `d37cca16` feat: W-space Sleeper anomaly spawning — auto-detect WH class, create Sleeper combat sites with factionSleepers
- `19f5a188` feat: W-space — guaranteed wormhole per system (1 WH minimum for classes 1-6)
- `ad5c081e` feat: K-space wormholes — 5% chance for anomaly to become WH in hi/lo/null-sec
- `bf152208` feat: mapLocationWormholeClasses from SDE
- миграции-фиксы: `f7439b2d`, `20d81927`, `273a5c24`, `130d2d20`, `2145ab10` (typeName/description, valueFloat, categoryID, salvage typeID 34010-34021→34100-34111, NULL-fix)

**Восстановление**: `git cherry-pick` на master по одному (порядок ~ хронологический, сначала `bf152208`→`3cabe90b`→`6748ee96`→...), аккуратно разрешая конфликты с текущим кодом (за 2 недели код сильно ушёл вперёд: боты, декор, эффекты). Либо `git checkout e512bc1b~1 -- <файлы>` для точечных файлов. **Проверить, что `e512bc1b~1` (= `4074df35`) — голова до ресета, из неё всё достижимо.** После cherry-pick: проверить, что миграции не конфликтуют с текущими, БД-таблицы (invTypes/dgmTypeAttributes для слиперов) на месте.

**ПОЛНЫЙ СПИСОК ПОТЕРЯННЫХ КОММИТОВ (104 шт, `git log --oneline e512bc1b~1 --not master`, достижимы через reflog):**
```
4074df35 fix: remove all PackagedAction/SubStream from QueueDestinyUpdate — DoPackage updates go through transformation (no SubStream); SetState is the only immediate path
4995dd83 fix: auto-generated AddBalls2 format is (stamp, (AddBalls2, ((state, extra),))); inner[1] is already the complete chunk — no extra wrapping needed
d2595ebd fix: transform BOTH nested and flat non-string formats — (stamp, (str, args)) and (stamp, str, args) → (str, ((args,),))
5e445eb7 fix: transform (int, (str, args...)) → (str, (args,)) for apply() — wraps args in single-element tuple for correct dispatch
894ad342 fix: restore Client.cpp from 7599cdcc — was corrupted by null char injection
b93c1f63 revert: remove all AddBalls2 wrappers — unknown Encode() format causes SIGSEGV; keep only PyIncRef fix from 7599cdcc
42d9b469 fix: addballs2.Encode() returns (stamp, (AddBalls2, state, extra)) — get state/extra from inner tuple
7523a47e fix: all 4 AddBalls2 callers wrap as (AddBalls2, (state, extraBallData)); revert _SendQueuedUpdates to original (SubStream crash is pre-existing build-13 bug)
0458e655 fix: all 4 AddBalls2 callers now wrap as (AddBalls2, (state, extraBallData)) — eliminates TypeError from flat-format tuples
a37ecc67 fix: SystemManager AddBalls2 — wrap as (AddBalls2, (state, extraBallData)) for client RealFlushState
2a143703 fix: filter SubStream items from queue before encoding — prevents Marshal recursion from any source
bb400acd docs: sync AGENTS.md with session end state — SIGSEGV fixed, TypeError fixed, server stable under load
884fcb41 fix: remove ALL PackagedAction/SubStream from QueueDestinyUpdate — only SetState immediate, all else through queue with transformation; eliminates Marshal recursion entirely
5892dcf7 fix: ShipService AddBalls2 — use QueueDestinyUpdate with DoPackage=true instead of BubblecastDestiny (no funcName wrapping)
0f0bd022 fix: transform non-string updates (stamp, (funcName, args...)) → (funcName, stamp, args...) for client RealFlushState
4b2c241a fix: manual PackagedAction via Marshal+PyBuffer (no PySubStream) — wraps non-string updates as (PackagedAction, buffer); eliminates Marshal recursion
cb9594b7 fix: disable all PackagedAction wrapping — SubStreams cause infinite Marshal recursion (MarshalStream does not track visited objects); client gets TypeError on non-string updates but server no longer crashes
7599cdcc fix: re-add PyIncRef in deferred path (was reverted with build 13 rollback) — prevents use-after-free in FlushPendingDestinyUpdates
780c08a0 fix: remove paList->AddItem(m_destinyUpdateQueue) — causes circular reference in MarshalStream (infinite recursion → SIGSEGV), existed in build 13
08fb4559 revert: full code rollback to build 13 (9f822aa4) — keep SQL/doc changes
07a97727 fix: disable DoPackage path (except SetState) — PackagedAction wrapping creates circular refs that crash MarshalStream; all updates go to queue directly
9b378ca7 fix: re-apply circular reference fix (paList->AddItem(m_destinyUpdateQueue) was restored by Client.cpp revert; clone items instead)
c455b258 fix: PyIncRef in QueueDestinyUpdate deferred path — prevents use-after-free when caller PyDecRef's after return; pre-existing build-13 bug
3ce19cff fix: revert Client.cpp to pre-build-14 state — all QueueDestinyUpdate/Flush changes reverted, client was unplayable
95544d5d revert: remove QueueDestinyUpdate validation — caused client hang (was too aggressive, skipped all binary updates)
1d5fa3ad fix: validate QueueDestinyUpdate — skip updates with non-string first element (prevents RealFlushState crash)
db681019 fix: clear destiny queues in SetBallPark — stale data from previous system/crash caused malformed DoDestinyUpdate
f6a43af0 fix: circular reference in QueueDestinyUpdate — paList->AddItem(m_destinyUpdateQueue) created cycle, causing MarshalStream infinite recursion and SIGSEGV; clone items instead
127e5f32 fix: new migration for remaining decorations — first cleanup used wrong threshold (>100M instead of >14M)
6b3f2d96 fix: update cleanup SQL threshold to itemID > 14000000 (was 100M); add 10753 (SoftCloud) to cleanup list
aaef9200 fix: filter null items from queue before sending DoDestinyUpdate; prevents IndexError on empty list
bb8c0f03 fix: PyIncRef queues in _SendQueuedUpdates to prevent struct destructor from freeing shared queue pointers
7aba25be fix: disable SpawnDecorations (causes client issues); cleanup SQL already exists
067ae3f0 fix: skip empty DoDestinyUpdate — client crashes with IndexError: list index out of range on empty updates list
3685f466 fix: clear m_pendingUpdates in Client destructor — stale pointers from disconnected session caused use-after-free in FlushPendingDestinyUpdates on reconnect
cf1278ed fix: use-after-free in FlushPendingDestinyUpdates — DoDestinyAction destructor PyDecRef's update, causing double-free on next iteration
a46ce6bd fix: SpawnDecorations call/definition mismatch after revert — header expects 4 args, call passed 2
937bc9e5 docs: sync AGENTS.md with session end state
64718231 revert: полный откат кода к состоянию до build 14 кроме SleeperAI/SQL
e97fff61 fix: build error — act.update is PyRep*, use intermediate PyTuple* for SetItem
7ac9f6ed fix: AnomalyMgr crash — RemoveSignal already erases from m_sigBySigID, don't double-erase
c47f5af7 fix: replace PackagedAction+PySubStream with manual ('PackagedAction', PyBuffer) to avoid MarshalStream crash
2ea51171 fix: AnomalyMgr — add signature expiry (5min test / 30min prod) so new types keep spawning
585f4cfa fix: AnomalyMgr — add extra types on first cycle even if at max (so exploration sites get a chance to spawn)
104c9103 fix: GetShipState/GetChargeState return empty dict instead of nullptr (client crashes on NoneType iteration)
53c3b2bb fix: add Survey_Probe to Scan_Probe_Launcher compatibility (Expanded/Sisters launchers accept survey probes too)
5d261ab1 fix: add Scan_Probe_Launcher and Interdiction_Sphere_Launcher to IsChargeCompatible (probes couldn't be loaded into launcher)
529a3ae7 fix: AnomalyMgr re-populate typeList each cycle so new signatures keep spawning (not just on first load)
af891d5c fix: warp exit — use distance-based (ship radius) instead of speed (30 m/s); fix MWD scramble — start warpScrambleTimer on online completion
b7118100 fix: destiny update use-after-free — PyIncRef pending updates, nullify act.update/pa.substream after Encode to prevent destructor double-free
e43152c9 revert: откат Client.cpp (QueueDestinyUpdate/FlushPendingDestinyUpdates/_SendQueuedUpdates), DroneAI, WarpStop к состоянию до build 14
cba83f79 fix: warp exit jerk — use distance-based exit (≤radius) instead of speed (30 m/s), remove position snap in WarpStop
2145ab10 fix: add SQL fix script for NULL typeName/description on existing sleeper types
f7439b2d fix: sleeper migration — add typeName/description to all invTypes INSERTs (prevents nullptr crash in StaticDataMgr::Populate)
1dcc4c72 fix: incursion waves — merge gate INSERT split by comment (syntax error)
20d81927 fix: sleeper migration — dgmTypeAttributes uses valueFloat, not value
273a5c24 fix: sleeper migration — remove categoryID from invTypes INSERT, fix salvage typeID collision (34010-34021→34100-34111)
ecfcef74 docs: sync session info — Sleeper NPCs, WH, exploration, missions, incursion pockets, warp physics, mail, defender, MWD, crash fixes, tutorial
42ff980d fix: add npc/NPC.h include to DungeonMgr.cpp (incomplete type)
0ea80bb4 fix: mission destinations — store real coords on accept, return in GetMissionObjectives, create bookmarks, WarpToLocation uses stored coords
60f3a8bd feat: mission dungeons — acceleration gates + reinforcement wave room (procedural)
85add6d4 fix: SleeperAI — add Client.h include (incomplete type error)
aa087d2b feat: incursion waves — 3 pockets per site with acceleration gates (VG/AS/HQ)
ed14a557 fix: exploration site archetype mapping — dungeonType 2→27(Grav), 3-5→31(Radar/Data/Ladar)
ad5c081e feat: K-space wormholes — 5% chance for anomaly to become WH in hi/lo/null-sec
19f5a188 feat: W-space — guaranteed wormhole per system via AnomalyMgr (1 WH minimum for classes 1-6)
d37cca16 feat: W-space Sleeper anomaly spawning — auto-detect WH class, create Sleeper combat sites with factionSleepers
c269bbe8 feat: Sleeper capital escalation — CheckCapitalEscalation spawns 6/8 Guardians when capital enters bubble
a999b56b feat: W-space dungeons — acceleration gates between rooms, third wave for C3+ sites
0be18f5d fix: SleeperAI — use public accessors instead of private NPCAIMgr members, remove override
130d2d20 fix: sleeper migration — merge published flag into main INSERT
3cabe90b feat: Sleeper NPC system — typeIDs, W-space dungeons (C1-C6), salvage/relic loot drops
0b79c418 fix: FlushPendingDestinyUpdates build error (PyStatic returns PyRep*, not PyTuple*)
fbcd2202 fix: reduce WarpStop speed threshold to 30 m/s (more deceleration time = smoother exit)
110b2a8a fix: add direct HasWarpBubble() check in WarpToStuff and WarpTo — blocks warp when in a disruption bubble regardless of AttrWarpScrambleStatus
d8efe9c7 fix: FlushPendingDestinyUpdates — build tuple manually to avoid auto-generated encode destructor issues
01094494 fix: SleeperAI use m_npc->TargetMgr() instead of m_pDrone (DroneAI member)
6748ee96 feat: SleeperAI — remote armor/shield repair, energy neutralizer, advanced target switching
9ef81719 fix: move strength decl before log line (was using uninitialized value — scramble strength always 0)
93ca868c fix: add detailed logging to ScrambleProcess and WarpToStuff to debug why scramble is not blocking warp
bf152208 feat: mapLocationWormholeClasses from SDE — enables WH class lookup for wormhole creation
acc6caa5 feat: exploration sites — Gravimetric/Radar/Data/Ladar dungeons from SDE + procedural room generation
88abf0fc fix: s/AttrIsOnline/AttrOnline
94f36654 fix: DeployableSE — only SetImmediateOnline if AttrIsOnline is set; set AttrIsOnline on Online()
53477c54 fix: LaunchDrone guard — skip items with zero quantity (stack split edge case)
ee0130f4 fix: WarpStop — don't send CmdStop, let client's WarpLoop exit naturally; add migration for rewardTypeID fix
ea50708a fix: revert WarpStop to snap STOP (GOTO overshoots 120km); fix incursion rewardTypeID=1 for ISK, 2 for LP
3b9ce5b8 fix: WarpStop use-after-free — read warp_vector before SafeDelete(m_warpState)
e9d02177 fix: drone/NPC flying-in-place — Stop() now zeros velocity; SetIdle calls Stop(); Stop early-return checks velocity
a4b2edc7 fix: MWD scramble — call SetImmediateOnline when loading DeployableSE from DB (m_onlined was false, Process returned early before timer check)
284ae870 fix: warp exit — coast to GOTO mode instead of snap STOP; speed-based exit threshold
dde018cd fix: LaunchDrone double Move/ChangeSingleton — first move before checks loses drone when bandwidth fails
7b4094f5 fix: incursion rewards ZeroDivisionError — LP entries use lpAmount as quantity, not rewardQuantity (which is 0)
893c12d7 fix: systemic use-after-free in QueueDestinyUpdate — auto-generated struct destructors double-free raw PyRep pointers after Encode
9f993654 fix: disable SpawnDecorations until DB cleanup is complete
5620228b feat: difficulty-based decoration tiers — low/common ore, mid/uncommon+gas, high/rare+ice, faction flavour
87bbb86d feat: add asteroids (1230-1232) to decoration pool, 1-3 objects per room
4ceb059d refactor: safe CelestialSE decorations — only 23/26468 typeIDs, 1-2 per room, RIGID mode, no persistence
7321c9c1 fix: remove SaveItem from SpawnDecorations — decorations are ephemeral, should not persist to entity table
c4a513cb fix: add container/wreck typeIDs (23,3293,3296,3298,3465,24445,26468) to cleanup migration — old decorations with incomplete destiny data cause client IndexError
305d46b9 fix: remove items table DELETE from cleanup migration (table does not exist in this schema)
ac263526 fix: cleanup migration for old decoration entities with invalid typeIDs (beacons, construction parts, etc.)
14cee9fd fix: SpawnDecorations — remove all beacons/unverified typeIDs, use dungeon ownerID, reduce count to 2-5, reduce radius to 500-2500m
4aadbf47 fix: QueueDestinyUpdate must PyIncRef before pushing to m_pendingUpdates (caller may PyDecRef after, causing use-after-free)
```
⚠️ **ВАЖНО про накат**: в этом списке МНОГО коммитов по destiny-очереди/AddBalls2/SubStream, часть из которых УЖЕ пере-реализована в текущем master по-другому (PyIncRef-фиксы, AddBalls2-форматы). Не слепой cherry-pick всех 104! Накатывать ПОШАГОВО, разбирая каждый: многие destiny-фиксы могут конфликтовать или быть уже неактуальными. САМОЕ ЦЕННОЕ: SleeperAI (6748ee96→01094494→0be18f5d→85add6d4), W-space/Sleeper система (bf152208→3cabe90b→d37cca16→a999b56b→c269bbe8), exploration sites (acc6caa5), incursion waves (aa087d2b, 1dcc4c72), mission dungeons (60f3a8bd, 0ea80bb4). Destiny/AddBalls2/decoration коммиты — сверять с текущим кодом перед накатом.

## 12 августа (поздний вечер) — финальная сессия дня: ВХ-фиксы (краши + масса), пробы (Combat/Core), чат ботов
**Коммиты запушены в master: `6b72819d` (ВХ краш), `2b540e88` (масса ВХ), `a83f8598` (краш проб), `08e2178e` (типы проб), `d109cf4b` (скобка), `c9030ea3`/`cce6e949` (чат ботов). Юзер пересобрал; финальная проверка на след. сессии.**

- **Краш при входе в ВХ K162** (`6b72819d`): `Client::WormholeJump` брал `destWh = GetItemRefFromID(AttrWormholeTargetSystem2)` — у EXIT-ВХ этот атрибут = 0 → null → `destWh->position()` → ассерт `RefPtr<InventoryItem>::operator->` (SIGABRT). Фикс: для exit ВХ (`TargetSystem2==0`) лететь к исходному ВХ (`TargetSystem1` = его itemID, система = `locationID`); null-guards в `Client::WormholeJump`, `WormholeSvc::WormholeJump`, `CreateExit(unloaded)`.
- **Краш при возврате пробок** (`a83f8598`): двойной erase из `m_probes` — `RemoveProbe()` звал `sEntityList.RemoveProbe(itemID)` (убивал узел активного итератора), потом `EntityList::Process` erases тот же итератор → SEGV. Убрал лишний `sEntityList.RemoveProbe` из `RemoveProbe()`.
- **«В половину входов не пускает даже фрегат»** (`2b540e88`): ЕДИНИЦЫ. `AttrMass` корабля в **кг** (фрегат 1,155,000 кг), а `wormholeMaxJumpMass`/`wormholeMaxStableMass` в БД — в **тоннах (Mg=1000 кг)** (C1=62000 т, C3=375000 т, C5=2000000 т). Код сравнивал кг с тоннами → фрегат «слишком велик». Фикс: `/1000` в `WormholeSvc::WormholeJump`, `Client::WormholeJump` (валидация) и `ExecuteWormholeJump` (дедукт массы), `OnJump` — в тоннах. Проверено по лору: C1 (62M кг) пускает крейсер/БК, не линкор; C5 (2G кг) пускает дреды/носители (Moros 1.29M т, Nyx 1.62M т), титан (2.076M+ т) — нет (юзер согласился: «войти не значит выйти» — пусть остаётся возможность).
- **Пробы: Combat vs Core** (`08e2178e`): офф-клиент (`scanner.py`) рендерит 5 групп (Anomalies/Signatures/Ships/Structures/DronesAndProbes), логика на сервере. Combat (`probeCanScanShips`=1, тип 30028) находит корабли/структуры/дроны в космосе (`GetAllEntities` → scanGroupID Ship/Structure/DroneOrProbe) + сигнатуры (базовые исследовательские функции); Core (30013) — только сигнатуры (Grav/Ladar/Radar/Mag/Wormhole); **аномалии убраны из результатов проб** (видны в сканере без проб, `ShipScanResult`).
- **Чат ботов без зацикливания** (`c9030ea3`, `cce6e949`): fallback-ответ теперь требует lastUse ≥ 60с; история фраз на канал (`m_channelPhrases`, 10 шт/2 мин) блокирует повтор фразы любого бота; chain-breaker (4 бот-реплики подряд → стоп). SQL-escape апострофов в learned-reply UPDATE.

## 12 августа (поздний вечер) — большой блок: PvP-тактики, дроны, оружие, варп, экономика, портреты
**Сборка последних коммитов на сервере**: юзер собрал; ошибок нет. Боты спавнятся РАЗНЫЕ (баг «один бот» исправлен). 375 ботов, 201+ портрет докачан. Повтор фраз ботов в чате ПОТОМ ЗАКРЫТ (`c9030ea3`+`3998b45a`, 12 авг: no-repeat guards + chain-breaker).

- **Био/корпа по школам фракций** (`e6f7b2d3`): `CreateBotCharacter` выбирает кровь→расу→школу этой расы→корп школы (Imperial Academy/State War Academy/Republic Military School/Federal Navy Academy...). Бот = реальный новичок своей фракции. Без Rogue Drone/Serpentis.
- **Фикс «1 бот в локале» — КОРНЕВАЯ ПРИЧИНА** (`ebbfd509`): inline SQL-комментарий `-- no capsule legends` ломал запрос — C++-строки без `\n`, `--` комментировал `ORDER BY RAND() LIMIT 1` до конца строки → сервер всегда брал первую строку легенды = Sir Tobias Helm. Заменён на `//`-комментарий. Плюс retry-цикл легенд (`092cbd46`, `f5bd1773`): SpawnBot перебирает до 8 легенд, пока не найдёт пилота, не спавненного в системе; useCharID всегда = результат CreateBotCharacter (не live-EVE id).
- **Коллизия/варп** (`5b9ccfd9`, `95a737e7`): выталкивание и warp-bump только когда центр корабля ВНУТРИ объекта (гейты 14-19км раньше «убегали» микротелепортами и сбрасывали скорость при аппроаче). Варп не ждёт медленный разворот на скорости — после align-time heading доворачивается и варп стартует (разворот рендерит клиент во время accel).
- **Оружие по кораблю** (`85947e57`, `9dd9d7eb`): по декомпилу клиента (`spaceObject/entityShip.py`, `turretSet.py`) `gfxTurretID` (атрибут 245) = **typeID реального T1 орудия** — клиент строит модель туррета по нему. Caldari-крейсер/БК/БС → Light/Heavy/Cruise Missile Launcher I (499/501/13320) + реальные ракеты (210/209/203); Amarr→Gatling Pulse Laser (450), Gallente→75mm Rail (561), Minmatar→125mm AutoCannon (484). Дроны и турреты сосуществуют: туррет ставят ВСЕМ по фракции, число дронов = реальный `AttrDroneCapacity`(283 m3)/25, кламп 1..5. Эффект оружия теперь шлётся всегда (`f8841b34`) — раньше `if (gfxID>0)` пропускал.
- **Дроны ботов** (`f3885a35`): дроновые корабли выпускают DroneSE-дронов (Hobgoblin/Warrior/Hammerhead/Acolyte): орбита бота, атака своей цели, **ассист** (атакуют цель союзника-бота), лиш 30км → возврат, скуп при доке/бегстве/смерти. Управление напрямую через DestinyMgr (без DroneAI — он требует ShipSE).
- **Экономика** (`126336db`): трейдеры торгуют ТОЛЬКО докованными (ProcessDockedEconomy, ордера на своей станции), не в космосе; buy-ордера у производителей и трейдеров; курьерки бот-бот.
- **PvP бот-бот** (`8ecaa0b1`, `fffcb634`): хантеры воюют с ботами других корпов в лоу/нуле; стендинги (repStandings) между ботами после боёв (`f385b2d2`) — вражда по стендингам читается (grudge ≤ -1 = известный враг даже в хайсеке); таймер агрессии (`8a459e52`): 30-90с после атаки нельзя докаться/прыгнуть + мигающий значок (OnAggressionChange) + securityStatus (красные черепки от килов). Фракционный воин: 30% хантеров, фиксированные враги по фракции.
- **Портреты при спавне** (`015e1f26`): сервер форкает curl при спавне бота, скачивает ESI-портрет сразу в imageDir/Character/{serverID}_512.jpg (не ждёт cron). Cron каждые 15 мин остался как бэкап (`/tmp/bot_portraits.log`, пишет в /opt/evemu/image_cache → docker cp в volume, т.к. контейнер читает /app/image_cache = volume evemu_image_cache).
- **Языки чата** (`facbf169`): бот отвечает на языке игрока (промпт DeepSeek: «match the language»), не English-only.
- **Эффект лазеров у игрока — ЗАКРЫТО (`70c55303`, 21 авг)**: у игрока не было эффекта при стрельбе из лазерных туретов (у ботов был). Причина: боты шлют guid жёстко (`effects.Laser` в NPCAI), а игрок — через `FxDataMgr::GetEffectGuid(m_effectID)`, который возвращал `""` при отсутствии guid в серверном SDE → `ShowEffect` пропускал OnSpecialFX (guard `!guidStr.empty()`). Фикс: fallback в GetEffectGuid — `targetAttack(10)→effects.Laser`, `projectileFired(34)→effects.ProjectileFired`, `projectileFiredForEntities(1086)`. Плюс EFFECTS__TRACE-лог маппинга effectID→guid в ShowEffect.
- ВАЖНО: `StaticDataMgr::GetType` возвращает **void** (не bool!) — нельзя `if (GetType(...))`, только вызов + проверка `tdata.id`.

## 12 августа (поздний вечер): чистая сборка, портреты, био без абракадабры
- **Чистка БД** (финальная, перед сборкой): server остановлен (`sudo docker stop server`), выполнена `cleanup4.sql` — botChars=0, botMemory=0, chrCharacters=только Mr Tort (90000000, accountID=1 — НЕ трогать), crpCorporation=179 штатных NPC. RS Corp (98000000)/RS Corp Alliance (99000000) — штатные seed-объекты. Таблицы `botPortraits` в тот момент НЕ было — скрипт чистки должен не падать на ней (удалена строка).
- **Сервер пересобран и запущен юзером**; боты спавнятся заново.
- **Портреты**: `fetch_bot_portraits.py` прогнан (5 ботов получили лица: server 97230261-97230265 ← eveID из killmail-легенд). Таблицу `botPortraits` скрипт создаёт сам. Настроен **cron каждые 15 мин**: `*/15 * * * * /opt/conda/bin/python /opt/evemu/tools/fetch_bot_portraits.py --db-host 127.0.0.1 --db-user evemu --db-pass evemu --db-name evemu --image-dir /opt/evemu/image_cache >> /tmp/bot_portraits.log 2>&1`. Лог: `/tmp/bot_portraits.log`.
- **Био без абракадабры** (`fb07cac4`): убрана бессвязная конкатенация старт+середина+конец. Три осмысленных режима: цельная фраза (мем/zkillboard), короткая история (событие+логичное следствие), совет бывалого; редко пустое (2%). Уникальность по БД сохранена.
- **Регрессии закрыты** (в master): чат local (`e3ca1aec` senderType=1377 + cacheOwners, LSCChannel.cpp:215), имена кораблей (`e08a9128` MakeRandomShipName, BotMgr.cpp:589), NPC-спам скорости (`8f20e6b5` maxVelocity fallback, NPC.cpp:72/NPCAI.cpp:98).
- ВАЖНО про `sudo`: кэш креденшелов протухает; рабочий паттерн `echo gbnjy78 | sudo -S -p '' -v && sudo docker ...` (иногда «Sorry, try again» при пароле из stdin — повторять).

## 11 августа: ИИ-игроки (боты) — полный цикл (реализовано, сборка/чистка БД)
Цель: имитация живого сервера. Боты = полноценные персонажи в SQL (прогресс не пропадает), с легендой из реальных killmails, DeepSeek-чатом, профессиями и самообучением везде.
- **Прилёт через гейт** (`0692e31c`): боты НЕ спавнятся у игрока — `PopulateSystem` создаёт их в случайных соседних системах, `SetTravelDestination(система игрока)` + `MarkForTravel` → видимый варп к гейту (12-20с) → переход. Per-system фикс. target 60-100% капа (не ровно 30).
- **Портрет** (`6c1f3ed7`): `CreateBotCharacter` вставляет `chrPortraitData` (случайный фон/поза).
- **Корень «нет аватаров»** (`cc468334`): киллмейл-боты (charID!=0) РАНЬШЕ пропускали CreateBotCharacter → не было chrCharacters/портрета/навыков. Теперь SpawnBot всегда вызывает CreateBotCharacter с forceCharID (использует только если в серверном диапазоне 90000000..97999999; killmail ID вне его → новый); дедуп по имени.
- **Фиксы сборки/рантайма** (`e1be7c8e`): миграции ботов получили `-- +migrate Up/Down` (EVEDBTool/sql-migrate падал без них); skill-history INSERT и skillPoints UPDATE переведены на DBerror-вариант RunQuery (был «did not return a result» спам); SpawnBot валидирует killmail-хл (совр. корабли 33816 и т.п. отсутствуют в Crucible invTypes → fallback на T1 крейсер/БК).
- **Каркас** (`830e9d89`): `PlayerBot` (наследник NPC, корабль/Destiny/NPCAI) + `BotMgr` (синглтон, 1Hz тик, конфиг `<playerBots>`).
- **Персистентность** (`e83a5f3e`): `CharacterDB::CreateBotCharacter` — бот пишется как игрок: chrCharacters + навыки + chrSkillHistory + chrEmployment + баланс.
- **Киллмейл-легенды** (`72282a6f` + `147fac66`): `botKillmailLegends` (7438 записей импортировано с 10 регионов) + `tools/import_killmail_legends.py`. Запуск по регионам: `/opt/conda/bin/python tools/import_killmail_legends.py --db-host 127.0.0.1 --db-user evemu --db-pass evemu --db-name evemu --sleep 2 --regions 10000002,...`. beforeKillID даёт 403 — только регионы.
- **DeepSeek-чат** (`22bbd242`, `08025998`): боты в локале (AddBotChar/SendBotMessage fake sender), отвечают через `BotChat` (curl CLI), throttle 1/30с. Самообучение чата: chatLine при ответе, chatReply если игрок пишет в 60с.
- **Интеллектуальный бой** (`308a5b95`, `ae20ada0`): kill rights, оценка сил (класс корабля×2 + skillTier + память ±2), бегство, флот-поддержка, kill call, роли (fighter/logistics/support/commander), EWAR+логисты+бонусы.
- **Профессии** (`bd9ef234`, `a2801351`): 10% PvP-хантеры, 15% раттеры (красные крестики), 35% майнеры, 20% трейдеры, 15% курьеры, 5% хакеры. Кооперативный майнинг (охрана из корпа).
- **Скирминг нулей** (`7434658e`): PvP-корпы только в альянсах (PickCorp requireAlliance), ClaimSystem() захватывает бесхозные нули.
- **Самообучение** (`94dfbc52`): botMemory (wins/losses/kills/deaths/chatLines/chatReplies/ratKills/mineRuns/tradeRuns/hackRuns). Бой/чат/деятельность — персистентно.
- Чистка БД сделана: botMemory=0, ботовых сущностей нет, chrCharacters=только Mr Tort. botKillmailLegends=7438 сохранены.
- ВАЖНО: `utils/config` в .gitignore — секцию `<playerBots>` в `/opt/evemu/config/eve-server.xml` добавлять вручную (уже добавлена, DeepSeekKey=sk-placeholder — заменить).
- `git pull` на сервере: root-владение `.git/objects` → `sudo chown -R dmitry:dmitry /opt/evemu/.git`. Локальные правки `tools/import_killmail_legends.py` на сервере → `git checkout -- tools/import_killmail_legends.py` перед pull.
- **DeepSeek-чат** (`22bbd242`): боты в локале (AddBotChar/SendBotMessage — fake sender), отвечают игрокам через `BotChat` (curl CLI, no HTTP lib), конфиг `ChatEnabled/DeepSeekKey/DeepSeekURL`. Throttle 1/30с на канал.
- **Перелёт** (`a43f02e1`): бот видимо варпит к гейту (12-20с), потом переносится в соседнюю систему (mapSolarSystemJumps) — появляется у её гейта. **Док/андок** (`e75bc7e7`): боты на станции (в локале, без SE), андок → варп → гейт.
- **Интеллектуальный бой** (`308a5b95`, `ae20ada0`): kill rights (хайсек только криминалы/низкий sec; лоусек/нули свободно), оценка сил (класс корабля×2 + skillTier, AggroFactor), бегство при слабости, флот-поддержка (союзники того же корпа/альянса), kill call (PickPriorityTarget: командиры/логисты первыми), роли (60% fighter, логисты ремоут-репят, командиры дают бонус, саппорт — EWAR web/scram/ECM/paint через NPCAI::AttackTarget).
- **Профессии** (`bd9ef234`, `a2801351`): 10% PvP-хантеры, 15% мирные раттеры (ТОЛЬКО красные крестики, `RatForTarget`), 35% майнеры, 20% трейдеры, 15% курьеры, 5% хакеры. Кооперативный майнинг (`RequestFleetProtection` — охрана из корпа).
- **Скирминг нулей** (`7434658e`): PvP-корпы помещаются только в альянсы (PickCorp requireAlliance), `ClaimSystem()` захватывает бесхозные нули через `svDataMgr.AddSovClaim` (нужен альянс + практика).
- **Самообучение** (`94dfbc52`, `08025998`): `botMemory` таблица (wins/losses/kills/deaths/chatLines/chatReplies/ratKills/mineRuns/tradeRuns/hackRuns). Бой: победы→агрессивнее (±2 силы), смерти→осторожнее. Чат: линии+ответы → качество. Деятельность: практика→частота (30-70%). Персистентно.
- **Фиксы сборки**: `1c563faa`, `bb59ce3c`, `427e87a6`, `cc4a1212`, `d6bed9bc` (includes, invGroups имена, Lookup<LSCService>, GetMaxShipSpeed, syntax).
- ВАЖНО: `utils/config` в .gitignore — секцию `<playerBots>` в `/opt/evemu/config/eve-server.xml` добавлять вручную (Enabled/MaxPerSystem/ChatChance/AggroFactor/MinSkillLevel/MaxSkillLevel/ChatEnabled/DeepSeekKey/DeepSeekURL).
- `git pull` на сервере падал из-за root-владения `.git/objects` — лечится `sudo chown -R dmitry:dmitry /opt/evemu/.git`.

## 10 августа (день): фиксы орбиты/дронов/декора + подготовка к сборке
- **Орбита вокруг NPC раскручивалась** (`d52f47a2`): TooFar-ветка Orbit() имела ранний return при `m_orbiting==TooFar` — heading вычислялся один раз, корабль летел к устаревшей точке, дистанция росла 5→30км («орбитил NPC», дроны за кораблём = «телепорт в другой конец экрана»). Фикс: TooFar пересчитывает heading каждый тик.
- **Телепорт дрона при смене chase→orbit** (`3103b69a`): SetEngaged не слал SetPosition перед CmdOrbit (в отличие от SetApproaching/IdleOrbit) — клиентский Ballpark пересоздавал шар на новой орбите. Добавлен sync.
- **Дрон улетал за NPC** (`f9945956`): leash был 2x control range (40км); NPC с большим flyRange (Eradicator 27км) утаскивали дронов. Теперь 1x control range.
- **Орбита вокруг невидимой декорации** (`cc1cebb3`): IsTargetInvalid останавливает ORBIT/FOLLOW если у цели нет DestinyMgr (декорации CelestialSE) — фикс варпа «You are already warping».
- **import_prices.py**: порт 3306 контейнера db проброшен на хост (`-p 3306:3306`, данные в volume evemu_db). Запуск: `/opt/conda/bin/python tools/import_prices.py --db-host 127.0.0.1 --db-user evemu --db-pass evemu --db-name evemu`.
- **Контейнеры остановлены** для сборки юзером. Сеть evemu_default осталась (db не удалялся).


## 10 августа: подтверждено юзером — убитый дрон пропадает навсегда, врек остаётся (`2ebb22df` ClearController при DroneSE::Killed).
## Следующая проверка (cc1cebb3): варп после орбиты вокруг невидимой декорации.

## 9 августа (поздний вечер): SEGV при убийстве NPC — DOUBLE-REMOVE найден и исправлен
- **Симптом**: сервер упал в Segfault в 08:42:45 при бое в аномалии. Перед крашем `AnomalyMgr::RemoveSignal() - removing 750000078` **дважды** за 08:42:10 (двойное удаление NPC) + корабль юзера `140001543` удалён (варп) + `RemoveSignal(1000000288)` (ракета). Краш через 35с после двойного удаления — классический delayed heap-corruption.
- **Причина** (`5d086488`): `Damage::ApplyDamage` (Damage.cpp:373-374) вызывает `Killed(d)` (виртуальный → `NPC::Killed`) а затем `SystemEntity::Killed(d)`. Но `NPC::Killed` (конец метода) сам вызывал `m_system->RemoveNPC(this)`, который делает `RemoveEntity` + `pNPC->RemoveNPC()` (`m_self->Delete()`). Потом `SystemEntity::Killed` → `Delete()` снова `RemoveEntity` + `m_self->Delete()` → двойной RemoveEntity + двойной item-delete → heap corruption.
- **Фикс**: `NPC::Killed` теперь вызывает только `m_system->RemoveNPCFromList(this)` (новый метод: m_npcs.erase + sEntityList.RemoveNPC(), БЕЗ RemoveEntity/item-delete). Полное удаление (RemoveEntity + m_self->Delete) делает `SystemEntity::Killed`→`Delete()` сразу после.
- Также `/kill` и `/killallnpcs` (SystemCommands.cpp) вызывают `NPC::Killed` напрямую (не через Damage) — они теперь цепляют `npcEntity->SystemEntity::Killed(damage)` для полного удаления; `/kill` для NPC больше не делает отдельный `RemoveEntity` (он внутри SystemEntity::Killed).
- Комментарий в Damage.cpp:373 «Killed must NOT remove dead SE» — контракт восстановлен.

## Сессия 9 августа: декор видимость, призраки дронов, орбита/отталкивание
- **Декор невидим — ИСТИННАЯ причина** (`3a8490bd`): CelestialSE наследует `IsStaticEntity=true` → декор/контейнеры/облака попадали в static-map bubble (`m_entities`), а `GetEntities()`/`SendAddBalls()` шлют только `m_dynamicEntities`. `AddBallExclusive` при спавне не помогал (игрок заходит позже). Фикс: `CelestialSE::IsStaticEntity=false` — декор теперь dynamic и доходит до клиента. Гейты/планеты/луны — отдельные StaticSystemEntity классы, не затронуты.
- **Отталкивание корабля/NPC/дронов** (`0ca4a008`): collision check в `DestinyManager::ProcessState` выталкивал движущихся из КАЖДОЙ static-сущности с радиусом ≥500м — декорации (контейнеры 1174м) и врата ускорения (2341м) толкали всех в стороны каждый тик. Фикс: выталкивание только из IsGateSE/IsStationSE/IsPlanetSE/IsMoonSE.
- **Орбита раскручивалась** (`0ca4a008`): `mPos.y = radius * phi`, где phi уже радианы (~0.785) → вертикальный разлёт ±0.785×radius. Фикс: орбита в горизонтальной плоскости + малая Y-качка (0.05).
- **Призраки дронов в БД** (`2a8b3921` re-applied `7ec8e576`): `InventoryItem::Delete` для дронов (cat 18) удалял предмет полностью вместо junkyard-Move. Подтверждено: убитые дроны исчезают из entity. Клиентские «призраки» уходят по TerminalExplosion/RemoveBall.
- **Декор видимый** (`42d8e954` re-applied `c8e04d05`): AddBallExclusive после спавна (дополнительно к CelestialSE dynamic).

## Визуальный стиль аномалий по фракциям (лор юзера, реализовано в DungeonMgr.cpp `SpawnDecorations`)
- **Серпентис** (`f6744912` + облака `37970c68`): цвета тёмно-зелёный+чёрный («гедонистический декаданс»); атмосфера — **коричневые или тёмно-синие газовые облака** + крупные астероидные группы, мрачная индустриальность. Базы — НЕ хаотичное логово, а организованные объекты: КПП, верфи, штаб-квартиры (корпоративная природа).
- **Кровавые Рейдеры** (облака `37970c68`): фирменный фон — **красная туманность** (огонь/плазма/искры, алые облака). Критика в лоре: «нелогично»/«чёрный экран при входе» — технический переход стандартного фона системы в фон аномалии (мы облаками имитируем, не фоном).
- **Картель Ангелов** (облака `37970c68`): крупнейшая организованная фракция, власть от древних технологий **Йов**. Корабли — **коричневый камуфляж**, дизайн как **жук-рогач** (йовская «ракообразная» эстетика). Базы — продуманные иерархические комплексы (подразделения: Доминаторы, Архангелы, Страждущие, Спасительные Ангелы).
- **Гуристы**: хлорно-зелёные/кислотные облака (токсичные зелёные).
- **Санша**: ионные/плазменные голубовато-белые микро-туманности.
- **Роуг-дроны**: серые обломки/метеоры/пыль.
- Все палитры: `angelClouds`/`guristasClouds`/`bloodClouds`/`sanshaClouds`/`serpentisClouds`/`rogueClouds` в DungeonMgr.cpp; default — общий `cloudDeco`. Юзер обещал ещё описания для Гуристов/Санши/Роуг-дронов — если даст, подогнать палитры точнее.

## Осталось (TODO)
- ЗАКРЫТО: декор/ворота (17 авг), скорость/аппроач/ВХ/анимация прыжка/экспедиции (18 авг), сканер (откат `1d1357cd`), маркет sell-ордер (16 авг `ee9613be`), эффект лазеров у игрока (`70c55303`), incursion ISK (`f502d7ce`+миграции), повтор фраз ботов (`c9030ea3`+`3998b45a`), null-защита GetShipState (`6edff173`), био стабильное (`7e8869a7`+`7838e1b8`), инкурсийные Sansha невидимы (`dc354446`), скан в инкурсии (`1b9bec69`), кнопка скана (`ed62029d`), камера на челоботов (`4debf188`), невидимые NPC после варпа (`e3c7c0f0`).
- ЗАКРЫТО (25 авг `1547a65a`): маркет «нет в наличии» — uint8 order-limit поля оборачивали 20000 в 32 (GetStationAsks возвращал 32 типа вместо 10390).
- ЗАКРЫТО (25 авг вечер): невидимые Rogue Drone (`4a06b77d` — factionID 500022), дроны летят за целями через систему (`d492ef9f` — drop target вне баббла).
- ЗАКРЫТО (25 авг ночь): тубы файтеров 20 у Nyx (`df0a1207`+`57940a22` — было 35/26), агрессия PvP/бот↔игрок (`dcaea8a8` — OnAggression был мёртв), сервер вис от логов (`f5c95912` — ERROR-only log.ini), боты застревали при варпе вне баббла (`dda2b6de` — fallback re-add в бабл убран для NPC).
- ЗАКРЫТО (26 авг): краш/вис при атаке ТКУ (`1cc8cf04` — корп-мультикаст печатал Error+traceStack на каждое изменение атрибута), урон челоботов с 574 км (`24b48fb0` — ApplyDamage гейтится m_maxAttackRange), таргет челоботов с диких расстояний (`1be77a41` — m_sightRange = реальный AttrMaxTargetRange ×1.25, лок дропается при выходе за дальность), режимы дронов Focus Fire/Aggressive/Attack-Follow (`539cfdbc`+`adde225e`).
- ЗАКРЫТО (27 авг): орбита вокруг гейтов/станций от поверхности (`a12d4095`), турели стационарные + тип атаки по роли (`0c690a5c`+`84ef3e01`), файтер-бомберы всегда попадают (`74221cb1`), аналитическая оценка силы челоботов (`7673c9e6`+`5ce0cb67`), минимальный анализ чата (`be6c8571`+`734bcb90`), самооборона PvP и vs ботов — агрессия только на инициатора (`08b54e29`+`f9aefd79`+`ff78ea34`), MWD после анчоринга (проверено юзером).
- **Нерешённое (21 авг вечер)**: пересборка на `450892ca` и проверка — невидимые NPC в инкурсии после e3c7c0f0 (все ли видны); «варп завис при варпе на планету» — гейт 50004103 в ~2700км от центра планеты 40148131 (радиус 2980км), юзер мог быть внутри планеты; новый Mr Tort (97233346) — лаги/ботавское поведение после фикса идентификации; правый клик по луту слиперов в трюме корабля (клиентская `menusvc._InvItemMenu`); FXError fxID=0 Rocket Launcher II; DB Error #1366 notificationText; NPC-ремонты с атрибутами (Plunderer).
- **Проверить после пересборки на `dda2b6de`**: Rogue Drone видны/красные у бельтов и бьют с лучами/цифрами урона; бой в одном баббле (дроны не разбегаются за юзером); поведение ботов в системах без станции и в аномалиях; клиентский десинхрон варпа (разовое, «пока оставим»); 20 бомберов у Nyx (не 26/35); агрессия при PvP и атаке челоботом (флаги с обеих сторон); боты не застревают при уходе; сервер не виснет от логов.
- **Проверить после РЕСТАРТА на `1be77a41`**: дроны в режимах Focus Fire/Aggressive/Attack-Follow бьют самостоятельно; ТКУ атакуется без виса; челобот-Мегатрон не дамажит с 574 км и теряет лок за ~90 км (затаргет в пределах дальности таргета, урон ~35 км).
- **Проверить после рестарта (26 авг утро/день)**: ECM игрока джамит (сбивает лок); амбуши хантеров ставят варп-бабл; self-preservation (грузовики не дерутся, новички не суицидятся); джамп-драйвы требуют активное цино и жгут изотопы — **ПРОВЕРЕНО юзером (27 авг): прыжок работает, расход был занижен ~16x, фикс `1313d4cb` (double), бридж не тестирован**; клоны (clone bay install + CloneJump док на станции); FW system flip (плексы → переворот системы); SBU разворачивается у гейта — **ПРОВЕРЕНО юзером (27 авг): работает нормально**.
- **Защита данных — РЕШЕНО (26 авг)**: binlog включён (db-контейнер пересоздан с `--log-bin=mysql-bin --binlog-format=ROW`, `log_bin=ON`); скрипт `/opt/evemu/backup_db.sh` (mariadb-dump → gzip в `/opt/evemu/backups/`, хранит 7, cron `0 4 * * *`); первый дамп `evemu_20260826_2126.sql.gz` (342 МБ). Первый дамп до пересборки сервера: `/opt/evemu/backup_evemu_20260826.sql` (5 ГБ, сырой). binlog-файлы: `/var/lib/mysql/mysql-bin.00000N` (в volume evemu_db).
- Крупные системы (не реализованы): Memory Mgmt/RefPtr→shared_ptr (~400 файлов, полный перевод НЕ реалистичен — аудит 27 авг: 4281 new Py*, 6443 raw Py*, ~6000 мест; вместо него Этап 0: укрепление RefObject `b4f424b9` + ASAN-аудит). PyRep leak-фиксы — **НАЧАТО (28 авг, точечный аудит)**: найдены и закрыты утечки в `CreateNotification` (`d755f32f`), `CorpNotify`/`Broadcast`/`Multicast` (`deebe9cf`) — утекали PyRep-контейнеры и их items при DecRef без clear(). **Системная проблема**: деструкторы PyTuple/PyList/PyDict НЕ освобождают items (закомментировано с upstream), контейнеры чистятся только через явный clear(); системный фикс деструкторов ОТКЛОНЁН — `PyPacket::Encode` присваивает items без IncRef (двойное владение с PyPacket dtor) → double-free. Продолжать точечно (clear() перед DecRef) или рефакторить модель владения PyRep. Джамп-драйвы/бриджи — РЕШЕНО `6dd25d3d` (валидация активного цино + миграция топлива по расам). Клоны — РЕШЕНО `015f5736`+`ffc27824` (AcceptShipCloneInstallation сломанный SQL + CloneJump не двигал игрока). PI — РЕАЛИЗОВАНО (Colony/PlanetMgr/PlanetDB, PIEnabled=true), нужен e2e-тест. FW Plex capture — РЕАЛИЗОВАНО `b62e1310` (плексы спавнятся, таймеры захвата с лимитами по кораблю, contested-пауза, NPC-защитники, LP, переворот системы по occupierID).
- **НЕ ДИАГНОСТИРОВАН клиент-кик при атаке ТКУ файтерами**: юзер подозревал, что вылеты клиента начинаются когда он файтерами бьёт ТКУ. Диагностика (DAMAGE__MESSAGE/SE__SLIMITEM/NPC__AI_TRACE/CLIENT__SESSION/COLLECT__DESTINY/DESTINY__UPDATES) была включена, потом отключена до повтора проблемы. Если повторится — включить те же 6 каналов, воспроизвести, снять лог.
- Диагностика log.ini выключена (ERROR-only, 70 каналов).

## Главное достижение (3 августа): AP gate-jump ПОЧИНЕН
Автопилот теперь проходит много-гейтовый маршрут без зависаний и рывков:
- корабль подлетает к гейту, прыгает, сразу продолжает (варп к следующему гейту), прыгает дальше, докидывает до станции.

### Три бага, которые были найдены и исправлены
1. **AP-стагнация после прыжка** (~2 мин молчания, потом сам очнулся; док/андок «лечил»). Причина: клиентский `starmap.UpdateRoute(fakeUpdate=True)` в `OnSessionChanged` мог отработать слишком рано во время перехода (stale маршрут → `destID None` → AP молча ждёт). **Фикс (`960d6715`)**: сервер через 5 сек после прыжка шлёт повторную «доброкачественную» смену сессии (`nextSessionChange`) → клиент заново вызывает `OnSessionChanged → UpdateRoute`, маршрут продвигается, AP продолжает. Код: `Client.cpp` `StargateJump` (m_apSessionRetry=true + m_stateTimer.Start(5000)) и `ProcessClient` Idle-кейс.
2. **Первый варп после прыжка срывался Halt()**. Причина: после прыжка корабль в битом состоянии (`USF=0, m_stop=true, но TF/ASF=1.0` — наследие от pre-jump follow/warp), MoveObject при `USF==0` вызывал `Halt()` → варп отменялся, потом re-warp. **Фикс (`cc5ffaa2`)**: в выравнивании WARP-режима `else if (m_userSpeedFraction < 0.7499) SetSpeedFraction(1.0f, true)` — убрано условие `m_timeFraction < 0.749` (теперь перевооружает корабль независимо от TF).
3. **Посадка AP-варпа на больших гейтах** (была вплотную ~950м от поверхности). Клиентский WarpLoop всегда сажает warp-to-gate ~15км от **ЦЕНТРА** гейта независимо от stop-distance → на большом гейте (радиус 14км) корабль на гейте; увеличение stop-distance → пролёт сквозь гейт + рывок назад. **Фикс (`29ecf6ce`)**: AP варпит к **ТОЧКЕ** `radius + apWarptoDistance` от центра гейта (warp-to-point, сажается точно). Итог: ~11км от поверхности малого гейта, ~14км от большого, без рывков. Код: `BeyonceService.cpp` `CmdWarpToStuffAutopilot` (GPoint landPoint = gatePos - toGate * (radius + apWarptoDistance); WarpTo(landPoint, 0)).

## Карта для отладки (системы/гейты)
- 30000197 = Uemon, 30000198 = Paala, 30002355 = LXQ2-T, 30000196 = Otosela.
- Гейты: 50011022 «Stargate (Paala)» в Uemon→Paala (radius ~19km); 50011530 «Stargate (Uemon)» в Paala→Uemon (radius 3532); 50014211 «Stargate (LXQ2-T)» в Paala→LXQ2-T (radius 14051); 50014212 «Stargate (Paala)» в LXQ2-T→Paala.
- Rattlesnake warpSpeedMultiplier=3 (3 AU/s).

## Клиент (важно)
- Реальный клиент: **`C:\Program Files (x86)\CCP\EVE`** (НЕ `C:\EVE`). Crucible 2012, `start.ini` server=router.iks-online.net:26000.
- Декомпил: `C:\opencode-projects\other\all\` (autopilot.py, starMapSvc_py.py, sessions_py.py, pathfinder_py.py).
- Dev-консоль клиента (`~`) НЕ работает — инспектировать клиент нельзя.
- Клиентские log-каналы (`svc.autoPilot` в log.ini/start.ini [log]) НЕ пишут в gamelog — попытки не сработали.

## Сервер: docker-compose сломан
`docker-compose up` падает `KeyError: ContainerConfig` (compose 1.29.2 vs docker 29.1.3). Контейнеры вручную:
```bash
docker stop server db && docker rm server db
docker run -d -t -i --name server --network evemu_default -v /opt/evemu/config:/app/etc -v evemu_server_cache:/app/server_cache -v evemu_image_cache:/app/image_cache -v evemu_ccache:/ccache -p 26000:26000 -p 26001:26001 -e SEED_MARKET=TRUE -e SEED_SATURATION=75 -e 'SEED_REGIONS=...' -e RUN_WITH_GDB=FALSE evemu_server
docker run -d --name db --network evemu_default -v evemu_db:/var/lib/mysql -e MARIADB_RANDOM_ROOT_PASSWORD=true -e MARIADB_USER=evemu -e MARIADB_PASSWORD=evemu -e MARIADB_DATABASE=evemu mariadb:11.8 --innodb-buffer-pool-size=1G --innodb-log-file-size=256M --innodb-flush-log-at-trx-commit=2 --max-allowed-packet=64M --bulk-insert-buffer-size=64M
```
**ОБЯЗАТЕЛЬНО `-t -i`** для server (иначе спам `Command not recognized:`). Включён `DESTINY__WARP_TRACE=1` и `AUTOPILOT__MESSAGE=1` в /opt/evemu/config/log.ini.

## Осталось
1. **Дроны** — 4 фикса задеплоены (5 авг): scoop→бэй корабля (`m_shipRef->itemID()`, было `GetLocationID()`=система → дроны «пропадали»), мягкий sync позиций откачен (рывки орбиты), возврат дрона за 2x контроль-дистанции в бою (цель сбежала/варпнула), сообщения запуска (`MaxBandwidthExceeded2` правильное имя+аргументы droneName/bandwidthNeeded/droneBandwidthUsed, LaunchDrone→enum различает лимит/банду). Отдельные баги дронов (призрак в БД, телепорт при смене режима, килл-эхо) — в секции «Дроны — диагностика» ниже, закрыты.
2. **Bracket crash** — `'NoneType' object has no attribute 'lower'` при наведении.
3. **Мобилка после анчоринга не скрамблит** — ЗАКРЫТО `d8ace19c` (25 авг): MWD сразу online после анчора (bubble + scramble активны), стоп-стак деплоябла `eb08f76c` (Split(1)). **Проверено юзером (27 авг): работает.**
4. **NPC AI** — ОСНОВНОЕ СДЕЛАНО (5 авг): NPC больше не «вклозе», орбитят по дальности, атакуют, скрамлят. Юзер подтвердил: «атака идет, скрамблят, орбитят». См. секцию «NPC AI (5 августа)» ниже. TODO: проверить ремонты на NPC с атрибутами (Plunderer: щит+25/5000мс); мелочи.
- ВАЖНО про диск: повторные `docker build` копят старые слои образов → диск 100% (237G/249G) → сервер падает на старте в `CachedObjectMgr::SaveCachedToFile` (fwrite assert). Лечится `docker image prune -f` (освободил 193GB). Периодически чистить.

## NPC AI (5 августа) — «не вклозе, а орбита по оптималам»
- **Диагностика**: NPC в бою орбитили вплотную (Plunderer dist 500-600м, Mortifier ~950м). Причины: (1) `AttrMaxRange` у части NPC = 0/223м (Dire Guristas Murderer/Plunderer, Guristas Plunderer) → команда орбиты = точка-в-точку; (2) `Attack()` вызывается только из `CheckDistance`, а `m_mainAttackTimer` запускался ТОЛЬКО в `Target()` — если NPC агрил через `Targeted()` (игрок/дрон атакует первым), таймер не стартовал → NPC орбитил молча, вообще не стрелял; (3) `AttrSpeed` (цикл оружия в ms) у Guristas Plunderer/Mortifier = **30000 (30 сек!)** → казалось что NPC мёртвые.
- **Фиксы**:
  - `672afd73`: минимальная дистанция орбиты — `if (m_optimalRange < 1000) m_optimalRange = max(1500, min(5000, m_maxAttackRange/2))` в конструкторе NPCAI.cpp.
  - `04ee6348`: в `Targeted()` добавлен запуск `m_mainAttackTimer.Start(m_attackSpeed)` + `m_missileTimer` (раньше только в `Target()`); кламп `m_attackSpeed` — `if (m_attackSpeed < 500 or > 15000) m_attackSpeed = MakeRandomInt(3000, 8000)`.
- **Данные БД (атрибуты NPC, dgmTypeAttributes)**: `speed`(51)=цикл оружия ms (норма 2500-3500; битые 30000), `maxRange`(54)=оптимальная дистанция (у многих 0), `entityAttackRange`(247)=дальность атаки, `entityFlyRange`(416)=радиус орбиты, `entityAttackDelayMin/Max`(475/476)=задержка первого выстрела (НЕ используется кодом — при желании подключить), `entityChaseMaxDistance`(665)=boostRange, `entityShieldBoostAmount/Duration`(637/636)=ремонт. Guristas Arrogator=2382 (speed 2500, maxRange 500, dmg 0.625), Plunderer=2386 (speed 30000, dmg 0), Dire Mortifier=23307 (speed 30000), Dire Plunderer=23332 (speed 2750, maxRange 11250, dmg 3).
- **Результат юзера (тест 14:15 UTC)**: «Идет бой», «атака идет, скрамблят, орбитят». Орбиты: Sunder/Decimator Drone (мили) ~1.4-1.75км, Atomizer Drone (дальний) 10-15км — по своим оптималам.
- **Запросы к БД**: клиент mariadb в db-контейнере = `/usr/bin/mariadb` (НЕ mysql, нет в PATH). sudo-скрипт: `echo gbnjy78 | sudo -S -p '' -v` (кэш креденшелов) потом `sudo docker exec -i db /usr/bin/mariadb -uevemu -pevemu evemu < file.sql`. PowerShell манглит кавычки в plink — писать SQL в файл и передавать base64 (сообщение `base64: invalid input` — НЕ ошибка). entity НЕ имеет solarSystemID — позиции по `locationID=<system>` + x/y/z (устаревшие для движущихся объектов).

## Дроны — диагностика (4 августа)
- **«1 пропадает при запуске»**: лимит дронов = 5 (AttrMaxActiveDrones, DCU нет). Клиент шлёт 6 LaunchDrone → 6-й отклоняется лимитом (SE не создаётся) → «1 потерялся». Это корректное поведение лимита, но клиентский drone-window не синхронизируется с отказом. Задеплоен фикс `f86e7191`: `ShipSE::GetDroneLimit()` (char AttrMaxActiveDrones + online DCU-бонусы) — раньше Drop и LaunchDrone считали лимит по-разному (LaunchDrone не учитывал DCU), из-за чего последний дрон партии отклонялся.
- **«Дроны улетают в дальний космос при scoop / не забрать»**: первый клик scoop РАБОТАЕТ (итем уходит в дрон-бэй, SE удаляются, flight-лист пуст — подтверждено DIAG: itemFlag=87, flightCount=0). Второй клик не находит SE (`Unable to find droneSE`) — дроны уже в бэе. Проблема в клиенте: drone-ball'ы не удаляются/не останавливаются корректно → визуально «улетают в космос», юзер думает что дроны в космосе и жмёт scoop снова. Это клиентский FOLLOW/ORBIT десинхрон. TODO: разобраться с удалением шара дрона при scoop и/или с возвратом дрона.
- **NEW: дроны улетают сами (idle) — НАЙДЕНА ПРИЧИНА (5 авг)**:
  - Запуск → IdleOrbit (ORBIT, usf=0.6) → орбит работает. Через ~6-11 сек клиент шлёт `CmdReturnHome` («Return and Orbit» — это НОРМАЛЬНОЕ поведение клиента при отправке орбиты дронам) → `EntityBound::CmdReturnHome` → `DroneAIMgr::Return()` → `Follow(ship, 0)` + Departing.
  - Дрон в FOLLOW стартует со СТАРОГО тангенса орбиты (не успевает довернуть: `m_degPerTic=(60-agility)/10` давал ~6°/тик даже для agility 0.005) → летит по прямой от корабля. Ускорение FOLLOW (`newSpeed=usf+0.15`, cap 0.8) → 1824 м/с → 210-600 км.
  - **Фиксы (9a59bc49, деплой проверен юзером — «дроны и нпс ожили, орбитят»)**:
    - `Follow()` мгновенно пере-прицеливает heading дрона на цель (нет дрейфа по тангенсу).
    - `m_degPerTic = 60/(agility+1)` вместо `(60-agility)/10` — поворот реально работает.
    - drone chase-speed floor 2000 → 100.
  - maxVelocity Infiltrator II в БД = 2280 (в совр. SDE typeDogma = 2760) — НЕ завышена, дроны реально быстрые.
- **Крэш при отзыве дронов** (SEGV, фикс `60ed19f4`): ScoopDrone → Offline → `AssignShip(nullptr)`, а Departing-хендлер разыменовывал `m_assignedShip->GetPosition()` без null-проверки → NULL deref. Фикс: guard в начале Departing-кейса.
- **Ещё крэш (SIGABRT refcount underflow)** — не воспроизведён повторно; анализ через GDB-режим (`start.sh` теперь `gdb -batch -ex run -ex "bt 40"` при RUN_WITH_GDB=TRUE).
- **Мягкий sync позиций** (`fedd39cb`, деплой ожидает проверки): каждые ~5с шлётся `SetBallPosition` для FOLLOW/ORBIT/GOTO шаров (дрон/NPC/корабль) — клиентский Ballpark плавно lerp'ит, дрейф ограничен метрами. Включён DESTINY__TRACE для логирования sync.
- **Варп: телепорт+стоп в конце** — серверное торможение фикс. 21с, клиентское другое (масса/agility). Сервер заканчивает варп раньше клиента → WarpStop шлёт CmdStop+SetPosition(цель) пока клиент ещё тормозит → рывок. Константы декеля клиента из destiny.dll (OnDeactivatingWarp) извлечены: DAT_10063f20=0.1, DAT_1005f948=1.496e11 (1 AU), DAT_10064028=1.5, DAT_10063fe0=3, DAT_10064018=0.3333, DAT_10064020=-3, DAT_10064088=1.496e12, DAT_10064098=0.7, DAT_100640a0=0.01, DAT_1005f718=1, DAT_1005f710=0.5 (accel tanh), DAT_1005f720=2, DAT_10063f10=1e-5 (мин. скорость).
  - **Сделано (68c3ef81, деплой 10:17 UTC)**: decelDist = `min(mass*0.1*AU, warpSpeed*1.5)/3` (проверено по логу: для 24AU long-warp decelDist=2.24e11 совпал), время = ln(decelDist); холд 5с у выхода (клиентский декел ~10% дольше, gap растёт с дальностью: 334м на 0.5AU, 6км на 24AU); WarpStop НЕ шлёт SetBallPosition (клиент уже на цели, снап = стоп-рывок).
  - **Осталось**: микро-рывок самого стопа (клиентский WARP→STOP переход обнуляет остаточную скорость резко). Тайминги сошлись (торможение успевает), но плавной остановки нет — вероятно, врождённый артефакт клиентского Ballpark без точного совпадения формул (accel клиента — tanh-кривая `(tanh+1)*0.5*warpSpeed`, у сервера exp-accel). Если юзер настаивает — нужно реализовать клиентский tanh-accel + двухфазный декел (OnDeactivatingWarp branch 2 линейный, потом branch 1 exp).
  - **ИТОГ (11:43 UTC, деплой 76f62478)**: двухфазный декел (линейная 3с фаза → exp-затухание, формульный остаток — не накапливаемый), холд 3с у выхода, WarpStop НЕ шлёт CmdStop (клиентский Ballpark сам завершает варп при достижении цели — наблюдалось в накопительной сборке: клиент останавливался плавно без CmdStop). Результат юзера: «Варп работает, немного дёргает на стопе, но именно немного и не каждый раз». Дальнейшая доводка — только точная копия клиентского tanh-accel, высокий риск. Можно считать закрытым, если юзер согласен.
  - ВАЖНО про декел: формульный остаток (не накопление) — иначе кламп скорости 1e-5 застрелял корабль на нескольких сотнях метров → «You are already warping» после каждого варпа.
- Лог-каналы: DRONE__ERROR=1, DRONE__AI_TRACE=1, NPC__AI_TRACE=1, DESTINY__ORBIT_TRACE=1, DESTINY__MOVE_TRACE=1, DESTINY__WARP_TRACE=1 (диагностика дронов/варпа — после фиксов убрать).

## Имена предметов — приведены к клиенту (4 августа)
- **Проблема**: `/create 'Inferno Precision Light Missile'` → «Unable to find valid type to create», хотя итем продаётся на рынке. Причина: серверная `invTypes` содержит старые (Crucible-era) имена (`Flameburst Precision Light Missile`, `Bloodclaw Light Missile`, `Standard Missile Launcher I`), а клиент (SDE новее) показывает современные (`Inferno Precision Light Missile`, `Scourge Light Missile`, `Light Missile Launcher I`). Рынок работает (типы по typeID), но поиск по имени в `/create` падал. До ресета базы эта проблема не была — старая база уже имела client-like имена.
- **Источник клиентских имён**: `C:\Program Files (x86)\CCP\EVE\bulkdata\600004.cache2` — это клиентский кэш CRowset'а `invTypes` (blue.marshal, PyPackedRow). Распарсен скриптом `C:\Users\Dima\AppData\Local\Temp\opencode\dump_cache2.py` → `client_names.tsv` (18710 строк). Также есть современный SDE `C:\opencode-projects\misc\sde\fsd\types.yaml` (153MB, имена `en:`), который совпадает с cache2.
- **Фикс**: сравнены клиентские vs серверные имена → **1405 расхождений** (17272 уже совпадали). Сгенерирован `rename_invtypes.sql` (`UPDATE invTypes SET typeName=...` + `UPDATE entity SET itemName=...`) и выполнен в db-контейнере. Проверка: 2647 = «Inferno Precision Light Missile», 202 = «Mjolnir Cruise Missile», 499 = «Light Missile Launcher I».
- **Несовпавшие**: 33 типа есть только в клиенте (новые), 1041 только в сервере (непубличные/внутренние) — им имена не менялись.
- ВАЖНО: имена в `invTypes` — мастер-данные; при ресете базы (evedbtool install) они откатятся к старым — нужно повторить скрипт.

## Git Log (верх)
```
f5e22ece debug(market): dump GetStationAsks dict response (MARKET__DUMP)
9b956859 fix(market): GetStationAsks/GetSystemAsks/GetRegionBest return plain dict {typeID: util.KeyVal} (DBResultToTypeKeyValDict) — CIndexedRowset decodes to 0 rows in Crucible client
70646181 fix(market): return dbutil.CIndexedRowset instead of util.IndexRowset (has .get() + DBRow attribute access)
8040631b fix(market): raise Station/System/RegionOrderLimit 10->20000 + MARKET__TRACE on ask methods
26cc777d feat(dungeon): Guristas decoration set — freight pads (23237), gas/storage silos (10788), storage warehouse (30786), asteroid colony tower (10779), talocan silo (30506), scanner sentry (10144) — pirate-base logistics clutter per faction lore
9cf29c15 fix(dungeon): acceleration gate 25-30km from pocket center (was 0.8*NEXT_DUNGEON_ROOM_DIST=40M km, outside bubble, invisible/unreachable)
cc96e832 feat(dungeon+mining): mineable asteroid belts in dungeon pockets (30-40 of one ore, temp, cleaned on expiry) + DroneAI::MiningAttack now depletes AttrQuantity and deletes rock when empty (was 'never-depleting ore') + AsteroidSE::Delete null-guard on m_beltMgr
3ec64963 fix(dungeon): anomaly decorations purely visual — clouds (grp 227/312) + LCO ship Wreckage (grp 226, non-lootable), removed interactive containers/wrecks that rendered as 'gates don't activate, wrecks can't loot, empty containers'
d52f47a2 fix(destiny): orbit 'TooFar' recomputes approach heading every tick — stale-point early return let ships/NPCs fly toward where the target WAS, distance grew 5->30km ('orbited an NPC', drones followed into space = 'teleport across screen'). Same bug as NPCAI::SetChasing stale point (9d4e8f8c).
3103b69a fix(drone): no teleport when drone switches chase→orbit (SetEngaged now syncs SetPosition before CmdOrbit, same as SetApproaching/IdleOrbit — client Ballpark re-anchored ball to new orbit point from desynced position)
f9945956 fix(drone): control-range leash — drone stops chasing targets beyond GetControlRange() (was 2x=40km; NPCs with huge flyRange like Eradicator 27km pulled drones into deep space)
cc1cebb3 fix(ship): stop orbiting invisible decorations — IsTargetInvalid now stops ORBIT/FOLLOW when target has no DestinyMgr (decorations CelestialSE/ItemSystemEntity have m_destiny=nullptr + IsDynamicEntity()==false, so orbit persisted forever around invisible point -> 'circles invisible object', warp align never settled -> 'warp align/speed incorrect' -> stuck WARP -> 'You are already warping')
a43b2bab fix(drone): no teleport when drone returns to carrier after target dies — IdleOrbit now syncs position (SetBallPosition) before CmdOrbit, same as SetApproaching; client Ballpark re-anchored ball to new orbit point from desynced position -> visible teleport at moment target dies
2ebb22df fix(drone+ship): killed drone phantom 'distant space' (ClearController before SetIdle/StateChange so OnDroneStateChange prunes dead drone from client stateByDroneID — live controllerID made client think drone still in space + issue Return commands, yanking other drones) + removed legacy every-50-tic SetBallPosition to pilot's ship (~12.5s jerk in non-ORBIT modes)
5d086488 fix(npc): NPC killed twice — NPC::Killed called RemoveNPC (RemoveEntity + m_self->Delete) and Damage::ApplyDamage then called SystemEntity::Killed -> Delete() again. Double removal corrupted heap; segfault ~35s later on missile removal. NPC::Killed now only unregisters from lists (RemoveNPCFromList); full removal via SystemEntity::Killed. /kill and /killallnpcs chain SystemEntity::Killed.
42d8e954 fix(dungeon): decor/gate entities are static (IsStaticEntity=true) so SendAddBalls never delivered them — AddBallExclusive after spawn
2a8b3921 fix(drone): InventoryItem::Delete no longer junkyard-moves drones (cat 18) — dead drones left ghost entity rows at (0,0,0)
2b54fa15 fix(drone): drop stale target when destroyed structure (no TargetMgr) is removed — drone chased dead target into deep space
40b94991 feat(dungeon): transient decorations (Sansha LCO structures), acceleration gates between multi-room dungeons, cleanup script
e74345ac fix(dungeon): decorations spawn as transient items (SpawnTemp, no SaveItem)
f502d7ce fix: incursion ISK rewards use rewardTypeID=2 (const.rewardTypeISK) not 0
f8bca947 fix: StargateJump range check surface-to-surface distance
92918a2d fix: wrong table name crpCorporations -> crpCorporation
27afe41d feat: ECM bomb jams targets + Energy bomb drains 1800GJ
eea108d8 feat: add Bomb_ECM/Bomb_Energy group enum + AoE detonation
88b4f45e feat: bomb launcher mechanics — bombs fly straight, detonate AoE
4ee3d397 fix: missile launcher cycle not displayed — ShowEffect charge effectID
011a2b98 fix: TCU/anchored structures with AttrAnchoringDelay
46a3ef07 fix: reject CmdStargateJump beyond maxStargateJumpingDistance
ab1d6677 feat: gate activation effects (GateActivity)
c5df3622 fix: add jump cloak + invul after gate jump
a02fca16 fix: StargateJump mirrors Command_tr exactly
7aa86e89 fix: AP gate jump uses .tr-style MoveToLocation flow
143c28e3 fix(orbit): point heading at orbit circle point instead of tangent — orbit no longer unwinds
04ee6348 fix(npc): NPCs aggroed via Targeted() never fired — attack/missile timers only started in Target(); clamp absurdly long AttrSpeed cycles (30000ms) to 3-8s
672afd73 fix(npc): minimum orbit range 1500-5000m when AttrMaxRange is tiny/0 — NPCs no longer clinch the player
670b8635 docs: drone fixes (scoop/orbit/return/messages) + disk-full warning (docker image prune)
29ecf6ce fix: AP warp-to-gate targets a point radius+apWarptoDistance from gate center (warp-to-point)
8b674e50 fix: AP warp-to-gate lands apWarptoDistance from gate surface (add gate radius), capped at 25km
865a5b64 revert: AP warp stop distance back to apWarptoDistance (15000 from gate center)
95fe7f34 fix: AP warp-to-gate lands apWarptoDistance from gate surface (REVERTED)
cc5ffaa2 fix: warp alignment re-arms ship when USF<0.75 regardless of TF
960d6715 fix: re-send benign session change (nextSessionChange) ~5s after gate jump
b6e77ec3 fix: remove double-wrap in AddBalls XML
fe8ece5b fix: remove double-wrapping in AddBalls2 XML
```

## Key Decisions
fe8ece5b fix: remove double-wrapping in AddBalls2 XML — client expects (state, extraBallData)
339357f8 fix: add PyIncRef before act.update = *update — prevents use-after-free
2ac27b78 fix: add targeted action raw-type logging in _SendQueuedUpdates
b00af344 fix: reject destiny updates with non-scalar first item (e.g., list)
e5e76d7a fix: remove last stateStamp ref in SendStaticBall
8d99f0b6 fix: remove stateStamp ref in ShipService.cpp
c901541f fix: restructure AddBalls2 to start with string funcName
d5fd2877 fix: PyIncRef before push to m_pendingUpdates
d1b34cc8 fix: remove stateStamp refs in SystemManager.cpp
d24189e7 chore: comment SIGSEGV guard in MakeSetState
182a8cce fix: auto-cleanup orphaned decorations on server restart
df680a34 fix: add RemoveEntity in DroneSE destructor
a7e30938 fix: cleanup corrupt decoration entities from DB
48d15e07 fix: disable DoPackage path except SetState
c52e7993 fix: add RemoveEntity before delete this in ContainerSE::Process
a37a3a6a fix: remove DestinyMgr null check from MakeSetState
94954991 fix: make SystemBubble::GetID() const
41462a87 fix: build error — GetID() not const in MakeSetState
db232d41 debug: log MakeSetState entry
7519ad51 fix: skip entities not in m_ticEntities during MakeSetState
e4c7209e fix: set velocity toward target before InitWarp on alignment timeout
e9e5e202 fix: drone AI use-after-free + MakeSetState null check
9f822aa4 fix: BubblecastDestinyUpdate clones before broadcast
8d13e0c5 fix: TargetManager::QueueUpdate clones before broadcast
d4fee97c fix: SendDamageStateChanged crash
ffc681ae fix: incursionRewards PK fix
331413a9 feat: procedural faction-based dungeon decorations
```

## Key Decisions

### Warp Disrupt / MWD Bubble
- **No distance check in destiny.dll** — `OnActivatingWarp` only checks `m_activations[0].size() > 0` via `PyDict_GetItem`, no range check
- **Dynamic bubble toggle removed** — WarpDisruptFieldGenerating always stays visible; `AttrWarpScrambleStatus` handles range-based scramble per-ship
- **Bubble::Add** skips `SendAddBalls` for warping ships (WarpLoop crash prevention)
- `warpScrambleTimer` runs every 1000ms, sets `AttrWarpScrambleStatus` based on actual distance to bubble center

### Warp Physics
Based on decompiled `destiny.dll` (stored at `C:\opencode\projects\other\`):
- **Accel**: `distance = e^(3t)`, `speed = 3e^(3t)` ✓ (already implemented)
- **Decel**: `distance = total - e^(-t)*decelDist`, `speed = warpSpeed * e^(-t)` ✓ (already implemented)
- **Exit condition**: `distance < ball->radius` (changed from hardcoded 100m to `m_radius`)
- **Catch-all/30° warp**: no longer zeroes `m_velocity` — ship enters warp with momentum
- **PyMethodDef table**: only 4 exports — `FindShortestPath`, `Test`, `GetBoxCenter`, `SetConstant`

### Bump Physics
- **Fixed collision formula**: `distance -= (r1 + r2)` instead of `(r1 - r2)` — was computing surface distance incorrectly

### Client Crashes (all confirmed via decompiled `.py` files)
1. **graphicInfo=None** → `effects.SmartBomb`/`MicroWarpDrive` skipped in `ShowEffect`; `SendSpecialEffect` routes to `OnSpecialFX10` when graphicInfo=0
2. **bracket None.lower()** → `"name"` field added to `MakeSlimItem()` for all entity types
3. **AddBalls2 KeyError** → `EncodeDestiny()` for DeployableSE includes `DataSector` for `IsFree`
4. **WarpLoop crash** → `WarpStop()` sends no packets; `Bubble::Add()` skips `SendAddBalls` for warping ships

### Mail System
- `SelfEveMail()` in LSCService — inserts `mailMessage` + `mailStatus`
- `GetMailBody()` returns raw compressed data (client decompresses)
- `MailingListGetInfo/GetSettings` — fixed leading spaces in list names
- `OnMailSent` notifications

### Sovereignty
- `militaryPoints`/`industrialPoints` default changed from 5 to 0 (client expects raw values, no fallback)

### Ghidra Analysis
- JDK 21 + Ghidra 12.1.2 installed at `C:\Users\User\AppData\Local\Temp\ghidra_extract\`
- Decompiled `destiny.dll` output: `C:\Users\User\AppData\Local\Temp\opencode\destiny_decompile.txt` (48KB)
- Method table dump: `C:\Users\User\AppData\Local\Temp\opencode\destiny_methods.txt`
- Key functions: `OnActivatingWarp @ 0x1001b3c0`, `EntityWarpIn @ 0x10020ed0`, `OnDeactivatingWarp @ 0x100209f0`, `OnExitWarp @ 0x10004b00`
- Scripts: `ghidra_script.java`, `ghidra_full.java`, `ghidra_methods.java` in `C:\Users\User\AppData\Local\Temp\opencode\`

### Decompiled Client Python
- 1082 files decrypted from compiled.code → `C:\opencode\projects\other\all\`
- Key files: `michelle.py`, `fxSequencer.py`, `bracketMgr.py`, `godma.py`, `evemail.py`, `sovSvc.py`
- Doc string shows Python 2.4 compatibility, Crucible branch

## Progress

### Done
- **Mail**: SelfEveMail inserts mailMessage+mailStatus; GetMailBody raw compressed; mailing list leading-space fix; all 8 SendMail overloads (PyInt/PyBool/PyString/PyWString); MarkAsUnreadByList signature fix; MarkAsReadByList/TrashByList listID→messageID bug; SyncMail range filter via second param; OnMailSent notification
- **MWD deployables**: bubble toggle removed (always visible); server-side AttrWarpScrambleStatus check in WarpTo(); Bubble::Add sends OnSpecialFX14 with graphicInfo(range); warpScrambleTimer periodic 1000ms range checks; EncodeDestiny DataSector for IsFree; range hardcoded per SDE typeID (5k–48k); scramble cleanup on bubble exit (Remove) + per-player range check
- **Warp physics**: accel formula divide-by-3 fix (was 1/3 dist); capacitor mass unit fix (kg→Mkg, was ×1000); decel exit m_radius instead of hardcoded 100m; catch-all/30° no longer zeroes velocity; warp-to-0 surface landing; collision detection (bump off gates/stations); warp intercept via HasWarpBubble; JumpIn broadcast on gate jump
- **Defender missiles**: ShipSE::MissileLaunched auto-fires defenders; Missile::HitTarget intercepts missiles; public Destroy() method; Countermeasure_Launcher enabled in ModuleFactory
- **Client crash fixes**: graphicInfo=None→skip SmartBomb/MicroWarpDrive; bracket "name" field in all MakeSlimItem; AddBalls2 DataSector; WarpLoop SendAddBalls skip; Bump null-check pilots; sourceShipID in Missile MakeSlimItem
- **Destiny update use-after-free**: PyIncRef before act.update assignment in all paths; PyIncRef before push to m_pendingUpdates; reject empty tuples and non-scalar first items
- **AddBalls/AddBalls2 double-wrap**: removed extra tuple wrapping ((data),) → (data); restructured to start with string funcName; removed stateStamp field
- **OnModuleAttributeChange size**: restored oldValue (7-item tuple); events sent separately via OnMultiEvent to avoid RealFlushState unpack mismatch
- **Sovereignty**: militaryPoints/industrialPoints default 5→0

### To Test
- **Warp scramble**: ship inside MWD bubble gets "Warp drive is disrupted."; outside bubble (>range) can warp
- **Gate arrival**: JumpIn + GateActivity effects play when warping to a gate
- **Capacitor**: frigate ~3 GJ / 5 AU; battleship ~300 GJ / 5 AU; Warp Drive Operation skill reduces drain
- **Mail**: Cyrillic; reply/forward flags; labels; mailing lists; blocked-contact filter; rate limit
- **Defender**: active launcher + defender charge intercepts incoming missiles
- **Bump**: correct surface distance (r1+r2+BUMP_DISTANCE); notification messages
- **Brackets**: all entity types display name/type/corp/alliance without AttributeError
- **Sovereignty dashboard**: loads without error; index values display correctly
- **Gate jump**: JumpOut→JumpIn→GateActivity sequence; no SceneManager `NoneType.vx`

### Remaining Issues
- Full `WarpDisruptFieldGenerating` effect classification missing in Crucible
- SceneManager crash (`NoneType.vx`) is a secondary effect — primary warp crash now fixed
- `tabgroup UnicodeDecodeError` (CP1252) — client-side, needs `errors='replace'` in editplaintext.py
