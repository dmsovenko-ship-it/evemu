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
#include "npc/NPC.h"
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
  m_chaseSpeed(static_cast<uint32>(std::max<int64>(who->GetSelf()->GetAttribute(AttrMaxVelocity).get_int(), 100))),
  m_entityFlyRange(std::max(who->GetSelf()->GetAttribute(AttrEntityFlyRange).get_float() + who->GetSelf()->GetAttribute(AttrMaxRange).get_float(), 25000.0f)),
  m_entityChaseRange(std::max(who->GetSelf()->GetAttribute(AttrEntityChaseMaxDistance).get_float() * 2, 5000.0f)),
  m_entityOrbitRange(std::max(who->GetSelf()->GetAttribute(AttrMaxRange).get_float(), 1000.0f)),
  m_entityAttackRange(std::max(who->GetSelf()->GetAttribute(AttrEntityAttackRange).get_float() * 2, 10000.0f)),
  m_shieldBoosterDuration(who->GetSelf()->GetAttribute(AttrEntityShieldBoostDuration).get_int()),
  m_armorRepairDuration(who->GetSelf()->GetAttribute(AttrEntityArmorRepairDuration).get_int()),
  m_subType(DroneAI::SubType_Unknown),
  m_ewarStrength(0.0f),
  m_repairAmount(0.0f),
  m_paintTimer(0),
  m_paintTargetID(0),
  m_paintedSigRadius(0.0f),
  m_webApplied(false),
  m_webTargetID(0)
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
            // Target painting drones have AttrSignatureRadiusBonus instead of AttrScanStrengthBonus
            if (m_pDrone->GetSelf()->HasAttribute(AttrSignatureRadiusBonus)) {
                m_subType = DroneAI::SubType_TargetPaint;
                m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrSignatureRadiusBonus).get_float();
            } else {
                m_subType = DroneAI::SubType_ECM;
                if (m_pDrone->GetSelf()->HasAttribute(AttrScanStrengthBonus))
                    m_ewarStrength = m_pDrone->GetSelf()->GetAttribute(AttrScanStrengthBonus).get_float();
            }
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
            // Sentry drones have zero max velocity — they don't move
            m_subType = (m_pDrone->GetSelf()->GetAttribute(AttrMaxVelocity).get_float() <= 0.0f)
                      ? DroneAI::SubType_Sentry
                      : DroneAI::SubType_Combat;
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
    // skip if drone was destroyed/freed (use-after-free guard)
    if ((m_pDrone == nullptr) or (m_pDrone->DestinyMgr() == nullptr))
        return;
    // skip check while Departing (drone needs to return), Engaged, Approaching, or Mining
    // (actively working drones should not be interrupted by distance)
    if ((m_state != DroneAI::State::Departing)
    and (m_state != DroneAI::State::Engaged)
    and (m_state != DroneAI::State::Approaching)
    and (m_state != DroneAI::State::Mining)
    and (m_state != DroneAI::State::Assisting)
    and (m_state != DroneAI::State::Guarding)
    and (m_assignedShip != nullptr) and (m_assignedShip->DestinyMgr() != nullptr)) {
        double dist = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
        double controlRange = GetControlRange();
        if (dist > controlRange * 3.0) {
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
            // Fighters: re-engage existing target after reload
            if (m_pDrone->IsFighter() and (m_pDrone->GetFighterAmmo() > 0)
            and (m_pDrone->TargetMgr() != nullptr)) {
                SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(false);
                if (pTarget != nullptr and pTarget->SysBubble() != nullptr)
                    Target(pTarget);
            }
            // Drone/fighter behaviour settings (from the ship's drone window):
            //   - Focus fire (атака одной цели): all drones hit the ship's target.
            //   - Aggressive (агрессивный): drones auto-engage hostiles (rats) in range.
            //   - Fighters attack-and-follow (атаковать и преследовать): fighters
            //     pursue hostiles on their own.
            if (m_pDrone->TargetMgr() != nullptr) {
                bool focusFire = m_pDrone->GetSelf()->HasAttribute(AttrDroneFocusFire)
                              && m_pDrone->GetSelf()->GetAttribute(AttrDroneFocusFire).get_int() > 0;
                bool aggressive = m_pDrone->GetSelf()->HasAttribute(AttrDroneIsAgressive)
                               && m_pDrone->GetSelf()->GetAttribute(AttrDroneIsAgressive).get_int() > 0;
                bool attackFollow = m_pDrone->IsFighter()
                                && m_pDrone->GetSelf()->HasAttribute(AttrFightersAttackAndFollow)
                                && m_pDrone->GetSelf()->GetAttribute(AttrFightersAttackAndFollow).get_int() > 0;

                if (focusFire or aggressive or attackFollow) {
                    // Already fighting something — let the combat states handle it.
                    if (m_pDrone->TargetMgr()->GetFirstTarget(false) != nullptr)
                        break;

                    // Focus fire: engage the ship's current target.
                    if (focusFire and m_assignedShip != nullptr and m_assignedShip->TargetMgr() != nullptr) {
                        SystemEntity* shipTarget = m_assignedShip->TargetMgr()->GetFirstTarget(true);
                        if (shipTarget != nullptr and shipTarget->DestinyMgr() != nullptr) {
                            _log(DRONE__AI_TRACE, "Drone %s(%u): focus fire on %s(%u).",
                                 m_pDrone->GetName(), m_pDrone->GetID(),
                                 shipTarget->GetName(), shipTarget->GetID());
                            Target(shipTarget);
                            break;
                        }
                    }

                    // Aggressive / attack-and-follow: hunt a hostile NPC in range.
                    if (aggressive or attackFollow) {
                        SystemEntity* prey = FindAggroTarget();
                        if (prey != nullptr) {
                            _log(DRONE__AI_TRACE, "Drone %s(%u): aggressive engage %s(%u).",
                                 m_pDrone->GetName(), m_pDrone->GetID(),
                                 prey->GetName(), prey->GetID());
                            Target(prey);
                            break;
                        }
                    }
                }
            }
            // Delegate to Assisting state if assisting, or Idle otherwise
            if (m_pDrone->IsAssisting())
                m_state = DroneAI::State::Assisting;
            else if (m_pDrone->IsGuarding())
                m_state = DroneAI::State::Guarding;
        } break;
        case DroneAI::State::Engaged:
        case DroneAI::State::Approaching: {
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(m_state == DroneAI::State::Engaged);
            if (pTarget == nullptr) {
                if (m_pDrone->TargetMgr()->HasNoTargets()) {
                    _log(DRONE__AI_TRACE, "Drone %s(%u): Stopped %s, GetFirstTarget() returned NULL.", m_pDrone->GetName(), m_pDrone->GetID(), GetStateName(m_state).c_str());
                    // Return to assist/guard state instead of idle if applicable
                    if (m_pDrone->IsAssisting())
                        m_state = DroneAI::State::Assisting;
                    else if (m_pDrone->IsGuarding())
                        m_state = DroneAI::State::Guarding;
                    else
                        SetIdle();
                }
                return;
            } else if (pTarget->SysBubble() == nullptr) {
                ClearTarget(pTarget);
                return;
            } else if (m_pDrone->SystemMgr() == nullptr
                    or m_pDrone->SystemMgr()->GetSE(pTarget->GetID()) == nullptr) {
                // Target entity was destroyed/removed from the system (e.g. an
                // acceleration gate / structure without a TargetMgr, whose death
                // never triggers ClearFromTargets). GetFirstTarget() still holds
                // the stale pointer; drop it and return to the ship instead of
                // chasing a dead target into deep space.
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) no longer exists in system. Clearing target.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
                ClearTarget(pTarget);
                SetIdle();
                return;
            } else if (pTarget->DestinyMgr() != nullptr
                   and pTarget->DestinyMgr()->GetState() == Destiny::Ball::Mode::WARP) {
                _log(DRONE__AI_TRACE, "Drone %s(%u): Target %s(%u) is warping.  Clearing target and returning to idle.",
                     m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
                m_pDrone->DestinyMgr()->Stop();
                ClearTarget(pTarget);
                SetIdle();
                return;
            } else if (m_assignedShip != nullptr and m_assignedShip->DestinyMgr() != nullptr) {
                // Defensive: if the drone chased its target beyond control range
                // (an NPC fled/warped mid-combat and the client-driven follow ran away),
                // stop pursuing and return to the ship — prevents "drone flew into
                // deep space" during combat.
                double distToShip = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
                if (distToShip > GetControlRange()) {
                    _log(DRONE__AI_TRACE, "Drone %s(%u): beyond control range (%.0fm) chasing %s(%u). Returning to ship.",
                         m_pDrone->GetName(), m_pDrone->GetID(), distToShip, pTarget->GetName(), pTarget->GetID());
                    ClearTarget(pTarget);
                    Return();
                    return;
                }
                // Focus fire: if the pilot switched the ship's locked target, the
                // drone must follow. Without this the drone keeps hammering the old
                // target after the player retargets (e.g. to a newly-arrived NPC
                // defender drone or the TCU). Only re-evaluate while the current
                // target is still valid (the guards above already returned otherwise).
                bool focusFire = m_pDrone->GetSelf()->HasAttribute(AttrDroneFocusFire)
                               && m_pDrone->GetSelf()->GetAttribute(AttrDroneFocusFire).get_int() > 0;
                if (focusFire and m_assignedShip->TargetMgr() != nullptr) {
                    SystemEntity* shipTarget = m_assignedShip->TargetMgr()->GetFirstTarget(true);
                    if (shipTarget != nullptr and shipTarget != pTarget
                            and shipTarget->DestinyMgr() != nullptr) {
                        _log(DRONE__AI_TRACE, "Drone %s(%u): focus fire switching to ship's new target %s(%u).",
                             m_pDrone->GetName(), m_pDrone->GetID(),
                             shipTarget->GetName(), shipTarget->GetID());
                        ClearTarget(pTarget);
                        Target(shipTarget);
                        return;
                    }
                }
            }
            CheckDistance(pTarget);
        } break;

        case DroneAI::State::Departing: { // return to ship.  when close enough, scoop or orbit
            if (m_assignedShip == nullptr or m_assignedShip->DestinyMgr() == nullptr) {
                // ship vanished (scoop/offline already ran AssignShip(nullptr)).
                // nothing to return to — go idle; DroneSE::Process will schedule removal.
                SetIdle();
                break;
            }
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
                ClearTarget(pTarget);
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

        case DroneAI::State::Assisting: {
            SystemEntity* pAssistSE = m_pDrone->SystemMgr()->GetSE(m_pDrone->GetAssistTargetID());
            if (pAssistSE == nullptr or pAssistSE->DestinyMgr() == nullptr) {
                SetIdle();
                break;
            }
            // Orbit the assisted player
            if (!m_pDrone->DestinyMgr()->IsOrbiting())
                m_pDrone->DestinyMgr()->Orbit(pAssistSE, m_entityOrbitRange);
            // Follow their NPC target
            SystemEntity* pAssistTarget = pAssistSE->TargetMgr()->GetFirstTarget(false);
            if (pAssistTarget != nullptr and !pAssistTarget->HasPilot()) {
                m_pDrone->DestinyMgr()->Follow(pAssistTarget, m_entityOrbitRange);
                Target(pAssistTarget);
            }
            CheckDistance(pAssistSE);
        } break;

        case DroneAI::State::Guarding: {
            SystemEntity* pGuardSE = m_pDrone->SystemMgr()->GetSE(m_pDrone->GetGuardTargetID());
            if (pGuardSE == nullptr) {
                SetIdle();
                break;
            }
            // Orbit the guarded player
            if (!m_pDrone->DestinyMgr()->IsOrbiting())
                m_pDrone->DestinyMgr()->Orbit(pGuardSE, m_entityOrbitRange);
            // Check bubble for anyone attacking the guarded player
            if (pGuardSE->SysBubble() != nullptr) {
                std::map<uint32, SystemEntity*> bubbleEnts;
                pGuardSE->SysBubble()->GetEntities(bubbleEnts);
                for (auto& cur : bubbleEnts) {
                    SystemEntity* pEntity = cur.second;
                    if (pEntity == nullptr or pEntity == m_pDrone or pEntity == pGuardSE) continue;
                    if (pEntity->TargetMgr() == nullptr) continue;
                    if (pGuardSE->TargetMgr()->IsTargetedBy(pEntity)) {
                        Target(pEntity);
                        break;
                    }
                }
            }
            CheckDistance(pGuardSE);
        } break;

        case DroneAI::State::Pursuit: {
            // Target out of attack range but within sight — chase with MWD speed.
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(true);
            if (pTarget == nullptr or pTarget->SysBubble() == nullptr) {
                ClearTarget(pTarget); SetIdle(); break;
            }
            if (pTarget->DestinyMgr() != nullptr
            and pTarget->DestinyMgr()->GetState() == Destiny::Ball::Mode::WARP) {
                ClearTarget(pTarget); SetIdle(); break;
            }
            double dist = m_pDrone->GetPosition().distance(pTarget->GetPosition());
            float range = m_entityFlyRange * (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneSharpshooting));
            if (dist > range * 1.5) {
                // Too far even for pursuit — give up
                ClearTarget(pTarget); SetIdle(); break;
            }
            // Chase at max speed
            float vel = m_chaseSpeed * 2 * (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::DroneNavigation));
            m_pDrone->DestinyMgr()->SetMaxVelocity(vel);
            m_pDrone->DestinyMgr()->Follow(pTarget, m_entityOrbitRange);
            // Recheck distance; if back in attack range, re-engage
            if (dist < m_entityAttackRange * (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneSharpshooting)))
                SetEngaged(pTarget);
        } break;

        case DroneAI::State::Operating: {
            // Mining drone actively mining — stay at asteroid and cycle
            SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(true);
            if (pTarget == nullptr) { SetIdle(); break; }
            if (!m_miningTimer.Enabled())
                m_miningTimer.Start(m_attackSpeed);
            if (m_miningTimer.Check())
                MiningAttack(pTarget);
        } break;

        case DroneAI::State::Fleeing: {
            // Drone running away — head back to ship at max speed
            if (m_assignedShip == nullptr) { SetIdle(); break; }
            float vel = m_chaseSpeed * 3;
            m_pDrone->DestinyMgr()->SetMaxVelocity(vel);
            m_pDrone->DestinyMgr()->Follow(m_assignedShip, 0);
            double dist = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
            if (dist < m_entityOrbitRange * 2)
                SetIdle();
        } break;

        case DroneAI::State::Unknown:
        case DroneAI::State::Incapacitated:
        case DroneAI::State::Combat:
        case DroneAI::State::Departing2: {
           // reported only — handled by GetState() mapping
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
    // Clean up target paint / web / scramble on any lingering target (drone is
    // returning to idle — e.g. via DroneSE::Killed — so release all applied EWAR).
    if (m_paintTargetID != 0) {
        SystemEntity* paintedSE = m_pDrone->SystemMgr()->GetSE(m_paintTargetID);
        if (paintedSE != nullptr)
            CleanupTargetEwar(paintedSE);
    }
    // not doing anything....idle.
    m_pDrone->ClearTargetID();
    m_pDrone->StateChange();
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: returning to idle.",
         m_pDrone->GetName(), m_pDrone->GetID());
    // Reload fighter ammo on return to carrier
    if (m_pDrone->IsFighter() and !m_pDrone->IsFighterBomber() and (m_pDrone->GetFighterAmmo() < m_pDrone->GetFighterMaxAmmo())) {
        m_pDrone->ReloadFighter();
        _log(DRONE__AI_TRACE, "Fighter %s(%u): Reloaded to %u ammo on return to carrier.",
             m_pDrone->GetName(), m_pDrone->GetID(), m_pDrone->GetFighterAmmo());
    }
    // Fighter-bombers do NOT auto-reload here — they return to bay (ReturnBay)
    // for reload, matching EVE behaviour.  Reloading in space + re-engaging a
    // stale target caused bombers to chase a distant target into deep space.

    m_state = DroneAI::State::Idle;
    m_beginFindTarget.Disable();
    m_mainAttackTimer.Disable();

    // After reload, re-engage the last target if it still exists — but only for
    // regular fighters, NOT fighter-bombers.  Bombers that auto-returned (Return)
    // without ReturnBay have a stale TargetMgr entry pointing at a now-distant
    // target; re-engaging it sends the bomber flying off into deep space.
    if (!m_pDrone->IsFighterBomber() and m_pDrone->TargetMgr() != nullptr) {
        SystemEntity* pTarget = m_pDrone->TargetMgr()->GetFirstTarget(false);
        if (pTarget != nullptr) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): SetIdle: re-engaging last target %s(%u) after reload.",
                 m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
            SetApproaching(pTarget);
            return;
        }
    }

    // disable ewar timers and release EWAR on any remaining targets
    if (m_pDrone->TargetMgr() != nullptr) {
        m_webifierTimer.Disable();
        m_warpScramblerTimer.Disable();
        m_paintTimer.Disable();
        PyList* targets = m_pDrone->TargetMgr()->GetTargets();
        if (targets != nullptr) {
            for (PyRep* t : targets->items) {
                uint32 tID = PyRep::IntegerValueU32(t);
                SystemEntity* tSE = m_pDrone->SystemMgr()->GetSE(tID);
                if (tSE != nullptr)
                    CleanupTargetEwar(tSE);
            }
        }
    }

    // orbit assigned ship (guard against stale m_assignedShip)
    if (m_assignedShip != nullptr and m_pDrone->DestinyMgr() != nullptr)
        m_pDrone->IdleOrbit(m_assignedShip);
}

