-- Remaining Epic Arcs: Amarr, Caldari, Gallente, Minmatar, AIR
-- +migrate Up

-- ====== Epic arc agents ======
REPLACE INTO `agtAgents` (`agentID`, `divisionID`, `corporationID`, `locationID`, `level`, `quality`, `agentTypeID`, `isLocator`) VALUES
(3019510, 24, 1000179, 60015054, 4, 20, 10, 0),  -- Karde Romu (Right to Rule)
(3019511, 24, 1000180, 60015067, 4, 20, 10, 0),  -- Aursa Kunivuri (Penumbra)
(3019512, 24, 1000181, 60015080, 4, 20, 10, 0),  -- Roineron Aviviere (Syndication)
(3019513, 24, 1000182, 60015093, 1, 20, 10, 0),  -- Arsten Takalo (Wildfire)
(3019514, 24, 1000179, 60015054, 1, 20, 10, 0);  -- Cassandra Ahfrim (Vision of Greatness)

-- ====== 1. Right to Rule (Amarr, lvl 4) ======
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80096, 0, 'Aiding an Investigator', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80097, 0, 'Late Reports', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80098, 0, 'The Outclassed Outpost', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80099, 0, 'Raging Sansha', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80100, 0, 'Cowardly Commander', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80101, 0, 'Aralin Jick', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80102, 0, 'Background Check', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80103, 0, 'Longing Leman', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80104, 0, 'Languishing Lord', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80105, 0, 'Razing the Outpost', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80106, 0, 'Ascending Nobles', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80107, 0, 'Hunting the Hunter', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80108, 0, 'Fate of a Madman', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
-- Ch3 branch: Amarr path
(80109, 0, 'Catching the Scent', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80110, 0, 'Falling into Place', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80111, 0, 'Making an Arrest', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80112, 0, 'An Unfortunate End', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80113, 0, 'Panic Response', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80114, 0, 'The Right to Rule', 4, 10, 0, 0, 0, 0, 0, 0, 1500000, 0, 0, 300000, 0),
-- Ch3 branch: Sansha path
(80115, 0, 'Silence Rahsa', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80116, 0, 'A Human Body', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80117, 0, 'A Metal Mind', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80118, 0, 'A Digital Soul', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80119, 0, 'Regal Replacement', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0);

