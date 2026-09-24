
/**
 * @name POS_AI.h
 *   Class for POS Weapon Artificial Intelligence.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */


#ifndef EVEMU_POS_POS_AI_H_
#define EVEMU_POS_POS_AI_H_


class TowerSE;
class StructureSE;
class SystemEntity;

class POS_AI
{
public:
    POS_AI(StructureSE* pWeapon);
    ~POS_AI();

    void Process();

    void                        TargetLost(uint32 entityID);

protected:
    void FindTarget();
    void FireWeapon(uint32 targetID);
    void LaunchMissile(uint32 typeID, SystemEntity* pTarget);
    void ReleaseWeb();   // remove the stasis web from m_webTargetID (symmetric WebbedMe(false))
    void ReleaseDamp();  // undo the sensor damp applied to m_dampTargetID (symmetric restore)

private:
    StructureSE* m_pWeapon;
    TowerSE* m_pTower;

    uint32 m_targetID;
    int64  m_lastTargetScan;
    int64  m_lastAttackTime;

    bool   m_webApplied = false;   // stasis web currently applied to m_webTargetID
    uint32 m_webTargetID = 0;

    bool   m_dampApplied = false;  // sensor damp currently applied to m_dampTargetID
    uint32 m_dampTargetID = 0;
    float  m_dampSavedRange = 0.0f;   // target's base maxTargetRange before the damp
    float  m_dampSavedScanRes = 0.0f; // target's base scanResolution before the damp

    bool m_active;
};


#endif  // EVEMU_POS_POS_AI_H_
