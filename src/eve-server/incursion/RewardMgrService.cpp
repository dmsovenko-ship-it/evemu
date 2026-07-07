#include "eve-server.h"

#include "incursion/RewardMgrService.h"

RewardMgrService::RewardMgrService() :
    Service("rewardMgr")
{
    this->Add("GetDelayedRewardsByGroupIDs", &RewardMgrService::GetDelayedRewardsByGroupIDs);
    this->Add("GetRewardData", &RewardMgrService::GetRewardData);
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
                rewardList->AddItem(new PyObject("util.KeyVal", reward));
            }
            result->SetItem(item, rewardList);
        }
    }
    return result;
}

PyResult RewardMgrService::GetRewardData(PyCallArgs& call, PyInt* rewardID)
{
    // Returns reward data keyed by rewardID
    // Client expects immediateRewards/delayedRewards lists

    uint32 id = rewardID->value();

    DBQueryResult res;
    PyDict* immediateRewards = new PyDict();
    PyDict* delayedRewards = new PyDict();

    if (sDatabase.RunQuery(res,
        "SELECT rewardTypeID, rewardQuantity, lpTypeID, lpAmount"
        " FROM incursionRewards WHERE rewardGroupID = %u", id))
    {
        DBResultRow row;
        while (res.GetRow(row)) {
            uint32 rewardTypeID = row.GetUInt(0);
            PyDict* reward = new PyDict();
            reward->SetItemString("rewardTypeID",  new PyInt(rewardTypeID));
            reward->SetItemString("rewardQuantity", new PyInt(row.GetUInt(1)));
            reward->SetItemString("lpTypeID",      new PyInt(row.GetUInt(2)));
            reward->SetItemString("lpAmount",       new PyInt(row.GetUInt(3)));
            // Wrap in a list so client can iterate
            PyList* rewardList = new PyList();
            rewardList->AddItem(new PyObject("util.KeyVal", reward));
            immediateRewards->SetItem(new PyInt(rewardTypeID), rewardList);
        }
    }

    PyDict* result = new PyDict();
    result->SetItemString("immediateRewards", immediateRewards);
    result->SetItemString("delayedRewards", delayedRewards);
    result->SetItemString("rewardID", new PyInt(id));
    return new PyObject("util.KeyVal", result);
}
