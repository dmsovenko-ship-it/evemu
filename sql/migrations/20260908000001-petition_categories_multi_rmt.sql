-- +migrate Up
-- Petition categories for the two biggest live-EVE problem areas (shared by the
-- in-game F12 wizard and the web portal): multiboxing/botting and RMT. They are
-- leaves under the "Harassment & Abuse / Жалобы и нарушения" group (5).
INSERT IGNORE INTO portal_petition_categories (categoryID, parentCategoryID, languageID, categoryName, description, sortOrder) VALUES
    (601, 5, 'ru',     'Мультиаккаунтинг и боты',       'Многократный вход, автоматизация, ботоводство.', 0),
    (601, 5, 'en-us',  'Multiboxing & Botting',         'Multiple accounts, automation, botting.', 0),
    (602, 5, 'ru',     'RMT (продажа ISK за деньги)',   'Вывод/ввод игровой валюты за реальные деньги.', 0),
    (602, 5, 'en-us',  'RMT (selling ISK for money)',   'Trading in-game currency for real money.', 0);
-- +migrate Down
DELETE FROM portal_petition_categories WHERE categoryID IN (601, 602);
