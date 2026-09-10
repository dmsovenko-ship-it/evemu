
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
#include "system/CrimeWatch.h"
#include "standing/StandingDB.h"
#include "tables/invGroups.h"
#include "pos/POS_AI.h"
#include "pos/Tower.h"
#include "ship/Ship.h"
#include "pos/Structure.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"
#include "system/SystemEntity.h"
#include "system/SystemManager.h"
#include "inventory/AttributeEnum.h"


POS_AI::POS_AI(StructureSE* pWeapon)
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

static bool IsValidTargetInternal(SystemEntity* pEntity, TowerSE* pTower, WeaponSE* pWeapon)
{
    if (pEntity == nullptr)
        return false;

    if (!pEntity->HasPilot())
        return false;

    Client* pClient = pEntity->GetPilot();
    if (pClient == nullptr)
        return false;

    // check if cloaked (via ship's destiny manager)
    ShipSE* pShip = pClient->GetShipSE();
    if (pShip != nullptr and pShip->DestinyMgr() != nullptr and pShip->DestinyMgr()->IsCloaked())
        return false;

    if (pTower != nullptr) {
        if (pClient->GetCorporationID() == pTower->GetCorporationID())
            return false;
        uint32 towerAlly = pTower->GetAllianceID();
        if (towerAlly != 0 && pClient->GetAllianceID() == towerAlly)
            return false;
    }

    // --- tower sentry settings: standings / war ------------------------------
    // The tower owner configures a standing threshold and whether to shoot at
    // war targets. A target at/below the threshold (or at war) is hostile even
    // in high-sec.
    bool hostileBySettings = false;
    if (pTower != nullptr) {
        float threshold = pTower->GetStanding();       // e.g. -5.0
        if (threshold < 0.0f) {
            uint32 towerOwner = pTower->GetCorporationID();
            double stChar = StandingDB::GetStanding(towerOwner, pClient->GetCharacterID());
            double stCorp = StandingDB::GetStanding(towerOwner, pClient->GetCorporationID());
            double stAlly = (pClient->GetAllianceID() != 0)
                          ? StandingDB::GetStanding(towerOwner, pClient->GetAllianceID()) : 0.0;
            double worst = stChar;
            if (stCorp < worst) worst = stCorp;
            if (stAlly < worst) worst = stAlly;
            if (worst <= threshold)
                hostileBySettings = true;
        }
        // At war with the target's corp/alliance → hostile.
        if (pTower->GetCorpWar() && pClient->GetCorporationID() != pTower->GetCorporationID())
            hostileBySettings = true;
    }

    // High-sec: POS guns may only engage hostiles (criminals / aggressors /
    // outlaws / standings-or-war hostiles). Low/null: any non-corp pilot.
    float sec = pWeapon->SystemMgr() != nullptr ? pWeapon->SystemMgr()->GetSystemSecurityRating() : 0.0f;
    if (sec >= 0.5f && !hostileBySettings) {
        CrimeWatch* cw = pClient->GetCrimeWatch();
        if (cw == nullptr)
            return false;
        if (!cw->IsCriminal() && !cw->IsAggressed() && !cw->IsOutlaw())
            return false;
    }

    return true;
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
        if (!IsValidTargetInternal(pEntity, m_pTower, m_pWeapon))
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

void POS_AI::FireWeapon(uint32 targetID)
{
    SystemManager* pSystem = m_pWeapon->SystemMgr();
    if (pSystem == nullptr)
        return;

    SystemEntity* pTarget = pSystem->GetSE(targetID);
    if (pTarget == nullptr)
        return;

    InventoryItemRef weaponRef = m_pWeapon->GetSelf();
    uint16 grp = weaponRef->groupID();

    float range = m_pWeapon->GetPosition().distance(pTarget->GetPosition());

    // --- EWAR batteries: apply the matching effect instead of damage ---------
    switch (grp) {
        case EVEDB::invGroups::Stasis_Webification_Battery: {
            if (pTarget->DestinyMgr() != nullptr)
                pTarget->DestinyMgr()->WebbedMe(weaponRef, true);
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), pTarget->GetID(),
                                                         "effects.ModifyTargetSpeed", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s webbed %s.", m_pWeapon->GetName(), pTarget->GetName());
            return;
        }
        case EVEDB::invGroups::Warp_Scrambling_Battery: {
            EvilNumber strength = weaponRef->HasAttribute(AttrWarpScrambleStrength)
                                ? weaponRef->GetAttribute(AttrWarpScrambleStrength) : EvilOne;
            pTarget->GetSelf()->SetAttribute(AttrWarpScrambleStatus, strength, true);
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), pTarget->GetID(),
                                                         "effects.WarpScramble", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s scrambled %s.", m_pWeapon->GetName(), pTarget->GetName());
            return;
        }
        case EVEDB::invGroups::Energy_Neutralizing_Battery: {
            float amount = weaponRef->HasAttribute(AttrEntityCapacitorDrainAmount)
                         ? weaponRef->GetAttribute(AttrEntityCapacitorDrainAmount).get_float() : 100.0f;
            EvilNumber cap = pTarget->GetSelf()->GetAttribute(AttrCapacitorCharge);
            cap -= amount;
            if (cap < EvilZero) cap = EvilZero;
            pTarget->GetSelf()->SetAttribute(AttrCapacitorCharge, cap, true);
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), pTarget->GetID(),
                                                         "effects.EnergyDestabilization", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s neutralized %.0f GJ from %s.", m_pWeapon->GetName(), amount, pTarget->GetName());
            return;
        }
        default:
            break;   // weapon batteries -> damage below
    }

    // --- weapon batteries need a loaded charge (chargeGroup1) ----------------
    if (weaponRef->HasAttribute(AttrChargeGroup1)) {
        uint32 chargeGroup = weaponRef->GetAttribute(AttrChargeGroup1).get_uint32();
        if (chargeGroup != 0) {
            DBQueryResult cres;
            uint32 chargeItemID = 0, chargeQty = 0;
            if (sDatabase.RunQuery(cres,
                "SELECT e.itemID, e.quantity FROM entity e JOIN invTypes t ON t.typeID = e.typeID"
                " WHERE e.locationID = %u AND t.groupID = %u AND e.quantity > 0 LIMIT 1",
                m_pWeapon->GetID(), chargeGroup)) {
                DBResultRow cr;
                if (cres.GetRow(cr)) { chargeItemID = cr.GetUInt(0); chargeQty = cr.GetUInt(1); }
            }
            if (chargeItemID == 0) {
                // out of ammo — no shot
                _log(POS__MESSAGE, "POS_AI: %s is out of charges (group %u).", m_pWeapon->GetName(), chargeGroup);
                return;
            }
            InventoryItemRef cRef = sItemFactory.GetItemRef(chargeItemID);
            if (cRef.get() != nullptr) {
                if (chargeQty <= 1) cRef->Delete();
                else cRef->SetQuantity((int32)(chargeQty - 1), false);
            }
        }
    }

    float dmgMult = weaponRef->GetAttribute(AttrDamageMultiplier).get_float();
    if (dmgMult < 0.01f)
        dmgMult = 1.0f;

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
            "effects.StandardWeapon", 1, 1, 1);

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
