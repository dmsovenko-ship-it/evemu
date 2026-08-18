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

#include "eve-server.h"

#include <cmath>
#include <cstdio>

#include "ConsoleCommands.h"
#include "Client.h"
#include "Container.h"
#include "EVEServerConfig.h"
#include "exploration/Probes.h"
#include "math/Trig.h"

#include "StatisticMgr.h"
#include "account/AccountService.h"
#include "character/Character.h"
#include "fleet/FleetService.h"
#include "inventory/AttributeEnum.h"
#include "planet/CustomsOffice.h"
#include "planet/Planet.h"
#include "pos/Structure.h"
#include "standing/StandingMgr.h"
#include "system/DestinyManager.h"
#include "station/Station.h"
#include "system/SystemBubble.h"
#include "incursion/IncursionMgr.h"
#include "system/SystemEntity.h"
#include "system/SystemManager.h"



SystemEntity::SystemEntity(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: m_self(self),
m_services(services),
m_system(system),
m_bubble(nullptr),
m_destiny(nullptr),
m_targMgr(nullptr),
m_killed(false)
{
    assert(m_system != nullptr);
    assert(m_self.get() != nullptr);

    m_warID = 0;
    m_allyID = 0;
    m_corpID = 0;
    m_fleetID = 0;
    m_ownerID = 1;

    m_radius = m_self->GetAttribute(AttrRadius).get_double();

    m_harmonic = EVEPOS::Harmonic::Inactive;

    _log(SE__DEBUG, "Created SE for item %s (%u) with radius of %.1f.", self->name(), self->itemID(), m_radius);
}

// copy c'tor
SystemEntity::SystemEntity(const SystemEntity* oth) : m_self(oth->m_self),m_services(oth->m_services),m_system(oth->m_system),
m_bubble(oth->m_bubble),m_destiny(oth->m_destiny),m_targMgr(oth->m_targMgr),m_killed(oth->m_killed),m_warID(oth->m_warID),
m_allyID(oth->m_allyID),m_corpID(oth->m_corpID),m_fleetID(oth->m_fleetID),m_ownerID(oth->m_ownerID),m_radius(oth->m_radius),
m_harmonic(oth->m_harmonic)
{
    sLog.Error("SE::SE()", "copy c'tor.");
    // wip
}


void SystemEntity::Process() {
    if (m_killed) {
        _log(SE__DEBUG, "SE::Process() - %s(%u) is dead but still in system.", m_self->name(), m_self->itemID());
        return;
    }

    if (m_destiny != nullptr)
        m_destiny->Process();
}

PyTuple* SystemEntity::MakeDamageState() {
    if (IsWreckSE()) {
        DoDestinyDamageState3 ddds;
            ddds.shield = 0;
            ddds.armor = 0;
            ddds.structure = 1.0;
        return ddds.Encode();
    }
    DoDestinyDamageState ddds;
    MakeDamageState(ddds);
    return ddds.Encode();
}

void SystemEntity::MakeDamageState(DoDestinyDamageState &into) {
    into.shield = 1;
    into.recharge = 110000;
    into.armor = 1;
    into.structure = 1;
    into.timestamp = GetFileTimeNow();
}

PyDict* SystemEntity::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for SE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("typeID",       new PyInt(m_self->typeID()));
        slim->SetItemString("ownerID",      new PyInt(m_ownerID));
        // categoryID/groupID are REQUIRED by the client's spaceObject factory to
        // pick the render class (spaceObject.LargeCollidableObject etc.). Without
        // them decor (Sleeper/Talocan structures, clouds) had no model and stayed
        // invisible even though the RIGID ball was delivered. Asteroids/wrecks
        // include these fields; ObjectSystemEntity::MakeSlimItem already does.
        slim->SetItemString("categoryID",   new PyInt(m_self->categoryID()));
        slim->SetItemString("groupID",      new PyInt(m_self->groupID()));
        slim->SetItemString("itemID",       new PyLong(m_self->itemID()));
        slim->SetItemString("name",         new PyString(m_self->itemName()));
        slim->SetItemString("corpID",       IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
        slim->SetItemString("allianceID",   IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
        slim->SetItemString("warFactionID", IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
        slim->SetItemString("charID",       PyStatic.NewNone());
    return slim;
}

void SystemEntity::EncodeDestiny( Buffer& into )
{
    using namespace Destiny;

    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::RIGID;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsGlobal;
    into.Append( head );
    RIGID_Struct main;
        main.formationID = 0xFF;
    into.Append( main );
    _log(SE__DESTINY, "SE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

void SystemEntity::Killed(Damage& damage)
{
    if (m_targMgr != nullptr) {
        // loop thru list of all modules targeting this entity and let them know it has been killed.
        m_targMgr->Destroyed();
        // notify all entities targeting us that we are gone, BEFORE deleting m_targMgr
        m_targMgr->ClearFromTargets();
        // remove TargMgr here to avoid redundant calls upon object deletion
        SafeDelete(m_targMgr);
    }
    Delete();
}

void SystemEntity::Delete()
{
    if (m_targMgr != nullptr)
        m_targMgr->ClearFromTargets(); //OnTarget(nullptr, TargMgr::Mode::Clear, TargMgr::Msg::Deleted);
    if (m_system != nullptr)
        m_system->RemoveEntity(this);
    // containers have additional calls to process and calls this.  calling here will create infinite loop
    if (!IsContainerSE())
        m_self->Delete();
}

double SystemEntity::DistanceTo2(const SystemEntity* other) {
    if (other->m_bubble == nullptr)
        return 1000000.0;
    return GetPosition().distance(other->GetPosition());
}

void SystemEntity::SendDamageStateChanged() {  //working 24Apr15
     DamageDetails dmgState;
        dmgState.shield = m_self->GetAttribute(AttrShieldCharge).get_double() / m_self->GetAttribute(AttrShieldCapacity).get_double();
        dmgState.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
        dmgState.timestamp = GetFileTimeNow();
        dmgState.armor = (1.0 - (m_self->GetAttribute(AttrArmorDamage).get_double() / m_self->GetAttribute(AttrArmorHP).get_double()));
        dmgState.structure = (1.0 - (m_self->GetAttribute(AttrDamage).get_double() / m_self->GetAttribute(AttrHP).get_double()));
     OnDamageStateChange dmgChange;
        dmgChange.entityID = m_self->itemID();
        dmgChange.state = dmgState.Encode();
    PyTuple *up = dmgChange.Encode();
    // Bubble broadcast first (clones), THEN targMgr (which nulls the pointer)
    if (SysBubble() != nullptr) {
        std::vector<Client*> players;
        SysBubble()->GetPlayers(players);
        for (auto p : players) {
            if (p == nullptr) continue;
            PyTuple* clone = static_cast<PyTuple*>(up->Clone());
            p->QueueDestinyUpdate(&clone);
            PySafeDecRef(clone);
        }
    }
    if (m_targMgr != nullptr)
        m_targMgr->QueueUpdate(&up);
    PySafeDecRef(up);
    _log(DAMAGE__MESSAGE, "%s(%u): DamageUpdate - S:%f A:%f H:%f.", \
            m_self->name(), m_self->itemID(), dmgState.shield, dmgState.armor, dmgState.structure);
}

void SystemEntity::DropLoot(WreckContainerRef wreckRef, uint32 groupID, uint32 owner) {
    /*   allan 27Nov14    */
    std::vector<LootList> lootList;
    sDataMgr.GetLoot(groupID, lootList);
    if (lootList.empty()) {
        _log(LOOT__INFO, "lootList empty for %s(%u)", m_self->name(), m_self->itemID());
        return;
    }

    uint32 quantity(0);
    std::vector<LootList>::iterator itr = lootList.begin();
    while (itr != lootList.end()) {
        if (itr->minDrop == itr->maxDrop) {
            quantity = itr->minDrop;
        } else {
            quantity = (uint32)(MakeRandomInt(itr->minDrop, itr->maxDrop));
        }
        if (quantity == 0)
            quantity = 1;

        ItemData iLoot(itr->itemID, owner, wreckRef->itemID(), flagNone, quantity);
        wreckRef->AddItem(sItemFactory.SpawnItem(iLoot));
        _log(LOOT__INFO, "added %u of %u to list for %s(%u)", quantity, itr->itemID, m_self->name(), m_self->itemID());
        ++itr;
    }
}

/** @todo (allan)  this doesnt need to be here */
void SystemEntity::AwardSecurityStatus(InventoryItemRef iRef, Character* pChar) {
    if (pChar == nullptr) {
        _log(EFFECTS__ERROR, "AwardSecurityStatus() called with null Character");
        return;
    }
    //New Status = ((10 - Old Status) * Sec Incr) + Old Status
    double oldSec = pChar->GetSecurityRating();
    EvilNumber maxGain = 0;
    if (iRef->HasAttribute(AttrEntitySecurityMaxGain, maxGain))
        if (oldSec > maxGain.get_double())
            return;
    float killBonus = iRef->GetAttribute(AttrEntitySecurityStatusKillBonus).get_float();
    double secAward = (((10 - oldSec) * killBonus) + oldSec) / 100;
    secAward *=  (1 + (0.05f * (pChar->GetSkillLevel(EvESkill::FastTalk, true))));      // 5% increase
    if (killBonus and secAward) {
        secAward *= sConfig.rates.secRate;
        sLog.Magenta("SE::AwardSecurityStatus()"," %s(%u): killBonus: %f.  oldSec: %f.  secAward: %f.",
                     GetName(), iRef->itemID(), killBonus, oldSec, secAward);
        pChar->secStatusChange( secAward );
        std::string msg = "Status Change for killing";
        if (iRef->HasPilot()) {
            msg += iRef->GetPilot()->GetName();
            msg += " in ";
            msg += m_system->GetNameStr();
            sStandingMgr.UpdateStandings(iRef->itemID(), pChar->itemID(), Standings::CombatShipKill, secAward, msg);
        } else {
            msg += " pirates in ";
            msg += m_system->GetNameStr();
            sStandingMgr.UpdateStandings(corpCONCORD, pChar->itemID(), Standings::LawEnforcement, secAward, msg);
            // decrease standings with faction of this npc kill
            sStandingMgr.UpdateStandings(iRef->ownerID(), pChar->itemID(), Standings::CombatShipKill, -secAward, msg);
        }
    }

    /** @todo msg need work for details to appear correctly.  currently working, but could be better. (incomplete, but working)
     * see data in eve/common/script/util/eveFormat.py:300 for details
     *
     *
     * SE::AwardSecurityStatus():  Shadow Serpentis Port Admiral(750000006): killBonus: 0.072000.  oldSec: 0.044933.  secAward: 0.007617.
     * SE::AwardSecurityStatus():               Estamel Tharchon(750000001): killBonus: 0.244141.  oldSec: 0.000000.  secAward: 0.024414.
     *
     */
}

void SystemEntity::Abandon()
{
    m_warID = 0;
    m_allyID = 0;
    m_corpID = 0;
    m_fleetID = 0;
    m_ownerID = 1;
    m_self->ChangeOwner(1); // update this to use system owner?    not sure.  logs show this as "1" for all non-player items
    /** @todo  should this have a slimupdate or bubblecast or something?  */
}


/* Static / Non-Mobile / Non-Destructable / Celestial Objects - Suns, Planets, Moons, Belts, Gates, Stations */
StaticSystemEntity::StaticSystemEntity(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: SystemEntity(self, services, system)
{
}

// copy c'tor
StaticSystemEntity::StaticSystemEntity(const StaticSystemEntity* oth)
: SystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("SSE::SSE()", "copy c'tor.");
    // wip
}

bool StaticSystemEntity::LoadExtras() {
    return true;
}

PyDict* StaticSystemEntity::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for SSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID",       new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",       new PyInt(m_self->typeID()));
        slim->SetItemString("name",         new PyString(m_self->itemName()));
        slim->SetItemString("nameID",       PyStatic.NewNone());
        slim->SetItemString("ownerID",      PyStatic.NewOne());
    return slim;
}

void StaticSystemEntity::EncodeDestiny( Buffer& into ) {
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::RIGID;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.radius = m_radius;
        head.flags = Ball::Flag::IsGlobal;
    into.Append( head );
    RIGID_Struct main;
        main.formationID = 0xFF;
    into.Append( main );
    _log(SE__DESTINY, "SSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X, radius:%.1f", GetName(), head.entityID, head.mode, head.flags, head.radius);
}

BeltSE::BeltSE(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: StaticSystemEntity(self, services, system)
{
}

// copy c'tor
BeltSE::BeltSE(const BeltSE* oth)
: StaticSystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("SSE::BeltSE()", "copy c'tor.");
    // wip
}

bool BeltSE::LoadExtras() {
    if (!StaticSystemEntity::LoadExtras())
        return false;

    if (m_bubble == nullptr)
        sBubbleMgr.Add(this);

    m_bubble->SetBelt(m_self);
    _log(DESTINY__BUBBLE_DEBUG, "BeltSE::LoadExtras() - IsBelt set to true for bubble %u.", m_bubble->GetID() );
    return true;
}

StargateSE::StargateSE(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: StaticSystemEntity(self, services, system),
m_sbuSE(nullptr)
{
}

// copy c'tor
StargateSE::StargateSE(const StargateSE* oth)
: StaticSystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("SSE::gateSE()", "copy c'tor.");
    // wip
}

bool StargateSE::LoadExtras() {
    if (!StaticSystemEntity::LoadExtras())
        return false;

    if (m_bubble == nullptr)
        sBubbleMgr.Add(this);

    m_bubble->SetGate(true);
    _log(DESTINY__BUBBLE_DEBUG, "StargateSE::LoadExtras() - IsGate set to true for bubble %u.", m_bubble->GetID() );
    m_jumps = SystemDB::ListJumps(m_self->itemID());
    if (m_jumps != nullptr)
        return true;

    return false;
}

PyDict* StargateSE::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for StargateSE %s(%u)", GetName(), m_self->itemID());
    /** @todo  finish gate rotation data
    PyTuple* rotation = new PyTuple(3);
        rotation->SetItem(0, new PyFloat(0));
        rotation->SetItem(1, new PyFloat(0));
        rotation->SetItem(2, new PyFloat(0));*/
    PyDict *slim = new PyDict();
        //slim->SetItemString("dunRotation", rotation);
        slim->SetItemString("typeID",       new PyInt(m_self->typeID()));
        /** @todo (allan) make function to lookup controlling faction id for this */
        //  NOTE:  maybe not...logs show this is "1" for all items.
        slim->SetItemString("ownerID",      PyStatic.NewOne());
        slim->SetItemString("itemID",       new PyLong(m_self->itemID()));
        slim->SetItemString("name",         new PyString(m_self->itemName()));
    if (m_jumps != nullptr)
        slim->SetItemString("jumps", m_jumps->Clone());
    return slim;
}


/* Non-Static / Non-Mobile / Non-Destructable / Celestial Objects - Containers, DeadSpace, ForceFields, ScanProbes */
ItemSystemEntity::ItemSystemEntity(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: SystemEntity(self, services, system),
m_keyType(0)
{
}

// copy c'tor
ItemSystemEntity::ItemSystemEntity(const ItemSystemEntity* oth)
: SystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("ISE::ISE()", "copy c'tor.");
    // wip
}

PyDict* ItemSystemEntity::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for ISE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID",       new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",       new PyInt(m_self->typeID()));
        slim->SetItemString("ownerID",      new PyInt(m_ownerID));
        slim->SetItemString("categoryID",   new PyInt(m_self->categoryID()));
        slim->SetItemString("groupID",      new PyInt(m_self->groupID()));
        slim->SetItemString("name",         new PyString(m_self->itemName()));
        if (m_self->groupID() == EVEDB::invGroups::Warp_Gate
            || m_self->typeID() == 2902) {  // LCS Acceleration Gate (SDE group 319)
            // this is incomplete........
            slim->SetItemString("dunSkillLevel", PyStatic.NewNone());   //?
            slim->SetItemString("dunSkillTypeID", PyStatic.NewNone());   //?
            slim->SetItemString("dunObjectID", new PyInt(160449));  //?   902139
            slim->SetItemString("dunToGateID", new PyInt(160484));  //?   902140
            slim->SetItemString("dunCloaked", new PyBool(0));   //?
            slim->SetItemString("dunScenarioID", new PyInt(23));    //?  3347
            slim->SetItemString("dunSpawnID", new PyInt(1572));  //?
            slim->SetItemString("dunAmount", new PyFloat(0.0));  //?
            PyList* classList = new PyList();
                classList->AddItem( new PyInt(324));
                classList->AddItem( new PyInt(420));
                classList->AddItem( new PyInt(541));
                classList->AddItem( new PyInt(834));
                classList->AddItem( new PyInt(25));
                classList->AddItem( new PyInt(830));
            slim->SetItemString("dunShipClasses", classList);   //?
            PyList* dirList = new PyList();
            // dunDirection = unit vector from the gate toward the next room
            // (the warp path the ship takes through the gate). Stored in
            // customInfo as "gate_to:x:y:z". Fall back to +x (rooms are laid
            // out along +x).
            GPoint from(m_self->position());
            GPoint to(from.x + 1, from.y, from.z);
            if (m_self->customInfo().rfind("gate_to:", 0) == 0) {
                long long gx, gy, gz;
                if (sscanf(m_self->customInfo().c_str(), "gate_to:%lld:%lld:%lld", &gx, &gy, &gz) == 3)
                    to = GPoint((double)gx, (double)gy, (double)gz);
            }
            double dx = to.x - from.x;
            double dy = to.y - from.y;
            double dz = to.z - from.z;
            double len = sqrt(dx * dx + dy * dy + dz * dz);
            if (len < 1.0)
                len = 1.0;
            dirList->AddItem(PyStatic.NewInt((int)round(dx / len * 10000.0)));
            dirList->AddItem(PyStatic.NewInt((int)round(dy / len * 10000.0)));
            dirList->AddItem(PyStatic.NewInt((int)round(dz / len * 10000.0)));
            slim->SetItemString("dunDirection", dirList);
            // dunRotation = (yaw, pitch, roll). Point the gate plane at the
            // next room in the horizontal plane (yaw only), keep it strictly
            // horizontal (pitch=0, roll=0) so the acceleration runs right over
            // the gate.  yaw = atan2(runX, runZ) as in CustomsOffice.
            // +180°: the Acceleration Gate model faces AWAY from the oncoming
            // ship, so rotate half a turn to face the warp vector.
            PyTuple* rotTuple = new PyTuple(3);
                float yawDeg = EvE::Trig::Rad2Deg(atan2(dx, dz)) + 180.0f;
                if (yawDeg > 180.0f) yawDeg -= 360.0f;
                else if (yawDeg < -180.0f) yawDeg += 360.0f;
                rotTuple->SetItem(0, new PyFloat(yawDeg));
                rotTuple->SetItem(1, new PyFloat(0.0));
                rotTuple->SetItem(2, new PyFloat(0.0));
            slim->SetItemString("dunRotation", rotTuple);
            slim->SetItemString("dunKeyLock", PyStatic.NewNone());   //?
            slim->SetItemString("dunWipeNPC", new PyBool(0));   //?
            slim->SetItemString("dunKeyQuantity", PyStatic.NewOne());   //?
            slim->SetItemString("dunKeyTypeID", new PyInt(m_keyType));   //Training Complex Passkey   group Acceleration_Gate_Keys
            slim->SetItemString("dunOpenUntil", new PyInt(Win32TimeNow()+EvE::Time::Hour));   //?
            slim->SetItemString("dunRoomName", new PyString("Lobby"));   //?
            slim->SetItemString("dunMusicUrl", new PyString("res:/Sound/Music/Ambient031combat.ogg"));
        }
    /** @todo  finish rotation data
    Large_Collidable_Structure
    Large_Collidable_Ship
    Large_Collidable_Object
    PyTuple* rotation = new PyTuple(3);
        rotation->SetItem(0, new PyFloat(0));
        rotation->SetItem(1, new PyFloat(0));
        rotation->SetItem(2, new PyFloat(0));
    slim->SetItemString("dunRotation", rotation);
    */
    return slim;
}

