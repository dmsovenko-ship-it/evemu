#include "eve-server.h"
#include "npc/ConvoyAI.h"
#include "npc/NPC.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"

ConvoyAI::ConvoyAI(NPC* who, uint32 stationA, uint32 stationB)
: m_npc(who), m_state(Idle), m_stationA(stationA), m_stationB(stationB),
  m_goingToB(true), m_waitTimer(nullptr)
{
    m_waitTimer = new Timer(10000); // wait 10s before first warp
    m_waitTimer->Start(10000);
}

ConvoyAI::~ConvoyAI()
{
    SafeDelete(m_waitTimer);
}

void ConvoyAI::Process()
{
    if (m_npc == nullptr || m_npc->IsWarping() || m_npc->DestinyMgr() == nullptr)
        return;

    switch (m_state) {
        case Idle:
            if (m_waitTimer->Check()) {
                uint32 target = m_goingToB ? m_stationB : m_stationA;
                WarpToStation(target);
                m_state = Warping;
            }
            break;

        case Warping:
            // WarpComplete is called by NPC AI when warp finishes
            break;

        case Waiting:
            if (m_waitTimer->Check()) {
                m_goingToB = !m_goingToB;
                m_state = Idle;
                m_waitTimer->Start(15000);
            }
            break;
    }
}

void ConvoyAI::WarpToStation(uint32 stationID)
{
    if (m_npc->SystemMgr() == nullptr) return;
    SystemEntity* station = m_npc->SystemMgr()->GetSE(stationID);
    if (station == nullptr) return;
    GPoint pos = station->GetPosition();
    // Warp to a point near the station (15-20km out)
    GVector dir(m_npc->GetPosition(), pos);
    dir.normalize();
    GPoint warpTo = pos - (dir * 18000.0f);
    m_npc->DestinyMgr()->WarpTo(warpTo, 0);
}

void ConvoyAI::WarpComplete()
{
    m_state = Waiting;
    m_waitTimer->Start(60000); // wait 60s at station
}
