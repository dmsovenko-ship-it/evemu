/**
 * DroneAI.cpp
 *      this class is for drone AI
 *
 * @Author:     Allan
 * @Version:    0.16
 * @Date:       27Nov19
 */

#include "eve-server.h"

#include <algorithm>
#include "Client.h"
#include "inventory/AttributeEnum.h"
#include "system/DestinyManager.h"
#include "npc/Drone.h"
#include "npc/DroneAI.h"
#include "inventory/ItemFactory.h"
#include "inventory/Inventory.h"
#include "ship/Ship.h"
#include "system/Damage.h"
#include "system/BubbleManager.h"
#include "system/SystemBubble.h"
#include "tables/invGroups.h"

DroneAIMgr::DroneAIMgr(DroneSE* who)
: m_state(DroneAI::State::Idle),
  m_pDrone(who),
  m_assignedShip(nullptr),
  m_returnToBay(false),
  m_singleMineCycle(false),
  m_mainAttackTimer(0),// dont start timer until we have a target
  m_processTimer(0),
  m_beginFindTarget(0),
  m_warpScramblerTimer(0),     // Timer for warp scramble duration
  m_webifierTimer(0),          // Timer for web duration
  m_sigRadius(std::max(who->GetSelf()->GetAttribute(AttrSignatureRadius).get_float(), 50.0f)),
  m_attackSpeed(std::max(who->GetSelf()->GetAttribute(AttrSpeed).get_float(), 4000.0f)),
  m_cruiseSpeed(static_cast<uint32>(std::max<int64>(who->GetSelf()->GetAttribute(AttrEntityCruiseSpeed).get_int(), 500))),
  m_chaseSpeed(static_cast<uint32>(std::max<int64>(who->GetSelf()->GetAttribute(AttrMaxVelocity).get_int(), 2000))),
  m_entityFlyRange(std::max(who->GetSelf()->GetAttribute(AttrEntityFlyRange).get_float() + who->GetSelf()->GetAttribute(AttrMaxRange).get_float(), 25000.0f)),
  m_entityChaseRange(std::max(who->GetSelf()->GetAttribute(AttrEntityChaseMaxDistance).get_float() * 2, 5000.0f)),
  m_entityOrbitRange(std::max(who->GetSelf()->GetAttribute(AttrMaxRange).get_float(), 1000.0f)),
  m_entityAttackRange(std::max(who->GetSelf()->GetAttribute(AttrEntityAttackRange).get_float() * 2, 10000.0f)),
  m_shieldBoosterDuration(who->GetSelf()->GetAttribute(AttrEntityShieldBoostDuration).get_int()),
  m_armorRepairDuration(who->GetSelf()->GetAttribute(AttrEntityArmorRepairDuration).get_int()),
  m_subType(DroneAI::SubType_Unknown),
  m_ewarStrength(0.0f),
  m_repairAmount(0.0f)
{
    m_processTimer.Start(5000);     //arbitrary.

    // Detect drone subtype from groupID
    switch (m_pDrone->GetGroupID()) {
        case EVEDB::invGroups::Warp_Scrambling_Drone: {
            m_subType = DroneAI::SubType_WarpScramble;
            if (m_pDrone->GetSelf()->HasAttribute(AttrWarpScrambleStrength))
                m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrWarpScrambleStrength).get_float();
            break;
        }
        case EVEDB::invGroups::Stasis_Webifying_Drone: {
            m_subType = DroneAI::SubType_Web;
            if (m_pDrone->GetSelf()->HasAttribute(AttrSpeedFactor))
                m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrSpeedFactor).get_float();
            break;
        }
        case EVEDB::invGroups::Electronic_Warfare_Drone: {
            m_subType = DroneAI::SubType_ECM;
            // ECM strength is based on scan strength vs target's sensor strength
            // Store scan strength bonus as ewarStrength
            if (m_pDrone->GetSelf()->HasAttribute(AttrScanStrengthBonus))
                m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrScanStrengthBonus).get_float();
            break;
        }
        case EVEDB::invGroups::Logistic_Drone:
        case EVEDB::invGroups::Repair_Drone: {
            m_subType = DroneAI::SubType_Logistics;
            // Determine repair type and amount
            // Player drones use AttrShieldBonus (68) / AttrArmorDamageAmount (84),
            // NPC entities use AttrEntityShieldBoostAmount (1532) / AttrEntityArmorRepairAmount (629).
            if (m_pDrone->GetSelf()->HasAttribute(AttrShieldBonus)) {
                m_repairAmount = m_pDrone->GetSelf()->GetAttribute(AttrShieldBonus).get_float();
            } else if (m_pDrone->GetSelf()->HasAttribute(AttrEntityShieldBoostAmount)) {
                m_repairAmount = m_pDrone->GetSelf()->GetAttribute(AttrEntityShieldBoostAmount).get_float();
            } else if (m_pDrone->GetSelf()->HasAttribute(AttrArmorDamageAmount)) {
                m_repairAmount = m_pDrone->GetSelf()->GetAttribute(AttrArmorDamageAmount).get_float();
            } else if (m_pDrone->GetSelf()->HasAttribute(AttrEntityArmorRepairAmount)) {
                m_repairAmount = m_pDrone->GetSelf()->GetAttribute(AttrEntityArmorRepairAmount).get_float();
            }
            break;
        }
        case EVEDB::invGroups::Cap_Drain_Drone: {
            m_subType = DroneAI::SubType_CapDrain;
            // Neut amount is stored in entity capacitor drain amount
            if (m_pDrone->GetSelf()->HasAttribute(AttrEntityCapacitorDrainAmount))
                m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrEntityCapacitorDrainAmount).get_float();
            break;
        }
        case EVEDB::invGroups::Mining_Drone: {
            m_subType = DroneAI::SubType_Mining;
            break;
        }
        case EVEDB::invGroups::Fighter_Drone: {
            m_subType = DroneAI::SubType_Fighter;
            break;
        }
        case EVEDB::invGroups::Fighter_Bomber: {
            m_subType = DroneAI::SubType_FighterBomber;
            break;
        }
        default: {
            // Combat drones (Combat_Drone=100)
            m_subType = DroneAI::SubType_Combat;
            break;
        }
    }

    _log(DRONE__AI_TRACE, "Drone %s(%u): subtype=%d, ewarStrength=%.2f, repairAmount=%.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), m_subType, m_ewarStrength, m_repairAmount);
}

