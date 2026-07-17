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

    float range = m_self->GetAttribute(AttrEmpFieldRange).get_float();
    if (range < 1.0f)
        range = 5000.0f;

    float baseDamage = m_self->GetAttribute(AttrDamage).get_float();
    float dmgMultiplier = m_self->GetAttribute(AttrSmartbombDamageMultiplier).get_float();
    if (dmgMultiplier < 0.01f) dmgMultiplier = 1.0f;
    float totalDamage = baseDamage * dmgMultiplier;

    GPoint myPos = pShipSE->GetPosition();
    SystemBubble* pBubble = pShipSE->SysBubble();
    if (pBubble == nullptr)
        return 0;

    // Send smartbomb FX to the bubble
    OnSpecialFX14 fx;
        fx.entityID = pShipSE->GetID();
        fx.guid = "effects.SmartBomb";
        fx.isOffensive = 1;
        fx.start = 1;
        fx.active = 1;
        fx.duration = (uint32)(m_cycleTime * 1000);
        fx.repeat = 0;
        fx.startTime = GetFileTimeNow();
    PyTuple* t = fx.Encode();
    pBubble->BubblecastDestinyEvent(&t, "SmartBomb");

    // Apply splash damage
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
        // Falloff: 100% at center, 50% at edge
        float falloff = 1.0f - (dist / range) * 0.5f;
        if (falloff < 0.1f) falloff = 0.1f;
        uint32 dmg = (uint32)(totalDamage * falloff);
        // Simple shield damage for now
        InventoryItemRef targetItem = pTargetSE->GetSelf();
        double shield = targetItem->GetAttribute(AttrShieldCharge).get_float();
        targetItem->SetAttribute(AttrShieldCharge, std::max(0.0, shield - dmg), true);
    }
    return 0;
}
