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
    Updates:        Allan
*/


#include "eve-server.h"

#include "Client.h"
#include "EntityList.h"
#include "map/MapDB.h"
#include "npc/NPC.h"
#include "npc/ConvoyAI.h"
#include "npc/NPCAI.h"
#include "system/Container.h"
#include "system/Damage.h"
#include "system/SystemManager.h"
#include "standing/StandingMgr.h"


NPC::NPC(InventoryItemRef self, EVEServiceManager& services, SystemManager* system, const FactionData& data, SpawnMgr* spawnMgr)
: DynamicSystemEntity(self, services, system),
m_spawnMgr(spawnMgr),
m_convoyAI(nullptr),
m_AI(new NPCAIMgr(this))
{
    m_allyID = data.allianceID;
    m_warID = data.factionID;
    m_corpID = data.corporationID;
    m_ownerID = data.ownerID;

    // Create default dynamic attributes in the AttributeMap:
    m_self->SetAttribute(AttrInetia,              1.0f, false);
    m_self->SetAttribute(AttrDamage,              EvilZero, false);
    m_self->SetAttribute(AttrArmorDamage,         EvilZero, false);
    m_self->SetAttribute(AttrWarpCapacitorNeed,   0.00001, false);
    m_self->SetAttribute(AttrMass,                m_self->type().mass(), false);
    m_self->SetAttribute(AttrRadius,              m_self->type().radius(), false);
    m_self->SetAttribute(AttrVolume,              m_self->type().volume(), false);
    m_self->SetAttribute(AttrCapacity,            m_self->type().capacity(), false);
    m_self->SetAttribute(AttrShieldCapacity,      10000.0f, false);
    m_self->SetAttribute(AttrShieldCharge,        10000.0f, false);
    m_self->SetAttribute(AttrCapacitorCharge,     m_self->GetAttribute(AttrCapacitorCapacity), false);
    m_self->SetAttribute(AttrWarpSpeedMultiplier, 1.0f, false);
    m_self->SetAttribute(AttrEntityCruiseSpeed,  150.0f, false);
    m_self->SetAttribute(AttrShieldRechargeRate, 500000.0f, false);    // ms for full recharge (500s = slow regen)
    m_self->SetAttribute(AttrArmorHP,             10000.0f, false);
    m_self->SetAttribute(AttrHP,                  10000.0f, false);
    // Set default combat attributes if DB doesn't have them
    uint32 gid = m_self->groupID();
    if (gid == EVEDB::invGroups::Convoy_Drone) {
        // Convoy guard: frigate-level (Condor/Kestrel types)
        if (!m_self->HasAttribute(AttrEmDamage))
            m_self->SetAttribute(AttrEmDamage,        5.0f, false);
        if (!m_self->HasAttribute(AttrKineticDamage))
            m_self->SetAttribute(AttrKineticDamage,   8.0f, false);
        if (!m_self->HasAttribute(AttrThermalDamage))
            m_self->SetAttribute(AttrThermalDamage,   4.0f, false);
        if (!m_self->HasAttribute(AttrExplosiveDamage))
            m_self->SetAttribute(AttrExplosiveDamage, 2.0f, false);
        if (!m_self->HasAttribute(AttrDamageMultiplier))
            m_self->SetAttribute(AttrDamageMultiplier, 1.0f, false);
        if (!m_self->HasAttribute(AttrSpeed))
            m_self->SetAttribute(AttrSpeed,           5000.0f, false);
        if (!m_self->HasAttribute(AttrMaxVelocity))
            m_self->SetAttribute(AttrMaxVelocity,     450.0f, false);
        if (!m_self->HasAttribute(AttrEntityFlyRange))
            m_self->SetAttribute(AttrEntityFlyRange,  15000.0f, false);
        if (!m_self->HasAttribute(AttrMaxRange))
            m_self->SetAttribute(AttrMaxRange,        15000.0f, false);
        if (!m_self->HasAttribute(AttrFalloff))
            m_self->SetAttribute(AttrFalloff,         5000.0f, false);
        if (!m_self->HasAttribute(AttrTrackingSpeed))
            m_self->SetAttribute(AttrTrackingSpeed,   0.1f, false);
        if (!m_self->HasAttribute(AttrOptimalSigRadius))
            m_self->SetAttribute(AttrOptimalSigRadius, 40.0f, false);
        if (!m_self->HasAttribute(AttrSignatureRadius))
            m_self->SetAttribute(AttrSignatureRadius, 50.0f, false);
    } else if (gid == EVEDB::invGroups::Convoy || gid == EVEDB::invGroups::Mission_Faction_Industrials) {
        // Convoy hauler: industrial-level (weak weapons)
        if (!m_self->HasAttribute(AttrEmDamage))
            m_self->SetAttribute(AttrEmDamage,        2.0f, false);
        if (!m_self->HasAttribute(AttrKineticDamage))
            m_self->SetAttribute(AttrKineticDamage,   3.0f, false);
        if (!m_self->HasAttribute(AttrThermalDamage))
            m_self->SetAttribute(AttrThermalDamage,   2.0f, false);
        if (!m_self->HasAttribute(AttrExplosiveDamage))
            m_self->SetAttribute(AttrExplosiveDamage, 1.0f, false);
        if (!m_self->HasAttribute(AttrDamageMultiplier))
            m_self->SetAttribute(AttrDamageMultiplier, 0.5f, false);
        if (!m_self->HasAttribute(AttrSpeed))
            m_self->SetAttribute(AttrSpeed,           8000.0f, false);
        if (!m_self->HasAttribute(AttrMaxVelocity))
            m_self->SetAttribute(AttrMaxVelocity,     400.0f, false);
        if (!m_self->HasAttribute(AttrEntityFlyRange))
            m_self->SetAttribute(AttrEntityFlyRange,  10000.0f, false);
        if (!m_self->HasAttribute(AttrMaxRange))
            m_self->SetAttribute(AttrMaxRange,        10000.0f, false);
        if (!m_self->HasAttribute(AttrFalloff))
            m_self->SetAttribute(AttrFalloff,         3000.0f, false);
        if (!m_self->HasAttribute(AttrTrackingSpeed))
            m_self->SetAttribute(AttrTrackingSpeed,   0.05f, false);
        if (!m_self->HasAttribute(AttrOptimalSigRadius))
            m_self->SetAttribute(AttrOptimalSigRadius, 40.0f, false);
        if (!m_self->HasAttribute(AttrSignatureRadius))
            m_self->SetAttribute(AttrSignatureRadius, 100.0f, false);
    }

    /* Gets the value from the NPC and put on our own vars */
    m_emDamage = m_self->GetAttribute(AttrEmDamage).get_float(),
    m_kinDamage = m_self->GetAttribute(AttrKineticDamage).get_float(),
    m_therDamage = m_self->GetAttribute(AttrThermalDamage).get_float(),
    m_expDamage = m_self->GetAttribute(AttrExplosiveDamage).get_float(),
    m_hullDamage = m_self->GetAttribute(AttrDamage).get_float();
    m_armorDamage = m_self->GetAttribute(AttrArmorDamage).get_float();
    m_shieldCharge = m_self->GetAttribute(AttrShieldCharge).get_float();
    m_shieldCapacity = m_self->GetAttribute(AttrShieldCapacity).get_float();

    _log(NPC__TRACE, "Created NPC object for %s (%u) - Data: O:%u, C:%u, A:%u, W:%u", \
            m_self.get()->name(), m_self.get()->itemID(), \
            m_ownerID, m_corpID, m_allyID, m_warID);
}

