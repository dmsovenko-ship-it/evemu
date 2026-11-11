-- System broadcasts to all real players (portal admin page).
-- status: 0 = scheduled/pending, 1 = sent, 2 = cancelled.
-- +migrate Up
CREATE TABLE IF NOT EXISTS systemBroadcasts (
  broadcastID   INT UNSIGNED NOT NULL AUTO_INCREMENT,
  subject       VARCHAR(255) NOT NULL,
  body          MEDIUMTEXT NOT NULL,
  authorName    VARCHAR(100) NOT NULL DEFAULT '',
  status        TINYINT NOT NULL DEFAULT 0,
  scheduledAt   DATETIME NULL,
  sentAt        DATETIME NULL,
  sentCount     INT UNSIGNED NOT NULL DEFAULT 0,
  messageID     INT UNSIGNED NOT NULL DEFAULT 0,
  createdAt     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (broadcastID),
  KEY idx_broadcast_due (status, scheduledAt)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS systemBroadcasts;