void DroneAIMgr::SetEngaged(SystemEntity* pTarget) {
    if (m_state == DroneAI::State::Engaged)
        return;
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetEngaged: %s(%u) begin engaging.",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID());
    // use chase speed if far from carrier, cruise speed for close orbit
    double distToShip = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
    double controlRange = GetControlRange();
    float skillBonus = 1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::DroneNavigation);
    // Ship module speed bonus (Drone Navigation Computer)
    if (m_assignedShip != nullptr and m_assignedShip->GetSelf()->HasAttribute(AttrDroneMaxVelocityBonus))
        skillBonus *= (1.0f + m_assignedShip->GetSelf()->GetAttribute(AttrDroneMaxVelocityBonus).get_float());
    float vel;
    if (distToShip > controlRange) {
        vel = m_chaseSpeed * skillBonus;
        m_pDrone->DestinyMgr()->Follow(pTarget, m_entityOrbitRange);
    } else {
        // Sync drone position to client BEFORE issuing CmdOrbit. When switching
        // from chasing/approaching a target to orbiting it, the client's Ballpark
        // re-anchors the ball to the new orbit point from its own (desynced)
        // position — visible as a teleport to "the other side of space". Sending
        // the current server position first lets the client lerp smoothly (same
        // pattern as SetApproaching and DroneSE::IdleOrbit).
        m_pDrone->DestinyMgr()->SetPosition(m_pDrone->GetPosition(), true);
        vel = m_cruiseSpeed * skillBonus;
        m_pDrone->DestinyMgr()->Orbit(pTarget, m_entityOrbitRange);
    }
    m_pDrone->DestinyMgr()->SetMaxVelocity(vel);
    m_state = DroneAI::State::Engaged;
}

