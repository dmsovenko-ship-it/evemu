/**
 * @name AllianceDB.cpp
 *      database methods for alliance data
 *
 * @author Allan
 * @updates James
 * @date 24 May 2019
 *
 */

#include "Client.h"
#include "StaticDataMgr.h"
#include "character/Character.h"
#include "alliance/AllianceDB.h"

void AllianceDB::AddBulletin(uint32 allyID, uint32 ownerID, uint32 cCharID, const std::string &title, const std::string &body)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "INSERT INTO alnBulletins (allianceID, ownerID, createCharacterID, createDateTime, editCharacterID, editDateTime, title, body)"
                       " VALUES (%u, %u, %u, %f, %u, %f, '%s', '%s')",
                       allyID, ownerID, cCharID, GetFileTimeNow(), cCharID, GetFileTimeNow(), title.c_str(), body.c_str());
}

void AllianceDB::EditBulletin(uint32 bulletinID, uint32 eCharID, int64 eDataTime, std::string &title, std::string &body)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "UPDATE alnBulletins SET editCharacterID = %u, editDateTime = %lli, title = '%s', body = '%s'"
                       " WHERE bulletinID = %u",
                       eCharID, eDataTime, title.c_str(), body.c_str(), bulletinID);
}

void AllianceDB::DeleteBulletin(uint32 bulletinID)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "DELETE from alnBulletins "
                       " WHERE bulletinID = %u",
                       bulletinID);
}

PyRep *AllianceDB::GetBulletins(uint32 allyID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT allianceID, bulletinID, ownerID, createCharacterID, createDateTime, editCharacterID, editDateTime, title, body"
                            " FROM alnBulletins"
                            " WHERE allianceID = %u ",
                            allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }
    return DBResultToCRowset(res);
}

PyRep *AllianceDB::GetAlliance(uint32 allyID)
{
    // called by alliance member
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            " SELECT "
                            " a.allianceID, a.allianceName, a.description, a.typeID, a.shortName, a.executorCorpID, a.creatorCorpID, "
                            " a.creatorCharID, a.startDate, a.memberCount, a.url, a.deleted, 0 as dictatorial, "
                            " COALESCE(a.taxRate, 0.0) as taxRate "
                            " FROM alnAlliance AS a"
                            " WHERE allianceID = %u",
                            allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in retrieving alliance's data (%u)", allyID);
        return nullptr;
    }

    DBResultRow row;
    if (!res.GetRow(row))
    {
        codelog(ALLY__DB_WARNING, "Unable to find alliance's data (%u)", allyID);
        // Return an empty CRowset for missing alliances (e.g. faction system sov with allianceID=0)
        DBRowDescriptor *header = new DBRowDescriptor();
        header->AddColumn("allianceID", DBTYPE_I4);
        header->AddColumn("allianceName", DBTYPE_STR);
        header->AddColumn("description", DBTYPE_STR);
        header->AddColumn("typeID", DBTYPE_I4);
        header->AddColumn("shortName", DBTYPE_STR);
        header->AddColumn("executorCorpID", DBTYPE_I4);
        header->AddColumn("creatorCorpID", DBTYPE_I4);
        header->AddColumn("creatorCharID", DBTYPE_I8);
        header->AddColumn("startDate", DBTYPE_FILETIME);
        header->AddColumn("memberCount", DBTYPE_I4);
        header->AddColumn("url", DBTYPE_STR);
        header->AddColumn("deleted", DBTYPE_BOOL);
        header->AddColumn("dictatorial", DBTYPE_BOOL);
        header->AddColumn("taxRate", DBTYPE_R8);
        return new CRowSet(&header);
    }

    //return DBRowToRow(row);
    return DBRowToPackedRow(row);
    //return DBResultToRowset(res);
}

