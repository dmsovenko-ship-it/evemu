-- Market self-learning for trader bots (stage-1 economy). Each trader remembers
-- how its market-making / arbitrage fills went so it can widen its margin after
-- losses and tighten it after profits (BotMemory::RecordTradeResult / GetTradeConfidence).
-- +migrate Up
ALTER TABLE `botMemory`
    ADD COLUMN IF NOT EXISTS `tradeProfit` BIGINT NOT NULL DEFAULT 0 AFTER `profession`,
    ADD COLUMN IF NOT EXISTS `tradeLosses` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `tradeProfit`;

-- +migrate Down
ALTER TABLE `botMemory`
    DROP COLUMN IF EXISTS `tradeProfit`,
    DROP COLUMN IF EXISTS `tradeLosses`;
