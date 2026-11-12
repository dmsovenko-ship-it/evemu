-- Remaining COSMOS item missions with wiki briefings and hand-in semantics.
-- +migrate Up

UPDATE missionObjectives SET objectiveType=5, briefingText='With political relations between the Ammatar and the Minmatar Republic quickly deteriorating, things look dire for our citizens across the border. There are thousands of Nefantar inhabitants stuck in the Tartatven constellation, some who are working there on official business, others who simply settled there years ago. The Republic government refuses to let the majority of them leave their sovereign space and, in fact, has many under constant surveillance. 
We fear that their lives may be at risk should the Ammatar declare war on the Republic and invade Audesder. There''s already a huge buildup of military forces here in Kenobanala, and many fear that war is inevitable. So it''s my task to ensure the safe return of as many Nefantar to Ammatar space as possible. 
What I need you to do is deliver a group of Nefantar refugees from Aeddin II - Imperial Armaments Warehouse to Fort Kumar. I will then relocate them to one of our settlements in Joas. 
May God light your path.' WHERE missionID=93094;
UPDATE missionObjectives SET objectiveType=5, briefingText='You have done well . But our task is not finished. There are still thousands of Nefantars that need to be moved. If you have more time on your hands. I''d like you to make a few more runs to Aedald for me.' WHERE missionID=93095;
UPDATE missionObjectives SET objectiveType=5, briefingText='Ahem. I''m really sorry if I insulted you, , but there''s nothing I can do about this. I was given the order to test EVERYONE before we send them out into the fray, and if I would ignore that order I risk being demoted. I hope you understand. Minas is very strict about us following his orders.
Now you''ll find the details of your test assignment in the briefing which I''m about to load into your NeoCom. Complete the assignment. which I''m sure you''ll do in no time. and I''ll send you off to the general. And don''t forget to read the advice I''ve uploaded into your journal. Good luck.' WHERE missionID=93534;

-- +migrate Down
-- (briefings/objectives stay)
