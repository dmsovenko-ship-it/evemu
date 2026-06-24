#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b)
: stationA(a), stationB(b), goToB(true), phase(0), phaseTimer(nullptr), attackTimer(nullptr), refCount(0)
{
    phaseTimer = new Timer(30000);
    phaseTimer->Start(30000);
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
        // FormUp — all ships stop and wait for timer
        dest->SetSpeedFraction(0.0f);
        if (m_index == 0 && m_group->phaseTimer->Check()) {
            m_group->phase = 1;
        }
    }
    else if (phase == 1) {
        // Departure — ALL ships fly independently to the departure point (150km from station)
        GPoint dp = GetDeparturePoint();
        double distToDP = m_npc->GetPosition().distance(dp);

        if (distToDP > 10000.0) {
            // Not at departure point yet — fly there
            dest->GotoPoint(dp);
        }

        // Check if the LAST member reached departure point
        if (m_index == m_group->members.size() - 1 && distToDP < 10000.0) {
            // Last ship is in position — initiate group warp
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
    else if (phase == 2) {
        // WarpOut — arrived at destination station
        GPoint stationPos = GetStationPosition(targetStation);
        double dist = m_npc->GetPosition().distance(stationPos);
        if (dist < 100000.0) {
            bool isLast = (m_index == m_group->members.size() - 1);
            if (isLast) {
                m_group->phase = 3;
                m_group->phaseTimer->Start(120000);
            }
        }
    }
    else if (phase == 3) {
        // Waiting — then return trip
        if (m_index == 0 && m_group->phaseTimer->Check()) {
            m_group->goToB = !m_group->goToB;
            m_group->phase = 0;
            m_group->phaseTimer->Start(30000);
        }
    }
}