PyRep *AllianceDB::GetMyApplications(uint32 corpID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            " SELECT applicationID, corporationID, allianceID, applicationText, "
                            " state, applicationDateTime, deleted "
                            " FROM alnApplications"
                            " WHERE corporationID = %u ",
                            corpID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    PyObjectEx *obj = DBResultToCIndexedRowset(res, "corporationID");
    if (is_log_enabled(CORP__RSP_DUMP))
        obj->Dump(CORP__RSP_DUMP, "");

    return obj;
}

PyRep *AllianceDB::GetApplications(uint32 allyID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            " SELECT applicationID, corporationID, allianceID, applicationText, "
                            " state, applicationDateTime, deleted "
                            " FROM alnApplications"
                            " WHERE allianceID = %u AND state NOT IN (%u, %u)",
                            allyID, EveAlliance::AppStatus::AppAccepted, EveAlliance::AppStatus::AppRejected))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }
    PyObjectEx *obj = DBResultToCIndexedRowset(res, "corporationID");
    if (is_log_enabled(CORP__RSP_DUMP))
        obj->Dump(CORP__RSP_DUMP, "");

    return obj;
}

bool AllianceDB::GetCurrentApplicationInfo(uint32 allyID, uint32 corpID, Alliance::ApplicationInfo &aInfo)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            " SELECT applicationID, corporationID, allianceID, applicationText, "
                            " state, applicationDateTime, deleted "
                            " FROM alnApplications"
                            " WHERE corporationID = %u AND allianceID = %u",
                            corpID, allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        aInfo.valid = false;
        return false;
    }

    DBResultRow row;
    if (!res.GetRow(row))
    {
        codelog(ALLY__DB_WARNING, "There's no previous application.");
        aInfo.valid = false;
        return false;
    }

    aInfo.appID = row.GetInt(0);
    aInfo.allyID = allyID;
    aInfo.corpID = corpID;
    aInfo.appText = row.GetText(3);
    aInfo.state = row.GetInt(4);
    aInfo.appTime = row.GetInt64(5);
    aInfo.deleted = row.GetInt(6);
    aInfo.valid = true;
    return true;
}

bool AllianceDB::InsertApplication(Alliance::ApplicationInfo &aInfo)
{
    if (!aInfo.valid)
    {
        _log(ALLY__DB_WARNING, "InsertApplication(): aInfo contains invalid data");
        return false;
    }

    std::string escaped;
    sDatabase.DoEscapeString(escaped, aInfo.appText);
    DBerror err;
    if (!sDatabase.RunQueryLID(err, aInfo.appID,
                               " INSERT INTO alnApplications"
                               " (corporationID, allianceID, applicationText, state, applicationDateTime)"
                               " VALUES (%u, %u, '%s', %u, %lli)",
                               aInfo.corpID, aInfo.allyID, escaped.c_str(), aInfo.state, GetFileTimeNow()))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", err.c_str());
        return false;
    }

    return true;
}

bool AllianceDB::UpdateApplication(const Alliance::ApplicationInfo &aInfo)
{
    if (!aInfo.valid)
    {
        _log(ALLY__DB_WARNING, "UpdateApplication(): info contains invalid data");
        return false;
    }

    DBerror err;
    std::string escaped;
    sDatabase.DoEscapeString(escaped, aInfo.appText);
    if (!sDatabase.RunQuery(err,
                            " UPDATE alnApplications"
                            " SET state = %u, applicationText = '%s'"
                            " WHERE corporationID = %u and state = 1",
                            aInfo.state, escaped.c_str(), aInfo.corpID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", err.c_str());
        return false;
    }
    return true;
}

bool AllianceDB::DeleteApplication(const Alliance::ApplicationInfo &aInfo)
{
    DBerror err;
    if (!sDatabase.RunQuery(err,
                            " DELETE FROM alnApplications"
                            " WHERE corporationID = %u and allianceID = %u ",
                            aInfo.corpID, aInfo.allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", err.c_str());
        return false;
    }
    return true;
}

PyRep *AllianceDB::GetContacts(uint32 allyID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT contactID, inWatchlist, relationshipID, labelMask"
                            " FROM alnContacts WHERE ownerID = %u",
                            allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    PyObjectEx *obj = DBResultToCIndexedRowset(res, "contactID");
    if (is_log_enabled(CORP__RSP_DUMP))
        obj->Dump(CORP__RSP_DUMP, "");

    return obj;
}

void AllianceDB::AddContact(uint32 ownerID, int32 contactID, int32 relationshipID)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "INSERT INTO alnContacts (ownerID, contactID, relationshipID, "
                       " inWatchlist, labelMask) VALUES "
                       " (%u, %u, %i, 0, 0) ",
                       ownerID, contactID, relationshipID);
}