void ItemSystemEntity::EncodeDestiny( Buffer& into )
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::RIGID;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        // IsMassive like ObjectSystemEntity (asteroids): a RIGID ball with no flags
        // isn't created/rendered by the client's Ballpark (decor/clouds were
        // invisible), and IsInteractive/IsGlobal weren't delivered reliably when
        // warping into an already-spawned anomaly. IsMassive is the only flag that
        // reliably renders decor on both the spawn and warp-in paths.
        head.flags = Ball::Flag::IsMassive;
    into.Append( head );
    RIGID_Struct main;
        main.formationID = 0xFF;
    into.Append( main );

    _log(SE__DESTINY, "ISE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

void ItemSystemEntity::MakeDamageState(DoDestinyDamageState &into) {
    if (m_self->groupID() == EVEDB::invGroups::Force_Field) {
        SystemEntity::MakeDamageState(into);
    } else {
        into.shield = (m_self->GetAttribute(AttrShieldCharge).get_double() / m_self->GetAttribute(AttrShieldCapacity).get_double());
        into.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
        into.timestamp = GetFileTimeNow();
        into.armor = 1.0 - (m_self->GetAttribute(AttrArmorDamage).get_double() / m_self->GetAttribute(AttrArmorHP).get_double());
        into.structure = 1.0 - (m_self->GetAttribute(AttrDamage).get_double() / m_self->GetAttribute(AttrHP).get_double());
    }
}

FieldSE::FieldSE(InventoryItemRef self, EVEServiceManager &services, SystemManager *system, const FactionData& data)
: ItemSystemEntity(self, services, system)
{
    m_warID = data.factionID;
    m_allyID = data.allianceID;
    m_corpID = data.corporationID;
    m_ownerID = data.ownerID;
}

// copy c'tor
FieldSE::FieldSE(const FieldSE* oth)
: ItemSystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("ISE::FieldSE()", "copy c'tor.");
    // wip
}

