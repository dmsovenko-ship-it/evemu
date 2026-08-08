#include "eve-server.h"

#include "incursion/RewardMgrService.h"

RewardMgrService::RewardMgrService() :
    Service("rewardMgr")
{
    this->Add("GetDelayedRewardsByGroupIDs", &RewardMgrService::GetDelayedRewardsByGroupIDs);
    this->Add("GetRewardData", &RewardMgrService::GetRewardData);
    this->Add("GetRewardLPLogs", &RewardMgrService::GetRewardLPLogs);
}

PyResult RewardMgrService::GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyRep* rewardGroupIDs)
{
    PyDict* result = new PyDict();

    // Client may send PyList or PyTuple
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
                // Client expects an 'entries' field (list of sub-entries)
                reward->SetItemString("entries", new PyList());
                rewardList->AddItem(new PyObject("util.KeyVal", reward));
            }
            result->SetItem(item, rewardList);
        }
    }
    return result;
}

PyResult RewardMgrService::GetRewardData(PyCallArgs& call, PyInt* rewardID)
{
    uint32 id = rewardID->value();

    DBQueryResult res;
    PyDict* immediateRewards = new PyDict();
    PyDict* delayedRewards = new PyDict();

    // Client expects rewards grouped by rewardCriteria
    // Use rewardCriteriaAll as default (criteria 0 = all security bands)
    const uint32 criteriaAll = 0;

    if (sDatabase.RunQuery(res,
        "SELECT rewardTypeID, rewardQuantity, lpTypeID, lpAmount"
        " FROM incursionRewards WHERE rewardGroupID = %u", id))
    {
        PyList* rewardList = new PyList();
        DBResultRow row;
        while (res.GetRow(row)) {
            PyDict* reward = new PyDict();
            reward->SetItemString("rewardTypeID",  new PyInt(row.GetUInt(0)));
            reward->SetItemString("rewardQuantity", new PyInt(row.GetUInt(1)));
            reward->SetItemString("lpTypeID",      new PyInt(row.GetUInt(2)));
            reward->SetItemString("lpAmount",       new PyInt(row.GetUInt(3)));
            // entries: list of {playerCount, quantity} per player bracket
            PyDict* entry = new PyDict();
            entry->SetItemString("playerCount", new PyInt(1));
            entry->SetItemString("quantity", new PyInt(row.GetUInt(1)));
            PyList* entries = new PyList();
            entries->AddItem(new PyObject("util.KeyVal", entry));
            reward->SetItemString("entries", entries);
            rewardList->AddItem(new PyObject("util.KeyVal", reward));
            _log(DATABASE__MESSAGE, "RewardMgr::GetRewardData(%u) — row type=%u qty=%u", id, row.GetUInt(0), row.GetUInt(1));
        }
        immediateRewards->SetItem(new PyInt(criteriaAll), rewardList);
    }

    PyDict* result = new PyDict();
    result->SetItemString("immediateRewards", immediateRewards);
    result->SetItemString("delayedRewards", delayedRewards);
    result->SetItemString("rewardID", new PyInt(id));
    return new PyObject("util.KeyVal", result);
}

PyResult RewardMgrService::GetRewardLPLogs(PyCallArgs& call)
{
    // Returns LP reward logs for incursion participation
    // Client expects a list of LP log entries
    return new PyList();
}
