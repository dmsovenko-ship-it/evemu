#include "eve-server.h"
#include "ship/modules/SmartBomb.h"

#include "system/SystemBubble.h"
#include "system/Damage.h"
#include "system/CrimeWatch.h"

SmartBomb::SmartBomb(ModuleItemRef mRef, ShipItemRef sRef)
: ActiveModule(mRef, sRef)
{
}

uint32 SmartBomb::DoCycle()
{
    // Call base DoCycle for capacitor consumption and standard checks
    uint32 cycleTime = ActiveModule::DoCycle();
    if (cycleTime < 100)
        return cycleTime;

    Client* pClient = m_shipRef->GetPilot();
    if (pClient == nullptr)
        return cycleTime;

    SystemEntity* pShipSE = pClient->GetShipSE();
    if (pShipSE == nullptr)
        return cycleTime;

    float range = GetAttribute(AttrEmpFieldRange).get_float();
    if (range < 1.0f)
        range = 5000.0f;

    float dmgMultiplier = GetAttribute(AttrSmartbombDamageMultiplier).get_float();
    if (dmgMultiplier < 0.01f) dmgMultiplier = 1.0f;

    GPoint myPos = pShipSE->GetPosition();
    SystemBubble* pBubble = pShipSE->SysBubble();
    if (pBubble == nullptr)
        return cycleTime;

    // Effects sent via ActivateCycle->ShowEffect; DoCycle only applies damage.

    // Damage players
    std::vector<Client*> players;
    pBubble->GetPlayers(players);
    for (auto target : players) {
        if (target == nullptr || target == pClient)
            continue;
        SystemEntity* pTargetSE = target->GetShipSE();
        if (pTargetSE == nullptr) continue;
        if (pTargetSE->DestinyMgr() == nullptr) continue;
        if (pTargetSE->DestinyMgr()->IsWarping()) continue;
        float dist = myPos.distance(pTargetSE->GetPosition());
        if (dist > range) continue;
        float falloff = 1.0f - (dist / range) * 0.5f;
        if (falloff < 0.1f) falloff = 0.1f;
        float em = GetAttribute(AttrEmDamage).get_float() * falloff;
        float therm = GetAttribute(AttrThermalDamage).get_float() * falloff;
        float kin = GetAttribute(AttrKineticDamage).get_float() * falloff;
        float exp = GetAttribute(AttrExplosiveDamage).get_float() * falloff;
        Damage splash(pShipSE, m_modRef, kin, therm, em, exp,
                      dmgMultiplier * falloff, m_effectID);
        pTargetSE->ApplyDamage(splash);
        // Trigger weapon timer and aggression for each player hit
        float sec = pClient->SystemMgr()->GetSystemSecurityRating();
        pClient->GetCrimeWatch()->OnWeaponFired();
        pClient->GetCrimeWatch()->OnAggression(target, sec);
    }
    // Damage NPC entities (sentry guns, NPC ships, etc.)
    std::map<uint32, SystemEntity*> allEntities;
    pBubble->GetAllEntities(allEntities);
    for (auto& [id, pTargetSE] : allEntities) {
        if (pTargetSE == nullptr || pTargetSE == pShipSE)
            continue;
        if (!pTargetSE->IsNPCSE() && !pTargetSE->IsSentrySE() && !pTargetSE->IsDroneSE() && !pTargetSE->IsProbeSE())
            continue;
        float dist = myPos.distance(pTargetSE->GetPosition());
        if (dist > range) continue;
        float falloff = 1.0f - (dist / range) * 0.5f;
        if (falloff < 0.1f) falloff = 0.1f;
        float em = GetAttribute(AttrEmDamage).get_float() * falloff;
        float therm = GetAttribute(AttrThermalDamage).get_float() * falloff;
        float kin = GetAttribute(AttrKineticDamage).get_float() * falloff;
        float exp = GetAttribute(AttrExplosiveDamage).get_float() * falloff;
        Damage splash(pShipSE, m_modRef, kin, therm, em, exp,
                      dmgMultiplier * falloff, m_effectID);
        pTargetSE->ApplyDamage(splash);
    }
    return cycleTime;
}
