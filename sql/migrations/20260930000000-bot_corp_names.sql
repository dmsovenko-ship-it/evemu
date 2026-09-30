-- +migrate Up
-- Real (killboard-sourced) corporation names/tickers used by chelobot-founded corps.
-- The EDK killboards (kb.sotzone.ru) show the real corp of every victim; the tool
-- tools/import_edk_corps.py fills this table, and BotMgr::MakeCorpName() draws an
-- unused row so bot corps look like real player corps instead of procedural names.
CREATE TABLE IF NOT EXISTS `botCorpNames` (
  `id`       INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `corpName` VARCHAR(128) NOT NULL,
  `ticker`   VARCHAR(16)  NOT NULL DEFAULT '',
  `used`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uq_corpName` (`corpName`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- +migrate Down
DROP TABLE IF EXISTS `botCorpNames`;
