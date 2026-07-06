#include "eve-server.h"

#include "incursion/IncursionService.h"
#include "EVEDBUtils.h"

IncursionService::IncursionService() :
    Service("incursion")
{
    this->Add("GetDelayedRewardsByGroupIDs", &IncursionService::GetDelayedRewardsByGroupIDs);
}

PyResult IncursionService::GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyList* rewardGroupIDs)
{
    PyDict* result = new PyDict();
    for (auto itr = rewardGroupIDs->begin(); itr != rewardGroupIDs->end(); ++itr) {
        if (!(*itr)->IsInt())
            continue;

        PyList* rewardList = new PyList();
        result->SetItem(*itr, rewardList);
    }
    return result;
}