void AllianceDB::UpdateContact(int32 relationshipID, uint32 contactID, uint32 ownerID)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "UPDATE alnContacts SET relationshipID=%i "
                       " WHERE contactID=%u AND ownerID=%u ",
                       relationshipID, contactID, ownerID);
}

void AllianceDB::RemoveContact(uint32 contactID, uint32 ownerID)
{
    DBerror err;
    sDatabase.RunQuery(err,
                       "DELETE from alnContacts "
                       " WHERE contactID=%u AND ownerID=%u ",
                       contactID, ownerID);
}

void AllianceDB::UpdateContactLabelMask(uint32 ownerID, uint32 contactID, uint32 mask, bool add)
{
    DBerror err;
    if (add) {
        sDatabase.RunQuery(err,
            "UPDATE alnContacts SET labelMask = labelMask | %u"
            " WHERE contactID=%u AND ownerID=%u",
            mask, contactID, ownerID);
    } else {
        sDatabase.RunQuery(err,
            "UPDATE alnContacts SET labelMask = labelMask & ~%u"
            " WHERE contactID=%u AND ownerID=%u",
            mask, contactID, ownerID);
    }
}

PyRep *AllianceDB::GetLabels(uint32 allyID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT labelID, color, name FROM alnLabels WHERE ownerID = %u", allyID))
    {
        codelog(DATABASE__ERROR, "Error on query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToCIndexedRowset(res, "labelID");
}

void AllianceDB::SetLabel(uint32 allyID, uint32 color, std::string name)
{
    std::string escaped;
    sDatabase.DoEscapeString(escaped, name);

    DBQueryResult res;
    sDatabase.RunQuery(res, "INSERT INTO alnLabels (color, name, ownerID) VALUES (%u, '%s', %u)", color, escaped.c_str(), allyID);
}

void AllianceDB::DeleteLabel(uint32 allyID, uint32 labelID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "DELETE FROM alnLabels WHERE ownerID = %u AND labelID = %u",
        allyID, labelID);
}

void AllianceDB::EditLabel(uint32 allyID, uint32 labelID, uint32 color, std::string name)
{
    std::string escaped;
    sDatabase.DoEscapeString(escaped, name);
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE alnLabels SET color = %u, name = '%s' WHERE ownerID = %u AND labelID = %u",
        color, escaped.c_str(), allyID, labelID);
}

bool AllianceDB::AddEmployment(uint32 allyID, uint32 corpID)
{
    DBerror err;
    if (!sDatabase.RunQuery(err,
                            "INSERT INTO crpEmployment"
                            "  (allianceID, corporationID, startDate)"
                            " VALUES (%u, %u, %f)",
                            allyID, corpID, GetFileTimeNow()))
    {
        codelog(DATABASE__ERROR, "Error in employment insert query: %s", err.c_str());
    }

    if (!sDatabase.RunQuery(err, "UPDATE alnAlliance SET memberCount = memberCount+1 WHERE allianceID = %u", allyID))
        codelog(ALLY__DB_ERROR, "Error in new corp member increase query: %s", err.c_str());
    return true;
}

