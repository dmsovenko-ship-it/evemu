#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b, bool sameCorpFlag)
: stationA(a), stationB(b), goToB(true), sameCorp(sameCorpFlag),
  phase(0), phaseTimer(nullptr), attackTimer(nullptr), refCount(0)
{
}

ConvoyGroup::~ConvoyGroup()
{
    SafeDelete(phaseTimer);
    SafeDelete(attackTimer);
    members.clear();
}

void ConvoyGroup::SetAttacked()
{
    if (attackTimer == nullptr)
        attackTimer = new Timer(30000);
    attackTimer->Start(30000);
}

ConvoyAI::ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx)
: m_npc(who), m_group(group), m_index(idx), m_startTimer(nullptr)
{
    m_group->refCount++;
    // Each ship starts with a staggered delay: 15-45s * (index + 1)
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
    return srcPos + (dir * 150000.0);
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

    // Phase 0: waiting for staggered start timer
    if (phase == 0) {
        if (!m_startTimer->Enabled()) {
            // Timer already fired — fly to departure point (catches stragglers after warp)
            dest->GotoPoint(depPoint);
        } else if (m_startTimer->Check()) {
            // Timer just expired — start departure
            dest->GotoPoint(depPoint);
            if (m_index > 0)
                m_group->phase = 1;
        }
        return;
    }

    // Phase 1: en route to departure point
    if (phase == 1) {
        double dist = m_npc->GetPosition().distance(depPoint);
        if (dist > 5000.0)
            dest->GotoPoint(depPoint);
        // Last ship arrival triggers group warp
        if (m_index == m_group->members.size() - 1 && dist < 5000.0) {
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
        return;
    }

    // Phase 2: arrived at destination
    if (phase == 2) {
        double dist = m_npc->GetPosition().distance(GetStationPosition(targetStation));
        if (dist < 100000.0) {
            if (m_index == m_group->members.size() - 1) {
                m_group->phase = 3;
                m_group->phaseTimer = new Timer(120000);
                m_group->phaseTimer->Start(120000);
            }
        }
        return;
    }

    // Phase 3: waiting, then return trip
    if (phase == 3) {
        if (m_index == 0 && m_group->phaseTimer != nullptr && m_group->phaseTimer->Check()) {
            SafeDelete(m_group->phaseTimer);
            m_group->goToB = !m_group->goToB;
            m_group->phase = 0;
            uint32 interval = 15000 + MakeRandomInt(0, 30000);
            m_startTimer->Start(interval * (m_index + 1));
        }
    }
}