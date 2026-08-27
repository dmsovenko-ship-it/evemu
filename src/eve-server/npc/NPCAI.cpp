/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Zhur
    Rewrite:    Allan
    AI Version: 0.57
*/

/** @todo  ai update ideas
 *   bubble call *SomeFunction* to tell ai of new ship arriving in bubble
 *   method to use npc's preferred sig radius for targets
 *   finish flee and signal action methods (and determine who can use them and when)
 *      - this should take system sov, npc anomalies, destruction speed, and pirate faction
 *   add methods to check target/targeter warping out and chance of npc following (and possibly calling backup)
 *
 *  have data...needs coding...
 *   chase duration/distance timers
 *   ewar shit, including point/tackle
 */

#include "eve-server.h"

#include "Client.h"
#include "inventory/AttributeEnum.h"
#include "npc/NPC.h"
#include "npc/NPCAI.h"
#include "npc/PlayerBot.h"
#include "ship/Missile.h"
#include "system/DestinyManager.h"
#include "system/Damage.h"
#include "system/SystemBubble.h"
#include "standing/StandingDB.h"

NPCAIMgr::NPCAIMgr(NPC* who)
: m_state(NPCAI::State::Idle),
  m_npc(who),
  m_destiny(who->DestinyMgr()),
  m_self(who->GetSelf()),
  m_processTimer(0),
  m_mainAttackTimer(0),
  m_missileTimer(0),
  m_warpOutTimer(0),
  m_shieldBoosterTimer(0),
  m_armorRepairTimer(0),
  m_beginFindTarget(0),
  m_warpScramblerTimer(0),
  m_webifierTimer(0),
  m_ambushWakeTimer(0),
  m_ambushTimeout(0),
  m_missileTypeID(0),
  m_webber(false),
  m_warpScram(false),
  m_isWandering(false),
  m_isAmbush(false),
  m_isStationary(false),
  m_ewarOnly(false),
  m_neutRange(0), m_neutAmount(0), m_neutChance(0), m_neutDuration(0), m_neutTimer(0)
{
    assert(m_self.get() != nullptr);
    m_damageMultiplier = m_self->GetAttribute(AttrDamageMultiplier).get_float();

    // All NPC sentry/turret structures are stationary — they fire in place and
    // never chase/orbit/wander. Treating them as moving NPCs made them "fly"
    // toward targets (flyRange fallback = radius*5 > 0).
    //   Sentry Gun (99), Protective Sentry Gun (180), Mobile Sentry Gun (336),
    //   Destructible Sentry Gun (383: Tower Sentry/Sentry Gun/Stasis Tower by
    //   faction+level), Mobile Missile Sentry (417: Light/Heavy/Cruise/Torpedo/
    //   Citadel Torpedo Battery).
    uint32 grp = m_self->groupID();
    if (grp == EVEDB::invGroups::Sentry_Gun
     || grp == EVEDB::invGroups::Protective_Sentry_Gun
     || grp == EVEDB::invGroups::Mobile_Sentry_Gun
     || grp == EVEDB::invGroups::Destructible_Sentry_Gun
     || grp == EVEDB::invGroups::Mobile_Missile_Sentry)
        m_isStationary = true;

    // Turret attack TYPE must match the turret's role:
    //   - Tower Sentry / Sentry Guns: direct turret fire (has weapon damage).
    //   - Stasis Towers / Energy Neutralizers: EWAR-only, NO weapon damage.
    //   - Missile Batteries (group 417): fire actual missiles (from chargeGroup).
    // A turret is "EWAR-only" when it has EWAR attributes but zero weapon damage
    // (all four damage types are 0/absent). Missile batteries never have direct
    // damage either — they're handled by the missile path below.
    bool hasWeaponDmg =
        (m_self->GetAttribute(AttrEmDamage).get_float() > 0.0f)
     || (m_self->GetAttribute(AttrKineticDamage).get_float() > 0.0f)
     || (m_self->GetAttribute(AttrThermalDamage).get_float() > 0.0f)
     || (m_self->GetAttribute(AttrExplosiveDamage).get_float() > 0.0f);

    // Missile batteries: pick the actual missile type from the battery's charge
    // group (384 Light / 385 Heavy / 386 Cruise / 89 Torpedo / 476 Citadel Torpedo).
    if (grp == EVEDB::invGroups::Mobile_Missile_Sentry) {
        uint16 missileTypeID = 0;
        if (m_self->HasAttribute(AttrChargeGroup1))
            missileTypeID = PickMissileForChargeGroup(m_self->GetAttribute(AttrChargeGroup1).get_uint32());
        if (missileTypeID == 0)
            missileTypeID = 210;   // Scourge Light Missile fallback
        m_self->SetAttribute(AttrEntityMissileTypeID, missileTypeID, false);
    }
    if (m_self->HasAttribute(AttrEntityMissileTypeID) && m_self->GetAttribute(AttrEntityMissileTypeID).get_uint32() > 0)
        m_missileTypeID = m_self->GetAttribute(AttrEntityMissileTypeID).get_uint32();

    // EWAR-only if no direct weapon damage and this is a sentry/turret structure.
    if (m_isStationary && !hasWeaponDmg && m_missileTypeID == 0)
        m_ewarOnly = true;

    // Energy neutralizer turrets (Sansha Energy Neutralizer Sentry etc.)
    if (m_self->HasAttribute(AttrEntityCapacitorDrainMaxRange)) {
        m_neutRange = m_self->GetAttribute(AttrEntityCapacitorDrainMaxRange).get_uint32();
        m_neutAmount = m_self->GetAttribute(AttrEntityCapacitorDrainAmount).get_float();
        m_neutDuration = m_self->GetAttribute(AttrEntityCapacitorDrainDuration).get_uint32();
        // entityCapacitorDrainDurationChance is the chance to apply (1 = always),
        // same convention as SleeperAI::EnergyNeut.
        if (m_self->HasAttribute(AttrEntityCapacitorDrainDurationChance))
            m_neutChance = m_self->GetAttribute(AttrEntityCapacitorDrainDurationChance).get_float();
        else
            m_neutChance = 1.0f;
        if (m_neutDuration == 0) m_neutDuration = m_attackSpeed;
    }

    /* set npc ship data */
    m_sigResolution = m_self->GetAttribute(AttrOptimalSigRadius).get_uint32();
    m_attackSpeed = m_self->GetAttribute(AttrSpeed).get_uint32();
    // AttrSpeed is the weapon cycle in ms, but some NPC types have absurd values
    // (e.g. 30000 = 30s on Guristas Plunderer/Mortifier) making them seem dead.
    if (m_attackSpeed < 500 or m_attackSpeed > 15000)
        m_attackSpeed = MakeRandomInt(3000, 8000);
    m_sigRadius = m_self->GetAttribute(AttrSignatureRadius).get_uint32();
    m_launcherCycleTime = m_self->GetAttribute(AttrMissileLaunchDuration).get_uint32();
    if (m_launcherCycleTime > 100)
        m_missileTypeID = m_self->GetAttribute(AttrEntityMissileTypeID).get_uint32();

    //  AttrEntityDefenderChance = 497,  <<< for defender missiles

    /** @todo  all of these need to be verified and/or updated */

    // ship speeds
    // absolute (boosted) Max Ship Speed
    m_maxSpeed = m_self->GetAttribute(AttrMaxVelocity).get_uint32();
    // Orbit Velocity
    m_orbitSpeed = m_self->GetAttribute(AttrEntityCruiseSpeed).get_uint32();   // ship speed when not chasing target
    // Some NPC types in the DB have no maxVelocity / cruise speed (0). That makes
    // Orbit() produce 'velocity <=0' and the ship can't align/warp — spamming
    // logs and leaving the rat stuck. Fall back to a sensible speed by hull size.
    if (m_maxSpeed == 0 || m_orbitSpeed == 0) {
        float r = m_self->GetAttribute(AttrRadius).get_float();
        uint16 fallback = 250;
        if (r >= 280)       fallback = 140;   // battleship-ish: slow
        else if (r >= 150)  fallback = 190;   // BC
        else if (r >= 60)   fallback = 230;   // cruiser
        if (m_maxSpeed == 0)  m_maxSpeed = fallback;
        if (m_orbitSpeed == 0) m_orbitSpeed = fallback;
    }
    //AttrEntityChaseMaxDelay  - time before 'chase speed' kicks in
    //AttrEntityChaseMaxDelayChance  - chance npc will wait AttrEntityChaseMaxDelay before chasing
    //AttrEntityChaseMaxDuration  - max time a chase will last (unless weapons fired)
    //AttrEntityChaseMaxDurationChance  - chance that any chase will last for AttrEntityChaseMaxDuration

    // ship distances
    //AttrEntityMaxWanderRange
    // Optimal Range  - TODO: test for 0
    m_optimalRange = m_self->GetAttribute(AttrMaxRange).get_uint32();  // distance which npc starts using weapons
    // Accuracy falloff  (distance past optimal range at which accuracy has fallen by half) - TODO: test for 0
    m_falloff = m_self->GetAttribute(AttrFalloff).get_uint32();
    m_trackingSpeed = m_self->GetAttribute(AttrTrackingSpeed).get_double();  //rad/sec
    // Orbit Range, Follow Range  - npc tries to stay at this distance from active target
    m_flyRange = m_self->GetAttribute(AttrEntityFlyRange).get_uint32();    //AttrOrbitRange is 0 for npc
    if (!m_flyRange)
        m_flyRange = m_self->GetAttribute(AttrRadius).get_uint32() * 5;
    // distance for Speed Boost activation  (this needs to be revisited)
    m_boostRange = m_self->GetAttribute(AttrEntityChaseMaxDistance).get_uint32();
    if (!m_boostRange)
        m_boostRange = 0;
    // some npcs have flyRange > boostRange.  this corrects it. (extends boost range)
    if (m_flyRange > m_boostRange)
        m_boostRange += m_boostRange + m_flyRange;

    // Ensure the NPC orbits at a sensible distance. Many NPCs have a tiny/zero
    // AttrMaxRange (e.g. 223m on Plunderers, 0 on some Guristas) which made them
    // "clinch" the player at point-blank instead of fighting at weapon range.
    if (m_optimalRange < 1000)
        m_optimalRange = 5000;
    m_orbitRange = m_optimalRange;   // default orbit at weapon optimal; bots may override

    // Max achievable weapon range = what a PERFECT player with skills+modules can
    // reach. NPCs can never deal damage past this. Do NOT use entityAttackRange
    // (Eradicator: 245km) — it gates fire from across the grid.
    float weaponRange = 0;
    if (m_missileTypeID > 0) {
        // missiles fly until flight time runs out: range = velocity * flightTime
        float missileSpeed = 3750.0f, missileFlightTime = 12.0f;
        std::vector<DmgTypeAttribute> attrs;
        sDataMgr.GetDgmTypeAttrVec(m_missileTypeID, attrs);
        for (auto& a : attrs) {
            if (a.attributeID == AttrMaxVelocity)
                missileSpeed = a.value.get_float();
            else if (a.attributeID == AttrExplosionDelay)
                missileFlightTime = a.value.get_float() / 1000.0f;
        }
        weaponRange = missileSpeed * missileFlightTime;
    } else if (m_optimalRange > 0) {
        weaponRange = (float)m_optimalRange + (float)m_falloff * 2.0f;
    }
    if (weaponRange < 10000)
        weaponRange = 10000;
    m_maxAttackRange = weaponRange;

    // 'sight' range = how far the NPC notices/targets the player. A perfect player's
    // targeting range grows with ship class + skills; NPCs must never target farther
    // than that. Class tiers based on hull radius (frigate<cruiser<bs<capital).
    float radius = m_self->GetAttribute(AttrRadius).get_float();
    // Real player hulls (chelobots fly actual ships like Megathron) carry
    // AttrMaxTargetRange (Megathron 72500m, Raven 75000m). They must target like
    // a real pilot: base range boosted by Long Range Targeting (5%/level, V =
    // x1.25) — NOT the inflated radius-based sight range above.
    if (m_self->HasAttribute(AttrMaxTargetRange)
    && m_self->GetAttribute(AttrMaxTargetRange).get_float() > 0) {
        float baseTargetRange = m_self->GetAttribute(AttrMaxTargetRange).get_float();
        m_sightRange = (uint32)(baseTargetRange * 1.25f);   // Long Range Targeting V
    } else if (radius < 30) {
        m_sightRange = 50000;       // frigate
    } else if (radius < 60) {
        m_sightRange = 75000;       // destroyer/cruiser
    } else if (radius < 150) {
        m_sightRange = 100000;      // battlecruiser
    } else if (radius < 280) {
        m_sightRange = 125000;      // battleship
    } else if (radius < 550) {
        m_sightRange = 150000;      // large battleship / carrier
    } else {
        m_sightRange = 200000;      // capital
    }

    // ship targets
    m_maxAttackTargets = m_self->GetAttribute(AttrMaxAttackTargets).get_uint32();
    if (m_maxAttackTargets < 1)
        m_maxAttackTargets = 1;
    m_maxLockedTargets = m_self->GetAttribute(AttrMaxLockedTargets).get_uint32();
    if (m_maxLockedTargets < 1) {
        if (m_maxAttackTargets > 1) {
            m_maxLockedTargets = m_maxAttackTargets;
        } else {
            m_maxLockedTargets = 1;
        }
    }

    /** @todo change these next 2 (rep and boost) to boolean to avoid timer creation/checks */

    // this is chance an npc has of delaying it's rep (if applicable)
    if (m_self->HasAttribute(AttrEntityArmorRepairDelayChance)) {
        m_armorRepairDelayChance = m_self->GetAttribute(AttrEntityArmorRepairDelayChance).get_float();
    } else if (m_self->HasAttribute(AttrEntityArmorRepairDelayChanceSmall)) {
        m_armorRepairDelayChance = m_self->GetAttribute(AttrEntityArmorRepairDelayChanceSmall).get_float();
    } else if (m_self->HasAttribute(AttrEntityArmorRepairDelayChanceMedium)) {
        m_armorRepairDelayChance = m_self->GetAttribute(AttrEntityArmorRepairDelayChanceMedium).get_float();
    } else if (m_self->HasAttribute(AttrEntityArmorRepairDelayChanceLarge)) {
        m_armorRepairDelayChance = m_self->GetAttribute(AttrEntityArmorRepairDelayChanceLarge).get_float();
    } else {
        m_armorRepairDuration = 0;
        m_armorRepairDelayChance = 0;
    }
    if (m_armorRepairDelayChance)
        m_armorRepairDuration = m_self->GetAttribute(AttrEntityArmorRepairDuration).get_uint32();

    // this is chance an npc has of delaying it's sebo (if applicable)
    if (m_self->HasAttribute(AttrEntityShieldBoostDelayChance)) {
        m_shieldBoosterDelayChance = m_self->GetAttribute(AttrEntityShieldBoostDelayChance).get_float();
    } else if (m_self->HasAttribute(AttrEntityShieldBoostDelayChanceSmall)) {
        m_shieldBoosterDelayChance = m_self->GetAttribute(AttrEntityShieldBoostDelayChanceSmall).get_float();
    } else if (m_self->HasAttribute(AttrEntityShieldBoostDelayChanceMedium)) {
        m_shieldBoosterDelayChance = m_self->GetAttribute(AttrEntityShieldBoostDelayChanceMedium).get_float();
    } else if (m_self->HasAttribute(AttrEntityShieldBoostDelayChanceLarge)) {
        m_shieldBoosterDelayChance = m_self->GetAttribute(AttrEntityShieldBoostDelayChanceLarge).get_float();
    } else {
        m_shieldBoosterDuration = 0;
        m_shieldBoosterDelayChance = 0;
    }
    if (m_shieldBoosterDelayChance)
        m_shieldBoosterDuration = m_self->GetAttribute(AttrEntityShieldBoostDuration).get_uint32();

    // advanced AI variables  only used by sleepers for now (and on live).  will update advanced npcs to use these also
    if (m_self->HasAttribute(AttrAI_ShouldUseTargetSwitching)) {
        m_useTargSwitching = true;
    } else {
        m_useTargSwitching = false;
    }
    if (m_self->HasAttribute(AttrAI_ShouldUseSecondaryTarget)) {
        m_useSecondTarget = true;
    } else {
        m_useSecondTarget = false;
    }
    if (m_self->HasAttribute(AttrAI_ShouldUseSignatureRadius)) {
        m_useSigRadius = true;
        m_preferedSigRadius = m_self->GetAttribute(AttrAI_PreferredSignatureRadius).get_uint32();
    } else {
        m_useSigRadius = false;
        m_preferedSigRadius = 0;
    }
    if (m_self->HasAttribute(AttrAI_ChanceToNotTargetSwitch)) {
        m_switchTargChance = 1.0 - m_self->GetAttribute(AttrAI_ChanceToNotTargetSwitch).get_float();
    } else {
        m_switchTargChance = 0;
    }

    if (m_self->HasAttribute(AttrWarpScrambleRange)) {
        m_warpScramRange = m_self->GetAttribute(AttrWarpScrambleRange).get_float();
    } else {
        m_warpScramRange = 0;
    }
    if (m_self->HasAttribute(AttrEntityWarpScrambleChance)) {
        m_warpScramChance = 1.0 - m_self->GetAttribute(AttrEntityWarpScrambleChance).get_float();
    } else {
        m_warpScramChance = 0;
    }
    if (m_self->HasAttribute(AttrWarpScrambleStrength)) {
        m_warpScramStrength = m_self->GetAttribute(AttrWarpScrambleStrength).get_float();
    } else {
        m_warpScramStrength = 0;
    }

    // EWAR — stasis webifier
    if (m_self->HasAttribute(AttrModifyTargetSpeedRange))
        m_webRange = m_self->GetAttribute(AttrModifyTargetSpeedRange).get_uint32();
    else
        m_webRange = 0;
    // Strength: use the turret's real AttrSpeedFactor (Stasis Towers: -75 => -75%)
    // when present, else default -60%. Chance: AttrModifyTargetSpeedChance (towers
    // have 100 = always), else default 30%.
    if (m_self->HasAttribute(AttrSpeedFactor) && m_self->GetAttribute(AttrSpeedFactor).get_float() < 0.0f)
        m_webStrength = -m_self->GetAttribute(AttrSpeedFactor).get_float() / 100.0f;   // -75 -> 0.75
    else
        m_webStrength = 0.6f; // default -60% speed
    if (m_self->HasAttribute(AttrModifyTargetSpeedChance)) {
        // Convention varies: normal NPCs store a 0..1 float chance to NOT use the
        // web (Police=1 -> always web); sentry turrets store a percentage
        // (Stasis Tower=100 -> always, Sentinel=50 -> half). Normalize both.
        float c = m_self->GetAttribute(AttrModifyTargetSpeedChance).get_float();
        if (c > 1.0f) c /= 100.0f;     // percent form (turrets)
        m_webChance = 1.0f - c;         // chance to NOT apply, matching MakeRandomFloat()>m_webChance
    }
    else
        m_webChance = 0.3f;  // default 30% chance per cycle
    m_webApplied = false;   // symmetric web undo tracking
    m_webTargetID = 0;

    // EWAR — ECM (target jamming)
    if (m_self->HasAttribute(AttrEntityTargetJamMaxRange))
        m_ecmRange = m_self->GetAttribute(AttrEntityTargetJamMaxRange).get_uint32();
    else
        m_ecmRange = 0;
    if (m_self->HasAttribute(AttrEntityTargetJam))
        m_ecmStrength = m_self->GetAttribute(AttrEntityTargetJam).get_float();
    else
        m_ecmStrength = 0;
    if (m_self->HasAttribute(AttrEntityTargetJamDurationChance))
        m_ecmChance = 1.0f - m_self->GetAttribute(AttrEntityTargetJamDurationChance).get_float();
    else
        m_ecmChance = 1.0f;
    if (m_self->HasAttribute(AttrEntityTargetJamDuration))
        m_ecmDuration = m_self->GetAttribute(AttrEntityTargetJamDuration).get_uint32();
    else
        m_ecmDuration = m_attackSpeed;

    // Smartbomb/AoE range
    if (m_self->HasAttribute(AttrEmpFieldRange))
        m_smartbombRange = m_self->GetAttribute(AttrEmpFieldRange).get_uint32();
    else
        m_smartbombRange = 0;

    // EWAR — target painting
    if (m_self->HasAttribute(AttrEntityTargetPaintMaxRange))
        m_paintRange = m_self->GetAttribute(AttrEntityTargetPaintMaxRange).get_uint32();
    else
        m_paintRange = 0;
    if (m_self->HasAttribute(AttrEntityTargetPaintMultiplier))
        m_paintMultiplier = m_self->GetAttribute(AttrEntityTargetPaintMultiplier).get_float();
    else
        m_paintMultiplier = 0;
    if (m_self->HasAttribute(AttrEntityTargetPaintDurationChance))
        m_paintChance = 1.0f - m_self->GetAttribute(AttrEntityTargetPaintDurationChance).get_float();
    else
        m_paintChance = 1.0f;
    if (m_self->HasAttribute(AttrEntityTargetPaintDuration))
        m_paintDuration = m_self->GetAttribute(AttrEntityTargetPaintDuration).get_uint32();
    else
        m_paintDuration = m_attackSpeed;

    /*
    AttrEntityTargetJam = 928,
    AttrEntityTargetJamDuration = 929,
    AttrEntityTargetJamDurationChance = 930,    // npcActivationChanceAttributeID in dgmEffects
    AttrEntityCapacitorDrainDurationChance = 931,   // npcActivationChanceAttributeID in dgmEffects
    AttrEntitySensorDampenDurationChance = 932,   // npcActivationChanceAttributeID in dgmEffects
    AttrEntityTrackingDisruptDurationChance = 933,   // npcActivationChanceAttributeID in dgmEffects
    AttrEntityTargetPaintDurationChance = 935,   // npcActivationChanceAttributeID in dgmEffects
    AttrEntityTargetJamMaxRange = 936,
    AttrEntityCapacitorDrainMaxRange = 937,
    AttrEntitySensorDampenMaxRange = 938,
    AttrEntityTrackingDisruptMaxRange = 940,
    AttrEntityTargetPaintMaxRange = 941,
    AttrEntityCapacitorDrainDuration = 942,
    AttrEntitySensorDampenDuration = 943,
    AttrEntityTrackingDisruptDuration = 944,
    AttrEntityTargetPaintDuration = 945,
    AttrEntityCapacitorDrainAmount = 946,
    AttrEntitySensorDampenMultiplier = 947,
    AttrEntityTrackingDisruptMultiplier = 948,
    AttrEntityTargetPaintMultiplier = 949,
    AttrEntitySensorDampenFallOff = 950,
    AttrEntityTrackingDisruptFallOff = 951,
    AttrEntityCapacitorFallOff = 952,
    AttrEntityTargetJamFallOff = 953,
    AttrEntityTargetPaintFallOff = 954,
    */

    // does this need to be running if there are no players in bubble?
    //  yes...npcs will warp out when no targets in sight range, but need a process tic to do that.
   // m_processTimer.Start(m_attackSpeed);

    // maybe this can be used to tell spawnMgr to respawn this npc as required....
    //    AttrEntityGroupRespawnChance = 640,
}

