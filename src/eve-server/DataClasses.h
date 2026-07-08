
/**
 * @name DataClasses.h
 *  data container classes that cannot be trivially constructed/destructed
 *
 * @author: allan
 * @date 4 January 2018
 */


#ifndef EVE_DATA_CLASSES_H
#define EVE_DATA_CLASSES_H

#include "eve-server.h"
#include "POD_containers.h"
#include "../eve-common/EVE_POS.h"


// POS class container for processing-type items
class ReactorData {
public:
    ReactorData();
    ~ReactorData();

    void Init();
    void Clear();

    bool IsActive() const                               { return active; }
    void SetActive(bool set)                            { active = set; }
    void AddConnection(EVEPOS::POS_Connections& conn);
    void ClearConnections()                             { connections.clear(); }
    std::map<uint32, EVEPOS::POS_Connections>& GetConnections()     { return connections; }
    std::map<uint32, EVEPOS::POS_Resource>& GetDemands()            { return demands; }
    std::map<uint32, EVEPOS::POS_Resource>& GetSupplies()           { return supplies; }
    int16 GetReaction() const                           { return reaction; }
    void SetReaction(int16 react)                       { reaction = react; }
    int32 GetItemID() const                             { return itemID; }
    void SetItemID(int32 id)                            { itemID = id; }

private:
    bool active;
    int32 itemID;
    int16 reaction;     // bp typeID?
    std::map<uint32, EVEPOS::POS_Connections> connections;  // itemID, data
    std::map<uint32, EVEPOS::POS_Resource> demands;         // itemID, resourceData(typeID/quantity)
    std::map<uint32, EVEPOS::POS_Resource> supplies;        // itemID, resourceData(typeID/quantity)
};


#endif  // EVE_DATA_CLASSES_H