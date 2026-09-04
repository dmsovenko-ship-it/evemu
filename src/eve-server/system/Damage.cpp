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
*/


#include "system/Damage.h"

#include "../../eve-common/EVE_Damage.h"

#include "Client.h"
#include "EntityList.h"
#include "EVEServerConfig.h"
#include "incursion/IncursionMgr.h"
#include "manufacturing/Blueprint.h"
#include "map/MapDB.h"
#include "npc/NPC.h"
#include "npc/PlayerBot.h"
#include "pos/sovStructures/IHub.h"
#include "station/Outpost.h"
#include "npc/NPCAI.h"
#include "npc/Drone.h"
#include "ship/Ship.h"
#include "ship/modules/GenericModule.h"
#include "system/Container.h"
#include "system/SystemBubble.h"
#include "system/cosmicMgrs/AnomalyMgr.h"
#include "standing/StandingMgr.h"
#include "standing/KillRightDB.h"
#include "StaticDataMgr.h"

/*
DAMAGE
DAMAGE__ERROR
DAMAGE__WARNING
DAMAGE__MESSAGE
DAMAGE__INFO
DAMAGE__TRACE
DAMAGE__DEBUG
*/

// this is for turrets
Damage::Damage(SystemEntity* pSE, InventoryItemRef wRef, float kin, float ther, float emp, float exp, float mod, uint16 eID)
: srcSE(pSE), effectID(eID), weaponRef(wRef), chargeRef(InventoryItemRef(nullptr)),
em(emp), kinetic(kin), thermal(ther), explosive(exp), modifier(mod)
{
}

// this is for npcs
Damage::Damage(SystemEntity* pSE, InventoryItemRef wRef, float mod, uint16 eID)
: srcSE(pSE), effectID(eID), weaponRef(wRef), chargeRef(InventoryItemRef(nullptr)),
modifier(mod),
em(wRef->GetAttribute(AttrEmDamage).get_float()),
kinetic(wRef->GetAttribute(AttrKineticDamage).get_float()),
thermal(wRef->GetAttribute(AttrThermalDamage).get_float()),
explosive(wRef->GetAttribute(AttrExplosiveDamage).get_float())
{
    _log(DAMAGE__WARNING, "Damage:C'tor - Called by source %s(%u) with weapon %s(%u).",
         srcSE->GetName(), srcSE->GetID(), wRef->name(), wRef->itemID() );
}

// this is for missiles
Damage::Damage(SystemEntity* pSE, InventoryItemRef wRef, InventoryItemRef cRef, uint16 eID)
: srcSE(pSE), effectID(eID), weaponRef(wRef), chargeRef(cRef),
modifier(1),
em(cRef->GetAttribute(AttrEmDamage).get_float()),
kinetic(cRef->GetAttribute(AttrKineticDamage).get_float()),
thermal(cRef->GetAttribute(AttrThermalDamage).get_float()),
explosive(cRef->GetAttribute(AttrExplosiveDamage).get_float())
{
    _log(DAMAGE__WARNING, "Damage:C'tor - Called by source %s(%u) with weapon %s(%u) using charge %s(%u).",
         srcSE->GetName(), srcSE->GetID(), wRef->name(), wRef->itemID(), cRef->name(), cRef->itemID() );
}

// No specific damage dealt here, just killed
Damage::Damage(SystemEntity* pSE, bool fatal_blow/*false*/)
: srcSE(pSE), effectID(EVEEffectID::targetAttack),
em(0.0f),kinetic(0.0f),thermal(0.0f),explosive(0.0f),
weaponRef(InventoryItemRef(nullptr)),
chargeRef(InventoryItemRef(nullptr))
{
    assert(fatal_blow and "Damage() fatal_blow called without 2nd param being true!");
}

