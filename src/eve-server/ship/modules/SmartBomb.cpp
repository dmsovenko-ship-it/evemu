#include "eve-server.h"
#include "ship/modules/SmartBomb.h"

#include "system/SystemBubble.h"

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

    float baseDamage = 0.0f;
    if (m_chargeRef.get() != nullptr)
        baseDamage = m_chargeRef->GetAttribute(AttrDamage).get_float();
    if (baseDamage < 1.0f)
        baseDamage = GetAttribute(AttrDamage).get_float();
    float dmgMultiplier = GetAttribute(AttrSmartbombDamageMultiplier).get_float();
    if (dmgMultiplier < 0.01f) dmgMultiplier = 1.0f;
    float totalDamage = baseDamage * dmgMultiplier;

    GPoint myPos = pShipSE->GetPosition();
    SystemBubble* pBubble = pShipSE->SysBubble();
    if (pBubble == nullptr)
        return 0;

    uint32 cycleTime = GetRemainingCycleTimeMS();
    if (cycleTime < 100) cycleTime = 5000;

    m_destinyMgr->SendSpecialEffect(pShipSE->GetID(), m_modRef->itemID(), m_modRef->typeID(),
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
        uint32 dmg = (uint32)(totalDamage * falloff);
        InventoryItemRef targetItem = pTargetSE->GetSelf();
        double shield = targetItem->GetAttribute(AttrShieldCharge).get_float();
        targetItem->SetAttribute(AttrShieldCharge, std::max(0.0, shield - dmg), true);
    }
    return 0;
}
