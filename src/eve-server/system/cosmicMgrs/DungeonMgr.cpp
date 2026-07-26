 /**
  * @name DungeonMgr.cpp
  *     Dungeon management system for EVEmu
  *
  * @Author:        James
  * @date:          13 December 2022
  */
 
#include "eve-server.h"

#include "EVEServerConfig.h"

#include "StaticDataMgr.h"
#include "dungeon/DungeonDB.h"
#include "inventory/Inventory.h"
#include "inventory/InventoryItem.h"
#include "system/SystemBubble.h"
#include "system/SystemEntity.h"
#include "system/cosmicMgrs/SpawnMgr.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "system/cosmicMgrs/BeltMgr.h"
#include "system/cosmicMgrs/DungeonMgr.h"

/*
Dungeon flow:
1. load dungeons from db on init
2. when signature/anomaly is created, spawn first room of dungeon at ship position (how to handle multi-room dungeons?)

Multi-room dungeons have each room with acceleration gate pointing to next room (accel gate is not implemented????)

*/


DungeonDataMgr::DungeonDataMgr()
    : m_dungeonID(0)
{
    m_dungeons.clear();
}

int DungeonDataMgr::Initialize()
{
    // Loads all dungeons from database upon server initialisation
    Populate();
    sLog.Blue("       DunDataMgr", "Dungeon Data Manager Initialized.");
    return 1;
}

void DungeonDataMgr::UpdateDungeon(uint32 dungeonID)
{
    _log(DUNG__INFO, "UpdateDungeon() - Updating dungeon %u's object in DataMgr...", dungeonID);
    // Get dungeon from DB by dungeonID
    DBQueryResult *res = new DBQueryResult();
    DBResultRow row;
    DungeonDB::GetAllDungeonDataByDungeonID(*res, dungeonID);

    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    // Update a dungeon's in-memory object
    auto it = byDungeonID.find(dungeonID);
    if (it != byDungeonID.end()) {
        // Dungeon already exists, update in-memory object
        byDungeonID.erase(dungeonID);

        if (res->GetRowCount() > 0) {
            while (res->GetRow(row))
            {
                CreateDungeon(row);
                FillObject(row);
            }
        } else {
            _log(DUNG__ERROR, "UpdateDungeon() - Failed to find dungeon %u in database. This should never happen.", dungeonID);
        }

    } else {
        _log(DUNG__ERROR, "UpdateDungeon() - Failed to find dungeon %u's object in DataMgr. This should never happen.", dungeonID);
    }
    SafeDelete(res);
}

void DungeonDataMgr::GetRandomDungeon(Dungeon::Dungeon& dungeon, uint8 archetype, uint32 factionID /*=0*/, float security /*=1.0*/) {
    auto& archetypeIndex = m_dungeons.get<Dungeon::DungeonsByArchetype>();
    auto range = archetypeIndex.equal_range(archetype);
    // Collect dungeons matching faction + security requirements
    std::vector<Dungeon::Dungeon> candidates;
    for (auto it = range.first; it != range.second; ++it) {
        if (factionID != 0 && it->factionID != factionID) continue;
        if (security < it->minSecurity || security > it->maxSecurity) continue;
        candidates.push_back(*it);
    }
    // Fallback: faction only (ignore security)
    if (candidates.empty()) {
        for (auto it = range.first; it != range.second; ++it) {
            if (factionID != 0 && it->factionID != factionID) continue;
            candidates.push_back(*it);
        }
    }
    // Fallback: any dungeon of this archetype
    if (candidates.empty()) {
        for (auto it = range.first; it != range.second; ++it)
            candidates.push_back(*it);
    }
    if (candidates.empty()) return;
    uint32 randomIndex = rand() % candidates.size();
    dungeon = candidates[randomIndex];
}

void DungeonDataMgr::GetDungeon(Dungeon::Dungeon& dungeon, uint32 dungeonID) {
    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    auto it = byDungeonID.find(dungeonID);
        if (it != byDungeonID.end())
        {
            dungeon = *it;
        } else {
            _log(DUNG__ERROR, "GetDungeon() - Failed to find dungeon with id %u", dungeonID);
        }
}

