-- Aura tutorial pages: client pages reference localization messageIDs.
-- The table previously contained literal text, but the client calls
-- localization.GetByMessageID(page.textID).
-- +migrate Up
UPDATE tutorial_pages SET text='54064' WHERE tutorialID=110;
UPDATE tutorial_pages SET text='54086' WHERE tutorialID=112;
UPDATE tutorial_pages SET text='54091' WHERE tutorialID=117;
UPDATE tutorial_pages SET text='54068' WHERE tutorialID=136;
UPDATE tutorial_pages SET text='54002' WHERE tutorialID=137;
UPDATE tutorial_pages SET text='54103' WHERE tutorialID=204;
UPDATE tutorial_pages SET text='54089' WHERE tutorialID=210;
UPDATE tutorial_pages SET text='54102' WHERE tutorialID=212;
UPDATE tutorial_pages SET text='54098' WHERE tutorialID=215;
UPDATE tutorial_pages SET text='54001' WHERE tutorialID=217;
UPDATE tutorial_pages SET text='54003' WHERE tutorialID=221;
UPDATE tutorial_pages SET text='54090' WHERE tutorialID=222;
UPDATE tutorial_pages SET text='54017' WHERE tutorialID=224;
UPDATE tutorial_pages SET text='54049' WHERE tutorialID=226;
UPDATE tutorial_pages SET text='54056' WHERE tutorialID=228;
UPDATE tutorial_pages SET text='54077' WHERE tutorialID=230;
UPDATE tutorial_pages SET text='54021' WHERE tutorialID=231;
UPDATE tutorial_pages SET text='54060' WHERE tutorialID=232;
UPDATE tutorial_pages SET text='54021' WHERE tutorialID=233;
UPDATE tutorial_pages SET text='54087' WHERE tutorialID=234;
UPDATE tutorial_pages SET text='54059' WHERE tutorialID=249;
UPDATE tutorial_pages SET text='54087' WHERE tutorialID=250;
UPDATE tutorial_pages SET text='54057' WHERE tutorialID=251;
UPDATE tutorial_pages SET text='54078' WHERE tutorialID=252;

-- +migrate Down
-- (page text IDs stay)