void DroneAIMgr::SetApproaching(SystemEntity* pSE)
{
    if (m_state == DroneAI::State::Approaching)
        return;
    _log(DRONE__AI_TRACE, "Drone %s(%u): SetApproaching: %s(%u) begin pursuit.",
         m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID());
    // Sync drone position to client before starting pursuit — prevents
    // desync from orbit→follow transition when target is far away.
    m_pDrone->DestinyMgr()->SetPosition(m_pDrone->GetPosition(), true);
    float vel = m_chaseSpeed * (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::DroneNavigation));
    // Ship module speed bonus (Drone Navigation Computer)
    if (m_assignedShip != nullptr and m_assignedShip->GetSelf()->HasAttribute(AttrDroneMaxVelocityBonus))
        vel *= (1.0f + m_assignedShip->GetSelf()->GetAttribute(AttrDroneMaxVelocityBonus).get_float());
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
    // Ship module range bonus (Drone Control Range Module)
    if (m_assignedShip->GetSelf()->HasAttribute(AttrDroneRangeBonus))
        range *= (1.0f + m_assignedShip->GetSelf()->GetAttribute(AttrDroneRangeBonus).get_float());

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
        ClearTarget(pSE);
        SetIdle();
        return;
    }

    double dist = m_pDrone->GetPosition().distance(pSE->GetPosition());
    // Drone Sharpshooting: +10% optimal/falloff per level
    float rangeMult = 1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneSharpshooting);
    float flyRange = m_entityFlyRange * rangeMult;
    float attackRange = m_entityAttackRange * rangeMult;

    // Control-range leash: a drone must not chase a target beyond the carrier's
    // control range. NPCs with huge flyRange (e.g. Eradicator 27km) pull drones
    // away from the ship into "deep space" (visible as drones flinging off /
    // orbiting nothing). If the target is outside control range, drop it and
    // return to the carrier — same behaviour as EVE.
    if (m_assignedShip != nullptr and m_assignedShip->DestinyMgr() != nullptr) {
        double distToShip = m_pDrone->GetPosition().distance(m_assignedShip->GetPosition());
        if (distToShip > GetControlRange()) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): target %s(%u) beyond control range (%.0fm > %.0fm). Returning to ship.",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID(),
                 distToShip, GetControlRange());
            m_pDrone->DestinyMgr()->Stop();
            ClearTarget(pSE);
            Return();
            return;
        }
    }

    // If we're approaching and still far away, keep chasing
    if ((m_state == DroneAI::State::Approaching) && (dist > flyRange)) {
        // keep approaching — flyRange only limits attack, not pursuit
        return;
    }

    if (dist > flyRange) {
        if (m_state == DroneAI::State::Mining) {
            m_pDrone->DestinyMgr()->Follow(pSE, m_entityOrbitRange);
            return;
        }
        // Target out of fly range — approach/chase instead of giving up immediately
        if (dist < flyRange * 2.0f) {
            SetApproaching(pSE);
            // fall through to attack timer — drone can fire while chasing
        } else {
            _log(DRONE__AI_TRACE, "Drone %s(%u): CheckDistance: %s(%u) too far (%.0f).  Clear target.",
                 m_pDrone->GetName(), m_pDrone->GetID(), pSE->GetName(), pSE->GetID(), dist);
            ClearTarget(pSE);
            return;
        }
    }
    if (dist > attackRange) {
        // within fly range but outside attack range — approach
        if (m_state == DroneAI::State::Mining) {
            m_pDrone->DestinyMgr()->Follow(pSE, m_entityOrbitRange);
            return;
        }
        SetApproaching(pSE);
        // Start attack timer so drone fires while approaching
        if (!m_mainAttackTimer.Enabled()) {
            m_mainAttackTimer.Start(m_attackSpeed);
            AttackTarget(pSE);
        }
        Attack(pSE);
        return; // Don't fall through to SetEngaged — target still out of attack range
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
        if (m_pDrone->SysBubble() == nullptr or !m_pDrone->SysBubble()->InBubble(pSE->GetPosition())) {
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
    // Release any EWAR (web / warp scramble / target paint) this drone applied
    // to the target before dropping it. Without this the web slows the target
    // forever, the warp-scramble status stays set (blocking warp) and the paint
    // keeps the signature inflated — same bug as NPCAIMgr::ClearTarget.
    CleanupTargetEwar(pSE);

    m_pDrone->TargetMgr()->ClearTarget(pSE);
    //m_pDrone->TargetMgr()->OnTarget(pSE, TargMgr::Mode::Lost);

    if (m_pDrone->TargetMgr()->HasNoTargets())
        SetIdle();
}

void DroneAIMgr::CleanupTargetEwar(SystemEntity* pSE) {
    if ((pSE == nullptr) or (m_pDrone->DestinyMgr() == nullptr))
        return;
    InventoryItemRef targetItem = pSE->GetSelf();
    if (targetItem.get() == nullptr)
        return;

    // Warp scramble — clear status + stop the sticky client beam.
    if (m_subType == DroneAI::SubType_WarpScramble) {
        if (targetItem->HasAttribute(AttrWarpScrambleStatus))
            targetItem->SetAttribute(AttrWarpScrambleStatus, 0.0f);
        m_warpScramblerTimer.Disable();
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
                                                 pSE->GetID(), 0, "effects.WarpScramble",
                                                 1, 0, 0, m_attackSpeed, 0, 0);
    }

    // Stasis web — restore target speed + stop the beam. WebbedMe(false) reads
    // AttrSpeedFactor from the drone, so temporarily re-apply the boosted factor
    // to cancel the exact multiplication done in WebAttack (symmetric undo).
    if (m_subType == DroneAI::SubType_Web and m_webApplied and m_webTargetID == pSE->GetID()
    and pSE->DestinyMgr() != nullptr) {
        InventoryItemRef droneRef = m_pDrone->GetSelf();
        if (droneRef->HasAttribute(AttrSpeedFactor)) {
            float skillMult = 1.0f;
            if (m_pDrone->GetOwner() != nullptr)
                skillMult += 0.10f * GetOwnerSkillLevel(EvESkill::PropulsionJammingDroneInterfacing);
            float origFactor = droneRef->GetAttribute(AttrSpeedFactor).get_float();
            droneRef->SetAttribute(AttrSpeedFactor, origFactor * skillMult, false);
            pSE->DestinyMgr()->WebbedMe(droneRef, false);
            droneRef->SetAttribute(AttrSpeedFactor, origFactor, false);
        }
        m_webApplied = false;
        m_webTargetID = 0;
        m_webifierTimer.Disable();
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
                                                 pSE->GetID(), 0, "effects.ModifyTargetSpeed",
                                                 1, 0, 0, m_attackSpeed, 0, 0);
    }

    // Target paint — restore the original signature radius + stop the beam.
    if (m_subType == DroneAI::SubType_TargetPaint and m_paintTargetID == pSE->GetID()) {
        if (targetItem->HasAttribute(AttrSignatureRadius))
            targetItem->SetAttribute(AttrSignatureRadius, m_paintedSigRadius, false);
        m_paintTargetID = 0;
        m_paintedSigRadius = 0.0f;
        m_paintTimer.Disable();
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
                                                 pSE->GetID(), 0, "effects.TargetPaint",
                                                 1, 0, 0, m_attackSpeed, 0, 0);
    }
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
        case DroneAI::SubType_TargetPaint:
            PaintAttack(pTarget);
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
    std::string guid = "effects.StandardWeapon"; // client's StandardWeapon class handles all turret types
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxTurretID))// graphicID for turret for drone type ships
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxTurretID).get_uint32();
    if (m_pDrone->DestinyMgr() != nullptr and m_pDrone->SysBubble() != nullptr)
        m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->itemID(),
                                                 m_pDrone->GetSelf()->typeID(),
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
                  * (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneInterfacing));
        // heavy vs sentry damage skill
        if (m_subType == DroneAI::SubType_Sentry)
            skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(EvESkill::SentryDroneInterfacing));
        else
            skillMult *= (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::HeavyDroneOperation));
        // racial specialization (+2% per level)
        int8 raceID = m_pDrone->GetSelf()->type().race();
        uint16 racialSkill = (raceID == 1 ? EvESkill::CaldariDroneSpecialization
                           : raceID == 2 ? EvESkill::MinmatarDroneSpecialization
                           : raceID == 4 ? EvESkill::AmarrDroneSpecialization
                           : raceID == 8 ? EvESkill::GallenteDroneSpecialization
                           : 0);
        if (racialSkill != 0)
            skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(racialSkill));
        // Advanced Drone Interfacing (+2% per level, all drones)
        skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(EvESkill::AdvancedDroneInterfacing));
    }
    d *= skillMult;

    // Apply ship module bonuses (Drone Damage Amplifier, etc.)
    if (m_assignedShip != nullptr) {
        InventoryItemRef shipRef = m_assignedShip->GetSelf();
        if (shipRef->HasAttribute(AttrDroneDamageBonus))
            d *= (1.0f + shipRef->GetAttribute(AttrDroneDamageBonus).get_float());
    }

    d *= sConfig.rates.damageRate;      /** @todo this should be a separate config value */
    _log(DRONE__AI_TRACE, "Drone %s(%u): CombatAttack -> %s(%u) total=%.2f (K:%.1f T:%.1f EM:%.1f E:%.1f mult=%.2f skill=%.2f hit=%.3f rate=%.3f)",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(),
         m_pDrone->GetKinetic(), m_pDrone->GetThermal(), m_pDrone->GetEM(), m_pDrone->GetExplosive(),
         dmgMult, skillMult, d.GetModifier(), sConfig.rates.damageRate);
    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
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

    // Notify crime watch of weapon use
    if (m_pDrone->GetOwner() != nullptr and m_pDrone->GetOwner()->GetCrimeWatch() != nullptr)
        m_pDrone->GetOwner()->GetCrimeWatch()->OnWeaponFired();

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
        skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(EvESkill::AdvancedDroneInterfacing));
        d *= skillMult;
    }

    // Apply ship module bonuses (Drone Damage Amplifier, etc.)
    if (m_assignedShip != nullptr) {
        InventoryItemRef shipRef = m_assignedShip->GetSelf();
        if (shipRef->HasAttribute(AttrDroneDamageBonus))
            d *= (1.0f + shipRef->GetAttribute(AttrDroneDamageBonus).get_float());
    }

    d *= sConfig.rates.damageRate;
    _log(DRONE__AI_TRACE, "Fighter %s(%u): FighterAttack -> %s(%u) total=%.2f ammo=%u/%u",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(), m_pDrone->GetFighterAmmo(), m_pDrone->GetFighterMaxAmmo());
    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
    if (pTarget->ApplyDamage(d)) {
        return;
    }
}