void DungeonDataMgr::Populate()
{
    // Populate dungeon datasets from DB
    double start = GetTimeMSeconds();
    DBQueryResult *res = new DBQueryResult();
    DBResultRow row;

    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    DungeonDB::GetAllDungeonData(*res);
    while (res->GetRow(row))
    {
        CreateDungeon(row);
        FillObject(row);
    }

    sLog.Cyan("       DunDataMgr", "%lu Dungeon data sets loaded in %.3fms.",
              byDungeonID.size(), (GetTimeMSeconds() - start));

    //cleanup
    SafeDelete(res);
}

void DungeonDataMgr::CreateDungeon(DBResultRow row) {
    // Multi-index view by dungeonID
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    // Check if dungeon already exists for this object
    auto it = byDungeonID.find(row.GetUInt(0));
    if (it != byDungeonID.end()) {
        Dungeon::Dungeon dData = *it;
        // Check if room already exists for this object
        if (dData.rooms.find(row.GetUInt(5)) == dData.rooms.end()) {
            // Create new room
            Dungeon::Room rData;
            rData.roomID = row.GetUInt(5);
            dData.rooms.insert({rData.roomID, rData});

            // Replace record in container
            byDungeonID.erase(dData.dungeonID);
            byDungeonID.insert(dData);
        }
    } else {
        // Create dungeon and room
        Dungeon::Dungeon dData;
        Dungeon::Room rData;
        rData.roomID = row.GetUInt(5);
        dData.rooms.insert({rData.roomID, rData});
        dData.dungeonID = row.GetUInt(0);
        byDungeonID.insert(dData);
    }
}

void DungeonDataMgr::FillObject(DBResultRow row) {
    /*    if (!sDatabase.RunQuery(res, "SELECT dunDungeons.dungeonID "
    "dungeonName, dungeonStatus, factionID, archetypeID, "
    "dunRooms.roomID, dunRooms.roomName, objectID, typeID, groupID, "
    "x, y, z, yaw, pitch, roll, radius "
    "FROM ((dunDungeons "
    "INNER JOIN dunRooms ON dunDungeons.dungeonID = dunRooms.dungeonID) "
    "INNER JOIN dunRoomObjects ON dunRooms.roomID = dunRoomObjects.roomID)"))*/

    // Multi-index view used for inserting
    auto &byDungeonID = m_dungeons.get<Dungeon::DungeonsByID>();

    auto it = byDungeonID.find(row.GetUInt(0));
    if (it != byDungeonID.end()) {
        Dungeon::Dungeon dData = *it;

        // Add the object to the room
        Dungeon::RoomObject oData;
        oData.objectID = row.GetUInt(7);
        oData.typeID = row.GetUInt(8);
        oData.groupID = row.GetUInt(9);
        oData.x = row.GetInt(10);
        oData.y = row.GetInt(11);
        oData.z = row.GetInt(12);
        oData.yaw = row.GetInt(13);
        oData.pitch = row.GetInt(14);
        oData.roll = row.GetInt(15);
        oData.radius = row.GetInt(16);
        dData.rooms[row.GetUInt(5)].objects.push_back(oData);
        // Populate all data for the dungeon and room
        dData.name = row.GetText(1);
        dData.status = row.GetUInt(2);
        dData.factionID = row.GetUInt(3);
        dData.archetypeID = row.GetUInt(4);
        dData.minSecurity = row.IsNull(17) ? -1.0f : row.GetFloat(17);
        dData.maxSecurity = row.IsNull(18) ? 1.0f : row.GetFloat(18);
        dData.difficulty = row.IsNull(19) ? 1 : row.GetUInt(19);
        dData.rooms[row.GetUInt(5)].roomID = row.GetUInt(5);
        dData.rooms[row.GetUInt(5)].roomName = row.GetText(6);

        // Replace item in container
        byDungeonID.erase(dData.dungeonID);
        byDungeonID.insert(dData);
    } 
    else {
        _log(DUNG__ERROR, "FillObject() - Failed to find dungeon %u's object in DataMgr. This should never happen.", row.GetUInt(0));
    }
}

