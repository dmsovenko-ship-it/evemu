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

#ifndef __WAR_REGISTRY_SERVICE__H__INCL__
#define __WAR_REGISTRY_SERVICE__H__INCL__

#include "services/BoundService.h"
#include "Client.h"

class EVEServiceManager;
class WarRegistryBound;

void ProcessWarBills();

class WarRegistryService : public BindableService<WarRegistryService, WarRegistryBound> {
public:
    WarRegistryService(EVEServiceManager& mgr);

    void BoundReleased (WarRegistryBound* bound) override;
protected:
    BoundDispatcher* BindObject(Client* client, PyRep* bindParameters) override;

private:
    std::map<uint32, WarRegistryBound*> m_instances;
};

class WarRegistryBound : public EVEBoundObject<WarRegistryBound> {
    friend WarRegistryService;
public:
    PyResult GetWars(PyCallArgs& args, PyInt* ownerID, std::optional<PyInt*> forceRefresh);
    PyResult RetractWar(PyCallArgs& args, PyInt* againstID);
    PyResult DeclareWarAgainst(PyCallArgs& args, PyInt* againstID);
    PyResult ChangeMutualWarFlag(PyCallArgs& args, PyInt* warID, PyBool* mutual);
    PyResult GetCostOfWarAgainst(PyCallArgs& args, PyInt* ownerID);

    uint32 GetCorporationID() { return this->mCorporationID; }
protected:
    WarRegistryBound(uint32 corporationID, EVEServiceManager& mgr, WarRegistryService& parent);

private:
    uint32 mCorporationID;

    uint32 CreateWarRecord(uint32 declaredByID, uint32 againstID);
    void   EndWar(uint32 warID, uint32 retractedBy);
};

#endif /* __WAR_REGISTRY_SERVICE__H__INCL__ */


