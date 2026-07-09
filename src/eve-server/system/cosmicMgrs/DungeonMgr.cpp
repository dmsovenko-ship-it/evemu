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
                    // Define ItemData object for each RoomObject
                    ItemData dData(object.typeID, sig.ownerID, sig.systemID, flagNone, sDataMgr.GetTypeName(object.typeID), pos);

                    // Spawn the object (and persist it across system unloads)
                    iRef = sItemFactory.SpawnItem(dData);
                    if (iRef.get() == nullptr) {
                        _log(COSMIC_MGR__ERROR, "DungeonMgr::Create() - Unable to spawn item with type %s for room %u dungeon with anomaly itemID %u", sDataMgr.GetTypeName(object.typeID), roomCounter, newDungeon.anomalyID);
                        return false;
                    }
                    iRef->SetCustomInfo(("livedungeon_" + std::to_string(newDungeon.anomalyID)).c_str());
                    iRef->SaveItem();

                    cSE = new CelestialSE(iRef, m_system->GetServiceMgr(), m_system);
                    m_system->AddEntity(cSE, false);
                    newRoom.items.push_back(iRef->itemID());
                }
            }

            newDungeon.rooms.insert({roomCounter, newRoom});
            roomCounter++;
        }

        // Finally add the new dungeon to the system-wide list for tracking
        m_dungeonList.insert({newDungeon.anomalyID, newDungeon});
        return true;
    }
    return false;
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
    // High-sec (0.5 to 1.0)
    if (r < 0.10) return 3;
    if (r < 0.30) return 2;
    return 1;
}
