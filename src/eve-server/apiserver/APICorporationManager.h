#ifndef __APICORPORATIONMANAGER__H__INCL__
#define __APICORPORATIONMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APICorporationManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;
};

#endif // __APICORPORATIONMANAGER__H__INCL__
