#ifndef __APIACCOUNTMANAGER__H__INCL__
#define __APIACCOUNTMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APIAccountManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;
};

#endif // __APIACCOUNTMANAGER__H__INCL__
