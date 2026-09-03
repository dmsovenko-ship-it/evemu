#include "eve-server.h"
#include "apiserver/APICharacterManager.h"

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
        std::string q = "SELECT killID, solarSystemID, victimCharacterID, victimCorporationID, "
            "victimAllianceID, victimFactionID, victimShipTypeID, victimDamageTaken, "
            "finalCharacterID, finalCorporationID, finalAllianceID, finalFactionID, "
            "finalShipTypeID, finalWeaponTypeID, finalSecurityStatus, finalDamageDone, "
            "killTime, moonID FROM chrKillTable "
            "WHERE (victimCharacterID = " + cid + " OR finalCharacterID = " + cid + ")";
        if (beforeKillID > 0)
            q += " AND killID < " + std::to_string(beforeKillID);
        q += " ORDER BY killID DESC LIMIT 2500";

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
            xml += " moonID=\"" + std::to_string(row.GetUInt(17)) + "\"/>\n";
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
            xml += " bid=\"" + (row.GetBool(11) ? "True" : "False") + "\"";
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
            xml += " standing=\"" + std::string(row.GetText(1)) + "\"/>\n";
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
            xml += " description=\"" + std::string(row.GetText(9)) + "\"/>\n";
        }
        xml += "    </transactions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