void NPC::OnAttacked(SystemEntity* attacker)
{
    if (m_killed || attacker == nullptr)
        return;

    // Notify convoy group of attack (triggers defensive AI)
    if (m_convoyAI != nullptr)
        m_convoyAI->NotifyAttacked();

    // Distress call — broadcast to all players in system
    if (m_system != nullptr && attacker->HasPilot()) {
        std::vector<Client*> clients;
        m_system->GetClientList(clients);
        for (Client* c : clients) {
            c->SendNotifyMsg("CONVOY ALERT: %s in %s is under attack by %s!",
                             GetName(), m_system->GetNameStr().c_str(), attacker->GetPilot()->GetName());
        }
    }
}

NPC::~NPC() {
    SafeDelete(m_AI);
    SafeDelete(m_convoyAI);
}

bool NPC::IsConvoy() const
{
    uint32 gid = m_self->groupID();
    return gid == EVEDB::invGroups::Convoy || gid == EVEDB::invGroups::Convoy_Drone
        || gid == EVEDB::invGroups::Mission_Faction_Industrials;
}

bool NPC::IsConvoyUnderAttack() const
{
    return m_convoyAI != nullptr && m_convoyAI->IsGroupUnderAttack();
}

PyDict* NPC::MakeSlimItem()
{
    PyDict* slim = new PyDict();
    slim->SetItemString("itemID",          new PyLong(m_self->itemID()));
    slim->SetItemString("typeID",          new PyInt(m_self->typeID()));
    slim->SetItemString("name",            new PyString(m_self->itemName()));
    slim->SetItemString("ownerID",         new PyInt(m_ownerID));
    slim->SetItemString("corpID",          IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
    slim->SetItemString("allianceID",      IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
    slim->SetItemString("warFactionID",    IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
    slim->SetItemString("categoryID",      new PyInt(m_self->categoryID()));
    slim->SetItemString("groupID",         new PyInt(m_self->groupID()));
    return slim;
}

bool NPC::Load()
{
    m_destiny->UpdateShipVariables();

    SetResists();

    return DynamicSystemEntity::Load();
}


void NPC::Process() {
    if (m_killed)
        return;

    double profileStartTime(GetTimeUSeconds());

    /*  Enable base call to Process Targeting and Movement  */
    SystemEntity::Process();

    m_AI->Process();
    if (m_convoyAI != nullptr) m_convoyAI->Process();

    if (sConfig.debug.UseProfiling)
        sProfiler.AddTime(Profile::npc, GetTimeUSeconds() - profileStartTime);
}

void NPC::Orbit(SystemEntity *who) {
    if (who == nullptr) {
        m_orbitingID = 0;
    } else {
        m_orbitingID = who->GetID();
    }
}

void NPC::TargetLost(SystemEntity *who) {
    m_AI->TargetLost(who);
}

void NPC::TargetedAdd(SystemEntity *who) {
    m_AI->Targeted(who);
}

void NPC::EncodeDestiny( Buffer& into )
{
    // Custom EncodeDestiny: uses GetState() for mode (unlike DSE which always hardcodes STOP)
    // but only flags = IsFree (0x01) without IsMassive, matching DSE's working binary format.
    using namespace Destiny;

    uint8 mode = m_destiny->GetState();

    BallHeader head = BallHeader();
        head.entityID = GetID();
        head.mode = mode;
        head.radius = GetRadius();
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsFree;
    into.Append( head );
    MassSector mass = MassSector();
        mass.mass = m_destiny->GetMass();
        mass.cloak = (m_destiny->IsCloaked() ? 1 : 0);
        mass.harmonic = m_harmonic;
        mass.corporationID = m_corpID;
        mass.allianceID = (IsAlliance(m_allyID) ? m_allyID : -1);
    into.Append( mass );
    DataSector data = DataSector();
        data.maxSpeed = m_destiny->GetMaxVelocity();
        data.velX = m_destiny->GetVelocity().x;
        data.velY = m_destiny->GetVelocity().y;
        data.velZ = m_destiny->GetVelocity().z;
        data.inertia = m_destiny->GetInertia();
        data.speedfraction = m_destiny->GetSpeedFraction();
    into.Append( data );
    switch (mode) {
        case Ball::Mode::WARP: {
            GPoint target = m_destiny->GetTargetPoint();
            WARP_Struct warp;
                warp.formationID = 0xFF;
                warp.targX = target.x;
                warp.targY = target.y;
                warp.targZ = target.z;
                warp.speed = m_destiny->GetWarpSpeed();
                warp.effectStamp = -1;
                warp.followRange = 0;
                warp.followID = 0;
            into.Append( warp );
        }  break;
        case Ball::Mode::FOLLOW: {
            FOLLOW_Struct follow;
                follow.followID = m_destiny->GetTargetID();
                follow.followRange = m_destiny->GetFollowDistance();
                follow.formationID = 0xFF;
            into.Append( follow );
        }  break;
        case Ball::Mode::ORBIT: {
            ORBIT_Struct orbit;
                orbit.targetID = m_destiny->GetTargetID();
                orbit.followRange = m_destiny->GetFollowDistance();
                orbit.formationID = 0xFF;
            into.Append( orbit );
        }  break;
        case Ball::Mode::GOTO: {
            GPoint target = m_destiny->GetTargetPoint();
            GOTO_Struct go;
                go.formationID = 0xFF;
                go.x = target.x;
                go.y = target.y;
                go.z = target.z;
            into.Append( go );
        }  break;
        default: {
            STOP_Struct main;
                main.formationID = 0xFF;
            into.Append( main );
        } break;
    }
}

void NPC::UseShieldRecharge()
{
    // We recharge our shield until it's full.
    if (m_self->GetAttribute(AttrShieldCapacity) > m_shieldCharge) {
        m_shieldCharge += m_self->GetAttribute(AttrEntityShieldBoostAmount).get_float();
        if (m_shieldCharge > m_self->GetAttribute(AttrShieldCapacity).get_float())
            m_shieldCharge = m_self->GetAttribute(AttrShieldCapacity).get_float();
        m_self->SetAttribute(AttrShieldCharge, m_shieldCharge);
    } else {
        m_AI->DisableRepTimers(true, false);
    }

    uint32 gfxID = m_self->HasAttribute(AttrGfxBoosterID) ? m_self->GetAttribute(AttrGfxBoosterID).get_uint32() : 0;
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                 m_self->itemID(), 0, "effects.ShieldBoosting",
                                 0, 1, 1, m_AI->GetShieldBoosterDuration(), 0, gfxID);
    SendDamageStateChanged();
}

void NPC::UseArmorRepairer()
{
    if (m_armorDamage > 0) {
        m_armorDamage -= m_self->GetAttribute(AttrEntityArmorRepairAmount).get_float();
        if (m_armorDamage < 0.0)
            m_armorDamage = 0.0;
        m_self->SetAttribute(AttrArmorDamage, m_armorDamage);
    } else {
        m_AI->DisableRepTimers(false, true);
    }

    uint32 gfxID = m_self->HasAttribute(AttrGfxBoosterID) ? m_self->GetAttribute(AttrGfxBoosterID).get_uint32() : 0;
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                 m_self->itemID(), 0, "effects.ArmorRepair",
                                 0, 1, 1, m_AI->GetArmorRepairDuration(), 0, gfxID);
    SendDamageStateChanged();
}

void NPC::UseHullRepairer()
{
    if (m_hullDamage > 0) {
        //m_hullDamage -= m_self->GetAttribute(AttrEntityhullRepairAmount).get_float();  << NSA - create later
        if (m_hullDamage < 0.0)
            m_hullDamage = 0.0;
        m_self->SetAttribute(AttrDamage, m_hullDamage);
    } else {
        m_AI->DisableRepTimers(false, false);
    }

    uint32 gfxID = m_self->HasAttribute(AttrGfxBoosterID) ? m_self->GetAttribute(AttrGfxBoosterID).get_uint32() : 0;
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
                                 m_self->itemID(), 0, "effects.ArmorRepair",
                                 0, 1, 1, m_AI->GetArmorRepairDuration(), 0, gfxID);
    SendDamageStateChanged();
}