bool SystemEntity::ApplyDamage(Damage &d) {
    double profileStartTime(GetTimeUSeconds());

    // Null source guard (e.g. sentry gun damage)
    if (d.srcSE == nullptr) {
        d.srcSE = this; // Damage originates from self (no attribution)
    }

    // PvP aggression — EVE rule: attacking another pilot (or their drones/fighters)
    // sets an aggression flag on the attacker (15 min, no dock/jump). Works for
    // player↔player, player↔charbot and drone owners (a drone hit transfers to its
    // pilot). The attacker is the pilot of the source SE (or the drone's owner);
    // the victim is the pilot of this SE (or this drone's owner). Charbots are
    // PlayerBots (NPCSE with a pilot identity) — they flag themselves via their
    // own StartAggressionTimer, the real-player side is handled here.
    //
    // SELF-DEFENCE: whoever hits FIRST is the aggressor; the victim has the right
    // to defend without being flagged. When the victim returns fire, their attack
    // must not grant them aggression. We record who hit the victim first, then
    // skip aggression for the victim's counter-fire.
    if (sConfig.crime.Enabled) {
        Client* atkClient = nullptr;
        if (d.srcSE->HasPilot())
            atkClient = d.srcSE->GetPilot();
        else if (d.srcSE->IsDroneSE() && d.srcSE->GetDroneSE()->GetOwner() != nullptr)
            atkClient = d.srcSE->GetDroneSE()->GetOwner();

        Client* vicClient = nullptr;
        if (HasPilot())
            vicClient = GetPilot();
        else if (IsDroneSE() && GetDroneSE()->GetOwner() != nullptr)
            vicClient = GetDroneSE()->GetOwner();

        // The attacker's identity for self-defence tracking: a Client's charID,
        // or a charbot's botCharID.
        uint32 atkID = 0;
        if (atkClient != nullptr)
            atkID = atkClient->GetCharacterID();
        else if (d.srcSE->IsNPCSE() && d.srcSE->GetNPCSE() != nullptr
                 && d.srcSE->GetNPCSE()->IsPlayerBot())
            atkID = dynamic_cast<PlayerBot*>(d.srcSE->GetNPCSE())->GetBotCharID();

        // Player↔player / player↔drone-owner: both are Clients.
        if (atkClient != nullptr && vicClient != nullptr && atkClient != vicClient) {
            float sysSec = m_system ? m_system->GetSystemSecurityRating() : 0.0f;
            // Victim returning fire at whoever hit them first — no new aggression.
            if (vicClient->GetCrimeWatch() != nullptr
                && atkClient->GetCrimeWatch() != nullptr
                && atkClient->GetCrimeWatch()->WasAttackedBy(vicClient->GetCharacterID()))
                return false;
            // Mark the victim as having been attacked by this attacker first
            // (so THEIR counter-fire is self-defence).
            if (vicClient->GetCrimeWatch() != nullptr && atkID != 0)
                vicClient->GetCrimeWatch()->RegisterAttackBy(atkID);
            if (atkClient->GetCrimeWatch() != nullptr)
                atkClient->GetCrimeWatch()->OnAggression(vicClient, sysSec);
        }
        // Player attacking a charbot still gets flagged — but only if the charbot
        // didn't start the fight. The charbot flags itself in its own OnAttacked.
        else if (atkClient != nullptr && vicClient == nullptr && IsNPCSE()
                 && GetNPCSE() != nullptr && GetNPCSE()->IsPlayerBot()) {
            float sysSec = m_system ? m_system->GetSystemSecurityRating() : 0.0f;
            PlayerBot* pbot = dynamic_cast<PlayerBot*>(GetNPCSE());
            if (pbot != nullptr && atkClient->GetCrimeWatch() != nullptr) {
                // Self-defence: if the charbot started the fight, no flags.
                if (atkClient->GetCrimeWatch()->WasAttackedBy(pbot->GetBotCharID()))
                    return false;
                atkClient->GetCrimeWatch()->OnBotAggression(pbot->GetBotCharID(), sysSec);
            }
        }
        // A charbot attacking a REAL player: mark the player as the charbot's
        // victim so their self-defence (return fire) doesn't flag them.
        if (vicClient != nullptr && atkClient == nullptr && d.srcSE->IsNPCSE()
            && d.srcSE->GetNPCSE() != nullptr && d.srcSE->GetNPCSE()->IsPlayerBot()
            && vicClient->GetCrimeWatch() != nullptr) {
            PlayerBot* atkBot = dynamic_cast<PlayerBot*>(d.srcSE->GetNPCSE());
            if (atkBot != nullptr)
                vicClient->GetCrimeWatch()->RegisterAttackBy(atkBot->GetBotCharID());
        }
    }

    // Track damage contribution for incursion contest rewards
    if (d.srcSE->HasPilot() && this->IsNPCSE() && m_bubble != nullptr && m_bubble->IsIncursion()) {
        sIncursionMgr.RecordDamage(m_bubble->GetID(), d.srcSE->GetPilot()->GetCharacterID(), d.GetTotal());
    }

    // Standing loss when a player attacks a convoy NPC
    if (d.srcSE->HasPilot() && this->IsNPCSE() && this->GetNPCSE()->IsConvoy()) {
        Client* attacker = d.srcSE->GetPilot();
        int32 factionID = this->GetNPCSE()->GetWarFactionID();
        if (factionID > 0) {
            sStandingMgr.UpdateStandings(factionID, attacker->GetCharacterID(),
                                         Standings::CombatAggression, -0.001,
                                         "Convoy Aggression");
        }
        // Notify the NPC (distress call, escort defense trigger)
        this->GetNPCSE()->OnAttacked(d.srcSE);
    }

    // Crucible: warp disruption probes are invulnerable to direct fire
    // Smartbombs and bombs can still destroy probes
    if (this->IsProbeSE()) {
        // Allow smartbombs to damage probes
        if (d.weaponRef.get() != nullptr
            and d.weaponRef->groupID() == EVEDB::invGroups::Smart_Bomb)
        {
            _log(DAMAGE__MESSAGE, "%s(%u): Probe hit by smartbomb. Applying %.2f damage.",
                 GetName(), GetID(), d.GetTotal());
        } else {
            _log(DAMAGE__MESSAGE, "%s(%u): Probe is immune to direct damage. Ignoring %.2f damage from %s(%u)",
                 GetName(), GetID(), d.GetTotal(), d.srcSE->GetName(), d.srcSE->GetID());
            return true;
        }
    }

    if (is_log_enabled(DAMAGE__MESSAGE)) {
        if (d.srcSE->IsNPCSE()) {
            _log(DAMAGE__MESSAGE, "%s(%u): Initializing %.2f damage from NPC %s(%u) with K:%.3f, T:%.3f, EM:%.3f, E:%.3f",\
                    GetName(), GetID(), d.GetTotal(), d.srcSE->GetName(), d.srcSE->GetID(), \
                    d.GetKinetic(), d.GetThermal(), d.GetEM(), d.GetExplosive() );
        } else if (d.srcSE->IsDroneSE()){
            _log(DAMAGE__MESSAGE, "%s(%u): Initializing %.2f damage from Drone %s(%u) with K:%.3f, T:%.3f, EM:%.3f, E:%.3f",\
                    GetName(), GetID(), d.GetTotal(), d.srcSE->GetName(), d.srcSE->GetID(), \
                    d.GetKinetic(), d.GetThermal(), d.GetEM(), d.GetExplosive() );
        } else if (d.srcSE->HasPilot()) {
            _log(DAMAGE__MESSAGE, "%s(%u): Initializing %.2f damage from %s's %s(%u) using %s(%u) %s with K:%.3f, T:%.3f, EM:%.3f, E:%.3f",\
                    GetName(), GetID(), d.GetTotal(), d.srcSE->GetPilot()->GetName(), d.srcSE->GetName(), d.srcSE->GetID(), \
                    d.weaponRef->name(), d.weaponRef->itemID(), (d.chargeRef ? d.chargeRef->name() : ""), \
                    d.GetKinetic(), d.GetThermal(), d.GetEM(), d.GetExplosive() );
        } else {
            _log(DAMAGE__MESSAGE, "%s(%u): Initializing %.2f damage from unknown source.", GetName(), GetID(), d.GetTotal());
        }
    }

    int8 damageID(0);
    switch (d.weaponRef->groupID()) {
        case EVEDB::invGroups::Missile_Launcher_Assault:
        case EVEDB::invGroups::Missile_Launcher_Bomb:       // not sure here
        case EVEDB::invGroups::Missile_Launcher_Citadel:
        case EVEDB::invGroups::Missile_Launcher_Cruise:
        case EVEDB::invGroups::Missile_Launcher_Defender:   // not sure here
        case EVEDB::invGroups::Missile_Launcher_Heavy:
        case EVEDB::invGroups::Missile_Launcher_Heavy_Assault:
        case EVEDB::invGroups::Missile_Launcher_Rocket:
        case EVEDB::invGroups::Missile_Launcher_Siege:
        case EVEDB::invGroups::Missile_Launcher_Standard: {
            // apply damage modifier from config
            d *= sConfig.rates.missileDamage;
            // should this be adjusted based on damage?
            damageID = 6;
        } break;
        case EVEDB::invGroups::Super_Weapon: {
        /*   TODO
         * this damage will need to be adjusted based on distance from target, then called for each target,
         *  and modified/corrected as the weapon implementation is completed.
         *  all modifiers to be calc'd in weapon code and sent here for correct damageID
         */
            damageID = 5;
        } break;
        case EVEDB::invGroups::Missile_Launcher_Snowball: {
            // these dont do any damage
            //  update this to use real toHit data (once we implement them....)
            damageID = MakeRandomInt(0,8);
        } break;
        default: {
            float modifier = d.GetModifier();
            d *= modifier;
                 if (modifier == 3.0f)   { damageID = 8; }  //strikes perfectly, wrecking
            else if (modifier > 1.2501f) { damageID = 7; } //places an excellent hit
            else if (modifier > 0.9999f) { damageID = 6; } //aims well
            else if (modifier > 0.7501f) { damageID = 5; } //hits
            else if (modifier > 0.6251f) { damageID = 4; } //lightly hits
            else if (modifier > 0.4121f) { damageID = 3; } //barely scratches
            else if (modifier > 0.3751f) { damageID = 2; } //glances off
            else if (modifier > 0.2501f) { damageID = 1; } //barely misses
            else                         { damageID = 0; } //misses completely
            _log(DAMAGE__TRACE, "%s(%u): Modifier: %.3f, damageID: %u.", GetName(), GetID(), modifier, damageID);
        } break;
    }

    // apply damage modifier from config
    d *= sConfig.rates.damageRate;

    // this is calculated and created on every call...
    Damage DamageToShield = d.MultiplyDup(
        m_self->GetAttribute(AttrShieldKineticDamageResonance).get_float(),
        m_self->GetAttribute(AttrShieldThermalDamageResonance).get_float(),
        m_self->GetAttribute(AttrShieldEmDamageResonance).get_float(),
        m_self->GetAttribute(AttrShieldExplosiveDamageResonance).get_float() );

    bool killed(false);
    float total_damage(0.0f);
    float shield_damage(DamageToShield.GetTotal());
    float available_shield(m_self->GetAttribute(AttrShieldCharge).get_float());
    if (shield_damage <= available_shield) {
        /** @todo  this works, but still needs work....
        if (HasPilot())
            if (damageID > 2) {
                float uniformity = m_self->GetAttribute(AttrShieldUniformity).get_float();
                uniformity += (0.05 * GetPilot()->GetChar()->GetSkillLevel(EvESkill::TacticalShieldManipulation));
                if ((available_shield /m_self->GetAttribute(AttrShieldCapacity).get_float()) < uniformity) {
                    float bleedthru = (d.GetTotal() * 0.01f);
                    m_self->SetAttribute(AttrArmorDamage, (bleedthru + m_self->GetAttribute(AttrArmorDamage).get_float()));
                    shield_damage -= bleedthru;
                }
            }
        */
        total_damage += shield_damage;
        float new_charge = available_shield - shield_damage;
        m_self->SetAttribute(AttrShieldCharge, new_charge);

        // Shield reinforcement threshold (25%) for IHub
        if (this->IsIHubSE()) {
            IHubSE* ihub = this->GetIHubSE();
            int8 st = ihub->GetState();
            if (st != EVEPOS::StructureState::SheildReinforced
                && st != EVEPOS::StructureState::ArmorReinforced
                && st != EVEPOS::StructureState::Reinforced) {
                float cap = m_self->GetAttribute(AttrShieldCapacity).get_float();
                if (cap > 0.0f && (new_charge / cap) < 0.25f && ihub->CheckShieldReinforce()) {
                    killed = false;
                    return true;
                }
            }
        }

        _log(DAMAGE__DEBUG, "%s(%u): Applying %.2f damage to shields. New charge: %.2f.",
             GetName(), GetID(), shield_damage, new_charge);
    } else {
        // get fraction of damage partial shield absorbs, and lower total damage by that fraction
        d *= (1 - (available_shield /shield_damage));
        total_damage += available_shield;

        if (available_shield > 0.0f) {
            _log(DAMAGE__INFO, "%s(%u): Shield depleted with %.2f damage. %.2f damage remains.",
                 GetName(), GetID(), available_shield, d.GetTotal());
            m_self->SetAttribute(AttrShieldCharge, EvilZero);
        }

        //Armor:
        float available_armor = m_self->GetAttribute(AttrArmorHP).get_float() - m_self->GetAttribute(AttrArmorDamage).get_float();
        Damage DamageToArmor = d.MultiplyDup(
            m_self->GetAttribute(AttrArmorKineticDamageResonance).get_float(),
            m_self->GetAttribute(AttrArmorThermalDamageResonance).get_float(),
            m_self->GetAttribute(AttrArmorEmDamageResonance).get_float(),
            m_self->GetAttribute(AttrArmorExplosiveDamageResonance).get_float() );

        float armor_damage = DamageToArmor.GetTotal();
        if (armor_damage <= available_armor) {
            if (HasPilot()) {
                if ((available_armor /m_self->GetAttribute(AttrArmorHP).get_float()) < m_self->GetAttribute(AttrArmorUniformity).get_float()) {
                    float new_damage = d.GetTotal() * 0.01;
                    float hull_damage = m_self->GetAttribute(AttrDamage).get_float() + new_damage;
                    _log(DAMAGE__DEBUG, "%s(%u): Applying %.2f leakthru damage to structure. New structure damage: %.2f",
                         GetName(), GetID(), new_damage, hull_damage);
                    m_self->SetAttribute(AttrDamage, hull_damage);
                    // remove this leakthru damage from armor damage
                    armor_damage -= new_damage;
                }
            }
            total_damage += armor_damage;
            float new_damage = m_self->GetAttribute(AttrArmorDamage).get_float() + armor_damage;
            m_self->SetAttribute(AttrArmorDamage, new_damage);
            _log(DAMAGE__DEBUG, "%s(%u): Applying %.2f damage to armor. New armor damage: %.2f",
                 GetName(), GetID(), armor_damage, new_damage);
        } else {
            d *= (1 - (available_armor /armor_damage));
            total_damage += available_armor;

            if (available_armor > 0) {
                _log(DAMAGE__INFO, "%s(%u): Armor depleted with %.2f damage. %.2f damage remains.",
                     GetName(), GetID(), available_armor, d.GetTotal());
                m_self->SetAttribute(AttrArmorDamage, m_self->GetAttribute(AttrArmorHP));
            }

            // Check for structure reinforcement before entering hull damage
            if ((this->IsIHubSE() && this->GetIHubSE()->CheckReinforce())
                || (this->IsOutpostSE() && this->GetOutpostSE()->CheckReinforce())) {
                killed = false;
                return true;
            }

            //Hull/Structure:
            //The base hp and damage attributes represent structure.
            float available_hull = m_self->GetAttribute(AttrHP).get_float() - m_self->GetAttribute(AttrDamage).get_float();
            Damage DamageToHull = d.MultiplyDup(
                m_self->GetAttribute(AttrKineticDamageResonance).get_float(),
                m_self->GetAttribute(AttrThermalDamageResonance).get_float(),
                m_self->GetAttribute(AttrEmDamageResonance).get_float(),
                m_self->GetAttribute(AttrExplosiveDamageResonance).get_float() );

            float hull_damage = DamageToHull.GetTotal();
            if (hull_damage < available_hull) {
                total_damage += hull_damage;
                float new_damage = m_self->GetAttribute(AttrDamage).get_float() + hull_damage;
                m_self->SetAttribute(AttrDamage, new_damage);
                _log(DAMAGE__DEBUG, "%s(%u): Applying %.2f damage to structure. New structure damage: %.2f",
                     GetName(), GetID(), hull_damage, new_damage);
            } else {
                total_damage += available_hull;
                //dead....
                _log(DAMAGE__INFO, "%s(%u): %.2f damage has depleted our structure. Time to explode.",
                     GetName(), GetID(), hull_damage);
                killed = true;
            }

            // module damage.
            //  after armor is gone, make damage to random module.
            if (HasPilot())
                GetShipSE()->DamageRandModule(sConfig.server.ModuleDamageChance);    // config option for random module damage chance
        }
    }

    // Check for structure reinforcement before killing
    if (killed) {
        bool reinforced = false;
        if (this->IsIHubSE())
            reinforced = this->GetIHubSE()->CheckReinforce();
        else if (this->IsOutpostSE())
            reinforced = this->GetOutpostSE()->CheckReinforce();

        if (reinforced) {
            killed = false;
            m_killed = false;
            m_self->SetAttribute(AttrDamage, EvilZero);
        }
    }

    if (killed) {
        if (m_killed)
            return true;

        m_killed = true;

        if ((m_destiny == nullptr) or (m_bubble == nullptr)) {
            _log(DAMAGE__ERROR, "%s(%u): Cannot process kill - destiny or bubble is null.", GetName(), GetID());
            return true;
        }
        // OnNotify:OnTransmission -  (235799, `You have killed this defenseless NPC, bully.  Also, you have killed this NPC and are receiving this message.`)
        m_destiny->SendTerminalExplosion(m_self->itemID(), m_bubble->GetID(), isGlobal());

        Killed(d);  // this must NOT remove dead SE from system.
        SystemEntity::Killed(d);    // this removes dead SE from system then deletes itemRef and its all contents
    } else {
        /**
         * ALL dmg msgs working  22Apr15 (hacked - found the actual msgIDs)
         * fixed msgIDs and removed xmlp  - 15Sept19
         * @todo  still need to check/add detailed dmg msgs
         */
        if (HasPilot()) {
            //  notify player of damage received
            _log(DAMAGE__MESSAGE, "OnDamageMessage: %.1f damage to %s(%u) from %s(%u)", total_damage, GetName(), GetID(), d.srcSE->GetName(), d.srcSE->GetID());
            PyDict* dict = new PyDict();
                dict->SetItemString("source", new PyInt(d.srcSE->GetID()));
                dict->SetItemString("weapon", new PyInt((d.chargeRef.get() != nullptr ? d.chargeRef->typeID() : d.weaponRef->typeID())));
                dict->SetItemString("target", new PyInt(GetID()));
                dict->SetItemString("damage", new PyFloat(total_damage));
            PyTuple* tuple = new PyTuple(3);
                tuple->SetItem(0, new PyString("OnDamageMessage"));
                tuple->SetItem(1, new PyString(Dmg::Msg::Taken[damageID]));
                tuple->SetItem(2, dict);
            GetPilot()->QueueDestinyEvent(&tuple);
        } else if (IsDroneSE() && GetDroneSE()->GetOwner() != nullptr) {
            //  notify the drone/fighter owner that their drone/fighter is taking
            //  damage — without this the owner sees no combat-log entry when
            //  NPCs/charbots shoot their drones/fighters.
            Client* droneOwner = GetDroneSE()->GetOwner();
            PyDict* dict = new PyDict();
                dict->SetItemString("source", new PyInt(d.srcSE->GetID()));
                dict->SetItemString("weapon", new PyInt((d.chargeRef.get() != nullptr ? d.chargeRef->typeID() : d.weaponRef->typeID())));
                PyTuple* ownerTuple = new PyTuple(2);
                    ownerTuple->SetItem(0, new PyInt(GetID()));                       // drone/fighter entity
                    ownerTuple->SetItem(1, new PyInt(droneOwner->GetCharacterID()));  // owning pilot
                dict->SetItemString("owner", ownerTuple);
                dict->SetItemString("damage", new PyFloat(total_damage));
            PyTuple* tuple = new PyTuple(3);
                tuple->SetItem(0, new PyString("OnDamageMessage"));
                tuple->SetItem(1, new PyString(Dmg::Msg::Taken[damageID]));
                tuple->SetItem(2, dict);
            droneOwner->QueueDestinyEvent(&tuple);
        }
        if (d.srcSE->HasPilot()) {
            //notify to player of damage done:
            _log(DAMAGE__MESSAGE, "OnDamageMessage(Given): %.1f damage by %s(%u) to %s(%u)", total_damage, d.srcSE->GetName(), d.srcSE->GetID(), GetName(), GetID());
            PyDict* dict = new PyDict();
                dict->SetItemString("source", new PyInt(d.srcSE->GetID()));
                dict->SetItemString("weapon", new PyInt((d.chargeRef.get() != nullptr ? d.chargeRef->typeID() : d.weaponRef->typeID())));
                dict->SetItemString("target", new PyInt(GetID()));
                dict->SetItemString("damage", new PyFloat(total_damage));
            PyTuple* tuple = new PyTuple(3);
            bool banked = false;
            tuple->SetItem(0, new PyString("OnDamageMessage"));
            if (d.weaponRef->IsModuleItem()) {
                GenericModule* pMod = d.srcSE->GetShipSE()->GetShipItemRef()->GetModule(d.weaponRef->flag());
                if (pMod != nullptr)
                    if (pMod->IsLinked())
                        banked = true;
            }
            if (banked) {
                tuple->SetItem(1, new PyString(Dmg::Msg::Banked[damageID]));
            } else {
                tuple->SetItem(1, new PyString(Dmg::Msg::Given[damageID]));
            }
            tuple->SetItem(2, dict);
            d.srcSE->GetPilot()->QueueDestinyEvent(&tuple);
        } else if (d.srcSE->IsDroneSE()) {
            // verify drone has owner set
            if (d.srcSE->GetDroneSE()->GetOwner() != nullptr) {
                //  notify player of damage done by drone
                PyDict* dict = new PyDict();
                    dict->SetItemString("source", new PyInt(d.srcSE->GetID()));
                    dict->SetItemString("weapon", new PyInt((d.chargeRef.get() != nullptr ? d.chargeRef->typeID() : d.weaponRef->typeID())));
                    dict->SetItemString("target", new PyInt(GetID()));
                    dict->SetItemString("damage", new PyFloat(total_damage));
                PyTuple* tuple = new PyTuple(3);
                    tuple->SetItem(0, new PyString("OnDamageMessage"));
                    tuple->SetItem(1, new PyString(Dmg::Msg::Given[damageID]));
                    tuple->SetItem(2, dict);
                d.srcSE->GetDroneSE()->GetOwner()->QueueDestinyEvent(&tuple);
            } else {
                // make error about active drone with no owner set
                _log(DRONE__WARNING, "Drone %u attacking %s with no owner set.", d.srcSE->GetID(), GetName());
            }
        }

        SendDamageStateChanged();

        // Flush immediately so damage notifications reach the client in real time,
        // not delayed until the player's next input packet.
        if (HasPilot())
            GetPilot()->FlushQueue();
        else if (IsDroneSE() && GetDroneSE()->GetOwner() != nullptr)
            GetDroneSE()->GetOwner()->FlushQueue();
    }

    if (sConfig.debug.UseProfiling)
        sProfiler.AddTime(Profile::damage, GetTimeUSeconds() - profileStartTime);

    return killed;
}

