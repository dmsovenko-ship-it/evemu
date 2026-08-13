#include "eve-server.h"

#include "npc/SleeperAI.h"
#include "npc/NPC.h"
#include "system/SystemBubble.h"
#include "system/SystemEntity.h"
#include "Client.h"

SleeperAIMgr::SleeperAIMgr(NPC* npc)
: NPCAIMgr(npc), m_pNPC(npc)
{
    InventoryItemRef self = npc->GetSelf();

    m_remoteRepairRange    = self->GetAttribute(AttrNpcAssistanceRange).get_float();
    if (m_remoteRepairRange < 1.0f) m_remoteRepairRange = 15000.0f;

    m_remoteRepairAmount   = self->GetAttribute(AttrNpcRemoteArmorRepairAmount).get_float();
    if (m_remoteRepairAmount < 1.0f) {
        m_remoteRepairAmount = self->GetAttribute(AttrNpcRemoteShieldBoostAmount).get_float();
        m_remoteRepairArmor  = false;
    } else {
        m_remoteRepairArmor  = true;
    }

    m_remoteRepairDuration = self->GetAttribute(AttrNpcRemoteArmorRepairDuration).get_float();
    if (m_remoteRepairDuration < 1.0f)
        m_remoteRepairDuration = self->GetAttribute(AttrNpcRemoteShieldBoostDuration).get_float();
    if (m_remoteRepairDuration < 1.0f)
        m_remoteRepairDuration = 5000.0f;

    m_remoteRepairChance   = self->GetAttribute(AttrNpcRemoteArmorRepairChance).get_float();
    if (m_remoteRepairChance < 0.01f)
        m_remoteRepairChance = self->GetAttribute(AttrNpcRemoteShieldBoostChance).get_float();

    m_remoteRepairThreshold = self->GetAttribute(AttrNpcRemoteArmorRepairThreshold).get_float();
    if (m_remoteRepairThreshold < 1.0f)
        m_remoteRepairThreshold = self->GetAttribute(AttrNpcRemoteShieldBoostThreshold).get_float();
    if (m_remoteRepairThreshold < 1.0f)
        m_remoteRepairThreshold = 30.0f;

    m_remoteRepairMaxTargets = self->GetAttribute(AttrNpcRemoteArmorRepairMaxTargets).get_uint32();
    if (m_remoteRepairMaxTargets < 1)
        m_remoteRepairMaxTargets = self->GetAttribute(AttrNpcRemoteShieldBoostMaxTargets).get_uint32();
    if (m_remoteRepairMaxTargets < 1)
        m_remoteRepairMaxTargets = 1;

    m_neutRange    = self->GetAttribute(AttrEntityCapacitorDrainMaxRange).get_float();
    if (m_neutRange < 1.0f) m_neutRange = 15000.0f;

    m_neutAmount   = self->GetAttribute(AttrEntityCapacitorDrainAmount).get_float();
    if (m_neutAmount < 1.0f) m_neutAmount = 100.0f;

    m_neutDuration = self->GetAttribute(AttrEntityCapacitorDrainDuration).get_float();
    if (m_neutDuration < 1.0f) m_neutDuration = 5000.0f;

    m_neutChance   = self->GetAttribute(AttrEntityCapacitorDrainDurationChance).get_float();
    if (m_neutChance < 0.01f) m_neutChance = 0.5f;

    m_remoteRepairTimer.Start(static_cast<uint32_t>(m_remoteRepairDuration));
    m_neutTimer.Start(static_cast<uint32_t>(m_neutDuration));
    m_capCheckTimer.Start(5000);
    m_escalationCount = 0;

    _log(NPC__AI_TRACE, "SleeperAIMgr: %s(%u) initialized: repairRange=%.0f repairAmt=%.0f "
         "neutRange=%.0f neutAmt=%.0f",
         m_pNPC->GetName(), m_pNPC->GetID(),
         m_remoteRepairRange, m_remoteRepairAmount, m_neutRange, m_neutAmount);
}

void SleeperAIMgr::Process()
{
    NPCAIMgr::Process();

    if (m_remoteRepairTimer.Check(true))
        RemoteRepair();

    if (m_neutTimer.Check(true)) {
        SystemEntity* target = m_pNPC->TargetMgr()->GetFirstTarget(false);
        if (target != nullptr && m_pNPC->GetPosition().distance(target->GetPosition()) <= m_neutRange)
            EnergyNeut(target);
    }

    // Check for capital ships every 5s for escalation
    if (m_capCheckTimer.Check(true))
        CheckCapitalEscalation();
}

