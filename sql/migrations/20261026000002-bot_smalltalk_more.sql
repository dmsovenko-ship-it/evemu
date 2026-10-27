-- +migrate Up
-- More varied / less formal chat lines per profession (the base pool was a bit
-- "clean". The pick prefers least-used lines, so more lines = more variety and
-- fewer robotic repeats). English pool; Russian-speaking bots use the pools in
-- BotMgr::BuildBotSmalltalkLine.
INSERT INTO botSmalltalk (profession, line) VALUES
-- Miner (2)
 (2, 'hold''s full, heading back before someone ganks me lol'),
 (2, 'this belt''s picked clean, off to the next one'),
 (2, 'still on t1 lasers, don''t judge'),
 (2, 'rats keep chewing my drones, annoying'),
 (2, 'refine or sell raw? tell me i''m not the only one overthinking it'),
 (2, '3 hours of mining, 40m isk, worth it? eh'),
 (2, 'afk-ish, watching the strip cycle'),
 (2, 'if a neut shows up i''m out, no heroics'),
-- RatHunter (1)
 (1, 'anom popped, going in solo'),
 (1, 'these rats hit way harder than the last site'),
 (1, 'faction spawn dropped, today''s my day'),
 (1, 'need a haven, this system''s farmed out'),
 (1, 'bounties stacking up, not bad for an evening'),
 (1, 'officer on scan, backing off, not dying today'),
 (1, 'site''s clear, on to the next'),
 (1, 'loot''s garbage but the bounty was fine'),
-- Trader (3)
 (3, 'undercut war in Jita, love to see it'),
 (3, 'bought the dip, now we wait'),
 (3, 'margin''s thin but volume saves me'),
 (3, 'someone dumped 10k units, rip my orders'),
 (3, 'hauling profits at 0.01 isk, classic'),
 (3, 'market''s dead tonight, everyone asleep'),
-- Courier (4)
 (4, 'contract up, lowsec route, pray for me'),
 (4, 'collateral''s brutal on this one'),
 (4, 'delivered, paid, on to the next run'),
 (4, 'freighter life: slow, boring, profitable'),
 (4, 'gate camp on the route, rerouting'),
-- Hacker (5)
 (5, 'relic site paid out, decryptor''s mine'),
 (5, 'data sites camped again, ugh'),
 (5, 'hacked a can, got trash, moving on'),
 (5, 'scanning for sigs, nothing yet'),
 (5, 'one more relic and i''m done'),
-- Explorer (6)
 (6, 'found a hole, not going in, nope'),
 (6, 'sig''s a combat site, skipping'),
 (6, 'probing is 90% waiting'),
 (6, 'null static in the chain, tempting'),
 (6, 'got a nice data site, fingers crossed'),
-- Missioner (7)
 (7, 'agent gave me a long one, brb'),
 (7, 'ratting missions again, steady isk'),
 (7, 'loot from the mission, meh but ok'),
 (7, 'back at station, reporting in'),
-- Industrialist (8)
 (8, 'mineral prices up, my margins cry'),
 (8, 'production line running, don''t touch my bpos'),
 (8, 'bpc''s done, time to build'),
 (8, 'hauling materials, the usual grind'),
-- Combat (255)
 (255, 'get off me'),
 (255, 'nice shot, gg'),
 (255, 'warping out, not today'),
 (255, 'who''s tackling me lol')
ON DUPLICATE KEY UPDATE line = VALUES(line);

-- +migrate Down
-- content-only addition; nothing structural to revert
