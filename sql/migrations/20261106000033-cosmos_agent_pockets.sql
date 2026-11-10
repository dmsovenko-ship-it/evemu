-- COSMOS agents inside a deadspace complex: remember which pocket they stand in.
-- +migrate Up
ALTER TABLE cosmosAgentSites ADD COLUMN IF NOT EXISTS pocket tinyint(3) unsigned NOT NULL DEFAULT 1;
UPDATE cosmosAgentSites SET pocket=2 WHERE agentID=3018370;
UPDATE cosmosAgentSites SET pocket=2 WHERE agentID=3018396;
UPDATE cosmosAgentSites SET pocket=2 WHERE agentID=3018449;
UPDATE cosmosAgentSites SET pocket=3 WHERE agentID=3018458;
UPDATE cosmosAgentSites SET pocket=4 WHERE agentID=3018564;
UPDATE cosmosAgentSites SET pocket=4 WHERE agentID=3018634;
-- +migrate Down
-- (column stays)
