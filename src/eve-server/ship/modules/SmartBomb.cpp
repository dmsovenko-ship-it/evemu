#include "eve-server.h"
#include "ship/modules/SmartBomb.h"

#include "system/SystemBubble.h"
#include "system/Damage.h"

SmartBomb::SmartBomb(ModuleItemRef mRef, ShipItemRef sRef)
: ActiveModule(mRef, sRef)
{
}

uint32 SmartBomb::DoCycle()
{
    Client* pClient = m_shipRef->GetPilot();
    if (pClient == nullptr)
        return 0;

    SystemEntity* pShipSE = pClient->GetShipSE();
    if (pShipSE == nullptr)
        return 0;

    float range = GetAttribute(AttrEmpFieldRange).get_float();
    if (range < 1.0f)
        range = 5000.0f;

    float dmgMultiplier = GetAttribute(AttrSmartbombDamageMultiplier).get_float();
    if (dmgMultiplier < 0.01f) dmgMultiplier = 1.0f;

    GPoint myPos = pShipSE->GetPosition();
    SystemBubble* pBubble = pShipSE->SysBubble();
    if (pBubble == nullptr)
        return 0;

    uint32 cycleTime = GetRemainingCycleTimeMS();
    if (cycleTime < 100) {
        // First cycle — read the module's cycle speed attribute
        EvilNumber speed;
        if (m_modRef->HasAttribute(AttrSpeed, speed))
            cycleTime = speed.get_uint32();
        else if (m_modRef->HasAttribute(AttrDuration, speed))
            cycleTime = speed.get_uint32();
        else
            cycleTime = 5000;
    }

    m_destinyMgr->SendSpecialEffect(pShipSE->GetID(), pShipSE->GetID(), m_modRef->typeID(),
                                    0, 0, "effects.SmartBomb", true, true, true, cycleTime, 0);

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
    }
    return cycleTime;
}
