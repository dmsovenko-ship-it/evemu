#include "eve-server.h"
#include "apiserver/APIServerManager.h"

std::string APIServerManager::ProcessCall(const std::string& handler,
                                          const std::map<std::string, std::string>& params)
{
    if (handler == "ServerStatus.xml.aspx") {
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <serverOnline>1</serverOnline>\n";
        xml += "    <serverVersion>EVEmu</serverVersion>\n";
        xml += "    <onlinePlayers>" + std::to_string(0) + "</onlinePlayers>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
