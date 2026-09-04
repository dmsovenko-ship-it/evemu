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
            "COUNT(DISTINCT c.characterID) as playerCount, "
            "COUNT(DISTINCT e.itemID) as shipCount "
            "FROM mapSolarSystems ss "
            "LEFT JOIN chrCharacters c ON c.solarSystemID = ss.solarSystemID "
            "LEFT JOIN entity e ON e.locationID = ss.solarSystemID AND e.flag = 0 "
            "GROUP BY ss.solarSystemID, ss.solarSystemName, ss.security "
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

    if (handler == "TopKills.xml.aspx") {
        auto get2 = [&](const std::string& k) -> std::string {
            auto it = params.find(k);
            return it != params.end() ? it->second : "";
        };
        std::string period = get2("period");
        uint32 days = 7;
        if (period == "24h") days = 1;
        else if (period == "30d") days = 30;
        else if (period == "all") days = 36500;

        uint32 page = get2("page").empty() ? 1 : std::stoul(get2("page"));
        uint32 perPage = 50;
        uint32 offset = (page - 1) * perPage;

        DBQueryResult res;

        // total count
        uint32 total = 0;
        {
            DBQueryResult cres;
            sDatabase.RunQuery(cres,
                "SELECT COUNT(*) FROM chrKillTable WHERE killTime > DATE_SUB(NOW(), INTERVAL %u DAY)", days);
            DBResultRow r; if (cres.GetRow(r)) total = r.GetUInt(0);
        }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <total>" + std::to_string(total) + "</total>\n";
        xml += "    <page>" + std::to_string(page) + "</page>\n";
        xml += "    <kills>\n";

        std::string q = "SELECT k.killID, k.solarSystemID, k.victimCharacterID, k.victimCorporationID, "
            "k.victimAllianceID, k.victimFactionID, k.victimShipTypeID, k.victimDamageTaken, "
            "k.finalCharacterID, k.finalCorporationID, k.finalAllianceID, k.finalFactionID, "
            "k.finalShipTypeID, k.finalWeaponTypeID, k.finalSecurityStatus, k.finalDamageDone, "
            "k.killTime, k.moonID, "
            "vc.characterName, fc.characterName, "
            "iv.typeName, if_.typeName, iw.typeName, "
            "ss.solarSystemName "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN invTypes iw ON iw.typeID = k.finalWeaponTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "WHERE k.killTime > DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY) "
            "ORDER BY k.victimDamageTaken DESC "
            "LIMIT " + std::to_string(perPage) + " OFFSET " + std::to_string(offset);

        if (sDatabase.RunQuery(res, q.c_str())) {
            DBResultRow row;
            while (res.GetRow(row)) {
                xml += "      <row killid=\"" + std::to_string(row.GetUInt(0)) + "\"";
                xml += " systemid=\"" + std::to_string(row.GetUInt(1)) + "\"";
                xml += " victimid=\"" + std::to_string(row.GetUInt(2)) + "\"";
                xml += " victimcorpid=\"" + std::to_string(row.GetUInt(3)) + "\"";
                xml += " victimallianceid=\"" + std::to_string(row.GetInt(4)) + "\"";
                xml += " victimfactionid=\"" + std::to_string(row.GetUInt(5)) + "\"";
                xml += " victimshiptypeid=\"" + std::to_string(row.GetUInt(6)) + "\"";
                xml += " victimdamagetaken=\"" + std::to_string(row.GetUInt(7)) + "\"";
                xml += " finalid=\"" + std::to_string(row.GetUInt(8)) + "\"";
                xml += " finalcorpid=\"" + std::to_string(row.GetUInt(9)) + "\"";
                xml += " finalallianceid=\"" + std::to_string(row.GetInt(10)) + "\"";
                xml += " finalfactionid=\"" + std::to_string(row.GetUInt(11)) + "\"";
                xml += " finalshiptypeid=\"" + std::to_string(row.GetUInt(12)) + "\"";
                xml += " finalweapontypeid=\"" + std::to_string(row.GetUInt(13)) + "\"";
                xml += " finalsecstatus=\"" + std::string(row.GetText(14)) + "\"";
                xml += " finaldamagedone=\"" + std::to_string(row.GetUInt(15)) + "\"";
                xml += " killtime=\"" + std::string(row.GetText(16)) + "\"";
                xml += " moonid=\"" + std::to_string(row.GetUInt(17)) + "\"";
                xml += " victimname=\"" + xmlEscape(row.GetText(18)) + "\"";
                xml += " finalname=\"" + xmlEscape(row.GetText(19)) + "\"";
                xml += " victimshipname=\"" + xmlEscape(row.GetText(20)) + "\"";
                xml += " finalshipname=\"" + xmlEscape(row.GetText(21)) + "\"";
                xml += " finalweaponname=\"" + xmlEscape(row.GetText(22)) + "\"";
                xml += " systemname=\"" + xmlEscape(row.GetText(23)) + "\"";
                xml += "/>\n";
            }
        }

        xml += "    </kills>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "Search.xml.aspx") {
        auto get2 = [&](const std::string& k) -> std::string {
            auto it = params.find(k);
            return it != params.end() ? it->second : "";
        };
        std::string q = get2("q");
        if (q.empty()) return BuildErrorXML("105", "Missing search query.");

        std::string like = "%" + q + "%";
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // characters
        xml += "    <characters>\n";
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT characterID, characterName, corporationID, skillPoints, securityRating "
                "FROM chrCharacters WHERE characterName LIKE '%s' ORDER BY skillPoints DESC LIMIT 20",
                like.c_str())) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " corpid=\"" + std::to_string(row.GetUInt(2)) + "\"";
                    xml += " sp=\"" + std::to_string(row.GetInt64(3)) + "\"";
                    xml += " sec=\"" + std::string(row.GetText(4)) + "\"/>\n";
                }
            }
        }
        xml += "    </characters>\n";

        // corporations
        xml += "    <corporations>\n";
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT corporationID, corporationName, tickerName, memberCount, allianceID "
                "FROM crpCorporation WHERE corporationName LIKE '%s' OR tickerName LIKE '%s' "
                "ORDER BY memberCount DESC LIMIT 20",
                like.c_str(), like.c_str())) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " ticker=\"" + xmlEscape(row.GetText(2)) + "\"";
                    xml += " members=\"" + std::to_string(row.GetUInt(3)) + "\"";
                    xml += " allianceid=\"" + std::to_string(row.GetInt(4)) + "\"/>\n";
                }
            }
        }
        xml += "    </corporations>\n";

        // systems
        xml += "    <systems>\n";
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT solarSystemID, solarSystemName, security "
                "FROM mapSolarSystems WHERE solarSystemName LIKE '%s' LIMIT 10",
                like.c_str())) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " sec=\"" + std::string(row.GetText(2)) + "\"/>\n";
                }
            }
        }
        xml += "    </systems>\n";

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
