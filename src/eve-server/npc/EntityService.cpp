
 /**
  * @name EntityService.cpp
  *   Drone Control class
  * @Author:    Allan
  * @date:      06 November 2016
  */


#include "eve-server.h"

#include "EVEServerConfig.h"
#include "EntityList.h"
#include "npc/Drone.h"
#include "npc/EntityService.h"
#include "ship/Ship.h"
#include "system/SystemManager.h"
#include "services/ServiceManager.h"

EntityService::EntityService(EVEServiceManager& mgr) :
    BindableService("entity", mgr)
{
}

/*  drone states...
namespace DroneAI {
    namespace State {
        enum {
            Invalid           = -1,
            // defined in client
            Idle              = 0,  // not doing anything....idle.
            Combat            = 1,  // fighting - needs targetID
            Mining            = 2,  // unsure - needs targetID
            Approaching       = 3,  // too close to chase, but to far to engage
            Departing         = 4,  // return to ship
            Departing2        = 5,  // leaving.  different from Departing
            Pursuit           = 6,  // target out of range to attack/follow, but within npc sight range....use mwd/ab if equiped
            Fleeing           = 7,  // running away
            Operating         = 9,  // whats diff from engaged here?
            Engaged           = 10, // non-combat? - needs targetID
            // internal only
            Unknown           = 8,  // as stated
            Guarding          = 11,
            Assisting         = 12,
            Incapacicated     = 13  //
        };
    }
}
*/

/*
DRONE__ERROR
DRONE__WARNING
DRONE__MESSAGE
DRONE__INFO
DRONE__TRACE
DRONE__DUMP
DRONE__AI_TRACE
*/

/** @todo  will need to make sure this object is deleted when changing systems  */
BoundDispatcher *EntityService::BindObject(Client* client, PyRep* bindParameters) {
    _log(DRONE__DUMP, "EntityService bind request");
    bindParameters->Dump(DRONE__DUMP, "    ");
    if (!bindParameters->IsInt()) {
        codelog(SERVICE__ERROR, "%s: Non-integer bind argument '%s'", client->GetName(), bindParameters->TypeString());
        return nullptr;
    }

    uint32 systemID = bindParameters->AsInt()->value();
    if (!sDataMgr.IsSolarSystem(systemID)) {
        codelog(SERVICE__ERROR, "%s: Expected systemID, but got %u.", client->GetName(), systemID);
        return nullptr;
    }

    auto it = this->m_instances.find (systemID);

    if (it != this->m_instances.end ())
        return it->second;

    EntityBound* bound = new EntityBound(this->GetServiceManager(), *this, client->SystemMgr(), systemID);

    this->m_instances.insert_or_assign (systemID, bound);

    return bound;
}

void EntityService::BoundReleased (EntityBound* bound) {
    auto it = this->m_instances.find (bound->GetSystemID());

    if (it == this->m_instances.end ())
        return;

    this->m_instances.erase (it);
}

EntityBound::EntityBound(EVEServiceManager &mgr, EntityService& parent, SystemManager* systemMgr, uint32 systemID) :
    EVEBoundObject(mgr, parent),
    m_sysMgr(systemMgr),
    m_systemID(systemID)
{
    this->Add("CmdEngage", &EntityBound::CmdEngage);
    this->Add("CmdRelinquishControl", &EntityBound::CmdRelinquishControl);
    this->Add("CmdDelegateControl", &EntityBound::CmdDelegateControl);
    this->Add("CmdAssist", &EntityBound::CmdAssist);
    this->Add("CmdGuard", &EntityBound::CmdGuard);
    this->Add("CmdMine", &EntityBound::CmdMine);
    this->Add("CmdMineRepeatedly", &EntityBound::CmdMineRepeatedly);
    this->Add("CmdUnanchor", &EntityBound::CmdUnanchor);
    this->Add("CmdReturnHome", &EntityBound::CmdReturnHome);
    this->Add("CmdReturnBay", &EntityBound::CmdReturnBay);
    this->Add("CmdAbandonDrone", &EntityBound::CmdAbandonDrone);
    this->Add("CmdReconnectToDrones", &EntityBound::CmdReconnectToDrones);
}

