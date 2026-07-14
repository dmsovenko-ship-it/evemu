/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2011 The EVEmu Team
    For the latest information visit http://evemu.org
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


#include "missions/MissionMgrService.h"
#include "Client.h"

MissionMgrService::MissionMgrService() :
    Service("missionMgr")
{
    this->Add("GetMyCourierMissions", &MissionMgrService::GetMyCourierMissions);
}

PyResult MissionMgrService::GetMyCourierMissions(PyCallArgs& call)
{
    sLog.White("MissionMgrService", "Handle_GetMyCourierMissions() size=%lli", call.tuple->size());

    uint32 charID = call.client->GetCharacterID();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT"
        "  o.offerID,"
        "  o.typeID,"
        "  o.agentID,"
        "  o.stateID,"
        "  o.expiryTime,"
        "  o.rewardISK,"
        "  o.rewardLP,"
        "  CASE WHEN o.originID > 0 THEN o.originID ELSE (SELECT agt.locationID FROM agtAgents AS agt WHERE agt.agentID = o.agentID) END AS originID,"
        "  CASE WHEN o.originOwnerID > 0 THEN o.originOwnerID ELSE (SELECT agt.corporationID FROM agtAgents AS agt WHERE agt.agentID = o.agentID) END AS originOwnerID,"
        "  CASE WHEN o.originSystemID > 0 THEN o.originSystemID ELSE (SELECT COALESCE(chr.solarSystemID, itm.solarSystemID, agt.locationID) FROM agtAgents AS agt LEFT JOIN chrNPCCharacters AS chr ON chr.characterID = agt.agentID LEFT JOIN mapDenormalize AS itm ON itm.itemID = agt.locationID WHERE agt.agentID = o.agentID) END AS originSystemID,"
        "  CASE WHEN o.destinationID > 0 THEN o.destinationID ELSE (SELECT agt.locationID FROM agtAgents AS agt WHERE agt.agentID = o.agentID) END AS destinationID,"
        "  CASE WHEN o.destinationTypeID > 0 THEN o.destinationTypeID ELSE 0 END AS destinationTypeID,"
        "  CASE WHEN o.destinationOwnerID > 0 THEN o.destinationOwnerID ELSE (SELECT agt.corporationID FROM agtAgents AS agt WHERE agt.agentID = o.agentID) END AS destinationOwnerID,"
        "  CASE WHEN o.destinationSystemID > 0 THEN o.destinationSystemID ELSE (SELECT COALESCE(chr.solarSystemID, itm.solarSystemID, agt.locationID) FROM agtAgents AS agt LEFT JOIN chrNPCCharacters AS chr ON chr.characterID = agt.agentID LEFT JOIN mapDenormalize AS itm ON itm.itemID = agt.locationID WHERE agt.agentID = o.agentID) END AS destinationSystemID,"
        "  CASE WHEN o.courierTypeID > 0 THEN o.courierTypeID ELSE 23 END as courierTypeID,"
        "  CASE WHEN o.courierAmount > 0 THEN o.courierAmount ELSE 1 END as courierAmount,"
        "  o.courierVolume,"
        "  o.acceptFee"
        " FROM agtOffers o"
        " WHERE o.characterID = %u AND o.stateID < 2 AND o.typeID = 3",
        charID))
    {
        return new PyList();
    }
    return DBResultToCRowset(res);
}
