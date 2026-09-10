
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

private:
    StructureSE* m_pWeapon;
    TowerSE* m_pTower;

    uint32 m_targetID;
    int64  m_lastTargetScan;
    int64  m_lastAttackTime;

    bool m_active;
};


#endif  // EVEMU_POS_POS_AI_H_