void DroneAIMgr::Process() {
    double profileStartTime(GetTimeUSeconds());

    // Check warp scrambler duration expiry
    // Note: On expiry, we only disable the timer. The target's AttrWarpScrambleStatus
    // remains set until the scrambler stops being re-applied (next attack re-applies it).
    // This matches current emulator behavior where EWAR persists while attacking.
    if (m_warpScramblerTimer.Enabled() && m_warpScramblerTimer.Check()) {
        m_warpScramblerTimer.Disable();
        _log(DRONE__AI_TRACE, "Drone %s(%u): warp scrambler effect cycle expired.",
             m_pDrone->GetName(), m_pDrone->GetID());
    }

    // Check web duration expiry
    if (m_webifierTimer.Enabled() && m_webifierTimer.Check()) {
        m_webifierTimer.Disable();
        _log(DRONE__AI_TRACE, "Drone %s(%u): webifier effect cycle expired.",
             m_pDrone->GetName(), m_pDrone->GetID());
    }

    /* Drone::State definitions   -allan 27Nov19
     *   Invalid
     *   Idle              = 0,  // not doing anything....idle.
     *   Combat            = 1,  // fighting - needs targetID
     *   Mining            = 2,  // unsure - needs targetID
     *   Approaching       = 3,  // too close to chase, but to far to engage
     *   Departing         = 4,  // return to ship
     *   Departing2        = 5,  // leaving.  different from Departing
     *   Pursuit           = 6,  // target out of range to attack/follow, but within npc sight range....use mwd/ab if equiped
     *   Fleeing           = 7,  // running away
     *   Operating         = 9,  // whats diff from engaged here?  mining maybe?
     *   Engaged           = 10, // non-combat? - needs targetID
     *   // internal only
     *   Unknown           = 8,  // as stated
     *   Guarding          = 11,
     *   Assisting         = 12,
     *   Incapacitated     = 13  // out of control range, but online
     */

    // test for control distance - offline drones outside AttrDroneControlDistance
    // skip check while Departing (drone needs to return), Engaged, Approaching, or Mining
    // (actively working drones should not be interrupted by distance)
    if ((m_state != DroneAI::State::Departing)
    and (m_state != DroneAI::State::Engaged)
    and (m_state != DroneAI::State::Approaching)
    and (m_state != DroneAI::State::Mining)
    and (m_assignedShip != nullptr) and (m_assignedShip->DestinyMgr() != nullptr)) {
        double dist = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
        double controlRange = GetControlRange();
        if (dist > controlRange * 1.1) {
            if (m_state != DroneAI::State::Incapacitated) {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Out of control range (%.0fm > %.0fm) ship=(%.0f,%.0f,%.0f) drone=(%.0f,%.0f,%.0f).  Incapacitated.",
                     m_pDrone->GetName(), m_pDrone->GetID(), dist, controlRange * 1.1,
                     m_assignedShip->GetPosition().x, m_assignedShip->GetPosition().y, m_assignedShip->GetPosition().z,
                     m_pDrone->GetPosition().x, m_pDrone->GetPosition().y, m_pDrone->GetPosition().z);
                m_pDrone->DestinyMgr()->Stop();
                m_pDrone->Disable();
                m_state = DroneAI::State::Incapacitated;
            }
        } else if (m_state == DroneAI::State::Incapacitated) {
            m_pDrone->Enable();
            SetIdle();
        }
    }

    switch(m_state) {
        case DroneAI::State::Invalid: {
            // check everything in this state.   return to ship?
        } break;
        case DroneAI::State::Idle: {
            // orbiting controlling ship
            // For logistics drones in idle, we could auto-repair, but currently just orbit
        } break;
        case DroneAI::State::Engaged:
        case DroneAI::State::Approaching: {
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(m_state == DroneAI::State::Engaged);
            if (pTarget == nullptr) {
                if (m_pDrone->TargetMgr()->HasNoTargets()) {
                    _log(DRONE__AI_TRACE, "Drone %s(%u): Stopped %s, GetFirstTarget() returned NULL.", m_pDrone->GetName(), m_pDrone->GetID(), GetStateName(m_state).c_str());
                    SetIdle();
                }
                return;
            } else if (pTarget->SysBubble() == nullptr) {
                m_pDrone->TargetMgr()->ClearTarget(pTarget);
                return;
            } else if (pTarget->DestinyMgr() != nullptr
                   and pTarget->DestinyMgr()->GetState() == Destiny::Ball::Mode::WARP) {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) is warping.  Clearing target and returning to idle.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
                m_pDrone->DestinyMgr()->Stop();
                m_pDrone->TargetMgr()->ClearTarget(pTarget);
                SetIdle();
                return;
            }
            CheckDistance(pTarget);
        } break;

        case DroneAI::State::Departing: { // return to ship.  when close enough, scoop or orbit
            double arriveDist = std::min(m_entityOrbitRange, 5000.0);
            if (m_pDrone->GetPosition().distance(m_assignedShip->GetPosition()) < arriveDist) {
                if (m_returnToBay) {
                    m_returnToBay = false;
                    m_assignedShip->ScoopDrone(m_pDrone);  // removes from flight list, calls Offline()
                    return;                                  // drone is offline; do not touch state further
                }
                SetIdle();
            }
        } break;
        case DroneAI::State::Mining: {
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(true);
            if (pTarget == nullptr) {
                if (m_pDrone->TargetMgr()->HasNoTargets()) {
                    _log(DRONE__AI_TRACE, "Drone %s(%u): Mining stopped, no target.", m_pDrone->GetName(), m_pDrone->GetID());
                    SetIdle();
                }
                return;
            }
            if (pTarget->SysBubble() == nullptr) {
                m_pDrone->TargetMgr()->ClearTarget(pTarget);
                return;
            }
            double dist = m_pDrone->GetPosition().distance(pTarget->GetPosition());
            if (dist > m_entityFlyRange) {
                // move toward target without changing state to Approaching
                m_pDrone->DestinyMgr()->Follow(pTarget, m_entityOrbitRange);
                return;
            }
            if (!m_pDrone->DestinyMgr()->IsOrbiting()) {
                m_pDrone->DestinyMgr()->Orbit(pTarget, m_entityOrbitRange);
            }
            if (!m_miningTimer.Enabled())
                m_miningTimer.Start(m_attackSpeed);
            if (m_miningTimer.Check())
                MiningAttack(pTarget);
        } break;

        // not sure how im gonna do these...
        case DroneAI::State::Fleeing:
        case DroneAI::State::Operating:
        case DroneAI::State::Unknown:
        case DroneAI::State::Incapacitated:
        case DroneAI::State::Guarding:
        case DroneAI::State::Assisting:
        case DroneAI::State::Combat:
        case DroneAI::State::Departing2:
        case DroneAI::State::Pursuit: {
           // do nothing here yet
        } break;

    //no default on purpose
    }
    if (sConfig.debug.UseProfiling)
        sProfiler.AddTime(Profile::drone, GetTimeUSeconds() - profileStartTime);
}