PyResult EntityBound::CmdEngage(PyCallArgs &call, PyList* droneIDs, PyInt* targetID) {
 // ret = entity.CmdEngage(droneIDs, targetID)
    _log(DRONE__TRACE, "EntityBound::CmdEngage()");

    SystemEntity* pTarget = m_sysMgr->GetSE(targetID->value());
    if (pTarget == nullptr) {
        _log(DRONE__MESSAGE, "CmdEngage: target %u not found in system.", targetID->value());
        return new PyDict();
    }

    // Return dict maps droneID -> error tuple for any drones that couldn't comply.
    PyDict* errors = new PyDict();

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);

        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdEngage: drone %u not found in system.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();

        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdEngage: %s tried to command drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }
        if (!pDrone->IsEnabled()) {
            _log(DRONE__MESSAGE, "CmdEngage: drone %u is offline.", droneID);
            continue;
        }

        _log(DRONE__TRACE, "CmdEngage: ordering drone %u to engage %u.", droneID, targetID->value());
        pDrone->ClearAssistTarget();  // manual engagement cancels assist
        pDrone->SetTarget(pTarget);
        pDrone->GetAI()->Target(pTarget);   // initiates targeting and CheckDistance
        pDrone->StateChange();
    }

    return errors;
}

PyResult EntityBound::CmdRelinquishControl(PyCallArgs &call, PyList* IDs) {
 // ret = entity.CmdRelinquishControl(IDs)
    _log(DRONE__TRACE, "EntityBound::CmdRelinquishControl()");
    call.Dump(DRONE__DUMP);

    for (PyList::const_iterator itr = IDs->begin(); itr != IDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);
        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) continue;
        DroneSE* pDrone = pSE->GetDroneSE();
        // Only the original owner or the delegated controller can relinquish
        if (!pDrone->CanCommand(call.client->GetCharacterID())) continue;
        // Return control to original owner
        pDrone->RestoreOriginalOwner();
        pDrone->StateChange();
        _log(DRONE__TRACE, "CmdRelinquishControl: drone %u control returned to original owner.", droneID);
    }
    return new PyDict();
}

PyResult EntityBound::CmdDelegateControl(PyCallArgs &call, PyList* droneIDs, PyInt* controllerID) {
 // ret = entity.CmdDelegateControl(droneIDs, controllerID)
    _log(DRONE__TRACE, "EntityBound::CmdDelegateControl()");
    call.Dump(DRONE__DUMP);

    uint32 newControllerCharID = controllerID->value();
    PyDict* errors = new PyDict();

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);
        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdDelegateControl: drone %u not found.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        // Only the original owner or delegated controller can delegate
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdDelegateControl: %s tried to delegate drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }
        if (!pDrone->IsEnabled()) {
            _log(DRONE__MESSAGE, "CmdDelegateControl: drone %u is offline.", droneID);
            continue;
        }
        // Clear assist on delegated drones
        pDrone->ClearAssistTarget();
        // Save original owner if first delegation
        if (!pDrone->IsDelegated())
            pDrone->SaveOriginalOwner(pDrone->GetControllerOwnerID(), pDrone->GetControllerOwnerID(), pDrone->GetControllerID());
        // Transfer control to new controller
        pDrone->SetDelegatedControllerID(newControllerCharID);
        // Update display owner so client shows delegated controller as owner
        Client* pDelegatedClient = sEntityList.FindClientByCharID(newControllerCharID);
        if (pDelegatedClient != nullptr) {
            ShipSE* pDelegatedShip = pDelegatedClient->GetShipSE();
            if (pDelegatedShip != nullptr)
                pDrone->SetDisplayOwner(newControllerCharID, pDelegatedShip->GetOwnerID(), pDelegatedShip->GetID());
        }
        pDrone->StateChange();
        _log(DRONE__TRACE, "CmdDelegateControl: drone %u delegated to char %u.", droneID, newControllerCharID);
    }
    return errors;
}

PyResult EntityBound::CmdAssist(PyCallArgs &call, PyInt* assistID, PyList* droneIDs) {
 // ret = entity.CmdAssist(assistID, droneIDs)
    _log(DRONE__TRACE, "EntityBound::CmdAssist()");
    call.Dump(DRONE__DUMP);

    uint32 assistCharID = assistID->value();
    PyDict* errors = new PyDict();

    // Validate assist target exists in this system
    SystemEntity* pAssistSE = m_sysMgr->GetSE(assistCharID);
    if (pAssistSE == nullptr) {
        for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
            uint32 droneID = PyRep::IntegerValueU32(*itr);
            errors->SetItem(new PyInt(droneID), new PyString("Assist target not found in system."));
        }
        return errors;
    }

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);
        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdAssist: drone %u not found.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdAssist: %s tried to command drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }
        if (!pDrone->IsEnabled()) {
            _log(DRONE__MESSAGE, "CmdAssist: drone %u is offline.", droneID);
            continue;
        }
        // Crucible-era: assist only works for NPC targets, but we still set the assist target
        pDrone->SetAssistTargetID(assistCharID);
        _log(DRONE__TRACE, "CmdAssist: drone %u now assisting char %u.", droneID, assistCharID);
    }
    return errors;
}

