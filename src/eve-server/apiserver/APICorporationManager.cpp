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
    Author:        Aknor Jaden
*/

#include "eve-server.h"

#include "apiserver/APICorporationManager.h"

APICorporationManager::APICorporationManager(const PyServiceMgr &services)
: APIServiceManager(services)
{
}

std::tr1::shared_ptr<std::string> APICorporationManager::ProcessCall(const APICommandCall * pAPICommandCall)
{
    sLog.Debug("APICorporationManager::ProcessCall()", "EVEmu API - Corporation Service Manager");

    if ( pAPICommandCall->find( "servicehandler" ) == pAPICommandCall->end() )
    {
        sLog.Error( "APICorporationManager::ProcessCall()", "Cannot find 'servicehandler' specifier in pAPICommandCall packet" );
        return std::tr1::shared_ptr<std::string>(new std::string(""));
    }

    if ( pAPICommandCall->find( "servicehandler" )->second == "KillMails.xml.aspx" )
        return _KillMails(pAPICommandCall);

    return BuildErrorXMLResponse( "9999", "EVEmu API Server: Corporation Manager - Unknown call." );
}

std::tr1::shared_ptr<std::string> APICorporationManager::_KillMails(const APICommandCall * pAPICommandCall)
{
    sLog.Debug("APICorporationManager::_KillMails()", "EVEmu API - Corporation KillMails.xml.aspx");

    if ( pAPICommandCall->find( "corporationid" ) == pAPICommandCall->end() )
    {
        sLog.Error( "APICorporationManager::_KillMails()", "ERROR: No 'corporationID' parameter" );
        return BuildErrorXMLResponse( "105", "Invalid corporationID." );
    }

    uint32 corporationID = atoi( pAPICommandCall->find( "corporationid" )->second.c_str() );

    uint32 beforeKillID = 0;
    if ( pAPICommandCall->find( "beforekillid" ) != pAPICommandCall->end() )
        beforeKillID = atoi( pAPICommandCall->find( "beforekillid" )->second.c_str() );

    DBQueryResult res;
    std::string query = "SELECT killID, solarSystemID, victimCharacterID, victimCorporationID, "
        "victimAllianceID, victimFactionID, victimShipTypeID, victimDamageTaken, "
        "finalCharacterID, finalCorporationID, finalAllianceID, finalFactionID, "
        "finalShipTypeID, finalWeaponTypeID, finalSecurityStatus, finalDamageDone, "
        "killTime, moonID FROM chrKillTable "
        "WHERE (victimCorporationID = %u OR finalCorporationID = %u)";

    if (beforeKillID > 0)
        query += " AND killID < " + std::string(itoa(beforeKillID));

    query += " ORDER BY killID DESC LIMIT 2500";

    if (!sDatabase.RunQuery(res, query.c_str(), corporationID, corporationID))
    {
        sLog.Error( "APICorporationManager::_KillMails()", "ERROR: Query failed: %s", res.error.c_str() );
        return BuildErrorXMLResponse( "999", "Query failed." );
    }

    std::string xml;
    xml.append("<?xml version='1.0' encoding='UTF-8'?>\n");
    xml.append("<eveapi version=\"2\">\n");
    xml.append("  <currentTime>" + Win32TimeToString(static_cast<uint64>(EvilTimeNow().get_int())) + "</currentTime>\n");
    xml.append("  <result>\n");
    xml.append("    <kills>\n");

    DBResultRow row;
    while (res.GetRow(row))
    {
        xml.append("      <row killID=\"" + std::string(itoa(row.GetUInt(0))) + "\"");
        xml.append(" solarSystemID=\"" + std::string(itoa(row.GetUInt(1))) + "\"");
        xml.append(" victimCharacterID=\"" + std::string(itoa(row.GetUInt(2))) + "\"");
        xml.append(" victimCorporationID=\"" + std::string(itoa(row.GetUInt(3))) + "\"");
        xml.append(" victimAllianceID=\"" + std::string(itoa(row.GetInt(4))) + "\"");
        xml.append(" victimFactionID=\"" + std::string(itoa(row.GetUInt(5))) + "\"");
        xml.append(" victimShipTypeID=\"" + std::string(itoa(row.GetUInt(6))) + "\"");
        xml.append(" victimDamageTaken=\"" + std::string(itoa(row.GetUInt(7))) + "\"");
        xml.append(" finalCharacterID=\"" + std::string(itoa(row.GetUInt(8))) + "\"");
        xml.append(" finalCorporationID=\"" + std::string(itoa(row.GetUInt(9))) + "\"");
        xml.append(" finalAllianceID=\"" + std::string(itoa(row.GetInt(10))) + "\"");
        xml.append(" finalFactionID=\"" + std::string(itoa(row.GetUInt(11))) + "\"");
        xml.append(" finalShipTypeID=\"" + std::string(itoa(row.GetUInt(12))) + "\"");
        xml.append(" finalWeaponTypeID=\"" + std::string(itoa(row.GetUInt(13))) + "\"");
        xml.append(" finalSecurityStatus=\"" + row.GetText(14) + "\"");
        xml.append(" finalDamageDone=\"" + std::string(itoa(row.GetUInt(15))) + "\"");
        xml.append(" killTime=\"" + row.GetText(16) + "\"");
        xml.append(" moonID=\"" + std::string(itoa(row.GetUInt(17))) + "\"/>\n");
    }

    xml.append("    </kills>\n");
    xml.append("  </result>\n");
    xml.append("</eveapi>\n");

    return std::tr1::shared_ptr<std::string>(new std::string(xml));
}
