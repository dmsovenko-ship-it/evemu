#ifndef __REWARDMGR_SERVICE_H_INCL__
#define __REWARDMGR_SERVICE_H_INCL__

#include "services/Service.h"

class RewardMgrService : public Service<RewardMgrService>
{
public:
    RewardMgrService();
protected:
    PyResult GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyRep* rewardGroupIDs);
    PyResult GetRewardData(PyCallArgs& call, PyInt* rewardID);
    PyResult GetRewardLPLogs(PyCallArgs& call);
};

#endif
