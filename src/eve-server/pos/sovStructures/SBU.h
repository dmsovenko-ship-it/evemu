/**
 * @name SBU.h
 *   Class for Sovereignty Blockade Units.
 *
 * @Author:         James
 * @date:   8 April 2021
 */


#ifndef EVEMU_POS_SBU_H_
#define EVEMU_POS_SBU_H_

#include "pos/Structure.h"

class SBUSE
: public StructureSE
{
public:
    SBUSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& fData);
    virtual ~SBUSE()                                  { /* do nothing here */ }

    /* class type pointer querys. */
    virtual SBUSE*              GetSBUSE()            { return this; }
    virtual bool                isGlobal()              { return true; }
    virtual bool                IsOperSE()              { return true; }

    /* class type tests. */
    virtual bool                IsSBUSE()             { return true; }

    /* SystemEntity interface */
    virtual void                Process();
    virtual void                SetOnline();
    virtual void                SetOffline();
    // Destroying an SBU re-evaluates the contest: if the surviving online SBUs no
    // longer exceed 50% of the system's stargates, the TCU becomes invulnerable
    // again and the system stops being contested.
    virtual void                Killed(Damage& damage);

    /* virtual functions default to base class and overridden as needed */
    virtual void                Init();

protected:

private:

    /* Class-specific functions */
    float GetGates();
    float GetSBUs();
    void MarkContested(uint32 systemID, bool contested);

};

#endif  // EVEMU_POS_SBU_H_
