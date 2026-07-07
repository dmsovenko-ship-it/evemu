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
*/

#include "eve-server.h"

#include "station/JumpCloneService.h"
#include "station/StationDB.h"
#include "services/ServiceManager.h"
#include "character/CharacterDB.h"

JumpCloneService::JumpCloneService(EVEServiceManager& mgr) :
    BindableService("jumpCloneSvc", mgr)
{
}

BoundDispatcher* JumpCloneService::BindObject (Client* client, PyRep* bindParameters)
{
    return new JumpCloneBound(this->GetServiceManager(), *this, &m_db, client->GetLocationID());
}

void JumpCloneService::BoundReleased (JumpCloneBound* bound) {
}

JumpCloneBound::JumpCloneBound (EVEServiceManager& mgr, JumpCloneService& parent, StationDB* db, uint32 locationID) :
    EVEBoundObject(mgr, parent),
    m_db(db),
    m_locationID(locationID),
    m_locGroupID(EVEDB::invGroups::Station)
{
    this->Add("GetCloneState", &JumpCloneBound::GetCloneState);
    this->Add("GetShipCloneState", &JumpCloneBound::GetShipCloneState);
    this->Add("GetPriceForClone", &JumpCloneBound::GetPriceForClone);
    this->Add("InstallCloneInStation", &JumpCloneBound::InstallCloneInStation);
    this->Add("GetStationCloneState", &JumpCloneBound::GetStationCloneState);
    this->Add("OfferShipCloneInstallation", &JumpCloneBound::OfferShipCloneInstallation);
    this->Add("DestroyInstalledClone", &JumpCloneBound::DestroyInstalledClone);
    this->Add("AcceptShipCloneInstallation", &JumpCloneBound::AcceptShipCloneInstallation);
    this->Add("CancelShipCloneInstallation", &JumpCloneBound::CancelShipCloneInstallation);
    this->Add("CloneJump", &JumpCloneBound::CloneJump);

    if (sDataMgr.IsStation(m_locationID))
        m_locGroupID = EVEDB::invGroups::Solar_System;
}

PyResult JumpCloneBound::GetCloneState(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();

    DBQueryResult res;
    m_db->GetClones(charID, res);

    PyList* clones = new PyList();
    PyDict* implants = new PyDict();
    PyList* shipClones = new PyList();

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 cloneID = row.GetUInt(0);
        uint32 locID = row.GetUInt(2);
        uint8 isActive = row.GetUInt(5);

        // Return each clone as util.KeyVal with cloneID and locationID
        PyDict* entry = new PyDict();
        entry->SetItemString("cloneID", new PyInt(cloneID));
        entry->SetItemString("locationID", new PyInt(locID));
        entry->SetItemString("isActive", new PyInt(isActive));
        entry->SetItemString("typeID", new PyInt(row.GetUInt(1)));
        clones->AddItem(new PyObject("util.KeyVal", entry));

        // Load per-clone implants from chrJumpCloneImplants
        DBQueryResult implantRes;
        m_db->GetCloneImplants(cloneID, implantRes);

        std::vector<uint32> implantTypeIDs;
        while (implantRes.GetRow(row))
            implantTypeIDs.push_back(row.GetUInt(0));

        PyTuple* implantTuple = new PyTuple(implantTypeIDs.size());
        for (size_t i = 0; i < implantTypeIDs.size(); ++i)
            implantTuple->SetItem(i, new PyInt(implantTypeIDs[i]));
        implants->SetItem(new PyInt(cloneID), implantTuple);
    }

    PyDict* dict = new PyDict();
    dict->SetItemString("clones", clones);
    dict->SetItemString("shipClones", shipClones);
    dict->SetItemString("implants", implants);
    dict->SetItemString("timeLastJump", new PyLong(GetFileTimeNow() - (EvE::Time::Hour * MakeRandomFloat(1, 23))));

    return new PyObject("util.KeyVal", dict);
}

PyResult JumpCloneBound::GetStationCloneState(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();

    DBQueryResult res;
    m_db->GetClones(charID, res);

    PyList* clones = new PyList();
    PyDict* implants = new PyDict();

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 cloneID = row.GetUInt(0);
        uint32 locID = row.GetUInt(2);
        uint8 isActive = row.GetUInt(5);
        if (locID == m_locationID) {
            PyDict* entry = new PyDict();
            entry->SetItemString("cloneID", new PyInt(cloneID));
            entry->SetItemString("locationID", new PyInt(locID));
            entry->SetItemString("isActive", new PyInt(isActive));
            entry->SetItemString("typeID", new PyInt(row.GetUInt(1)));
            clones->AddItem(new PyObject("util.KeyVal", entry));

            DBQueryResult implantRes;
            m_db->GetCloneImplants(cloneID, implantRes);

            std::vector<uint32> implantTypeIDs;
            while (implantRes.GetRow(row))
                implantTypeIDs.push_back(row.GetUInt(0));

            PyTuple* implantTuple = new PyTuple(implantTypeIDs.size());
            for (size_t i = 0; i < implantTypeIDs.size(); ++i)
                implantTuple->SetItem(i, new PyInt(implantTypeIDs[i]));
            implants->SetItem(new PyInt(cloneID), implantTuple);
        }
    }

    PyDict* dict = new PyDict();
    dict->SetItemString("clones", clones);
    dict->SetItemString("implants", implants);
    dict->SetItemString("timeLastJump", new PyLong(GetFileTimeNow() - (EvE::Time::Hour * MakeRandomFloat(1, 23))));
    dict->SetItemString("isShipCloneBayInstalled", PyStatic.NewFalse());

    return new PyObject("util.KeyVal", dict);
}

