-- Career agent mission content: tutorial pages, criteria, and rewards
-- Based on official EVE career agent chains
-- +migrate Up

-- Tutorial completion tracking
CREATE TABLE IF NOT EXISTS `characterTutorialState` (
  `characterID` int(11) NOT NULL DEFAULT 0,
  `tutorialID` int(11) NOT NULL DEFAULT 0,
  `pageID` int(11) NOT NULL DEFAULT 0,
  `completed` tinyint(1) NOT NULL DEFAULT 0,
  `completedDateTime` bigint(20) NOT NULL DEFAULT 0,
  PRIMARY KEY (`characterID`,`tutorialID`,`pageID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Tutorial reward definitions
CREATE TABLE IF NOT EXISTS `tutorial_rewards` (
  `tutorialID` int(11) NOT NULL DEFAULT 0,
  `pageID` int(11) NOT NULL DEFAULT 0,
  `iskAmount` bigint(20) NOT NULL DEFAULT 0,
  `typeID` int(11) NOT NULL DEFAULT 0,
  `quantity` int(11) NOT NULL DEFAULT 0,
  `skillTypeID` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`tutorialID`,`pageID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Tutorial categories (structure from SDE)

INSERT IGNORE INTO `tutorial_categories` (`categoryID`, `categoryName`, `description`) VALUES
(1, 'Tutorial', 'Core tutorial system'),
(2, 'Career', 'Career agent missions'),
(3, 'Industrial', 'Industrial career path'),
(4, 'Business', 'Business career path'),
(5, 'Exploration', 'Exploration career path');

-- Tutorials, tutorial_pages, tutorial_criteria, tutorial_page_criteria, tutorials_criterias — from SDE

-- ============================================================
-- Cash Flow for Capsuleers — Caldari Military career (10 missions)
-- ============================================================
INSERT IGNORE INTO `tutorials` (`tutorialID`, `tutorialName`, `nextTutorialID`, `categoryID`) VALUES
(1001, 'Cash Flow: Mission 1 - Clear the Pirates', 1002, 2),
(1002, 'Cash Flow: Mission 2 - Rescue the Miner', 1003, 2),
(1003, 'Cash Flow: Mission 3 - Secret Documents', 1004, 2),
(1004, 'Cash Flow: Mission 4 - Approach the Stargate', 1005, 2),
(1005, 'Cash Flow: Mission 5 - Pirate Meet', 1006, 2),
(1006, 'Cash Flow: Mission 6 - Destroy the Outpost', 1007, 2),
(1007, 'Cash Flow: Mission 7 - Convoy Ambush', 1008, 2),
(1008, 'Cash Flow: Mission 8 - Retrieve the VIPs', 1009, 2),
(1009, 'Cash Flow: Mission 9 - Narcotics Warehouse', 1010, 2),
(1010, 'Cash Flow: Mission 10 - Tahamar', 0, 2);

-- Pages for each tutorial
INSERT IGNORE INTO `tutorial_pages` (`pageID`, `tutorialID`, `pageNumber`, `pageName`, `text`, `imagePath`, `audioPath`) VALUES
(2001, 1001, 1, 'Clear the Pirates',
 'Your agent has requested you clear pirates from the asteroid belt on behalf of a group of miners. Warp to the deadspace location and destroy all pirate vessels. Use your weapons to engage the enemies - start with the closest single ship, then work through the groups.',
 '', ''),
(2002, 1002, 1, 'Rescue the Miner',
 'Eliminate the pirates and rescue the civilian miner they are holding as a captive. The civilian is in a cargo container attached to one of the pirate wrecks. Destroy all hostiles then loot the container to recover the civilian. Return to your agent when done.',
 '', ''),
(2003, 1003, 1, 'Secret Documents',
 'Destroy the pirates and loot the secret documents from the container they leave behind. Use your stasis webifier to slow down the fast drone targets. One wreck will have a cargo container - loot it to retrieve the documents.',
 '', ''),
(2004, 1004, 1, 'Approach the Stargate',
 'Warp to the deadspace area and approach the stargate. This mission teaches you about shield tanking - the stargate deals damage to your shields to demonstrate the mechanic. You do not need to destroy anything. Get close enough for the journal update then return to station.',
 '', ''),
(2005, 1005, 1, 'Pirate Meet',
 'Meet the pirate at the repair outpost. Follow his instructions - he will taunt you and wait for you to attack. Destroy his ship and report back to your agent.',
 '', ''),
(2006, 1006, 1, 'Destroy the Outpost',
 'Destroy Tahamar\'s Outpost. Warp to the deadspace location, destroy all guarding pirates, then activate the acceleration gate. In the next room, destroy the outpost structure. Watch for spawning reinforcements when you attack the outpost.',
 '', ''),
(2007, 1007, 1, 'Convoy Ambush',
 'Destroy the pirates at the convoy ambush site. This is a tough fight with 11 pirates total arriving in waves. Use your shield booster and be ready to warp out if your shields get low - you can return to continue the fight where you left off.',
 '', ''),
(2008, 1008, 1, 'Retrieve the VIPs',
 'Fly to the hotel to retrieve the VIPs. However, something is amiss - the hotel is destroyed and you take heavy damage upon arrival. Warp out immediately and report back to your agent with the bad news.',
 '', ''),
(2009, 1009, 1, 'Narcotics Warehouse',
 'Destroy the narcotics warehouse. Warp in, defeat the guarding pirates, activate the acceleration gate, fight through more pirates in the next room, then destroy the warehouse structure. It does not fight back but takes time to destroy.',
 '', ''),
(2010, 1010, 1, 'Find and Destroy Tahamar',
 'Find Tahamar and take him out once and for all. This mission takes you to another system. Use your journal to set destination. Navigate through three deadspace rooms - destroy guard ships in each, activate acceleration gates, and in the final room destroy Tahamar and his stasis tower.',
 '', '');

-- Completion criteria (generic type: kill/loot/approach)
INSERT IGNORE INTO `tutorial_criteria` (`criteriaID`, `criteriaName`, `messageText`, `audioPath`) VALUES
(1, 'Destroy Ships', 'Destroy the target ships', ''),
(2, 'Loot Item', 'Loot the required item from the wreck', ''),
(3, 'Approach Object', 'Approach the target object', ''),
(4, 'Navigate Gate', 'Activate the acceleration gate', '');

INSERT IGNORE INTO `tutorials_criterias` (`tutorialID`, `criteriaID`) VALUES
(1001, 1), (1002, 1), (1002, 2), (1003, 1), (1003, 2),
(1004, 3), (1005, 1), (1006, 1), (1006, 4), (1007, 1),
(1008, 3), (1009, 1), (1009, 4), (1010, 1), (1010, 4);

-- Rewards for each tutorial completion
INSERT IGNORE INTO `tutorial_rewards` (`tutorialID`, `pageID`, `iskAmount`, `typeID`, `quantity`, `skillTypeID`) VALUES
(1001, 2001, 20000, 0, 0, 0),
(1002, 2002, 53000, 0, 0, 0),
(1003, 2003, 81000, 0, 0, 0),
(1004, 2004, 33000, 0, 0, 0),
(1005, 2005, 26000, 0, 0, 0),
(1006, 2006, 59000, 0, 0, 0),
(1007, 2007, 68000, 0, 0, 0),
(1008, 2008, 39000, 0, 0, 0),
(1009, 2009, 54000, 0, 0, 0),
(1010, 2010, 148000, 0, 0, 0);

-- +migrate Down
DELETE FROM `tutorial_rewards` WHERE `tutorialID` BETWEEN 1001 AND 1010;
DELETE FROM `tutorials_criterias` WHERE `tutorialID` BETWEEN 1001 AND 1010;
DELETE FROM `tutorial_criteria` WHERE `criteriaID` BETWEEN 1 AND 4;
DELETE FROM `tutorial_page_criteria` WHERE `pageID` BETWEEN 2001 AND 2010;
DELETE FROM `tutorial_pages` WHERE `tutorialID` BETWEEN 1001 AND 1010;
DELETE FROM `tutorials` WHERE `tutorialID` BETWEEN 1001 AND 1010;
DELETE FROM `tutorial_categories` WHERE `categoryID` BETWEEN 1 AND 5;
DROP TABLE IF EXISTS `characterTutorialState`;
DROP TABLE IF EXISTS `tutorial_rewards`;
