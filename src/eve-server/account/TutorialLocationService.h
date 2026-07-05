#ifndef __TUTORIALLOCATIONSVC_SERVICE_H_INCL__
#define __TUTORIALLOCATIONSVC_SERVICE_H_INCL__

#include "services/Service.h"

class TutorialLocationService : public Service<TutorialLocationService>
{
public:
    TutorialLocationService();

protected:
    PyResult GiveTutorialGoodies(PyCallArgs& call, PyInt* tutorialID, PyInt* pageID, PyInt* pageNo);
};

#endif
