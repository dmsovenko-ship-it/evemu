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
    Author:        Reve,
    Rewrite:    Allan
    Updates:    James
*/

#include "eve-server.h"

#include "Client.h"
#include "system/sov/SovereigntyDataMgr.h"
#include "system/sov/SovereigntyDB.h"
#include "system/sov/SovereigntyMgrService.h"
#include "system/SystemManager.h"
#include "pos/sovStructures/IHub.h"

SovereigntyMgrService::SovereigntyMgrService() :
    Service("sovMgr")
{
    this->Add("GetSystemSovereigntyInfo", &SovereigntyMgrService::GetSystemSovereigntyInfo);
    this->Add("GetSystemUpgrades", &SovereigntyMgrService::GetSystemUpgrades);
    this->Add("InstallUpgrade", &SovereigntyMgrService::InstallUpgrade);
    this->Add("RemoveUpgrade", &SovereigntyMgrService::RemoveUpgrade);
    this->Add("GetSovOverview", &SovereigntyMgrService::GetSovOverview);
    this->Add("SetReinforceHour", &SovereigntyMgrService::SetReinforceHour);
}

PyResult SovereigntyMgrService::GetSystemSovereigntyInfo(PyCallArgs &call, PyInt* systemID) {
    return svDataMgr.GetSystemSovereignty(systemID->value());
}

PyResult SovereigntyMgrService::GetSovOverview(PyCallArgs& call) {
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT s.solarSystemID, s.constellationID, s.regionID, "
        "  COALESCE(sov.corporationID, 0) as corporationID, "
        "  COALESCE(sov.allianceID, 0) as allianceID, "
        "  COALESCE(s.factionID, 0) as factionID, "
        "  COALESCE(sov.claimTime, 0) as claimTime, "
        "  COALESCE(sov.claimStructureID, 0) as claimStructureID, "
        "  COALESCE(sov.hubID, 0) as hubID, "
        "  COALESCE(sov.contested, 0) as contested "
        "FROM mapSolarSystems s "
        "LEFT JOIN mapSystemSovInfo sov ON s.solarSystemID = sov.solarSystemID "
        "ORDER BY s.solarSystemID");
    DBResultRow row;
    DBRowDescriptor* header = new DBRowDescriptor();
        header->AddColumn("solarSystemID", DBTYPE_I4);
        header->AddColumn("constellationID", DBTYPE_I4);
        header->AddColumn("regionID", DBTYPE_I4);
        header->AddColumn("corporationID", DBTYPE_I4);
        header->AddColumn("allianceID", DBTYPE_I4);
        header->AddColumn("factionID", DBTYPE_I4);
        header->AddColumn("claimTime", DBTYPE_R8);
        header->AddColumn("claimStructureID", DBTYPE_I4);
        header->AddColumn("hubID", DBTYPE_I4);
        header->AddColumn("contested", DBTYPE_BOOL);
        header->AddColumn("sovereigntyLevel", DBTYPE_I4);
    CRowSet* rowset = new CRowSet(&header);
    int64 now = GetFileTimeNow();
    while (res.GetRow(row)) {
        PyPackedRow* pRow = rowset->NewRow();
        pRow->SetField("solarSystemID", new PyInt(row.GetUInt(0)));
        pRow->SetField("constellationID", new PyInt(row.GetUInt(1)));
        pRow->SetField("regionID", new PyInt(row.GetUInt(2)));
        pRow->SetField("corporationID", new PyInt(row.GetUInt(3)));
        pRow->SetField("allianceID", new PyInt(row.GetUInt(4)));
        pRow->SetField("factionID", new PyInt(row.GetUInt(5)));
        double claimTime = row.GetDouble(6);
        pRow->SetField("claimTime", new PyFloat(claimTime));
        pRow->SetField("claimStructureID", new PyInt(row.GetUInt(7)));
        pRow->SetField("hubID", new PyInt(row.GetUInt(8)));
        pRow->SetField("contested", new PyInt(row.GetUInt(9)));
        uint8 sovLevel = 0;
        if (claimTime > 0) {
            double daysSinceClaim = (now - (int64)claimTime) / (double)Win32Time_Day;
            sovLevel = std::min<uint8>(uint8(daysSinceClaim / 7), 5);
        }
        pRow->SetField("sovereigntyLevel", new PyInt(sovLevel));
    }
    return rowset;
}

PyResult SovereigntyMgrService::GetSystemUpgrades(PyCallArgs& call, PyInt* systemID) {
    DBQueryResult res;
    SovereigntyDB::GetUpgradesForSystem(res, systemID->value());
    return DBResultToCRowset(res);
}

PyResult SovereigntyMgrService::InstallUpgrade(PyCallArgs& call, PyInt* systemID, PyInt* upgradeTypeID) {
    SovereigntyDB::AddSystemUpgrade(systemID->value(), upgradeTypeID->value());
    return PyStatic.NewNone();
}

PyResult SovereigntyMgrService::RemoveUpgrade(PyCallArgs& call, PyInt* systemID, PyInt* upgradeID) {
    SovereigntyDB::RemoveSystemUpgrade(systemID->value(), upgradeID->value());
    return PyStatic.NewNone();
}

PyResult SovereigntyMgrService::SetReinforceHour(PyCallArgs& call, PyInt* systemID, PyInt* hour) {
    int8 h = static_cast<int8>(hour->value());
    if (h < 0 || h > 23) {
        call.client->SendNotifyMsg("Reinforce hour must be between 0 and 23.");
        return PyStatic.NewNone();
    }
    // Find the system and update its IHub reinforce hour
    SystemManager* pSys = sEntityList.FindOrBootSystem(systemID->value());
    if (pSys != nullptr) {
        // Persist to DB and update IHub
        svDataMgr.UpdateReinforceHour(systemID->value(), h);
        for (auto& ent : pSys->GetOperationalStatics()) {
            if (ent.second->IsIHubSE()) {
                ent.second->GetIHubSE()->SetReinforceHour(h);
                _log(SOV__INFO, "Reinforce hour set to %i for IHub in system %u", h, systemID->value());
                call.client->SendNotifyMsg("Reinforcement exit hour set to %02i:00.", h);
                return PyStatic.NewNone();
            }
        }
        call.client->SendNotifyMsg("Reinforcement hour saved to DB (no IHub in system).");
        return PyStatic.NewNone();
    }
    call.client->SendNotifyMsg("No IHub found in system %u.", systemID->value());
    return PyStatic.NewNone();
}