void NPCAIMgr::Process() {
    if (m_destiny->IsWarping())
        return;

    // Convoy NPCs only fight in self-defense
    if (m_npc->IsConvoy() && !m_npc->IsConvoyUnderAttack())
        return;

    if (m_warpOutTimer.Check(false)) {
        // disallow warpout if spawn has active respawn timer (spawn is being chained)
        // PlayerBot-owned NPCAIs have no SpawnMgr (m_spawnMgr == nullptr) — guard it.
        if (m_npc->GetSpawnMgr() != nullptr
            && m_npc->GetSpawnMgr()->IsChaining(m_npc->SysBubble()->GetID())) {
            m_state = NPCAI::State::Idle;
            m_warpOutTimer.Disable();
        }
    }

    /* NPCAI::State definitions   -allan 25July15  (UD 1June16)
     *   Idle,       // not doing anything, nothing in sight....idle.  call Wander() to loosely orbit random object in bubble ~10-20k at 1/2 orbit speed
     *   Chasing,    // target within npc sight range.  attacking begins here.  use m_maxSpeed to get within falloff
     *   Following,  // between optimal and falloff.  try to get closer, but still orbiting and attacking
     *   Engaged,    // actively fighting (in orbit).  use m_orbitSpeed.
     *   Fleeing,    // running away....use m_maxSpeed then warp away when out of range	(does this make sense??)
     *   Signaling   // calling for help..use m_orbitSpeed *2 to speed tank while calling for reinforcements
     */
    switch(m_state) {
        case NPCAI::State::Idle: {
            // Customs officials don't auto-target players — only respond when triggered by contraband
            if (m_self->groupID() == EVEDB::invGroups::Customs_Official)
                return;
            // Anomaly ambush: NPCs hold position in cover (no wander, no wide-area
            // scan) until they spot a player — then they fly out of hiding and
            // converge. This replaces the old "pile of rats scattering at warp-in".
            //
            // Two-phase response, depending on how far away the player landed:
            //   <= 30km  -> classic ambush: burst out of cover and engage right away
            //   <= sight -> spotted from afar: leave cover and move to intercept
            //              (ship approaches, orbit/attack begins when in range)
            //   beyond sight -> stay hidden until the player gets closer
            if (m_isAmbush) {
                // Still lying in cover — staggered wake-up so the rats spring as a wave.
                if (m_ambushWakeTimer.Enabled() && !m_ambushWakeTimer.Check(false))
                    return;
                // Nobody came close for a while — leave cover and patrol the site
                // so it doesn't sit as a pile of static crosses.
                if (m_ambushTimeout.Enabled() && m_ambushTimeout.Check(false)) {
                    _log(NPC__AI_TRACE, "%s(%u): Ambush timed out — leaving cover to patrol.",
                         m_npc->GetName(), m_npc->GetID());
                    m_isAmbush = false;
                    SetIdle();
                    return;
                }
                uint32 ambushRange = std::min(m_sightRange, (uint32)30000);
                std::vector<Client*> clientVec;
                m_npc->SysBubble()->GetPlayers(clientVec);
                Client* pClose = nullptr;
                Client* pFar = nullptr;
                double closeDist = 0.0, farDist = 0.0;
                for (auto cur : clientVec) {
                    if (cur->IsInvul())
                        continue;
                    if (cur->GetShipSE() == nullptr)
                        continue;
                    if (cur->InPod())
                        continue;
                    DestinyManager* pDestiny = cur->GetShipSE()->DestinyMgr();
                    if (pDestiny == nullptr || pDestiny->IsCloaked() || pDestiny->IsWarping())
                        continue;
                    double d = m_npc->GetPosition().distance(cur->GetShipSE()->GetPosition());
                    if (d > m_sightRange)
                        continue;   // beyond sight — ignore for now
                    // Faction patrols only aggro players with negative standing
                    if (m_npc->GetWarFactionID() > 0) {
                        if (StandingDB::GetStanding(m_npc->GetWarFactionID(), cur->GetCharacterID()) >= 0.0f)
                            continue;
                    }
                    if (d <= ambushRange) {
                        // pick the NEAREST ambush-range player
                        if (pClose == nullptr || d < closeDist) { pClose = cur; closeDist = d; }
                    } else {
                        // pick the NEAREST far player (within sight, outside ambush)
                        if (pFar == nullptr || d < farDist) { pFar = cur; farDist = d; }
                    }
                }
                if (pClose != nullptr) {
                    _log(NPC__AI_TRACE, "%s(%u): AMBUSH sprung — %s(%u) at %.0fm, bursting out of cover.",
                         m_npc->GetName(), m_npc->GetID(), pClose->GetName(), pClose->GetCharacterID(), closeDist);
                    m_isAmbush = false;
                    Target(pClose->GetShipSE());
                    return;
                }
                if (pFar != nullptr) {
                    _log(NPC__AI_TRACE, "%s(%u): Spotted %s(%u) at %.0fm — leaving cover to intercept.",
                         m_npc->GetName(), m_npc->GetID(), pFar->GetName(), pFar->GetCharacterID(), farDist);
                    m_isAmbush = false;
                    // Target() handles both cases: too far -> SetChasing (intercept),
                    // in range -> CheckDistance/SetEngaged. Attack timers arm there.
                    Target(pFar->GetShipSE());
                    return;
                }
                return;   // nobody in sight yet — stay hidden
            }
            if (m_beginFindTarget.Check()) {
                std::vector<Client*> clientVec;
                clientVec.clear();
                DestinyManager* pDestiny(nullptr);
                m_npc->SysBubble()->GetPlayers(clientVec); // what about player drones?  yes...later
                for (auto cur : clientVec) {
                    if (cur->IsInvul())
                        continue;
                    if (cur->GetShipSE() == nullptr)
                        continue;
                    if (cur->InPod()) {
                        if (sConfig.npc.TargetPod) {
                            if (m_npc->SystemMgr()->GetSystemSecurityRating() > sConfig.npc.TargetPodSec)
                                continue;
                        } else {
                            continue;
                        }
                    }
                    pDestiny = cur->GetShipSE()->DestinyMgr();
                    if (pDestiny == nullptr)   // this shouldnt be needed, but whatever...
                        continue;
                    if (pDestiny->IsCloaked() or pDestiny->IsWarping())
                        continue;
                    if (m_npc->GetPosition().distance(cur->GetShipSE()->GetPosition()) > m_sightRange)
                        continue;

                    // Faction patrols only aggro players with negative standing
                    if (m_npc->GetWarFactionID() > 0) {
                        if (StandingDB::GetStanding(m_npc->GetWarFactionID(), cur->GetCharacterID()) >= 0.0f)
                            continue;
                    }

                    // Simulated-player NPCs (PlayerBot): they MAY attack the player,
                    // but the risk must feel like EVE — safe to roam/mine in highsec,
                    // danger in low/null, and never a guaranteed jump. The generic
                    // Idle scan used to make EVERY bot in the bubble attack the player
                    // at once ("a horde of chelobots warps in to kill me"). Gate it:
                    // only hunters prowl for players, only in low/null (or highsec for
                    // criminals), and only with a chance + a long cooldown.
                    if (m_npc->IsPlayerBot()) {
                        // Only the bot's hunter profession prowls for players;
                        // peaceful professions defend only (PlayerBot::OnAttacked).
                        uint8 prof = 255;
                        PlayerBot* pbot = dynamic_cast<PlayerBot*>(m_npc);
                        if (pbot != nullptr)
                            prof = (uint8)pbot->GetProfession();
                        if (prof != (uint8)PlayerBot::BotProfession::Hunter) {
                            continue;   // peaceful bot — leave the player alone
                        }
                        float botSec = m_npc->SystemMgr()->GetSystemSecurityRating();
                        bool playerCriminal = false;
                        bool playerLowSec = false;
                        if (cur->GetCrimeWatch() != nullptr)
                            playerCriminal = cur->GetCrimeWatch()->IsCriminal();
                        playerLowSec = cur->GetSecurityRating() < -5.0f;
                        // Highsec: CONCORD protects — a hunter only engages a legal
                        // target (criminal / very low security status).
                        if (botSec >= 0.5f && !playerCriminal && !playerLowSec) {
                            m_beginFindTarget.Start(MakeRandomInt(20000, 40000));
                            continue;
                        }
                        // A hunter doesn't engage every time it spots someone:
                        // ~15% chance per scan in low/null + long cooldown.
                        if (MakeRandomInt(0, 99) >= 15) {
                            m_beginFindTarget.Start(MakeRandomInt(15000, 30000));
                            continue;
                        }
                        // Hunters only commit when they think they can win —
                        // reuse the same strength check PlayerBot uses.
                        if (!pbot->HunterWouldEngage(cur->GetShipSE())) {
                            m_beginFindTarget.Start(MakeRandomInt(20000, 40000));
                            continue;
                        }
                        pbot->StartAggressionTimer();
                        pbot->BroadcastAggression(cur->GetCharacterID());
                        // Advanced hunter with allies: try a warp-bubble ambush on
                        // the player before engaging (trap them so they can't warp).
                        pbot->TryAmbush(cur->GetShipSE());
                    }

                    Target(cur->GetShipSE());
                    return;
                }
                // Target player drones in the bubble
                std::map<uint32, SystemEntity*> entities;
                m_npc->SysBubble()->GetAllEntities(entities);
                for (auto& [id, pEnt] : entities) {
                    if (pEnt == nullptr || !pEnt->IsDroneSE())
                        continue;
                    if (pEnt->DestinyMgr() == nullptr || pEnt->DestinyMgr()->IsWarping())
                        continue;
                    if (m_npc->GetPosition().distance(pEnt->GetPosition()) > m_sightRange)
                        continue;
                    // Attacking a player's drone/fighter is PvP — a charbot gets
                    // flagged like any attacker would (drone hits transfer to the
                    // pilot who owns it).
                    if (m_npc->IsPlayerBot() && pEnt->GetDroneSE() != nullptr
                        && pEnt->GetDroneSE()->GetOwner() != nullptr) {
                        PlayerBot* pbot = dynamic_cast<PlayerBot*>(m_npc);
                        if (pbot != nullptr) {
                            Client* owner = pEnt->GetDroneSE()->GetOwner();
                            pbot->StartAggressionTimer();
                            pbot->BroadcastAggression(owner->GetCharacterID());
                        }
                    }
                    // A chelobot engages a player's fighter/drone only if it would
                    // also engage the OWNING ship. Drones aren't free targets: a
                    // Nyx's fighter-bombers belong to a supercarrier, and a pilot
                    // who commits to shooting them commits to that fight. Same
                    // analytic judgement as engaging the hull directly — a frigate
                    // hunter doesn't suicide into a carrier's fighter screen.
                    if (m_npc->IsPlayerBot()) {
                        Client* owner = sEntityList.FindClientByCharID(pEnt->GetSelf()->ownerID());
                        SystemEntity* ownerSE = (owner != nullptr) ? owner->GetShipSE() : nullptr;
                        if (ownerSE != nullptr && ownerSE != m_npc) {
                            PlayerBot* pbot = dynamic_cast<PlayerBot*>(m_npc);
                            if (pbot != nullptr && !pbot->HunterWouldEngage(ownerSE)) {
                                // Not worth picking a fight with that pilot's drones.
                                m_beginFindTarget.Start(MakeRandomInt(15000, 30000));
                                return;
                            }
                        }
                    }
                    Target(pEnt);
                    return;
                }
                if (sConfig.npc.IdleWander)
                    if (!m_isWandering)
                        SetWander();
            } else {
                if (!m_beginFindTarget.Enabled())
                    m_beginFindTarget.Start(m_attackSpeed);  //find target is based on npc attack speed.
            }
        } break;
        case NPCAI::State::Chasing:
        case NPCAI::State::Following:
        case NPCAI::State::Engaged: {
            if (m_npc->TargetMgr()->HasNoTargets()) {
                _log(NPC__AI_TRACE, "%s(%u): Stopped %s - HasNoTargets = true.", m_npc->GetName(), m_npc->GetID(), GetStateName(m_state).c_str());
                SetIdle();
                return;
            }
            SystemEntity* pSE = m_npc->TargetMgr()->GetFirstTarget(false);
            if (pSE == nullptr) {
                _log(NPC__AI_TRACE, "%s(%u): Stopped %s - GetFirstTarget() returned NULL.", m_npc->GetName(), m_npc->GetID(), GetStateName(m_state).c_str());
                SetIdle();
                return;
            }
            if (pSE->SysBubble() == nullptr) {
                ClearTarget(pSE);
                return;
            }
            // Don't chase a target that has left our bubble — an anomaly rat
            // (or its drone) shouldn't warp/fly across the system after a player
            // or charbot that warped away. The fight stays in one bubble.
            if (m_npc->SysBubble() != nullptr and pSE->SysBubble() != m_npc->SysBubble()) {
                _log(NPC__AI_TRACE, "%s(%u): target %s(%u) left our bubble (%u != %u) — dropping chase.",
                     m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID(),
                     m_npc->SysBubble()->GetID(), pSE->SysBubble()->GetID());
                ClearTarget(pSE);
                return;
            }
            CheckDistance(pSE);
            if (m_missileTimer.Check())
                LaunchMissile(m_missileTypeID, pSE);
        } break;
        case NPCAI::State::Fleeing:{
            // Fly away at max speed, then warp out
            if (!m_destiny->IsMoving() and !m_destiny->IsWarping()) {
                SystemEntity* target = m_npc->TargetMgr()->GetFirstTarget(false);
                if (target != nullptr) {
                    // Move away from target
                    GVector fleeDir = m_npc->GetPosition() - target->GetPosition();
                    fleeDir.normalize();
                    GPoint away = m_npc->GetPosition() + fleeDir * 50000;
                    m_destiny->SetMaxVelocity(m_maxSpeed);
                    m_destiny->GotoPoint(away);
                }
            }
            // After reaching distance, warp out
            SystemEntity* fleeTarget = m_npc->TargetMgr()->GetFirstTarget(false);
            if (fleeTarget == nullptr or m_npc->GetPosition().distance(fleeTarget->GetPosition()) > m_sightRange) {
                WarpOut();
            }
        } break;
        case NPCAI::State::Signaling:{
            // Signaling: stay at range and orbit, calling for reinforcements
            if (!m_destiny->IsOrbiting()) {
                SystemEntity* target = m_npc->TargetMgr()->GetFirstTarget(false);
                if (target != nullptr) {
                    m_destiny->SetMaxVelocity(m_orbitSpeed * 2);
                    m_destiny->Orbit(target, m_falloff);
                }
            }
            // Call for reinforcements from SpawnMgr (one-shot)
            SpawnMgr* spawnMgr = m_npc->GetSpawnMgr();
            SystemBubble* bubble = m_npc->SysBubble();
            if (spawnMgr != nullptr and bubble != nullptr and !bubble->IsAnomaly()) {
                spawnMgr->DoSpawnForBubble(bubble);
                _log(NPC__AI_TRACE, "%s(%u): Signaling — called reinforcements in bubble %u",
                     m_npc->GetName(), m_npc->GetID(), bubble->GetID());
                // Transition to Engaged after calling reinforcements
                m_state = NPCAI::State::Engaged;
            }
        } break;
        case NPCAI::State::WarpOut:
        case NPCAI::State::WarpFollow:{
            _log(NPC__AI_TRACE, "%s(%u): Called %s - needs to be completed.", m_npc->GetName(), m_npc->GetID(), GetStateName(m_state).c_str());
            m_state = NPCAI::State::Idle;
        } break;
    }

    if (m_shieldBoosterTimer.Enabled())
        if (m_shieldBoosterTimer.Check())
            m_npc->UseShieldRecharge();

    if (m_armorRepairTimer.Enabled())
        if (m_armorRepairTimer.Check())
            m_npc->UseArmorRepairer();
}