PyResult EntityBound::CmdGuard(PyCallArgs &call, PyInt* guardID, PyList* droneIDs) {
 // ret = entity.CmdGuard(guardID, droneIDs)
    _log(DRONE__TRACE, "EntityBound::Handle_CmdGuard()");
    call.Dump(DRONE__DUMP);

    call.client->SendNotifyMsg("This drone command is not yet supported.");
    return new PyDict();
}

PyResult EntityBound::CmdMine(PyCallArgs &call, PyList* droneIDs, PyInt* targetID) {
    _log(DRONE__TRACE, "EntityBound::CmdMine()");

    SystemEntity* pTarget = m_sysMgr->GetSE(targetID->value());
    if (pTarget == nullptr) {
        _log(DRONE__MESSAGE, "CmdMine: target %u not found in system.", targetID->value());
        return new PyDict();
    }

    PyDict* errors = new PyDict();
    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);
        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdMine: drone %u not found.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdMine: %s tried to command drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }
        if (!pDrone->IsEnabled()) {
            _log(DRONE__MESSAGE, "CmdMine: drone %u is offline.", droneID);
            continue;
        }
        if (pDrone->GetAI()->GetSubType() != DroneAI::SubType_Mining) {
            _log(DRONE__MESSAGE, "CmdMine: drone %u is not a mining drone.", droneID);
            continue;
        }
        _log(DRONE__TRACE, "CmdMine: ordering drone %u to mine %u (single cycle).", droneID, targetID->value());
        pDrone->SetTarget(pTarget);
        pDrone->GetAI()->MineTarget(pTarget, true);  // single cycle
        pDrone->StateChange();
    }
    return errors;
}

PyResult EntityBound::CmdMineRepeatedly(PyCallArgs &call, PyList* droneIDs, PyInt* targetID) {
    _log(DRONE__TRACE, "EntityBound::CmdMineRepeatedly()");

    SystemEntity* pTarget = m_sysMgr->GetSE(targetID->value());
    if (pTarget == nullptr) {
        _log(DRONE__MESSAGE, "CmdMineRepeatedly: target %u not found.", targetID->value());
        return new PyDict();
    }

    PyDict* errors = new PyDict();
    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);
        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) continue;
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) continue;
        if (!pDrone->IsEnabled()) continue;
        if (pDrone->GetAI()->GetSubType() != DroneAI::SubType_Mining) continue;
        pDrone->SetTarget(pTarget);
        pDrone->GetAI()->MineTarget(pTarget, false);  // continuous mining
        pDrone->StateChange();
    }
    return errors;
}

PyResult EntityBound::CmdUnanchor(PyCallArgs &call, PyList* droneIDs, PyInt* targetID) {
 // ret = entity.CmdUnanchor(droneIDs, targetID)
    _log(DRONE__TRACE, "EntityBound::Handle_CmdUnanchor()");
    call.Dump(DRONE__DUMP);

    call.client->SendNotifyMsg("This drone command is not yet supported.");
    return new PyDict();
}

PyResult EntityBound::CmdReturnHome(PyCallArgs &call, PyList* droneIDs) {
 // ret = entity.CmdReturnHome(droneIDs)
    // this is return and orbit command
    /*
02:18:26 [DroneTrace] EntityBound::Handle_CmdReturnHome()
02:18:26 [DroneDump]   Call Arguments:
02:18:26 [DroneDump]      Tuple: 1 elements
02:18:26 [DroneDump]       [ 0]   List: 1 elements
02:18:26 [DroneDump]       [ 0]   [ 0]    Integer: 140001219
*/
    _log(DRONE__TRACE, "EntityBound::CmdReturnHome()");
    call.Dump(DRONE__DUMP);

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);

        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdReturnHome: drone %u not found in system.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdReturnHome: %s tried to command drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }

        _log(DRONE__TRACE, "CmdReturnHome: ordering drone %u to return and orbit.", droneID);
        pDrone->ClearAssistTarget();
        pDrone->GetAI()->Return();
        pDrone->StateChange();
    }

    return new PyDict();
}

