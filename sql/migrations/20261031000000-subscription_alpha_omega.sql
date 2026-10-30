-- +migrate Up
-- Alpha/Omega subscription support (feature-flagged).
--
-- Gated by <subscription Enabled="true"> in eve-server.xml. When the flag is off
-- (default) nothing changes: every account behaves like an Omega account.
--
-- premiumUntil : filetime (100ns since 1601) until which the account is Omega.
--                0 = never paid -> Alpha.
-- alphaSkills  : whitelist of skill typeIDs an Alpha clone may train/use.
--                Skills outside this list are FROZEN for Alpha accounts (their
--                effects are not applied to the character; they are not deleted
--                and come back as soon as premiumUntil is in the future).
ALTER TABLE account ADD COLUMN IF NOT EXISTS premiumUntil BIGINT NOT NULL DEFAULT 0;

CREATE TABLE IF NOT EXISTS alphaSkills (
    typeID SMALLINT UNSIGNED NOT NULL,
    PRIMARY KEY (typeID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS alphaSkills;
ALTER TABLE account DROP COLUMN IF EXISTS premiumUntil;
