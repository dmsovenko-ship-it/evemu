-- Apply missing migrations that EVEDBTool skipped: incursions, dungeon_security, reinforce_hour
-- +migrate Up

-- === 20260707000004-incursions.sql ===
CREATE TABLE IF NOT EXISTS incursions (
    incursionID INT(10) NOT NULL AUTO_INCREMENT,
    factionID INT(10) NOT NULL DEFAULT 500019,
    stagingSolarSystemID INT(10) NOT NULL,
    constellationID INT(10) NOT NULL DEFAULT 0,
    regionID INT(10) NOT NULL DEFAULT 0,
    state TINYINT(1) NOT NULL DEFAULT 0,
    influence DOUBLE NOT NULL DEFAULT 0.0,
    hasBoss TINYINT(1) NOT NULL DEFAULT 0,
    rewardGroupID INT(10) NOT NULL DEFAULT 0,
    taleID INT(10) NOT NULL DEFAULT 0,
    graceTime INT(10) NOT NULL DEFAULT 30,
    decayRate DOUBLE NOT NULL DEFAULT 0.01,
    lastUpdated BIGINT(20) NOT NULL DEFAULT 0,
    PRIMARY KEY (incursionID)
);

CREATE TABLE IF NOT EXISTS incursionSystems (
    incursionID INT(10) NOT NULL,
    solarSystemID INT(10) NOT NULL,
    sceneType TINYINT(1) NOT NULL DEFAULT 0,
    influence DOUBLE NOT NULL DEFAULT 0.0,
    PRIMARY KEY (incursionID, solarSystemID)
);

CREATE TABLE IF NOT EXISTS incursionRewards (
    rewardGroupID INT(10) NOT NULL PRIMARY KEY,
    rewardTypeID INT(10) NOT NULL DEFAULT 0,
    rewardQuantity INT(10) NOT NULL DEFAULT 0,
    lpTypeID INT(10) NOT NULL DEFAULT 0,
    lpAmount INT(10) NOT NULL DEFAULT 0
);

INSERT IGNORE INTO incursions (incursionID, factionID, stagingSolarSystemID, constellationID, regionID, state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate) VALUES
(1, 500019, 30004323, 20000383, 10000045, 0, 0.0, 0, 3, 192, 30, 0.01);

-- === 20260711000002-dungeon_security.sql ===
ALTER TABLE dunDungeons
  ADD COLUMN IF NOT EXISTS `minSecurity` FLOAT NOT NULL DEFAULT -1.0 AFTER factionID,
  ADD COLUMN IF NOT EXISTS `maxSecurity` FLOAT NOT NULL DEFAULT 1.0 AFTER minSecurity,
  ADD COLUMN IF NOT EXISTS `difficulty`  TINYINT UNSIGNED NOT NULL DEFAULT 1 AFTER maxSecurity;

-- === 20260716000001-reinforce_hour.sql ===
ALTER TABLE mapSystemSovInfo
ADD COLUMN IF NOT EXISTS `reinforceHour` tinyint(4) NOT NULL DEFAULT 12 AFTER `contested`;

-- +migrate Down
