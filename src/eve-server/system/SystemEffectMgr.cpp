#include "eve-server.h"
#include "EVEServerConfig.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "incursion/IncursionMgr.h"
#include "inventory/AttributeEnum.h"
#include "system/SystemEffectMgr.h"
#include "system/SystemManager.h"
#include "inventory/InventoryItem.h"
#include "ship/Ship.h"
#include "Client.h"

/*
 * SystemEffectMgr — applies wormhole system effects (Pulsar, Magnetar, Cataclysmic, etc.)
 * and sovereignty upgrade effects to ships when they enter a system.
 */

SystemEffectMgr::SystemEffectMgr()
{
    // ── Wormhole effects ──────────────────────────────────────

    // Pulsar: shield bonus +50%, capacitor recharge -30%
    {
        WHEffectDef def;
        def.type = WH_Pulsar;
        def.modifiers = {
            {6,   2,  50.0},
            {54,  2, -30.0},
            {165, 2, -20.0},
        };
        m_effectDefs[WH_Pulsar] = def;
    }

    // Magnetar: sensor strength +50%, shield HP -30%
    {
        WHEffectDef def;
        def.type = WH_Magnetar;
        def.modifiers = {
            {6,   2, -30.0},
            {54,  2,  20.0},
            {63,  2,  50.0},
            {64,  2,  50.0},
            {65,  2,  50.0},
            {66,  2,  50.0},
        };
        m_effectDefs[WH_Magnetar] = def;
    }

    // Cataclysmic Variable: armor bonus +50%, shield HP -30%
    {
        WHEffectDef def;
        def.type = WH_Cataclysmic;
        def.modifiers = {
            {6,   2, -30.0},
            {72,  2,  50.0},
        };
        m_effectDefs[WH_Cataclysmic] = def;
    }

    // Black Hole: velocity +50%, tracking -25%
    {
        WHEffectDef def;
        def.type = WH_BlackHole;
        def.modifiers = {
            {37,  2,  50.0},
            {172, 2, -25.0},
            {1796,2,  15.0},
        };
        m_effectDefs[WH_BlackHole] = def;
    }

    // Red Giant: capacitor +50%, armor -30%
    {
        WHEffectDef def;
        def.type = WH_RedGiant;
        def.modifiers = {
            {72,  2, -30.0},
            {165, 2,  50.0},
        };
        m_effectDefs[WH_RedGiant] = def;
    }

    // Wolf-Rayet: shield -20%, armor +30%, sig -10%
    {
        WHEffectDef def;
        def.type = WH_WolfRayet;
        def.modifiers = {
            {6,   2, -20.0},
            {72,  2,  30.0},
            {1796,2, -10.0},
        };
        m_effectDefs[WH_WolfRayet] = def;
    }

    // ── Sovereignty upgrade effects ───────────────────────────

    // Cynosural Navigation (2008): cyno range +100% (affects system cyno gen)
    {
        SovUpgradeEffect eff;
        eff.typeID = 2008;
        eff.modifiers = {};
        m_sovEffects[2008] = eff;
    }

    // Cynosural Suppression (2001): cyno jam strength +50%
    {
        SovUpgradeEffect eff;
        eff.typeID = 2001;
        eff.modifiers = {};
        m_sovEffects[2001] = eff;
    }

    // Advanced Logistics Network (32422): jump bridge fuel -25%
    {
        SovUpgradeEffect eff;
        eff.typeID = 32422;
        eff.modifiers = {};
        m_sovEffects[32422] = eff;
    }

    // Supercapital Construction Facilities (2009):
    // capital ship build time -25%, capital component build time -25%
    {
        SovUpgradeEffect eff;
        eff.typeID = 2009;
        eff.modifiers = {};
        m_sovEffects[2009] = eff;
    }
}

SystemEffectMgr::~SystemEffectMgr()
{
    m_clientEffects.clear();
}

void SystemEffectMgr::Initialize()
{
    sLog.Blue(" System Effect Manager", "Initialized wormhole + sovereignty system effects.");
}

// ── Wormhole effect lookup ──────────────────────────────────────

SystemEffectMgr::WHEffectType SystemEffectMgr::GetWHEffect(uint32 systemID, uint8 whClass)
{
    if (whClass == 0) return WH_None;
    if (whClass <= 3) return WH_None;
    uint32 hash = systemID * 2654435761U;
    WHEffectType types[] = { WH_Pulsar, WH_Magnetar, WH_Cataclysmic, WH_BlackHole, WH_RedGiant, WH_WolfRayet };
    return types[hash % 6];
}

const SystemEffectMgr::WHEffectDef* SystemEffectMgr::GetEffectDef(WHEffectType type) const
{
    auto it = m_effectDefs.find(type);
    return (it != m_effectDefs.end()) ? &it->second : nullptr;
}

// ── Sovereignty upgrade effect lookup ──────────────────────────

void SystemEffectMgr::GetSovUpgradeEffects(uint32 systemID, std::vector<EffectModifier>& outModifiers)
{
    outModifiers.clear();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT typeID FROM sovUpgrades WHERE systemID = %u", systemID))
    {
        return;
    }
    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 typeID = row.GetUInt(0);
        auto it = m_sovEffects.find(typeID);
        if (it != m_sovEffects.end()) {
            for (const auto& mod : it->second.modifiers)
                outModifiers.push_back(mod);
        }
    }
}

