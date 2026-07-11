
#ifndef __MANUFACTURING_SERVICE_H__
#define __MANUFACTURING_SERVICE_H__

#include "services/Service.h"

class ManufacturingService : public Service<ManufacturingService>
{
public:
    ManufacturingService();

protected:
    PyResult GetPathToItem(PyCallArgs& call, PyInt* itemID);
};

#endif