bool NPCAIMgr::IsFighting() {
    // more to this here....
    return (m_state != NPCAI::State::Idle);
}

void NPCAIMgr::WakeUp() {
    // Force target find timer to fire immediately
    m_beginFindTarget.Start(1);
    m_state = NPCAI::State::Idle;
}

void NPCAIMgr::SetAmbush(bool ambush)
{
    m_isAmbush = ambush;
    if (ambush) {
        // Stationary ambush: stand in cover, no wandering until the player is
        // close enough to spot. m_beginFindTarget stays disabled so Idle doesn't
        // do its normal wide-area player scan.
        m_isWandering = false;
        m_destiny->Stop();
        // Stagger the rats by ROLE for drama — the ambush unfolds in waves:
        //   brawlers (short-range, scram/web) burst out of cover first (0.5-3s)
        //   ranged/missile rats hold position longer, then come in (3-8s)
        //   big ships / officers reveal LAST (adds 5-12s) — they enter when the
        //   first wave has already drawn the player's fire.
        uint32 wakeDelay;
        if (m_missileTypeID > 0 || m_optimalRange > 25000) {
            wakeDelay = MakeRandomInt(3000, 8000);
        } else {
            wakeDelay = MakeRandomInt(500, 3000);
        }
        if (m_self->HasAttribute(AttrRadius)) {
            float r = m_self->GetAttribute(AttrRadius).get_float();
            if (r >= 550)   // battleship/capital — officer/commander presence
                wakeDelay += MakeRandomInt(5000, 12000);
        }
        m_ambushWakeTimer.Start(wakeDelay);
        // If a player is in the system but never comes within sight range, the
        // rat eventually gives up hiding and patrols (keeps the site alive).
        m_ambushTimeout.Start(60000);
    } else {
        m_ambushWakeTimer.Disable();
        m_ambushTimeout.Disable();
    }
}

