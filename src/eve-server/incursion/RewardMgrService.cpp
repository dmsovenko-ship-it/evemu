#include "eve-server.h"

#include "incursion/RewardMgrService.h"

RewardMgrService::RewardMgrService() :
    Service("rewardMgr")
{
    this->Add("GetDelayedRewardsByGroupIDs", &RewardMgrService::GetDelayedRewardsByGroupIDs);
}

PyResult RewardMgrService::GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyList* rewardGroupIDs)
{
    // Returns incursion rewards grouped by rewardGroupID
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
