-- +migrate Up
-- Admin-published news: broadcast to the public (player) Telegram group and
-- stored here so the portal can list history.
CREATE TABLE IF NOT EXISTS serverNews (
    newsID      int(10) unsigned NOT NULL AUTO_INCREMENT,
    title       varchar(200)      NOT NULL DEFAULT '',
    body        text              NOT NULL,
    authorName  varchar(64)       NOT NULL DEFAULT '',
    createdAt   datetime          NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (newsID)
) ENGINE=InnoDB;
-- +migrate Down
DROP TABLE IF EXISTS serverNews;
