#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyGroup::ConvoyGroup(uint32 a, uint32 b)
: stationA(a), stationB(b), goToB(true), phase(0), phaseTimer(nullptr), attackTimer(nullptr), refCount(0)
{
    phaseTimer = new Timer(15000);
    phaseTimer->Start(15000);
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
        // FormUp — brief pause at departure point, then warp
        dest->SetSpeedFraction(0.0f);
        if (m_index == 0 && m_group->phaseTimer->Check()) {
            // Initiate group warp
            for (NPC* member : m_group->members) {
                if (member != nullptr && !member->IsDead() && member->DestinyMgr() != nullptr) {
                    GPoint targetPos = GetStationPosition(targetStation);
                    GVector dir(member->GetPosition(), targetPos);
                    dir.normalize();
                    member->DestinyMgr()->WarpTo(targetPos - (dir * 18000.0));
                }
            }
            m_group->phase = 1;
        }
    }
    else if (phase == 1) {
        // Warping — arrived at destination station
        GPoint stationPos = GetStationPosition(targetStation);
        double dist = m_npc->GetPosition().distance(stationPos);
        if (dist < 100000.0) {
            bool isLast = (m_index == m_group->members.size() - 1);
            if (isLast) {
                m_group->phase = 2;
                m_group->phaseTimer->Start(120000);
            }
        }
    }
    else if (phase == 2) {
        // Waiting — then return trip
        if (m_index == 0 && m_group->phaseTimer->Check()) {
            m_group->goToB = !m_group->goToB;
            m_group->phase = 0;
            m_group->phaseTimer->Start(15000);
        }
    }
}