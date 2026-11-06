-- +migrate Up
-- Password protection for containers (station containers / cargo containers).
-- A row means the container is locked; the owner can change/clear it, everyone
-- else sees the container as empty (contents hidden) until unlocked.
CREATE TABLE IF NOT EXISTS `containerPasswords` (
  `itemID` int(10) unsigned NOT NULL,
  `password` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`itemID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS `containerPasswords`;
