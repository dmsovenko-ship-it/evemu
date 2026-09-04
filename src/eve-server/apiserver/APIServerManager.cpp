#include "eve-server.h"
#include "EntityList.h"
#include "apiserver/APIServerManager.h"

static std::string xmlEscape(const char* s) {
    if (!s) return "";
    std::string out;
    out.reserve(strlen(s) + 16);
    for (const char* p = s; *p; ++p) {
        switch (*p) {
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '&':  out += "&amp;";  break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += *p;       break;
        }
    }
    return out;
}

std::string APIServerManager::ProcessCall(const std::string& handler,
                                          const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "ServerStatus.xml.aspx") {
        DBQueryResult res;
        uint32 accountCount = 0, charCount = 0, botCount = 0;
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM account");
        { DBResultRow r; if (res.GetRow(r)) accountCount = r.GetUInt(0); }
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM chrCharacters");
        { DBResultRow r; if (res.GetRow(r)) charCount = r.GetUInt(0); }
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM chrCharacters WHERE characterID >= 90000000 AND characterID < 98000000");
        { DBResultRow r; if (res.GetRow(r)) botCount = r.GetUInt(0); }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <serverOnline>1</serverOnline>\n";
        xml += "    <serverVersion>EVEmu Crucible</serverVersion>\n";
        xml += "    <onlinePlayers>" + std::to_string(sEntityList.GetClientCount()) + "</onlinePlayers>\n";
        xml += "    <accountCount>" + std::to_string(accountCount) + "</accountCount>\n";
        xml += "    <characterCount>" + std::to_string(charCount) + "</characterCount>\n";
        xml += "    <botCount>" + std::to_string(botCount) + "</botCount>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "KillStats.xml.aspx") {
        DBQueryResult res;
        uint32 totalKills = 0;
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM chrKillTable");
        { DBResultRow r; if (res.GetRow(r)) totalKills = r.GetUInt(0); }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <totalKills>" + std::to_string(totalKills) + "</totalKills>\n";

        // top killers
        xml += "    <topKillers>\n";
        if (sDatabase.RunQuery(res,
            "SELECT finalCharacterID, COUNT(*) as cnt FROM chrKillTable "
            "WHERE finalCharacterID > 0 GROUP BY finalCharacterID ORDER BY cnt DESC LIMIT 10")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                uint32 charID = row.GetUInt(0);
                uint32 cnt = row.GetUInt(1);
                std::string name;
                DBQueryResult nres;
                if (sDatabase.RunQuery(nres, "SELECT characterName FROM chrCharacters WHERE characterID = %u", charID)) {
                    DBResultRow nrow;
                    if (nres.GetRow(nrow)) name = nrow.GetText(0);
                }
                xml += "      <row characterid=\"" + std::to_string(charID) + "\"";
                xml += " name=\"" + xmlEscape(name.c_str()) + "\"";
                xml += " kills=\"" + std::to_string(cnt) + "\"/>\n";
            }
        }
        xml += "    </topKillers>\n";

        // top victims
        xml += "    <topVictims>\n";
        if (sDatabase.RunQuery(res,
            "SELECT victimCharacterID, COUNT(*) as cnt FROM chrKillTable "
            "WHERE victimCharacterID > 0 GROUP BY victimCharacterID ORDER BY cnt DESC LIMIT 10")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                uint32 charID = row.GetUInt(0);
                uint32 cnt = row.GetUInt(1);
                std::string name;
                DBQueryResult nres;
                if (sDatabase.RunQuery(nres, "SELECT characterName FROM chrCharacters WHERE characterID = %u", charID)) {
                    DBResultRow nrow;
                    if (nres.GetRow(nrow)) name = nrow.GetText(0);
                }
                xml += "      <row characterid=\"" + std::to_string(charID) + "\"";
                xml += " name=\"" + xmlEscape(name.c_str()) + "\"";
                xml += " deaths=\"" + std::to_string(cnt) + "\"/>\n";
            }
        }
        xml += "    </topVictims>\n";

        // kills by system
        xml += "    <killsBySystem>\n";
        if (sDatabase.RunQuery(res,
            "SELECT k.solarSystemID, ss.solarSystemName, COUNT(*) as cnt "
            "FROM chrKillTable k LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "GROUP BY k.solarSystemID ORDER BY cnt DESC LIMIT 10")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                const char* sysName = row.GetText(1);
                xml += "      <row systemid=\"" + std::to_string(row.GetUInt(0)) + "\"";
                xml += " name=\"" + xmlEscape(sysName) + "\"";
                xml += " kills=\"" + std::to_string(row.GetUInt(2)) + "\"/>\n";
            }
        }
        xml += "    </killsBySystem>\n";

        // most destroyed ship types
        xml += "    <topShips>\n";
        if (sDatabase.RunQuery(res,
            "SELECT k.victimShipTypeID, t.typeName, COUNT(*) as cnt "
            "FROM chrKillTable k LEFT JOIN invTypes t ON t.typeID = k.victimShipTypeID "
            "WHERE k.victimShipTypeID > 0 "
            "GROUP BY k.victimShipTypeID ORDER BY cnt DESC LIMIT 10")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                const char* shipName = row.GetText(1);
                xml += "      <row typeid=\"" + std::to_string(row.GetUInt(0)) + "\"";
                xml += " name=\"" + xmlEscape(shipName) + "\"";
                xml += " destroyed=\"" + std::to_string(row.GetUInt(2)) + "\"/>\n";
            }
        }
        xml += "    </topShips>\n";

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "ActiveSystems.xml.aspx") {
        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <systems>\n";

        if (sDatabase.RunQuery(res,
            "SELECT ss.solarSystemID, ss.solarSystemName, ss.security, "
            "(SELECT COUNT(*) FROM chrCharacters c WHERE c.solarSystemID = ss.solarSystemID) as playerCount, "
            "(SELECT COUNT(*) FROM entity e WHERE e.locationID = ss.solarSystemID AND e.flag = 0) as shipCount "
            "FROM mapSolarSystems ss "
            "HAVING playerCount > 0 OR shipCount > 0 "
            "ORDER BY playerCount DESC, shipCount DESC LIMIT 50")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                const char* sysName = row.GetText(1);
                xml += "      <row systemid=\"" + std::to_string(row.GetUInt(0)) + "\"";
                xml += " name=\"" + xmlEscape(sysName) + "\"";
                xml += " security=\"" + std::string(row.GetText(2)) + "\"";
                xml += " players=\"" + std::to_string(row.GetInt(3)) + "\"";
                xml += " ships=\"" + std::to_string(row.GetInt(4)) + "\"/>\n";
            }
        }

        xml += "    </systems>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "MarketStats.xml.aspx") {
        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // total orders
        uint32 totalOrders = 0;
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM mktOrders");
        { DBResultRow r; if (res.GetRow(r)) totalOrders = r.GetUInt(0); }
        xml += "    <totalOrders>" + std::to_string(totalOrders) + "</totalOrders>\n";

        // total isk volume
        xml += "    <topTraded>\n";
        if (sDatabase.RunQuery(res,
            "SELECT typeID, SUM(volEntered) as totalVol, AVG(price) as avgPrice "
            "FROM mktOrders GROUP BY typeID ORDER BY totalVol DESC LIMIT 10")) {
            DBResultRow row;
            while (res.GetRow(row)) {
                uint32 typeID = row.GetUInt(0);
                std::string typeName;
                DBQueryResult nres;
                if (sDatabase.RunQuery(nres, "SELECT typeName FROM invTypes WHERE typeID = %u", typeID)) {
                    DBResultRow nrow;
                    if (nres.GetRow(nrow)) typeName = nrow.GetText(0);
                }
                xml += "      <row typeid=\"" + std::to_string(typeID) + "\"";
                xml += " name=\"" + xmlEscape(typeName.c_str()) + "\"";
                xml += " volume=\"" + std::to_string(row.GetUInt(1)) + "\"";
                xml += " avgprice=\"" + std::string(row.GetText(2)) + "\"/>\n";
            }
        }
        xml += "    </topTraded>\n";

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
