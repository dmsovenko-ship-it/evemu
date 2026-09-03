#ifndef __APISERVICEMANAGER__H__INCL__
#define __APISERVICEMANAGER__H__INCL__

#include <string>
#include <map>
#include <memory>

class APIServiceManager
{
public:
    virtual ~APIServiceManager() = default;
    virtual std::string ProcessCall(const std::string& handler,
                                     const std::map<std::string, std::string>& params) = 0;

    static std::string BuildErrorXML(const std::string& code, const std::string& msg);
};

#endif // __APISERVICEMANAGER__H__INCL__