void NPCAIMgr::StartAttackCycle(uint32 intervalMs) {
    if (intervalMs == 0)
        intervalMs = m_attackSpeed;
    if (!m_mainAttackTimer.Enabled())
        m_mainAttackTimer.Start(intervalMs);
}

void NPCAIMgr::WarpOut()
{
    m_warpOutTimer.Disable();

    if (m_state == NPCAI::State::WarpOut) {
        m_state = NPCAI::State::Idle;
        return;
    }

    // Check warp scramble — if scrambled, NPC cannot warp out
    if (m_self->HasAttribute(AttrWarpScrambleStatus)
        and m_self->GetAttribute(AttrWarpScrambleStatus) > 0)
        return;

    m_state = NPCAI::State::WarpOut;
    SystemManager* pSys = m_npc->SystemMgr();

    /** @todo  eventually, this will check with anomaly mgr for possible npc hideouts in system
     * based on npc faction, system, players in system, players in bubble, and *more later*
     * to determine a warpto target for this npc, or this group
     *
     * for now, if there are players in system, just warp to another belt.
     * if there are no players in this system, avoid using proc tics on npcs
     */

    if (pSys->PlayerCount()) {
        // pSys->GetAnomMgr();
        uint32 newBeltID = pSys->GetRandBeltID();
        if (newBeltID == sBubbleMgr.GetBeltID(m_npc->SysBubble()->GetID()))
            newBeltID = pSys->GetRandBeltID();

        SystemEntity* newSE = pSys->GetSE(newBeltID);
        if (newSE == nullptr) {
            m_state = NPCAI::State::Idle;
            return;
        }
        m_destiny->WarpTo(newSE->GetPosition());
        // PlayerBot-owned NPCAIs have no SpawnMgr — MoveSpawn would be a nullptr deref.
        if (m_npc->GetSpawnMgr() != nullptr)
            m_npc->GetSpawnMgr()->MoveSpawn(m_npc, sBubbleMgr.FindBubble(newSE));
    }
}

