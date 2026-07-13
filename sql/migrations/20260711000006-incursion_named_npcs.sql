-- Replace generic Sansha NPCs with named incursion-specific typeIDs
-- +migrate Up

-- Vanguard (rooms 2100-2103): replace generic frigates/cruisers with named NPCs
UPDATE dunRoomObjects SET typeID = 2190 WHERE typeID = 10025 AND roomID BETWEEN 2100 AND 2103;  -- Renyn Meten (frigate web/tackle)
UPDATE dunRoomObjects SET typeID = 2191 WHERE typeID = 10030 AND roomID BETWEEN 2100 AND 2103;  -- Antem Neo (sniper cruiser)
UPDATE dunRoomObjects SET typeID = 2207 WHERE typeID = 11913 AND roomID BETWEEN 2100 AND 2103;  -- Vylade Dien (dmg cruiser)

-- Assault (rooms 2110-2112): replace with named cruisers/BS
UPDATE dunRoomObjects SET typeID = 2207 WHERE typeID = 10030 AND roomID BETWEEN 2110 AND 2112;  -- Vylade Dien (dmg cruiser)
UPDATE dunRoomObjects SET typeID = 2932 WHERE typeID = 11913 AND roomID BETWEEN 2110 AND 2112;  -- Ostingele Tectum (BS)
UPDATE dunRoomObjects SET typeID = 2845 WHERE typeID = 23383 AND roomID BETWEEN 2110 AND 2112;  -- Outuni Mesen (neut BS)

-- Headquarters (rooms 2120-2122): replace with named BS
UPDATE dunRoomObjects SET typeID = 2855 WHERE typeID = 11913 AND roomID BETWEEN 2120 AND 2122;  -- Intaki Colliculus (logi BS)
UPDATE dunRoomObjects SET typeID = 2932 WHERE typeID = 23383 AND roomID BETWEEN 2120 AND 2122;  -- Ostingele Tectum (dmg BS)

-- Staging (rooms 2130-2133): replace with lightweight frigates
UPDATE dunRoomObjects SET typeID = 3524 WHERE typeID = 10025 AND roomID BETWEEN 2130 AND 2133;  -- Jel Rhomben (basic frigate)
UPDATE dunRoomObjects SET typeID = 3525 WHERE typeID = 10030 AND roomID BETWEEN 2130 AND 2133;  -- Youl Meten (web frigate)

-- +migrate Down
UPDATE dunRoomObjects SET typeID = 10025 WHERE typeID IN (2190, 2966, 3524, 3525, 3526) AND roomID BETWEEN 2100 AND 2133;
UPDATE dunRoomObjects SET typeID = 10030 WHERE typeID IN (2191, 2207, 2209, 2931, 2936, 3527) AND roomID BETWEEN 2100 AND 2133;
UPDATE dunRoomObjects SET typeID = 11913 WHERE typeID IN (2845, 2855, 2932, 2933, 3484, 3485, 3486, 3487, 3490) AND roomID BETWEEN 2100 AND 2133;
UPDATE dunRoomObjects SET typeID = 23383 WHERE typeID IN (2845, 2932) AND roomID BETWEEN 2100 AND 2133;
