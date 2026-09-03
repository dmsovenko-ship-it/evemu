#ifndef __APIAUTHMANAGER__H__INCL__
#define __APIAUTHMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APIAuthManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;
};

#endif // __APIAUTHMANAGER__H__INCL__