void NPCAIMgr::SetWander()
{
    if (m_npc->GetSpawnMgr() == nullptr)
        return;
    if (m_isStationary)
        return;   // sentry turrets hold position, never wander
    if (!m_isWandering) {
        _log(NPC__AI_TRACE, "%s(%u): Wandering:  No Targets within my sight range of %um", \
                m_npc->GetName(), m_npc->GetID(), m_sightRange);
        m_isWandering = true;
    }

    SystemBubble* pBubble = m_npc->SysBubble();

    // wandering.  nothing to shoot.  look for target.
    if (pBubble == nullptr or pBubble->IsAnomaly() or pBubble->IsIncursion() or pBubble->IsMission()) {
        return;
    } else if (pBubble->HasDynamics() and pBubble->IsBelt()) {
        // pick random entity and loosely orbit it.  if no entity found, orbit center of belt
        SystemEntity* pSE = pBubble->GetRandomEntity();
        if (pSE == nullptr)
            pSE = m_npc->SystemMgr()->GetSE(sBubbleMgr.GetBeltID(pBubble->GetID()));
        if (pSE == nullptr) {
            _log(NPC__ERROR, "%s(%u): Wandering:  No Target or beltSE found.", m_npc->GetName(), m_npc->GetID());
            // nothing here...leave bubble
            WarpOut();
            return;
        }
        m_destiny->SetMaxVelocity(m_orbitSpeed);
        uint16 orbitDistance = MakeRandomInt(10000, 20000);
        m_destiny->Orbit(pSE, orbitDistance);
        _log(NPC__AI_TRACE, "%s(%u):  Just for shits-n-giggles, I\'m gonna orbit %s(%u) at %um.", \
                m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID(), orbitDistance);
    } else {
        /** @todo  figure out a way for npc to wander 'aimlessly' around their bubble */
        m_destiny->Stop();
    }
}

void NPCAIMgr::SetIdle() {
    if (m_state == NPCAI::State::Idle)
        return;
    // not doing anything....idle.

    /** @todo need to clear out targets here */

    _log(NPC__AI_TRACE, "%s(%u): Idle: returning to idle.", m_npc->GetName(), m_npc->GetID());
    m_state = NPCAI::State::Idle;
    m_destiny->Stop();
    m_destiny->SetMaxVelocity(m_orbitSpeed);

    m_missileTimer.Disable();
    m_webifierTimer.Disable();
    m_beginFindTarget.Disable();
    m_mainAttackTimer.Disable();
    m_armorRepairTimer.Disable();
    m_warpScramblerTimer.Disable();
    m_shieldBoosterTimer.Disable();
    m_ecmTimer.Disable();
    m_paintTimer.Disable();
    m_smartbombTimer.Disable();

    SystemBubble* pBubble = m_npc->SysBubble();
    //disallow warpout if anomaly, incursion or mission rat
    if (pBubble != nullptr and (pBubble->IsAnomaly() or pBubble->IsIncursion() or pBubble->IsMission()))
        return;

    //disallow warpout by NOT setting timer.
    if (sConfig.npc.WarpOut > 0)
        if (m_npc->GetSpawnMgr() != nullptr)
            m_warpOutTimer.Start(sConfig.npc.WarpOut *1000); // s to ms
}

