#ifndef __CONFIG_SERVICE_H_INCL__
#define __CONFIG_SERVICE_H_INCL__

#include "services/Service.h"

class ConfigService : public Service<ConfigService>
{
public:
    ConfigService();
protected:
    PyResult GetMapConnections(PyCallArgs& call);
};

#endif
