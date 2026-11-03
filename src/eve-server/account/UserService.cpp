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
    Author:        Bloody.Rabbit
*/

#include "eve-server.h"


#include "account/UserService.h"
#include "services/ServiceManager.h"

MovementService::MovementService(EVEServiceManager *mgr) :
    Service("movementServer"),
    m_manager(mgr)
{
    this->Add("ResolveNodeID", &MovementService::ResolveNodeID);
}

PyResult MovementService::ResolveNodeID(PyCallArgs& call, PyInt* newWorldSpaceId)
{
    sLog.Yellow( "MovementService", "ResolveNodeID" );
    call.Dump(CHARACTER__DEBUG);

    return new PyInt(this->m_manager->GetNodeID());
}

UserService::UserService() :
    Service("userSvc", eAccessLevel_User)
{
    this->Add("GetRedeemTokens", &UserService::GetRedeemTokens);
    this->Add("ReverseRedeem", &UserService::ReverseRedeem);
    this->Add("GetCreateDate", &UserService::GetCreateDate);
    this->Add("ReportISKSpammer", &UserService::ReportISKSpammer);
    this->Add("ReportBot", &UserService::ReportBot);
    this->Add("ApplyPilotLicence", &UserService::ApplyPilotLicence);
}

PyResult UserService::GetRedeemTokens(PyCallArgs& call)
{
    /*
    sLog.Yellow( "UserService", "Handle_GetRedeemTokens" );
    call.Dump(SERVICE__CALL_DUMP);
     * ==================== Sent from Client 84 bytes
     *
     * [PyObjectData Name: macho.CallReq]
     *  [PyTuple 7 items]
     *    [PyInt 6]
     *    [PyObjectData Name: macho.MachoAddress]
     *      [PyTuple 4 items]
     *        [PyInt 2]
     *        [PyInt 0]
     *        [PyIntegerVar 13]
     *        [PyNone]
     *    [PyObjectData Name: macho.MachoAddress]
     *      [PyTuple 3 items]
     *        [PyInt 8]
     *        [PyString "userSvc"]
     *        [PyNone]
     *    [PyInt 5654387]
     *    [PyTuple 1 items]
     *      [PyTuple 2 items]
     *        [PyInt 0]
     *        [PySubStream 31 bytes]
     *          [PyTuple 4 items]
     *            [PyInt 1]
     *            [PyString "GetRedeemTokens"]
     *            [PyTuple 0 items]
     *            [PyDict 1 kvp]
     *              [PyString "machoVersion"]
     *              [PyInt 1]
     *    [PyNone]
     *    [PyNone]
     *
     *
     *
     * ==================== Sent from Server 200 bytes [Compressed]
     *
     * [PyObjectData Name: macho.CallRsp]
     *  [PyTuple 7 items]
     *    [PyInt 7]
     *    [PyObjectData Name: macho.MachoAddress]
     *      [PyTuple 3 items]
     *        [PyInt 8]
     *        [PyString "userSvc"]
     *        [PyNone]
     *    [PyObjectData Name: macho.MachoAddress]
     *      [PyTuple 4 items]
     *        [PyInt 2]
     *        [PyIntegerVar 241241000001103]
     *        [PyIntegerVar 13]
     *        [PyNone]
     *    [PyInt 5654387]
     *    [PyTuple 1 items]
     *      [PySubStream 173 bytes]
     *        [PyObjectEx Type2]
     *          [PyTuple 2 items]
     *            [PyTuple 1 items]
     *              [PyToken dbutil.CRowset]
     *            [PyDict 1 kvp]
     *              [PyString "header"]
     *              [PyObjectEx Normal]
     *                [PyTuple 2 items]
     *                  [PyToken blue.DBRowDescriptor]
     *                  [PyTuple 1 items]
     *                    [PyTuple 10 items]
     *                      [PyTuple 2 items]
     *                        [PyString "tokenID"]
     *                        [PyInt 3]
     *                      [PyTuple 2 items]
     *                        [PyString "massTokenID"]
     *                        [PyInt 3]
     *                      [PyTuple 2 items]
     *                        [PyString "typeID"]
     *                        [PyInt 3]
     *                      [PyTuple 2 items]
     *                        [PyString "quantity"]
     *                        [PyInt 3]
     *                      [PyTuple 2 items]
     *                        [PyString "label"]
     *                        [PyInt 130]
     *                      [PyTuple 2 items]
     *                        [PyString "description"]
     *                        [PyInt 130]
     *                      [PyTuple 2 items]
     *                        [PyString "dateTime"]
     *                        [PyInt 64]
     *                      [PyTuple 2 items]
     *                        [PyString "expireDateTime"]
     *                        [PyInt 64]
     *                      [PyTuple 2 items]
     *                        [PyString "availableDateTime"]
     *                        [PyInt 64]
     *                      [PyTuple 2 items]
     *                        [PyString "stationID"]
     *                        [PyInt 3]
     *    [PyNone]
     *    [PyNone]
     *
     */

    return new PyList();
}