void SleeperAIMgr::CheckCapitalEscalation()
{
    if (m_escalationCount >= 2) return;  // max 2 escalations per site
    if (m_pNPC->SysBubble() == nullptr) return;

    SpawnMgr* spawnMgr = m_pNPC->GetSpawnMgr();
    if (spawnMgr == nullptr) return;

    // Check for capital ships in this bubble
    std::vector<Client*> players;
    m_pNPC->SysBubble()->GetPlayers(players);

    bool hasCapital = false;
    for (auto p : players) {
        if (p == nullptr) continue;
        ShipItemRef ship = p->GetShip();
        if (ship.get() == nullptr) continue;
        uint16 gid = ship->groupID();
        // Capital ship groups: Carrier=547, Dreadnought=485, Supercarrier=659, CapIndustrial=883
        if (gid == 547 || gid == 485 || gid == 659 || gid == 883) {
            hasCapital = true;
            break;
        }
    }
    if (!hasCapital) return;

    // Determine guardian typeID based on this NPC's tier — use existing Sleeper
    // Defender types (strongest per tier) as escalation guardians:
    // Sleepless=30195, Awakened=30205, Emergent=30214
    uint16 guardianType = 30195;  // default Sleepless Safeguard
    uint16 gid = m_pNPC->GetSelf()->groupID();
    // Awakened sites → Awakened Preserver (30205)
    if (gid == 960 || gid == 984 || gid == 985) guardianType = 30205;
    // Emergent sites → Emergent Preserver (30214)
    if (gid == 961 || gid == 986 || gid == 987) guardianType = 30214;

    uint8 count = (m_escalationCount == 0) ? 6 : 8;
    m_escalationCount++;

    _log(NPC__AI_TRACE, "SleeperAIMgr: %s(%u) — capital escalation %u spawning %u guardians type %u",
         m_pNPC->GetName(), m_pNPC->GetID(), m_escalationCount, count, guardianType);

    for (uint8 i = 0; i < count; ++i) {
        GPoint pos = m_pNPC->GetPosition();
        pos.x += MakeRandomFloat() * 2000.0f - 1000.0f;
        pos.y += MakeRandomFloat() * 2000.0f - 1000.0f;
        pos.z += MakeRandomFloat() * 1000.0f - 500.0f;
        spawnMgr->DoSpawnForAnomaly(m_pNPC->SysBubble(), pos, 1, guardianType, false);
    }
}

void SleeperAIMgr::OnCapitalEntered()
{
    // Called from Bubble::Add when a capital ship enters — already handled by CheckCapitalEscalation timer
}

SystemEntity* SleeperAIMgr::FindRepairTarget()
{
    if (m_pNPC->SysBubble() == nullptr) return nullptr;

    std::map<uint32, SystemEntity*> entities;
    m_pNPC->SysBubble()->GetAllEntities(entities);

    SystemEntity* bestTarget = nullptr;
    float bestRatio = 1.0f;

    for (auto& [id, se] : entities) {
        if (se == nullptr || se == m_pNPC) continue;
        if (!se->IsNPCSE()) continue;

        float ratio = 1.0f;
        if (m_remoteRepairArmor) {
            float armorHP = se->GetSelf()->GetAttribute(AttrArmorHP).get_float();
            float armorDmg = se->GetSelf()->GetAttribute(AttrArmorDamage).get_float();
            if (armorHP > 0.0f)
                ratio = 1.0f - armorDmg / armorHP;
        } else {
            float shieldCap = se->GetSelf()->GetAttribute(AttrShieldCapacity).get_float();
            float shieldChg = se->GetSelf()->GetAttribute(AttrShieldCharge).get_float();
            if (shieldCap > 0.0f)
                ratio = shieldChg / shieldCap;
        }
        if (ratio < m_remoteRepairThreshold / 100.0f && ratio < bestRatio) {
            bestRatio = ratio;
            bestTarget = se;
        }
        if (bestTarget != nullptr && m_remoteRepairMaxTargets <= 1)
            break;
    }
    return bestTarget;
}

void SleeperAIMgr::RemoteRepair()
{
    SystemEntity* repairTarget = FindRepairTarget();
    if (repairTarget == nullptr) return;

    DestinyManager* dest = m_pNPC->DestinyMgr();
    if (dest == nullptr) return;

    if (m_remoteRepairArmor) {
        EvilNumber dmg = repairTarget->GetSelf()->GetAttribute(AttrArmorDamage);
        dmg -= m_remoteRepairAmount;
        if (dmg < EvilZero) dmg = EvilZero;
        repairTarget->GetSelf()->SetAttribute(AttrArmorDamage, dmg);
        dest->SendSpecialEffect(m_pNPC->GetID(), m_pNPC->GetID(), m_pNPC->GetTypeID(),
                                repairTarget->GetID(), 0, "effects.RemoteArmourRepair",
                                0, 1, 1, static_cast<uint32>(m_remoteRepairDuration), 0);
    } else {
        EvilNumber shield = repairTarget->GetSelf()->GetAttribute(AttrShieldCharge);
        shield += m_remoteRepairAmount;
        EvilNumber shieldCap = repairTarget->GetSelf()->GetAttribute(AttrShieldCapacity);
        if (shield > shieldCap) shield = shieldCap;
        repairTarget->GetSelf()->SetAttribute(AttrShieldCharge, shield);
        dest->SendSpecialEffect(m_pNPC->GetID(), m_pNPC->GetID(), m_pNPC->GetTypeID(),
                                repairTarget->GetID(), 0, "effects.ShieldTransfer",
                                0, 1, 1, static_cast<uint32>(m_remoteRepairDuration), 0);
    }
    repairTarget->SendDamageStateChanged();
}

void SleeperAIMgr::EnergyNeut(SystemEntity* pTarget)
{
    if (pTarget == nullptr) return;
    if (MakeRandomFloat() > m_neutChance) return;

    EvilNumber cap = pTarget->GetSelf()->GetAttribute(AttrCapacitorCharge);
    cap -= m_neutAmount;
    if (cap < EvilZero) cap = EvilZero;
    pTarget->GetSelf()->SetAttribute(AttrCapacitorCharge, cap);

    DestinyManager* dest = m_pNPC->DestinyMgr();
    if (dest != nullptr)
        dest->SendSpecialEffect(m_pNPC->GetID(), m_pNPC->GetID(), m_pNPC->GetTypeID(),
                                pTarget->GetID(), 0, "effects.EnergyDestabilization",
                                0, 1, 1, static_cast<uint32>(m_neutDuration), 0);
}