PyRep *AllianceDB::GetEmploymentRecord(uint32 corpID)
{
    DBQueryResult res;
    //do we really need this order by??
    if (!sDatabase.RunQuery(res,
                            "SELECT startDate, allianceID, deleted "
                            "   FROM crpEmployment "
                            "   WHERE corporationID = %u "
                            "   ORDER BY startDate DESC",
                            corpID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

uint32 AllianceDB::GetExecutorID(uint32 allyID)
{
    uint32 executorID = 0;
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT executorCorpID FROM alnAlliance "
                            " WHERE allianceID = %u",
                            allyID))
    {
        codelog(DATABASE__ERROR, "Error in checking current alliance executorCorpID: %s", res.error.c_str());
    }

    DBResultRow row;
    while (res.GetRow(row))
    {
        executorID = row.GetUInt(0);
    }

    return executorID;
}

bool AllianceDB::UpdateCorpAlliance(uint32 allyID, uint32 corpID)
{
    uint32 executorID = GetExecutorID(allyID);

    DBerror err;
    if (!sDatabase.RunQuery(err,
                            "UPDATE crpCorporation SET "
                            "  allianceID = %u, "
                            "  allianceMemberStartDate = %f, "
                            "  chosenExecutorID = %u "
                            " WHERE corporationID = %u",
                            allyID, GetFileTimeNow(), executorID, corpID))
    {
        codelog(DATABASE__ERROR, "Error in corp alliance update query: %s", err.c_str());
    }
    return true;
}

void AllianceDB::DeleteMember(uint32 allyID, uint32 corpID)
{
    DBerror err;
    if (!sDatabase.RunQuery(err,
                            "UPDATE crpCorporation SET "
                            "  allianceID = 0, "
                            "  allianceMemberStartDate = 0, "
                            "  chosenExecutorID = 0 "
                            " WHERE corporationID = %u",
                            corpID))
    {
        codelog(DATABASE__ERROR, "Error in deleting corp from alliance: %s", err.c_str());
    }

    if (!sDatabase.RunQuery(err, "UPDATE alnAlliance SET memberCount = memberCount-1 WHERE allianceID = %u", allyID))
        codelog(ALLY__DB_ERROR, "Error in alliance member decrease query: %s", err.c_str());
}

void AllianceDB::DeclareExecutorSupport(uint32 corpID, uint32 chosenExecutor)
{
    DBerror err;
    if (!sDatabase.RunQuery(err,
                            "UPDATE crpCorporation SET "
                            "  chosenExecutorID = %u "
                            " WHERE corporationID = %u",
                            chosenExecutor, corpID))
    {
        codelog(DATABASE__ERROR, "Error in setting chosenExecutor: %s", err.c_str());
    }
}

bool AllianceDB::IsShortNameTaken(std::string shortName)
{
    DBQueryResult res;
    sDatabase.RunQuery(res, " SELECT allianceID FROM alnAlliance WHERE shortName = '%s'", shortName.c_str());
    return (res.GetRowCount() != 0);
}

void AllianceDB::UpdateAlliance(uint32 allyID, std::string description, std::string url)
{
    std::string aDesc, aURL;
    sDatabase.DoEscapeString(aDesc, description);
    sDatabase.DoEscapeString(aURL, url);

    DBerror err;

    if (!sDatabase.RunQuery(err,
                            " UPDATE alnAlliance "
                            "  SET description='%s', url='%s' "
                            " WHERE allianceID=%u ",
                            aDesc.c_str(), aURL.c_str(), allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in UpdateAlliance query: %s", err.c_str());
    }
}

void AllianceDB::SetTaxRate(uint32 allyID, double taxRate)
{
    DBerror err;
    taxRate = std::clamp(taxRate, 0.0, 1.0);
    if (!sDatabase.RunQuery(err,
                            " UPDATE alnAlliance "
                            "  SET taxRate=%.2f "
                            " WHERE allianceID=%u ",
                            taxRate, allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in SetTaxRate query: %s", err.c_str());
    }
}

bool AllianceDB::CreateAlliance(std::string name, std::string shortName, std::string description, std::string url, Client *pClient, uint32 &allyID, uint32 &corpID)
{
    std::string aName, aShort, aDesc, aURL;
    sDatabase.DoEscapeString(aName, name);
    sDatabase.DoEscapeString(aShort, shortName);
    sDatabase.DoEscapeString(aDesc, description);
    sDatabase.DoEscapeString(aURL, url);

    Character *pChar = pClient->GetChar().get();
    uint32 charID = pClient->GetCharacterID();
    corpID = pClient->GetCorporationID();

    DBerror err;

    if (!sDatabase.RunQueryLID(err, allyID,
                               " INSERT INTO alnAlliance ( "
                               "   allianceName, shortName, description, executorCorpID, creatorCorpID, creatorCharID, "
                               "   startDate, memberCount, url )"
                               " VALUES "
                               "   ('%s', '%s', '%s', %u, %u, %u, %f, 0, '%s') ",
                               aName.c_str(), aShort.c_str(), aDesc.c_str(), corpID, corpID, charID, GetFileTimeNow(), aURL.c_str()))
    {
        codelog(ALLY__DB_ERROR, "Error in CreateAlliance query: %s", err.c_str());
        return false;
    }
    // It has to go into the eveStaticOwners too
    sDatabase.RunQuery(err, " INSERT INTO eveStaticOwners (ownerID,ownerName,typeID) VALUES (%u, '%s', 16159)", allyID, aName.c_str());

    return true;
}

PyRep *AllianceDB::GetMembers(uint32 allyID) //to be called by member of alliance
{
    //This function is called to gather all of the corporationIDs associated to a particular alliance
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT c.corporationID, c.corporationName, c.tickerName,"
                            "       c.ceoID, c.memberCount, c.taxRate, c.stationID,"
                            "       c.allianceMemberStartDate, c.chosenExecutorID"
                            " FROM crpCorporation AS c"
                            " WHERE c.allianceID = %u AND c.deleted = 0",
                            allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    PyObject *obj = DBResultToIndexRowset(res, "corporationID");
    if (is_log_enabled(CORP__RSP_DUMP))
        obj->Dump(CORP__RSP_DUMP, "");

    return obj;
}

PyRep *AllianceDB::GetAllianceMembers(uint32 allyID) //to be called from show details pane
{
    //This function is called to gather all of the corporationIDs associated to a particular alliance
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT c.corporationID, c.corporationName, c.tickerName,"
                            "       c.ceoID, c.memberCount, c.taxRate, c.stationID,"
                            "       c.allianceMemberStartDate, c.chosenExecutorID"
                            " FROM crpCorporation AS c"
                            " WHERE c.allianceID = %u AND c.deleted = 0",
                            allyID))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

// Not sure how alliances but for now this will simply return an ordered list based upon member count
PyRep *AllianceDB::GetRankedAlliances()
{
    //This function is called to gather all of the corporationIDs associated to a particular alliance
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
                            "SELECT allianceID,allianceName,executorCorpID, "
                            " description, typeID, shortName, creatorCorpID, "
                            " creatorCharID, startDate, memberCount, url, deleted "
                            " from alnAlliance order by memberCount "))
    {
        codelog(ALLY__DB_ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    return DBResultToRowset(res);
}

bool AllianceDB::CreateAllianceChangePacket(OnAllianceChanged &ac, uint32 oldAllyID, uint32 newAllyID)
{
    // New Alliance \/
    if (newAllyID == 0)
    {
        ac.allianceIDNew = PyStatic.NewNone();
        ac.allianceNameNew = PyStatic.NewNone();
        ac.descriptionNew = PyStatic.NewNone();
        ac.typeIDNew = PyStatic.NewNone();
        ac.shortNameNew = PyStatic.NewNone();
        ac.executorCorpIDNew = PyStatic.NewNone();
        ac.creatorCorpIDNew = PyStatic.NewNone();
        ac.creatorCharIDNew = PyStatic.NewNone();
        ac.startDateNew = PyStatic.NewNone();
        ac.memberCountNew = PyStatic.NewNone();
        ac.urlNew = PyStatic.NewNone();
        ac.deletedNew = PyStatic.NewNone();
        ac.dictatorialNew = PyStatic.NewNone();
    }
    else
    {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
                                " SELECT "
                                " a.allianceID, a.allianceName, a.description, a.typeID, a.shortName, a.executorCorpID, a.creatorCorpID, "
                                " a.creatorCharID, a.startDate, a.memberCount, a.url, a.deleted, 0 as dictatorial " //Dictatorial is not used in Crucible but must be set
                                " FROM alnAlliance AS a"
                                " WHERE allianceID = %u",
                                newAllyID))
        {
            codelog(ALLY__DB_ERROR, "Error in retrieving new alliance's data (%u)", newAllyID);
            return false;
        }

        DBResultRow row;
        if (!res.GetRow(row))
        {
            codelog(ALLY__DB_WARNING, "Unable to find new alliance's data (%u)", newAllyID);
            return false;
        }

        ac.allianceIDNew = new PyInt(row.GetUInt(0));
        ac.allianceNameNew = new PyString(row.GetText(1));
        ac.descriptionNew = new PyString(row.GetText(2));
        ac.typeIDNew = new PyInt(row.GetUInt(3));
        ac.shortNameNew = new PyString(row.GetText(4));
        ac.executorCorpIDNew = new PyInt(row.GetUInt(5));
        ac.creatorCorpIDNew = new PyInt(row.GetUInt(6));
        ac.creatorCharIDNew = new PyInt(row.GetUInt(7));
        ac.startDateNew = new PyLong(row.GetInt64(8));
        ac.memberCountNew = new PyInt(row.GetUInt(9));
        ac.urlNew = new PyString(row.GetText(10));
        ac.deletedNew = new PyInt(row.GetInt(11));
        ac.dictatorialNew = new PyInt(row.GetInt(12));
    }

    // Old Alliance \/
    if (oldAllyID == 0)
    {
        ac.allianceIDOld = PyStatic.NewNone();
        ac.allianceNameOld = PyStatic.NewNone();
        ac.descriptionOld = PyStatic.NewNone();
        ac.typeIDOld = PyStatic.NewNone();
        ac.shortNameOld = PyStatic.NewNone();
        ac.executorCorpIDOld = PyStatic.NewNone();
        ac.creatorCorpIDOld = PyStatic.NewNone();
        ac.creatorCharIDOld = PyStatic.NewNone();
        ac.startDateOld = PyStatic.NewNone();
        ac.memberCountOld = PyStatic.NewNone();
        ac.urlOld = PyStatic.NewNone();
        ac.deletedOld = PyStatic.NewNone();
        ac.dictatorialOld = PyStatic.NewNone();
    }
    else
    {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
                                " SELECT "
                                " a.allianceID, a.allianceName, a.description, a.typeID, a.shortName, a.executorCorpID, a.creatorCorpID, "
                                " a.creatorCharID, a.startDate, a.memberCount, a.url, a.deleted, 0 as dictatorial " //Dictatorial is not used in Crucible but must be set
                                " FROM alnAlliance AS a"
                                " WHERE allianceID = %u",
                                oldAllyID))
        {
            codelog(ALLY__DB_ERROR, "Error in retrieving old alliance's data (%u)", oldAllyID);
            return false;
        }

        DBResultRow row;
        if (!res.GetRow(row))
        {
            codelog(ALLY__DB_WARNING, "Unable to find old alliance's data (%u)", oldAllyID);
            return false;
        }
        ac.allianceIDOld = new PyInt(row.GetUInt(0));
        ac.allianceNameOld = new PyString(row.GetText(1));
        ac.descriptionOld = new PyString(row.GetText(2));
        ac.typeIDOld = new PyInt(row.GetUInt(3));
        ac.shortNameOld = new PyString(row.GetText(4));
        ac.executorCorpIDOld = new PyInt(row.GetUInt(5));
        ac.creatorCorpIDOld = new PyInt(row.GetUInt(6));
        ac.creatorCharIDOld = new PyInt(row.GetUInt(7));
        ac.startDateOld = new PyLong(row.GetInt64(8));
        ac.memberCountOld = new PyInt(row.GetUInt(9));
        ac.urlOld = new PyString(row.GetText(10));
        ac.deletedOld = new PyInt(row.GetInt(11));
        ac.dictatorialOld = new PyInt(row.GetInt(12));
    }

    return true;
}