PyResult UserService::ReverseRedeem(PyCallArgs& call, PyInt* itemID)
{
    //sm.RemoteSvc('userSvc').ReverseRedeem(item.itemID)
    sLog.Yellow( "UserService", "Handle_ReverseRedeem" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
    
    /* {'messageKey': 'ReverseRedeemingCompleted', 'dataID': 17877681, 'suppressable': False, 'bodyID': 257344, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257343, 'messageID': 2917}
     * {'messageKey': 'ReverseRedeemingFatalError', 'dataID': 17877443, 'suppressable': False, 'bodyID': 257253, 'messageType': 'error', 'urlAudio': '', 'urlIcon': '', 'titleID': 257252, 'messageID': 2918}
     * {'messageKey': 'ReverseRedeemingIllegalType', 'dataID': 17877511, 'suppressable': False, 'bodyID': 257279, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257278, 'messageID': 2913}
     * {'messageKey': 'ReverseRedeemingOnlyInStation', 'dataID': 17877516, 'suppressable': False, 'bodyID': 257281, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257280, 'messageID': 2911}
     * {'messageKey': 'ReverseRedeemingYouDoNotOwnItem', 'dataID': 17877433, 'suppressable': False, 'bodyID': 257249, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 257248, 'messageID': 2914}
     */
}

PyResult UserService::GetCreateDate(PyCallArgs& call)
{
    return new PyLong(call.client->GetChar()->createDateTime());
}

PyResult UserService::ReportISKSpammer(PyCallArgs& call, PyInt* characterID, PyInt* channelID)
{
    // sm.RemoteSvc('userSvc').ReportISKSpammer(charID, channelID, spamEntries)
    sLog.Yellow( "UserService", "Handle_ReportISKSpammer" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
}

PyResult UserService::ReportBot(PyCallArgs& call, PyInt* itemID)
{
    sLog.Yellow( "UserService", "Handle_ReportBot" );
    call.Dump(CHARACTER__DEBUG);

    return nullptr;
}

PyResult UserService::ApplyPilotLicence(PyCallArgs& call, PyInt* itemID)
{
    sLog.Yellow("UserService", "Handle_ApplyPilotLicence");
    call.Dump(CHARACTER__DEBUG);

    uint32 plexID = (itemID != nullptr) ? (uint32)itemID->value() : 0;
    if (plexID == 0)
        return nullptr;

    Client* client = call.client;
    if (client == nullptr)
        return nullptr;
    uint32 charID = client->GetCharacterID();
    if (charID == 0)
        return nullptr;

    // The PLEX must be a real item owned by this character (hangar flag 4 or cargo flag 5)
    DBQueryResult pres;
    if (!sDatabase.RunQuery(pres,
            "SELECT itemID FROM entity WHERE itemID=%u AND ownerID=%u AND typeID=29668 AND quantity>0 LIMIT 1",
            plexID, charID))
        return nullptr;
    DBResultRow prow;
    if (!pres.GetRow(prow))
        return nullptr;   // not a PLEX / not ours

    // Resolve the account
    uint32 accountID = 0;
    DBQueryResult ares;
    if (sDatabase.RunQuery(ares, "SELECT accountID FROM chrCharacters WHERE characterID=%u", charID)) {
        DBResultRow arow;
        if (ares.GetRow(arow))
            accountID = arow.GetUInt(0);
    }
    if (accountID == 0)
        return nullptr;

    // Consume the PLEX
    DBerror err;
    sDatabase.RunQuery(err, "DELETE FROM entity WHERE itemID=%u", plexID);
    sDatabase.RunQuery(err, "DELETE FROM entity_attributes WHERE itemID=%u", plexID);

    // Extend the subscription (same math as the portal's char/ActivatePlex)
    uint32 days = (sConfig.subscription.PremiumDays > 0 ? sConfig.subscription.PremiumDays : 30);
    int64 add = (int64)days * 86400LL * 10000000LL;
    sDatabase.RunQuery(err,
        "UPDATE account SET premiumUntil = GREATEST(premiumUntil, (UNIX_TIMESTAMP()+11644473600)*10000000) + %lld WHERE accountID=%u",
        add, accountID);

    // The alpha flag cache in Character.cpp re-reads premiumUntil on a 60s TTL.

    client->SendNotifyMsg("30 Day Pilot's License Extension activated - account is now Omega for %u days.", days);
    return new PyInt((int)days);
}
