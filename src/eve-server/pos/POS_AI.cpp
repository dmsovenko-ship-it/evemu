
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
#include "effects/EffectsDataMgr.h"
#include "system/CrimeWatch.h"
#include "standing/StandingDB.h"
#include "tables/invGroups.h"
#include "pos/POS_AI.h"
#include "pos/Tower.h"
#include "ship/Ship.h"
#include "ship/Missile.h"
#include "inventory/ItemFactory.h"
#include "inventory/ItemDB.h"
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
        ReleaseWeb();   // battery went offline/anchored-out: don't leave the web on the target
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
        uint32 manual = (m_pTower != nullptr) ? m_pTower->GetManualTarget() : 0;
        if (manual != 0)
            m_targetID = manual;          // operator-selected target (focus fire)
        else if (m_targetID == 0)
            FindTarget();
    }

    if (m_targetID != 0) {
        SystemEntity* pTarget = m_pWeapon->SystemMgr()->GetSE(m_targetID);
        if (pTarget == nullptr) {
            ReleaseWeb();
            m_targetID = 0;
            return;
        }

        bool isManual = (m_pTower != nullptr && m_pTower->GetManualTarget() == m_targetID);
        float range = m_pWeapon->GetPosition().distance(pTarget->GetPosition());
        float maxRange = m_pWeapon->GetSelf()->GetAttribute(AttrMaxRange).get_float();
        float falloff = m_pWeapon->GetSelf()->GetAttribute(AttrFalloff).get_float();
        float sightRange = m_pWeapon->GetSelf()->GetAttribute(AttrProximityRange).get_float();
        // A neut/EWAR battery has no maxRange attribute — its reach is
        // energyDestabilizationRange (250 km for a standard Energy Neutralizing
        // Battery). Without this, range was 0: a manually-targeted neut could
        // never fire and automatic targeting only worked inside proximity range.
        if (maxRange <= 0.0f && m_pWeapon->GetSelf()->HasAttribute(AttrEnergyDestabilizationRange))
            maxRange = m_pWeapon->GetSelf()->GetAttribute(AttrEnergyDestabilizationRange).get_float();

        // Manual gunnery ignores the automatic sight range (an operator can use
        // the gun's full range); only the weapon's own reach still applies.
        if (range > (maxRange + falloff) and !isManual and range > sightRange) {
            ReleaseWeb();
            m_targetID = 0;
            return;
        }
        if (isManual && range > (maxRange + falloff)) {
            ReleaseWeb();
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

static bool IsValidTargetInternal(SystemEntity* pEntity, TowerSE* pTower, StructureSE* pWeapon)
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
        // Security-status threshold (tower setting): engage targets at/below it.
        float statusThreshold = pTower->GetStatus();
        if (statusThreshold < 0.0f && statusThreshold > -10.0f) {
            if (pClient->GetSecurityRating() <= statusThreshold)
                hostileBySettings = true;
        }
    }

    // A pilot who is shooting this POS is a valid target even in high-sec: the
    // starbase defends itself against its attacker.
    if (pTower != nullptr && pTower->IsAggressor(pClient->GetCharacterID()))
        return true;

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
            // A stasis web is a FIXED modifier while the module cycles — it must
            // NOT be re-applied every cycle: DestinyManager::WebbedMe multiplies
            // m_maxShipSpeed by (1+SpeedFactor/100), so re-applying stacks it down
            // to the speed floor (the "1 m/s despite MWD" bug). Apply once, keep a
            // symmetric undo, and only (re)apply when the target changes.
            uint32 tgt = pTarget->GetID();
            if (m_webApplied && m_webTargetID != tgt)
                ReleaseWeb();
            if (!m_webApplied && pTarget->DestinyMgr() != nullptr) {
                pTarget->DestinyMgr()->WebbedMe(weaponRef, true);
                m_webApplied = true;
                m_webTargetID = tgt;
            }
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), tgt,
                                                         "effects.ModifyTargetSpeed", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s webbed %s.", m_pWeapon->GetName(), pTarget->GetName());
            return;
        }
        case EVEDB::invGroups::Warp_Scrambling_Battery: {
            EvilNumber strength = weaponRef->HasAttribute(AttrWarpScrambleStrength)
                                ? weaponRef->GetAttribute(AttrWarpScrambleStrength) : EvilOne;
            // Runtime-only (persist=false): a scram status baked into the DB
            // survives logout/restart, and an undocked-then-redocked ship comes
            // back with "warp drive disrupted" already on it (jitter at undock).
            pTarget->GetSelf()->SetAttribute(AttrWarpScrambleStatus, strength, false);
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), pTarget->GetID(),
                                                         "effects.WarpScramble", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s scrambled %s.", m_pWeapon->GetName(), pTarget->GetName());
            return;
        }
        case EVEDB::invGroups::Energy_Neutralizing_Battery: {
            // POS neut batteries carry the drain in energyDestabilizationAmount
            // (attr 97, 1000 GJ for the standard battery). entityCapacitorDrainAmount
            // (946) is ABSENT on them, so the old code drained a fixed 100 GJ — too
            // small to notice next to capacitor regen (and the pilot saw no drop).
            float amount = 0.0f;
            if (weaponRef->HasAttribute(AttrEnergyDestabilizationAmount))
                amount = weaponRef->GetAttribute(AttrEnergyDestabilizationAmount).get_float();
            else if (weaponRef->HasAttribute(AttrEntityCapacitorDrainAmount))
                amount = weaponRef->GetAttribute(AttrEntityCapacitorDrainAmount).get_float();
            if (amount <= 0.0f)
                amount = 1000.0f;
            EvilNumber cap = pTarget->GetSelf()->GetAttribute(AttrCapacitorCharge);
            cap -= amount;
            if (cap < EvilZero) cap = EvilZero;
            // persist=false: capacitor is transient; AttributeMap::Change still sends
            // OnModuleAttributeChange to the pilot so the cap gauge drops.
            pTarget->GetSelf()->SetAttribute(AttrCapacitorCharge, cap, false);
            m_pWeapon->DestinyMgr()->SendSpecialEffect10(m_pWeapon->GetID(), pTarget->GetID(),
                                                         "effects.EnergyDestabilization", 1, 1, 1);
            _log(POS__MESSAGE, "POS_AI: %s neutralized %.0f GJ from %s.", m_pWeapon->GetName(), amount, pTarget->GetName());
            return;
        }
        default:
            break;   // weapon batteries -> damage below
    }

    // --- weapon batteries need a loaded charge (chargeGroup1) ----------------
    InventoryItemRef loadedCharge;
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
                loadedCharge = cRef;
                if (chargeQty <= 1) cRef->Delete();
                else cRef->SetQuantity((int32)(chargeQty - 1), false);
            }
        }
    }

    // --- missile batteries: launch a real missile SE (visible flight) ---------
    // The client has no turret for a POS missile sentry (its gfxTurretID is NULL,
    // so spaceObject/structureSentryGun.py cannot fit a launcher and
    // effects.StandardWeapon renders nothing). Towers/NPCs fire missiles by
    // spawning an actual Missile ball, which is what the client animates, so do
    // the same here. The missile applies its own damage on impact, so return
    // before the direct-damage path below.
    if (grp == EVEDB::invGroups::Mobile_Missile_Sentry) {
        uint32 missileTypeID = (loadedCharge.get() != nullptr) ? loadedCharge->typeID() : 0;
        if (missileTypeID == 0 && weaponRef->HasAttribute(AttrEntityMissileTypeID))
            missileTypeID = weaponRef->GetAttribute(AttrEntityMissileTypeID).get_uint32();
        if (missileTypeID != 0)
            LaunchMissile(missileTypeID, pTarget);
        return;
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

    // POS gun damage comes from the loaded CHARGE (the battery type carries no
    // turret *Damage attributes — only a damageMultiplier), so read the charge's
    // damage and scale by the multiplier. Fall back to the weapon's own damage
    // when there is no charge.
    InventoryItemRef dmgSrc = (loadedCharge.get() != nullptr) ? loadedCharge : weaponRef;
    float mod = hitChance * dmgMult;
    Damage d(m_pWeapon, weaponRef,
             dmgSrc->GetAttribute(AttrKineticDamage).get_float(),
             dmgSrc->GetAttribute(AttrThermalDamage).get_float(),
             dmgSrc->GetAttribute(AttrEmDamage).get_float(),
             dmgSrc->GetAttribute(AttrExplosiveDamage).get_float(),
             mod, 0);

    bool killed = pTarget->ApplyDamage(d);
    _log(POS__MESSAGE, "POS_AI::FireWeapon() - %s(%u) fired at %s(%u)",
            m_pWeapon->GetName(), m_pWeapon->GetID(),
            pTarget->GetName(), pTarget->GetID());

    // Turret firing animation. The client's POS gun (spaceObject/structureSentryGun.py)
    // fits a turret on the battery and keys it as modules[<battery itemID>]
    // (self.modules[self.id] = newTurretSet), and effects.StandardWeapon looks it up
    // via shipBall.modules.get(trigger.moduleID) in spaceObject/model/turretSet.py.
    // SendSpecialEffect10 does NOT fill moduleID, so the lookup fails ("Turret not
    // fitted") and nothing renders. Send the full OnSpecialFX14 instead, with
    // moduleID == the battery's own itemID and otherTypeID == the loaded charge
    // (drives the muzzle/ammo colour). graphicInfo = the type's gfxTurretID.
    uint32 attackDelay = weaponRef->GetAttribute(AttrSpeed).get_uint32();
    if (attackDelay < 1000)
        attackDelay = 15000;
    uint32 chargeTypeID = (loadedCharge.get() != nullptr) ? loadedCharge->typeID() : 0;
    int32 gfxTurretID = weaponRef->HasAttribute(AttrGfxTurretID)
                      ? weaponRef->GetAttribute(AttrGfxTurretID).get_int() : 0;

    m_pWeapon->DestinyMgr()->SendSpecialEffect(
            m_pWeapon->GetID(),          // shipID  (ball that carries the fitted turret)
            m_pWeapon->GetID(),          // moduleID (turret key)
            weaponRef->typeID(),         // moduleTypeID
            pTarget->GetID(),            // targetID
            chargeTypeID,                // otherTypeID (ammo colour)
            TurretEffectGuidByGroup(grp),
            1, 1, 1,                     // isOffensive, start, active
            (int32)attackDelay, 0,       // duration(ms), repeat
            gfxTurretID);

    if (killed) {
        m_targetID = 0;
        m_lastTargetScan = 0;
    }
}

