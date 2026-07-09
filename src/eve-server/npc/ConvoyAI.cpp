#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "system/DestinyManager.h"
#include "system/Damage.h"
#include "system/SystemManager.h"
#include "system/cosmicMgrs/CivilianMgr.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b, bool sameCorpFlag)
: stationA(a), stationB(b), destSystemID(0), sourceGateID(0), destGateID(0),
  goToB(true), sameCorp(sameCorpFlag), phase(0), factionID(0),
  phaseTimer(nullptr), attackTimer(nullptr), transitTimer(nullptr), refCount(0)
{
}

ConvoyGroup::~ConvoyGroup()
{
    SafeDelete(phaseTimer);
    SafeDelete(attackTimer);
    SafeDelete(transitTimer);
    members.clear();
}

void ConvoyGroup::SetAttacked(SystemEntity* attacker)
{
    if (attackTimer == nullptr)
        attackTimer = new Timer(30000);
    attackTimer->Start(30000);
    WakeUpAll(attacker);
}

void ConvoyGroup::WakeUpAll(SystemEntity* attacker)
{
    for (NPC* npc : members) {
        if (npc != nullptr && !npc->IsDead() && npc->GetAIMgr() != nullptr) {
            npc->GetAIMgr()->WakeUp();
            if (attacker != nullptr) {
                npc->GetAIMgr()->Targeted(attacker);
                npc->GetAIMgr()->StartAttackCycle(2000);
                // Direct retaliation — bypass NPCAI entirely
                float dmgMult = 2.0f;
                Damage d(npc, npc->GetSelf(), npc->GetKinetic() * dmgMult,
                         npc->GetThermal() * dmgMult, npc->GetEM() * dmgMult,
                         npc->GetExplosive() * dmgMult, 1.0f, EVEEffectID::targetAttack);
                attacker->ApplyDamage(d);
            }
        }
    }
}

ConvoyAI::ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx)
: m_npc(who), m_group(group), m_index(idx), m_startTimer(nullptr), m_transferRequested(false)
{
    m_group->refCount++;
    uint32 interval = 15000 + MakeRandomInt(0, 30000);
    m_startTimer = new Timer(interval * (idx + 1));
    m_startTimer->Start(interval * (idx + 1));
}

ConvoyAI::~ConvoyAI()
{
    SafeDelete(m_startTimer);
    if (m_group != nullptr) {
        m_group->refCount--;
        if (m_group->refCount == 0)
            SafeDelete(m_group);
    }
}

GPoint ConvoyAI::GetStationPosition(uint32 stationID)
{
    SystemManager* sys = m_npc->SystemMgr();
    if (sys == nullptr) return GPoint(0, 0, 0);
    SystemEntity* se = sys->GetSE(stationID);
    if (se == nullptr) return GPoint(0, 0, 0);
    return se->GetPosition();
}

GPoint ConvoyAI::GetDeparturePoint()
{
    uint32 src = m_group->goToB ? m_group->stationA : m_group->stationB;
    uint32 dst = m_group->goToB ? m_group->stationB : m_group->stationA;
    GPoint srcPos = GetStationPosition(src);
    GPoint dstPos = GetStationPosition(dst);
    GVector dir(srcPos, dstPos);
    dir.normalize();
    GPoint dep = srcPos + (dir * 150000.0);
    double behind = (double)m_index * 2500.0;
    return GPoint(dep.x - dir.x * behind, dep.y, dep.z - dir.z * behind);
}

void ConvoyAI::ResetAfterTransit()
{
    // Reset state after cross-system jump complete
    m_group->phase = 0;
    m_group->goToB = !m_group->goToB;
    m_transferRequested = false;

    // Restart staggered departure timer
    SafeDelete(m_startTimer);
    uint32 interval = 15000 + MakeRandomInt(0, 30000);
    m_startTimer = new Timer(interval * (m_index + 1));
    m_startTimer->Start(interval * (m_index + 1));
}

void ConvoyAI::Process()
{
    if (m_npc == nullptr || m_npc->DestinyMgr() == nullptr || m_npc->IsDead())
        return;

    DestinyManager* dest = m_npc->DestinyMgr();
    if (dest->IsWarping())
        return;

    int8 phase = m_group->phase;
    uint32 targetStation = m_group->goToB ? m_group->stationB : m_group->stationA;
    GPoint depPoint = GetDeparturePoint();

    if (phase == 0) {
        if (m_startTimer->Enabled() && !m_startTimer->Check())
            return;
        if (m_index == 0) {
            dest->GotoPoint(depPoint);
        } else {
            NPC* lead = m_group->members[m_index - 1];
            if (lead != nullptr && !lead->IsDead())
                dest->Follow(lead, 2500);
        }
        if (m_index > 0 && !m_startTimer->Enabled())
            m_group->phase = 1;
        return;
    }

    if (phase == 1) {
        if (m_index == 0) {
            double dist = m_npc->GetPosition().distance(depPoint);
            if (dist > 5000.0)
                dest->GotoPoint(depPoint);
        } else {
            NPC* lead = m_group->members[m_index - 1];
            if (lead != nullptr && !lead->IsDead())
                dest->Follow(lead, 2500);
        }
        if (m_index == m_group->members.size() - 1) {
            double dist = m_npc->GetPosition().distance(depPoint);
            if (dist < 5000.0) {
                for (NPC* member : m_group->members) {
                    if (member != nullptr && !member->IsDead() && member->DestinyMgr() != nullptr) {
                        GPoint targetPos = GetStationPosition(targetStation);
                        GVector dir(member->GetPosition(), targetPos);
                        dir.normalize();
                        member->DestinyMgr()->WarpTo(targetPos - (dir * 18000.0));
                    }
                }
                m_group->phase = 2;
            }
        }
        return;
    }

    if (phase == 2) {
        double dist = m_npc->GetPosition().distance(GetStationPosition(targetStation));
        if (dist < 100000.0) {
            if (m_index == m_group->members.size() - 1) {
                // Cross-system: arriving at stargate triggers gate jump
                if (m_group->IsCrossSystem()) {
                    m_group->phase = 4;
                    m_group->transitTimer = new Timer(MakeRandomInt(30000, 60000));
                    m_group->transitTimer->Start(MakeRandomInt(30000, 60000));
                } else {
                    m_group->phase = 3;
                    m_group->phaseTimer = new Timer(120000);
                    m_group->phaseTimer->Start(120000);
                }
            }
        }
        return;
    }

    if (phase == 3) {
        if (m_index == 0 && m_group->phaseTimer != nullptr && m_group->phaseTimer->Check()) {
            SafeDelete(m_group->phaseTimer);
            m_group->goToB = !m_group->goToB;
            m_group->phase = 0;
            uint32 interval = 15000 + MakeRandomInt(0, 30000);
            m_startTimer->Start(interval * (m_index + 1));
        }
        return;
    }

    // Phase 4: Gate jump — remove from current system, transfer to destination
    if (phase == 4) {
        if (m_index == 0 && !m_transferRequested) {
            m_transferRequested = true;
            sCivMgr.TransferCrossSystem(m_group);
        }
        return;
    }
}