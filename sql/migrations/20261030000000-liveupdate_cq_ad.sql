-- +migrate Up
-- REMOVED: the CQ image template (live-update #10) broke the ship rendering of
-- the in-game ads. Videos still work through cache/CQScreenVideos/*.bik, so the
-- image template is dropped here (and deleted from liveupdates on boot).
DELETE FROM liveupdates WHERE updateID=10;

-- +migrate Down
-- nothing to do