const char* DungeonDataMgr::GetDungeonType(int8 typeID)
{
    // Return the string representation of the given dungeon type ID
    switch (typeID) {
        case Dungeon::Type::Mission:        return "Mission";
        case Dungeon::Type::Gravimetric:    return "Gravimetric";
        case Dungeon::Type::Magnetometric:  return "Magnetometric";
        case Dungeon::Type::Radar:          return "Radar";
        case Dungeon::Type::Ladar:          return "Ladar";
        case Dungeon::Type::Rated:          return "Rated";
        case Dungeon::Type::Anomaly:        return "Anomaly";
        case Dungeon::Type::Unrated:        return "Unrated";
        case Dungeon::Type::Escalation:     return "Escalation";
        case Dungeon::Type::Wormhole:       return "Wormhole";
        default:                            return "Invalid";
    }
}


DungeonMgr::DungeonMgr(SystemManager* mgr, EVEServiceManager& svc)
: m_system(mgr),
m_services(svc),
m_anomMgr(nullptr),
m_spawnMgr(nullptr),
m_initalized(false),
m_procTimer(0)
{
}

DungeonMgr::~DungeonMgr()
{
    // TODO: clean up and free any resources
}

bool DungeonMgr::Init(AnomalyMgr* anomMgr, SpawnMgr* spawnMgr)
{
    if (!m_initalized)
    {
        m_anomMgr = anomMgr;
        m_spawnMgr = spawnMgr;

        if (m_anomMgr == nullptr) {
            _log(COSMIC_MGR__ERROR, "System Init Fault. anomMgr == nullptr.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return m_initalized;
        }

        if (m_spawnMgr == nullptr) {
            _log(COSMIC_MGR__ERROR, "System Init Fault. spawnMgr == nullptr.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return m_initalized;
        }

        if (!sConfig.cosmic.DungeonEnabled){
            _log(COSMIC_MGR__INIT, "Dungeon System Disabled.  Not Initializing Dungeon Manager for %s(%u)", m_system->GetName(), m_system->GetID());
            return true;
        }

        m_spawnMgr->SetDungMgr(this);

        // Check for expired dungeons every 5 minutes
        m_procTimer.Start(300000);

        _log(COSMIC_MGR__INIT, "DungeonMgr Initialized for %s(%u)", m_system->GetName(), m_system->GetID());

        m_initalized = true;
        return true;
    }
    return false;
}

void DungeonMgr::Process()
{
    if (!m_initalized)
        return;
    if (!m_procTimer.Check(false))
        return;

    int64 now = GetFileTimeNow();
    std::vector<uint32> toExpire;

    for (auto& kv : m_dungeonList) {
        const Dungeon::LiveDungeon& dung = kv.second;
        if (now >= dung.expiryTime)
            toExpire.push_back(dung.anomalyID);
    }

    for (uint32 anomalyID : toExpire) {
        auto it = m_dungeonList.find(anomalyID);
        if (it == m_dungeonList.end())
            continue;

        const Dungeon::LiveDungeon& dung = it->second;
        int8 dungeonType = dung.dungeonType;

        // Remove all room items from space and delete them
        for (auto& roomKV : dung.rooms) {
            for (uint32 itemID : roomKV.second.items) {
                SystemEntity* pSE = m_system->GetSE(itemID);
                if (pSE != nullptr)
                    m_system->RemoveEntity(pSE);
                InventoryItemRef iRef = sItemFactory.GetItemRefFromID(itemID);
                if (iRef.get() != nullptr)
                    iRef->Delete();
            }
        }

        // Remove the root anomaly SE — this also calls AnomalyMgr::RemoveSignal,
        // which decrements the counter and removes it from scanner maps
        SystemEntity* pRootSE = m_system->GetSE(anomalyID);
        if (pRootSE != nullptr)
            m_system->RemoveEntity(pRootSE);
        InventoryItemRef rootRef = sItemFactory.GetItemRefFromID(anomalyID);
        if (rootRef.get() != nullptr)
            rootRef->Delete();

        // Re-queue this dungeon type so AnomalyMgr respawns it
        m_anomMgr->QueueRespawn(dungeonType);

        m_dungeonList.erase(it);

        _log(COSMIC_MGR__MESSAGE, "DungeonMgr::Process() - Expired dungeon anomalyID %u (type %u), re-queued for respawn", anomalyID, dungeonType);
    }
}

bool DungeonMgr::MakeDungeon(CosmicSignature& sig, uint32 dungeonID)
{
    // TODO: create a new dungeon using the given signature

    Dungeon::Dungeon dData;

    // If we are given a dungeonID, use it otherwise pick a random dungeon based on archetype + security
    if (dungeonID == 0) {
        float sec = m_system != nullptr ? m_system->GetSecValue() : 1.0;
        sDunDataMgr.GetRandomDungeon(dData, sig.dungeonType, sig.ownerID, sec);
    } else {
        sDunDataMgr.GetDungeon(dData, dungeonID);
    }

    if ((sig.sigGroupID == EVEDB::invGroups::Cosmic_Signature) || (sig.sigGroupID == EVEDB::invGroups::Cosmic_Anomaly)) {
        // Create a new anomaly inventory item to track entire dungeon under
        ItemData iData(sig.sigTypeID, sig.ownerID, sig.systemID, flagNone, sig.sigName.c_str(), sig.position/*, info*/);

        InventoryItemRef iRef = sItemFactory.SpawnItem(iData);
        if (iRef.get() == nullptr)
            return false;
        iRef->SetCustomInfo(std::string("livedungeon").c_str());
        iRef->SaveItem();

        CelestialSE* cSE = new CelestialSE(iRef, m_system->GetServiceMgr(), m_system);

        if (cSE == nullptr)
            return false;

        // dont add signal thru sysMgr.  signal is added when this returns to anomMgr
        m_system->AddEntity(cSE, false);
        sig.sigItemID = iRef->itemID();
        sig.bubbleID = cSE->SysBubble()->GetID();

        _log(COSMIC_MGR__TRACE, "DungeonMgr::Create() - %s using dungeonID %u", sig.sigName.c_str(), dData.dungeonID);

        // Create the new live dungeon
        Dungeon::LiveDungeon newDungeon;
        newDungeon.anomalyID = iRef->itemID();
        newDungeon.systemID = sig.systemID;
        newDungeon.dungeonType = sig.dungeonType;
        // Sites expire after 2 hours; Process() cleans them up and re-queues for respawn
        newDungeon.expiryTime = GetFileTimeNow() + (2 * EvE::Time::Hour);

        // Iterate through rooms and handle item spawning for each room
        uint16 roomCounter = 0;
        for (auto const& room : dData.rooms) {
            Dungeon::LiveRoom newRoom;
            // Set room position
            // Handle first room differently as it will be at the origin point of signature
            if (roomCounter == 0) {
                newRoom.position = sig.position;
            } else {
                // The following rooms shall be 100M kilometers in x direction from previous room.
                GPoint pos;
                pos.x = newDungeon.rooms[roomCounter - 1].position.x + NEXT_DUNGEON_ROOM_DIST;
                pos.y = newDungeon.rooms[roomCounter - 1].position.y;
                pos.z = newDungeon.rooms[roomCounter - 1].position.z;
                newRoom.position = pos;
            }

            for (auto object : room.second.objects ) {
                GPoint pos;
                // Set position for each object
                pos.x = newRoom.position.x + object.x;
                pos.y = newRoom.position.y + object.y;
                pos.z = newRoom.position.z + object.z;

                // If invGroup is an NPC, we must spawn it as such
                Inv::TypeData objType;
                Inv::GrpData objGroup;
                sDataMgr.GetType(object.typeID, objType);
                sDataMgr.GetGroup(objType.groupID, objGroup);
                if (objGroup.catID == EVEDB::invCategories::Ship || 
                    objGroup.catID == EVEDB::invCategories::Drone ||
                    objGroup.catID == EVEDB::invCategories::Entity) {
                    bool isIncursion = (dungeonID >= 2100 && dungeonID <= 2122);
                    sLog.Debug("MakeDungeon", "Spawning NPC typeID=%u cat=%u group=%u", object.typeID, objGroup.catID, objType.groupID);
                    m_spawnMgr->DoSpawnForAnomaly(sBubbleMgr.FindBubble(m_system->GetID(), pos), pos, GetRandLevel(), object.typeID, isIncursion);
                } else {
                    sLog.Debug("MakeDungeon", "Spawning CELESTIAL typeID=%u cat=%u group=%u", object.typeID, objGroup.catID, objType.groupID);
                    ItemData itemData(object.typeID, sig.ownerID, sig.systemID, flagNone, sDataMgr.GetTypeName(object.typeID), pos);

                    iRef = sItemFactory.SpawnItem(itemData);
                    if (iRef.get() == nullptr) {
                        _log(COSMIC_MGR__ERROR, "DungeonMgr::Create() - Unable to spawn item with type %s for room %u dungeon with anomaly itemID %u", sDataMgr.GetTypeName(object.typeID), roomCounter, newDungeon.anomalyID);
                        return false;
                    }

                    // Configure containers based on site type
                    if (sig.dungeonType == 3 || sig.dungeonType == 4) {
                        // Data/Relic site containers: settable access diff for hacking
                        iRef->SetCustomInfo(std::to_string(sig.ownerID).c_str());
                        iRef->SetAttribute(AttrAccessDifficulty, 15.0f + MakeRandomInt(0, 15), false);
                        if (sig.dungeonType == 3)
                            iRef->SetAttribute(AttrScanMagnetometricStrength, 1.0f, false);
                        else
                            iRef->SetAttribute(AttrScanRadarStrength, 1.0f, false);
                    } else if (sig.dungeonType == 10) {
                        // DED complex containers: locked until NPCs die, pre-populated with loot
                        iRef->SetCustomInfo(("ded_" + std::to_string(sig.ownerID)).c_str());
                        iRef->SetAttribute(AttrAccessDifficulty, 50.0f, false);  // locked
                        // Pre-populate DED loot based on tier (1/10-5/10)
                        PopulateDEDContainer(iRef, sig.ownerID, dData.difficulty);
                    } else {
                        iRef->SetCustomInfo(("livedungeon_" + std::to_string(newDungeon.anomalyID)).c_str());
                    }
                    iRef->SaveItem();

                    cSE = new CelestialSE(iRef, m_system->GetServiceMgr(), m_system);
                    m_system->AddEntity(cSE, false);
                    newRoom.items.push_back(iRef->itemID());
                }
            }

            // Spawn procedural decorations based on faction
            SpawnDecorations(newRoom.position, dData.factionID);

            newDungeon.rooms.insert({roomCounter, newRoom});
            roomCounter++;
        }

        // Finally add the new dungeon to the system-wide list for tracking
        m_dungeonList.insert({newDungeon.anomalyID, newDungeon});
        return true;
    }
    return false;
}

void DungeonMgr::SpawnDecorations(const GPoint& roomPos, uint32 factionID)
{
    // Faction-specific decorative structure typeIDs (common SDE celestial objects)
    static const std::vector<uint32> genericDeco = {
        23,      // CargoContainer
        3298,    // HangarContainer
        3293,    // MediumStandardContainer
        3296,    // LargeStandardContainer
        24445,   // GeneralFreightContainer
        26468,   // Wreck
        10645,   // CelestialBeacon
        10124,   // Beacon
        10753,   // SoftCloud
        10754,   // WispyOrangeCloud
        10758,   // WispyChlorineCloud
    };
    // Asteroid chunks for visual debris
    static const std::vector<uint32> asteroidDeco = {
        1230,    // Veldspar
        1231,    // Scordite
        1232,    // Pyroxeres
        1228,    // Plagioclase
        1226,    // Kernite
        1225,    // Jaspet
        1227,    // Hemorphite
        1229,    // Omber
    };
    // Wreck types per race
    static const std::vector<uint32> wreckDeco = {
        26483,   // AmarrFreighterWreck
        26505,   // CaldariFreighterWreck
        26527,   // GallenteFreighterWreck
        26549,   // MinmatarFreighterWreck
    };

    // Faction-specific structure pools
    uint32 decoCount = 8 + MakeRandomInt(0, 7);
    std::vector<uint32> factionDeco;
    switch (factionID) {
        case factionAngel: {
            factionDeco = {17366, 3298, 24445, 3465, 3296, 26549, 26527};
            decoCount = 10 + MakeRandomInt(0, 8);
            break;
        }
        case factionGuristas: {
            factionDeco = {17366, 3298, 3465, 26483, 26505, 10754};
            decoCount = 8 + MakeRandomInt(0, 6);
            break;
        }
        case factionBloodRaider: {
            factionDeco = {17366, 3467, 19373, 24545, 10758, 3465};
            decoCount = 6 + MakeRandomInt(0, 5);
            break;
        }
        case factionSanshas: {
            factionDeco = {17366, 26483, 26527, 10754, 3298, 24445};
            decoCount = 8 + MakeRandomInt(0, 7);
            break;
        }
        case factionSerpentis: {
            factionDeco = {17366, 3298, 24445, 10753, 3296, 3465};
            decoCount = 10 + MakeRandomInt(0, 8);
            break;
        }
        case factionRogueDrones: {
            factionDeco = {26468, 26549, 26527, 24445, 10758, 3465};
            decoCount = 12 + MakeRandomInt(0, 8);
            break;
        }
        default: {
            factionDeco = {17366, 3298, 24445, 3465, 3296, 26549, 26527, 26483, 26505};
            decoCount = 6 + MakeRandomInt(0, 4);
            break;
        }
    }

    // Mix in some of each category
    std::vector<uint32> decoPool;
    for (auto t : factionDeco) decoPool.push_back(t);
    for (auto t : genericDeco) decoPool.push_back(t);
    if (MakeRandomInt(0, 2) > 0)
        for (auto t : asteroidDeco) decoPool.push_back(t);
    if (MakeRandomInt(0, 3) == 0)
        for (auto t : wreckDeco) decoPool.push_back(t);

    for (uint32 i = 0; i < decoCount; ++i) {
        uint32 typeID = decoPool[MakeRandomInt(0, decoPool.size() - 1)];
        // Random position within 3000-8000m radius from room center, at various heights
        double angle = MakeRandomFloat() * 2.0 * 3.14159;
        double radius = 3000.0 + MakeRandomFloat() * 5000.0;
        double height = (MakeRandomFloat() - 0.5) * 4000.0;
        GPoint pos;
        pos.x = roomPos.x + cos(angle) * radius;
        pos.y = roomPos.y + sin(angle) * radius;
        pos.z = roomPos.z + height;

        ItemData itemData(typeID, 1, m_system->GetID(), flagNone, "", pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
        if (iRef.get() == nullptr)
            continue;
        iRef->SaveItem();
        CelestialSE* cSE = new CelestialSE(iRef, m_services, m_system);
        m_system->AddEntity(cSE, false);
    }
}

int8 DungeonMgr::GetFaction(uint32 factionID)
{
    switch (factionID) {
        case factionAngel:          return 2;
        case factionSanshas:        return 5;
        case factionGuristas:       return 4;
        case factionSerpentis:      return 1;
        case factionBloodRaider:    return 3;
        case factionRogueDrones:    return 6;
        case 0:                     return 7;
        // these are incomplete.  set to default (region rat)
        case factionAmarr:
        case factionAmmatar:
        case factionCaldari:
        case factionGallente:
        case factionMinmatar:
        default:
            return GetFaction(sDataMgr.GetRegionRatFaction(m_system->GetRegionID()));
    }
}

int8 DungeonMgr::GetRandLevel()
{
    float sec = m_system != nullptr ? m_system->GetSecValue() : 1.0;
    // Higher security → lower levels, lower security → higher levels
    double r = MakeRandomFloat();
    _log(DUNG__TRACE, "DungeonMgr::GetRandLevel() - sec=%.2f r=%.2f", sec, r);

    // Null-sec (-1.0 to -0.0): weighted toward levels 3-5
    if (sec < -0.5) {
        if (r < 0.20) return 5;
        if (r < 0.50) return 4;
        if (r < 0.80) return 3;
        return 2;
    }
    // Low-sec (0.0 to 0.4)
    if (sec < 0.5) {
        if (r < 0.10) return 5;
        if (r < 0.30) return 4;
        if (r < 0.60) return 3;
        if (r < 0.85) return 2;
        return 1;
    }
    // High-sec (0.5 to 1.0) — almost always level 1, never above 2
    if (r < 0.05) return 2;
    return 1;
}

void DungeonMgr::PopulateDEDContainer(InventoryItemRef containerRef, uint32 factionID, uint8 difficulty)
{
    // Pre-populate DED complex container with faction modules, ship BPCs, and overseer effects
    // Difficulty = DED tier (1-5), influences loot quality and quantity
    if (containerRef.get() == nullptr) return;

    // Overseer's Personal Effects (always drops at 5/10, chance at other tiers)
    if (difficulty >= 5 || MakeRandomFloat() < 0.3f * difficulty) {
        // Tier 1-23 Overseer's Personal Effects (typeIDs 19400-19422)
        uint32 oeType = 19400 + MakeRandomInt(0, 22);
        ItemData oeData(oeType, 1, containerRef->itemID(), flagNone);
        InventoryItemRef oeRef = sItemFactory.SpawnItem(oeData);
        if (oeRef.get() != nullptr && containerRef->GetMyInventory() != nullptr)
            containerRef->GetMyInventory()->AddItem(oeRef);
    }

    // Faction-specific loot tables by tier
    struct FactionModule {
        uint32 typeID;
        uint8 metaLevel;  // 1=C-type, 2=B-type, 3=A-type
    };

    std::vector<FactionModule> modPool;

    switch (factionID) {
        case factionAngel: {
            // Angel → Domination / Gistii/Gistum/Gist
            if (difficulty <= 2) {
                // 1/10-2/10: C-type small modules
                modPool = {{13773,1}, {13776,1}, {13786,1}};  // small autocannons
            } else if (difficulty <= 3) {
                // 3/10: C-type medium + rare B-type
                modPool = {{13778,1}, {13786,1}, {13788,2}};  // medium autocannons
            } else {
                // 4/10-5/10: B-type + rare A-type
                modPool = {{13785,2}, {13775,2}, {13788,3}};  // large weapons
            }
            break;
        }
        case factionBloodRaider: {
            // Blood → Dark Blood / Corpii/Corpum/Corp
            if (difficulty <= 2)
                modPool = {{13803,1}, {13811,1}, {13795,1}};  // small lasers
            else if (difficulty <= 3)
                modPool = {{13811,1}, {13807,1}, {13801,2}};  // medium lasers
            else
                modPool = {{13807,2}, {13799,2}, {13793,3}};  // large lasers
            break;
        }
        case factionSanshas: {
            // Sansha → True Sansha / Centii/Centum/Cent
            if (difficulty <= 2)
                modPool = {{13803,1}, {13811,1}, {13795,1}};  // small lasers (same as Blood)
            else if (difficulty <= 3)
                modPool = {{13811,1}, {13807,1}, {13801,2}};
            else
                modPool = {{13807,2}, {13799,2}, {13793,3}};
            break;
        }
        default: {
            // Generic fallback: small weapons
            modPool = {{13773,1}, {13776,1}, {13803,1}};
            break;
        }
    }

    // Drop 1-3 faction modules
    uint8 modCount = 1 + MakeRandomInt(0, std::min<uint8>(2, difficulty));
    for (uint8 i = 0; i < modCount; ++i) {
        if (modPool.empty()) break;
        uint32 idx = MakeRandomInt(0, modPool.size() - 1);
        ItemData modData(modPool[idx].typeID, 1, containerRef->itemID(), flagNone);
        InventoryItemRef modRef = sItemFactory.SpawnItem(modData);
        if (modRef.get() != nullptr && containerRef->GetMyInventory() != nullptr)
            containerRef->GetMyInventory()->AddItem(modRef);
    }

    // Ship BPCs: faction frigates drop from lower tiers, cruisers from higher
    if (MakeRandomFloat() < 0.15f * difficulty) {
        uint32 bpcType = 0;
        switch (factionID) {
            case factionAngel:
                bpcType = (difficulty <= 3) ? 17933 : 17721;  // Dramiel or Cynabal BPC
                break;
            case factionBloodRaider:
                bpcType = (difficulty <= 3) ? 17927 : 17923;  // Cruor or Ashimmu BPC
                break;
            case factionGuristas:
                bpcType = (difficulty <= 3) ? 17931 : 17716;  // Worm or Gila BPC
                break;
            case factionSanshas:
                bpcType = (difficulty <= 3) ? 17925 : 17719;  // Succubus or Phantasm BPC
                break;
            case factionSerpentis:
                bpcType = (difficulty <= 3) ? 17929 : 17723;  // Daredevil or Vigilant BPC
                break;
        }
        if (bpcType != 0) {
            ItemData bpcData(bpcType, 1, containerRef->itemID(), flagNone);
            InventoryItemRef bpcRef = sItemFactory.SpawnItem(bpcData);
            if (bpcRef.get() != nullptr && containerRef->GetMyInventory() != nullptr)
                containerRef->GetMyInventory()->AddItem(bpcRef);
        }
    }
}
