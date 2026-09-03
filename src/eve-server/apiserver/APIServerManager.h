#ifndef __APISERVERMANAGER__H__INCL__
#define __APISERVERMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APIServerManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;
};

#endif // __APISERVERMANAGER__H__INCL__
