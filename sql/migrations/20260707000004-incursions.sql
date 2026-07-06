-- Incursion system tables
-- +migrate Up

CREATE TABLE IF NOT EXISTS incursions (
    incursionID INT(10) NOT NULL AUTO_INCREMENT,
    factionID INT(10) NOT NULL DEFAULT 500019,  -- Sansha's Nation
    stagingSolarSystemID INT(10) NOT NULL,
    constellationID INT(10) NOT NULL DEFAULT 0,
    regionID INT(10) NOT NULL DEFAULT 0,
    state TINYINT(1) NOT NULL DEFAULT 0,       -- 0=inactive, 1=mobilizing, 2=established, 3=withdrawing
    influence DOUBLE NOT NULL DEFAULT 0.0,
    hasBoss TINYINT(1) NOT NULL DEFAULT 0,
    rewardGroupID INT(10) NOT NULL DEFAULT 0,
    taleID INT(10) NOT NULL DEFAULT 0,
    graceTime INT(10) NOT NULL DEFAULT 30,      -- minutes
    decayRate DOUBLE NOT NULL DEFAULT 0.01,
    lastUpdated BIGINT(20) NOT NULL DEFAULT 0,
    PRIMARY KEY (incursionID)
);

CREATE TABLE IF NOT EXISTS incursionSystems (
    incursionID INT(10) NOT NULL,
    solarSystemID INT(10) NOT NULL,
    sceneType TINYINT(1) NOT NULL DEFAULT 0,    -- 1=vanguard, 2=assault, 3=headquarters, 4=staging
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

-- Seed a default Sansha incursion in a lowsec constellation
INSERT IGNORE INTO incursions (incursionID, factionID, stagingSolarSystemID, constellationID, regionID, state, influence, hasBoss, rewardGroupID, taleID, graceTime, decayRate) VALUES
(1, 500019, 30004323, 20000383, 10000045, 0, 0.0, 0, 192, 192, 30, 0.01);

-- +migrate Down
DROP TABLE IF EXISTS incursionRewards;
DROP TABLE IF EXISTS incursionSystems;
DROP TABLE IF EXISTS incursions;
