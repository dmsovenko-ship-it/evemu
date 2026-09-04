#include "eve-server.h"
#include "apiserver/APICorporationManager.h"

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
            xml += " moonid=\"" + std::to_string(row.GetUInt(17)) + "\"/>\n";
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
            xml += "      <row characterid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " name=\"" + xmlEscape(row.GetText(1)) + "\"";
            xml += " shiptypeid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " solarsystemid=\"" + std::to_string(row.GetUInt(3)) + "\"";
            xml += " logondatetime=\"" + Win32TimeToString(row.GetInt64(4)) + "\"";
            xml += " logoffdatetime=\"" + Win32TimeToString(row.GetInt64(5)) + "\"";
            xml += " logonminutes=\"" + std::to_string(row.GetUInt(6)) + "\"";
            xml += " skillpoints=\"" + std::to_string(row.GetUInt(7)) + "\"";
            xml += " online=\"" + std::string(row.GetBool(8) ? "True" : "False") + "\"";
            xml += " allianceid=\"" + std::to_string(row.GetInt(9)) + "\"/>\n";
        }
        xml += "    </rows>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
