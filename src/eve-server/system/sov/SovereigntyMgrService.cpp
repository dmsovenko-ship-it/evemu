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


#include "system/sov/SovereigntyDataMgr.h"
#include "system/sov/SovereigntyDB.h"
#include "system/sov/SovereigntyMgrService.h"

SovereigntyMgrService::SovereigntyMgrService() :
    Service("sovMgr")
{
    this->Add("GetSystemSovereigntyInfo", &SovereigntyMgrService::GetSystemSovereigntyInfo);
    this->Add("GetSystemUpgrades", &SovereigntyMgrService::GetSystemUpgrades);
    this->Add("InstallUpgrade", &SovereigntyMgrService::InstallUpgrade);
    this->Add("RemoveUpgrade", &SovereigntyMgrService::RemoveUpgrade);
}

PyResult SovereigntyMgrService::GetSystemSovereigntyInfo(PyCallArgs &call, PyInt* systemID) {
    return svDataMgr.GetSystemSovereignty(systemID->value());
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
