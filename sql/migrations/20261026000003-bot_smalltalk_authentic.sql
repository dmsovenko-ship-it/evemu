-- +migrate Up
-- Authentic RU chatter mined from live EVE players (goha.ru PvP/bar threads,
-- pikabu battle writeups).  Stored in the "Russian" pools as profession+100
-- (155 = RU combat; 255 is the EN combat pool); BotMgr::BuildBotSmalltalkLine mixes them in on top of its
-- hardcoded situational lines, so the ambient chat reads like real pilots
-- instead of uniform generated phrases.
INSERT INTO botSmalltalk (profession, line) VALUES
-- Hunter (100)
 (100, 'иду по локалу, вроде чисто'),
 (100, 'на гейте кемп, обхожу стороной'),
 (100, 'краблю нули, дроп идёт'),
 (100, 'поймал копалку, такл держу'),
-- RatHunter (101)
 (101, 'хавен сняли, иду на следующий'),
 (101, 'ановый дроп неплохой, живём'),
 (101, 'крысы жирные, но платят'),
 (101, 'фокус по структуре, добиваем'),
-- Miner (102)
 (102, 'копалку затаклили, пришлось отварпать'),
 (102, 'стрипы гудят, трюм полный'),
 (102, 'руда — наше всё, ребят'),
 (102, 'нейт в локале, аккуратнее'),
-- Trader (103)
 (103, 'спред норм, торгуем потихоньку'),
 (103, 'опять демпят, бесит'),
 (103, 'купил дёшево, поставил дорого'),
 (103, 'хаб движ, день удался'),
-- Courier (104)
 (104, 'груз тяжёлый, варп долгий'),
 (104, 'контракт взял, полетел'),
 (104, 'коллатерал дорогой, аккуратнее'),
 (104, 'лоу стрёмный, но везут'),
-- RU combat pool (155; 255 is the EN combat pool, the column is TINYINT)
 (155, 'таклим, варпайте сюда'),
 (155, 'минус!'),
 (155, 'весь ганг сливается, втапливай'),
 (155, 'грейте мвд, иначе не догоним'),
 (155, 'держу скребком'),
 (155, 'фокус по логистам'),
 (155, 'дреды пробросили, факсы не тянут'),
 (155, 'судный день прошёл, добиваем');

-- +migrate Down
DELETE FROM botSmalltalk WHERE profession IN (100, 101, 102, 103, 104, 155);
