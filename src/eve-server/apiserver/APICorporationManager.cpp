#include "eve-server.h"
#include "apiserver/APICorporationManager.h"

std::string APICorporationManager::ProcessCall(const std::string& handler,
                                               const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "KillMails.xml.aspx") {
        std::string cid = get("corporationid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid corporationID.");
        std::string beforeID = get("beforekillid");
        uint32 beforeKillID = beforeID.empty() ? 0 : std::stoul(beforeID);

        DBQueryResult res;
        std::string q = "SELECT killID, solarSystemID, victimCharacterID, victimCorporationID, "
            "victimAllianceID, victimFactionID, victimShipTypeID, victimDamageTaken, "
            "finalCharacterID, finalCorporationID, finalAllianceID, finalFactionID, "
            "finalShipTypeID, finalWeaponTypeID, finalSecurityStatus, finalDamageDone, "
            "killTime, moonID FROM chrKillTable "
            "WHERE (victimCorporationID = " + cid + " OR finalCorporationID = " + cid + ")";
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

    if (handler == "MemberTracking.xml.aspx") {
        std::string cid = get("corporationid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid corporationID.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT characterID, characterName, shipTypeID, solarSystemID, "
            "logonDateTime, logoffDateTime, logonMinutes, skillPoints, "
            "online, allianceID FROM chrCharacters "
            "WHERE corporationID = %u ORDER BY characterName", std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <rows>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row characterID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " name=\"" + std::string(row.GetText(1)) + "\"";
            xml += " shipTypeID=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " solarSystemID=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " logonDateTime=\"" + Win32TimeToString(row.GetInt64(4)) + "\"";
            xml += " logoffDateTime=\"" + Win32TimeToString(row.GetInt64(5)) + "\"";
            xml += " logonMinutes=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " skillPoints=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " online=\"" + (row.GetBool(8) ? "True" : "False") + "\"";
            xml += " allianceID=\"" + std::to_string(row.GetInt(9)) + "\"/>\n";
        }
        xml += "    </rows>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