void NPC::MissileLaunched(Missile* pMissile)
{
    m_AI->MissileLaunched(pMissile);
}

void NPC::SaveNPC()
{
    m_self->SaveItem();
}

void NPC::RemoveNPC()
{
    //this is called from SystemManager::RemoveNPC() which calls other SE* methods as needed
    m_self->Delete();
}

void NPC::SetResists() {
    /* fix for missing resist attribs -allan 18April16  */
    // Shield Resonance
    if (!m_self->HasAttribute(AttrShieldEmDamageResonance)) m_self->SetAttribute(AttrShieldEmDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrShieldExplosiveDamageResonance)) m_self->SetAttribute(AttrShieldExplosiveDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrShieldKineticDamageResonance)) m_self->SetAttribute(AttrShieldKineticDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrShieldThermalDamageResonance)) m_self->SetAttribute(AttrShieldThermalDamageResonance, EvilOne, false);
    // Armor Resonance
    if (!m_self->HasAttribute(AttrArmorEmDamageResonance)) m_self->SetAttribute(AttrArmorEmDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrArmorExplosiveDamageResonance)) m_self->SetAttribute(AttrArmorExplosiveDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrArmorKineticDamageResonance)) m_self->SetAttribute(AttrArmorKineticDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrArmorThermalDamageResonance)) m_self->SetAttribute(AttrArmorThermalDamageResonance, EvilOne, false);
    // Hull Resonance
    if (!m_self->HasAttribute(AttrEmDamageResonance)) m_self->SetAttribute(AttrEmDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrExplosiveDamageResonance)) m_self->SetAttribute(AttrExplosiveDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrKineticDamageResonance)) m_self->SetAttribute(AttrKineticDamageResonance, EvilOne, false);
    if (!m_self->HasAttribute(AttrThermalDamageResonance)) m_self->SetAttribute(AttrThermalDamageResonance, EvilOne, false);
}

