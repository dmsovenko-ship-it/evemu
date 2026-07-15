/**
 * @name TCU.h
 *   Class for Territorial Claim Units.
 *
 * @Author:         James
 * @date:   8 April 2021
 */


#ifndef EVEMU_POS_TCU_H_
#define EVEMU_POS_TCU_H_

#include "pos/Structure.h"

class TCUSE
: public StructureSE
{
public:
    TCUSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& fData);
    virtual ~TCUSE()                                  { /* do nothing here */ }

    virtual TCUSE*              GetTCUSE()            { return this; }

    virtual bool                isGlobal()              { return true; }
    virtual bool                IsTCUSE()               { return true; }
    virtual bool                IsOperSE()              { return true; }

    virtual void                Process();
    virtual void                SetOnline();
    virtual void                SetOffline();

    virtual void                Init();
    virtual void                Killed(Damage& damage);

private:
    int64                       m_claimTime;    // 0 = no pending claim, >0 = filetime when claim activates
    void                        FinalizeClaim();
};

#endif  // EVEMU_POS_TCU_H_
