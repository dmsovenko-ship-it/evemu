#include "eve-server.h"

#include "incursion/IncursionService.h"

IncursionService::IncursionService() :
    Service("incursion")
{
    this->Add("GetDelayedRewardsByGroupIDs", &IncursionService::GetDelayedRewardsByGroupIDs);
    this->Add("GetIncursions", &IncursionService::GetIncursions);
}

PyResult IncursionService::GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyRep* rewardGroupIDs)
{
    PyDict* result = new PyDict();

    std::vector<PyRep*> items;
    if (rewardGroupIDs->IsList()) {
        PyList* list = rewardGroupIDs->AsList();
        for (auto itr = list->begin(); itr != list->end(); ++itr)
            items.push_back(*itr);
    } else if (rewardGroupIDs->IsTuple()) {
        PyTuple* tuple = rewardGroupIDs->AsTuple();
        for (size_t i = 0; i < tuple->size(); ++i)
            items.push_back(tuple->GetItem(i));
    }

    for (auto& item : items) {
        if (!item->IsInt())
            continue;

        uint32 groupID = item->AsInt()->value();
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
            result->SetItem(item, rewardList);
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
