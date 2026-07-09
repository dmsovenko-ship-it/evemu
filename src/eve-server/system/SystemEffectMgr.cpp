#include "eve-server.h"
#include "EVEServerConfig.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "system/SystemEffectMgr.h"
#include "system/SystemManager.h"
#include "inventory/InventoryItem.h"
#include "ship/Ship.h"
#include "Client.h"

#include <algorithm>

/*
 * SystemEffectMgr — applies wormhole system effects (Pulsar, Magnetar, Cataclysmic, etc.)
 * to ships when they enter a wormhole system and removes them on exit.
 *
 * Effect attribute modifiers are applied directly via SetAttribute.
 * When leaving, affected attributes are recalculated from item base values.
 *
 * Attribute IDs used:
 *   6    - shieldCapacity (shield HP)
 *   37   - maxVelocity (max speed)
 *   38   - maxTargetingRange (targeting range)
 *   54   - shieldRechargeRate (shield recharge)
 *   63   - scanGravimetricStrength (sensor strength)
 *   64   - scanLadarStrength
 *   65   - scanMagnetometricStrength
 *   66   - scanRadarStrength
 *   72   - maxArmor (armor HP)
 *   165  - capacitorCapacity (cap)
 *   172  - trackingSpeed (turret tracking)
 *   244  - damageMultiplier (weapon damage)
 *   467  - emDamage (damage resistance)
 *   1796 - signatureRadius (sig radius)
 */

SystemEffectMgr::SystemEffectMgr()
{
    // Pulsar: shield bonus +50%, capacitor recharge -30%
    {
        WHEffectDef def;
        def.type = WH_Pulsar;
        def.modifiers = {
            {6,   2,  50.0},    // shieldCapacity +50%
            {54,  2, -30.0},    // shieldRechargeRate -30%
            {165, 2, -20.0},    // capacitorCapacity -20%
        };
        m_effectDefs[WH_Pulsar] = def;
    }

    // Magnetar: sensor strength +50%, shield HP -30%
    {
        WHEffectDef def;
        def.type = WH_Magnetar;
        def.modifiers = {
            {6,   2, -30.0},    // shieldCapacity -30%
            {54,  2,  20.0},    // shieldRechargeRate +20%
            {63,  2,  50.0},    // scanGravimetricStrength +50%
            {64,  2,  50.0},    // scanLadarStrength +50%
            {65,  2,  50.0},    // scanMagnetometricStrength +50%
            {66,  2,  50.0},    // scanRadarStrength +50%
        };
        m_effectDefs[WH_Magnetar] = def;
    }

    // Cataclysmic Variable: armor bonus +50%, shield HP -30%
    {
        WHEffectDef def;
        def.type = WH_Cataclysmic;
        def.modifiers = {
            {6,   2, -30.0},    // shieldCapacity -30%
            {72,  2,  50.0},    // maxArmor +50%
        };
        m_effectDefs[WH_Cataclysmic] = def;
    }

    // Black Hole: velocity +50%, tracking -25%
    {
        WHEffectDef def;
        def.type = WH_BlackHole;
        def.modifiers = {
            {37,  2,  50.0},    // maxVelocity +50%
            {172, 2, -25.0},    // trackingSpeed -25%
            {1796,2,  15.0},    // signatureRadius +15%
        };
        m_effectDefs[WH_BlackHole] = def;
    }

    // Red Giant: capacitor +50%, armor -30%
    {
        WHEffectDef def;
        def.type = WH_RedGiant;
        def.modifiers = {
            {72,  2, -30.0},    // maxArmor -30%
            {165, 2,  50.0},    // capacitorCapacity +50%
        };
        m_effectDefs[WH_RedGiant] = def;
    }

    // Wolf-Rayet: small weapon dmg +25%, large weapon dmg -25%
    {
        WHEffectDef def;
        def.type = WH_WolfRayet;
        def.modifiers = {
            {6,   2, -20.0},    // shieldCapacity -20%
            {72,  2,  30.0},    // maxArmor +30%
            {1796,2, -10.0},    // signatureRadius -10%
        };
        m_effectDefs[WH_WolfRayet] = def;
    }
}

SystemEffectMgr::~SystemEffectMgr()
{
    m_clientEffects.clear();
}

void SystemEffectMgr::Initialize()
{
    sLog.Blue(" System Effect Manager", "Initialized wormhole system effects (Pulsar, Magnetar, Cataclysmic, etc).");
}

