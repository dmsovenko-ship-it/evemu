#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b)
: stationA(a), stationB(b), goToB(true), phase(0), timer(nullptr), refCount(0)
{
    timer = new Timer(20000);
    timer->Start(20000);
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

GPoint ConvoyAI::GetFormationOffset()
{
    // Chain: lead guard at front, hauler 2000m behind, tail 2000m behind hauler
    uint32 src = m_group->goToB ? m_group->stationA : m_group->stationB;
    uint32 dst = m_group->goToB ? m_group->stationB : m_group->stationA;
    GPoint srcPos = GetStationPosition(src);
    GPoint dstPos = GetStationPosition(dst);
    GVector dir(srcPos, dstPos);
    dir.normalize();
    double behind = -(double)m_index * 2000.0;
    return GPoint(dir.x * behind, 0, dir.z * behind);
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

    if (phase == 0) {
        // FormUp — gather near station in chain
        uint32 srcStation = m_group->goToB ? m_group->stationA : m_group->stationB;
        GPoint stationPos = GetStationPosition(srcStation);
        GPoint targetPos = stationPos + GetFormationOffset();
        double dist = m_npc->GetPosition().distance(targetPos);
        if (dist > 500.0)
            dest->GotoPoint(targetPos);
        // Leader waits for timer, then initiates departure
        if (m_index == 0 && m_group->timer->Check()) {
            // Start moving toward departure point
            GPoint depPoint = GetDeparturePoint();
            dest->GotoPoint(depPoint);
            m_group->phase = 1;
        }
    }
    else if (phase == 1) {
        // Departure — fly 150km from station in chain
        GPoint depPoint = GetDeparturePoint();
        if (m_index == 0) {
            // Leader heads to departure point
            double distToDeparture = m_npc->GetPosition().distance(depPoint);
            if (distToDeparture > 5000.0) {
                dest->GotoPoint(depPoint);
            } else {
                // Reached departure point — initiate warp
                GPoint targetPos = GetStationPosition(targetStation);
                GVector dir(m_npc->GetPosition(), targetPos);
                dir.normalize();
                dest->WarpTo(targetPos - (dir * 18000.0));
                m_group->phase = 2;
            }
        } else {
            // Followers follow the preceding ship
            NPC* lead = m_group->members[m_index - 1];
            if (lead != nullptr && !lead->IsDead() && lead->DestinyMgr() != nullptr) {
                if (!lead->DestinyMgr()->IsWarping())
                    dest->Follow(lead, 2000);
            }
        }
    }
    else if (phase == 2) {
        // WarpOut — arrived at destination
        GPoint stationPos = GetStationPosition(targetStation);
        double dist = m_npc->GetPosition().distance(stationPos);
        if (dist < 100000.0) {
            bool isLast = (m_index == m_group->members.size() - 1);
            if (isLast) {
                m_group->phase = 3;
                m_group->timer->Start(120000); // wait 2 min
            }
        }
    }
    else if (phase == 3) {
        // Waiting — then return trip
        if (m_index == 0 && m_group->timer->Check()) {
            m_group->goToB = !m_group->goToB;
            m_group->phase = 0;
            m_group->timer->Start(20000);
        }
    }
}