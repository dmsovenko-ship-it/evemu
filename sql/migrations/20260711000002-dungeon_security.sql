-- Add security/difficulty columns to dunDungeons for anomaly tier selection
-- +migrate Up
ALTER TABLE dunDungeons
  ADD COLUMN `minSecurity` FLOAT NOT NULL DEFAULT -1.0 AFTER factionID,
  ADD COLUMN `maxSecurity` FLOAT NOT NULL DEFAULT 1.0 AFTER minSecurity,
  ADD COLUMN `difficulty`  TINYINT UNSIGNED NOT NULL DEFAULT 1 AFTER maxSecurity;

-- Seed security/difficulty for anomaly dungeons (archetypeID=7)
-- Hideaway / Hidden Hideaway / Forlorn Hideaway → highsec, DED 1-3
UPDATE dunDungeons SET minSecurity=0.5, maxSecurity=1.0, difficulty=1 WHERE archetypeID=7 AND dungeonName LIKE '%Hideaway' AND dungeonName NOT LIKE '%Hidden%' AND dungeonName NOT LIKE '%Forlorn%';
UPDATE dunDungeons SET minSecurity=0.3, maxSecurity=1.0, difficulty=2 WHERE archetypeID=7 AND dungeonName LIKE '%Hidden Hideaway%';
UPDATE dunDungeons SET minSecurity=0.1, maxSecurity=1.0, difficulty=3 WHERE archetypeID=7 AND dungeonName LIKE '%Forlorn Hideaway%';

-- Burrow → highsec-lowsec, DED 2
UPDATE dunDungeons SET minSecurity=0.3, maxSecurity=1.0, difficulty=2 WHERE archetypeID=7 AND dungeonName LIKE '%Burrow';

-- Den → highsec through nullsec, DED 3
UPDATE dunDungeons SET minSecurity=0.0, maxSecurity=1.0, difficulty=3 WHERE archetypeID=7 AND dungeonName LIKE '%Den' AND dungeonName NOT LIKE '%Hidden%' AND dungeonName NOT LIKE '%Forlorn%';
UPDATE dunDungeons SET minSecurity=-0.2, maxSecurity=1.0, difficulty=4 WHERE archetypeID=7 AND dungeonName LIKE '%Hidden Den%';
UPDATE dunDungeons SET minSecurity=-0.4, maxSecurity=0.9, difficulty=5 WHERE archetypeID=7 AND dungeonName LIKE '%Forlorn Den%';

-- Rally Point → lowsec+nullsec, DED 4-5
UPDATE dunDungeons SET minSecurity=0.1, maxSecurity=0.9, difficulty=4 WHERE archetypeID=7 AND dungeonName LIKE '%Rally Point' AND dungeonName NOT LIKE '%Hidden%' AND dungeonName NOT LIKE '%Forlorn%';
UPDATE dunDungeons SET minSecurity=-0.1, maxSecurity=0.7, difficulty=5 WHERE archetypeID=7 AND dungeonName LIKE '%Hidden Rally Point%';
UPDATE dunDungeons SET minSecurity=-0.3, maxSecurity=0.5, difficulty=6 WHERE archetypeID=7 AND dungeonName LIKE '%Forlorn Rally Point%';

-- Port → lowsec+nullsec, DED 6
UPDATE dunDungeons SET minSecurity=-0.2, maxSecurity=0.4, difficulty=6 WHERE archetypeID=7 AND dungeonName LIKE '%Port';

-- Hub → nullsec, DED 7-8
UPDATE dunDungeons SET minSecurity=-0.5, maxSecurity=0.0, difficulty=7 WHERE archetypeID=7 AND dungeonName LIKE '%Hub' AND dungeonName NOT LIKE '%Hidden%' AND dungeonName NOT LIKE '%Forlorn%' AND dungeonName NOT LIKE '%Forsaken%';
UPDATE dunDungeons SET minSecurity=-0.7, maxSecurity=-0.1, difficulty=8 WHERE archetypeID=7 AND dungeonName LIKE '%Hidden Hub%';
UPDATE dunDungeons SET minSecurity=-0.9, maxSecurity=-0.3, difficulty=9 WHERE archetypeID=7 AND (dungeonName LIKE '%Forsaken Hub%' OR dungeonName LIKE '%Forlorn Hub%');

-- Haven → deep nullsec, DED 9
UPDATE dunDungeons SET minSecurity=-1.0, maxSecurity=-0.3, difficulty=9 WHERE archetypeID=7 AND dungeonName LIKE '%Haven%';

-- Sanctum → deepest nullsec, DED 10
UPDATE dunDungeons SET minSecurity=-1.0, maxSecurity=-0.5, difficulty=10 WHERE archetypeID=7 AND dungeonName LIKE '%Sanctum%';

-- Rogue Drone sites
-- Drone Cluster → highsec
UPDATE dunDungeons SET minSecurity=0.5, maxSecurity=1.0, difficulty=1 WHERE archetypeID=7 AND dungeonName = 'Drone Cluster';
-- Drone Collection → highsec-lowsec
UPDATE dunDungeons SET minSecurity=0.1, maxSecurity=1.0, difficulty=2 WHERE archetypeID=7 AND dungeonName = 'Drone Collection';
-- Drone Assembly → lowsec
UPDATE dunDungeons SET minSecurity=0.0, maxSecurity=0.7, difficulty=3 WHERE archetypeID=7 AND dungeonName = 'Drone Assembly';
-- Drone Menagerie → lowsec
UPDATE dunDungeons SET minSecurity=-0.2, maxSecurity=0.5, difficulty=4 WHERE archetypeID=7 AND dungeonName = 'Drone Menagerie';
-- Drone Surveillance → lowsec
UPDATE dunDungeons SET minSecurity=-0.3, maxSecurity=0.4, difficulty=5 WHERE archetypeID=7 AND dungeonName = 'Drone Surveillance';
-- Drone Squad → nullsec
UPDATE dunDungeons SET minSecurity=-0.5, maxSecurity=0.1, difficulty=7 WHERE archetypeID=7 AND dungeonName = 'Drone Squad';
-- Drone Herd → nullsec
UPDATE dunDungeons SET minSecurity=-0.7, maxSecurity=-0.1, difficulty=8 WHERE archetypeID=7 AND dungeonName = 'Drone Herd';
-- Drone Patrol → nullsec
UPDATE dunDungeons SET minSecurity=-0.9, maxSecurity=-0.3, difficulty=9 WHERE archetypeID=7 AND dungeonName = 'Drone Patrol';
-- Drone Horde → deep nullsec
UPDATE dunDungeons SET minSecurity=-1.0, maxSecurity=-0.5, difficulty=10 WHERE archetypeID=7 AND dungeonName = 'Drone Horde';

-- +migrate Down
ALTER TABLE dunDungeons DROP COLUMN difficulty;
ALTER TABLE dunDungeons DROP COLUMN maxSecurity;
ALTER TABLE dunDungeons DROP COLUMN minSecurity;
