-- +migrate Up
-- Offline "player-like" skill training state for simulated pilots (chelobots).
-- One row per bot: fixed base attributes (bloodline x multiplier), the skill
-- currently in training, its progress toward the next level and the last tick
-- (filetime). BotMgr advances progress over real time with the EVE SP formula.
CREATE TABLE IF NOT EXISTS botTraining (
    charID       int(10) unsigned NOT NULL,
    attrInt      smallint(6)      NOT NULL DEFAULT 0,
    attrMem      smallint(6)      NOT NULL DEFAULT 0,
    attrPer      smallint(6)      NOT NULL DEFAULT 0,
    attrWill     smallint(6)      NOT NULL DEFAULT 0,
    attrCha      smallint(6)      NOT NULL DEFAULT 0,
    skillTypeID  int(10) unsigned NOT NULL DEFAULT 0,
    nextLevel    tinyint(3) unsigned NOT NULL DEFAULT 1,
    spProgress   double           NOT NULL DEFAULT 0,
    lastTrain    bigint(20) unsigned NOT NULL DEFAULT 0,
    PRIMARY KEY (charID)
) ENGINE=InnoDB;
-- +migrate Down
DROP TABLE IF EXISTS botTraining;