int8 DroneAIMgr::GetState() {
    switch (m_state) {
        case DroneAI::State::Invalid:
        case DroneAI::State::Unknown:
        case DroneAI::State::Incapacitated:
            return DroneAI::State::Idle;
        case DroneAI::State::Engaged:
        case DroneAI::State::Approaching:
            return DroneAI::State::Combat;
        case DroneAI::State::Guarding:
        case DroneAI::State::Assisting:
            return DroneAI::State::Engaged;
        default:
            return m_state;
    }
}

void DroneAIMgr::Return() {
    m_assignedShip = m_pDrone->GetHomeShip();
    m_pDrone->DestinyMgr()->SetMaxVelocity(m_chaseSpeed);
    m_pDrone->DestinyMgr()->Follow(m_assignedShip, 0);  // fly directly to ship; Departing handler checks < m_entityOrbitRange
    m_state = DroneAI::State::Departing;
}

void DroneAIMgr::ReturnBay() {
    m_returnToBay = true;
    Return();   // sets Departing state; drone flies to ship; Departing handler will scoop
}

void DroneAIMgr::SetIdle() {
    if (m_state == DroneAI::State::Idle)
        return;
    // not doing anything....idle.
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: returning to idle.",
         m_pDrone->GetName(), m_pDrone->GetID());
    // Reload fighter ammo on return to carrier
    if (m_pDrone->IsFighter() and !m_pDrone->IsFighterBomber() and (m_pDrone->GetFighterAmmo() < m_pDrone->GetFighterMaxAmmo())) {
        m_pDrone->ReloadFighter();
        _log(DRONE__AI_TRACE, "Fighter %s(%u): Reloaded to %u ammo on return to carrier.",
             m_pDrone->GetName(), m_pDrone->GetID(), m_pDrone->GetFighterAmmo());
    }
    // Bombers reload when scooped into bay (they need to dock for bomb reload)
    if (m_pDrone->IsFighterBomber() and (m_pDrone->GetFighterAmmo() < m_pDrone->GetFighterMaxAmmo())) {
        m_pDrone->ReloadFighter();
        _log(DRONE__AI_TRACE, "Bomber %s(%u): Reloaded to %u ammo on return to carrier.",
             m_pDrone->GetName(), m_pDrone->GetID(), m_pDrone->GetFighterAmmo());
    }
    m_state = DroneAI::State::Idle;
    m_beginFindTarget.Disable();
    m_mainAttackTimer.Disable();

    // after reload, re-engage the last target if it still exists and is within range
    SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(false);
    if (pTarget != nullptr) {
        double dist = m_pDrone->GetPosition().distance(pTarget->GetPosition());
        double controlRange = GetControlRange();
        if (dist < controlRange * 2.0) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: re-engaging last target %s(%u) after reload (dist=%.0fm).",
                 m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), dist);
            // approach first regardless of distance — CheckDistance in Approaching state
            // will keep chasing without clearing the target (flyRange only limits attack, not pursuit)
            SetApproaching(pTarget);
            return;
        }
        _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: target %s(%u) too far (%.0fm > %.0fm).  Orbiting carrier.",
             m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), dist, controlRange * 2.0);
        // clear the distant target so we don't re-acquire it later
        m_pDrone->TargetMgr()->ClearTarget(pTarget);
    }

    // disable ewar timers (only when no target to re-engage)
    m_webifierTimer.Disable();
    m_warpScramblerTimer.Disable();

    // orbit assigned ship
    m_pDrone->IdleOrbit(m_assignedShip);
}

void DroneAIMgr::SetEngaged(SystemEntity* pTarget) {
    if (m_state == DroneAI::State::Engaged)
        return;
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetEngaged: %s(%u) begin engaging.",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
    // actively fighting — use cruise (orbit) speed while orbiting, not max chase speed
    float vel = m_cruiseSpeed * (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::DroneNavigation));
    m_pDrone->DestinyMgr()->SetMaxVelocity(vel);
    m_pDrone->DestinyMgr()->Orbit(pTarget, m_entityOrbitRange);  //try to get inside orbit range
    m_state = DroneAI::State::Engaged;
}