// ── Alliance Voting ──────────────────────────────────────────────

bool AllianceDB::AddVoteCase(uint32 allyID, const std::string& voteCaseText, const std::string& description,
                             uint32 voteType, int64 startDateTime, int64 endDateTime, PyRep* options)
{
    DBerror err;
    uint32 voteCaseID = 0;
    if (!sDatabase.RunQueryLID(err, voteCaseID,
        "INSERT INTO alnVoteItems (allianceID, voteType, voteCaseText, description, inEffect, status,"
        " startDateTime, endDateTime, actedUpon, timeActedUpon, rescended, timeRescended, votesMade, votesProxied)"
        " VALUES (%u, %u, '%s', '%s', 1, 2, %lli, %lli, 0, 0, 0, 0, 0, 0)",
        allyID, voteType, voteCaseText.c_str(), description.c_str(), startDateTime, endDateTime))
    {
        codelog(ALLY__DB_ERROR, "AddVoteCase: %s", err.c_str());
        return false;
    }

    if (options != nullptr && options->IsObject()) {
        PyObjectEx* obj = options->AsObject();
        PyDict* args = obj->GetArgs();
        PyList* lines = nullptr;
        PyDictEntry* entry = args->GetEntry("lines");
        if (entry != nullptr && entry->second != nullptr)
            lines = entry->second->AsList();
        if (lines != nullptr) {
            for (size_t i = 0; i < lines->size(); ++i) {
                PyList* optLine = lines->GetItem(i)->AsList();
                if (optLine == nullptr || optLine->size() < 5) continue;
                std::string optText = PyRep::StringContent(optLine->GetItem(0));
                int8 optID = (int8)PyRep::IntegerValue(optLine->GetItem(1));
                int32 param = PyRep::IntegerValue(optLine->GetItem(2));
                int32 param1 = PyRep::IntegerValue(optLine->GetItem(3));
                int32 param2 = PyRep::IntegerValue(optLine->GetItem(4));
                sDatabase.RunQuery(err,
                    "INSERT INTO alnVoteOptions (voteCaseID, optionID, optionText, parameter,"
                    " parameter1, parameter2, votesFor) VALUES (%u, %u, '%s', %i, %i, %i, 0)",
                    voteCaseID, optID, optText.c_str(), param, param1, param2);
            }
        }
    }
    return true;
}

