-- +migrate Up
-- Persistent Telegram/telemetry markers on the single srvStatus row, so
-- notifications and the daily admin report survive server restarts:
--   lastDigest      = unix time the daily top-kills digest was last sent (24h)
--   lastOffline     = unix time the server last went down; the next boot only
--                     announces "online" if the downtime was real (>= 10 min)
--   lastAdminReport = unix time the evening admin report was last sent
--   bootCount/crashCount       = cumulative boots / unclean shutdowns
--   lastReport*                = values at the previous evening report (delta)
--   cleanShutdown              = 0 while running, 1 after a clean shutdown;
--                                a boot that finds 0 counts as a crash
ALTER TABLE srvStatus
    ADD COLUMN lastOffline      bigint(20) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN lastDigest       bigint(20) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN lastAdminReport  bigint(20) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN bootCount        int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN crashCount       int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN lastReportBoot   int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN lastReportCrash  int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN lastReportCommit int(10) unsigned NOT NULL DEFAULT 0,
    ADD COLUMN cleanShutdown    tinyint(1) NOT NULL DEFAULT 1;

-- +migrate Down
ALTER TABLE srvStatus
    DROP COLUMN lastOffline,
    DROP COLUMN lastDigest,
    DROP COLUMN lastAdminReport,
    DROP COLUMN bootCount,
    DROP COLUMN crashCount,
    DROP COLUMN lastReportBoot,
    DROP COLUMN lastReportCrash,
    DROP COLUMN lastReportCommit,
    DROP COLUMN cleanShutdown;
