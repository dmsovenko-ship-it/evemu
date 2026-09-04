-- Skill progression for chelobots (stage-2: bots actually level up). A bot's
-- combat/profession tier is stored so it survives respawns and can be raised by
-- practice (BotMemory::GetPractice / PracticeForNextLevel). 0xFF = not yet set
-- (legacy rows: rolled fresh on next spawn from config Min/MaxSkillLevel).
-- +migrate Up
ALTER TABLE `botMemory`
    ADD COLUMN IF NOT EXISTS `skillLevel` TINYINT UNSIGNED NOT NULL DEFAULT 255 AFTER `profession`;

-- +migrate Down
ALTER TABLE `botMemory`
    DROP COLUMN IF EXISTS `skillLevel`;