PyRep* AllianceDB::GetVoteItems(uint32 allyID, uint32 status, uint32 maxLen)
{
    DBQueryResult res;
    std::string query = "SELECT voteCaseID, voteType, voteCaseText, description, inEffect, status,"
                        " actedUpon, timeActedUpon, rescended, timeRescended, startDateTime, endDateTime"
                        " FROM alnVoteItems WHERE allianceID = " + std::to_string(allyID);
    if (status != 0)
        query += " AND status = " + std::to_string(status);
    query += " ORDER BY voteCaseID DESC";
    if (maxLen != 0)
        query += " LIMIT " + std::to_string(maxLen);

    if (!sDatabase.RunQuery(res, query.c_str())) {
        codelog(ALLY__DB_ERROR, "GetVoteItems: %s", res.error.c_str());
        return nullptr;
    }
    return DBResultToPackedRowDict(res, "voteCaseID");
}

PyRep* AllianceDB::GetVoteOptions(uint32 voteCaseID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT voteCaseID, optionID, optionText, parameter, votesFor,"
        " 0 AS votesMade, 0 AS votesProxied, parameter1, parameter2"
        " FROM alnVoteOptions WHERE voteCaseID = %u", voteCaseID))
    {
        codelog(ALLY__DB_ERROR, "GetVoteOptions: %s", res.error.c_str());
        return nullptr;
    }
    return DBResultToIndexRowset(res, "optionID");
}

