#ifndef __BILLBOARD_SERVICE_H_INCL__
#define __BILLBOARD_SERVICE_H_INCL__

#include "Service.h"

class BillboardService : public Service {
public:
    BillboardService(EVEServiceManager& mgr);
    PyResult GetServerMessages(PyCallArgs& call);
    PyResult GetBillboardData(PyCallArgs& call, PyInt* locationID);
};

#endif
