-- Fix mailListUsers schema + implement Join/Leave/Delete
-- +migrate Up

-- Fix primary key to allow multiple users per list
ALTER TABLE mailListUsers DROP PRIMARY KEY, ADD PRIMARY KEY (listID, characterID);

-- +migrate Down
ALTER TABLE mailListUsers DROP PRIMARY KEY, ADD PRIMARY KEY (listID);