void DroneAIMgr::FighterBomberAttack(SystemEntity* pTarget) {
    // Fighter Bomber: AoE bomb attack, return to carrier when empty
    if (!m_pDrone->ConsumeFighterAmmo()) {
        ReturnBay();   // scoop into bay for bomb reload (not bare Return)
        return;
    }

    // Notify crime watch of weapon use
    if (m_pDrone->GetOwner() != nullptr and m_pDrone->GetOwner()->GetCrimeWatch() != nullptr)
        m_pDrone->GetOwner()->GetCrimeWatch()->OnWeaponFired();

    // Bomb visual effect
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.StandardWeapon", 1, 1, 1, m_attackSpeed, 0, 0);

    // Bomber damage (higher base than regular fighters). Fighter bombers are AoE
    // munitions (SDE: aoeCloudSize/aoeVelocity/proximityRange) — they detonate on
    // reaching the target, so they ALWAYS hit (toHit=1.0). Using GetDroneToHit
    // here produced "misses completely" every shot: the SDE fighter-bomber hulls
    // have no trackingSpeed/falloff/optimalSigRadius attributes (they're not
    // turrets), so the formula divided by zero -> toHit 0 -> "too far away".
    Damage d(m_pDrone,
             m_pDrone->GetSelf(),
             m_pDrone->GetKinetic(),
             m_pDrone->GetThermal(),
             m_pDrone->GetEM(),
             m_pDrone->GetExplosive(),
             1.0f,          // fighter-bomber munitions always connect
             EVEEffectID::targetAttack
            );

    float dmgMult = m_pDrone->GetSelf()->HasAttribute(AttrDamageMultiplier)
        ? m_pDrone->GetSelf()->GetAttribute(AttrDamageMultiplier).get_float() : 1.0f;
    d *= dmgMult;

    if (m_pDrone->GetOwner() != nullptr) {
        float skillMult = 1.0f;
        skillMult *= (1.0f + 0.05f * GetOwnerSkillLevel(EvESkill::FighterBombers));
        skillMult *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::DroneInterfacing));
        skillMult *= (1.0f + 0.02f * GetOwnerSkillLevel(EvESkill::AdvancedDroneInterfacing));
        d *= skillMult;
    }

    // Apply ship module bonuses (Drone Damage Amplifier, etc.)
    if (m_assignedShip != nullptr) {
        InventoryItemRef shipRef = m_assignedShip->GetSelf();
        if (shipRef->HasAttribute(AttrDroneDamageBonus))
            d *= (1.0f + shipRef->GetAttribute(AttrDroneDamageBonus).get_float());
    }

    d *= sConfig.rates.damageRate;

    // Missile explosion formula — same as Missile::HitTarget but applied to the
    // bomb's raw damage.  Without this, target painters are useless against
    // fighter-bombers: they increase signature radius (Sr) but the server never
    // feeds Sr into a damage modifier.  With the formula the painter actually
    //amplify the damage through the Sr/Er ratio.
    if (pTarget->GetSelf().get() != nullptr
            && m_pDrone->GetSelf()->HasAttribute(AttrAoeCloudSize)
            && m_pDrone->GetSelf()->HasAttribute(AttrAoeVelocity)
            && m_pDrone->GetSelf()->HasAttribute(AttrAoeDamageReductionFactor)
            && m_pDrone->GetSelf()->HasAttribute(AttrAoeDamageReductionSensitivity)
            && pTarget->GetSelf()->HasAttribute(AttrSignatureRadius)) {
        double Sr = pTarget->GetSelf()->GetAttribute(AttrSignatureRadius).get_float();
        double Er = m_pDrone->GetSelf()->GetAttribute(AttrAoeCloudSize).get_float();
        double Ev = m_pDrone->GetSelf()->GetAttribute(AttrAoeVelocity).get_float();
        double DRF = m_pDrone->GetSelf()->GetAttribute(AttrAoeDamageReductionFactor).get_float();
        double DRS = m_pDrone->GetSelf()->GetAttribute(AttrAoeDamageReductionSensitivity).get_float();
        if (Er > 0 && Ev > 0 && DRF > 0 && DRS > 0) {
            GPoint Vel = pTarget->GetVelocity();
            double V = Vel.length();
            if (V <= 0) V = 1;
            double v1 = Sr / Er;
            double v2 = pow((Ev / V) * v1, log(DRF) / log(DRS));
            float modifier = static_cast<float>(EvE::min1(v1, v2));
            d *= modifier;
            _log(DRONE__AI_TRACE, "Bomber %s(%u): missile formula Sr=%.0f Er=%.0f Ev=%.0f V=%.0f mod=%.3f",
                 m_pDrone->GetName(), m_pDrone->GetID(), Sr, Er, Ev, V, modifier);
        }
    }

    // Apply damage to target
    _log(DRONE__AI_TRACE, "Bomber %s(%u): FighterBomberAttack -> %s(%u) total=%.2f ammo=%u/%u",
         m_pDrone->GetName(), m_pDrone->GetID(),
         pTarget->GetName(), pTarget->GetID(),
         d.GetTotal(), m_pDrone->GetFighterAmmo(), m_pDrone->GetFighterMaxAmmo());
    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
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

    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
    // Apply Propulsion Jamming Drone Interfacing skill (+10% web strength per level)
    float skillMult = 1.0f;
    if (m_pDrone->GetOwner() != nullptr)
        skillMult += 0.10f * GetOwnerSkillLevel(EvESkill::PropulsionJammingDroneInterfacing);
    // Temporarily boost speed factor so WebbedMe reads the modified value
    InventoryItemRef droneRef = m_pDrone->GetSelf();
    if (droneRef->HasAttribute(AttrSpeedFactor)) {
        float origFactor = droneRef->GetAttribute(AttrSpeedFactor).get_float();
        float boostedFactor = origFactor * skillMult;
        // The web stacks multiplicatively each cycle (WeppedMe multiplies
        // m_maxShipSpeed). Undo the previous application first so repeated cycles
        // don't collapse the target's speed to zero and so cleanup (ClearTarget /
        // SetIdle) can restore it with a single symmetric WebbedMe(false).
        if (m_webApplied) {
            if (m_webTargetID == pTarget->GetID()) {
                // Re-applying to the same target — undo then re-apply.
                if (pTarget->DestinyMgr() != nullptr) {
                    droneRef->SetAttribute(AttrSpeedFactor, boostedFactor, false);
                    pTarget->DestinyMgr()->WebbedMe(droneRef, false);
                    droneRef->SetAttribute(AttrSpeedFactor, origFactor, false);
                }
            } else {
                // Switching targets — release the web on the old one.
                SystemEntity* oldWebTarget = m_pDrone->SystemMgr()->GetSE(m_webTargetID);
                if (oldWebTarget != nullptr and oldWebTarget->DestinyMgr() != nullptr) {
                    droneRef->SetAttribute(AttrSpeedFactor, boostedFactor, false);
                    oldWebTarget->DestinyMgr()->WebbedMe(droneRef, false);
                    droneRef->SetAttribute(AttrSpeedFactor, origFactor, false);
                }
                m_webApplied = false;
                m_webTargetID = 0;
            }
        }
        droneRef->SetAttribute(AttrSpeedFactor, boostedFactor, false);
        pTarget->DestinyMgr()->WebbedMe(droneRef, true);
        droneRef->SetAttribute(AttrSpeedFactor, origFactor, false);
        m_webApplied = true;
        m_webTargetID = pTarget->GetID();
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

    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
    // Apply Propulsion Jamming Drone Interfacing skill (+10% scramble strength per level)
    float scrambleStr = m_ewarStrength;
    if (m_pDrone->GetOwner() != nullptr)
        scrambleStr *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::PropulsionJammingDroneInterfacing));
    // Set WarpScrambleStatus on target
    InventoryItemRef targetRef = pTarget->GetSelf();
    if (targetRef->HasAttribute(AttrWarpScrambleStatus)) {
        targetRef->SetAttribute(AttrWarpScrambleStatus, scrambleStr);
    } else {
        targetRef->SetAttribute(AttrWarpScrambleStatus, scrambleStr, false);
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

    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
    // Apply Electronic Warfare Drone Interfacing skill to ECM strength
    float ecmStr = m_ewarStrength;
    if (m_pDrone->GetOwner() != nullptr)
        ecmStr *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::ElectronicWarfareDroneInterfacing));
    // ECM chance calc: roll against target's strongest sensor type
    InventoryItemRef targetRef = pTarget->GetSelf();
    float maxSensorStr = 0.0f;
    if (targetRef->HasAttribute(AttrScanGravimetricStrength))
        maxSensorStr = EvE::max(maxSensorStr, targetRef->GetAttribute(AttrScanGravimetricStrength).get_float());
    if (targetRef->HasAttribute(AttrScanRadarStrength))
        maxSensorStr = EvE::max(maxSensorStr, targetRef->GetAttribute(AttrScanRadarStrength).get_float());
    if (targetRef->HasAttribute(AttrScanMagnetometricStrength))
        maxSensorStr = EvE::max(maxSensorStr, targetRef->GetAttribute(AttrScanMagnetometricStrength).get_float());
    if (targetRef->HasAttribute(AttrScanLadarStrength))
        maxSensorStr = EvE::max(maxSensorStr, targetRef->GetAttribute(AttrScanLadarStrength).get_float());
    if (maxSensorStr > 0.0f) {
        float chance = ecmStr / maxSensorStr;
        if (MakeRandomFloat(0.0f, 1.0f) < chance) {
            _log(DRONE__AI_TRACE, "Drone %s(%u): ECM success on %s(%u) (str=%.2f vs sensor=%.2f, chance=%.2f).",
                 m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(),
                 ecmStr, maxSensorStr, chance);
            pTarget->TargetMgr()->ClearTargets();
        } else {
            _log(DRONE__AI_TRACE, "Drone %s(%u): ECM fail on %s(%u) (str=%.2f vs sensor=%.2f, chance=%.2f).",
                 m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(),
                 ecmStr, maxSensorStr, chance);
        }
    } else {
        // Fallback: always break locks if no sensor strength found
        pTarget->TargetMgr()->ClearTargets();
    }
}

