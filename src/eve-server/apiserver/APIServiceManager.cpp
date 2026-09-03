#include "apiserver/APIServiceManager.h"

std::string APIServiceManager::BuildErrorXML(const std::string& code, const std::string& msg)
{
    return "<?xml version='1.0' encoding='UTF-8'?>\n"
           "<eveapi version=\"2\">\n"
           "  <error code=\"" + code + "\">" + msg + "</error>\n"
           "</eveapi>\n";
}