void ShipSE::Killed(Damage &fatal_blow) {
    if ((m_bubble == nullptr) or (m_destiny == nullptr) or (m_system == nullptr))
        return; // make error here?

    m_shipRef->SetPopped(true);

    /* {'messageKey': 'ShipExploded', 'dataID': 17881627, 'suppressable': True, 'bodyID': 258841, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 258840, 'messageID': 1558}
     * u'ShipExplodedBody'}(u'Your ship has been destroyed by {[character]charID.name}.', None, {u'{[character]charID.name}': {'conditionalValues': [], 'variableType': 0, 'propertyName': 'name', 'args': 0, 'kwargs': {}, 'variableName': 'charID'}})
     */
    uint32 killerID = 0, locationID = GetLocationID();
    Client* pClient(nullptr);
    SystemEntity* killer(fatal_blow.srcSE);

    if (killer->HasPilot()) {
        pClient = killer->GetPilot();
        killerID = pClient->GetCharacterID();
    } else if (killer->IsDroneSE()) {
        pClient = killer->GetDroneSE()->GetOwner();
        if (pClient == nullptr) {
            /** @todo  make error here */
            sLog.Error("Ship::Killed()", "killer == IsDrone and pPlayer == nullptr");
            EvE::traceStack();
        } else {
            killerID = pClient->GetCharacterID();
        }
    } else {
        killerID = killer->GetCorporationID();
        if (killerID == 0)
            killerID = killer->GetID();
    }

    // AttrFwLpKill

    //  log faction kill in dynamic data   -allan
    MapDB::AddKill(locationID);
    MapDB::AddFactionKill(locationID);

    // set up basic wreck data
    GPoint wreckPosition = m_destiny->GetPosition();
    if (wreckPosition.isNaN()) {
        sLog.Error("Ship::Killed()", "Wreck Position is NaN");
        return;
    }
    uint32 wreckTypeID = sDataMgr.GetWreckID(m_self->typeID());
    if (!IsWreckTypeID(wreckTypeID)) {
        sLog.Error("Ship::Killed()", "Could not get wreckType for %s of type %u", m_self->name(), m_self->typeID());
        // default to generic frigate wreck till i get better checks and/or complete wreck data
        wreckTypeID = 26557;
    }

    std::string wreck_name = m_self->itemName() + " Wreck";

    if (!m_self->HasPilot()) {
        // Spawn a wreck for the Ship that was destroyed:
        ItemData wreckItemData(wreckTypeID, killerID, locationID, flagNone, wreck_name.c_str(), wreckPosition, itoa(m_allyID));
        WreckContainerRef wreckItemRef = sItemFactory.SpawnWreckContainer( wreckItemData );
        if (wreckItemRef.get() == nullptr) {
            sLog.Error("Ship::Killed()", "Creating Wreck Item Failed for %s of type %u", wreck_name.c_str(), wreckTypeID);
            return;
        }

        if (is_log_enabled(PHYSICS__TRACE))
            _log(PHYSICS__TRACE, "Ship::Killed() - Ship %s(%u) Position: %.2f,%.2f,%.2f.  Wreck %s(%u) Position: %.2f,%.2f,%.2f.", \
                    GetName(), GetID(), x(), y(), z(), wreckItemRef->name(), wreckItemRef->itemID(), wreckPosition.x, wreckPosition.y, wreckPosition.z);

        DBSystemDynamicEntity wreckEntity = DBSystemDynamicEntity();
            wreckEntity.allianceID = killer->GetAllianceID();
            wreckEntity.categoryID = EVEDB::invCategories::Celestial;
            wreckEntity.corporationID = killer->GetCorporationID();
            wreckEntity.factionID = sDataMgr.GetWreckFaction(wreckTypeID);
            wreckEntity.groupID = EVEDB::invGroups::Wreck;
            wreckEntity.itemID = wreckItemRef->itemID();
            wreckEntity.itemName = wreck_name;
            wreckEntity.ownerID = killerID;
            wreckEntity.typeID = wreckTypeID;
            wreckEntity.position = wreckPosition;

        if (!m_system->BuildDynamicEntity(wreckEntity, m_self->itemID())) {
            sLog.Error("Ship::Killed()", "Spawning Wreck Failed: typeID or typeName not supported: '%u'", wreckTypeID);
            ; /** @todo make error msg here */  //  PyException( MakeCustomError ( "Spawning Wreck Failed: typeID or typeName not supported." ) );
            wreckItemRef->Delete();
            return;
        }

        m_destiny->SendJettisonPacket();
        // wreck was created successfully.  drop loot and add to wreck.
        DropLoot(wreckItemRef, m_self->groupID(), killerID);

        return;
    }

    Client* pPilot(m_self->GetPilot());
    PlayerBot* pBot = nullptr;
    uint32 victimCharID = 0;
    uint32 victimCorpID = m_corpID;

    if (pPilot != nullptr) {
        victimCharID = pPilot->GetCharacterID();
    } else {
        // check if this is a PlayerBot (simulated player) — find the NPC that owns this ship
        uint32 shipItemID = m_self->itemID();
        for (auto& [id, se] : m_system->GetEntities()) {
            if (se == nullptr || se->GetNPCSE() == nullptr)
                continue;
            PlayerBot* bot = dynamic_cast<PlayerBot*>(se->GetNPCSE());
            if (bot != nullptr && bot->GetShipItemRef().get() != nullptr
                && bot->GetShipItemRef()->itemID() == shipItemID) {
                pBot = bot;
                break;
            }
        }
        if (pBot != nullptr) {
            victimCharID = pBot->GetBotCharID();
            victimCorpID = pBot->GetCorporationID();
        } else {
            return;  // no pilot, no bot — nothing to record
        }
    }

    // security status penalty only for real players
    if (pClient != nullptr && pPilot != nullptr)
        if (m_system->GetSystemSecurityRating() > 0) {
            /* http://www.eveinfo.net/wiki/ind~4067.htm
             *  relative_sec_status_penalty = base_penalty * system_truesec * (1 + (victim_sec_status - agressor_sec_status) / 90)
             *  The actual drop in security status seen by the attacker is a function of their current security status and the relative penalty:
             *  security status loss = relative_penalty * (agressor_sec_status + 10)
             */
            /** @todo (allan) check for faction/corp status modifiers here. */
        // EVE formula: loss = 2.5% × sysSec × (1 + (victimSec - attackerSec) / 100) × (attackerSec + 10)
        double modifier = (1 + ((pPilot->GetSecurityRating() - pClient->GetSecurityRating()) / 100));
        double penalty = 0.025f * m_system->GetSystemSecurityRating() * modifier;
        double loss = penalty * ( pClient->GetSecurityRating() + 10);
            loss *= sConfig.rates.secRate;
            pClient->GetChar()->secStatusChange( -loss );
        }

    /* populate kill data for killMail and save to db  -allan 01May16  --updated 13July17 */
    KillData data = KillData();
        data.solarSystemID = m_system->GetID();
        data.victimCharacterID = victimCharID;
        data.victimCorporationID = victimCorpID;
        data.victimAllianceID = m_allyID;
        data.victimFactionID = m_warID;
        data.victimShipTypeID = m_self->typeID();

        data.finalCharacterID = killerID;
        // When the killing blow came from a fighter/drone, EVE reports the PILOT's
        // ship as the attacker and the drone as the weapon. The corp/alliance are
        // likewise the pilot's, not the drone's (drones carry no corp).
        uint16 finalShipTypeID = killer->GetTypeID();
        int32 finalCorpID = killer->GetCorporationID();
        int32 finalAllyID = killer->GetAllianceID();
        if (pClient != nullptr) {
            finalCorpID = pClient->GetCorporationID();
            finalAllyID = pClient->GetAllianceID();
            ShipSE* kShip = pClient->GetShipSE();
            if (kShip != nullptr)
                finalShipTypeID = kShip->GetTypeID();
        }
        data.finalCorporationID = finalCorpID;
        data.finalAllianceID = finalAllyID;
        data.finalFactionID = (pClient != nullptr ? pClient->GetWarFactionID()
                              : (killer->GetWarFactionID() > 500021 ? 500021 : killer->GetWarFactionID()));
        data.finalShipTypeID = finalShipTypeID;
        data.finalWeaponTypeID = fatal_blow.weaponRef->typeID();
        data.finalSecurityStatus = (pClient != nullptr) ? pClient->GetSecurityRating() : 0.0;
        data.finalDamageDone = fatal_blow.GetTotal();

        uint32 totalHP = m_self->GetAttribute(AttrHP).get_int();
            totalHP += m_self->GetAttribute(AttrArmorHP).get_int();
            totalHP += m_self->GetAttribute(AttrShieldCapacity).get_int();
        data.victimDamageTaken = totalHP;

    std::stringstream blob;
    std::vector<InventoryItemRef> survivedItems;
    if (pPilot != nullptr && pPilot->InPod()) {
        blob << "<items><i t=" << data.victimShipTypeID << " f=0 s=1 d=0 x=1/></items>";
    } else {
        AbortCycle();
        AbandonDrones();

        // remove all charges (per packet data)  ...why???
        //GetShipItemRef()->UnloadAllModules();

        blob << "<items>";
        /* killBlob contains destroyed/dropped items. u'<items><i t=3651 f=0 d=0 x=1/><i t=3634 f=0 d=0 x=1/></items>'  -allan 13July17
            " i*" tag is decoded as follows:
                t = item.typeID
                f = item.flag
                s = item.singleton
                d = item.qtyDropped
                x = item.qtyDestroyed
        */

        // Enumerate the ship's contents (fitted modules + cargo + drones) ONCE from
        // the in-memory inventory. The pilot sits on the ship at flagPilot — skip it
        // (the pilot is a character, never loot). Rigs/subsystems do not survive.
        // Each remaining stack rolls a 50% drop chance: "dropped" items (d>0) are
        // written to the killBlob AND physically moved to the wreck; "destroyed"
        // items only appear in the killBlob.
        std::map<uint32, InventoryItemRef> deadShipInventory;
        deadShipInventory.clear();
        GetShipItemRef()->GetMyInventory()->GetInventoryMap(deadShipInventory);
        bool anyItem = false;
        for (auto cur : deadShipInventory) {
            EVEItemFlags flag = cur.second->flag();
            if (flag == flagPilot)
                continue;                          // pilot is not an item
            anyItem = true;
            uint32 typeID = cur.second->typeID();
            uint32 qty = cur.second->quantity();
            uint32 singleton = (cur.second->isSingleton() ? 1 : 0);
            if (cur.second->categoryID() == EVEDB::invCategories::Blueprint) {
                BlueprintRef bpRef = BlueprintRef::StaticCast(cur.second);
                singleton = (bpRef->copy() ? 2 : singleton);
            }

            uint32 d = 0, x = qty;
            if (IsRigSlot(flag) || IsSubSystem(flag)) {
                // rigs/subsystems destroyed
            } else if (IsEven(MakeRandomInt(0, 100))) {
                // item survived — the whole stack (or part of it) drops to the wreck
                if (qty > 1) {
                    d = MakeRandomInt(0, qty);
                    x = qty - d;
                    if (d == 0)
                        continue;                  // nothing dropped — destroyed entirely
                } else {
                    d = 1; x = 0;                  // single item dropped intact
                }
                survivedItems.push_back(cur.second);
            }

            blob << "<i t=" << typeID << " f=" << flag << " q=" << qty << " s=" << singleton << " d=" << d << " x=" << x << "/>";
        }

        if (!anyItem) {
            // fallback if nothing found
            blob << "<i t=" << data.victimShipTypeID << " f=0 q=1 s=1 d=0 x=1/>";
        }
        blob << "</items>";
    }

    data.killBlob = blob.str().c_str();
    data.killTime = GetFileTimeNow();
    data.moonID = m_system->GetID();

    // save kill to DB (works for both real players and bots)
    ServiceDB::SaveKillOrLoss(data);

    // kill rights and notifications only for real player victims
    if (pPilot != nullptr) {
        // consume active kill rights against the victim
        {
            DBQueryResult krRes;
            if (sDatabase.RunQuery(krRes,
                " SELECT rightID FROM chrKillRights "
                " WHERE targetID = %u AND used = 0 AND expiryDate > %lli",
                victimCharID, static_cast<int64>(GetFileTimeNow())))
            {
                DBResultRow krRow;
                while (krRes.GetRow(krRow)) {
                    DBerror err;
                    sDatabase.RunQuery(err,
                        " UPDATE chrKillRights SET used = 1, activatedBy = %u WHERE rightID = %u",
                        killerID, krRow.GetInt(0));
                }
            }
        }

        // send killmail notification
        {
            const char* victimName = pPilot->GetName();
            const char* victimShip = sDataMgr.GetTypeName(m_self->typeID());
            const char* killerShip = sDataMgr.GetTypeName(killer->GetTypeID());
            const char* weaponName = sDataMgr.GetTypeName(data.finalWeaponTypeID);
            std::string victimCorp = sDataMgr.GetOwnerName(victimCorpID);
            std::string victimAlly = m_allyID ? sDataMgr.GetOwnerName(m_allyID) : "";
            std::string killerCorp = sDataMgr.GetOwnerName(killer->GetCorporationID());
            std::string killerName = (pClient != nullptr) ? pClient->GetName() : sDataMgr.GetOwnerName(killerID);

            // build items list from killBlob
            std::string itemsList;
            std::string killBlobStr(data.killBlob);
            size_t pos = 0;
            while ((pos = killBlobStr.find("<i ", pos)) != std::string::npos) {
                size_t end = killBlobStr.find("/>", pos);
                if (end == std::string::npos) break;
                std::string item = killBlobStr.substr(pos + 3, end - pos - 3);
                size_t tPos = item.find("t=");
                size_t qPos = item.find("q=");
                if (tPos != std::string::npos) {
                    uint32 typeID = std::stoul(item.substr(tPos + 2));
                    uint32 qty = 1;
                    if (qPos != std::string::npos) qty = std::stoul(item.substr(qPos + 2));
                    std::string typeName = sDataMgr.GetTypeName(typeID);
                    if (!typeName.empty() && typeName != "None")
                        itemsList += typeName + " x " + std::to_string(qty) + "\n";
                }
                pos = end + 2;
            }

            // EVE-style killmail format
            std::string secStr = std::to_string(m_system->GetSystemSecurityRating());
            secStr = secStr.substr(0, secStr.find('.') + 2);

            std::string kmBody;
            kmBody += "Victim: " + std::string(victimName) + ", Corp: " + victimCorp + "\n";
            if (!victimAlly.empty() && victimAlly != "None")
                kmBody += "Alliance: " + victimAlly + "\n";
            kmBody += "System: " + std::string(m_system->GetName()) + " (" + secStr + ")\n";
            kmBody += "Damage Taken: " + std::to_string(data.victimDamageTaken) + "\n\n";
            kmBody += "Final Blow: " + killerName + " flying " + killerShip + "\n";
            kmBody += "Corp: " + killerCorp + "\n";
            kmBody += "Damage Done: " + std::to_string(data.finalDamageDone) + "\n\n";
            kmBody += "Details:\n" + itemsList;

            // notify killer
            if (pClient != nullptr) {
                pClient->SendNotifyMsg("Kill: %s (%s) - %s (%s) - %u damage",
                    victimName, victimShip, pClient->GetName(), killerShip, data.victimDamageTaken);
                pClient->SelfEveMail("Kill Report", kmBody.c_str());
            }

            // notify victim
            pPilot->SendNotifyMsg("You were destroyed by %s (%s) with %s - %u damage",
                killerName.c_str(), killerShip, weaponName, data.victimDamageTaken);
            pPilot->SelfEveMail("Loss Report", kmBody.c_str());
        }
    } else if (pBot != nullptr) {
        // bot victim — notify killer only
        const char* victimName = pBot->GetName();
        const char* victimShip = sDataMgr.GetTypeName(m_self->typeID());
        const char* killerShip = sDataMgr.GetTypeName(killer->GetTypeID());
        std::string killerCorp = sDataMgr.GetOwnerName(killer->GetCorporationID());
        std::string killerName = (pClient != nullptr) ? pClient->GetName() : sDataMgr.GetOwnerName(killerID);

        // build items list from killBlob
        std::string itemsList;
        std::string killBlobStr(data.killBlob);
        size_t pos = 0;
        while ((pos = killBlobStr.find("<i ", pos)) != std::string::npos) {
            size_t end = killBlobStr.find("/>", pos);
            if (end == std::string::npos) break;
            std::string item = killBlobStr.substr(pos + 3, end - pos - 3);
            size_t tPos = item.find("t=");
            size_t qPos = item.find("q=");
            if (tPos != std::string::npos) {
                uint32 typeID = std::stoul(item.substr(tPos + 2));
                uint32 qty = 1;
                if (qPos != std::string::npos) qty = std::stoul(item.substr(qPos + 2));
                std::string typeName = sDataMgr.GetTypeName(typeID);
                if (!typeName.empty() && typeName != "None")
                    itemsList += typeName + " x " + std::to_string(qty) + "\n";
            }
            pos = end + 2;
        }

        std::string secStr = std::to_string(m_system->GetSystemSecurityRating());
        secStr = secStr.substr(0, secStr.find('.') + 2);

        std::string kmBody;
        kmBody += "Victim: " + std::string(victimName) + ", Corp: " + sDataMgr.GetOwnerName(pBot->GetCorporationID()) + "\n";
        kmBody += "System: " + std::string(m_system->GetName()) + " (" + secStr + ")\n";
        kmBody += "Damage Taken: " + std::to_string(data.victimDamageTaken) + "\n\n";
        kmBody += "Final Blow: " + killerName + " flying " + killerShip + "\n";
        kmBody += "Corp: " + killerCorp + "\n";
        kmBody += "Damage Done: " + std::to_string(data.finalDamageDone) + "\n\n";
        kmBody += "Details:\n" + itemsList;

        if (pClient != nullptr) {
            pClient->SendNotifyMsg("Kill: %s (%s) - %s (%s) - %u damage",
                victimName, victimShip, pClient->GetName(), killerShip, data.victimDamageTaken);
            pClient->SelfEveMail("Kill Report", kmBody.c_str());
        }
    }



    if (pPilot != nullptr && pPilot->InPod()) {
        // log podKill
        MapDB::AddPodKill(locationID);

        if (pClient != nullptr)
            pClient->GetChar()->PayBounty(pPilot->GetChar());

        std::string corpse_name = pPilot->GetName();
        corpse_name += "'s Frozen Corpse";
        uint32 corpseTypeID = 10041; // typeID from 'invTypes' table for "Frozen Corpse"
        ItemData corpseItemData(corpseTypeID, m_ownerID, locationID, flagNone, corpse_name.c_str(), wreckPosition);
        InventoryItemRef corpseItemRef = sItemFactory.SpawnItem( corpseItemData );
        if (corpseItemRef.get() == nullptr) {
            sLog.Error("Ship::Killed()", "Creating Corpse Item Failed for %s of type %u", corpse_name.c_str(), corpseTypeID);
            DBSystemDynamicEntity corpseEntity = DBSystemDynamicEntity();
            corpseEntity.allianceID = m_allyID;
            corpseEntity.categoryID = EVEDB::invCategories::Celestial;
            corpseEntity.corporationID = m_corpID;
            corpseEntity.factionID = m_warID;
            corpseEntity.groupID = EVEDB::invGroups::Biomass;
            corpseEntity.itemID = corpseItemRef->itemID();
            corpseEntity.itemName = corpse_name;
            corpseEntity.ownerID = 1;   //would this be 'owned' by killer?
            corpseEntity.typeID = corpseTypeID;
            corpseEntity.position = wreckPosition;
            if (!m_system->BuildDynamicEntity(corpseEntity)) {
                sLog.Error("Ship::Killed()", "Spawning Corpse Failed: typeID or typeName not supported: '%u'", corpseTypeID);
            } else if (is_log_enabled(PHYSICS__TRACE)) {
                _log(PHYSICS__TRACE, "Ship::Killed() - Pod %s(%u) Position: %.2f,%.2f,%.2f.  Corpse %s(%u) Position: %.2f,%.2f,%.2f.", \
                    GetName(), GetID(), x(), y(), z(), corpseItemRef->name(), corpseItemRef->itemID(), wreckPosition.x, wreckPosition.y, wreckPosition.z);
            }
        }

        // T3 ship hull loss — inflict SP loss on strategic cruiser skill (5% per pod kill)
        if (m_self->groupID() == EVEDB::invGroups::StrategicCruiser) {
            CharacterRef cRef = pPilot->GetChar();
            uint32 t3Skills[] = {
                EvESkill::AmarrStrategicCruiser, EvESkill::CaldariStrategicCruiser,
                EvESkill::GallenteStrategicCruiser, EvESkill::MinmatarStrategicCruiser
            };
            for (uint32 skillID : t3Skills) {
                SkillRef skillRef = cRef->GetCharSkillRef(skillID);
                if (skillRef.get() != nullptr) {
                    uint32 sp = skillRef->GetCurrentSP(cRef.get());
                    if (sp > 500) {
                        sp = std::max<uint32>(500, sp - (sp / 20));
                        skillRef->SetAttribute(AttrSkillPoints, sp, true);
                    }
                }
            }
        }

        // this method will reset char variables to last clone state after being podded.  NOTE  *** NOT TESTED YET ***
        pPilot->ResetAfterPodded();
    } else {
        PayInsurance();

        m_destiny->SendJettisonPacket();

        uint16 groupID = m_self->groupID();
        GPoint podPosition(wreckPosition);
        podPosition.MakeRandomPointOnSphere(GetShipItemRef()->radius() + pPilot->GetPod()->radius() + MakeRandomFloat(100, 200));
        // this resets client ship data
        pPilot->ResetAfterPopped(podPosition);

        ItemData wreckItemData(wreckTypeID, pPilot->GetCharacterID(), locationID, flagNone, wreck_name.c_str(), wreckPosition, itoa(m_allyID));
        WreckContainerRef wreckItemRef = sItemFactory.SpawnWreckContainer( wreckItemData );
        if (wreckItemRef.get() == nullptr) {
            sLog.Error("Ship::Killed()", "Creating Wreck Item Failed for %s of type %u", wreck_name.c_str(), wreckTypeID);
            return;
        }

        if (is_log_enabled(PHYSICS__TRACE))
            _log(PHYSICS__TRACE, "Ship::Killed() - Ship %s(%u) Position: %.2f,%.2f,%.2f.  Wreck %s(%u) Position: %.2f,%.2f,%.2f.", \
            GetName(), GetID(), x(), y(), z(), wreckItemRef->name(), wreckItemRef->itemID(), wreckPosition.x, wreckPosition.y, wreckPosition.z);

        DBSystemDynamicEntity wreckEntity = DBSystemDynamicEntity();
            wreckEntity.allianceID = killer->GetAllianceID();
            wreckEntity.categoryID = EVEDB::invCategories::Celestial;
            wreckEntity.corporationID = killer->GetCorporationID();
            wreckEntity.factionID = sDataMgr.GetWreckFaction(wreckTypeID);
            wreckEntity.groupID = EVEDB::invGroups::Wreck;
            wreckEntity.itemID = wreckItemRef->itemID();
            wreckEntity.itemName = wreck_name;
            wreckEntity.ownerID = pPilot->GetCharacterID();
            wreckEntity.typeID = wreckTypeID;
            wreckEntity.position = wreckPosition;

        if (!m_system->BuildDynamicEntity(wreckEntity, m_self->itemID())) {
            sLog.Error("Ship::Killed()", "Spawning Wreck Failed for typeID %u", wreckTypeID);
            wreckItemRef->Delete();
            return;
        }

        DropLoot(wreckItemRef, groupID, killerID);

        for (auto cur: survivedItems)
            cur->Move(wreckItemRef->itemID(), flagNone); // populate wreck with items that survived
    }
}