PyRep* AllianceDB::GetVotes(uint32 voteCaseID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT corpID, voteCaseID, optionID FROM alnVotes WHERE voteCaseID = %u", voteCaseID))
    {
        codelog(ALLY__DB_ERROR, "GetVotes: %s", res.error.c_str());
        return nullptr;
    }
    return DBResultToCIndexedRowset(res, "corpID");
}

bool AllianceDB::CastVote(uint32 corpID, uint32 allyID, uint32 voteCaseID, uint8 optionID)
{
    DBerror err;
    if (!sDatabase.RunQuery(err,
        "UPDATE alnVoteItems SET votesMade = votesMade + 1 WHERE voteCaseID = %u", voteCaseID))
    {
        codelog(ALLY__DB_ERROR, "CastVote items: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err,
        "UPDATE alnVoteOptions SET votesFor = votesFor + 1 WHERE voteCaseID = %u AND optionID = %u",
        voteCaseID, optionID))
    {
        codelog(ALLY__DB_ERROR, "CastVote options: %s", err.c_str());
        return false;
    }
    if (!sDatabase.RunQuery(err,
        "INSERT INTO alnVotes (corpID, allianceID, voteCaseID, optionID) VALUES (%u, %u, %u, %u)",
        corpID, allyID, voteCaseID, optionID))
    {
        codelog(ALLY__DB_ERROR, "CastVote insert: %s", err.c_str());
        return false;
    }
    return true;
}