void DroneAIMgr::SetApproaching(SystemEntity* pSE)
{
    if (m_state == DroneAI::State::Approaching)
        return;
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetApproaching: %s(%u) begin pursuit.",
         m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
    float vel = m_chaseSpeed * (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::DroneNavigation));
    m_pDrone->DestinyMgr()->SetMaxVelocity(vel);
    m_pDrone->DestinyMgr()->Follow(pSE, m_entityOrbitRange);
    m_state = DroneAI::State::Approaching;
}

double DroneAIMgr::GetControlRange() {
    if (m_assignedShip == nullptr)
        return 25000.0;

    double range = m_assignedShip->GetSelf()->GetAttribute(AttrDroneControlDistance).get_float();
    if (range < 1.0)
        range = 20000.0; // base 20km if attribute not set

    // Fx system doesn't apply skill bonuses to ship attributes, add manually
    range += GetOwnerSkillLevel(EvESkill::ScoutDroneOperation) * 5000;              // +5km/level
    range += GetOwnerSkillLevel(EvESkill::ElectronicWarfareDroneInterfacing) * 3000; // +3km/level

    // Supercarrier bonus: double control range for fighter-bombers
    if (m_assignedShip->GetSelf()->groupID() == EVEDB::invGroups::Supercarrier) {
        bool isFighterBomber = (m_pDrone->GetGroupID() == EVEDB::invGroups::Fighter_Bomber);
        if (isFighterBomber)
            range *= 2.0;
    }

    return range;
}

void DroneAIMgr::CheckDistance(SystemEntity* pSE)
{
    // do not pursue a target that is warping — drone does NOT follow into warp
    if (pSE->DestinyMgr() != nullptr
    and pSE->DestinyMgr()->GetState() == Destiny::Ball::Mode::WARP) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): CheckDistance: target %s(%u) is warping.  Aborting pursuit.",
             m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
        m_pDrone->DestinyMgr()->Stop();
        m_pDrone->TargetMgr()->ClearTarget(pSE);
        SetIdle();
        return;
    }

    double dist = m_pDrone->GetPosition().distance(pSE->GetPosition());
    // Drone Sharpshooting: +10% optimal/falloff per level
    float rangeMult = 1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneSharpshooting);
    float flyRange = m_entityFlyRange * rangeMult;
    float attackRange = m_entityAttackRange * rangeMult;

    // If we're approaching and still far away, keep chasing
    if ((m_state == DroneAI::State::Approaching) && (dist > flyRange)) {
        // keep approaching — flyRange only limits attack, not pursuit
        return;
    }

    if (dist > flyRange) {
        if (m_state == DroneAI::State::Mining) {
            // mining drones approach target without state change
            m_pDrone->DestinyMgr()->Follow(pSE, m_entityOrbitRange);
            return;
        }
        _log(DRONE__AI_TRACE, "Drone %s(%u): CheckDistance: %s(%u) is too far away (%.0f).  Return to Idle.",
             m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID(), dist);
        ClearTarget(pSE);
        return;
    }
    if (dist > attackRange) {
        // within fly range but outside attack range — approach
        if (m_state == DroneAI::State::Mining) {
            m_pDrone->DestinyMgr()->Follow(pSE, m_entityOrbitRange);
            return;
        }
        SetApproaching(pSE);
        return;
    }
    // within attack range — engage and orbit at weapon range
    if (m_state == DroneAI::State::Mining) {
        // mining drones stay in Mining state, don't switch to Engaged
        if (!m_miningTimer.Enabled())
            m_miningTimer.Start(m_attackSpeed);
        return;
    }

    SetEngaged(pSE);

    if (!m_mainAttackTimer.Enabled()) {
        m_mainAttackTimer.Start(m_attackSpeed);
        // fire immediately on first call
        AttackTarget(pSE);
    }

    Attack(pSE);
}

void DroneAIMgr::ClearTargets() {
    m_pDrone->TargetMgr()->ClearTargets();
}

void DroneAIMgr::ClearAllTargets() {
    m_pDrone->TargetMgr()->ClearAllTargets();
    //m_pDrone->TargetMgr()->OnTarget(nullptr, TargMgr::Mode::Clear, TargMgr::Msg::ClientReq);
}

void DroneAIMgr::Target(SystemEntity* pTarget) {
    // Logistics drones repair the commanded target (if a valid ship other than owner)

    bool chase = false;
    if (!m_pDrone->TargetMgr()->StartTargeting(pTarget, m_pDrone->GetSelf()->GetAttribute(AttrScanSpeed).get_uint32(), (uint8)m_pDrone->GetSelf()->GetAttribute(AttrMaxAttackTargets).get_int(), m_entityFlyRange, chase)) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): Targeting of %s(%u) failed (chase=%d).  Will approach first.",
             m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), chase);
        if (chase) {
            bool dummyChase = false;
            SetApproaching(pTarget);
            m_pDrone->TargetMgr()->StartTargeting(pTarget, m_pDrone->GetSelf()->GetAttribute(AttrScanSpeed).get_uint32(),
                (uint8)m_pDrone->GetSelf()->GetAttribute(AttrMaxAttackTargets).get_int(),
                BUBBLE_RADIUS_METERS, dummyChase);
        } else {
            SetIdle();
        }
        return;
    }
    m_beginFindTarget.Disable();
    CheckDistance(pTarget);

    /*
    std::map<std::string, PyRep *> arg;
    arg["target"] = new PyInt(args.arg);
    throw PyException(MakeUserError("DeniedDroneTargetForceField", arg));
    */
 //DeniedDroneTargetForceField
}

