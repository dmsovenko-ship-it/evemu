-- Fix damageControl effect (2302): effect_category should be 4 (Active) not 1 (Passive)
-- Verified against live ESI: https://esi.evetech.net/latest/dogma/effects/2302/
-- +migrate Up

UPDATE dgmEffects SET effectCategory = 4 WHERE effectID = 2302 AND effectCategory = 1;

-- +migrate Down
UPDATE dgmEffects SET effectCategory = 1 WHERE effectID = 2302 AND effectCategory = 4;