void NPCAIMgr::SetChasing(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    /** @todo implement chase timer using entityChaseMaxDuration to limit chase time. */
    if (m_state == NPCAI::State::Chasing) {
        // Already chasing — re-aim at the target's CURRENT position every tick.
        // The old early-return left GotoPoint aimed at a stale position, so a
        // moving target (player orbiting) was never caught up: the NPC chased a
        // phantom point, distance grew, and shots missed ("far miss").
        m_destiny->SetMaxVelocity(m_maxSpeed);
        m_destiny->GotoPoint(pSE->GetPosition());
        return;
    }
    _log(NPC__AI_TRACE, "%s(%u): Begin chasing.  Target is %s(%u).", \
         m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
    // target out of range to attack/follow, but within npc sight range....use mwd/ab if equiped
    m_destiny->SetMaxVelocity(m_maxSpeed);
    m_destiny->GotoPoint(pSE->GetPosition());  //head towards target
    m_state = NPCAI::State::Chasing;
    m_warpOutTimer.Disable();
}

void NPCAIMgr::SetFollowing(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    if ((m_state == NPCAI::State::Following) and (m_destiny->IsGoto() or m_destiny->IsFollowing()))
        return;
    _log(NPC__AI_TRACE, "%s(%u): Begin following.  Target is %s(%u).", \
         m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
    // too close to chase, but to far to engage
    m_destiny->SetMaxVelocity(m_orbitSpeed *2);
    m_destiny->Follow(pSE, m_falloff);  //try to get inside falloff range
    m_state = NPCAI::State::Following;
    m_warpOutTimer.Disable();
}

void NPCAIMgr::SetEngaged(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    if ((m_state == NPCAI::State::Engaged) and m_destiny->IsOrbiting())
        return;
    _log(NPC__AI_TRACE, "%s(%u): Begin engaging.  Target is %s(%u).", \
         m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
    // actively fighting
    m_destiny->SetMaxVelocity(m_orbitSpeed);
    m_destiny->Orbit(pSE, m_orbitRange > 0 ? m_orbitRange : m_optimalRange);  // orbit at weapon optimal (or bot style override)
    m_state = NPCAI::State::Engaged;
    m_warpOutTimer.Disable();
}

// not used yet
void NPCAIMgr::SetFleeing(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    if ((m_state == NPCAI::State::Fleeing) and m_destiny->IsMoving())
        return;
    _log(NPC__AI_TRACE, "%s(%u): Begin fleeing.  Target is %s(%u).", \
         m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
    // actively fleeing
    //  use superspeed to disengage, then warp.  << both these will need to be written.
    //  this state is only usable by higher-class npcs.
    m_destiny->SetMaxVelocity(m_maxSpeed);
    m_state = NPCAI::State::Fleeing;
    m_warpOutTimer.Disable();
}

void NPCAIMgr::Flee(SystemEntity* pTargSE)
{
    if (pTargSE == nullptr)
        return;
    // Register the threat as a target so the Fleeing state can track it, then
    // hand off to SetFleeing (runs away and warps out when out of range).
    if (m_npc->TargetMgr() != nullptr)
        m_npc->TargetMgr()->TargetedAdd(pTargSE);
    SetFleeing(pTargSE);
}

// not used yet
void NPCAIMgr::SetSignaling(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    if ((m_state == NPCAI::State::Signaling) and m_destiny->IsOrbiting())
        return;
    _log(NPC__AI_TRACE, "%s(%u): Begin signaling.  Target is %s(%u).", \
         m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
    // actively signaling
    //  start speedtanking while signaling.  (im sure this is cheating, but fuckem.)
    //  this state is only usable by higher-class npcs.
    m_destiny->SetMaxVelocity(m_orbitSpeed * 2);
    m_destiny->Orbit(pSE, m_falloff);  //try to get outside orbit range
    m_state = NPCAI::State::Signaling;
    m_warpOutTimer.Disable();
}

void NPCAIMgr::CheckDistance(SystemEntity* pSE)
{
    if (pSE == nullptr)
        return;
    double dist = m_npc->GetPosition().distance(pSE->GetPosition());
    // A chelobot is a real pilot: it loses lock the moment the target leaves its
    // targeting range, exactly like a player. A regular NPC keeps hunting an
    // aggressor that locked it first (IsTargetedBy), but also drops it beyond
    // its (much larger, radius-based) sight range.
    bool beyondRange = dist > m_sightRange;
    bool playerLike = m_npc->IsPlayerBot();
    if (beyondRange and (playerLike or !m_npc->TargetMgr()->IsTargetedBy(pSE))) {
        _log(NPC__AI_TRACE, "%s(%u): CheckDistance: %s(%u) is too far away (%.0fm).  Return to Idle.", \
             m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID(), dist);
        if (m_state != NPCAI::State::Idle) {
            // target is no longer in npc's "sight range" and is NOT targeting this npc.  unlock target and return to idle.
            //   should we do anything else here?  search for another target?  wander around?  yes..later
            // if npc is targeted greater than this distance, it will chase
            ClearTarget(pSE);
        }
        return;
    }
    m_isWandering = false;

    // Stationary sentry turrets (group 99/180/336/383/417) never move — they
    // hold position and only fire. Don't enter Chasing/Following/Engaged
    // (which orbit/follow the target); just attack from the spot.
    if (m_isStationary) {
        if (m_missileTypeID > 0 && m_missileTimer.Check())
            LaunchMissile(m_missileTypeID, pSE);
        Attack(pSE);
        return;
    }

    if (dist < m_flyRange) {
        SetEngaged(pSE);
    } else if (dist < m_boostRange) {
        SetFollowing(pSE);
    } else {
        SetChasing(pSE);
    }

    _log(NPC__AI_TRACE, "%s(%u): CheckDistance:  target: %s(%u), state: %s, dist: %.0fm, flyRange: %u, boostRange: %u.", \
            m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID(), GetStateName(m_state).c_str(), dist, m_flyRange, m_boostRange);

    Attack(pSE);
}

void NPCAIMgr::Target(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    float targetTime = GetTargetTime();
    bool chase = false;

    if (!m_npc->TargetMgr()->StartTargeting(pSE, targetTime, m_maxLockedTargets, m_sightRange, chase)) {
        if (chase) {
            _log(NPC__AI_TRACE, "%s(%u): Targeting of %s(%u) failed.  Begin Chasing.", \
                        m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
            SetChasing(pSE);
        } else {
            _log(NPC__AI_TRACE, "%s(%u): Targeting of %s(%u) failed.  Clear Target and Return to Idle.", \
                        m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
            SetIdle();
        }
        return;
    }
    m_beginFindTarget.Disable();
    CheckDistance(pSE);

    if (!m_mainAttackTimer.Enabled())
        m_mainAttackTimer.Start(m_attackSpeed);

    if (!m_missileTimer.Enabled() and (m_launcherCycleTime > 100))
        m_missileTimer.Start(m_launcherCycleTime);
}

void NPCAIMgr::Targeted(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    double targetTime = GetTargetTime();

    _log(NPC__AI_TRACE, "%s(%u): Targeted by %s(%u) while %s.", \
            m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID(), GetStateName(m_state).c_str());

    // Ambush NPC shot first — spring out of cover immediately
    m_isAmbush = false;

    switch(m_state) {
        case NPCAI::State::Idle: {
            _log(NPC__AI_TRACE, "%s(%u): Begin Approaching and start Targeting sequence.", \
                    m_npc->GetName(), m_npc->GetID());
            SetChasing(pSE);

            bool chase = false;
            if (!m_npc->TargetMgr()->StartTargeting( pSE, targetTime, m_maxLockedTargets, m_sightRange, chase)) {
                if (chase) {
                    _log(NPC__AI_TRACE, "%s(%u): Targeting of %s(%u) failed.  Begin Chasing.", \
                            m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
                    SetChasing(pSE);
                } else {
                    _log(NPC__AI_TRACE, "%s(%u): Targeting of %s(%u) failed.  Clear Target and Return to Idle.", \
                            m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
                    SetIdle();
                }
            }
            m_beginFindTarget.Disable();
            //CheckDistance(pAgressor);
        } break;

        /** @todo  determine if new targetedby entity is weaker than current target. use optimalSigRadius to test for 'optimal' target */
        case NPCAI::State::Chasing: {
        } break;
        case NPCAI::State::Following: {
        } break;
        case NPCAI::State::Engaged: {
        } break;
        case NPCAI::State::Fleeing: {
        } break;
        case NPCAI::State::Signaling: {
        } break;
    }
    if (!m_shieldBoosterTimer.Enabled())
        if (MakeRandomFloat() > m_shieldBoosterDelayChance)
            m_shieldBoosterTimer.Start(m_shieldBoosterDuration);
    if (!m_armorRepairTimer.Enabled())
        if (MakeRandomFloat() > m_armorRepairDelayChance)
            m_armorRepairTimer.Start(m_armorRepairDuration);

    // Start EWAR timers with staggered initial delays
    if (!m_webifierTimer.Enabled() and m_webRange > 0)
        m_webifierTimer.Start(m_attackSpeed * 2);
    if (!m_ecmTimer.Enabled() and m_ecmRange > 0 and m_ecmStrength > 0)
        m_ecmTimer.Start(m_attackSpeed * 3);
    if (!m_paintTimer.Enabled() and m_paintRange > 0 and m_paintMultiplier > 0)
        m_paintTimer.Start(m_attackSpeed * 4);
    if (!m_smartbombTimer.Enabled() and m_smartbombRange > 0)
        m_smartbombTimer.Start(m_attackSpeed * 5);

    // NPCs aggroed via Targeted() (player/drone attacked first) never got their
    // attack/missile timers started — Target() starts them, Targeted() did not.
    // Result: the NPC orbited the player (Engaged) but never fired a shot.
    if (!m_mainAttackTimer.Enabled())
        m_mainAttackTimer.Start(m_attackSpeed);
    if (!m_missileTimer.Enabled() and (m_launcherCycleTime > 100))
        m_missileTimer.Start(m_launcherCycleTime);
}

void NPCAIMgr::TargetLost(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    switch(m_state) {
        case NPCAI::State::Chasing:
        case NPCAI::State::Following:
        case NPCAI::State::Engaged: {
            // implement chance for npc to follow warping player
            // sConfig.npc.WarpFollowChance;
            // NPCAI::State::WarpFollow
            if (m_npc->TargetMgr()->HasNoTargets()) {
                _log(NPC__AI_TRACE, "%s(%u): Target %s(%u) lost. No targets remain.  Return to Idle.", \
                        m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
                SetIdle();
            } else {
                _log(NPC__AI_TRACE, "%s(%u): Target %s(%u) lost, but more targets remain.", \
                        m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
                /** @todo engage weakest target in current list */
                Attack(m_npc->TargetMgr()->GetFirstTarget(true));
            }
        }
    }
}

void NPCAIMgr::Attack(SystemEntity* pSE)
{
    if (pSE == nullptr)
        return;
    if (m_mainAttackTimer.Check()) {
        // Check to see if the target still in the bubble (Client warped out)
        if (m_npc->SysBubble() == nullptr or !m_npc->SysBubble()->InBubble(pSE->GetPosition())) {
            _log(NPC__AI_TRACE, "%s(%u): Target %s(%u) no longer in bubble.  Clear target and move on",
                    m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
            m_missileTimer.Disable();
            ClearTarget(pSE);
            return;
        }
        if (pSE->DestinyMgr() == nullptr) {
            _log(NPC__AI_TRACE, "%s(%u): Target %s(%u) has no destiny manager.  Clear target and move on",
                    m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
            m_missileTimer.Disable();
            ClearTarget(pSE);
            return;
        }
        // Check to see if the target is not cloaked:
        if (pSE->DestinyMgr()->IsCloaked()) {
            _log(NPC__AI_TRACE, "%s(%u): Target %s(%u) is cloaked.  Clear target and move on",
                    m_npc->GetName(), m_npc->GetID(), pSE->GetName(), pSE->GetID());
            m_missileTimer.Disable();
            ClearTarget(pSE);
            return;
        }
        if (m_npc->TargetMgr()->CanAttack())
            AttackTarget(pSE);
    }
}

void NPCAIMgr::FitModules()
{
    m_modules.clear();
    uint8 slotIdx = 0;

    // Determine weapon GUID from missile type
    std::string guid = "effects.Laser";
    if (m_missileTypeID > 0)
        guid = "effects.MissileDeployment";

    // Primary weapon module (hi slot)
    if (m_optimalRange > 0 || m_missileTypeID > 0) {
        NPCModule mod;
        // The weapon's real TYPE ID — the client's FitTurrets builds a turret model
        // from each fitted module's typeID (group must be in const.turretModuleGroups).
        // Prefer the hull's gfxTurretID (a real weapon module typeID we set on bots),
        // else the missile type, else fall back to a placeholder.
        uint32 weaponType = 248;
        if (m_self->HasAttribute(AttrGfxTurretID))
            weaponType = m_self->GetAttribute(AttrGfxTurretID).get_uint32();
        else if (m_missileTypeID > 0)
            weaponType = m_missileTypeID;
        mod.typeID = weaponType;
        mod.slotFlag = flagHiSlot0 + slotIdx++;
        mod.active = false;
        mod.cycleTime = m_attackSpeed;
        mod.cycleTimer.Start(m_attackSpeed);
        mod.effectGUID = guid;
        mod.graphicID = m_self->HasAttribute(AttrGfxTurretID) ? m_self->GetAttribute(AttrGfxTurretID).get_uint32() : 0;
        mod.optimalRange = m_optimalRange;
        mod.falloff = m_falloff;
        mod.trackingSpeed = m_trackingSpeed;
        mod.damageMultiplier = m_damageMultiplier;
        // Max reachable engagement range is computed in the ctor (weapon range for
        // turrets, missile flight range for missile ships) — the same value a perfect
        // player could achieve. entityAttackRange (245km on Eradicator) never gates fire.
        mod.effectiveRange = m_maxAttackRange;
        m_modules.push_back(mod);
    }

    // Missile launcher (separate from turret if both present)
    if (m_missileTypeID > 0 && guid != "effects.MissileDeployment") {
        NPCModule mod;
        mod.typeID = m_missileTypeID;
        mod.slotFlag = flagHiSlot0 + slotIdx++;
        mod.active = false;
        mod.cycleTime = m_launcherCycleTime > 100 ? m_launcherCycleTime : m_attackSpeed;
        mod.cycleTimer.Start(mod.cycleTime);
        mod.effectGUID = "effects.MissileDeployment";
        mod.damageMultiplier = m_damageMultiplier;
        m_modules.push_back(mod);
    }

    // Warp scrambler (mid slot)
    if (m_warpScramRange > 0 && m_warpScramStrength > 0) {
        NPCModule mod;
        mod.typeID = 227; // Warp Scrambler I placeholder
        mod.slotFlag = flagMidSlot0 + (slotIdx++ % 8);
        mod.cycleTime = m_attackSpeed;
        mod.cycleTimer.Start(m_attackSpeed);
        mod.effectGUID = "effects.WarpScramble";
        mod.ewarStrength = m_warpScramStrength;
        mod.ewarRange = m_warpScramRange;
        mod.ewarChance = m_warpScramChance;
        m_modules.push_back(mod);
    }

    // Stasis webifier (mid slot)
    if (m_webRange > 0) {
        NPCModule mod;
        mod.typeID = 324; // Stasis Webifier I placeholder
        mod.slotFlag = flagMidSlot0 + (slotIdx++ % 8);
        mod.cycleTime = m_attackSpeed;
        mod.cycleTimer.Start(m_attackSpeed);
        mod.effectGUID = "effects.ModifyTargetSpeed";
        mod.ewarStrength = m_webStrength;
        mod.ewarRange = m_webRange;
        mod.ewarChance = m_webChance;
        m_modules.push_back(mod);
    }

    // ECM (mid slot)
    if (m_ecmRange > 0 && m_ecmStrength > 0) {
        NPCModule mod;
        mod.typeID = 290; // ECM - White Noise Generator I placeholder
        mod.slotFlag = flagMidSlot0 + (slotIdx++ % 8);
        mod.cycleTime = m_ecmDuration > 0 ? m_ecmDuration : m_attackSpeed;
        mod.cycleTimer.Start(mod.cycleTime);
        mod.effectGUID = "effects.ElectronicAttributeModifyTarget";
        mod.ewarStrength = m_ecmStrength;
        mod.ewarRange = m_ecmRange;
        mod.ewarChance = m_ecmChance;
        m_modules.push_back(mod);
    }

    // Target painter (mid slot)
    if (m_paintRange > 0 && m_paintMultiplier > 0) {
        NPCModule mod;
        mod.typeID = 291; // Target Painter I placeholder
        mod.slotFlag = flagMidSlot0 + (slotIdx++ % 8);
        mod.cycleTime = m_paintDuration > 0 ? m_paintDuration : m_attackSpeed;
        mod.cycleTimer.Start(mod.cycleTime);
        mod.effectGUID = "effects.TargetPaint";
        mod.ewarStrength = m_paintMultiplier;
        mod.ewarRange = m_paintRange;
        mod.ewarChance = m_paintChance;
        m_modules.push_back(mod);
    }
}

void NPCAIMgr::CycleModules(SystemEntity* pTarget)
{
    if (pTarget == nullptr) return;
    GPoint npcPos = m_npc->GetPosition();

    for (auto& mod : m_modules) {
        if (!mod.cycleTimer.Check()) continue;
        mod.cycleTimer.Start(mod.cycleTime);

        double dist = npcPos.distance(pTarget->GetPosition());

        // Determine if this module can reach the target
        // Modules fire within their own effective range (= weapon reach for a perfect
        // player), never the huge entityAttackRange.
        float moduleRange = mod.effectiveRange > 0 ? mod.effectiveRange : (float)m_maxAttackRange;
        bool inRange = (mod.ewarRange > 0) ? (dist <= mod.ewarRange) : (dist <= moduleRange);

        if (!inRange) {
            // Out of range — try to activate anyway for effect (no damage)
            if (mod.graphicID > 0)
                m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                             pTarget->GetID(), 0, mod.effectGUID, 1, 1, 1, mod.cycleTime, 0, mod.graphicID);
            continue;
        }

        // EWAR modules — check activation chance
        if (mod.ewarRange > 0 && mod.ewarChance > 0 && MakeRandomFloat() < mod.ewarChance)
            continue;  // chance failed, skip this cycle

        mod.active = true;
        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                     pTarget->GetID(), 0, mod.effectGUID, 1, 1, 1, mod.cycleTime, 0, mod.graphicID);
    }
}

void NPCAIMgr::ClearTarget(SystemEntity* pSE) {
    // Clear warp scramble status on target if this NPC has scrambler capability
    if (m_warpScramStrength > 0 and pSE != nullptr) {
        InventoryItemRef targetItem = pSE->GetSelf();
        if (targetItem and targetItem->HasAttribute(AttrWarpScrambleStatus))
            targetItem->SetAttribute(AttrWarpScrambleStatus, 0.0f);
        // Stop the sticky WarpScramble beam on the client.
        if (m_destiny != nullptr)
            m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                         pSE->GetID(), 0, "effects.WarpScramble",
                                         1, 0, 0, m_attackSpeed, 0, 0);
    }
    // Release stasis webifier (restore target speed + stop the beam).
    // Only if we actually applied a web to THIS target — WebbedMe(false) divides
    // m_maxShipSpeed by (1+speedFactor/100) = 0.4 (x2.5) for a -60% web; calling
    // it without a matching WebbedMe(true) inflates the victim's speed on every
    // target drop (Sleeper webbers have SpeedFactor=-60 in SDE, so this used to
    // stack x2.5 per ClearTarget -> player ship speed ballooned to 93383 m/s).
    if (m_webRange > 0 and m_webApplied and m_webTargetID == pSE->GetID()
    and pSE->DestinyMgr() != nullptr) {
        pSE->DestinyMgr()->WebbedMe(m_self, false);
        m_webApplied = false;
        m_webTargetID = 0;
        if (m_destiny != nullptr)
            m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                         pSE->GetID(), 0, "effects.ModifyTargetSpeed",
                                         1, 0, 0, m_attackSpeed, 0, 0);
    }
    // Stop target-paint beam and restore the original signature radius.
    if (m_paintRange > 0 and m_destiny != nullptr) {
        auto pit = m_paintOriginals.find(pSE->GetID());
        if (pit != m_paintOriginals.end()) {
            InventoryItemRef tRef = pSE->GetSelf();
            if (tRef && tRef->HasAttribute(AttrSignatureRadius))
                tRef->SetAttribute(AttrSignatureRadius, pit->second);
            m_paintOriginals.erase(pit);
        }
        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                     pSE->GetID(), 0, "effects.TargetPaint",
                                     1, 0, 0, m_paintDuration, 0, 0);
    }
    m_npc->TargetMgr()->ClearTarget(pSE);
    //m_npc->TargetMgr()->OnTarget(pSE, TargMgr::Mode::Lost);

    if (m_npc->TargetMgr()->HasNoTargets())
        SetIdle();
}

void NPCAIMgr::AttackTarget(SystemEntity* pSE) {
    if (pSE == nullptr)
        return;
    // Apply warp scramble if NPC has scrambler capability
    if (m_warpScramRange > 0 and m_warpScramStrength > 0) {
        double dist = m_npc->GetPosition().distance(pSE->GetPosition());
        if (dist <= m_warpScramRange) {
            if (!m_warpScramblerTimer.Enabled() or m_warpScramblerTimer.Check()) {
                if (MakeRandomFloat() > m_warpScramChance) {
                    InventoryItemRef targetRef = pSE->GetSelf();
                    // notify=true so the client's godma gets OnModuleAttributeChange and
                    // shows the warp-scramble indicator on the target ship.
                    targetRef->SetAttribute(AttrWarpScrambleStatus, m_warpScramStrength, true);
                    uint32 scramGfxID = 0;
                    if (m_self->HasAttribute(AttrGfxBoosterID))
                        scramGfxID = m_self->GetAttribute(AttrGfxBoosterID).get_uint32();
                    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                                 pSE->GetID(), 0, "effects.WarpScramble",
                                                 1, 1, 1, m_attackSpeed, 0, scramGfxID);
                    m_warpScramblerTimer.Start(m_attackSpeed);
                }
            }
        } else {
            // Target left scrambler range — drop the effect and the status so the
            // client stops showing the sticky WarpScramble beam.
            InventoryItemRef targetRef = pSE->GetSelf();
            if (targetRef && targetRef->HasAttribute(AttrWarpScrambleStatus))
                targetRef->SetAttribute(AttrWarpScrambleStatus, 0.0f, true);
            m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                         pSE->GetID(), 0, "effects.WarpScramble",
                                         1, 0, 0, m_attackSpeed, 0, 0);
        }
    }

    // EWAR — stasis webifier
    if (m_webRange > 0) {
        double dist = m_npc->GetPosition().distance(pSE->GetPosition());
        if (dist <= m_webRange) {
            if (!m_webifierTimer.Enabled() or m_webifierTimer.Check()) {
                if (MakeRandomFloat() > m_webChance) {
                    if (pSE->DestinyMgr() != nullptr and m_self->HasAttribute(AttrSpeedFactor)) {
                        float origFactor = m_self->GetAttribute(AttrSpeedFactor).get_float();
                        // Undo any web currently applied before re-applying, so
                        // repeated cycles don't stack the speed multiplier and
                        // ClearTarget can restore speed symmetrically.
                        if (m_webApplied) {
                            if (m_webTargetID == pSE->GetID()) {
                                m_self->SetAttribute(AttrSpeedFactor, m_webStrength * -100.0f, false);
                                pSE->DestinyMgr()->WebbedMe(m_self, false);
                                m_self->SetAttribute(AttrSpeedFactor, origFactor, false);
                            } else {
                                SystemEntity* oldWebTarget = m_npc->SystemMgr()->GetSE(m_webTargetID);
                                if (oldWebTarget != nullptr and oldWebTarget->DestinyMgr() != nullptr) {
                                    m_self->SetAttribute(AttrSpeedFactor, m_webStrength * -100.0f, false);
                                    oldWebTarget->DestinyMgr()->WebbedMe(m_self, false);
                                    m_self->SetAttribute(AttrSpeedFactor, origFactor, false);
                                }
                                m_webApplied = false;
                                m_webTargetID = 0;
                            }
                        }
                        // Set web strength (-60% by default) via SpeedFactor attribute
                        m_self->SetAttribute(AttrSpeedFactor, m_webStrength * -100.0f, false);
                        pSE->DestinyMgr()->WebbedMe(m_self, true);
                        m_self->SetAttribute(AttrSpeedFactor, origFactor, false);
                        m_webApplied = true;
                        m_webTargetID = pSE->GetID();
                    }
                    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                                 pSE->GetID(), 0, "effects.ModifyTargetSpeed",
                                                 1, 1, 1, m_attackSpeed, 0, 0);
                    m_webifierTimer.Start(m_attackSpeed);
                }
            }
        }
    }

    // EWAR — ECM (break target's locks)
    if (m_ecmRange > 0 and m_ecmStrength > 0) {
        double dist = m_npc->GetPosition().distance(pSE->GetPosition());
        if (dist <= m_ecmRange) {
            if (!m_ecmTimer.Enabled() or m_ecmTimer.Check()) {
                if (MakeRandomFloat() > m_ecmChance) {
                    // ECM: compare jam strength to target's strongest sensor strength
                    // Use Gravimetric(463), Ladar(464), Radar(465), Magnetometric(466)
                    float targetSensorStrength = 0.0f;
                    InventoryItemRef targetRef = pSE->GetSelf();
                    if (targetRef) {
                        if (targetRef->HasAttribute(AttrScanGravimetricStrength))
                            targetSensorStrength = std::max(targetSensorStrength,
                                targetRef->GetAttribute(AttrScanGravimetricStrength).get_float());
                        if (targetRef->HasAttribute(AttrScanLadarStrength))
                            targetSensorStrength = std::max(targetSensorStrength,
                                targetRef->GetAttribute(AttrScanLadarStrength).get_float());
                        if (targetRef->HasAttribute(AttrScanRadarStrength))
                            targetSensorStrength = std::max(targetSensorStrength,
                                targetRef->GetAttribute(AttrScanRadarStrength).get_float());
                        if (targetRef->HasAttribute(AttrScanMagnetometricStrength))
                            targetSensorStrength = std::max(targetSensorStrength,
                                targetRef->GetAttribute(AttrScanMagnetometricStrength).get_float());
                    }
                    float jamChance = (targetSensorStrength > 0) ? m_ecmStrength / targetSensorStrength : 0.5f;
                    jamChance = std::min(jamChance, 0.95f);
                    if (MakeRandomFloat() < jamChance) {
                        // Break target's lock on this NPC
                        if (pSE->TargetMgr() != nullptr)
                            pSE->TargetMgr()->ClearTarget(m_npc);
                        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                                     pSE->GetID(), 0, "effects.ElectronicAttributeModifyTarget",
                                                     1, 1, 1, m_ecmDuration, 0, 0);
                    }
                    m_ecmTimer.Start(m_ecmDuration);
                }
            }
        }
    }

    // Cycle fitted modules (weapon effects)
    CycleModules(pSE);

    // EWAR — energy neutralizer (energy-drain turrets: Sansha Energy Neutralizer
    // Sentry etc.). Drains target capacitor; pure-EWAR turrets deal no damage.
    if (m_neutRange > 0 && m_neutAmount > 0) {
        double dist = m_npc->GetPosition().distance(pSE->GetPosition());
        if (dist <= m_neutRange) {
            if (!m_neutTimer.Enabled() or m_neutTimer.Check()) {
                if (MakeRandomFloat() < m_neutChance) {
                    InventoryItemRef targetRef = pSE->GetSelf();
                    if (targetRef && targetRef->HasAttribute(AttrCapacitorCharge)) {
                        double cap = targetRef->GetAttribute(AttrCapacitorCharge).get_double() - m_neutAmount;
                        if (cap < 0.0) cap = 0.0;
                        targetRef->SetAttribute(AttrCapacitorCharge, cap);
                        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                                     pSE->GetID(), 0, "effects.EnergyDestabilization",
                                                     0, 1, 1, m_neutDuration, 0, 0);
                    }
                }
                m_neutTimer.Start(m_neutDuration);
            }
        }
    }

    // EWAR — target painting (increase signature radius)
    if (m_paintRange > 0 and m_paintMultiplier > 0) {
        double dist = m_npc->GetPosition().distance(pSE->GetPosition());
        if (dist <= m_paintRange) {
            if (!m_paintTimer.Enabled() or m_paintTimer.Check()) {
                if (MakeRandomFloat() > m_paintChance) {
                    InventoryItemRef targetRef = pSE->GetSelf();
                    if (targetRef and targetRef->HasAttribute(AttrSignatureRadius)) {
                        EvilNumber sig = targetRef->GetAttribute(AttrSignatureRadius);
                        // Remember the original so ClearTarget can restore it (paint is temporary).
                        if (m_paintOriginals.find(pSE->GetID()) == m_paintOriginals.end())
                            m_paintOriginals[pSE->GetID()] = sig.get_float();
                        float boost = sig.get_float() * (1.0f + m_paintMultiplier);
                        targetRef->SetAttribute(AttrSignatureRadius, boost);
                        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                                     pSE->GetID(), 0, "effects.TargetPaint",
                                                     1, 1, 1, m_paintDuration, 0, 0);
                    }
                    m_paintTimer.Start(m_paintDuration);
                }
            }
        }
    }

    // EWAR-only turrets (stasis/web, energy neutralizer) don't fire a weapon and
    // deal no direct damage — their EWAR is applied above.
    if (m_ewarOnly)
        return;

    // Select effect GUID based on NPC weapon type (from SDE attributes).
    // Client's StandardWeapon handles all turret GUIDs identically (animates locators).
    // Missile-using NPCs get MissileDeployment effect instead of turret.
    std::string guid = "effects.Laser";
    if (m_missileTypeID > 0) {
        guid = "effects.MissileDeployment";
    } else if (m_self->HasAttribute(AttrGfxTurretID)) {
        // Use ProjectileFiredForEntities as fallback for non-laser NPC turrets
        guid = "effects.ProjectileFiredForEntities";
    }
    uint32 gfxID = 0;
    if (m_self->HasAttribute(AttrGfxTurretID))
        gfxID = m_self->GetAttribute(AttrGfxTurretID).get_uint32();
    // ALWAYS send the weapon effect (moduleID = ship itemID). The client finds the
    // turret by moduleID on the EntityShip ball — which it builds from the hull's
    // own gfxTurretID (godma), so even an NPC/bot with no AttrGfxTurretID in the
    // server DB renders a correct turret. gfxID is only an extra graphic hint.
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                 pSE->GetID(),0,guid,1,1,
                                 1,m_attackSpeed,0,gfxID);

    // Deal damage only while the target is within weapon reach. NPCs locked on a
    // target far beyond their max range (e.g. a bot Mega shooting a pilot 574 km
    // away) would otherwise hit regardless of distance. Missile batteries deal
    // their damage via actual missiles (LaunchMissile), not a direct hit.
    double dist = m_npc->GetPosition().distance(pSE->GetPosition());
    if (dist <= m_maxAttackRange && m_missileTypeID == 0) {
        Damage d(m_npc,
                 m_self,
                 m_npc->GetKinetic(),
                 m_npc->GetThermal(),
                 m_npc->GetEM(),
                 m_npc->GetExplosive(),
                 m_formula.GetNPCToHit(m_npc, pSE),
                 EVEEffectID::targetAttack
                );

        if (sConfig.npc.UseDamageMultiplier)
            if (m_damageMultiplier > 0)
                d *= m_damageMultiplier;

        pSE->ApplyDamage(d);
    }

    // Smartbomb/AoE: if NPC has EmpFieldRange, deal splash damage to all targets in range
    if (m_smartbombRange > 0 and (!m_smartbombTimer.Enabled() or m_smartbombTimer.Check())) {
        GPoint npcPos = m_npc->GetPosition();
        std::vector<Client*> bubbleClients;
        m_npc->SysBubble()->GetPlayers(bubbleClients);
        for (auto client : bubbleClients) {
            if (client == nullptr) continue;
            SystemEntity* targetSE = client->GetShipSE();
            if (targetSE == nullptr or targetSE == pSE) continue;
            if (targetSE->DestinyMgr() and targetSE->DestinyMgr()->IsCloaked()) continue;
            float dist = npcPos.distance(targetSE->GetPosition());
            if (dist <= m_smartbombRange) {
                // Falloff: full damage at center, half at max range
                float falloff = 1.0f - (dist / m_smartbombRange) * 0.5f;
                Damage splash(m_npc, m_self, m_npc->GetEM() * falloff, m_npc->GetExplosive() * falloff,
                              m_npc->GetKinetic() * falloff, m_npc->GetThermal() * falloff, 1.0f, 0);
                targetSE->ApplyDamage(splash);
                // effects.EMPWave is used instead of effects.SmartBomb (not in client Repository)
                m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                             targetSE->GetID(), 0, "effects.EMPWave", 1, 1, 1, m_attackSpeed * 3, 0, 0);
            }
        }
        m_smartbombTimer.Start(m_attackSpeed * 3);
    }
}

