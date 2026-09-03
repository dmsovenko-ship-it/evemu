#include "eve-server.h"
#include "apiserver/APIAccountManager.h"

std::string APIAccountManager::ProcessCall(const std::string& handler,
                                           const std::map<std::string, std::string>& params)
{
    if (handler == "APIKeyInfo.xml.aspx") {
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <key accessMask=\"268435455\" type=\"Account\" expires=\"\" />\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "Characters.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT characterID, characterName, corporationID, corporationName FROM chrCharacters "
            "WHERE accountID = 1"))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <characters>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row characterID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " characterName=\"" + std::string(row.GetText(1)) + "\"";
            xml += " corporationID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " corporationName=\"" + std::string(row.GetText(3)) + "\"/>\n";
        }
        xml += "    </characters>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