void FieldSE::EncodeDestiny( Buffer& into )
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = (m_harmonic > EVEPOS::Harmonic::Offline ? Ball::Mode::FIELD : Ball::Mode::STOP);
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsGlobal;
    into.Append( head );
    MassSector mass = MassSector();
        mass.mass = 10000000000;    // as seen in packets
        mass.cloak = 0;
        mass.harmonic = m_harmonic;
        mass.corporationID = m_corpID;
        mass.allianceID = (IsAlliance(m_allyID) ? m_allyID : -1);
    into.Append( mass );
    if (head.mode == Ball::Mode::FIELD) {
        FIELD_Struct main;
        main.formationID = 0xFF;
        into.Append( main );
    } else if (head.mode == Ball::Mode::STOP) {
        STOP_Struct main;
        main.formationID = 0xFF;
        into.Append( main );
    }

    _log(SE__DESTINY, "FSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

PyDict *FieldSE::MakeSlimItem()
{
    return SystemEntity::MakeSlimItem();
}


/* Non-Static / Non-Mobile / Destructible / Celestial Objects - POS Structures, Outposts, Deployables, empty Ships, Asteroids */
ObjectSystemEntity::ObjectSystemEntity(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: SystemEntity(self, services, system),
m_invul(false)
{
    m_targMgr = new TargetManager(this);
    m_destiny = new DestinyManager(this);

    assert(m_targMgr != nullptr);
    assert(m_destiny != nullptr);
}

// copy c'tor
ObjectSystemEntity::ObjectSystemEntity(const ObjectSystemEntity* oth)
: SystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("OSE::OSE()", "copy c'tor.");
    // wip
}

ObjectSystemEntity::~ObjectSystemEntity()
{
    if (m_targMgr != nullptr)
        if (!sConsole.IsShutdown()) {
            m_targMgr->ClearModules();
            m_targMgr->ClearAllTargets(false);
            //m_targMgr->OnTarget(nullptr, TargMgr::Mode::Clear, TargMgr::Msg::Destroyed);
        }

    SafeDelete(m_targMgr);
    SafeDelete(m_destiny);
}

void ObjectSystemEntity::EncodeDestiny( Buffer& into )
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::RIGID;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsMassive;
    into.Append( head );
    RIGID_Struct main;
        main.formationID = 0xFF;
    into.Append( main );

    _log(SE__DESTINY, "OSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

void DeployableSE::EncodeDestiny(Buffer& into)
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
    if (!m_anchored) {
        // Unanchored deployable — STOP mode like NPC ships (no DataSector needed for static objects).
        head.mode = Ball::Mode::STOP;
        head.flags = Ball::Flag::IsFree;
        into.Append(head);
        MassSector mass = MassSector();
            mass.mass = 1.0e9f;
            mass.cloak = 0;
            mass.corporationID = m_corpID;
            mass.allianceID = m_allyID;
            mass.harmonic = 0;
        into.Append(mass);
        // Client expects DataSector after MassSector when flags & IsFree (0x01).
        // Without it, the client reads STOP_Struct as DataSector, desyncs the entire
        // destiny binary stream, and causes "Unknown packet type" in SynchroniseToSimulationTime.
        DataSector data = DataSector();
            data.maxSpeed = 0.0f;
            data.velX = 0.0;
            data.velY = 0.0;
            data.velZ = 0.0;
            data.inertia = 0.0f;
            data.speedfraction = 0.0f;
        into.Append(data);
        STOP_Struct main;
            main.formationID = 0xFF;
        into.Append(main);
    } else {
        // Anchored deployable — RIGID mode (fixed in place).
        head.mode = Ball::Mode::RIGID;
        head.flags = Ball::Flag::IsMassive;
        into.Append(head);
        RIGID_Struct main;
            main.formationID = 0xFF;
        into.Append(main);
    }
    _log(SE__DESTINY, "DSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X, anchored:%d",
         GetName(), head.entityID, head.mode, head.flags, m_anchored);
}

PyDict* ObjectSystemEntity::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for OSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID",           new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",           new PyInt(GetTypeID()));
        slim->SetItemString("ownerID",          new PyInt(m_ownerID));
        slim->SetItemString("categoryID",       new PyInt(m_self->categoryID()));
        slim->SetItemString("groupID",          new PyInt(m_self->groupID()));
        slim->SetItemString("name",             new PyString(m_self->itemName()));
        slim->SetItemString("corpID",           IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
        slim->SetItemString("allianceID",       IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
        slim->SetItemString("warFactionID",     IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
    return slim;
}

PyDict* DeployableSE::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for DSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID",           new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",           new PyInt(GetTypeID()));
        slim->SetItemString("ownerID",          new PyInt(m_ownerID));
        slim->SetItemString("categoryID",       new PyInt(m_self->categoryID()));
        slim->SetItemString("groupID",          new PyInt(m_self->groupID()));
        slim->SetItemString("name",             new PyString(m_self->itemName()));
        slim->SetItemString("corpID",           IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
        slim->SetItemString("allianceID",       IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
        slim->SetItemString("warFactionID",     IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
        slim->SetItemString("flag",             new PyInt(flagNone));
        slim->SetItemString("posState",         new PyInt(m_posState));
        slim->SetItemString("posTimestamp",     PyStatic.NewInt(0));
        slim->SetItemString("posDelayTime",     new PyInt(m_anchorTime / 1000));
    return slim;
}

void ObjectSystemEntity::MakeDamageState(DoDestinyDamageState &into) {
    double shieldCap = m_self->GetAttribute(AttrShieldCapacity).get_double();
    double shieldCharge = m_self->GetAttribute(AttrShieldCharge).get_double();
    into.shield = (shieldCap > 0.0) ? (shieldCharge / shieldCap) : 1.0;
    into.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
    into.timestamp = GetFileTimeNow();
    double armorHP = m_self->GetAttribute(AttrArmorHP).get_double();
    double armorDmg = m_self->GetAttribute(AttrArmorDamage).get_double();
    into.armor = (armorHP > 0.0) ? (1.0 - armorDmg / armorHP) : 1.0;
    double hullHP = m_self->GetAttribute(AttrHP).get_double();
    double hullDmg = m_self->GetAttribute(AttrDamage).get_double();
    into.structure = (hullHP > 0.0) ? (1.0 - hullDmg / hullHP) : 1.0;
}

void ObjectSystemEntity::UpdateDamage()
{
    /** @todo (Allan) needs more work */
    SystemEntity::UpdateDamage();
     DamageDetails dmgState;
        dmgState.shield = m_self->GetAttribute(AttrShieldCharge).get_double() / m_self->GetAttribute(AttrShieldCapacity).get_double();
        dmgState.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
        dmgState.timestamp = GetFileTimeNow();
        dmgState.armor = 1.0 - m_self->GetAttribute(AttrArmorDamage).get_double() / m_self->GetAttribute(AttrArmorHP).get_double();
        dmgState.structure = 1.0 - m_self->GetAttribute(AttrDamage).get_double() / m_self->GetAttribute(AttrHP).get_double();
     OnDamageStateChange dmgChange;
        dmgChange.entityID = m_self->itemID();
        dmgChange.state = dmgState.Encode();
    PyTuple *up = dmgChange.Encode();
    if (SysBubble() != nullptr) {
        std::vector<Client*> players;
        SysBubble()->GetPlayers(players);
        for (auto p : players) {
            if (p == nullptr) continue;
            PyTuple* clone = static_cast<PyTuple*>(up->Clone());
            p->QueueDestinyUpdate(&clone);
            PySafeDecRef(clone);
        }
    }
    PySafeDecRef(up);
}

void ObjectSystemEntity::Killed(Damage& damage)
{
    // do we need to make wreck items here?
    // do these structures have loot?  probably so eventually

    /** @todo  test and complete this to null current customs office for this planet ... */
    if (IsCOSE()) {
        if (GetCOSE()->GetPlanetID() > 0) {
            SystemEntity* pSE = m_system->GetSE(GetCOSE()->GetPlanetID());
            pSE->GetPlanetSE()->SetCustomsOffice(nullptr);
        }
    }
}

DeployableSE::DeployableSE(InventoryItemRef self, EVEServiceManager &services, SystemManager *system, const FactionData& data)
: ObjectSystemEntity(self, services, system)
{
    m_warID = data.factionID;
    m_allyID = data.allianceID;
    m_corpID = data.corporationID;
    m_ownerID = data.ownerID;
    m_posState = EVEPOS::EntityState::Unanchored;  // -2 = unanchored
    m_anchorTime = m_self->GetAttribute(AttrAnchoringDelay).get_uint32();
    if (m_anchorTime < 1000) m_anchorTime = 5000;
    m_warpScrambleTimer.Start(1000);
}

void DeployableSE::Anchor(Client* pClient, const GPoint& pos)
{
    _log(POS__MESSAGE, "DeployableSE::Anchor %s(%u) at (%.0f,%.0f,%.0f)",
         m_self->name(), m_self->itemID(), pos.x, pos.y, pos.z);

    // Crucible: Mobile Warp Disruptors only in 0.0 (AttrAnchoringSecurityLevelMax)
    float maxSec = m_self->GetAttribute(AttrAnchoringSecurityLevelMax).get_float();
    SystemData sysData;
    sDataMgr.GetSystemData(pClient->GetSystemID(), sysData);
    if (sysData.securityRating > maxSec) {
        pClient->SendErrorMsg("You cannot anchor this structure in this security level.");
        return;
    }

    m_self->SetPosition(pos);
    uint32 anchorTime = m_self->GetAttribute(AttrAnchoringDelay).get_uint32();
    if (anchorTime < 1000) anchorTime = 5000;
    m_anchorTime = anchorTime;
    m_anchoring = true;
    m_anchorTimer.Start(anchorTime);

    m_posState = EVEPOS::EntityState::Anchoring;  // -6 = anchoring
    SendSlimUpdate();
    // AnchorDrop start=1,active=1 — client shows anchoring animation via SetBuiltStructureGraphics(1)
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(),
        0, 0, "effects.AnchorDrop", 0, 1, 1, anchorTime, 0);
}

void DeployableSE::Unanchor(Client* pClient)
{
    _log(POS__MESSAGE, "DeployableSE::Unanchor %s(%u)", m_self->name(), m_self->itemID());
    uint32 unanchorTime = m_self->GetAttribute(AttrUnanchoringDelay).get_uint32();
    if (unanchorTime < 1000) unanchorTime = 5000;
    m_anchorTimer.Start(unanchorTime);
    m_anchoring = false;
    m_unanchoring = true;
    m_posState = EVEPOS::EntityState::Unanchoring;  // -3 = unanchoring
    m_anchorTime = unanchorTime;
    SendSlimUpdate();

    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(), 0, 0, "effects.AnchorLift", 0, 1, 1, -1, 0);
}

void DeployableSE::SetImmediateOnline()
{
    _log(POS__MESSAGE, "DeployableSE::SetImmediateOnline %s(%u)", m_self->name(), m_self->itemID());
    m_anchored = true;
    m_anchoring = false;
    m_onlined = true;
    m_onlining = false;
    m_offlining = false;
    m_unanchoring = false;
    m_posState = EVEPOS::StructureState::Online;  // 4 = online
    if (SysBubble() != nullptr)
        SysBubble()->SetWarpBubble(true);
    m_warpScrambleTimer.Start(1000);
    SendSlimUpdate();
}

void DeployableSE::Online(Client* pClient)
{
    _log(POS__MESSAGE, "DeployableSE::Online %s(%u)", m_self->name(), m_self->itemID());
    if (!m_anchored) {
        pClient->SendErrorMsg("Structure must be anchored before it can be brought online.");
        return;
    }
    uint32 onlineTime = m_self->GetAttribute(AttrOnliningDelay).get_uint32();
    if (onlineTime < 1000) onlineTime = 5000;
    m_onlining = true;
    m_onlineTimer.Start(onlineTime);
}

void DeployableSE::Offline(Client* pClient)
{
    _log(POS__MESSAGE, "DeployableSE::Offline %s(%u)", m_self->name(), m_self->itemID());
    if (!m_onlined) {
        m_onlining = false;
        return;
    }
    m_offlining = true;
    m_onlining = false;
    uint32 offTime = m_self->GetAttribute(AttrAnchoringDelay).get_uint32(); // reuse anchor delay as offline delay
    if (offTime < 1000) offTime = 5000;
    m_offlineTimer.Start(offTime);
}

void DeployableSE::Process()
{
    ObjectSystemEntity::Process();

    // Handle timer-based state transitions (anchor/online/offline/unanchor)
    if (m_offlining && m_offlineTimer.Check(false)) {
        m_offlineTimer.Disable();
        m_offlining = false;
        m_onlined = false;
        m_posState = EVEPOS::StructureState::Anchored;  // 1 = anchored (offline, no bubble)
        // Clear warp bubble and scramble effect on all ships in bubble
        if (m_self->groupID() == EVEDB::invGroups::Mobile_Warp_Disruptor && SysBubble() != nullptr) {
            SysBubble()->SetWarpBubble(false);
            m_destiny->SendSpecialEffect10(m_self->itemID(), 0, "effects.AnchorLift", 0, 0, 0);
            m_destiny->SendSpecialEffect10(m_self->itemID(), 0, "effects.WarpDisruptFieldGenerating", 0, 0, 0);
            std::vector<Client*> players;
            SysBubble()->GetPlayers(players);
            for (auto pClient : players) {
                if (pClient == nullptr) continue;
                SystemEntity* pShipSE = pClient->GetShipSE();
                if (pShipSE == nullptr) continue;
                pShipSE->GetSelf()->SetAttribute(AttrWarpScrambleStatus, int64(0), false);
            }
        }
        _log(POS__MESSAGE, "DeployableSE::Process %s(%u) — offline complete", m_self->name(), m_self->itemID());
        SendSlimUpdate();
    } else if (m_unanchoring && m_anchorTimer.Check(false)) {
        m_anchorTimer.Disable();
        m_unanchoring = false;
        m_anchored = false;
        m_onlined = false;
        m_posState = EVEPOS::StructureState::Unanchored;  // 0 = unanchored
        // Clear warp bubble flag when unanchored (also clear scramble if not already done)
        if (m_self->groupID() == EVEDB::invGroups::Mobile_Warp_Disruptor && SysBubble() != nullptr) {
            SysBubble()->SetWarpBubble(false);
            m_destiny->SendSpecialEffect10(m_self->itemID(), 0, "effects.AnchorLift", 0, 0, 0);
            m_destiny->SendSpecialEffect10(m_self->itemID(), 0, "effects.WarpDisruptFieldGenerating", 0, 0, 0);
            std::vector<Client*> players;
            SysBubble()->GetPlayers(players);
            for (auto pClient : players) {
                if (pClient == nullptr) continue;
                SystemEntity* pShipSE = pClient->GetShipSE();
                if (pShipSE == nullptr) continue;
                pShipSE->GetSelf()->SetAttribute(AttrWarpScrambleStatus, int64(0), false);
            }
        }
        _log(POS__MESSAGE, "DeployableSE::Process %s(%u) — unanchor complete, waiting to be scooped", m_self->name(), m_self->itemID());
        SendSlimUpdate();
        m_self->SetFlag(flagNone, true);
        return;
    } else if (m_anchoring && m_anchorTimer.Check(false)) {
        m_anchorTimer.Disable();
        m_anchoring = false;
        m_anchored = true;
        m_onlined = false;  // separate Online() call required
        m_posState = EVEPOS::StructureState::Anchored;  // 1 = anchored (waiting for online)
        _log(POS__MESSAGE, "DeployableSE::Process %s(%u) — anchor complete, waiting for online", m_self->name(), m_self->itemID());
        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(), 0, 0, "effects.AnchorDrop", 0, 0, 0, -1, 0);
        SendSlimUpdate();
    } else if (m_onlining && m_onlineTimer.Check(false)) {
        m_onlineTimer.Disable();
        m_onlining = false;
        m_onlined = true;
        m_posState = EVEPOS::StructureState::Online;  // 4 = online
        _log(POS__MESSAGE, "DeployableSE::Process %s(%u) — online complete", m_self->name(), m_self->itemID());
        if (m_self->groupID() == EVEDB::invGroups::Mobile_Warp_Disruptor && SysBubble() != nullptr) {
            SysBubble()->SetWarpBubble(true);
            m_destiny->SendSpecialEffect10(m_self->itemID(), 0, "effects.StructureOnlined", 0, 1, 1);
            // Send WarpDisruptFieldGenerating with graphicInfo(KeyVal(range=xxx)) for correct bubble radius.
            OnSpecialFX14 fx;
                fx.entityID = m_self->itemID();
                fx.moduleID = m_self->itemID();
                fx.moduleTypeID = m_self->typeID();
                fx.targetID = PyStatic.NewNone();
                fx.chargeTypeID = PyStatic.NewNone();
                fx.area = new PyList();
                fx.guid = "effects.WarpDisruptFieldGenerating";
                fx.isOffensive = 0;
                fx.start = 1;
                fx.active = 1;
                fx.duration = -1;
                fx.repeat = 0;
                fx.startTime = GetFileTimeNow();
                PyDict* gd = new PyDict();
                gd->SetItemString("range", new PyFloat(m_self->GetAttribute(AttrWarpScrambleRange).get_float()));
                fx.graphicInfo = new PyObject("util.KeyVal", gd);
            PyTuple* payload = fx.Encode();
            m_destiny->SendSingleDestinyUpdate(&payload);
        }
        _log(POS__MESSAGE, "DeployableSE::Process %s(%u) — online complete", m_self->name(), m_self->itemID());
        m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(), 0, 0, "effects.AnchorDrop", 0, 0, 0, -1, 0);
        SendSlimUpdate();
    }

    // Warp scramble only when online (not offlining)
    if (!m_onlined || m_offlining) {
        if (m_self->groupID() == EVEDB::invGroups::Mobile_Warp_Disruptor && SysBubble() != nullptr)
            SysBubble()->SetWarpBubble(false);
        return;
    }
    if (m_self->groupID() != EVEDB::invGroups::Mobile_Warp_Disruptor)
        return;
    // Ensure warp bubble flag is set for online deployables (e.g. loaded from DB)
    if (SysBubble() != nullptr && !SysBubble()->HasWarpBubble())
        SysBubble()->SetWarpBubble(true);
    if (!m_warpScrambleTimer.Check(true)) {
        _log(DESTINY__WARNING, "DeployableSE::ScrambleCheck — timer not firing yet");
        return;
    }
    if (SysBubble() == nullptr) {
        _log(DESTINY__WARNING, "DeployableSE::ScrambleCheck — no SysBubble");
        return;
    }
    // EVE retail warp scramble ranges per typeID (SDE data)
    float range = 20000.0f;  // default fallback
    switch (m_self->typeID()) {
        case 12198: range = 5000.0f;   break;  // Mobile Small Warp Disruptor I
        case 26892: range = 7500.0f;   break;  // Mobile Small Warp Disruptor II
        case 28774: range = 7500.0f;   break;  // Syndicate Mobile Small Warp Disruptor
        case 12199: range = 11500.0f;  break;  // Mobile Medium Warp Disruptor I
        case 26890: range = 17500.0f;  break;  // Mobile Medium Warp Disruptor II
        case 28772: range = 17500.0f;  break;  // Syndicate Mobile Medium Warp Disruptor
        case 12200: range = 26500.0f;  break;  // Mobile Large Warp Disruptor I
        case 26888: range = 40000.0f;  break;  // Mobile Large Warp Disruptor II
        case 28770: range = 40000.0f;  break;  // Syndicate Mobile Large Warp Disruptor
        case 4386:  range = 48000.0f;  break;  // Mobile Large Jump Disruptor I
        default:
            range = m_self->GetAttribute(AttrWarpScrambleRange).get_float();
            if (range < 1.0f) range = 5000.0f;
            _log(DESTINY__WARNING, "DeployableSE::ScrambleProcess — unknown MWD typeID %u, using attribute range %.0f", m_self->typeID(), range);
            break;
    }
    _log(DESTINY__WARNING, "DeployableSE::ScrambleProcess — MWD %s(%u) typeID=%u range=%.0f", GetName(), m_self->itemID(), m_self->typeID(), range);
    uint32 strength = m_self->GetAttribute(AttrWarpScrambleStrength).get_uint32();
    if (strength < 1)
        strength = 1;
    bool hasTargetInRange = false;
    GPoint myPos = GetPosition();
    std::vector<Client*> players;
    SysBubble()->GetPlayers(players);
    for (auto pClient : players) {
        if (pClient == nullptr) continue;
        SystemEntity* pShipSE = pClient->GetShipSE();
        if (pShipSE == nullptr) continue;
        if (pShipSE->DestinyMgr()->IsCloaked())
            continue;
        if (pShipSE->GetSelf()->groupID() == EVEDB::invGroups::Shuttle)
            continue;
        if (pShipSE->GetSelf()->HasAttribute(AttrWarpBubbleImmune)
            && pShipSE->GetSelf()->GetAttribute(AttrWarpBubbleImmune).get_bool())
            continue;
        float dist = myPos.distance(pShipSE->GetPosition());
        if (dist <= range) {
            hasTargetInRange = true;
            pShipSE->GetSelf()->SetAttribute(AttrWarpScrambleStatus, (int)strength, true);
        } else {
            pShipSE->GetSelf()->SetAttribute(AttrWarpScrambleStatus, int64(0), true);
        }
    }
    // Bubble visual stays active always — distance-based scramble is handled by AttrWarpScrambleStatus.
}