bool AllianceDB::ResolveVote(uint32 voteCaseID)
{
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT optionID, parameter, votesFor FROM alnVoteOptions WHERE voteCaseID = %u"
        " ORDER BY votesFor DESC LIMIT 1", voteCaseID);
    DBResultRow row;
    if (!res.GetRow(row)) return false;

    uint32 winnerID = row.GetUInt(1);
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE alnVoteItems SET status = 0, actedUpon = 1, timeActedUpon = %f WHERE voteCaseID = %u",
        GetFileTimeNow(), voteCaseID);

    sDatabase.RunQuery(res, "SELECT allianceID, voteType FROM alnVoteItems WHERE voteCaseID = %u", voteCaseID);
    if (!res.GetRow(row)) return false;
    uint32 allyID = row.GetUInt(0);
    uint32 voteType = row.GetUInt(1);

    if (voteType == 0 && winnerID > 0) {
        sDatabase.RunQuery(err,
            "UPDATE alnAlliance SET executorCorpID = %u WHERE allianceID = %u", winnerID, allyID);
        sDatabase.RunQuery(err,
            "UPDATE crpCorporation SET chosenExecutorID = %u WHERE allianceID = %u", winnerID, allyID);
    }
    return true;
}

void AllianceDB::ProcessVoteExpiry(uint32 allyID)
{
    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT voteCaseID FROM alnVoteItems"
        " WHERE allianceID = %u AND inEffect = 1 AND endDateTime > 0 AND endDateTime < %f",
        allyID, GetFileTimeNow());

    DBResultRow row;
    while (res.GetRow(row))
        ResolveVote(row.GetUInt(0));
}