-- COSMOS site structures (wiki Structures tables) - visual/landmark objects.
-- +migrate Up
CREATE TABLE IF NOT EXISTS cosmosSiteStructures (
  siteID int(10) unsigned NOT NULL,
  pocket tinyint(3) unsigned NOT NULL,
  typeID int(10) unsigned NOT NULL,
  qty tinyint(3) unsigned NOT NULL DEFAULT 1,
  PRIMARY KEY (siteID, pocket, typeID)
) ENGINE=InnoDB;
DELETE FROM cosmosSiteStructures;
INSERT INTO cosmosSiteStructures (siteID, pocket, typeID, qty) VALUES
(9,2,16831,1),
(9,2,16840,1),
(9,2,21519,1),
(9,2,21617,1),
(14,2,21622,1),
(14,4,10267,1),
(49,3,25384,1),
(50,3,25402,1),
(84,1,22031,1),
(84,1,22032,1),
(84,1,22161,1),
(84,2,21884,1),
(84,2,22059,1),
(84,2,22158,1),
(84,2,22168,1),
(101,2,22241,1),
(101,2,22318,1),
(101,4,22232,1),
(128,3,16788,1),
(128,3,16792,1),
(128,3,24579,1),
(134,1,23595,1),
(134,4,23617,1),
(134,5,23600,1),
(134,5,23696,1),
(136,2,24356,1),
(137,2,24351,1),
(138,4,2011,1),
(138,4,11076,1),
(138,4,23240,1),
(172,5,23743,1),
(174,1,24237,1),
(174,2,24124,1),
(174,2,24237,1),
(174,3,24237,1),
(183,1,10261,1),
(185,1,24464,1),
(187,4,23964,1),
(187,6,24243,1),
(187,6,24256,1),
(207,1,21840,1);
-- +migrate Down
DROP TABLE IF EXISTS cosmosSiteStructures;
