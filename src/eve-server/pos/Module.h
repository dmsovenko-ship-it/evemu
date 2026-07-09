
/**
 * @name Module.h
 *   General Class for miscellanous POS Modules.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */


#ifndef EVEMU_POS_MODULE_H_
#define EVEMU_POS_MODULE_H_


#include "DataClasses.h"
#include "pos/Structure.h"


class ModuleSE
: public StructureSE
{
public:
    ModuleSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& data);
    virtual ~ModuleSE()                                 { /* do nothing here */ }

    /* class type pointer querys. */
    virtual ModuleSE*           GetModuleSE()           { return this; }

    /* class type tests. */
    virtual bool                IsModuleSE()            { return true; }

    /* SystemEntity interface */
    virtual void                Process();

    /* virtual functions default to base class and overridden as needed */
    virtual void                Init();


};


class ReactorSE
: public StructureSE
{
public:
    ReactorSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& data);
    virtual ~ReactorSE();

    virtual ReactorSE*          GetReactorSE()          { return this; }
    virtual bool                IsReactorSE()           { return true; }
    virtual void                Process();
    virtual void                Init();
    virtual void                InitData();

    void                        AddConnection(EVEPOS::POS_Connections& conn);
    void                        ClearConnections();
    bool                        IsActive()              { return pData->IsActive(); }
    void                        SetActive(bool set)     { pData->SetActive(set); }
    ReactorData*                GetReactorData()        { return pData; }

private:
    void                        ProcessReactionCycle();
    int32                       LookupReactionType();
    bool                        ConsumeInputs(int32 reactionTypeID);
    void                        ProduceOutputs(int32 reactionTypeID, int32 qty);

    ReactorData*                pData;
    Timer*                      m_cycleTimer;       // reaction cycle timer

};


#endif  // EVEMU_POS_MODULE_H_
