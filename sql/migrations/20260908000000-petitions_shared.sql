-- +migrate Up
-- Shared petition system (in-game F12 petitions + portal petitions in one place).
-- portal_petitions gains the fields the game client needs; message threads live in
-- portal_petition_messages; categories/groups live in portal_petition_categories.

ALTER TABLE portal_petitions
    ADD COLUMN characterID INT UNSIGNED NOT NULL DEFAULT 0 AFTER accountID,   -- petitioner char (0 = portal author)
    ADD COLUMN categoryID   INT UNSIGNED NOT NULL DEFAULT 0 AFTER characterID, -- leaf petition category
    ADD COLUMN claimedBy    INT UNSIGNED NOT NULL DEFAULT 0 AFTER authorName,  -- GM who claimed it (0 = unclaimed)
    ADD COLUMN deleted      TINYINT(1)    NOT NULL DEFAULT 0 AFTER status,
    ADD COLUMN updated      TINYINT(1)    NOT NULL DEFAULT 0 AFTER deleted,    -- 1 = has new GM/player message since last view
    ADD COLUMN touchDate    DATETIME      NULL AFTER createDate;

CREATE TABLE IF NOT EXISTS portal_petition_messages (
    messageID  INT UNSIGNED NOT NULL AUTO_INCREMENT,
    petitionID INT UNSIGNED NOT NULL,
    senderID   INT UNSIGNED NOT NULL DEFAULT 0,     -- charID (player) or GM account/char
    senderName VARCHAR(64)  NOT NULL DEFAULT '',
    isGM       TINYINT(1)   NOT NULL DEFAULT 0,     -- 1 = GM/Petitionee message, 0 = player
    comment    TINYINT(1)   NOT NULL DEFAULT 0,     -- GM internal comment (not shown as reply)
    text       MEDIUMTEXT,
    sentDate   DATETIME     NOT NULL,
    PRIMARY KEY (messageID),
    KEY idx_petition (petitionID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Category groups/parents + leaf categories, localized. Client filters by languageID
-- == its GetLanguageID() ('ru' / 'en-us' / ...); falls back to English if it sees
-- fewer than 2 parents or no children. Seeded bilingual below.
CREATE TABLE IF NOT EXISTS portal_petition_categories (
    categoryID       INT UNSIGNED NOT NULL,          -- leaf categoryID (used on petitions)
    parentCategoryID INT UNSIGNED NOT NULL DEFAULT 0, -- group/parent id
    languageID       VARCHAR(8)   NOT NULL DEFAULT 'en-us',
    categoryName     VARCHAR(80)  NOT NULL,
    description      VARCHAR(255) NOT NULL DEFAULT '',
    sortOrder        INT          NOT NULL DEFAULT 0,
    PRIMARY KEY (categoryID, languageID),
    KEY idx_parent (parentCategoryID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- --- RU ---
INSERT INTO portal_petition_categories (categoryID, parentCategoryID, languageID, categoryName, description, sortOrder) VALUES
 (1,  0, 'ru', 'Аккаунт и оплата',    '', 1),
 (2,  0, 'ru', 'Игровой процесс',     '', 2),
 (3,  0, 'ru', 'Технические проблемы','', 3),
 (4,  0, 'ru', 'Сайт и портал',       '', 4),
 (5,  0, 'ru', 'Жалобы и нарушения',  '', 5),
 (6,  0, 'ru', 'Прочее',              '', 6),
 (100, 1, 'ru', 'Оплата и биллинг',      'Проблемы с оплатой, счетами, таймкодами.', 1),
 (101, 1, 'ru', 'Данные аккаунта',       'Смена данных, доступ, восстановление пароля.', 2),
 (102, 1, 'ru', 'Создание персонажа',    'Вопросы создания и удаления персонажей.', 3),
 (200, 2, 'ru', 'Баг в игре',            'Сообщить об ошибке игрового мира.', 1),
 (201, 2, 'ru', 'Потеря предметов/ISK',  'Пропажа кораблей, грузов, денег.', 2),
 (202, 2, 'ru', 'Застрял в космосе/на станции', 'Невозможность двигаться, варпать, стыковаться.', 3),
 (203, 2, 'ru', 'Миссии и агенты',       'Проблемы с миссиями и агентами.', 4),
 (204, 2, 'ru', 'Кража/скам',            'Мошенничество со стороны игроков.', 5),
 (300, 3, 'ru', 'Проблемы с клиентом',   'Вылеты, зависания, ошибки клиента.', 1),
 (301, 3, 'ru', 'Проблемы с соединением','Разрывы соединения, лаги.', 2),
 (400, 4, 'ru', 'Портал и сайт',         'Проблемы с веб-порталом, регистрацией, входом.', 1),
 (500, 5, 'ru', 'Оскорбления в чате',    'Оскорбительное поведение игроков.', 1),
 (501, 5, 'ru', 'Эксплойты и читы',      'Подозрение на использование читов.', 2),
 (502, 5, 'ru', 'Боты и абуз',           'Подозрение на ботоводство/абуз.', 3),
 (600, 6, 'ru', 'Общий вопрос',          'Прочие вопросы и пожелания.', 1);

-- --- EN ---
INSERT INTO portal_petition_categories (categoryID, parentCategoryID, languageID, categoryName, description, sortOrder) VALUES
 (1,  0, 'en-us', 'Account & Billing',  '', 1),
 (2,  0, 'en-us', 'Gameplay',           '', 2),
 (3,  0, 'en-us', 'Technical',          '', 3),
 (4,  0, 'en-us', 'Website & Portal',   '', 4),
 (5,  0, 'en-us', 'Harassment & Abuse', '', 5),
 (6,  0, 'en-us', 'Other',              '', 6),
 (100, 1, 'en-us', 'Billing & Payment',   'Issues with payment, invoices, time codes.', 1),
 (101, 1, 'en-us', 'Account Details',     'Account data, access, password recovery.', 2),
 (102, 1, 'en-us', 'Character Creation',  'Questions about creating/deleting characters.', 3),
 (200, 2, 'en-us', 'In-Game Bug',         'Report a bug in the game world.', 1),
 (201, 2, 'en-us', 'Lost Items/ISK',      'Missing ships, cargo, money.', 2),
 (202, 2, 'en-us', 'Stuck in Space/Station', 'Cannot move, warp or dock.', 3),
 (203, 2, 'en-us', 'Missions & Agents',   'Problems with missions and agents.', 4),
 (204, 2, 'en-us', 'Theft/Scam',          'Player fraud.', 5),
 (300, 3, 'en-us', 'Client Issues',       'Crashes, freezes, client errors.', 1),
 (301, 3, 'en-us', 'Connection Issues',   'Disconnects, lag.', 2),
 (400, 4, 'en-us', 'Portal & Website',    'Problems with the web portal, registration, login.', 1),
 (500, 5, 'en-us', 'Chat Harassment',     'Abusive player behaviour.', 1),
 (501, 5, 'en-us', 'Exploits & Cheats',   'Suspected use of cheats.', 2),
 (502, 5, 'en-us', 'Bots & Abuse',        'Suspected botting/abuse.', 3),
 (600, 6, 'en-us', 'General Question',    'Other questions and suggestions.', 1);

-- +migrate Down
ALTER TABLE portal_petitions
    DROP COLUMN touchDate,
    DROP COLUMN updated,
    DROP COLUMN deleted,
    DROP COLUMN claimedBy,
    DROP COLUMN categoryID,
    DROP COLUMN characterID;
DROP TABLE IF EXISTS portal_petition_messages;
DROP TABLE IF EXISTS portal_petition_categories;
