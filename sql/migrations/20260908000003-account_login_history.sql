-- +migrate Up
-- Login IP history for admin multiboxing / RMT monitoring: one row per client
-- login (account, ip:port, time). Admin groups accounts that share an IP and
-- correlates them with flagged activity.
CREATE TABLE IF NOT EXISTS accountLoginHistory (
    accountID     int(10) unsigned NOT NULL,
    ip            varchar(64)       NOT NULL,
    loginTime     datetime          NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY idx_account (accountID),
    KEY idx_ip     (ip)
) ENGINE=InnoDB;
-- +migrate Down
DROP TABLE IF EXISTS accountLoginHistory;