-- ====== 2. Penumbra (Caldari, lvl 4) ======
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80120, 0, 'The Intermediary', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80121, 0, 'Trust And Discretion', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80122, 0, 'Their Loss, Our Profit', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80123, 0, 'The Paths That Are Hidden', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
-- Ch1 branch (Hyasyoda path)
(80124, 0, 'An Honorable Betrayal', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80125, 0, 'Proof of Intent', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80126, 0, 'Return to Isha', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80127, 0, 'Re-examining Options', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
-- Ch1 branch (Nugoeihuvi path)
(80128, 0, 'Two Steps Into Hell', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80129, 0, 'Playing It Safer', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80130, 0, 'Almost Unmasked', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
(80131, 0, 'Too Close for Comfort', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
-- Merge
(80132, 0, 'A General''s Best Friend', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80133, 0, 'Meet Sinas', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80134, 0, 'Right Tool for the Job', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80135, 0, 'The Breakout', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80136, 0, 'Whisper of a Conspiracy', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80137, 0, 'Practical Solutions', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80138, 0, 'Forewarning', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
-- Ch3 branch Hyasyoda
(80139, 0, 'The Knowledge to Act', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80140, 0, 'Slipping Away', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80141, 0, 'Across the Line', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
-- Ch3 branch Nugoeihuvi
(80142, 0, 'A Difference of Opinion', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80143, 0, 'Learning by Doing', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80144, 0, 'The Price of Silence', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80145, 0, 'Home in Peace', 4, 10, 0, 0, 0, 0, 0, 0, 750000, 0, 0, 150000, 0);

-- ====== 3. Syndication (Gallente, lvl 4) ======
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80146, 0, 'Impetus', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80147, 0, 'The Tolle Scar', 4, 10, 0, 0, 0, 0, 0, 0, 200000, 0, 0, 40000, 0),
(80148, 0, 'Priority One', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80149, 0, 'The Averon Exchange', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80150, 0, 'A Different Kind of Director', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80151, 0, 'Assistance', 4, 10, 0, 0, 0, 0, 0, 0, 300000, 0, 0, 60000, 0),
(80152, 0, 'The High or Low Road', 4, 10, 0, 0, 0, 0, 0, 0, 350000, 0, 0, 70000, 0),
-- Ch2 branch: Scoping the Scene
(80153, 0, 'Outside the Scope', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80154, 0, 'Hidden Camera', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80155, 0, 'Rendezvous', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80156, 0, 'Handoff', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80157, 0, 'With Authority', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
-- Ch2 branch: Eagle Grip
(80158, 0, 'Into the Black', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80159, 0, 'Poor Man''s Shakedown', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80160, 0, 'Underground Circus', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80161, 0, 'Intaki Chase', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
(80162, 0, 'Rat in a Corner', 4, 10, 0, 0, 0, 0, 0, 0, 400000, 0, 0, 80000, 0),
-- Ch3 merge
(80163, 0, 'Places to Hide', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80164, 0, 'Carry On', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80165, 0, 'Studio I', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80166, 0, 'Showtime', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
(80167, 0, 'Where''s the Line?', 4, 10, 0, 0, 0, 0, 0, 0, 500000, 0, 0, 100000, 0),
-- Final branch
(80168, 0, 'Everybody Has a Price', 4, 10, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 200000, 0),
(80169, 0, 'Safe Return', 4, 10, 0, 0, 0, 0, 0, 0, 1000000, 0, 0, 200000, 0);

-- ====== 4. Wildfire (Minmatar, lvl 1) ======
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80170, 0, 'A Demonstration', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80171, 0, 'The Cost of Preservation', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80172, 0, 'Written By The Victors', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80173, 0, 'Glowing Embers', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80174, 0, 'From Way Above', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80175, 0, 'Friends In High Places', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80176, 0, 'My Little Eye', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80177, 0, 'Dead End Intercept', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80178, 0, 'Surfacing', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80179, 0, 'Who Art in Heaven', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80180, 0, 'Playing All Their Cards', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80181, 0, 'History In The Making', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80182, 0, 'Church Of The Obsidian', 1, 10, 0, 0, 0, 0, 0, 0, 25000, 0, 0, 5000, 0),
(80183, 0, 'Heresiology', 1, 10, 0, 0, 0, 0, 0, 0, 25000, 0, 0, 5000, 0),
(80184, 0, 'Wildfire', 1, 10, 0, 0, 0, 0, 0, 0, 25000, 0, 0, 5000, 0),
(80185, 0, 'Stillwater', 1, 10, 0, 0, 0, 0, 0, 0, 25000, 0, 0, 5000, 0),
(80186, 0, 'With Great Power', 1, 10, 0, 0, 0, 0, 0, 0, 25000, 0, 0, 5000, 0),
-- Final branch
(80187, 0, 'Revelation', 1, 10, 0, 0, 0, 0, 0, 0, 50000, 0, 0, 10000, 0),
(80188, 0, 'Retraction', 1, 10, 0, 0, 0, 0, 0, 0, 50000, 0, 0, 10000, 0);

-- ====== 5. Vision of Greatness (AIR, lvl 1) ======
INSERT INTO `agtMissions` (`id`, `briefingID`, `name`, `level`, `typeID`, `important`, `storyline`, `raceID`, `constellationID`, `corporationID`, `dungeonID`, `rewardISK`, `rewardItemID`, `rewardItemQty`, `bonusISK`, `bonusTime`) VALUES
(80189, 0, 'Holographic Reenactment', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80190, 0, 'Caldari Prime Breakout', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80191, 0, 'Travel to Minmatar', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80192, 0, 'Battle of Pator', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80193, 0, 'Travel to Amarr', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80194, 0, 'Golgothan Fields', 1, 10, 0, 0, 0, 0, 0, 0, 20000, 0, 0, 4000, 0),
(80195, 0, 'Travel to the Gallente', 1, 10, 0, 0, 0, 0, 0, 0, 15000, 0, 0, 3000, 0),
(80196, 0, 'Intaki Liberation', 1, 10, 0, 0, 0, 0, 0, 0, 50000, 0, 0, 10000, 0);

-- ====== Epic arc definitions ======
REPLACE INTO `epicArc` (`arcID`, `arcName`, `factionID`, `description`, `cooldownDays`, `startingAgentID`, `startingSystemID`, `level`) VALUES
(5, 'Right to Rule', 500003, 'Amarr Empire Epic Arc — Uncover a conspiracy in Kor-Azor.', 90, 3019510, 30005217, 4),
(6, 'Penumbra', 500001, 'Caldari State Epic Arc — Corporate intrigue in Josameto.', 90, 3019511, 30005217, 4),
(7, 'Syndication', 500004, 'Gallente Federation Epic Arc — Crime and corruption in Dodixie.', 90, 3019512, 30005217, 4),
(8, 'Wildfire', 500002, 'Minmatar Republic Epic Arc — Tribal conflict in Frarn.', 90, 3019513, 30005217, 1),
(9, 'Vision of Greatness', 500020, 'AIR Epic Arc — Relive historic battles via holographic reenactment.', 90, 3019514, 30005217, 1);

-- ====== Chapters ======
REPLACE INTO `epicArcChapter` (`arcID`, `chapterNumber`, `chapterName`) VALUES
(5, 1, 'Interference'),
(5, 2, 'Spiderweb Politics'),
(5, 3, 'The Old Guard'),
(6, 1, 'Proof of Intent'),
(6, 2, 'Blackness Rising'),
(6, 3, 'The Bottom Line'),
(7, 1, 'Impetus'),
(7, 2, 'Scoping The Scene'),
(7, 3, 'Inertia'),
(8, 1, 'The Passage'),
(8, 2, 'Songs of the Past'),
(8, 3, 'Revelation'),
(9, 1, 'Vision of Greatness');

-- ====== Mission mappings ======
INSERT INTO `epicArcMission` (`arcID`, `chapterNumber`, `sequenceNumber`, `missionID`, `missionName`, `branchID`, `rewardISK`, `rewardStanding`) VALUES
-- Right to Rule (arc 5)
(5, 1, 1, 80096, 'Aiding an Investigator', 0, 200000, 0),
(5, 1, 2, 80097, 'Late Reports', 0, 200000, 0),
(5, 1, 3, 80098, 'The Outclassed Outpost', 0, 300000, 0),
(5, 1, 4, 80099, 'Raging Sansha', 0, 300000, 0),
(5, 1, 5, 80100, 'Cowardly Commander', 0, 300000, 0),
(5, 2, 6, 80101, 'Aralin Jick', 0, 350000, 0),
(5, 2, 7, 80102, 'Background Check', 0, 350000, 0),
(5, 2, 8, 80103, 'Longing Leman', 0, 350000, 0),
(5, 2, 9, 80104, 'Languishing Lord', 0, 350000, 0),
(5, 2, 10, 80105, 'Razing the Outpost', 0, 400000, 0),
(5, 2, 11, 80106, 'Ascending Nobles', 0, 400000, 0),
(5, 2, 12, 80107, 'Hunting the Hunter', 0, 400000, 0),
(5, 2, 13, 80108, 'Fate of a Madman', 0, 400000, 0),
(5, 3, 14, 80109, 'Catching the Scent', 1, 500000, 0),
(5, 3, 15, 80110, 'Falling into Place', 1, 500000, 0),
(5, 3, 16, 80111, 'Making an Arrest', 1, 500000, 0),
(5, 3, 17, 80112, 'An Unfortunate End', 1, 500000, 0),
(5, 3, 18, 80113, 'Panic Response', 1, 500000, 0),
(5, 3, 19, 80114, 'The Right to Rule', 1, 1500000, 0.1),
(5, 3, 14, 80115, 'Silence Rahsa', 2, 500000, 0),
(5, 3, 15, 80116, 'A Human Body', 2, 500000, 0),
(5, 3, 16, 80117, 'A Metal Mind', 2, 500000, 0),
(5, 3, 17, 80118, 'A Digital Soul', 2, 500000, 0),
(5, 3, 18, 80119, 'Regal Replacement', 2, 500000, 0),
-- Penumbra (arc 6)
(6, 1, 1, 80120, 'The Intermediary', 0, 200000, 0),
(6, 1, 2, 80121, 'Trust And Discretion', 0, 200000, 0),
(6, 1, 3, 80122, 'Their Loss, Our Profit', 0, 300000, 0),
(6, 1, 4, 80123, 'The Paths That Are Hidden', 0, 300000, 0),
(6, 1, 5, 80124, 'An Honorable Betrayal', 1, 350000, 0),
(6, 1, 6, 80125, 'Proof of Intent', 1, 350000, 0),
(6, 1, 7, 80126, 'Return to Isha', 1, 350000, 0),
(6, 1, 8, 80127, 'Re-examining Options', 1, 350000, 0),
(6, 1, 5, 80128, 'Two Steps Into Hell', 2, 350000, 0),
(6, 1, 6, 80129, 'Playing It Safer', 2, 350000, 0),
(6, 1, 7, 80130, 'Almost Unmasked', 2, 350000, 0),
(6, 1, 8, 80131, 'Too Close for Comfort', 2, 350000, 0),
(6, 1, 9, 80132, 'A General''s Best Friend', 0, 400000, 0),
(6, 2, 10, 80133, 'Meet Sinas', 0, 400000, 0),
(6, 2, 11, 80134, 'Right Tool for the Job', 0, 400000, 0),
(6, 2, 12, 80135, 'The Breakout', 0, 400000, 0),
(6, 2, 13, 80136, 'Whisper of a Conspiracy', 0, 400000, 0),
(6, 2, 14, 80137, 'Practical Solutions', 0, 400000, 0),
(6, 2, 15, 80138, 'Forewarning', 0, 400000, 0),
(6, 3, 16, 80139, 'The Knowledge to Act', 1, 500000, 0),
(6, 3, 17, 80140, 'Slipping Away', 1, 500000, 0),
(6, 3, 18, 80141, 'Across the Line', 1, 500000, 0),
(6, 3, 16, 80142, 'A Difference of Opinion', 2, 500000, 0),
(6, 3, 17, 80143, 'Learning by Doing', 2, 500000, 0),
(6, 3, 18, 80144, 'The Price of Silence', 2, 500000, 0),
(6, 3, 19, 80145, 'Home in Peace', 0, 750000, 0.1),
-- Syndication (arc 7)
(7, 1, 1, 80146, 'Impetus', 0, 200000, 0),
(7, 1, 2, 80147, 'The Tolle Scar', 0, 200000, 0),
(7, 1, 3, 80148, 'Priority One', 0, 300000, 0),
(7, 1, 4, 80149, 'The Averon Exchange', 0, 300000, 0),
(7, 1, 5, 80150, 'A Different Kind of Director', 0, 300000, 0),
(7, 1, 6, 80151, 'Assistance', 0, 300000, 0),
(7, 1, 7, 80152, 'The High or Low Road', 0, 350000, 0),
(7, 2, 8, 80153, 'Outside the Scope', 1, 400000, 0),
(7, 2, 9, 80154, 'Hidden Camera', 1, 400000, 0),
(7, 2, 10, 80155, 'Rendezvous', 1, 400000, 0),
(7, 2, 11, 80156, 'Handoff', 1, 400000, 0),
(7, 2, 12, 80157, 'With Authority', 1, 400000, 0),
(7, 2, 8, 80158, 'Into the Black', 2, 400000, 0),
(7, 2, 9, 80159, 'Poor Man''s Shakedown', 2, 400000, 0),
(7, 2, 10, 80160, 'Underground Circus', 2, 400000, 0),
(7, 2, 11, 80161, 'Intaki Chase', 2, 400000, 0),
(7, 2, 12, 80162, 'Rat in a Corner', 2, 400000, 0),
(7, 3, 13, 80163, 'Places to Hide', 0, 500000, 0),
(7, 3, 14, 80164, 'Carry On', 0, 500000, 0),
(7, 3, 15, 80165, 'Studio I', 0, 500000, 0),
(7, 3, 16, 80166, 'Showtime', 0, 500000, 0),
(7, 3, 17, 80167, 'Where''s the Line?', 0, 500000, 0),
(7, 3, 18, 80168, 'Everybody Has a Price', 1, 1000000, 0.1),
(7, 3, 18, 80169, 'Safe Return', 2, 1000000, 0.1),
-- Wildfire (arc 8)
(8, 1, 1, 80170, 'A Demonstration', 0, 15000, 0),
(8, 1, 2, 80171, 'The Cost of Preservation', 0, 15000, 0),
(8, 1, 3, 80172, 'Written By The Victors', 0, 15000, 0),
(8, 1, 4, 80173, 'Glowing Embers', 0, 15000, 0),
(8, 1, 5, 80174, 'From Way Above', 0, 15000, 0),
(8, 1, 6, 80175, 'Friends In High Places', 0, 15000, 0),
(8, 1, 7, 80176, 'My Little Eye', 0, 15000, 0),
(8, 2, 8, 80177, 'Dead End Intercept', 0, 20000, 0),
(8, 2, 9, 80178, 'Surfacing', 0, 20000, 0),
(8, 2, 10, 80179, 'Who Art in Heaven', 0, 20000, 0),
(8, 2, 11, 80180, 'Playing All Their Cards', 0, 20000, 0),
(8, 2, 12, 80181, 'History In The Making', 0, 20000, 0),
(8, 3, 13, 80182, 'Church Of The Obsidian', 0, 25000, 0),
(8, 3, 14, 80183, 'Heresiology', 0, 25000, 0),
(8, 3, 15, 80184, 'Wildfire', 0, 25000, 0),
(8, 3, 16, 80185, 'Stillwater', 0, 25000, 0),
(8, 3, 17, 80186, 'With Great Power', 0, 25000, 0),
(8, 3, 18, 80187, 'Revelation', 1, 50000, 0.1),
(8, 3, 18, 80188, 'Retraction', 2, 50000, 0.05),
-- Vision of Greatness (arc 9)
(9, 1, 1, 80189, 'Holographic Reenactment', 0, 15000, 0),
(9, 1, 2, 80190, 'Caldari Prime Breakout', 0, 15000, 0),
(9, 1, 3, 80191, 'Travel to Minmatar', 0, 15000, 0),
(9, 1, 4, 80192, 'Battle of Pator', 0, 20000, 0),
(9, 1, 5, 80193, 'Travel to Amarr', 0, 15000, 0),
(9, 1, 6, 80194, 'Golgothan Fields', 0, 20000, 0),
(9, 1, 7, 80195, 'Travel to the Gallente', 0, 15000, 0),
(9, 1, 8, 80196, 'Intaki Liberation', 0, 50000, 0);

-- +migrate Down
DELETE FROM `epicArcMission` WHERE `arcID` IN (5,6,7,8,9);
DELETE FROM `epicArcChapter` WHERE `arcID` IN (5,6,7,8,9);
DELETE FROM `epicArc` WHERE `arcID` IN (5,6,7,8,9);
DELETE FROM `agtMissions` WHERE `id` BETWEEN 80096 AND 80196;
DELETE FROM `agtAgents` WHERE `agentID` IN (3019510, 3019511, 3019512, 3019513, 3019514);
