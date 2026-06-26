#include "eve-server.h"

#include "standing/KillRightDB.h"

PyRep* KillRightDB::GetKillRights(uint32 ownerID, uint32 targetID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        " SELECT rightID, ownerID, targetID, price, accessMask, created, used "
        " FROM chrKillRights "
        " WHERE (ownerID = %u OR targetID = %u)"
        " AND used = 0 AND expiryDate > %lli",
        ownerID, targetID, GetFileTimeNow()))
    {
        codelog(DATABASE__ERROR, "Failed to query kill rights for %u", ownerID);
        return nullptr;
    }

    PyDict* killRights = new PyDict();
    PyDict* killedRights = new PyDict();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("rightID", new PyInt(row.GetInt(0)));
        entry->SetItemString("ownerID", new PyInt(row.GetInt(1)));
        entry->SetItemString("targetID", new PyInt(row.GetInt(2)));
        entry->SetItemString("price", new PyLong(row.GetInt64(3)));
        entry->SetItemString("accessMask", new PyInt(row.GetInt(4)));
        entry->SetItemString("created", new PyLong(row.GetInt64(5)));
        entry->SetItemString("used", new PyBool(row.GetInt(6) ? true : false));
        PyObject* obj = new PyObject("util.KeyVal", entry);

        uint32 rightOwner = row.GetInt(1);
        uint32 rightTarget = row.GetInt(2);
        if (rightOwner == ownerID) {
            killRights->SetItem(new PyInt(rightTarget), obj);
        } else if (rightTarget == ownerID) {
            killedRights->SetItem(new PyInt(rightOwner), obj);
        }
    }

    PyTuple* result = new PyTuple(2);
    result->SetItem(0, killRights);
    result->SetItem(1, killedRights);
    return result;
}

int32 KillRightDB::GrantKillRight(uint32 ownerID, uint32 targetID)
{
    // check if active right already exists
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
        " SELECT rightID FROM chrKillRights "
        " WHERE ownerID = %u AND targetID = %u AND used = 0 AND expiryDate > %lli",
        ownerID, targetID, GetFileTimeNow()))
    {
        DBResultRow row;
        if (res.GetRow(row))
            return row.GetInt(0); // already exists
    }

    DBerror err;
    int64 now = GetFileTimeNow();
    int64 expiry = now + 30LL * 24LL * 60LL * 60LL * 10000000LL; // 30 days
    uint32 rightID;
    if (!sDatabase.RunQueryLID(err, rightID,
        " INSERT INTO chrKillRights (ownerID, targetID, price, accessMask, created, expiryDate) "
        " VALUES (%u, %u, 0, 0, %lli, %lli)",
        ownerID, targetID, now, expiry))
    {
        codelog(DATABASE__ERROR, "Failed to grant kill right %u -> %u", ownerID, targetID);
        return 0;
    }
    return rightID;
}

bool KillRightDB::ActivateKillRight(uint32 rightID, uint32 activatedBy)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        " UPDATE chrKillRights SET used = 1, activatedBy = %u WHERE rightID = %u AND used = 0",
        activatedBy, rightID);
}

bool KillRightDB::UpdateKillRight(uint32 rightID, int64 price, uint8 accessMask)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        " UPDATE chrKillRights SET price = %" PRIi64 ", accessMask = %u WHERE rightID = %u",
        price, accessMask, rightID);
}

bool KillRightDB::DeleteKillRight(uint32 rightID)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        " DELETE FROM chrKillRights WHERE rightID = %u", rightID);
}