void DroneAIMgr::MineTarget(SystemEntity* pTarget, bool singleCycle) {
    m_state = DroneAI::State::Mining;
    m_singleMineCycle = singleCycle;
    m_beginFindTarget.Disable();
    m_miningTimer.Start(m_attackSpeed);

    bool chase = false;
    uint32 scanSpeed = m_pDrone->GetSelf()->GetAttribute(AttrScanSpeed).get_uint32();
    if (scanSpeed < 1000) scanSpeed = 2000;
    if (!m_pDrone->TargetMgr()->StartTargeting(pTarget, scanSpeed, 1, m_entityFlyRange, chase)) {
        if (chase) {
            // move toward target while in Mining state
            m_pDrone->DestinyMgr()->Follow(pTarget, m_entityOrbitRange);
            m_pDrone->TargetMgr()->StartTargeting(pTarget, scanSpeed, 1, BUBBLE_RADIUS_METERS, chase);
        }
        return;
    }
    // within range — orbit and mine
    CheckDistance(pTarget);
}

void DroneAIMgr::Targeted(SystemEntity* pAgressor) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): Targeted by %s(%u) while %s.",
                m_pDrone->GetName(), m_pDrone->GetID(), pAgressor->GetName(), pAgressor->GetID(), GetStateName(m_state).c_str());
    switch(m_state) {
        case DroneAI::State::Idle: {
            if (m_pDrone->GetSelf()->HasAttribute(AttrDroneIsAgressive)) {
                if (m_pDrone->GetSelf()->GetAttribute(AttrDroneIsAgressive).get_int() > 0)
                    if (m_pDrone->TargetMgr()->GetTarget(pAgressor->GetID(), false) == nullptr)
                        Target(pAgressor);
            }
        } break;
        case DroneAI::State::Operating: {
        } break;
        case DroneAI::State::Unknown: {
        } break;
        case DroneAI::State::Engaged: {
        } break;
        case DroneAI::State::Fleeing: {
        } break;
        case DroneAI::State::Incapacitated: {
        } break;
        case DroneAI::State::Guarding: {
        } break;
        case DroneAI::State::Assisting: {
        } break;
        case DroneAI::State::Combat: {
        } break;
        case DroneAI::State::Mining: {
        } break;
        case DroneAI::State::Approaching: {
        } break;
        case DroneAI::State::Departing: {
        } break;
        case DroneAI::State::Departing2: {
        } break;
        case DroneAI::State::Pursuit: {
        } break;
    }
}

void DroneAIMgr::TargetLost(SystemEntity* pTarget) {
    switch(m_state) {
        case DroneAI::State::Engaged: {
            if (m_pDrone->TargetMgr()->HasNoTargets()) {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) lost. No targets remain.  Return to Idle.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
                SetIdle();
            } else {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) lost, but more targets remain.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
            }

        } break;

        default:
            break;
    }
}

void DroneAIMgr::Attack(SystemEntity* pSE)
{
    if (m_mainAttackTimer.Check()) {
        if (pSE == nullptr)
            return;
        // Check to see if the target still in the bubble (Client warped out)
        // fighters/bombers are able to follow.
        if (!m_pDrone->SysBubble()->InBubble(pSE->GetPosition())) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) no longer in bubble.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }
        DestinyManager* pDestiny = pSE->DestinyMgr();
        if (pDestiny == nullptr) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) has no destiny manager.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }
        // Check to see if the target is not cloaked:
        if (pDestiny->IsCloaked()) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) is cloaked.  Clear target and move on",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
            ClearTarget(pSE);
            return;
        }

        if (m_pDrone->TargetMgr()->CanAttack())
            AttackTarget(pSE);
    }
}

void DroneAIMgr::ClearTarget(SystemEntity* pSE) {
    m_pDrone->TargetMgr()->ClearTarget(pSE);
    //m_pDrone->TargetMgr()->OnTarget(pSE, TargMgr::Mode::Lost);

    if (m_pDrone->TargetMgr()->HasNoTargets())
        SetIdle();
}

void DroneAIMgr::AttackTarget(SystemEntity* pTarget) {
    // Dispatch based on drone subtype
    switch (m_subType) {
        case DroneAI::SubType_WarpScramble:
            ScrambleAttack(pTarget);
            break;
        case DroneAI::SubType_Web:
            WebAttack(pTarget);
            break;
        case DroneAI::SubType_ECM:
            ECMAttack(pTarget);
            break;
        case DroneAI::SubType_Logistics:
            LogisticsRepair(pTarget);
            break;
        case DroneAI::SubType_CapDrain:
            CapDrainAttack(pTarget);
            break;
        case DroneAI::SubType_Fighter:
            FighterAttack(pTarget);
            break;
        case DroneAI::SubType_FighterBomber:
            FighterBomberAttack(pTarget);
            break;
        case DroneAI::SubType_Combat:
        default:
            CombatAttack(pTarget);
            break;
    }
}