void DroneAIMgr::PaintAttack(SystemEntity* pTarget) {
    _log(DRONE__AI_TRACE, "Drone %s(%u): PaintAttack on %s(%u) with strength %.2f",
         m_pDrone->GetName(), m_pDrone->GetID(), pTarget->GetName(), pTarget->GetID(), m_ewarStrength);

    // Send target painter visual effect
    uint32 gfxID = 0;
    if (m_pDrone->GetSelf()->HasAttribute(AttrGfxBoosterID))
        gfxID = m_pDrone->GetSelf()->GetAttribute(AttrGfxBoosterID).get_uint32();
    m_pDrone->DestinyMgr()->SendSpecialEffect(m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->itemID(),
                                             m_pDrone->GetSelf()->typeID(),
                                             pTarget->GetID(),
                                             0, "effects.TargetPaint",
                                             1, 1, 1, m_attackSpeed, 0, gfxID);

    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
        }
    }
    // Apply signature radius bonus to target
    float paintStr = m_ewarStrength;
    if (owner != nullptr)
        paintStr *= (1.0f + 0.10f * GetOwnerSkillLevel(EvESkill::ElectronicWarfareDroneInterfacing));
    InventoryItemRef targetRef = pTarget->GetSelf();
    if (targetRef->HasAttribute(AttrSignatureRadius)) {
        // If already painting this exact target, just keep the original base and
        // re-apply (no stacking). Otherwise restore the previous target first.
        if (m_paintTargetID != 0 and m_paintTargetID != pTarget->GetID()) {
            SystemEntity* oldTarget = m_pDrone->SystemMgr()->GetSE(m_paintTargetID);
            if (oldTarget != nullptr and oldTarget->GetSelf()->HasAttribute(AttrSignatureRadius))
                oldTarget->GetSelf()->SetAttribute(AttrSignatureRadius, m_paintedSigRadius, false);
            m_paintTargetID = 0;
            m_paintedSigRadius = 0.0f;
        }
        if (m_paintTargetID != pTarget->GetID()) {
            // First time painting this target — remember the ORIGINAL signature
            // radius so cleanup restores the true base (repeated cycles must not
            // stack the paint, which would inflate sig forever and make the undo
            // restore a too-high value).
            m_paintedSigRadius = targetRef->GetAttribute(AttrSignatureRadius).get_float();
            m_paintTargetID = pTarget->GetID();
        }
        targetRef->SetAttribute(AttrSignatureRadius, m_paintedSigRadius * (1.0f + paintStr / 100.0f), false);
    }
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
    Client* owner = m_pDrone->GetOwner();
    if (owner != nullptr and owner->GetCrimeWatch() != nullptr) {
        owner->GetCrimeWatch()->OnWeaponFired();
        if (pTarget->HasPilot() and pTarget->GetPilot() != owner) {
            float sec = owner->SystemMgr()->GetSystemSecurityRating();
            owner->GetCrimeWatch()->OnAggression(pTarget->GetPilot(), sec);
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
    // apply Mining Drone Specialization bonus (+2% per level)
    miningAmount *= (1.0f + 0.02f * GetOwnerSkillLevel(EvESkill::MiningDroneSpecialization));

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
            ClearTarget(pTarget);
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

    // Deplete the asteroid: subtract mined units from its quantity. Without this
    // mining drones keep extracting from the same rock forever ('never-depleting
    // ore'). When quantity runs out, remove the asteroid from the system so the
    // client drops its ball (same behaviour as MiningLaser::ProcessCycle).
    if (roidRef->HasAttribute(AttrQuantity)) {
        float qty = roidRef->GetAttribute(AttrQuantity).get_float() - oreUnits;
        if (qty <= 0.0f) {
            if (pTarget->DestinyMgr() != nullptr)
                pTarget->DestinyMgr()->Stop();
            ClearTarget(pTarget);
            pTarget->Delete();   // removes from system + sends RemoveBall to bubble
            SetIdle();
            m_pDrone->StateChange();
            return;
        }
        roidRef->SetAttribute(AttrQuantity, qty, false);
        // shrink the rock as it depletes (reverse of belt radius-from-qty formula)
        if (!roidRef->HasAttribute(AttrRadius) || roidRef->GetAttribute(AttrRadius).get_float() > 100.0f) {
            double radius = exp((qty + 112404.8) / 25000);
            roidRef->SetAttribute(AttrRadius, radius, false);
        }
    }

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
        ClearTarget(pTarget);
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

// Nearest hostile target within control range. Used by the drone's aggressive
// and the fighter's attack-and-follow behaviour: the drone picks the closest
// hostile and engages on its own (like an EVE aggressive drone guarding a miner).
// Hostiles include NPC rats, NPC defender drones, and hostile structures such as a
// TCU/tower being besieged — anything lockable that is not friendly to the owner.
SystemEntity* DroneAIMgr::FindAggroTarget() {
    if (m_pDrone->SysBubble() == nullptr)
        return nullptr;
    const uint32 ownerID = m_pDrone->GetSelf()->ownerID();
    const int32 ownerWar = m_pDrone->GetWarFactionID();
    std::map<uint32, SystemEntity*> entities;
    m_pDrone->SysBubble()->GetAllEntities(entities);
    double controlRange = GetControlRange();
    SystemEntity* best = nullptr;
    double bestDist = 1e18;
    for (auto& [id, se] : entities) {
        if (se == nullptr || se->GetSelf().get() == nullptr)
            continue;
        // Must be a lockable combat target (has a TargetManager). This excludes
        // scenery with no TargetManager (decor, clouds, acceleration gates).
        if (se->TargetMgr() == nullptr)
            continue;
        // Never attack the owner's own ship or the owner's own drones (friendly).
        if (se->GetSelf()->ownerID() == ownerID)
            continue;
        // Skip friendlies: same non-zero war faction (e.g. same militia in Faction War).
        const int32 seWar = se->GetWarFactionID();
        if (seWar != 0 && seWar == ownerWar)
            continue;
        // Don't attack the player's own bot allies (chelobots share the pilot-less
        // path but are friendly assets).
        if (se->IsNPCSE() && se->GetNPCSE()->IsPlayerBot())
            continue;
        // Self-preservation: don't engage drones belonging to a capital ship
        // (carrier / supercarrier / titan).  A frigate fighting near a capital
        // is suicidal — the capital's main guns/fighters will destroy it.
        if (se->IsDroneSE()) {
            DroneSE* drone = se->GetDroneSE();
            if (drone != nullptr) {
                Client* droneOwner = drone->GetOwner();
                if (droneOwner != nullptr) {
                    ShipItemRef ownerShip = droneOwner->GetShip();
                    if (ownerShip.get() != nullptr && m_pDrone->SystemMgr() != nullptr) {
                        SystemEntity* ownerSE = m_pDrone->SystemMgr()->GetSE(ownerShip->itemID());
                        if (ownerSE != nullptr && ownerSE->GetSelf().get() != nullptr) {
                            uint16 grp = ownerSE->GetSelf()->groupID();
                            if (grp == EVEDB::invGroups::Carrier
                                    || grp == EVEDB::invGroups::Supercarrier
                                    || grp == EVEDB::invGroups::Titan)
                                continue;   // carrier/supercarrier/titan nearby — skip
                        }
                    }
                }
            }
        }
        double d = m_pDrone->GetPosition().distance(se->GetPosition());
        if (d < bestDist) { bestDist = d; best = se; }
    }
    if (best != nullptr && bestDist <= controlRange)
        return best;
    return nullptr;
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
