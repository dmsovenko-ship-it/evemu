-- +migrate Up
-- MarketTops (portal) aggregates mktTransactions (438k rows) four times:
--   GROUP BY typeID       WHERE transactionType = {0,1}
--   GROUP BY characterID  WHERE transactionType = {0,1}   (SUM(quantity*price))
-- Single-column indexes force a full pass + temp table per query (~2s each,
-- 8.8s for the endpoint). Covering composites let the engine range-scan and
-- aggregate straight from the index.
CREATE INDEX IF NOT EXISTS idx_mkttx_type_cov ON mktTransactions (transactionType, typeID, quantity, price);
CREATE INDEX IF NOT EXISTS idx_mkttx_char_cov ON mktTransactions (transactionType, characterID, quantity, price);

-- +migrate Down
DROP INDEX IF EXISTS idx_mkttx_type_cov ON mktTransactions;
DROP INDEX IF EXISTS idx_mkttx_char_cov ON mktTransactions;
