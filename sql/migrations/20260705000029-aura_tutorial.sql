-- Aura tutorial chain — 4 missions: Covering the Basics, Combat Basics, The Academy, Moving Onwards
-- tutorialID range 215-233 (matches tutorialsvc_connections data)
-- +migrate Up

-- Main Aura tutorial chain
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`) VALUES
(215, 'Covering the Basics (part 1)', 217, 1),
(217, 'Covering the Basics (part 2)', 136, 1),
(136, 'Camera Movement', 249, 1),
(249, 'Camera Movement (cont)', 110, 1),
(110, 'Basic Commands', 137, 1),
(137, 'Ship Movement', 204, 1),
(204, 'Looting', 117, 1),
(117, 'Mission 1 Complete', 250, 1),
(250, 'Continue', 234, 1),
(234, 'Your Next Mission', 226, 1),
(226, 'Fitting your ship', 224, 1),
(224, 'Undocking Instantly', 228, 1),
(228, 'Weapons: Hybrids', 210, 1),
(210, 'Target Practice', 112, 1),
(112, 'Basic Combat', 222, 1),
(222, 'Mission 2 Complete', 251, 1),
(251, 'Continue (2)', 221, 1),
(221, 'Interstellar Travel', 212, 1),
(212, 'Inside the Academy', 230, 1),
(230, 'The Mission Journal', 231, 1),
(231, 'Further Training', 232, 1),
(232, 'Moving Onwards', 252, 1),
(252, 'Continue (3)', 233, 1),
(233, 'Career Agents', 0, 1);

-- Pages for each tutorial
INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(3001, 215, 1, 'Welcome',
 'Welcome to EVE Online. This tutorial will teach you the basics. Accept the mission from Aura and prepare to undock.', '', ''),
(3002, 217, 1, 'Undock',
 'Click the Undock button (>>> or the station services panel) to leave the station and enter space.', '', ''),
(3003, 136, 1, 'Camera Controls',
 'Use your mouse to rotate the camera view. Hold the right mouse button and drag to look around. Use the mouse wheel to zoom in and out.', '', ''),
(3004, 249, 1, 'Camera Practice',
 'Practice moving your camera around your ship. Find the ship floating nearby and board it.', '', ''),
(3005, 110, 1, 'Basic Commands',
 'Right-click on objects in space to see available commands. Use the Overview panel on the right to select targets.', '', ''),
(3006, 137, 1, 'Ship Movement',
 'Use the Overview panel to find the Acceleration Gate. Right-click it and select Activate Gate to warp to the next area.', '', ''),
(3007, 204, 1, 'Looting',
 'After warping through the gate, find the cargo container near the station. Right-click it and select Open Cargo. Drag the Pilot License item into your ship cargo hold.', '', ''),
(3008, 117, 1, 'Mission Complete',
 'Return to the station and dock. Open the agent conversation and click Complete Mission. Then click Request Mission to continue.', '', ''),
(3009, 250, 1, 'Next Steps',
 'Well done pilot. Your next mission will introduce combat. Make sure you have trained your skills as prompted by Aura.', '', ''),
(3010, 234, 1, 'Your Next Mission',
 'Your agent has more work for you. Train the skills Aura recommends and accept the next mission when ready.', '', ''),
(3011, 226, 1, 'Fitting Your Ship',
 'Open the Fitting window from the Neocom. Drag your weapon from the Item Hangar to a turret slot on your ship. Make sure you have ammunition loaded.', '', ''),
(3012, 224, 1, 'Undocking',
 'Click the Undock button to leave the station. You are now ready for combat training.', '', ''),
(3013, 228, 1, 'Weapons Training',
 'Your ship is fitted with hybrid weapons. When you target an enemy and activate your weapon, it will automatically fire. Load your ammunition before engaging.', '', ''),
(3014, 210, 1, 'Target Practice',
 'Find the Acceleration Gate in your Overview and activate it to enter the combat area. A Target Fuel Depot will be in front of you - destroy it with your weapons.', '', ''),
(3015, 112, 1, 'Basic Combat',
 'Enemy ships will attack you. Lock them by right-clicking and selecting Lock Target. Activate your weapons to return fire. Destroy both waves of enemies.', '', ''),
(3016, 222, 1, 'Combat Complete',
 'Return to the station and dock. Complete the mission with your agent to receive your rewards.', '', ''),
(3017, 251, 1, 'Continue Training',
 'Your combat training is complete. More advanced training awaits.', '', ''),
(3018, 221, 1, 'Interstellar Travel',
 'You must travel to another system. Open the mission window and right-click the location text to Set Destination. Follow the yellow route through stargates.', '', ''),
(3019, 212, 1, 'Inside the Academy',
 'Warp to the academy location. Find the academy office in your Overview, open its cargo, and retrieve the Pilot Certification Documents.', '', ''),
(3020, 230, 1, 'The Mission Journal',
 'Open your Journal (F12 or Neocom) to view your mission history. This is where you can track all active and completed missions.', '', ''),
(3021, 231, 1, 'Further Training',
 'Your agent recommends additional training. Consider exploring the career agents for specialized profession training.', '', ''),
(3022, 232, 1, 'Moving Onwards',
 'Travel to your new home station. This is your final destination. Dock at the station and complete the mission with your agent.', '', ''),
(3023, 252, 1, 'Final Steps',
 'You have arrived at your new home. From here you can access career agents and continue your journey in New Eden.', '', ''),
(3024, 233, 1, 'Career Agents',
 'Congratulations! You have completed the basic training. Visit the Career Agents to learn about professions: Military, Industry, Business, Exploration, and Advanced Military.', '', '');

INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(117, 3008, 45000, 0, 0, 0),
(222, 3016, 33000, 0, 0, 0),
(232, 3022, 50000, 0, 0, 0),
(233, 3024, 10000, 0, 0, 0);

-- +migrate Down
DELETE FROM `tutorial_rewards` WHERE `tutorialID` BETWEEN 215 AND 233;
DELETE FROM `tutorial_pages` WHERE `tutorialID` BETWEEN 215 AND 233;
DELETE FROM `tutorials` WHERE `tutorialID` BETWEEN 215 AND 233;
