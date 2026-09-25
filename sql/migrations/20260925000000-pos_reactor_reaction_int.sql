-- +migrate Up
-- reactionTypeID can exceed SMALLINT range (e.g. 30344) — widen to UNSIGNED INT.
ALTER TABLE posReactorData MODIFY reaction INT UNSIGNED NOT NULL DEFAULT 0;

-- +migrate Down
ALTER TABLE posReactorData MODIFY reaction SMALLINT NOT NULL DEFAULT 0;