void DroneAIMgr::CombatAttack(SystemEntity* pTarget) {
    // effects are listed in EVE_Effects.h
    //  NOTE: drones are called 'entities' in client; EVE_Effects has 'entityxxx' for gfx
    std::string guid = "effects.Laser"; // client looks for 'turret' in ship.ball.modules for 'effects.laser'
    //effects.ProjectileFiredForEntities
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxTurretID))// graphicID for turret for drone type ships
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(), //m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_int(),
                                             pTarget->GetID(),
                                             0,guid,1,1,1,m_attackSpeed,0,gfxID);

    Damage d(m_pDrone,
             m_pDrone->GetSelf(),
             m_pDrone->GetKinetic(),
             m_pDrone->GetThermal(),
             m_pDrone->GetEM(),
             m_pDrone->GetExplosive(),
             m_formula.GetDroneToHit(m_pDrone, pTarget),
             EVEEffectID::targetAttack
            );

    float dmgMult = m_pDrone->GetSelf()->HasAttribute(AttrDamageMultiplier)
        ? m_pDrone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() : 1.0f;
    d *= dmgMult;

    // apply owner's drone skills
    float skillMult = 1.0f;
    if (m_pDrone->GetOwner() != nullptr) {
        skillMult = (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::Drones))
                  * (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneInterfacing))
                  * (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::HeavyDroneOperation));
        // racial specialization (+2% per level)
        int8 raceID = m_pDrone->GetSelf()->type().race();
        uint16 racialSkill = (raceID == 1 ? EvESkill::CaldariDroneSpecialization
                           : raceID == 2 ? EvESkill::MinmatarDroneSpecialization
                           : raceID == 4 ? EvESkill::AmarrDroneSpecialization
                           : raceID == 8 ? EvESkill::GallenteDroneSpecialization
                           : 0);
        if (racialSkill != 0)
            skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(racialSkill));
    }
    d *= skillMult;

    d *= sConfig.rates.damageRate;      /** @todo this should be a separate config value */
    _log(DRONE__AI_TRACE, "Drone %s(%u): CombatAttack -> %s(%u) total=%.2f (K:%.1f T:%.1f EM:%.1f E:%.1f mult=%.2f skill=%.2f hit=%.3f rate=%.3f)",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(),
         m_pDrone->GetKinetic(), m_pDrone->GetThermal(), m_pDrone->GetEM(), m_pDrone->GetExplosive(),
         dmgMult, skillMult, d.GetModifier(), sConfig.rates.damageRate);
    if (pTarget->ApplyDamage(d)) {
        return;
    }
}

void DroneAIMgr::FighterAttack(SystemEntity* pTarget) {
    // Fighter: fire missile (consumes ammo), return to carrier when empty
    if (!m_pDrone->ConsumeFighterAmmo()) {
        // Out of ammo — return to carrier for reload
        Return();
        return;
    }

    std::string guid = "effects.Laser";
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxTurretID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, guid, 1, 1, 1, m_attackSpeed, 0, gfxID);

    Damage d(m_pDrone,
             m_pDrone->GetSelf(),
             m_pDrone->GetKinetic(),
             m_pDrone->GetThermal(),
             m_pDrone->GetEM(),
             m_pDrone->GetExplosive(),
             m_formula.GetDroneToHit(m_pDrone, pTarget),
             EVEEffectID::targetAttack
            );

    float dmgMult = m_pDrone->GetSelf()->HasAttribute(AttrDamageMultiplier)
        ? m_pDrone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() : 1.0f;
    d *= dmgMult;

    // Apply carrier pilot's fighter skills
    if (m_pDrone->GetOwner() != nullptr) {
        float skillMult = 1.0f;
        skillMult *= (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::Fighters));
        skillMult *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneInterfacing));
        d *= skillMult;
    }

    d *= sConfig.rates.damageRate;
    _log(DRONE__AI_TRACE, "Fighter %s(%u): FighterAttack -> %s(%u) total=%.2f ammo=%u/%u",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(), m_pDrone->GetFighterAmmo(), m_pDrone->GetFighterMaxAmmo());
    if (pTarget->ApplyDamage(d)) {
        return;
    }
}

void DroneAIMgr::FighterBomberAttack(SystemEntity* pTarget) {
    // Fighter Bomber: AoE bomb attack, return to carrier when empty
    if (!m_pDrone->ConsumeFighterAmmo()) {
        Return();
        return;
    }

    // Bomb visual effect
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.Laser", 1, 1, 1, m_attackSpeed, 0, 0);

    // Bomber damage (higher base than regular fighters)
    Damage d(m_pDrone,
             m_pDrone->GetSelf(),
             m_pDrone->GetKinetic(),
             m_pDrone->GetThermal(),
             m_pDrone->GetEM(),
             m_pDrone->GetExplosive(),
             m_formula.GetDroneToHit(m_pDrone, pTarget),
             EVEEffectID::targetAttack
            );

    float dmgMult = m_pDrone->GetSelf()->HasAttribute(AttrDamageMultiplier)
        ? m_pDrone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() : 1.0f;
    d *= dmgMult;

    if (m_pDrone->GetOwner() != nullptr) {
        float skillMult = 1.0f;
        skillMult *= (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::FighterBombers));
        skillMult *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneInterfacing));
        d *= skillMult;
    }

    d *= sConfig.rates.damageRate;

    // Apply damage to target
    _log(DRONE__AI_TRACE, "Bomber %s(%u): FighterBomberAttack -> %s(%u) total=%.2f ammo=%u/%u",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(), m_pDrone->GetFighterAmmo(), m_pDrone->GetFighterMaxAmmo());
    if (pTarget->ApplyDamage(d)) {
        return;
    }
}

void DroneAIMgr::WebAttack(SystemEntity* pTarget) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): WebAttack on %s(%u) with strength %.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), m_ewarStrength);

    // Send web visual effect
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.ModifyTargetSpeed",
                                             1, 1, 1, m_attackSpeed, 0, gfxID);

    // Apply web effect via WebbedMe using drone's own itemRef (must have AttrSpeedFactor)
    InventoryItemRef droneRef = m_pDrone->GetSelf();
    if (droneRef->HasAttribute(AttrSpeedFactor)) {
        pTarget->DestinyMgr()->WebbedMe(droneRef, true);
        // Set timer to remove web after one cycle (attackSpeed is cycle time in ms)
        m_webifierTimer.Start(m_attackSpeed);
    }
}

