#ifndef __INCURSION_SERVICE_H_INCL__
#define __INCURSION_SERVICE_H_INCL__

#include "services/Service.h"

class IncursionService : public Service<IncursionService>
{
public:
    IncursionService();
protected:
    PyResult GetDelayedRewardsByGroupIDs(PyCallArgs& call, PyList* rewardGroupIDs);
    PyResult GetIncursions(PyCallArgs& call);
};

#endif
