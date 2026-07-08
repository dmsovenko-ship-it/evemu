
/**
 * @name POS_AI.cpp
 *   Class for POS Weapon Artificial Intelligence.
 *
 * @Author:          Allan
 * @date:   28 December 17
 */


#include <cmath>

#include "Client.h"
#include "EntityList.h"
#include "StaticDataMgr.h"
#include "pos/POS_AI.h"
#include "pos/Tower.h"
#include "pos/Weapon.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"
#include "system/SystemEntity.h"
#include "system/SystemManager.h"


POS_AI::POS_AI(WeaponSE* pWeapon)
: m_pWeapon(pWeapon),
  m_pTower(nullptr),
  m_targetID(0),
  m_lastTargetScan(0),
  m_lastAttackTime(0),
  m_active(false)
{
}

POS_AI::~POS_AI() = default;

void POS_AI::Process()
{
    if (m_pWeapon == nullptr)
        return;

    if (m_pWeapon->GetState() < EVEPOS::StructureState::Online) {
        m_active = false;
        return;
    }

    if (m_pTower == nullptr) {
        SystemManager* pSystem = m_pWeapon->SystemMgr();
        if (pSystem == nullptr)
            return;
        SystemEntity* pSE = pSystem->GetSE(m_pWeapon->GetTowerID());
        if (pSE == nullptr)
            return;
        m_pTower = pSE->GetTowerSE();
        if (m_pTower == nullptr)
            return;
        m_active = true;
    }

    if (!m_active)
        return;

    int64 now = GetFileTimeNow();

    if ((now - m_lastTargetScan) > (EvE::Time::Second * 5)) {
        m_lastTargetScan = now;
        if (m_targetID == 0)
            FindTarget();
    }

    if (m_targetID != 0) {
        SystemEntity* pTarget = m_pWeapon->SystemMgr()->GetSE(m_targetID);
        if (pTarget == nullptr) {
            m_targetID = 0;
            return;
        }

        float range = m_pWeapon->GetPosition().distance(pTarget->GetPosition());
        float maxRange = m_pWeapon->GetSelf()->GetAttribute(AttrMaxRange).get_float();
        float falloff = m_pWeapon->GetSelf()->GetAttribute(AttrFalloff).get_float();
        float sightRange = m_pWeapon->GetSelf()->GetAttribute(AttrProximityRange).get_float();

        if (range > (maxRange + falloff) and range > sightRange) {
            m_targetID = 0;
            return;
        }

        uint32 attackDelay = m_pWeapon->GetSelf()->GetAttribute(AttrSpeed).get_uint32();
        if (attackDelay < 1000)
            attackDelay = 15000;

        if ((now - m_lastAttackTime) > (int64)attackDelay * EvE::Time::mSecond) {
            m_lastAttackTime = now;
            FireWeapon(m_targetID);
        }
    }
}

void POS_AI::FindTarget()
{
    SystemBubble* pBubble = m_pWeapon->SysBubble();
    if (pBubble == nullptr)
        return;

    float sightRange = m_pWeapon->GetSelf()->GetAttribute(AttrProximityRange).get_float();
    if (sightRange < 1.0f)
        sightRange = 250000.0f;

    std::map<uint32, SystemEntity*> entities;
    pBubble->GetEntities(entities);

    SystemEntity* bestTarget = nullptr;
    float bestRange = 0.0f;

    for (auto& cur : entities) {
        SystemEntity* pEntity = cur.second;
        if (!IsValidTargetInternal(pEntity, m_pTower))
            continue;

        float dist = m_pWeapon->GetPosition().distance(pEntity->GetPosition());
        if (dist > sightRange)
            continue;

        if (bestTarget == nullptr or dist < bestRange) {
            bestTarget = pEntity;
            bestRange = dist;
        }
    }

    if (bestTarget != nullptr)
        m_targetID = bestTarget->GetID();
}

static bool IsValidTargetInternal(SystemEntity* pEntity, TowerSE* pTower)
{
    if (pEntity == nullptr)
        return false;

    if (!pEntity->HasPilot())
        return false;

    Client* pClient = pEntity->GetPilot();
    if (pClient == nullptr)
        return false;

    if (pClient->IsCloaked())
        return false;

    if (pTower != nullptr) {
        if (pClient->GetCorporationID() == pTower->GetCorporationID())
            return false;
    }

    return true;
}

void POS_AI::FireWeapon(uint32 targetID)
{
    SystemManager* pSystem = m_pWeapon->SystemMgr();
    if (pSystem == nullptr)
        return;

    SystemEntity* pTarget = pSystem->GetSE(targetID);
    if (pTarget == nullptr)
        return;

    InventoryItemRef weaponRef = m_pWeapon->GetSelf();

    float dmgMult = weaponRef->GetAttribute(AttrDamageMultiplier).get_float();
    if (dmgMult < 0.01f)
        dmgMult = 1.0f;

    float range = m_pWeapon->GetPosition().distance(pTarget->GetPosition());
    float maxRange = weaponRef->GetAttribute(AttrMaxRange).get_float();
    float falloff = weaponRef->GetAttribute(AttrFalloff).get_float();
    float hitChance = 0.8f;
    if (range > maxRange and falloff > 0.0f) {
        float falloffRatio = (range - maxRange) / falloff;
        hitChance *= powf(0.5f, falloffRatio * falloffRatio);
    }

    if (hitChance < 0.01f)
        return;

    Damage d(m_pWeapon, weaponRef, hitChance, 0);
    d *= dmgMult;

    bool killed = pTarget->ApplyDamage(d);
    _log(POS__MESSAGE, "POS_AI::FireWeapon() - %s(%u) fired at %s(%u)",
            m_pWeapon->GetName(), m_pWeapon->GetID(),
            pTarget->GetName(), pTarget->GetID());

    m_pWeapon->DestinyMgr()->SendSpecialEffect10(
            m_pWeapon->GetID(), pTarget->GetID(),
            "effects.Laser", 1, 1, 1);

    if (killed) {
        m_targetID = 0;
        m_lastTargetScan = 0;
    }
}

void POS_AI::TargetLost(uint32 entityID)
{
    if (entityID == m_targetID)
        m_targetID = 0;
}
