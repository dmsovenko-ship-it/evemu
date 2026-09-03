#ifndef __APICHARACTERMANAGER__H__INCL__
#define __APICHARACTERMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APICharacterManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;
};

#endif // __APICHARACTERMANAGER__H__INCL__