/* missile shit..
 * //AttrEntityDefenderChance  - chance to shoot defender missile at incomming missile
 * //AttrMissileLaunchDuration  - missile cycle time
 * //AttrEntityMissileTypeID
 * //AttrMissileEntityVelocityMultiplier
 * //AttrMissileEntityFlightTimeMultiplier
 * //AttrMissileEntityAoeCloudSizeMultiplier
 * //AttrMissileEntityAoeVelocityMultiplier
 * //AttrMissileEntityAoeFalloffMultiplier
 */

uint16 NPCAIMgr::PickMissileForChargeGroup(uint32 chargeGroupID) const
{
    // Concrete T1 missiles per charge group (SDE Crucible typeIDs). Used by
    // missile batteries (group 417) which have no AttrEntityMissileTypeID but a
    // chargeGroup attribute instead.
    switch (chargeGroupID) {
        case 384:   return 210;   // Light Missile   -> Scourge Light Missile
        case 385:   return 209;   // Heavy Missile   -> Scourge Heavy Missile
        case 386:   return 203;   // Cruise Missile  -> Scourge Cruise Missile
        case 89:    return 267;   // Torpedo         -> Scourge Torpedo
        case 476:   return 2678;  // Citadel Torpedo -> Inferno Citadel Torpedo
        default:    return 0;
    }
}