void DroneAIMgr::ScrambleAttack(SystemEntity* pTarget) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): ScrambleAttack on %s(%u) with strength %.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), m_ewarStrength);

    // Send warp scramble visual effect
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.WarpScramble",
                                             1, 1, 1, m_attackSpeed, 0, gfxID);

    // Set WarpScrambleStatus on target
    InventoryItemRef targetRef = pTarget->GetSelf();
    if (targetRef->HasAttribute(AttrWarpScrambleStatus)) {
        targetRef->SetAttribute(AttrWarpScrambleStatus, m_ewarStrength);
    } else {
        targetRef->SetAttribute(AttrWarpScrambleStatus, m_ewarStrength, false);
    }

    // Set timer to remove scramble after cycle time
    m_warpScramblerTimer.Start(m_attackSpeed);
}

void DroneAIMgr::ECMAttack(SystemEntity* pTarget) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): ECMAttack on %s(%u) with strength %.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), m_ewarStrength);

    // Send ECM visual effect
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.ElectronicAttributeModifyTarget",
                                             1, 1, 1, m_attackSpeed, 0, gfxID);

    // ECM clears target's lock list, breaking their targeting
    pTarget->TargetMgr()->ClearTargets();
    // Also clear anyone targeting the drone
    // This simulates ECM breaking all locks on and by the target
}

void DroneAIMgr::LogisticsRepair(SystemEntity* pTarget) {
    // Orbit the repair target for visual feedback (like combat drones orbit their target)
    if (pTarget != nullptr)
        m_pDrone->DestinyMgr()->Orbit(pTarget, m_entityOrbitRange);

    // Repair only OTHER ships, never the owner (matches real EVE mechanics)
    ShipSE* repairTarget = nullptr;
    if ((pTarget != nullptr) and pTarget->IsShipSE()) {
        repairTarget = pTarget->GetShipSE();
    }
    if ((repairTarget == nullptr) or (repairTarget == m_assignedShip)) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): LogisticsRepair skipped (cannot repair self, target=%s).",
             m_pDrone->GetName(), m_pDrone->GetID(),
             repairTarget ? repairTarget->GetName() : "null");
        return;
    }

    InventoryItemRef targetShip = repairTarget->GetSelf();
    double amount = m_repairAmount;

    // Apply Repair Drone Operation skill bonus (+5% per level)
    if (m_pDrone->GetOwner() != nullptr) {
        amount *= (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::RepairDroneOperation));
    }

    // Determine if this is a shield or armor logistics drone based on own attributes
    // Player drones use AttrShieldBonus, NPCs use AttrEntityShieldBoostAmount
    bool isShieldLogistics = m_pDrone->GetSelf()->HasAttribute(AttrShieldBonus)
        || m_pDrone->GetSelf()->HasAttribute(AttrEntityShieldBoostAmount);

    _log(DRONE__AI_TRACE, "Drone %s(%u): LogisticsRepair on %s(%u), amount=%.2f, isShield=%d",
         m_pDrone->GetName(), m_pDrone->GetID(),
         repairTarget->GetName(), repairTarget->GetID(), amount, isShieldLogistics);

    if (isShieldLogistics) {
        // Shield repair
        double shieldCharge = targetShip->GetAttribute(AttrShieldCharge).get_float();
        double shieldCap = targetShip->GetAttribute(AttrShieldCapacity).get_float();
        shieldCharge += amount;
        if (shieldCharge > shieldCap)
            shieldCharge = shieldCap;
        targetShip->SetAttribute(AttrShieldCharge, shieldCharge);

        // Send shield boost visual effect
        uint32 gfxID = m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID) ?
            m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32() : 0;
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
                                                 repairTarget->GetID(),
                                                 0, "effects.ShieldBoosting",
                                                 0, 1, 1, m_attackSpeed, 0, gfxID);
    } else {
        // Armor repair
        double armorDamage = targetShip->GetAttribute(AttrArmorDamage).get_float();
        armorDamage -= amount;
        if (armorDamage < 0.0)
            armorDamage = 0.0;
        targetShip->SetAttribute(AttrArmorDamage, armorDamage);

        // Send armor repair visual effect
        uint32 gfxID = m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID) ?
            m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32() : 0;
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
                                                 repairTarget->GetID(),
                                                 0, "effects.ArmorRepair",
                                                 0, 1, 1, m_attackSpeed, 0, gfxID);
    }

    // Notify clients of damage state change
    repairTarget->SendDamageStateChanged();
}

void DroneAIMgr::CapDrainAttack(SystemEntity* pTarget) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): CapDrainAttack on %s(%u) with amount %.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), m_ewarStrength);

    // Send energy neutralizer visual effect
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.EnergyDestabilization",
                                             1, 1, 1, m_attackSpeed, 0, gfxID);

    // Drain capacitor from target
    InventoryItemRef targetRef = pTarget->GetSelf();
    double drainAmount = m_ewarStrength;
    if (drainAmount < 1.0) {
        // Fallback: use entity capacitor drain amount from drone
        if (m_pDrone->GetSelf()->HasAttribute(AttrEntityCapacitorDrainAmount))
            drainAmount = m_pDrone->GetSelf()->GetAttribute(AttrEntityCapacitorDrainAmount).get_float();
    }

    if (targetRef->HasAttribute(AttrCapacitorCharge)) {
        double targetCap = targetRef->GetAttribute(AttrCapacitorCharge).get_float();
        targetCap -= drainAmount;
        if (targetCap < 0.0)
            targetCap = 0.0;
        targetRef->SetAttribute(AttrCapacitorCharge, targetCap);

        // Transfer drained cap to owner ship if available
        if (m_assignedShip != nullptr) {
            InventoryItemRef ownerRef = m_assignedShip->GetSelf();
            if (ownerRef->HasAttribute(AttrCapacitorCharge)) {
                double ownerCap = ownerRef->GetAttribute(AttrCapacitorCharge).get_float();
                double ownerMaxCap = ownerRef->GetAttribute(AttrCapacitorCapacity).get_float();
                ownerCap += drainAmount;
                if (ownerCap > ownerMaxCap)
                    ownerCap = ownerMaxCap;
                ownerRef->SetAttribute(AttrCapacitorCharge, ownerCap);
            }
        }
    }
}