void NPC::Killed(Damage &damage) {
    if ((m_bubble == nullptr) or (m_destiny == nullptr) or (m_system == nullptr))
        return; // make error here?

    //notify our spawn manager that we are gone.
    if ((m_spawnMgr != nullptr) and (m_self.get() != nullptr))
        m_spawnMgr->SpawnKilled(m_bubble, m_self->itemID());

    uint32 killerID = 0;
    Client* pClient(nullptr);
    SystemEntity *killer(damage.srcSE);

    if (killer->HasPilot()) {
        pClient = killer->GetPilot();
        killerID = pClient->GetCharacterID();
    } else if (killer->IsDroneSE()) {
        pClient = sEntityList.FindClientByCharID( killer->GetSelf()->ownerID() );
        if (pClient == nullptr) {
            sLog.Error("NPC::Killed()", "killer == IsDrone and pPlayer == nullptr");
        } else {
            killerID = pClient->GetCharacterID();
        }
    } else {
        killerID = killer->GetID();
    }

    uint32 locationID = GetLocationID();
    //  log faction kill in dynamic data   -allan
    MapDB::AddKill(locationID);
    MapDB::AddFactionKill(locationID);

    if (pClient != nullptr) {
        //award kill bounty.
        AwardBounty( pClient );
        if (m_system->GetSystemSecurityRating() > 0)
            AwardSecurityStatus(m_self, pClient->GetChar().get());  // this awards secStatusChange for npcs in empire space

        // Faction standing changes on NPC kill
        if (m_warID > 0) {
            // Standing decrease with this NPC's faction
            float penalty = -0.0005f;
            sStandingMgr.UpdateStandings(m_warID, pClient->GetCharacterID(),
                                         Standings::CombatShipKill, penalty,
                                         "NPC kill - faction penalty");
            // Standing increase with enemy factions
            std::vector<int32> enemies = StandingDB::GetEnemyFactions(m_warID);
            for (int32 enemyID : enemies) {
                float reward = 0.0003f;
                sStandingMgr.UpdateStandings(enemyID, pClient->GetCharacterID(),
                                             Standings::CombatShipKill, reward,
                                             "NPC kill - enemy faction reward");
            }
        }
    }

    GPoint wreckPosition = m_destiny->GetPosition();
    if (wreckPosition.isNaN()) {
        sLog.Error("NPC::Killed()", "Wreck Position is NaN");
        return;
    }
    uint32 wreckTypeID = sDataMgr.GetWreckID(m_self->typeID());
    if (!IsWreckTypeID(wreckTypeID)) {
        sLog.Error("NPC::Killed()", "Could not get wreckType for %s of type %u", m_self->name(), m_self->typeID());
        // default to generic frigate wreck till i get better checks and/or complete wreck data
        wreckTypeID = 26557;
    }

    std::string wreck_name = m_self->itemName();
    wreck_name += " Wreck";
    ItemData wreckItemData(wreckTypeID, killerID, locationID, flagNone, wreck_name.c_str(), wreckPosition, itoa(m_allyID));
    WreckContainerRef wreckItemRef = sItemFactory.SpawnWreckContainer( wreckItemData );
    if (wreckItemRef.get() == nullptr) {
        sLog.Error("NPC::Killed()", "Creating Wreck Item Failed for %s of type %u", wreck_name.c_str(), wreckTypeID);
        return;
    }

    if (is_log_enabled(PHYSICS__TRACE))
        _log(PHYSICS__TRACE, "NPC::Killed() - NPC %s(%u) Position: %.2f,%.2f,%.2f.  Wreck %s(%u) Position: %.2f,%.2f,%.2f.", \
                GetName(), GetID(), x(), y(), z(), wreckItemRef->name(), wreckItemRef->itemID(), wreckPosition.x, wreckPosition.y, wreckPosition.z);

    if ((MakeRandomFloat() < sConfig.npc.LootDropChance) or (m_allyID == factionRogueDrones))
        DropLoot(wreckItemRef, m_self->groupID(), killerID);

    // Convoy-specific loot (always drops)
    if (IsConvoy()) {
        // Minerals: Tritanium, Pyerite, Mexallon, Isogen
        uint32 minMinerals[4] = { 34, 35, 36, 37 }; // typeIDs
        uint32 minQty = 100 + MakeRandomInt(0, 1000);
        for (int i = 0; i < 4; ++i) {
            ItemData iLoot(minMinerals[i], killerID, wreckItemRef->itemID(), flagNone, minQty);
            wreckItemRef->AddItem(sItemFactory.SpawnItem(iLoot));
        }
        // Random faction ammo or module
        if (MakeRandomFloat() < 0.3f) {
            uint32 modTypes[] = { 218, 219, 220, 221, 222 }; // various ammo
            uint32 modID = modTypes[MakeRandomInt(0, 4)];
            ItemData iLoot(modID, killerID, wreckItemRef->itemID(), flagNone, MakeRandomInt(1, 5));
            wreckItemRef->AddItem(sItemFactory.SpawnItem(iLoot));
        }
    }

    DBSystemDynamicEntity wreckEntity = DBSystemDynamicEntity();
        wreckEntity.allianceID = (killer->GetAllianceID() == 0 ? m_allyID : killer->GetAllianceID());
        wreckEntity.categoryID = EVEDB::invCategories::Celestial;
        wreckEntity.corporationID = killer->GetCorporationID();
        wreckEntity.factionID = (killer->GetWarFactionID() == 0 ? m_warID : killer->GetWarFactionID());
        wreckEntity.groupID = EVEDB::invGroups::Wreck;
        wreckEntity.itemID = wreckItemRef->itemID();
        wreckEntity.itemName = wreck_name;
        wreckEntity.ownerID = killerID;
        wreckEntity.typeID = wreckTypeID;
        wreckEntity.position = wreckPosition;

    if (!m_system->BuildDynamicEntity(wreckEntity, m_self->itemID())) {
        sLog.Error("NPC::Killed()", "Spawning Wreck Failed for typeID %u", wreckTypeID);
        wreckItemRef->Delete();
        return;
    }
    m_destiny->SendJettisonPacket();
}

void NPC::CmdDropLoot()
{
    std::ostringstream name;
    name << m_self->itemName() << "(" << m_self->itemID() << ")  Loot Container";
    // create new container
    ItemData p_idata(23,   // 23 = cargo container
                     ownerSystem,
                     locTemp,
                     flagNone,
                     name.str().c_str(),
                     GetPosition());

    CargoContainerRef jetCanRef = sItemFactory.SpawnCargoContainer(p_idata);

    if (jetCanRef.get() != nullptr) {
        FactionData data = FactionData();
        data.allianceID = m_allyID;
        data.corporationID = m_corpID;
        data.factionID = m_warID;
        data.ownerID = m_self->ownerID();
        ContainerSE* cSE = new ContainerSE(jetCanRef, GetServices(), m_system, data);
        jetCanRef->SetMySE(cSE);
        m_system->AddEntity(cSE);
        m_destiny->SendJettisonPacket();
        // this needs a wreckItemRef, but i dont feel like making one right now
        //DropLoot(jetCanRef, m_self->groupID());
    }
}
