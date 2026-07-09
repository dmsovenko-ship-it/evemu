
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

    struct EffectModifier {
        uint16 attributeID;
        uint8 operation;   // 0=multiply, 1=add, 2=percent
        double value;
    };

    struct WHEffectDef {
        WHEffectType type;
        std::vector<EffectModifier> modifiers;
    };

    struct SovUpgradeEffect {
        uint32 typeID;
        std::vector<EffectModifier> modifiers;
    };

    WHEffectType GetWHEffect(uint32 systemID, uint8 whClass);
    const WHEffectDef* GetEffectDef(WHEffectType type) const;
    void GetSovUpgradeEffects(uint32 systemID, std::vector<EffectModifier>& outModifiers);

    void ApplyEffect(InventoryItemRef ship, const EffectModifier& mod, const char* context);
    void ApplyModifiers(InventoryItemRef ship, const std::vector<EffectModifier>& mods, const char* context);
    void RemoveModifiers(InventoryItemRef ship, const std::vector<EffectModifier>& mods);

    // Track which client has which effect applied
    struct AppliedEffects {
        std::vector<EffectModifier> whMods;    // wormhole effect modifiers
        std::vector<EffectModifier> sovMods;   // sovereignty upgrade modifiers
    };
    std::map<uint32, AppliedEffects> m_clientEffects;

    // Effect definitions
    std::map<WHEffectType, WHEffectDef> m_effectDefs;
    std::map<uint32, SovUpgradeEffect> m_sovEffects;
};

#define sSystemEffectMgr \
( SystemEffectMgr::get() )

#endif  // EVEMU_SYSTEM_SYSTEMEFFECTMGR_H_