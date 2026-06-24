#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b)
: stationA(a), stationB(b), goToB(true), phase(0), timer(nullptr), refCount(0)
{
    timer = new Timer(15000);
    timer->Start(15000);
}

ConvoyGroup::~ConvoyGroup()
{
    SafeDelete(timer);
    members.clear();
}

ConvoyAI::ConvoyAI(NPC* who, ConvoyGroup* group, uint32 idx)
: m_npc(who), m_group(group), m_index(idx)
{
    m_group->refCount++;
}

ConvoyAI::~ConvoyAI()
{
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

void ConvoyAI::Process()
{
    if (m_npc == nullptr || m_npc->DestinyMgr() == nullptr || m_npc->IsDead())
        return;

    DestinyManager* dest = m_npc->DestinyMgr();
    if (dest->IsWarping())
        return;

    uint32 targetStation = m_group->goToB ? m_group->stationB : m_group->stationA;

    if (m_group->phase == 0) {
        // FormUp — stop and wait for leader timer
        dest->SetSpeedFraction(0.0f);
        if (m_index == 0 && m_group->timer->Check()) {
            // Leader initiates warp for all members
            GPoint stationPos = GetStationPosition(targetStation);
            GPoint warpDest = stationPos;
            GVector dir(m_npc->GetPosition(), stationPos);
            dir.normalize();
            dest->WarpTo(stationPos - (dir * 18000.0));
            m_group->phase = 1;
        }
    }
    else if (m_group->phase == 1) {
        // WarpOut — all members warp if not already there
        GPoint stationPos = GetStationPosition(targetStation);
        double dist = m_npc->GetPosition().distance(stationPos);

        if (dist > 200000.0) {
            // Not at destination yet — initiate warp
            GVector dir(m_npc->GetPosition(), stationPos);
            dir.normalize();
            dest->WarpTo(stationPos - (dir * 18000.0));
        } else if (dist < 100000.0) {
            // Arrived at destination
            bool isLast = (m_index == m_group->members.size() - 1);
            if (isLast) {
                m_group->phase = 2;
                m_group->timer->Start(60000);
            }
        }
    }
    else if (m_group->phase == 2) {
        // Waiting at station — timer controls return
        if (m_index == 0 && m_group->timer->Check()) {
            m_group->goToB = !m_group->goToB;
            m_group->phase = 1; // warp directly to other station
            // Leader initiates warp
            uint32 nextTarget = m_group->goToB ? m_group->stationB : m_group->stationA;
            GPoint nextPos = GetStationPosition(nextTarget);
            GVector dir(m_npc->GetPosition(), nextPos);
            dir.normalize();
            dest->WarpTo(nextPos - (dir * 18000.0));
        }
    }
}