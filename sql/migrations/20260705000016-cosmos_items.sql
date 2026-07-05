-- COSMOS unique mission items + market group
-- Custom typeIDs start at 99000 to avoid SDE conflicts
-- +migrate Up

-- COSMOS market group
INSERT IGNORE INTO `invMarketGroups` (`marketGroupID`, `parentGroupID`, `marketGroupName`, `description`, `graphicID`, `iconID`, `hasTypes`, `dataID`, `marketGroupNameID`, `descriptionID`) VALUES
(99000, 0, 'COSMOS Mission Items', 'Unique items required for COSMOS missions.', 0, 0, 1, 0, 0, 0);

-- COSMOS mission items
INSERT IGNORE INTO `invTypes` (`typeID`, `groupID`, `typeName`, `description`, `mass`, `volume`, `capacity`, `portionSize`, `raceID`, `basePrice`, `marketGroupID`, `published`) VALUES
-- Ani constellation items
(99001, 526, 'Sleeper Nanite Cluster', 'A cluster of nanites from Sleeper technology. Required for COSMOS missions.', 0, 1.0, 0, 1, 0, 50000, 99000, 1),
(99002, 526, 'Sleeper Data Crystal', 'An encoded data crystal of Sleeper origin.', 0, 1.0, 0, 1, 0, 75000, 99000, 1),
(99003, 526, 'Sleeper Manuscript', 'Ancient Sleeper text fragments.', 0, 1.0, 0, 1, 0, 100000, 99000, 1),
(99004, 526, 'Sleeper Foundation Block', 'Structural component from Sleeper architecture.', 0, 5.0, 0, 1, 0, 50000, 99000, 1),
(99005, 526, 'Sleeper Split Cable', 'A damaged but still functional Sleeper power cable.', 0, 2.0, 0, 1, 0, 35000, 99000, 1),
(99006, 526, 'Spy ID Slice', 'Forged identification chip used by intelligence operatives.', 0, 0.1, 0, 1, 0, 25000, 99000, 1),
(99007, 526, 'Hacker ID Slice', 'Modified ID chip with hacking credentials.', 0, 0.1, 0, 1, 0, 30000, 99000, 1),
(99008, 526, 'Sniper ID Slice', 'Military ID chip with sniper credentials.', 0, 0.1, 0, 1, 0, 30000, 99000, 1),
(99009, 526, 'Punk ID Slice', 'Gang-member identification chip.', 0, 0.1, 0, 1, 0, 15000, 99000, 1),
(99010, 526, 'Angel Simple Trigger Mechanism', 'Standard trigger assembly used by Angel Cartel weaponry.', 0, 0.5, 0, 1, 0, 20000, 99000, 1),
(99011, 526, 'Angel Spatial Analyzer', 'Angel Cartel scanning equipment.', 0, 2.0, 0, 1, 0, 40000, 99000, 1),
(99012, 526, 'Angel Drug Addict Tag', 'Identification chip from Angel Cartel drug users.', 0, 0.1, 0, 1, 0, 5000, 99000, 1),
(99013, 526, 'Angel Cartel Scanner Data', 'Stolen scanner data from Angel Cartel operations.', 0, 0.5, 0, 1, 0, 30000, 99000, 1),
(99014, 526, 'Angel Cartel Computer Hardware', 'Angel Cartel computer components.', 0, 3.0, 0, 1, 0, 40000, 99000, 1),
(99015, 526, 'Angel Diamond Tag', 'High-value identity tag from Arch Angel forces.', 0, 0.1, 0, 1, 0, 500000, 99000, 1),
(99016, 526, 'Angel Gold Tag', 'Valuable identity tag from Arch Angel officers.', 0, 0.1, 0, 1, 0, 250000, 99000, 1),
(99017, 526, 'Angel Silver Tag', 'Identity tag from Arch Angel soldiers.', 0, 0.1, 0, 1, 0, 100000, 99000, 1),
(99018, 526, 'Minmatar Republic Narcotic Officer Tag', 'Official tag from Minmatar narcotics division.', 0, 0.1, 0, 1, 0, 25000, 99000, 1),
(99019, 526, 'Navy Issue Amplifier', 'Military-grade signal amplifier.', 0, 5.0, 0, 1, 0, 100000, 99000, 1),
(99020, 526, 'Ancient Nefantar Sculpture', 'Ancient Minmatar tribal artifact from the Nefantar clan.', 0, 10.0, 0, 1, 0, 150000, 99000, 1),
(99021, 526, 'Transputer Orb', 'Advanced data storage device.', 0, 1.0, 0, 1, 0, 80000, 99000, 1),
(99022, 526, 'Cracked Keycard', 'A damaged security keycard.', 0, 0.1, 0, 1, 0, 10000, 99000, 1),
(99023, 526, 'Okham''s Head', 'The preserved head of Okham, a notorious criminal.', 0, 3.0, 0, 1, 0, 75000, 99000, 1),
(99024, 526, 'Bono Zakan Corpse', 'The body of Bono Zakan, a COSMOS target.', 0, 5.0, 0, 1, 0, 50000, 99000, 1),
(99025, 526, 'Kutill''s Data Chip', 'A datachip belonging to Kutill.', 0, 0.1, 0, 1, 0, 50000, 99000, 1),
(99026, 526, 'Godun Sakt''s Diamond Drill', 'High-grade mining drill used by Godun Sakt.', 0, 10.0, 0, 1, 0, 200000, 99000, 1),
(99027, 526, 'ST 60 Memory Chip', 'Memory chip from a ST 60 drone.', 0, 0.1, 0, 1, 0, 60000, 99000, 1),
(99028, 526, 'Kyan Magdesh''s DNA', 'DNA sample from Kyan Magdesh.', 0, 0.1, 0, 1, 0, 75000, 99000, 1),
(99029, 526, 'Rekker''s Keycard', 'Security keycard belonging to Rekker.', 0, 0.1, 0, 1, 0, 25000, 99000, 1),
(99030, 526, 'Runic Tablet', 'Ancient tablet with runic inscriptions.', 0, 8.0, 0, 1, 0, 120000, 99000, 1),
(99031, 526, 'Mysterious Portal Parts', 'Components from an unknown portal structure.', 0, 10.0, 0, 1, 0, 150000, 99000, 1),
(99032, 526, 'Gist Database Codes', 'Encrypted access codes from Guristas databases.', 0, 0.1, 0, 1, 0, 80000, 99000, 1),
(99033, 526, 'Destroyed ComLink Scanner', 'Salvaged communications scanner.', 0, 3.0, 0, 1, 0, 35000, 99000, 1),
(99034, 526, 'Searcher Drone Memory Chip', 'Memory core from a seacher drone.', 0, 0.1, 0, 1, 0, 45000, 99000, 1),
(99035, 526, 'Kardimo Palettan', 'A notorious criminal in cryogenic storage.', 0, 5.0, 0, 1, 0, 100000, 99000, 1),
(99036, 526, 'Lagaster Malotoff''s Tag', 'Identification tag of Lagaster Malotoff.', 0, 0.1, 0, 1, 0, 40000, 99000, 1),
(99037, 526, 'Nanom Basskel''s Ship Logs', 'Captained logs from Nanom Basskel''s vessel.', 0, 1.0, 0, 1, 0, 50000, 99000, 1),
(99038, 526, 'Vanir Makono''s DNA', 'DNA sample from the scientist Vanir Makono.', 0, 0.1, 0, 1, 0, 75000, 99000, 1),
(99039, 526, 'Norak Pakkul''s DNA', 'DNA sample from Norak Pakkul.', 0, 0.1, 0, 1, 0, 80000, 99000, 1),
(99040, 526, 'Republic Pilot Tags', 'ID tags from Republic fleet pilots.', 0, 0.1, 0, 1, 0, 30000, 99000, 1),
(99041, 526, 'Red Hammer''s Personal Effects', 'Personal belongings of the pirate Red Hammer.', 0, 2.0, 0, 1, 0, 60000, 99000, 1),
(99042, 526, 'Recruitment Center Data Log', 'Stolen recruitment data.', 0, 1.0, 0, 1, 0, 35000, 99000, 1);

-- +migrate Down
DELETE FROM `invTypes` WHERE `typeID` BETWEEN 99001 AND 99042;
DELETE FROM `invMarketGroups` WHERE `marketGroupID` = 99000;