// Spawn a real missile SE and send it at the target — mirrors
// NPCAIMgr::LaunchMissile so the client draws the launch + flight + impact.
void POS_AI::LaunchMissile(uint32 typeID, SystemEntity* pTarget)
{
    if (typeID == 0 || pTarget == nullptr)
        return;

    SystemManager* pSystem = m_pWeapon->SystemMgr();
    if (pSystem == nullptr)
        return;

    InventoryItemRef weaponRef = m_pWeapon->GetSelf();
    ItemData idata(typeID, m_pWeapon->GetID(), m_pWeapon->GetLocationID(),
                   flagMissile, "POS Missile", m_pWeapon->GetPosition());
    InventoryItemRef missileRef = sItemFactory.SpawnItem(idata);
    if (missileRef.get() == nullptr)
        return;

    Missile* pMissile = new Missile(missileRef, pSystem->GetServiceMgr(), pSystem,
                                    weaponRef, pTarget, m_pWeapon);
    double distance = pMissile->GetPosition().distance(pTarget->GetPosition());
    double missileSpeed = missileRef->GetAttribute(AttrMaxVelocity).get_float();
    if (missileSpeed < 1.0)
        missileSpeed = 1000.0;
    double travelTime = distance / missileSpeed;
    if (travelTime < 1.0)
        travelTime = 1.0;
    pMissile->SetSpeed(missileSpeed);
    pMissile->SetHitTimer((uint32)(travelTime * 1000.0));
    pMissile->DestinyMgr()->MakeMissile(pMissile);

    // Let the target's defenders react (defender missiles), as NPCAI does.
    pTarget->MissileLaunched(pMissile);

    _log(POS__MESSAGE, "POS_AI: %s launched missile %u at %s (%u).",
         m_pWeapon->GetName(), typeID, pTarget->GetName(), pTarget->GetID());
}

void POS_AI::TargetLost(uint32 entityID)
{
    if (m_webApplied && m_webTargetID == entityID)
        ReleaseWeb();
    if (entityID == m_targetID)
        m_targetID = 0;
}

// Undo a stasis web (WebbedMe(false) divides m_maxShipSpeed back). Called when the
// webbed target changes, is lost, or the tower/weapon stops firing at it.
void POS_AI::ReleaseWeb()
{
    if (!m_webApplied)
        return;
    m_webApplied = false;
    uint32 old = m_webTargetID;
    m_webTargetID = 0;
    SystemManager* pSystem = m_pWeapon->SystemMgr();
    if (pSystem == nullptr)
        return;
    SystemEntity* oldSE = pSystem->GetSE(old);
    if (oldSE != nullptr && oldSE->DestinyMgr() != nullptr)
        oldSE->DestinyMgr()->WebbedMe(m_pWeapon->GetSelf(), false);
}
