
/**
 * @name POS_AI.h
 *   Class for POS Weapon Artificial Intelligence.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */


#ifndef EVEMU_POS_POS_AI_H_
#define EVEMU_POS_POS_AI_H_


#include "system/SystemEntity.h"

class TowerSE;
class WeaponSE;

class POS_AI
{
public:
    POS_AI(WeaponSE* pWeapon);
    ~POS_AI();

    void Process();

    void TargetLost(SystemEntity* who);

protected:
    void FindTarget();
    bool IsValidTarget(SystemEntity* pEntity);
    void FireWeapon(SystemEntity* pTarget);
    float GetRangeTo(SystemEntity* pTarget);

private:
    WeaponSE* m_pWeapon;
    TowerSE* m_pTower;

    uint32 m_targetID;
    int64  m_lastTargetScan;
    int64  m_lastAttackTime;

    bool m_active;
};


#endif  // EVEMU_POS_POS_AI_H_
