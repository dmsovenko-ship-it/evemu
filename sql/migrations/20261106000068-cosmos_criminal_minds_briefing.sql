-- Criminal Minds - The Investigation (2 of 4): SDE briefing.
-- +migrate Up
UPDATE missionObjectives SET briefingText='/h2> Dammit, dammit, dammit! Once again, the narcs are causing me bloody trouble. My inside man just told me that they launched an official investigation against my operation. They''re on to me and are trying to figure out what it is that I actually do here. This is bad news for us, you. We have to get them to close their investigation. I´ve got an idea how to solve this mess, though. If we co-operate with them and get them to think we have nothing to hide, maybe they''ll go away. Take these reports to the destination. They contain fake data on all our assets, journals, and transaction logs. If this isn''t enough to convince them we''re not up to anything, I don''t know what will. </span' WHERE missionID=93451 AND (briefingText IS NULL OR briefingText='');

-- +migrate Down
-- (briefing stays)