PyResult JumpCloneBound::GetShipCloneState(PyCallArgs &call) {
    // Ship clone bay - for ships with clone bay (Rorqual, etc.)
    // Returns list of clones installed in the current ship
    PyList* clones = new PyList();

    // TODO: ship clone bay not implemented yet
    // Need to query entity table for clones with locationID = shipID and flag = flagClone

    return clones;
}

PyResult JumpCloneBound::GetPriceForClone(PyCallArgs &call) {
    // Client passes the clone typeID to check price
    // If no typeID provided, default to Clone Grade Alpha
    uint32 typeID = itemCloneAlpha;
    if (call.byname.find("typeID") != call.byname.end())
        typeID = PyRep::IntegerValueU32(call.byname.find("typeID")->second);
    else if (call.tuple->size() > 0)
        typeID = PyRep::IntegerValueU32(call.tuple->GetItem(0));

    return new PyInt(m_db->GetClonePrice(typeID));
}

PyResult JumpCloneBound::InstallCloneInStation(PyCallArgs &call) {
    uint32 charID = call.client->GetCharacterID();
    uint32 stationID = call.client->GetStationID();

    if (stationID == 0)
        return PyStatic.NewFalse();

    DBQueryResult res;
    m_db->GetClones(charID, res);

    int cloneCount = 0;
    DBResultRow row;
    while (res.GetRow(row)) {
        if (row.GetUInt(2) == stationID)
            return PyStatic.NewFalse();
        ++cloneCount;
    }

    const int MAX_JUMP_CLONES = 5;
    if (cloneCount >= MAX_JUMP_CLONES)
        return PyStatic.NewFalse();

    std::string customInfo = "Jump clone installed at station ";
    customInfo += std::to_string(stationID);

    uint32 newCloneID = m_db->CreateClone(charID, itemCloneAlpha, stationID, "Clone Grade Alpha", customInfo.c_str());
    if (newCloneID == 0)
        return PyStatic.NewFalse();

    return new PyInt(newCloneID);
}

PyResult JumpCloneBound::DestroyInstalledClone(PyCallArgs &call, PyInt* cloneID) {
    DBerror err;
    sDatabase.RunQuery(err, "DELETE FROM chrJumpCloneImplants WHERE jumpCloneID = %u", cloneID->value());
    m_db->DeleteClone(cloneID->value());
    return PyStatic.NewTrue();
}

PyResult JumpCloneBound::CloneJump(PyCallArgs &call, PyInt* locationID) {
    uint32 charID = call.client->GetCharacterID();
    uint32 destLocationID = locationID->value();

    DBQueryResult res;
    m_db->GetClones(charID, res);

    DBResultRow row;
    uint32 targetCloneID = 0;
    while (res.GetRow(row)) {
        if (row.GetUInt(2) == destLocationID) {
            targetCloneID = row.GetUInt(0);
            break;
        }
    }

    if (targetCloneID == 0)
        return PyStatic.NewFalse();

    // Deactivate old active clone, activate new one
    m_db->SetCloneActive(charID, targetCloneID);

    // Update character's home station to destination
    CharacterDB::ChangeCloneLocation(charID, destLocationID);

    PyTuple* payload = new PyTuple(0);
    call.client->SendNotification("OnJumpCloneCacheInvalidated", "clientID", payload, false);

    return PyStatic.NewTrue();
}

PyResult JumpCloneBound::OfferShipCloneInstallation(PyCallArgs &call, PyInt* characterID) {
    // Offer to install a jump clone in this ship for the target character
    uint32 targetCharID = characterID->value();
    uint32 shipID = call.client->GetShipID();

    if (shipID == 0)
        return PyStatic.NewFalse();

    // Notify the target character of the offer
    PyDict* args = new PyDict();
        args->SetItemString("characterID", new PyInt(call.client->GetCharacterID()));
        args->SetItemString("shipID", new PyInt(shipID));
        args->SetItemString("locationID", new PyInt(call.client->GetLocationID()));

    PyTuple* payload = new PyTuple(1);
        payload->SetItem(0, new PyObject("util.KeyVal", args));

    Client* targetClient = sEntityList.FindClientByCharID(targetCharID);
    if (targetClient != nullptr)
        targetClient->SendNotification("OnShipJumpCloneInstallationOffered", "clientID", payload, false);

    return PyStatic.NewTrue();
}

PyResult JumpCloneBound::AcceptShipCloneInstallation(PyCallArgs &call) {
    // Accept a ship clone installation offer
    // This creates a jump clone in the offering ship's clone bay
    // For now, basic implementation
    return PyStatic.NewFalse();
}

PyResult JumpCloneBound::CancelShipCloneInstallation(PyCallArgs &call) {
    // Cancel a pending ship clone installation offer
    PyTuple* payload = new PyTuple(0);
    call.client->SendNotification("OnShipJumpCloneInstallationCanceled", "clientID", payload, false);
    return PyStatic.NewTrue();
}

