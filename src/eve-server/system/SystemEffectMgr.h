
#ifndef EVEMU_SYSTEM_SYSTEMEFFECTMGR_H_
#define EVEMU_SYSTEM_SYSTEMEFFECTMGR_H_

#include "utils/Singleton.h"
#include <map>
#include <vector>

class Client;
class SystemManager;
class InventoryItem;

class SystemEffectMgr
: public Singleton<SystemEffectMgr>
{
public:
    SystemEffectMgr();
    ~SystemEffectMgr();

    void Initialize();

    // Apply/remove system effects for a client entering/leaving a system
    void OnEnterSystem(Client* client, uint32 systemID);
    void OnLeaveSystem(Client* client, uint32 systemID);

private:
    enum WHEffectType {
        WH_None = 0,
        WH_Pulsar,
        WH_Magnetar,
        WH_Cataclysmic,
        WH_BlackHole,
        WH_RedGiant,
        WH_WolfRayet
    };

    struct WHEffectDef {
        WHEffectType type;
        // Attribute modifiers: { attributeID, operation, value }
        // operation: 0=multiply, 1=add, 2=percent
        std::vector<std::tuple<uint16, uint8, double>> modifiers;
    };

    WHEffectType GetWHEffect(uint32 systemID, uint8 whClass);
    const WHEffectDef* GetEffectDef(WHEffectType type) const;

    void ApplyEffect(InventoryItemRef ship, const WHEffectDef* effect);
    void RemoveEffect(InventoryItemRef ship, const WHEffectDef* effect);

    // Track which client has which effect applied (clientID → effect copy)
    std::map<uint32, WHEffectDef> m_clientEffects;

    // Effect definitions
    std::map<WHEffectType, WHEffectDef> m_effectDefs;
};

#define sSystemEffectMgr \
( SystemEffectMgr::get() )

#endif  // EVEMU_SYSTEM_SYSTEMEFFECTMGR_H_