// ── Apply/Remove helpers ───────────────────────────────────────

void SystemEffectMgr::ApplyEffect(InventoryItemRef ship, const EffectModifier& mod, const char* context)
{
    double current = ship->GetAttribute(mod.attributeID).get_float();
    if (std::isnan(current) || current <= 0.0) return;

    double newVal = current;
    switch (mod.operation) {
        case 0: newVal = current * mod.value; break;
        case 1: newVal = current + mod.value; break;
        case 2: newVal = current * (1.0 + mod.value / 100.0); break;
    }
    ship->SetAttribute(mod.attributeID, newVal);
    _log(SERVICE__MESSAGE, "SystemEffectMgr [%s]: attr %u: %.2f -> %.2f", context, mod.attributeID, current, newVal);
}

void SystemEffectMgr::ApplyModifiers(InventoryItemRef ship, const std::vector<EffectModifier>& mods, const char* context)
{
    for (const auto& mod : mods)
        ApplyEffect(ship, mod, context);
}

void SystemEffectMgr::RemoveModifiers(InventoryItemRef ship, const std::vector<EffectModifier>& mods)
{
    for (const auto& mod : mods) {
        double current = ship->GetAttribute(mod.attributeID).get_float();
        if (std::isnan(current) || current <= 0.0) continue;

        double newVal = current;
        switch (mod.operation) {
            case 0: newVal = current / mod.value; break;
            case 1: newVal = current - mod.value; break;
            case 2: newVal = current / (1.0 + mod.value / 100.0); break;
        }
        ship->SetAttribute(mod.attributeID, newVal);
        _log(SERVICE__MESSAGE, "SystemEffectMgr:   revert attr %u: %.2f -> %.2f", mod.attributeID, current, newVal);
    }
}

// ── System enter/leave ──────────────────────────────────────────

void SystemEffectMgr::OnEnterSystem(Client* client, uint32 systemID)
{
    if (client == nullptr || client->GetShipSE() == nullptr) return;
    InventoryItemRef ship = client->GetShipSE()->GetSelf();
    if (ship.get() == nullptr) return;

    AppliedEffects applied;

    // Wormhole effects
    uint8 whClass = sDataMgr.GetWHSystemClass(systemID);
    WHEffectType effectType = GetWHEffect(systemID, whClass);
    if (effectType != WH_None) {
        const WHEffectDef* def = GetEffectDef(effectType);
        if (def != nullptr) {
            ApplyModifiers(ship, def->modifiers, "wormhole");
            applied.whMods = def->modifiers;
        }
    }

    // Sovereignty upgrade effects
    std::vector<EffectModifier> sovMods;
    GetSovUpgradeEffects(systemID, sovMods);
    if (!sovMods.empty()) {
        ApplyModifiers(ship, sovMods, "sov");
        applied.sovMods = sovMods;
    }

    // Incursion constellation-wide penalties
    if (sIncursionMgr.IsIncursionSystem(systemID)) {
        uint8 sceneType = sIncursionMgr.GetSceneType(systemID);
        float penaltyMult = 1.0f;
        switch (sceneType) {
            case 3:  penaltyMult = 0.1f; break;  // Vanguard: 10%
            case 2:  penaltyMult = 0.25f; break; // Assault: 25%
            case 1:  penaltyMult = 0.50f; break; // HQ: 50%
            default: penaltyMult = 0.0f;  break; // Staging: no penalty
        }
        if (penaltyMult > 0.0f) {
            std::vector<EffectModifier> incMods;
            // Damage reduction: AttrDamageMultiplier
            incMods.push_back({AttrDamageMultiplier, 2, -penaltyMult * 100.0});
            // Resist penalty: shield HP, armor HP, hull HP reduction
            incMods.push_back({AttrShieldCapacity, 2, -penaltyMult * 100.0});
            incMods.push_back({AttrArmorHP, 2, -penaltyMult * 100.0});
            incMods.push_back({AttrHP, 2, -penaltyMult * 100.0});
            ApplyModifiers(ship, incMods, "incursion");
            applied.incMods = incMods;
        }
    }

    m_clientEffects[client->GetCharacterID()] = applied;
}

void SystemEffectMgr::OnLeaveSystem(Client* client, uint32 systemID)
{
    if (client == nullptr) return;
    uint32 charID = client->GetCharacterID();
    auto it = m_clientEffects.find(charID);
    if (it == m_clientEffects.end()) return;
    if (client->GetShipSE() == nullptr) return;
    InventoryItemRef ship = client->GetShipSE()->GetSelf();
    if (ship.get() == nullptr) return;

    // Remove wormhole effects
    if (!it->second.whMods.empty())
        RemoveModifiers(ship, it->second.whMods);

    // Remove sovereignty effects
    if (!it->second.sovMods.empty())
        RemoveModifiers(ship, it->second.sovMods);

    // Remove incursion effects
    if (!it->second.incMods.empty())
        RemoveModifiers(ship, it->second.incMods);

    m_clientEffects.erase(it);
}
