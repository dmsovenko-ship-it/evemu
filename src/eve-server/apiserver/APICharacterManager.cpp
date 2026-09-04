#include "eve-server.h"
#include "apiserver/APICharacterManager.h"

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

std::string APICharacterManager::ProcessCall(const std::string& handler,
                                             const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "KillMails.xml.aspx") {
        std::string cid = get("characterid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid characterID.");
        uint32 characterID = std::stoul(cid);
        std::string beforeID = get("beforekillid");
        uint32 beforeKillID = beforeID.empty() ? 0 : std::stoul(beforeID);

        DBQueryResult res;
        std::string q = "SELECT k.killID, k.solarSystemID, k.victimCharacterID, k.victimCorporationID, "
            "k.victimAllianceID, k.victimFactionID, k.victimShipTypeID, k.victimDamageTaken, "
            "k.finalCharacterID, k.finalCorporationID, k.finalAllianceID, k.finalFactionID, "
            "k.finalShipTypeID, k.finalWeaponTypeID, k.finalSecurityStatus, k.finalDamageDone, "
            "k.killTime, k.moonID, "
            "vc.characterName, fc.characterName, "
            "iv.typeName, if_.typeName, iw.typeName, "
            "ss.solarSystemName, k.killBlob "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN invTypes iw ON iw.typeID = k.finalWeaponTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "WHERE (k.victimCharacterID = " + cid + " OR k.finalCharacterID = " + cid + ")";
        if (beforeKillID > 0)
            q += " AND k.killID < " + std::to_string(beforeKillID);
        q += " ORDER BY k.killID DESC LIMIT 2500";

        if (!sDatabase.RunQuery(res, q.c_str()))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <kills>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row killid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " solarsystemid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " victimcharacterid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " victimcorporationid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " victimallianceid=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " victimfactionid=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " victimshiptypeid=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " victimdamagetaken=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " finalcharacterid=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " finalcorporationid=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " finalallianceid=\"" + std::to_string(row.GetInt(10)) + "\"";
            xml += " finalfactionid=\"" + std::to_string(row.GetUInt(11)) + "\"";
            xml += " finalshiptypeid=\"" + std::to_string(row.GetUInt(12)) + "\"";
            xml += " finalweapontypeid=\"" + std::to_string(row.GetUInt(13)) + "\"";
            xml += " finalsecuritystatus=\"" + std::string(row.GetText(14)) + "\"";
            xml += " finaldamagedone=\"" + std::to_string(row.GetUInt(15)) + "\"";
            xml += " killtime=\"" + std::string(row.GetText(16)) + "\"";
            xml += " moonid=\"" + std::to_string(row.GetUInt(17)) + "\"";
            // resolved names
            const char* vName = row.GetText(18);
            const char* fName = row.GetText(19);
            const char* vShip = row.GetText(20);
            const char* fShip = row.GetText(21);
            const char* wName = row.GetText(22);
            const char* sName = row.GetText(23);
            xml += " victimname=\"" + xmlEscape(vName) + "\"";
            xml += " finalname=\"" + xmlEscape(fName) + "\"";
            xml += " victimshipname=\"" + xmlEscape(vShip) + "\"";
            xml += " finalshipname=\"" + xmlEscape(fShip) + "\"";
            xml += " finalweaponname=\"" + xmlEscape(wName) + "\"";
            xml += " solarsystemname=\"" + xmlEscape(sName) + "\"";
            // killBlob for item drops
            const char* blob = row.GetText(24);
            xml += " killblob=\"" + xmlEscape(blob) + "\"";
            xml += "/>\n";
        }
        xml += "    </kills>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "MarketOrders.xml.aspx") {
        std::string cid = get("characterid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid characterID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT orderID, typeID, stationID, regionID, orderRange, "
            "accountKey, duration, price, volEntered, volRemaining, minVolume, bid, issued "
            "FROM mktOrders WHERE ownerID = %u ORDER BY orderID", std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <orders>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row orderid=\"" + std::to_string(row.GetInt64(0)) + "\"";
            xml += " typeid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " stationid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " regionid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " range=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " accountkey=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " duration=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " price=\"" + std::string(row.GetText(7)) + "\"";
            xml += " volentered=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " volremaining=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " minvolume=\"" + std::to_string(row.GetUInt(10)) + "\"";
            xml += " bid=\"" + std::string(row.GetBool(11) ? "True" : "False") + "\"";
            xml += " issued=\"" + Win32TimeToString(row.GetInt64(12)) + "\"/>\n";
        }
        xml += "    </orders>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "Standings.xml.aspx") {
        std::string cid = get("characterid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid characterID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT fromID, standing FROM repStandings WHERE toID = %u ORDER BY fromID", std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <standings>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row fromid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " toid=\"" + cid + "\"";
            xml += " standing=\"" + xmlEscape(row.GetText(1)) + "\"/>\n";
        }
        xml += "    </standings>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "WalletJournal.xml.aspx") {
        std::string cid = get("characterid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid characterID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT transactionID, transactionDate, referenceID, entryTypeID, "
            "ownerID1, ownerID2, accountKey, amount, balance, description "
            "FROM acrWalletJournal WHERE ownerID = %u ORDER BY transactionID DESC LIMIT 1000",
            std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <transactions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row transactionid=\"" + std::to_string(row.GetInt64(0)) + "\"";
            xml += " transactiondatetime=\"" + Win32TimeToString(row.GetInt64(1)) + "\"";
            xml += " referenceid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " entrytypeid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " ownerid1=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " ownerid2=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " accountkey=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " amount=\"" + std::string(row.GetText(7)) + "\"";
            xml += " balance=\"" + std::string(row.GetText(8)) + "\"";
            xml += " description=\"" + xmlEscape(row.GetText(9)) + "\"/>\n";
        }
        xml += "    </transactions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "AllKills.xml.aspx") {
        std::string beforeID = get("beforekillid");
        uint32 beforeKillID = beforeID.empty() ? 0 : std::stoul(beforeID);

        DBQueryResult res;
        std::string q = "SELECT k.killID, k.solarSystemID, k.victimCharacterID, k.victimCorporationID, "
            "k.victimAllianceID, k.victimFactionID, k.victimShipTypeID, k.victimDamageTaken, "
            "k.finalCharacterID, k.finalCorporationID, k.finalAllianceID, k.finalFactionID, "
            "k.finalShipTypeID, k.finalWeaponTypeID, k.finalSecurityStatus, k.finalDamageDone, "
            "k.killTime, k.moonID, "
            "vc.characterName, fc.characterName, "
            "iv.typeName, if_.typeName, iw.typeName, "
            "ss.solarSystemName, k.killBlob "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN invTypes iw ON iw.typeID = k.finalWeaponTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID ";
        if (beforeKillID > 0)
            q += "WHERE k.killID < " + std::to_string(beforeKillID) + " ";
        q += "ORDER BY k.killID DESC LIMIT 2500";

        if (!sDatabase.RunQuery(res, q.c_str()))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <kills>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row killid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " solarsystemid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " victimcharacterid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " victimcorporationid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " victimallianceid=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " victimfactionid=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " victimshiptypeid=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " victimdamagetaken=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " finalcharacterid=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " finalcorporationid=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " finalallianceid=\"" + std::to_string(row.GetInt(10)) + "\"";
            xml += " finalfactionid=\"" + std::to_string(row.GetUInt(11)) + "\"";
            xml += " finalshiptypeid=\"" + std::to_string(row.GetUInt(12)) + "\"";
            xml += " finalweapontypeid=\"" + std::to_string(row.GetUInt(13)) + "\"";
            xml += " finalsecuritystatus=\"" + std::string(row.GetText(14)) + "\"";
            xml += " finaldamagedone=\"" + std::to_string(row.GetUInt(15)) + "\"";
            xml += " killtime=\"" + std::string(row.GetText(16)) + "\"";
            xml += " moonid=\"" + std::to_string(row.GetUInt(17)) + "\"";
            const char* vName = row.GetText(18);
            const char* fName = row.GetText(19);
            const char* vShip = row.GetText(20);
            const char* fShip = row.GetText(21);
            const char* wName = row.GetText(22);
            const char* sName = row.GetText(23);
            xml += " victimname=\"" + xmlEscape(vName) + "\"";
            xml += " finalname=\"" + xmlEscape(fName) + "\"";
            xml += " victimshipname=\"" + xmlEscape(vShip) + "\"";
            xml += " finalshipname=\"" + xmlEscape(fShip) + "\"";
            xml += " finalweaponname=\"" + xmlEscape(wName) + "\"";
            xml += " solarsystemname=\"" + xmlEscape(sName) + "\"";
            const char* blob = row.GetText(24);
            xml += " killblob=\"" + xmlEscape(blob) + "\"";
            xml += "/>\n";
        }
        xml += "    </kills>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "Resolve.xml.aspx") {
        std::string ids = get("ids");
        if (ids.empty()) return BuildErrorXML("105", "No IDs provided.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <names>\n";

        // character names
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT characterID, characterName FROM chrCharacters WHERE characterID IN (%s)", ids.c_str())) {
                DBResultRow row;
                while (res.GetRow(row))
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\" name=\"" + std::string(row.GetText(1)) + "\" type=\"character\"/>\n";
            }
        }
        // ship type names (+ median market price for ISK valuation on the portal).
        // MEDIAN (PERCENTILE_CONT 0.5) is robust to outlier orders; fall back to
        // basePrice when the type has no market orders at all.
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT t.typeID, t.typeName, "
                "IFNULL((SELECT DISTINCT PERCENTILE_CONT(0.5) WITHIN GROUP (ORDER BY price) OVER (PARTITION BY typeID) "
                "        FROM mktOrders WHERE typeID = t.typeID), t.basePrice) "
                "FROM invTypes t WHERE t.typeID IN (%s)", ids.c_str())) {
                DBResultRow row;
                while (res.GetRow(row)) {
                    std::ostringstream prc; prc.precision(2); prc << std::fixed << row.GetDouble(2);
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\" name=\"" + std::string(row.GetText(1))
                        + "\" type=\"ship\" price=\"" + prc.str() + "\"/>\n";
                }
            }
        }
        // system names
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT solarSystemID, solarSystemName FROM mapSolarSystems WHERE solarSystemID IN (%s)", ids.c_str())) {
                DBResultRow row;
                while (res.GetRow(row))
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\" name=\"" + std::string(row.GetText(1)) + "\" type=\"system\"/>\n";
            }
        }
        // corporation names
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT corporationID, corporationName FROM crpCorporation WHERE corporationID IN (%s)", ids.c_str())) {
                DBResultRow row;
                while (res.GetRow(row))
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\" name=\"" + std::string(row.GetText(1)) + "\" type=\"corporation\"/>\n";
            }
        }

        xml += "    </names>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "CharacterList.xml.aspx") {
        std::string aid = get("accountid");
        bool allChars = aid.empty();
        uint32 page = get("page").empty() ? 1 : std::stoul(get("page"));
        uint32 perPage = 50;
        uint32 offset = (page - 1) * perPage;

        DBQueryResult res;
        std::string q = "SELECT c.characterID, c.characterName, c.skillPoints, c.corporationID, "
            "c.securityRating, c.raceID, c.gender, c.solarSystemID, c.stationID, "
            "cr.corporationName, cr.tickerName, "
            "ss.solarSystemName, "
            "e.typeID as shipTypeID, t.typeName as shipName "
            "FROM chrCharacters c "
            "LEFT JOIN crpCorporation cr ON cr.corporationID = c.corporationID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = c.solarSystemID "
            "LEFT JOIN entity e ON e.itemID = c.shipID "
            "LEFT JOIN invTypes t ON t.typeID = e.typeID ";
        if (!allChars)
            q += "WHERE c.accountID = " + aid + " ";
        q += "ORDER BY c.skillPoints DESC LIMIT " + std::to_string(perPage) + " OFFSET " + std::to_string(offset);

        if (!sDatabase.RunQuery(res, q.c_str()))
            return BuildErrorXML("999", "Query failed.");

        // total count
        uint32 total = 0;
        {
            DBQueryResult cres;
            std::string cq = "SELECT COUNT(*) FROM chrCharacters";
            if (!allChars) cq += " WHERE accountID = " + aid;
            if (sDatabase.RunQuery(cres, cq.c_str())) {
                DBResultRow crow;
                if (cres.GetRow(crow)) total = crow.GetUInt(0);
            }
        }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <total>" + std::to_string(total) + "</total>\n";
        xml += "    <page>" + std::to_string(page) + "</page>\n";
        xml += "    <perpage>" + std::to_string(perPage) + "</perpage>\n";
        xml += "    <characters>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            const char* charName = row.GetText(1);
            const char* corpName = row.GetText(9);
            const char* ticker = row.GetText(10);
            const char* sysName = row.GetText(11);
            const char* shipName = row.GetText(13);
            xml += "      <row characterid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " charactername=\"" + xmlEscape(charName) + "\"";
            xml += " skillpoints=\"" + std::to_string(row.GetInt64(2)) + "\"";
            xml += " corporationid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " corporationname=\"" + xmlEscape(corpName) + "\"";
            xml += " ticker=\"" + xmlEscape(ticker) + "\"";
            xml += " securityrating=\"" + std::string(row.GetText(4)) + "\"";
            xml += " raceid=\"" + std::to_string(row.GetInt(5)) + "\"";
            xml += " gender=\"" + std::to_string(row.GetInt(6)) + "\"";
            xml += " systemid=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " systemname=\"" + xmlEscape(sysName) + "\"";
            xml += " stationid=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " shiptypeid=\"" + std::to_string(row.GetUInt(12)) + "\"";
            xml += " shipname=\"" + xmlEscape(shipName) + "\"";
            xml += "/>\n";
        }
        xml += "    </characters>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "CharacterInfo.xml.aspx") {
        std::string cid = get("characterid");
        if (cid.empty()) return BuildErrorXML("105", "Missing characterID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT c.characterID, c.characterName, c.skillPoints, c.corporationID, "
            "c.securityRating, c.raceID, c.gender, c.solarSystemID, c.stationID, "
            "cr.corporationName, cr.tickerName, "
            "ss.solarSystemName, "
            "e.typeID as shipTypeID, t.typeName as shipName "
            "FROM chrCharacters c "
            "LEFT JOIN crpCorporation cr ON cr.corporationID = c.corporationID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = c.solarSystemID "
            "LEFT JOIN entity e ON e.itemID = c.shipID "
            "LEFT JOIN invTypes t ON t.typeID = e.typeID "
            "WHERE c.characterID = %u", std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1004", "Character not found.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <characterid>" + std::to_string(row.GetUInt(0)) + "</characterid>\n";
        xml += "    <charactername>" + xmlEscape(row.GetText(1)) + "</charactername>\n";
        xml += "    <skillpoints>" + std::to_string(row.GetInt64(2)) + "</skillpoints>\n";
        xml += "    <corporationid>" + std::to_string(row.GetUInt(3)) + "</corporationid>\n";
        xml += "    <corporationname>" + xmlEscape(row.GetText(9)) + "</corporationname>\n";
        xml += "    <ticker>" + xmlEscape(row.GetText(10)) + "</ticker>\n";
        xml += "    <securityrating>" + std::string(row.GetText(4)) + "</securityrating>\n";
        xml += "    <raceid>" + std::to_string(row.GetInt(5)) + "</raceid>\n";
        xml += "    <gender>" + std::to_string(row.GetInt(6)) + "</gender>\n";
        xml += "    <systemid>" + std::to_string(row.GetUInt(7)) + "</systemid>\n";
        xml += "    <systemname>" + xmlEscape(row.GetText(11)) + "</systemname>\n";
        xml += "    <stationid>" + std::to_string(row.GetUInt(8)) + "</stationid>\n";
        xml += "    <shiptypeid>" + std::to_string(row.GetUInt(12)) + "</shiptypeid>\n";
        xml += "    <shipname>" + xmlEscape(row.GetText(13)) + "</shipname>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "RelatedKills.xml.aspx") {
        std::string kid = get("killid");
        if (kid.empty()) return BuildErrorXML("105", "Missing killID.");

        // find kills in same system within ±24h
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT k2.killID, k2.solarSystemID, k2.killTime, "
            "k2.victimCharacterID, k2.victimShipTypeID, k2.victimDamageTaken, "
            "k2.finalCharacterID, k2.finalShipTypeID, k2.finalDamageDone, "
            "vc.characterName, fc.characterName, "
            "iv.typeName, if_.typeName "
            "FROM chrKillTable k1 "
            "JOIN chrKillTable k2 ON k2.solarSystemID = k1.solarSystemID "
            "AND k2.killID != k1.killID "
            "AND ABS(TIMESTAMPDIFF(SECOND, "
            "FROM_UNIXTIME((k2.killTime - 116444736000000000) / 10000000), "
            "FROM_UNIXTIME((k1.killTime - 116444736000000000) / 10000000))) < 86400 "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k2.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k2.finalCharacterID "
            "LEFT JOIN invTypes iv ON iv.typeID = k2.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k2.finalShipTypeID "
            "WHERE k1.killID = %u "
            "ORDER BY k2.killTime DESC LIMIT 20", std::stoul(kid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <related>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row killid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " systemid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " killtime=\"" + std::string(row.GetText(2)) + "\"";
            xml += " victimid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " victimshiptypeid=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " victimdamagetaken=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " finalid=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " finalshiptypeid=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " finaldamagedone=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " victimname=\"" + xmlEscape(row.GetText(9)) + "\"";
            xml += " finalname=\"" + xmlEscape(row.GetText(10)) + "\"";
            xml += " victimshipname=\"" + xmlEscape(row.GetText(11)) + "\"";
            xml += " finalshipname=\"" + xmlEscape(row.GetText(12)) + "\"";
            xml += "/>\n";
        }
        xml += "    </related>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "KillMail.xml.aspx") {
        std::string kid = get("killid");
        if (kid.empty()) return BuildErrorXML("105", "Missing killID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT k.killID, k.killTime, k.solarSystemID, k.victimDamageTaken, "
            "k.victimCharacterID, k.victimCorporationID, k.victimAllianceID, k.victimShipTypeID, "
            "k.finalCharacterID, k.finalCorporationID, k.finalShipTypeID, k.finalWeaponTypeID, "
            "k.finalDamageDone, k.finalSecurityStatus, k.killBlob, "
            "vc.characterName, fc.characterName, "
            "crp1.corporationName, crp2.corporationName, "
            "iv.typeName, if_.typeName, iw.typeName, "
            "ss.solarSystemName "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN crpCorporation crp1 ON crp1.corporationID = k.victimCorporationID "
            "LEFT JOIN crpCorporation crp2 ON crp2.corporationID = k.finalCorporationID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN invTypes iw ON iw.typeID = k.finalWeaponTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "WHERE k.killID = %u", std::stoul(kid)))
            return BuildErrorXML("999", "Query failed.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1004", "Kill not found.");

        // Build EVE-style killmail text
        // NOTE: GetText returns nullptr for NULL columns — wrap in safe helper
        auto safeText = [](const char* s) -> std::string {
            return (s != nullptr) ? std::string(s) : "";
        };
        std::string killmail;
        std::string vName = safeText(row.GetText(15));
        std::string vCorp = safeText(row.GetText(17));
        std::string vShip = safeText(row.GetText(19));
        killmail += "Victim: " + vName + ", Corp: " + vCorp + "\n";
        // victim alliance via alnAlliance (victimAllianceID = col 6)
        if (row.GetInt(6) > 0) {
            std::string allyName;
            DBQueryResult aRes;
            if (sDatabase.RunQuery(aRes, "SELECT allianceName FROM alnAlliance WHERE allianceID = %u", row.GetUInt(6))) {
                DBResultRow aRow;
                if (aRes.GetRow(aRow)) allyName = safeText(aRow.GetText(0));
            }
            if (!allyName.empty()) killmail += "Alliance: " + allyName + "\n";
        }
        killmail += "System: " + safeText(row.GetText(22)) + " (" + safeText(row.GetText(13)) + ")\n";
        killmail += "Damage Taken: " + std::to_string(row.GetUInt(3)) + "\n";
        killmail += "\n";
        killmail += "Final Blow: " + safeText(row.GetText(16)) + " flying " + safeText(row.GetText(20)) + "\n";
        killmail += "Corp: " + safeText(row.GetText(18)) + "\n";
        killmail += "Damage Done: " + std::to_string(row.GetUInt(12)) + "\n";
        killmail += "\nDetails:\n";

        // parse killBlob for items
        const char* blob = row.GetText(14);
        if (blob && strlen(blob) > 10) {
            std::string blobStr(blob);
            // simple XML parse for items
            size_t pos = 0;
            while ((pos = blobStr.find("<i ", pos)) != std::string::npos) {
                size_t end = blobStr.find("/>", pos);
                if (end == std::string::npos) break;
                std::string item = blobStr.substr(pos + 3, end - pos - 3);
                // extract t= and q=
                size_t tPos = item.find("t=");
                size_t qPos = item.find("q=");
                if (tPos != std::string::npos) {
                    uint32 typeID = std::stoul(item.substr(tPos + 2));
                    uint32 qty = 1;
                    if (qPos != std::string::npos) qty = std::stoul(item.substr(qPos + 2));
                    // get type name
                    std::string typeName;
                    DBQueryResult tRes;
                    if (sDatabase.RunQuery(tRes, "SELECT typeName FROM invTypes WHERE typeID = %u", typeID)) {
                        DBResultRow tRow;
                        if (tRes.GetRow(tRow)) typeName = tRow.GetText(0);
                    }
                    if (typeName.empty()) typeName = "Unknown Type " + std::to_string(typeID);
                    killmail += typeName + " x " + std::to_string(qty) + "\n";
                }
                pos = end + 2;
            }
        }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <killmail>" + xmlEscape(killmail.c_str()) + "</killmail>\n";
        xml += "    <killblob>" + xmlEscape(blob ? blob : "") + "</killblob>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "KillDetail.xml.aspx") {
        std::string kid = get("killid");
        if (kid.empty()) return BuildErrorXML("105", "Missing killID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT k.killID, k.solarSystemID, k.killTime, k.victimDamageTaken, k.finalDamageDone, "
            "k.victimCharacterID, k.victimCorporationID, k.victimAllianceID, k.victimFactionID, k.victimShipTypeID, "
            "k.finalCharacterID, k.finalCorporationID, k.finalAllianceID, k.finalFactionID, k.finalShipTypeID, "
            "k.finalWeaponTypeID, k.finalSecurityStatus, k.moonID, k.killBlob, "
            "vc.characterName, fc.characterName, "
            "vcc.corporationName, fcc.corporationName, vcc.tickerName, fcc.tickerName, "
            "va.allianceName, fa.allianceName, "
            "iv.typeName, if_.typeName, iw.typeName, "
            "ss.solarSystemName, ss.security, ss.regionID, ss.constellationID, "
            "con.constellationName, reg.regionName "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN crpCorporation vcc ON vcc.corporationID = k.victimCorporationID "
            "LEFT JOIN crpCorporation fcc ON fcc.corporationID = k.finalCorporationID "
            "LEFT JOIN alnAlliance va ON va.allianceID = k.victimAllianceID "
            "LEFT JOIN alnAlliance fa ON fa.allianceID = k.finalAllianceID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN invTypes iw ON iw.typeID = k.finalWeaponTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "LEFT JOIN mapConstellations con ON con.constellationID = ss.constellationID "
            "LEFT JOIN mapRegions reg ON reg.regionID = ss.regionID "
            "WHERE k.killID = %u", std::stoul(kid)))
            return BuildErrorXML("999", "Query failed.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1004", "Kill not found.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <row ";
        xml += "killid=\"" + std::to_string(row.GetUInt(0)) + "\"";
        xml += " systemid=\"" + std::to_string(row.GetUInt(1)) + "\"";
        xml += " systemname=\"" + xmlEscape(row.GetText(30)) + "\"";
        xml += " security=\"" + xmlEscape(row.GetText(31)) + "\"";
        xml += " regionid=\"" + std::to_string(row.GetUInt(32)) + "\"";
        xml += " regionname=\"" + xmlEscape(row.GetText(35)) + "\"";
        xml += " constellationid=\"" + std::to_string(row.GetUInt(33)) + "\"";
        xml += " constellationname=\"" + xmlEscape(row.GetText(34)) + "\"";
        xml += " killtime=\"" + std::string(row.GetText(2)) + "\"";
        xml += " moonid=\"" + std::to_string(row.GetUInt(17)) + "\"";

        xml += " victimid=\"" + std::to_string(row.GetUInt(5)) + "\"";
        xml += " victimname=\"" + xmlEscape(row.GetText(19)) + "\"";
        xml += " victimcorpid=\"" + std::to_string(row.GetUInt(6)) + "\"";
        xml += " victimcorpname=\"" + xmlEscape(row.GetText(21)) + "\"";
        xml += " victimticker=\"" + xmlEscape(row.GetText(23)) + "\"";
        xml += " victimallianceid=\"" + std::to_string(row.GetUInt(7)) + "\"";
        xml += " victimalliancename=\"" + xmlEscape(row.GetText(25)) + "\"";
        xml += " victimfactionid=\"" + std::to_string(row.GetUInt(8)) + "\"";
        xml += " victimshiptypeid=\"" + std::to_string(row.GetUInt(9)) + "\"";
        xml += " victimshipname=\"" + xmlEscape(row.GetText(27)) + "\"";
        xml += " victimdamagetaken=\"" + std::to_string(row.GetUInt(3)) + "\"";

        xml += " finalid=\"" + std::to_string(row.GetUInt(10)) + "\"";
        xml += " finalname=\"" + xmlEscape(row.GetText(20)) + "\"";
        xml += " finalcorpid=\"" + std::to_string(row.GetUInt(11)) + "\"";
        xml += " finalcorpname=\"" + xmlEscape(row.GetText(22)) + "\"";
        xml += " finalticker=\"" + xmlEscape(row.GetText(24)) + "\"";
        xml += " finalallianceid=\"" + std::to_string(row.GetUInt(12)) + "\"";
        xml += " finalalliancename=\"" + xmlEscape(row.GetText(26)) + "\"";
        xml += " finalfactionid=\"" + std::to_string(row.GetUInt(13)) + "\"";
        xml += " finalshiptypeid=\"" + std::to_string(row.GetUInt(14)) + "\"";
        xml += " finalshipname=\"" + xmlEscape(row.GetText(28)) + "\"";
        xml += " finalweapontypeid=\"" + std::to_string(row.GetUInt(15)) + "\"";
        xml += " finalweaponname=\"" + xmlEscape(row.GetText(29)) + "\"";
        xml += " finalsecstatus=\"" + std::string(row.GetText(16)) + "\"";
        xml += " finaldamagedone=\"" + std::to_string(row.GetUInt(4)) + "\"";

        const char* blob2 = row.GetText(18);
        xml += " killblob=\"" + xmlEscape(blob2 ? blob2 : "") + "\"";

        // victim ship slot counts (for the fitting-window style fit schematic on the portal)
        uint32 vShipTypeID = row.GetUInt(9);
        {
            DBQueryResult sRes;
            if (sDatabase.RunQuery(sRes,
                "SELECT "
                "  (SELECT valueInt FROM dgmTypeAttributes WHERE typeID = %u AND attributeID = 14), "   // hiSlots
                "  (SELECT valueInt FROM dgmTypeAttributes WHERE typeID = %u AND attributeID = 13), "   // medSlots
                "  (SELECT valueInt FROM dgmTypeAttributes WHERE typeID = %u AND attributeID = 12), "   // lowSlots
                "  (SELECT valueInt FROM dgmTypeAttributes WHERE typeID = %u AND attributeID = 1137), " // rigSlots
                "  (SELECT valueInt FROM dgmTypeAttributes WHERE typeID = %u AND attributeID = 1366) ", // subSystemSlot
                vShipTypeID, vShipTypeID, vShipTypeID, vShipTypeID, vShipTypeID)) {
                DBResultRow sRow;
                if (sRes.GetRow(sRow)) {
                    xml += " hipslots=\"" + std::to_string(sRow.GetInt(0)) + "\"";
                    xml += " midslots=\"" + std::to_string(sRow.GetInt(1)) + "\"";
                    xml += " lowslots=\"" + std::to_string(sRow.GetInt(2)) + "\"";
                    xml += " rigslots=\"" + std::to_string(sRow.GetInt(3)) + "\"";
                    xml += " subslots=\"" + std::to_string(sRow.GetInt(4)) + "\"";
                }
            }
        }

        xml += "/>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
