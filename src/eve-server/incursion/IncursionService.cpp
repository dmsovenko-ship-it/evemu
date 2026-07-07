#include "eve-server.h"

#include "incursion/IncursionService.h"

IncursionService::IncursionService() :
    Service("incursion")
{
    this->Add("GetDelayedRewardsByGroupIDs", &IncursionService::GetDelayedRewardsByGroupIDs);
    this->Add("GetIncursions", &IncursionService::GetIncursions);
}

PyResult IncursionService::GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyList* rewardGroupIDs)
{
    PyDict* result = new PyDict();
    for (auto itr = rewardGroupIDs->begin(); itr != rewardGroupIDs->end(); ++itr) {
        if (!(*itr)->IsInt())
            continue;

        uint32 groupID = (*itr)->AsInt()->value();
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT rewardTypeID, rewardQuantity, lpTypeID, lpAmount "
            "FROM incursionRewards WHERE rewardGroupID = %u", groupID))
        {
            PyList* rewardList = new PyList();
            DBResultRow row;
            while (res.GetRow(row)) {
                PyDict* reward = new PyDict();
                reward->SetItemString("rewardTypeID",  new PyInt(row.GetUInt(0)));
                reward->SetItemString("rewardQuantity", new PyInt(row.GetUInt(1)));
                reward->SetItemString("lpTypeID",      new PyInt(row.GetUInt(2)));
                reward->SetItemString("lpAmount",       new PyInt(row.GetUInt(3)));
                rewardList->AddItem(new PyObject("util.KeyVal", reward));
            }
            result->SetItem(*itr, rewardList);
        }
    }
    return result;
}

PyResult IncursionService::GetIncursions(PyCallArgs& call)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT stagingSolarSystemID, state, influence, hasBoss, rewardGroupID, taleID, "
        "graceTime, decayRate, lastUpdated"
        " FROM incursions WHERE state > 0"))
    {
        return new PyList();
    }

    PyList* incursions = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("stagingSolarSystemID", new PyInt(row.GetUInt(0)));
        entry->SetItemString("state",                new PyInt(row.GetUInt(1)));
        entry->SetItemString("influence",            new PyFloat(row.GetDouble(2)));
        entry->SetItemString("hasBoss",              new PyInt(row.GetUInt(3)));
        entry->SetItemString("rewardGroupID",        new PyInt(row.GetUInt(4)));
        entry->SetItemString("taleID",               new PyInt(row.GetUInt(5)));
        entry->SetItemString("graceTime",            new PyFloat(static_cast<double>(row.GetUInt(6))));
        entry->SetItemString("decayRate",            new PyFloat(row.GetDouble(7)));
        entry->SetItemString("lastUpdated",          new PyLong(row.GetInt64(8)));
        incursions->AddItem(new PyObject("util.KeyVal", entry));
    }

    return incursions;
}
