-- Jump Clones support
-- Stores jump clone installations (separate from active medical clone in entity table)

CREATE TABLE IF NOT EXISTS chrJumpClones (
    cloneID INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
    characterID INT(10) UNSIGNED NOT NULL,
    stationID INT(10) UNSIGNED NOT NULL,
    typeID INT(10) UNSIGNED NOT NULL DEFAULT 0,
    cloneName VARCHAR(100) DEFAULT NULL,
    PRIMARY KEY (cloneID),
    INDEX idx_characterID (characterID),
    INDEX idx_stationID (stationID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Implants installed in jump clones
CREATE TABLE IF NOT EXISTS chrJumpCloneImplants (
    jumpCloneID INT(10) UNSIGNED NOT NULL,
    typeID INT(10) UNSIGNED NOT NULL,
    PRIMARY KEY (jumpCloneID, typeID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