PyResult EntityBound::CmdReturnBay(PyCallArgs &call, PyList* droneIDs) {
 // ret = entity.CmdReturnBay(droneIDs)
    /*
        [PySubStream 97 bytes]
          [PyTuple 4 items]
            [PyInt 1]
            [PyString "MachoBindObject"]
            [PyTuple 2 items]
              [PyInt 30000302]
              [PyTuple 3 items]
                [PyString "CmdReturnBay"]
                [PyTuple 1 items]
                  [PyList 5 items]
                    [PyIntegerVar 1005909162494]
                    [PyIntegerVar 1005902743336]
                    [PyIntegerVar 1005909162497]
                    [PyIntegerVar 1005909162499]
                    [PyIntegerVar 1005909162492]
                [PyDict 0 kvp]

    [PyTuple 1 items]
      [PySubStream 42 bytes]
        [PyTuple 2 items]
          [PySubStruct]
            [PySubStream 31 bytes]
              [PyTuple 2 items]
                [PyString "N=790408:2886"]
                [PyIntegerVar 129756563162318175]
          [PyDict 0 kvp]
          */
    _log(DRONE__TRACE, "EntityBound::CmdReturnBay()");
    call.Dump(DRONE__DUMP);

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);

        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdReturnBay: drone %u not found in system.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdReturnBay: %s tried to command drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }

        _log(DRONE__TRACE, "CmdReturnBay: ordering drone %u to return to bay.", droneID);
        pDrone->ClearAssistTarget();
        pDrone->GetAI()->ReturnBay();
        pDrone->StateChange();
    }

    return new PyDict();
}

PyResult EntityBound::CmdAbandonDrone(PyCallArgs &call, PyList* droneIDs) {
 // ret = entity.CmdAbandonDrone(droneIDs)
    /*
     * 16:23:23 [DroneTrace] EntityBound::Handle_CmdAbandonDrone()
     * 16:23:23 [DroneDump]   Call Arguments:
     * 16:23:23 [DroneDump]      Tuple: 1 elements
     * 16:23:23 [DroneDump]       [ 0]   List: 1 elements
     * 16:23:23 [DroneDump]       [ 0]   [ 0]    Integer: 140024263
     */
    _log(DRONE__TRACE, "EntityBound::CmdAbandonDrone()");
    call.Dump(DRONE__DUMP);

    ShipSE* pShipSE = call.client->GetShipSE();
    if (pShipSE == nullptr)
        return new PyDict();

    for (PyList::const_iterator itr = droneIDs->begin(); itr != droneIDs->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);

        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdAbandonDrone: drone %u not found in system.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        if (!pDrone->CanCommand(call.client->GetCharacterID())) {
            _log(DRONE__WARNING, "CmdAbandonDrone: %s tried to abandon drone %u owned by %u.",
                 call.client->GetName(), droneID, pDrone->GetControllerOwnerID());
            continue;
        }

        _log(DRONE__TRACE, "CmdAbandonDrone: abandoning drone %u.", droneID);
        pShipSE->AbandonDrone(pSE);
    }

    return new PyDict();
}

PyResult EntityBound::CmdReconnectToDrones(PyCallArgs &call, PyList* droneCandidates) {
    // ret = entity.CmdReconnectToDrones(droneCandidates)
    //     for errStr, dicty in ret.iteritems():
    // this sends a list of drones in local space owned by calling character
    /*
     * 09:09:48 [DroneDump]   Call Arguments:
     * 09:09:48 [DroneDump]      Tuple: 1 elements
     * 09:09:48 [DroneDump]       [ 0]   List: 1 elements
     * 09:09:48 [DroneDump]       [ 0]   [ 0]    Integer: 140007055
     */
    _log(DRONE__TRACE, "EntityBound::CmdReconnectToDrones()");
    call.Dump(DRONE__DUMP);

    ShipSE* pShipSE = call.client->GetShipSE();
    if (pShipSE == nullptr)
        return new PyDict();

    uint32 charID = call.client->GetCharacterID();

    for (PyList::const_iterator itr = droneCandidates->begin(); itr != droneCandidates->end(); ++itr) {
        uint32 droneID = PyRep::IntegerValueU32(*itr);

        SystemEntity* pSE = m_sysMgr->GetSE(droneID);
        if (pSE == nullptr || !pSE->IsDroneSE()) {
            _log(DRONE__WARNING, "CmdReconnectToDrones: drone %u not found in system.", droneID);
            continue;
        }
        DroneSE* pDrone = pSE->GetDroneSE();
        // Only reconnect drones that belong to this character or are delegated to them
        if (!pDrone->CanCommand(charID)) {
            _log(DRONE__WARNING, "CmdReconnectToDrones: drone %u is owned by %u, skipping.",
                 droneID, pDrone->GetControllerOwnerID());
            continue;
        }

        _log(DRONE__TRACE, "CmdReconnectToDrones: reconnecting drone %u.", droneID);
        pDrone->SetOwner(call.client);
        pDrone->Online(pShipSE);
        pShipSE->AddDroneToFlight(pDrone);
    }

    return new PyDict();
}
