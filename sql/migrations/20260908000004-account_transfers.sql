-- +migrate Up
-- Legitimate account transfers: on live EVE selling a character/account through
-- the official portal was allowed (for us: approved by admins, usually via a
-- category-603 petition). RMT monitoring must NOT flag flows between a pair that
-- an admin has explicitly approved. One row = one approved hand-over.
CREATE TABLE IF NOT EXISTS accountTransfers (
    transferID        int(10) unsigned NOT NULL AUTO_INCREMENT,
    sellerAccountID   int(10) unsigned NOT NULL,
    buyerAccountID    int(10) unsigned NOT NULL,
    petitionID        int(10) unsigned NULL,
    approvedAt        datetime          NOT NULL DEFAULT CURRENT_TIMESTAMP,
    approvedBy        int(10) unsigned NOT NULL DEFAULT 0,
    note              varchar(255)      NOT NULL DEFAULT '',
    PRIMARY KEY (transferID),
    KEY idx_pair (sellerAccountID, buyerAccountID)
) ENGINE=InnoDB;

-- Player-facing "request an account/character hand-over" category so legitimate
-- sales happen in the open instead of being flagged as RMT.
INSERT IGNORE INTO portal_petition_categories (categoryID, parentCategoryID, languageID, categoryName, description, sortOrder) VALUES
    (603, 1, 'ru',     'Передача/продажа аккаунта', 'Официальная передача аккаунта или персонажа другому игроку.', 0),
    (603, 1, 'en-us',  'Account/Character Transfer', 'Official hand-over of an account or character to another player.', 0);
-- +migrate Down
DROP TABLE IF EXISTS accountTransfers;
DELETE FROM portal_petition_categories WHERE categoryID = 603;
