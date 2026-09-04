#include "eve-server.h"
#include "EntityList.h"
#include "npc/BotMgr.h"
#include "apiserver/APIServerManager.h"

#define EVEMU_API_VERSION "2.0.0"

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
        xml += "    <serveronline>1</serveronline>\n";
        xml += "    <serverversion>EVEmu Crucible</serverversion>\n";
        xml += "    <apiversion>" EVEMU_API_VERSION "</apiversion>\n";
        // Chelobots imitate real pilots, so the world population shown on the
        // portal = real clients + chelobots flying in space + chelobots docked.
        uint32 online = sEntityList.GetClientCount() + sBotMgr.CountActiveBots() + sBotMgr.GetDockedBotCount();
        xml += "    <onlineplayers>" + std::to_string(online) + "</onlineplayers>\n";
        xml += "    <onlineplayersreal>" + std::to_string(sEntityList.GetClientCount()) + "</onlineplayersreal>\n";
        xml += "    <accountcount>" + std::to_string(accountCount) + "</accountcount>\n";
        xml += "    <charactercount>" + std::to_string(charCount) + "</charactercount>\n";
        xml += "    <botcount>" + std::to_string(botCount) + "</botcount>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "KillStats.xml.aspx") {
        DBQueryResult res;
        uint32 totalkills = 0;
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM chrKillTable");
        { DBResultRow r; if (res.GetRow(r)) totalkills = r.GetUInt(0); }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <totalkills>" + std::to_string(totalkills) + "</totalkills>\n";

        // top killers
        xml += "    <topkillers>\n";
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
        xml += "    </topkillers>\n";

        // top victims
        xml += "    <topvictims>\n";
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
        xml += "    </topvictims>\n";

        // kills by system
        xml += "    <killsbysystem>\n";
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
        xml += "    </killsbysystem>\n";

        // most destroyed ship types
        xml += "    <topships>\n";
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
        xml += "    </topships>\n";

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

    if (handler == "MarketTops.xml.aspx") {
        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        auto part = [&](const char* tag, const std::string& where) {
            std::string out;
            DBQueryResult q;
            std::string qs = "SELECT t.typeID, MAX(iv.typeName) as typeName, "
                "SUM(t.quantity * t.price) as isk, SUM(t.quantity) as qty "
                "FROM mktTransactions t JOIN invTypes iv ON iv.typeID = t.typeID "
                "WHERE " + where + " GROUP BY t.typeID "
                "ORDER BY isk DESC LIMIT 10";
            if (sDatabase.RunQuery(q, qs.c_str())) {
                DBResultRow r;
                while (q.GetRow(r)) {
                    std::ostringstream isk; isk.precision(0); isk << std::fixed << r.GetDouble(2);
                    out += "      <row typeid=\"" + std::to_string(r.GetUInt(0)) + "\"";
                    out += " name=\"" + xmlEscape(r.GetText(1)) + "\"";
                    out += " isk=\"" + isk.str() + "\"";
                    out += " qty=\"" + std::to_string(r.GetUInt(3)) + "\"/>\n";
                }
            }
            xml += "    <" + std::string(tag) + ">\n" + out + "    </" + std::string(tag) + ">\n";
        };
        auto party = [&](const char* tag, const std::string& where, const char* joinSel, const char* joinOn) {
            std::string out;
            DBQueryResult q;
            std::string qs = std::string("SELECT t.characterID, ") + joinSel + ", "
                "SUM(t.quantity * t.price) as isk, COUNT(*) as trades "
                "FROM mktTransactions t " + joinOn + " "
                "WHERE " + where + " GROUP BY t.characterID, " + joinSel + " "
                "ORDER BY isk DESC LIMIT 10";
            if (sDatabase.RunQuery(q, qs.c_str())) {
                DBResultRow r;
                while (q.GetRow(r)) {
                    std::ostringstream isk; isk.precision(0); isk << std::fixed << r.GetDouble(2);
                    out += "      <row id=\"" + std::to_string(r.GetUInt(0)) + "\"";
                    out += " name=\"" + xmlEscape(r.GetText(1)) + "\"";
                    out += " isk=\"" + isk.str() + "\"";
                    out += " trades=\"" + std::to_string(r.GetUInt(3)) + "\"/>\n";
                }
            }
            xml += "    <" + std::string(tag) + ">\n" + out + "    </" + std::string(tag) + ">\n";
        };

        part("topboughtitems", "t.transactionType = 1");
        part("topsolditems",   "t.transactionType = 0");
        party("topbuyers",     "t.transactionType = 1", "COALESCE(cc.characterName, cr.corporationName, 'Unknown')",
              "LEFT JOIN chrCharacters cc ON cc.characterID = t.characterID "
              "LEFT JOIN crpCorporation cr ON cr.corporationID = t.characterID");
        party("topsellers",    "t.transactionType = 0", "COALESCE(cc.characterName, cr.corporationName, 'Unknown')",
              "LEFT JOIN chrCharacters cc ON cc.characterID = t.characterID "
              "LEFT JOIN crpCorporation cr ON cr.corporationID = t.characterID");

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "CourierContracts.xml.aspx") {
        // Public courier contracts on the open market (the haul-jobs board): both
        // player-created jobs and chelobot courier jobs (some now carrying real
        // ctrItems cargo that physically travels between stations). Portal shows
        // route, reward, volume and whether real cargo is attached.
        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // limit param (default 50). Filter by system via fromsystem/tosystem.
        int limit = 50;
        std::string lim = get("limit");
        if (!lim.empty()) { int v = atoi(lim.c_str()); if (v > 0 && v <= 500) limit = v; }

        std::string where = "c.contractType = 3 AND c.status = 0 AND c.isPrivate = 0";
        std::string fs = get("fromsystem"), ts = get("tosystem");
        if (!fs.empty() && atoi(fs.c_str()) > 0) where += " AND c.startSolarSystemID = " + fs;
        if (!ts.empty() && atoi(ts.c_str()) > 0) where += " AND c.endSolarSystemID = " + ts;

        std::string qs =
            "SELECT c.contractId, "
            "  ss.stationName as fromstation, cs.solarSystemName as fromsystem, "
            "  es.stationName as tostation, cs2.solarSystemName as tosystem, "
            "  c.volume, c.reward, "
            "  COALESCE(cc.characterName, cr.corporationName, c.issuerID) as issuer, "
            "  (SELECT COUNT(*) FROM ctrItems ci WHERE ci.contractId = c.contractId AND ci.itemID != 0) as itemcount, "
            "  (SELECT COALESCE(SUM(e2.quantity),0) FROM ctrItems ci2 "
            "     LEFT JOIN entity e2 ON e2.itemID = ci2.itemID "
            "     WHERE ci2.contractId = c.contractId) as units "
            "FROM ctrContracts c "
            "LEFT JOIN staStations ss  ON ss.stationID = c.startStationID "
            "LEFT JOIN mapSolarSystems cs ON cs.solarSystemID = c.startSolarSystemID "
            "LEFT JOIN staStations es  ON es.stationID = c.endStationID "
            "LEFT JOIN mapSolarSystems cs2 ON cs2.solarSystemID = c.endSolarSystemID "
            "LEFT JOIN chrCharacters cc ON cc.characterID = c.issuerID "
            "LEFT JOIN crpCorporation cr ON cr.corporationID = c.issuerID "
            "WHERE " + where + " ORDER BY c.dateIssued DESC LIMIT " + std::to_string(limit);

        std::string out;
        if (sDatabase.RunQuery(res, qs.c_str())) {
            DBResultRow r;
            while (res.GetRow(r)) {
                std::ostringstream vol, rw;
                vol.precision(0); vol << std::fixed << r.GetDouble(5);
                rw.precision(0);  rw << std::fixed << r.GetDouble(6);
                out += "    <contract contractid=\"" + std::to_string(r.GetUInt(0)) + "\"";
                out += " fromstation=\"" + xmlEscape(r.GetText(1)) + "\"";
                out += " fromsystem=\"" + xmlEscape(r.GetText(2)) + "\"";
                out += " tostation=\"" + xmlEscape(r.GetText(3)) + "\"";
                out += " tosystem=\"" + xmlEscape(r.GetText(4)) + "\"";
                out += " volume=\"" + vol.str() + "\"";
                out += " reward=\"" + rw.str() + "\"";
                out += " issuer=\"" + xmlEscape(r.GetText(7)) + "\"";
                out += " itemcount=\"" + std::to_string(r.GetUInt(8)) + "\"";
                out += " units=\"" + std::to_string(r.GetUInt(9)) + "\"/>\n";
            }
        }
        xml += out + "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "MarketStats.xml.aspx") {
        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // total orders
        uint32 totalorders = 0;
        sDatabase.RunQuery(res, "SELECT COUNT(*) FROM mktOrders");
        { DBResultRow r; if (res.GetRow(r)) totalorders = r.GetUInt(0); }
        xml += "    <totalorders>" + std::to_string(totalorders) + "</totalorders>\n";

        // total isk volume
        xml += "    <toptraded>\n";
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
        xml += "    </toptraded>\n";

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
                "SELECT COUNT(*) FROM chrKillTable WHERE (killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL %u DAY))", days);
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
            "WHERE (k.killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY)) "
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
                xml += " solarsystemname=\"" + xmlEscape(row.GetText(23)) + "\"";
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

    if (handler == "MapData.xml.aspx") {
        auto get2 = [&](const std::string& k) -> std::string {
            auto it = params.find(k);
            return it != params.end() ? it->second : "";
        };
        std::string sid = get2("systemid");
        if (sid.empty()) return BuildErrorXML("105", "Missing systemID.");
        uint32 systemID = std::stoul(sid);

        DBQueryResult res;
        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // current system + its constellation/region
        if (sDatabase.RunQuery(res,
            "SELECT ss.solarSystemID, ss.solarSystemName, ss.security, ss.x, ss.z, "
            "ss.constellationID, ss.regionID, con.constellationName, reg.regionName "
            "FROM mapSolarSystems ss "
            "LEFT JOIN mapConstellations con ON con.constellationID = ss.constellationID "
            "LEFT JOIN mapRegions reg ON reg.regionID = ss.regionID "
            "WHERE ss.solarSystemID = %u", systemID)) {
            DBResultRow row;
            if (res.GetRow(row)) {
                xml += "    <system id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                xml += " security=\"" + std::string(row.GetText(2)) + "\"";
                xml += " x=\"" + std::string(row.GetText(3)) + "\"";
                xml += " z=\"" + std::string(row.GetText(4)) + "\"";
                xml += " constellationid=\"" + std::to_string(row.GetUInt(5)) + "\"";
                xml += " regionid=\"" + std::to_string(row.GetUInt(6)) + "\"";
                xml += " constellationname=\"" + xmlEscape(row.GetText(7)) + "\"";
                xml += " regionname=\"" + xmlEscape(row.GetText(8)) + "\"/>\n";
            }
        }

        // constellations of the region (for the region map) — centroids x/z
        uint32 regionID = 0;
        {
            DBQueryResult cres;
            if (sDatabase.RunQuery(cres,
                "SELECT regionID FROM mapSolarSystems WHERE solarSystemID = %u", systemID)) {
                DBResultRow r0; if (cres.GetRow(r0)) regionID = r0.GetUInt(0);
            }
        }
        if (regionID > 0) {
            xml += "    <constellations>\n";
            if (sDatabase.RunQuery(res,
                "SELECT constellationID, constellationName, x, z FROM mapConstellations WHERE regionID = %u", regionID)) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " x=\"" + std::string(row.GetText(2)) + "\"";
                    xml += " z=\"" + std::string(row.GetText(3)) + "\"/>\n";
                }
            }
            xml += "    </constellations>\n";
        }

        // systems of the same constellation (for the constellation / system maps)
        uint32 constellationID = 0;
        {
            DBQueryResult cres;
            if (sDatabase.RunQuery(cres,
                "SELECT constellationID FROM mapSolarSystems WHERE solarSystemID = %u", systemID)) {
                DBResultRow r0; if (cres.GetRow(r0)) constellationID = r0.GetUInt(0);
            }
        }
        if (constellationID > 0) {
            xml += "    <systems>\n";
            if (sDatabase.RunQuery(res,
                "SELECT solarSystemID, solarSystemName, security, x, z "
                "FROM mapSolarSystems WHERE constellationID = %u", constellationID)) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " security=\"" + std::string(row.GetText(2)) + "\"";
                    xml += " x=\"" + std::string(row.GetText(3)) + "\"";
                    xml += " z=\"" + std::string(row.GetText(4)) + "\"/>\n";
                }
            }
            xml += "    </systems>\n";

            // intra-constellation jump lines
            xml += "    <jumps>\n";
            if (sDatabase.RunQuery(res,
                "SELECT fromSolarSystemID, toSolarSystemID FROM mapSolarSystemJumps "
                "WHERE fromConstellationID = %u AND toConstellationID = %u", constellationID, constellationID)) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row from=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " to=\"" + std::to_string(row.GetUInt(1)) + "\"/>\n";
                }
            }
            xml += "    </jumps>\n";
        }

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "Activity.xml.aspx") {
        auto get2 = [&](const std::string& k) -> std::string {
            auto it = params.find(k);
            return it != params.end() ? it->second : "";
        };
        uint32 days = 7;
        std::string period = get2("period");
        if (period == "24h") days = 1;
        else if (period == "30d") days = 30;
        else if (period == "all") days = 36500;
        std::string where = "(k.killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY))";

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";

        // Current Activity summary
        {
            DBQueryResult res;
            uint32 total = 0, chars = 0, corps = 0, allies = 0, ships = 0, systems = 0, regions = 0;
            std::string q;
            q = "SELECT COUNT(*) FROM chrKillTable k WHERE " + where;
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) total = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT k.finalCharacterID) FROM chrKillTable k WHERE " + where + " AND k.finalCharacterID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) chars = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT k.finalCorporationID) FROM chrKillTable k WHERE " + where + " AND k.finalCorporationID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) corps = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT k.finalAllianceID) FROM chrKillTable k WHERE " + where + " AND k.finalAllianceID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) allies = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT k.victimShipTypeID) FROM chrKillTable k WHERE " + where + " AND k.victimShipTypeID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) ships = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT k.solarSystemID) FROM chrKillTable k WHERE " + where + " AND k.solarSystemID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) systems = r.GetUInt(0); }
            q = "SELECT COUNT(DISTINCT ss.regionID) FROM chrKillTable k "
                "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID WHERE " + where + " AND ss.regionID > 0";
            sDatabase.RunQuery(res, q.c_str());
            { DBResultRow r; if (res.GetRow(r)) regions = r.GetUInt(0); }
            xml += "    <summary total=\"" + std::to_string(total) + "\"";
            xml += " characters=\"" + std::to_string(chars) + "\"";
            xml += " corporations=\"" + std::to_string(corps) + "\"";
            xml += " alliances=\"" + std::to_string(allies) + "\"";
            xml += " ships=\"" + std::to_string(ships) + "\"";
            xml += " systems=\"" + std::to_string(systems) + "\"";
            xml += " regions=\"" + std::to_string(regions) + "\"/>\n";
        }

        auto emitTop = [&](const char* tag, const char* sql) {
            xml += std::string("    <") + tag + ">\n";
            DBQueryResult res;
            if (sDatabase.RunQuery(res, sql)) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
                    xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
                    xml += " count=\"" + std::to_string(row.GetUInt(2)) + "\"/>\n";
                }
            }
            xml += std::string("    </") + tag + ">\n";
        };

        emitTop("characters",
            ("SELECT k.finalCharacterID, c.characterName, COUNT(*) FROM chrKillTable k "
             "LEFT JOIN chrCharacters c ON c.characterID = k.finalCharacterID "
             "WHERE " + where + " AND k.finalCharacterID > 0 "
             "GROUP BY k.finalCharacterID ORDER BY COUNT(*) DESC LIMIT 10").c_str());
        emitTop("corporations",
            ("SELECT k.finalCorporationID, cc.corporationName, COUNT(*) FROM chrKillTable k "
             "LEFT JOIN crpCorporation cc ON cc.corporationID = k.finalCorporationID "
             "WHERE " + where + " AND k.finalCorporationID > 0 "
             "GROUP BY k.finalCorporationID ORDER BY COUNT(*) DESC LIMIT 10").c_str());
        emitTop("alliances",
            ("SELECT k.finalAllianceID, aa.allianceName, COUNT(*) FROM chrKillTable k "
             "LEFT JOIN alnAlliance aa ON aa.allianceID = k.finalAllianceID "
             "WHERE " + where + " AND k.finalAllianceID > 0 "
             "GROUP BY k.finalAllianceID ORDER BY COUNT(*) DESC LIMIT 10").c_str());
        emitTop("ships",
            ("SELECT k.victimShipTypeID, iv.typeName, COUNT(*) FROM chrKillTable k "
             "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
             "WHERE " + where + " AND k.victimShipTypeID > 0 "
             "GROUP BY k.victimShipTypeID ORDER BY COUNT(*) DESC LIMIT 10").c_str());
        emitTop("systems",
            ("SELECT k.solarSystemID, ss.solarSystemName, COUNT(*) FROM chrKillTable k "
             "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
             "WHERE " + where + " AND k.solarSystemID > 0 "
             "GROUP BY k.solarSystemID ORDER BY COUNT(*) DESC LIMIT 10").c_str());

        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "TopValuables.xml.aspx") {        auto get2 = [&](const std::string& k) -> std::string {
            auto it = params.find(k);
            return it != params.end() ? it->second : "";
        };
        uint32 days = 7;
        std::string period = get2("period");
        if (period == "24h") days = 1;
        else if (period == "30d") days = 30;
        else if (period == "all") days = 36500;
        uint32 limit = get2("limit").empty() ? 20 : std::stoul(get2("limit"));
        if (limit < 1) limit = 1; if (limit > 200) limit = 200;

        // Candidate pool (top by raw HP for the window) — capped to bound the
        // per-type price lookups. Real "value" (ship+fit) is then computed below.
        struct RowV { uint32 killID, sysID, victimID, shipTypeID, dmg; int64 killTime;
                      std::string victimName, shipName, sysName, finalName, blob;
                      int32 catID; double value; };
        std::vector<RowV> pool;
        {
            DBQueryResult res;
            std::string poolQ =
                "SELECT k.killID, k.solarSystemID, k.victimCharacterID, k.victimShipTypeID, "
                "k.victimDamageTaken, k.killTime, k.killBlob, k.finalCharacterID, "
                "vc.characterName, fc.characterName, iv.typeName, iv.groupID, "
                "ig.categoryID, ss.solarSystemName "
                "FROM chrKillTable k "
                "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
                "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
                "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
                "LEFT JOIN invGroups ig ON ig.groupID = iv.groupID "
                "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
                "WHERE (k.killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY)) "
                "ORDER BY k.victimDamageTaken DESC LIMIT 400";
            if (sDatabase.RunQuery(res, poolQ.c_str())) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    RowV r;
                    r.killID = row.GetUInt(0); r.sysID = row.GetUInt(1);
                    r.victimID = row.GetUInt(2); r.shipTypeID = row.GetUInt(3);
                    r.dmg = row.GetUInt(4); r.killTime = row.GetInt64(5);
                    r.victimName = xmlEscape(row.GetText(8));
                    r.finalName = xmlEscape(row.GetText(9));
                    r.shipName = xmlEscape(row.GetText(10));
                    r.catID = row.GetInt(12);
                    r.sysName = xmlEscape(row.GetText(13));
                    const char* b = row.GetText(6);
                    r.blob = (b ? b : "");
                    pool.push_back(std::move(r));
                }
            }
        }

        // one price lookup for every involved typeID (hull + blob items)
        std::map<uint32, double> price;
        if (!pool.empty()) {
            std::map<uint32, double> need;
            for (auto& r : pool) need[r.shipTypeID] = 0;
            for (auto& r : pool) {
                std::string& b = r.blob; size_t pos = 0;
                while ((pos = b.find("t=", pos)) != std::string::npos) {
                    size_t e = pos + 2; while (e < b.size() && isdigit((unsigned char)b[e])) ++e;
                    if (e > pos + 2) need[std::stoul(b.substr(pos + 2, e - pos - 2))] = 0;
                    pos = e;
                }
            }
            if (!need.empty()) {
                std::string ids; bool first = true;
                for (auto& kv : need) { if (!first) ids += ","; first = false; ids += std::to_string(kv.first); }
                DBQueryResult res;
                if (sDatabase.RunQuery(res,
                    "SELECT typeID, AVG(price) FROM mktOrders WHERE typeID IN (%s) GROUP BY typeID", ids.c_str())) {
                    DBResultRow row;
                    while (res.GetRow(row)) price[row.GetUInt(0)] = row.GetDouble(1);
                }
            }
        }
        auto typePrice = [&](uint32 t) -> double { auto it = price.find(t); return it != price.end() ? it->second : 0.0; };

        for (auto& r : pool) {
            double v = typePrice(r.shipTypeID);   // hull
            std::string& b = r.blob; size_t pos = 0;
            while ((pos = b.find("<i ", pos)) != std::string::npos) {
                size_t te = b.find("t=", pos);
                size_t qe = b.find("q=", pos);
                size_t xe = b.find("/>", pos);
                if (te == std::string::npos || xe == std::string::npos) break;
                size_t tEnd = te + 2; while (tEnd < xe && isdigit((unsigned char)b[tEnd])) ++tEnd;
                uint32 tid = std::stoul(b.substr(te + 2, tEnd - te - 2));
                uint32 qty = 1;
                if (qe != std::string::npos && qe < xe) {
                    size_t qEnd = qe + 2; while (qEnd < xe && isdigit((unsigned char)b[qEnd])) ++qEnd;
                    if (qEnd > qe + 2) qty = std::max<uint32>(1, std::stoul(b.substr(qe + 2, qEnd - qe - 2)));
                }
                if (tid != r.shipTypeID)
                    v += typePrice(tid) * qty;
                pos = xe + 2;
            }
            r.value = v;
        }

        std::sort(pool.begin(), pool.end(),
                  [](const RowV& a, const RowV& b) { return a.value > b.value; });
        if (pool.size() > limit) pool.resize(limit);

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <kills>\n";
        for (auto& r : pool) {
            std::ostringstream val; val.precision(2); val << std::fixed << r.value;
            xml += "      <row killid=\"" + std::to_string(r.killID) + "\"";
            xml += " systemid=\"" + std::to_string(r.sysID) + "\"";
            xml += " solarsystemname=\"" + r.sysName + "\"";
            xml += " victimid=\"" + std::to_string(r.victimID) + "\"";
            xml += " victimname=\"" + r.victimName + "\"";
            xml += " victimshiptypeid=\"" + std::to_string(r.shipTypeID) + "\"";
            xml += " victimshipname=\"" + r.shipName + "\"";
            xml += " categoryid=\"" + std::to_string(r.catID) + "\"";
            xml += " damage=\"" + std::to_string(r.dmg) + "\"";
            xml += " value=\"" + val.str() + "\"";
            xml += " killtime=\"" + std::to_string(r.killTime) + "\"";
            xml += " finalname=\"" + r.finalName + "\"/>\n";
        }
        xml += "    </kills>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
