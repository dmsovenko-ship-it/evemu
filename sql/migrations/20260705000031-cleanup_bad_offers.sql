-- Clean up stale mission offers for agent types that should not have missions
-- Event(8), Aura(11) agents should not have mission offers in the DB
-- +migrate Up

DELETE o FROM agtOffers o
  INNER JOIN agtAgents a ON a.agentID = o.agentID
  WHERE a.agentTypeID IN (8, 11);

-- +migrate Down
-- No down migration needed — these offers were invalid
