-- Aura mission briefings from client localization (messageIDs 145021/145024/144684/145029).
-- +migrate Up
INSERT INTO missionObjectives (missionID,briefingID,objectiveType,itemTypeID,itemQty,npcCount,note,npcFactionID,structureTypeID,structureQty,objectiveText,briefingText,destinationSystemID,destinationStationID,destinationAgentID) VALUES (58556,0,0,0,0,0,'aura',0,0,0,'Travel to the career agents and complete your basic training.','You’ve completed your basic training, {[character]player.name}. Now you’re ready for something more involving. <br><br>
There are a number of career training agents not far from here who can teach you about all the various activities and careers available to you. They’ll let you get hands-on training and experience with everything from asteroid mining to deep space exploration. <br><br>
When you’re ready, I will refer you to them. You should take everything else with you as well, since you probably won’t be coming back here afterward.',0,0,0) ON DUPLICATE KEY UPDATE objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText);
INSERT INTO missionObjectives (missionID,briefingID,objectiveType,itemTypeID,itemQty,npcCount,note,npcFactionID,structureTypeID,structureQty,objectiveText,briefingText,destinationSystemID,destinationStationID,destinationAgentID) VALUES (58557,0,0,0,0,0,'aura',0,0,0,'Board your first starship and bring it back to the station.','Hello, {[character]player.name}. At the moment, all you have to fly is a capsule. Our first priority is to get you into a starship. <br><br>

<b>Your assignment:</b> Warp to the hidden location in space and board your first starship, then use it to bring an item back here to the station. <br><br>
• Once you have boarded your starship, fly it to the end of the course. At the end of the course you will see a small Minmatar cargo rig. <br>
• You must take the <b>Tribal Sponsorship</b> document from the cargo rig. <br>
• Return to me with the document and you will have completed your assignment. <br><br> 
I’ll be providing you with tutorials along the way to help you learn how to navigate and interact with the world around you. If you ever get lost, remember that you can simply quit the mission and start over again from the beginning.',0,0,0) ON DUPLICATE KEY UPDATE objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText);
INSERT INTO missionObjectives (missionID,briefingID,objectiveType,itemTypeID,itemQty,npcCount,note,npcFactionID,structureTypeID,structureQty,objectiveText,briefingText,destinationSystemID,destinationStationID,destinationAgentID) VALUES (58558,0,0,0,0,0,'aura',0,0,0,'Fit the provided modules, fly to the training site, and destroy the target.','You’re doing well so far, {[character]player.name}. Your next test will involve some basic combat training. <br><br>
When you are ready, you’ll be provided with some modules to fit to your starship. Once you are fitted, you will fly out to a location in space to begin your combat training. <br><br> 
Once again, you will be provided with tutorials to guide you along the way.',0,0,0) ON DUPLICATE KEY UPDATE objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText);
INSERT INTO missionObjectives (missionID,briefingID,objectiveType,itemTypeID,itemQty,npcCount,note,npcFactionID,structureTypeID,structureQty,objectiveText,briefingText,destinationSystemID,destinationStationID,destinationAgentID) VALUES (58559,0,0,0,0,0,'aura',0,0,0,'Travel to the academy in the neighboring system, retrieve the Pilot Certification Documents, and return.','You’ve been learning quickly, {[character]player.name}. You’re ready to start setting your own goals soon, but before you head out on your own you should stop off at a local academy of ours. 
<br><br>

The academy is in a neighboring system, so this will also test your ability to travel between solar systems by using stargates.  At the academy you’ll be awarded your certification documents, proving that you have completed this course. <br><br>
As usual, there will be tutorials to guide you along the way.',0,0,0) ON DUPLICATE KEY UPDATE objectiveText=VALUES(objectiveText), briefingText=VALUES(briefingText);

-- +migrate Down
DELETE FROM missionObjectives WHERE missionID IN (58556,58557,58558,58559) AND note='aura';