void NPCAIMgr::LaunchMissile(uint16 typeID, SystemEntity* pSE)
{
    if (typeID == 0)
        return;
    // Actually Launch a missile, creating a new Destiny object for it
    // ItemData( uint32 _typeID, uint32 _ownerID, uint32 _locationID, EVEItemFlags _flag, const char *_name = "", \
              const GPoint &_position = NULL_ORIGIN, const char *_customInfo = "", bool _contraband = false);
    ItemData idata(typeID, m_npc->GetID(), m_npc->GetLocationID(), flagMissile, "NPC Missile", m_npc->GetPosition());
    InventoryItemRef missileRef = sItemFactory.SpawnItem(idata);
    if (missileRef.get() == nullptr)
        return;  // make error here

    // modify missile based on npc attribs
    if (m_self->HasAttribute(AttrMissileEntityVelocityMultiplier))
        missileRef->MultiplyAttribute(AttrMaxVelocity, m_self->GetAttribute(AttrMissileEntityVelocityMultiplier));
    if (m_self->HasAttribute(AttrMissileEntityFlightTimeMultiplier))    // this may be wrong
        missileRef->MultiplyAttribute(AttrExplosionDelay, m_self->GetAttribute(AttrMissileEntityFlightTimeMultiplier));
    if (m_self->HasAttribute(AttrMissileEntityAoeVelocityMultiplier))
        missileRef->MultiplyAttribute(AttrAoeVelocity, m_self->GetAttribute(AttrMissileEntityAoeVelocityMultiplier));
    if (m_self->HasAttribute(AttrMissileEntityAoeCloudSizeMultiplier))
        missileRef->MultiplyAttribute(AttrAoeCloudSize, m_self->GetAttribute(AttrMissileEntityAoeCloudSizeMultiplier));
    if (m_self->HasAttribute(AttrMissileEntityAoeFalloffMultiplier))
        missileRef->MultiplyAttribute(AttrAoeFalloff, m_self->GetAttribute(AttrMissileEntityAoeFalloffMultiplier));

    SystemManager* pSystem = m_npc->SystemMgr();
    // Missile(InventoryItemRef self, PyServiceMgr &services, SystemManager* system, InventoryItemRef module, SystemEntity* target, ShipItem* ship);
    Missile* pMissile = new Missile(missileRef, pSystem->GetServiceMgr(),  pSystem, m_self, pSE, m_npc);
    if (pMissile == nullptr)
        return; // make error here
    double distance = pMissile->GetPosition().distance(pSE->GetPosition());
    double missileSpeed = missileRef->GetAttribute(AttrMaxVelocity).get_float();
    double travelTime = (distance/missileSpeed);
    if (travelTime < 1)
        travelTime = 1;
    pMissile->SetSpeed(missileSpeed);
    pMissile->SetHitTimer(travelTime *1000);
    pMissile->DestinyMgr()->MakeMissile(pMissile);

    // tell target a missile has been launched at them.. (defender missile trigger for ship, tower, pos, npc, others?)
    if (typeID != 265)  // but only if it's NOT a defender missile
        pSE->MissileLaunched(pMissile);
}

void NPCAIMgr::MissileLaunched(Missile* pMissile)
{
    float chance = m_self->GetAttribute(AttrEntityDefenderChance).get_float();
    if (sConfig.npc.DefenderMissileChance)
        chance = sConfig.npc.DefenderMissileChance;
    // check chance to shoot defender missile at incomming missile (working, ??/??/??)
    if (MakeRandomFloat() < chance)
        LaunchMissile(265, pMissile); // defender missile
}

float NPCAIMgr::GetTargetTime()
{
    float targetTime = (m_self->GetAttribute(AttrScanSpeed).get_float());
    float radius = m_self->GetAttribute(AttrRadius).get_float();
    if (targetTime < 1) {
        if (radius < 30) {
            targetTime = 1500;
        } else if (radius < 60) {
            targetTime = 2500;
        } else if (radius < 150) {
            targetTime = 4000;
        } else if (radius < 280) {
            targetTime = 6000;
        } else if (radius < 550) {
            targetTime = 8000;
        } else {
            targetTime = 13000;
        }
    }
    return targetTime;
}

void NPCAIMgr::DisableRepTimers(bool shield/*true*/, bool armor/*true*/)
{
    if (armor)
        m_armorRepairTimer.Disable();
    if (shield)
        m_shieldBoosterTimer.Disable();
}

std::string NPCAIMgr::GetStateName(int8 stateID)
{
    switch (stateID) {
        case NPCAI::State::Idle:           return "Idle";
        case NPCAI::State::Chasing:        return "Chasing";
        case NPCAI::State::Engaged:        return "Engaged";
        case NPCAI::State::Fleeing:        return "Fleeing";
        case NPCAI::State::Following:      return "Following";
        case NPCAI::State::Signaling:      return "Signaling";
        case NPCAI::State::WarpOut:        return "Warping Out";
        case NPCAI::State::WarpFollow:     return "Following Warp";
        default:                           return "Invalid";
    }
}
