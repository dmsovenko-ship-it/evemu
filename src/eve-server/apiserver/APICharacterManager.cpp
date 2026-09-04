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
            xml += "      <row killID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " solarSystemID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " victimCharacterID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " victimCorporationID=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " victimAllianceID=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " victimFactionID=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " victimShipTypeID=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " victimDamageTaken=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " finalCharacterID=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " finalCorporationID=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " finalAllianceID=\"" + std::to_string(row.GetInt(10)) + "\"";
            xml += " finalFactionID=\"" + std::to_string(row.GetUInt(11)) + "\"";
            xml += " finalShipTypeID=\"" + std::to_string(row.GetUInt(12)) + "\"";
            xml += " finalWeaponTypeID=\"" + std::to_string(row.GetUInt(13)) + "\"";
            xml += " finalSecurityStatus=\"" + std::string(row.GetText(14)) + "\"";
            xml += " finalDamageDone=\"" + std::to_string(row.GetUInt(15)) + "\"";
            xml += " killTime=\"" + std::string(row.GetText(16)) + "\"";
            xml += " moonID=\"" + std::to_string(row.GetUInt(17)) + "\"";
            // resolved names
            const char* vName = row.GetText(18);
            const char* fName = row.GetText(19);
            const char* vShip = row.GetText(20);
            const char* fShip = row.GetText(21);
            const char* wName = row.GetText(22);
            const char* sName = row.GetText(23);
            xml += " victimName=\"" + xmlEscape(vName) + "\"";
            xml += " finalName=\"" + xmlEscape(fName) + "\"";
            xml += " victimShipName=\"" + xmlEscape(vShip) + "\"";
            xml += " finalShipName=\"" + xmlEscape(fShip) + "\"";
            xml += " finalWeaponName=\"" + xmlEscape(wName) + "\"";
            xml += " solarSystemName=\"" + xmlEscape(sName) + "\"";
            // killBlob for item drops
            const char* blob = row.GetText(24);
            xml += " killBlob=\"" + xmlEscape(blob) + "\"";
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
            xml += "      <row orderID=\"" + std::to_string(row.GetInt64(0)) + "\"";
            xml += " typeID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " stationID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " regionID=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " range=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " accountKey=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " duration=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " price=\"" + std::string(row.GetText(7)) + "\"";
            xml += " volEntered=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " volRemaining=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " minVolume=\"" + std::to_string(row.GetUInt(10)) + "\"";
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
            xml += "      <row fromID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " toID=\"" + cid + "\"";
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
            xml += "      <row transactionID=\"" + std::to_string(row.GetInt64(0)) + "\"";
            xml += " transactionDateTime=\"" + Win32TimeToString(row.GetInt64(1)) + "\"";
            xml += " referenceID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " entryTypeID=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " ownerID1=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " ownerID2=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " accountKey=\"" + std::to_string(row.GetUInt(6)) + "\"";
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
            xml += "      <row killID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " solarSystemID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " victimCharacterID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " victimCorporationID=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " victimAllianceID=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " victimFactionID=\"" + std::to_string(row.GetUInt(5)) + "\"";
            xml += " victimShipTypeID=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " victimDamageTaken=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " finalCharacterID=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " finalCorporationID=\"" + std::to_string(row.GetUInt(9)) + "\"";
            xml += " finalAllianceID=\"" + std::to_string(row.GetInt(10)) + "\"";
            xml += " finalFactionID=\"" + std::to_string(row.GetUInt(11)) + "\"";
            xml += " finalShipTypeID=\"" + std::to_string(row.GetUInt(12)) + "\"";
            xml += " finalWeaponTypeID=\"" + std::to_string(row.GetUInt(13)) + "\"";
            xml += " finalSecurityStatus=\"" + std::string(row.GetText(14)) + "\"";
            xml += " finalDamageDone=\"" + std::to_string(row.GetUInt(15)) + "\"";
            xml += " killTime=\"" + std::string(row.GetText(16)) + "\"";
            xml += " moonID=\"" + std::to_string(row.GetUInt(17)) + "\"";
            const char* vName = row.GetText(18);
            const char* fName = row.GetText(19);
            const char* vShip = row.GetText(20);
            const char* fShip = row.GetText(21);
            const char* wName = row.GetText(22);
            const char* sName = row.GetText(23);
            xml += " victimName=\"" + xmlEscape(vName) + "\"";
            xml += " finalName=\"" + xmlEscape(fName) + "\"";
            xml += " victimShipName=\"" + xmlEscape(vShip) + "\"";
            xml += " finalShipName=\"" + xmlEscape(fShip) + "\"";
            xml += " finalWeaponName=\"" + xmlEscape(wName) + "\"";
            xml += " solarSystemName=\"" + xmlEscape(sName) + "\"";
            const char* blob = row.GetText(24);
            xml += " killBlob=\"" + xmlEscape(blob) + "\"";
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
        // ship type names
        {
            DBQueryResult res;
            if (sDatabase.RunQuery(res,
                "SELECT typeID, typeName FROM invTypes WHERE typeID IN (%s)", ids.c_str())) {
                DBResultRow row;
                while (res.GetRow(row))
                    xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\" name=\"" + std::string(row.GetText(1)) + "\" type=\"ship\"/>\n";
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
        if (aid.empty()) return BuildErrorXML("105", "Missing accountID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT characterID, characterName, skillPoints, corporationID "
            "FROM chrCharacters WHERE accountID = %u ORDER BY characterID", std::stoul(aid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <characters>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row characterID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " characterName=\"" + std::string(row.GetText(1)) + "\"";
            xml += " skillPoints=\"" + std::to_string(row.GetInt64(2)) + "\"";
            xml += " corporationID=\"" + std::to_string(row.GetUInt(3)) + "\"/>\n";
        }
        xml += "    </characters>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