void DroneAIMgr::MiningAttack(SystemEntity* pTarget) {
    if (m_assignedShip == nullptr || m_assignedShip->DestinyMgr() == nullptr)
        return;

    // check distance to ship — ore can't be transferred if too far
    double distToShip = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
    double controlRange = GetControlRange();
    if (distToShip > controlRange * 1.5)
        return; // too far, wait until ship gets closer

    // get mining amount from drone attributes
    float miningAmount = m_pDrone->GetSelf()->GetAttribute(AttrMiningAmount).get_float();
    if (miningAmount < 1.0)
        miningAmount = 10.0; // fallback

    // apply Mining skill bonus (+5% per level)
    int8 miningSkill = GetOwnerSkillLevel(EvESkill::Mining);
    miningAmount *= (1.0f + 0.05f * miningSkill);

    // apply Mining Drone Operation bonus (+20% per level)
    int8 miningDroneSkill = GetOwnerSkillLevel(EvESkill::MiningDroneOperation);
    miningAmount *= (1.0f + 0.20f * miningDroneSkill);

    // get asteroid ore volume
    InventoryItemRef roidRef = pTarget->GetSelf();
    float oreVolume = roidRef->GetAttribute(AttrVolume).get_float();
    if (oreVolume <= 0) {
        oreVolume = 1.0;
    }

    if (miningAmount < oreVolume) {
        _log(DRONE__AI_TRACE, "Drone %s(%u): miningAmount %.2f < oreVolume %.2f for %s(%u), skipping cycle.",
             m_pDrone->GetName(), m_pDrone->GetID(), miningAmount, oreVolume,
             pTarget->GetName(), pTarget->GetID());
        return;
    }

    float oreUnits = floor(miningAmount / oreVolume);
    if (oreUnits < 1.0)
        return;

    uint16 oreTypeID = roidRef->typeID();

    // add ore to ship cargo hold
    InventoryItemRef shipRef = m_assignedShip->GetSelf();

    // check remaining cargo capacity
    Inventory* inv = sItemFactory.GetInventoryFromId(shipRef->itemID());
    if (inv != nullptr) {
        double remaining = inv->GetRemainingCapacity(flagCargoHold);
        double oreVol = oreUnits * oreVolume;
        if (oreVol > remaining) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): Cargo full (need %.0f, have %.0f), stopping.",
                 m_pDrone->GetName(), m_pDrone->GetID(), oreVol, remaining);
            m_pDrone->GetOwner()->SendNotifyMsg("Mining drones deactivated: cargo hold full.");
            m_pDrone->TargetMgr()->ClearTarget(pTarget);
            SetIdle();
            m_pDrone->StateChange();
            return;
        }
    }

    ItemData idata(oreTypeID, shipRef->ownerID(), shipRef->itemID(), flagNone, static_cast<int32>(oreUnits));
    InventoryItemRef oRef = sItemFactory.SpawnItem(idata);
    if (oRef.get() == nullptr) {
        _log(DRONE__MESSAGE, "MiningAttack: Could not create mined ore for ship %s(%u)",
             shipRef->name(), shipRef->itemID());
        return;
    }
    oRef->Move(shipRef->itemID(), flagCargoHold, true);
    _log(DRONE__AI_TRACE, "Drone %s(%u): Added %.0f units of ore type %u to ship cargo.",
         m_pDrone->GetName(), m_pDrone->GetID(), oreUnits, oreTypeID);

    // play mining visual effect on the asteroid
    m_pDrone->DestinyMgr()->SendSpecialEffect(
        m_pDrone->GetSelf()->itemID(),
        m_pDrone->GetSelf()->itemID(),
        m_pDrone->GetSelf()->typeID(),
        pTarget->GetID(),
        0,
        "effects.Mining",
        1, 1, 1,
        m_attackSpeed,
        0,
        0
    );

    // single cycle mode — return to orbit after one successful mine
    if (m_singleMineCycle) {
        m_pDrone->TargetMgr()->ClearTarget(pTarget);
        SetIdle();
        m_pDrone->StateChange();
    }
}

int8 DroneAIMgr::GetOwnerSkillLevel(uint16 skillID) const {
    Client* pOwner = m_pDrone->GetOwner();
    if ((pOwner == nullptr) or (pOwner->GetChar().get() == nullptr))
        return 0;
    return pOwner->GetChar()->GetSkillLevel(skillID);
}

ShipSE* DroneAIMgr::GetOwnerShip() {
    return m_assignedShip;
}


std::string DroneAIMgr::GetStateName(int8 stateID)
{
    switch (stateID) {
        case DroneAI::State::Idle:            return "Idle";
        case DroneAI::State::Combat:          return "Combat";
        case DroneAI::State::Mining:          return "Mining";
        case DroneAI::State::Approaching:     return "Approaching";
        case DroneAI::State::Departing:       return "Returning to ship";
        case DroneAI::State::Departing2:      return "Departing2";
        case DroneAI::State::Pursuit:         return "Pursuit";
        case DroneAI::State::Engaged:         return "Engaged";
        case DroneAI::State::Fleeing:         return "Fleeing";
        case DroneAI::State::Unknown:         return "Unknown";
        case DroneAI::State::Operating:       return "Operating";
        case DroneAI::State::Assisting:       return "Assisting";
        case DroneAI::State::Guarding:        return "Guarding";
        case DroneAI::State::Incapacitated:   return "Incapacitated";
        default:                              return "Invalid";
    }
}
