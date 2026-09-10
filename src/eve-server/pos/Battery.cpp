
/**
 * @name Battery.cpp
 *   Class for POS Battery Modules.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */

/*
 * POS__ERROR
 * POS__WARNING
 * POS__MESSAGE
 * POS__DUMP
 * POS__DEBUG
 * POS__DESTINY
 * POS__SLIMITEM
 * POS__TRACE
 */


#include "pos/Battery.h"
#include "pos/POS_AI.h"


BatterySE::BatterySE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& data)
: StructureSE(structure, services, system, data),
  m_ai(nullptr)
{

}

BatterySE::~BatterySE()
{
    SafeDelete(m_ai);
}

void BatterySE::Init()
{
    StructureSE::Init();
    m_ai = new POS_AI(this);
}

void BatterySE::Process()
{
    /* called by EntityList::Process on every loop */
    /*  Enable base call to Process state changes  */
    StructureSE::Process();
    if (m_ai != nullptr)
        m_ai->Process();
}

void BatterySE::TargetLost(uint32 entityID)
{
    if (m_ai != nullptr)
        m_ai->TargetLost(entityID);
}