void DeployableSE::SendSlimUpdate()
{
    PyDict *slim = new PyDict();
    slim->SetItemString("name", new PyString(m_self->itemName()));
    slim->SetItemString("itemID", new PyLong(m_self->itemID()));
    slim->SetItemString("typeID", new PyInt(m_self->typeID()));
    slim->SetItemString("ownerID", new PyInt(m_ownerID));
    slim->SetItemString("corpID", IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
    slim->SetItemString("allianceID", IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
    slim->SetItemString("warFactionID", IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
    slim->SetItemString("categoryID", new PyInt(m_self->categoryID()));
    slim->SetItemString("groupID", new PyInt(m_self->groupID()));
    slim->SetItemString("flag", new PyInt(flagNone));
    slim->SetItemString("posState", new PyInt(m_posState));
    slim->SetItemString("posTimestamp", new PyLong(GetFileTimeNow()));
    slim->SetItemString("posDelayTime", new PyInt(m_anchorTime / 1000));
    PyTuple *shipData = new PyTuple(2);
    shipData->SetItem(0, new PyLong(m_self->itemID()));
    shipData->SetItem(1, new PyObject("foo.SlimItem", slim));
    PyTuple *sItem = new PyTuple(2);
    sItem->SetItem(0, new PyString("OnSlimItemChange"));
    sItem->SetItem(1, shipData);
    m_destiny->SendSingleDestinyUpdate(&sItem);
}

// copy c'tor
DeployableSE::DeployableSE(const DeployableSE* oth)
: ObjectSystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("OSE::DeployableSE()", "copy c'tor.");
    // wip
}


/* Non-Static / Mobile / Destructible / Celestial Objects - PC's, NPC's, Drones, Ships, Missiles, Wrecks  */
DynamicSystemEntity::DynamicSystemEntity(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: SystemEntity(self, services, system),
m_invul(false),
m_frozen(false)
{
    m_targMgr = new TargetManager(this);
    m_destiny = new DestinyManager(this);

    assert(m_targMgr != nullptr);
    assert(m_destiny != nullptr);
}

// copy c'tor
DynamicSystemEntity::DynamicSystemEntity(const DynamicSystemEntity* oth)
: SystemEntity(oth->m_self, oth->m_services, oth->m_system)
{
    sLog.Error("DSE::DSE()", "copy c'tor.");
    // wip
}

DynamicSystemEntity::~DynamicSystemEntity()
{
    if (m_targMgr != nullptr)
        if (!sConsole.IsShutdown()) {
            m_targMgr->ClearModules();
            m_targMgr->ClearAllTargets(false);
            //m_targMgr->OnTarget(nullptr, TargMgr::Mode::Clear, TargMgr::Msg::Destroyed);
        }

    SafeDelete(m_targMgr);
    SafeDelete(m_destiny);
}

PyDict *DynamicSystemEntity::MakeSlimItem() {
    _log(SE__SLIMITEM, "MakeSlimItem for DSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID",           new PyLong(m_self->itemID()));
        slim->SetItemString("typeID",           new PyInt(m_self->typeID()));
        slim->SetItemString("ownerID",          new PyInt(m_ownerID));
        slim->SetItemString("categoryID",       new PyInt(m_self->categoryID()));
        slim->SetItemString("groupID",          new PyInt(m_self->groupID()));
        slim->SetItemString("name",             new PyString(m_self->itemName()));
        slim->SetItemString("corpID",           IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
        slim->SetItemString("allianceID",       IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
        slim->SetItemString("warFactionID",     IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
        slim->SetItemString("charID",           PyStatic.NewNone());
    return (slim);
}

void DynamicSystemEntity::EncodeDestiny( Buffer& into )
{
    using namespace Destiny;
    BallHeader head = BallHeader();
        head.entityID = m_self->itemID();
        head.mode = Ball::Mode::STOP;
        head.radius = m_radius;
        head.posX = x();
        head.posY = y();
        head.posZ = z();
        head.flags = Ball::Flag::IsFree | Ball::Flag::IsInteractive | Ball::Flag::IsMassive;
    into.Append( head );
    MassSector mass = MassSector();
        mass.mass = m_destiny->GetMass();
        mass.cloak = (m_destiny->IsCloaked() ? 1 : 0);
        mass.harmonic = m_harmonic;
        mass.corporationID = m_corpID;
        mass.allianceID = (IsAlliance(m_allyID) ? m_allyID : -1);
    into.Append( mass );
    DataSector data = DataSector();
        data.inertia = m_destiny->GetInertia();
        data.maxSpeed = m_destiny->GetMaxVelocity();
        data.velX = m_destiny->GetVelocity().x;
        data.velY = m_destiny->GetVelocity().y;
        data.velZ = m_destiny->GetVelocity().z;
        data.speedfraction = m_destiny->GetSpeedFraction();
    into.Append( data );
    STOP_Struct main;
        main.formationID = 0xFF;
    into.Append( main );

    _log(SE__DESTINY, "DSE::EncodeDestiny(): %s - id:%lli, mode:%u, flags:0x%X", GetName(), head.entityID, head.mode, head.flags);
}

void DynamicSystemEntity::MakeDamageState(DoDestinyDamageState &into) {
    into.shield = (m_self->GetAttribute(AttrShieldCharge).get_double() / m_self->GetAttribute(AttrShieldCapacity).get_double());
    into.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
    into.timestamp = GetFileTimeNow();
    into.armor = 1.0 - (m_self->GetAttribute(AttrArmorDamage).get_double() / m_self->GetAttribute(AttrArmorHP).get_double());
    into.structure = 1.0 - (m_self->GetAttribute(AttrDamage).get_double() / m_self->GetAttribute(AttrHP).get_double());
}

void DynamicSystemEntity::UpdateDamage()
{
    /** @todo (Allan) needs more work */
    SystemEntity::UpdateDamage();
     DamageDetails dmgState;
        dmgState.shield = m_self->GetAttribute(AttrShieldCharge).get_double() / m_self->GetAttribute(AttrShieldCapacity).get_double();
        dmgState.recharge = m_self->GetAttribute(AttrShieldRechargeRate).get_double();
        dmgState.timestamp = GetFileTimeNow();
        dmgState.armor = 1.0 - m_self->GetAttribute(AttrArmorDamage).get_double() / m_self->GetAttribute(AttrArmorHP).get_double();
        dmgState.structure = 1.0 - m_self->GetAttribute(AttrDamage).get_double() / m_self->GetAttribute(AttrHP).get_double();
     OnDamageStateChange dmgChange;
        dmgChange.entityID = m_self->itemID();
        dmgChange.state = dmgState.Encode();
    PyTuple *up = dmgChange.Encode();
    //source->QueueDestinyUpdate(&up);
    PySafeDecRef(up);
}

void DynamicSystemEntity::AwardBounty(Client* pClient)
{
    // this will use a map{charID/BountyData} in system manager for using a bounty timer.
    double bounty = m_self->GetAttribute(AttrEntityKillBounty).get_double();
    bounty *= sConfig.rates.npcBountyMultiply;
    if (bounty < 1)
        return;

    // Apply incursion bounty penalty (-25% in incursion systems)
    if (sIncursionMgr.IsIncursionSystem(m_system->GetID()))
        bounty *= 0.75;

    // add data to StatisticMgr
    sStatMgr.Add(Stat::npcBounties, bounty);

    std::string reason = "Bounty for killing a pirate in ";
    reason += pClient->GetSystemName();

    BountyData data = BountyData();
    data.fromID = m_self->itemID();
    data.toID = pClient->GetCharacterID();
    data.refTypeID = Journal::EntryType::BountyPrize;
    data.fromKey = Account::KeyType::Cash;
    data.toKey = Account::KeyType::Cash;
    data.reason = reason;

    // handle distribution to fleets
    if (pClient->InFleet()) {
        // get fleet members onGrid and distrubute bounty
        std::vector< uint32 > members;
        sFltSvc.GetFleetMembersOnGrid(pClient, members);
        // split bounty between members
        bounty /= members.size();
        // send bounty to members
        if (sConfig.server.BountyPayoutDelayed and sConfig.server.FleetShareDelayed) {
            for (auto cur :members)
                m_system->AddBounty(cur, data);
        } else {
            reason += " (FleetShare) ";
            reason += " by ";
            reason += pClient->GetName();
            data.reason = reason;
            for (auto cur :members)
                AccountService::TransferFunds(corpCONCORD, cur, bounty, reason.c_str(), Journal::EntryType::BountyPrize, -GetTypeID());
        }
    } else {
        data.amount = bounty;
        if (sConfig.server.BountyPayoutDelayed) {
            m_system->AddBounty(pClient->GetCharacterID(), data);
        } else {
            AccountService::TransferFunds(corpCONCORD, pClient->GetCharacterID(), bounty, reason.c_str(), Journal::EntryType::BountyPrize, -GetTypeID());
        }
    }
}

DungeonEditSE::DungeonEditSE(InventoryItemRef self, EVEServiceManager& services, SystemManager* system, Dungeon::RoomObject data)
: ObjectSystemEntity(self, services, system),
    m_data(data)
{
}

PyDict* DungeonEditSE::MakeSlimItem()
{
    _log(SE__SLIMITEM, "MakeSlimItem for DungeonEditSE %s(%u)", GetName(), m_self->itemID());
    PyDict *slim = new PyDict();
        slim->SetItemString("itemID", new PyLong(m_self->itemID()));
        slim->SetItemString("typeID", new PyInt(m_self->typeID()));
        slim->SetItemString("groupID", new PyInt(m_self->groupID()));
        slim->SetItemString("dunObjectID", new PyInt(m_self->itemID()));
        slim->SetItemString("dunRadius", new PyFloat(m_data.radius));
        slim->SetItemString("dunRoomID", new PyInt(m_data.roomID));
        slim->SetItemString("dunX", new PyFloat(m_data.x));
        slim->SetItemString("dunY", new PyFloat(m_data.y));
        slim->SetItemString("dunZ", new PyFloat(m_data.z));
        std::string itemName = m_self->itemName();
        if (itemName.empty())
            itemName = sDataMgr.GetTypeName(m_self->typeID());
        slim->SetItemString("name",         new PyString(itemName.empty() ? "unknown" : itemName));

    return slim;
}