SystemEffectMgr::WHEffectType SystemEffectMgr::GetWHEffect(uint32 systemID, uint8 whClass)
{
    if (whClass == 0)
        return WH_None;

    // C1-C3: no permanent effects
    if (whClass <= 3)
        return WH_None;

    // C4-C6: assign effect based on systemID hash for variety
    // Approximately even distribution across 6 effect types
    uint32 hash = systemID * 2654435761U;  // golden ratio hash
    WHEffectType types[] = {
        WH_Pulsar, WH_Magnetar, WH_Cataclysmic,
        WH_BlackHole, WH_RedGiant, WH_WolfRayet
    };
    uint8 idx = hash % 6;

    // C4: weaker version of effects
    // C5: standard
    // C6: stronger (we don't differentiate magnitudes for now)
    return types[idx];
}

const SystemEffectMgr::WHEffectDef* SystemEffectMgr::GetEffectDef(WHEffectType type) const
{
    auto it = m_effectDefs.find(type);
    if (it != m_effectDefs.end())
        return &it->second;
    return nullptr;
}

void SystemEffectMgr::OnEnterSystem(Client* client, uint32 systemID)
{
    if (client == nullptr || client->GetShipSE() == nullptr)
        return;

    InventoryItemRef ship = client->GetShipSE()->GetSelf();
    if (ship.get() == nullptr)
        return;

    uint8 whClass = sDataMgr.GetWHSystemClass(systemID);
    WHEffectType effectType = GetWHEffect(systemID, whClass);
    if (effectType == WH_None)
        return;

    const WHEffectDef* def = GetEffectDef(effectType);
    if (def == nullptr)
        return;

    ApplyEffect(ship, def);
    m_clientEffects[client->GetCharacterID()] = *def;

    _log(SERVICE__MESSAGE, "SystemEffectMgr: Applied %s effect to %s in system %u",
         (effectType == WH_Pulsar ? "Pulsar" :
          effectType == WH_Magnetar ? "Magnetar" :
          effectType == WH_Cataclysmic ? "Cataclysmic" :
          effectType == WH_BlackHole ? "BlackHole" :
          effectType == WH_RedGiant ? "RedGiant" : "WolfRayet"),
         client->GetName(), systemID);
}

void SystemEffectMgr::OnLeaveSystem(Client* client, uint32 systemID)
{
    if (client == nullptr)
        return;

    uint32 charID = client->GetCharacterID();
    auto it = m_clientEffects.find(charID);
    if (it == m_clientEffects.end())
        return;

    if (client->GetShipSE() == nullptr)
        return;

    InventoryItemRef ship = client->GetShipSE()->GetSelf();
    if (ship.get() == nullptr)
        return;

    RemoveEffect(ship, &it->second);
    m_clientEffects.erase(it);

    _log(SERVICE__MESSAGE, "SystemEffectMgr: Removed system effect from %s", client->GetName());
}

void SystemEffectMgr::ApplyEffect(InventoryItemRef ship, const WHEffectDef* effect)
{
    for (auto& [attrID, op, value] : effect->modifiers) {
        double current = ship->GetAttribute(attrID).get_float();
        if (std::isnan(current) || current <= 0.0)
            continue;

        double newVal = current;
        switch (op) {
            case 0: newVal = current * value; break;       // multiply
            case 1: newVal = current + value; break;       // add
            case 2: newVal = current * (1.0 + value / 100.0); break;  // percent
        }

        ship->SetAttribute(attrID, newVal);
        _log(SERVICE__MESSAGE, "SystemEffectMgr:   attr %u: %.2f -> %.2f", attrID, current, newVal);
    }
}

void SystemEffectMgr::RemoveEffect(InventoryItemRef ship, const WHEffectDef* effect)
{
    for (auto& [attrID, op, value] : effect->modifiers) {
        double current = ship->GetAttribute(attrID).get_float();
        if (std::isnan(current) || current <= 0.0)
            continue;

        // Reverse the modifier
        double newVal = current;
        switch (op) {
            case 0: newVal = current / value; break;                    // reverse multiply
            case 1: newVal = current - value; break;                     // reverse add
            case 2: newVal = current / (1.0 + value / 100.0); break;    // reverse percent
        }

        ship->SetAttribute(attrID, newVal);
        _log(SERVICE__MESSAGE, "SystemEffectMgr:   revert attr %u: %.2f -> %.2f", attrID, current, newVal);
    }
}
