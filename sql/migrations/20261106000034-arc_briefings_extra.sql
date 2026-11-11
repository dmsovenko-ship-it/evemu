-- Remaining arc mission briefings (alternate wiki titles).
-- +migrate Up
UPDATE missionObjectives SET briefingText='We got some interesting information out of your trip. It seems no one fired on the Damsel. Something must have happened inside the ship. We''d like you to track those survivors down, see if you can get any information about the ship''s path, pilot, or cargo. We need to know what caused this disaster. CONCORD is the first logical step. I''ve gotten in touch with CONCORD Agent Tevis Jak, who may have information for you. His location is being uploaded to your NeoCom: he''s currently acting as an ORE agent. Go see if you can find a piece of this puzzle.' WHERE missionID=80001;
-- +migrate Down
-- (briefings stay)
