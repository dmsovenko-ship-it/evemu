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

    if (handler == "CorporationSheet.xml.aspx") {
        std::string cid = get("corporationid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid corporationID.");
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT c.corporationID, c.corporationName, c.tickerName, c.memberCount, "
            "c.ceoID, c.allianceID, ch.characterName "
            "FROM crpCorporation c "
            "LEFT JOIN chrCharacters ch ON ch.characterID = c.ceoID "
            "WHERE c.corporationID = %u", std::stoul(cid)))
            return BuildErrorXML("999", "Query failed.");
        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("404", "Corporation not found.");

        std::string aName, aTicker;
        uint32 allianceID = row.GetUInt(5);
        if (allianceID != 0) {
            DBQueryResult ar;
            if (sDatabase.RunQuery(ar, "SELECT allianceName, shortName FROM alnAlliance WHERE allianceID = %u", allianceID)) {
                DBResultRow arow;
                if (ar.GetRow(arow)) { aName = arow.GetText(0) ? arow.GetText(0) : ""; aTicker = arow.GetText(1) ? arow.GetText(1) : ""; }
            }
        }

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <corporationid>" + std::to_string(row.GetUInt(0)) + "</corporationid>\n";
        xml += "    <corporationname>" + xmlEscape(row.GetText(1)) + "</corporationname>\n";
        xml += "    <ticker>" + xmlEscape(row.GetText(2)) + "</ticker>\n";
        xml += "    <membercount>" + std::to_string(row.GetUInt(3)) + "</membercount>\n";
        xml += "    <ceoid>" + std::to_string(row.GetUInt(4)) + "</ceoid>\n";
        xml += "    <ceoname>" + xmlEscape(row.GetText(6)) + "</ceoname>\n";
        xml += "    <allianceid>" + std::to_string(allianceID) + "</allianceid>\n";
        xml += "    <alliancename>" + xmlEscape(aName.c_str()) + "</alliancename>\n";
        xml += "    <allianceticker>" + xmlEscape(aTicker.c_str()) + "</allianceticker>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "KillMails.xml.aspx") {
        std::string cid = get("corporationid");
        if (cid.empty()) return BuildErrorXML("105", "Invalid corporationID.");
        std::string beforeID = get("beforekillid");
        uint32 beforeKillID = beforeID.empty() ? 0 : std::stoul(beforeID);

        DBQueryResult res;
        std::string q = "SELECT k.killID, k.solarSystemID, k.victimCharacterID, k.victimCorporationID, "
            "k.victimAllianceID, k.victimFactionID, k.victimShipTypeID, k.victimDamageTaken, "
            "k.finalCharacterID, k.finalCorporationID, k.finalAllianceID, k.finalFactionID, "
            "k.finalShipTypeID, k.finalWeaponTypeID, k.finalSecurityStatus, k.finalDamageDone, "
            "k.killTime, k.moonID, "
            "vc.characterName, fc.characterName, "
            "iv.typeName, if_.typeName, "
            "ss.solarSystemName "
            "FROM chrKillTable k "
            "LEFT JOIN chrCharacters vc ON vc.characterID = k.victimCharacterID "
            "LEFT JOIN chrCharacters fc ON fc.characterID = k.finalCharacterID "
            "LEFT JOIN invTypes iv ON iv.typeID = k.victimShipTypeID "
            "LEFT JOIN invTypes if_ ON if_.typeID = k.finalShipTypeID "
            "LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = k.solarSystemID "
            "WHERE (k.victimCorporationID = " + cid + " OR k.finalCorporationID = " + cid + ")";
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
            xml += " victimname=\"" + xmlEscape(row.GetText(18)) + "\"";
            xml += " finalname=\"" + xmlEscape(row.GetText(19)) + "\"";
            xml += " victimshipname=\"" + xmlEscape(row.GetText(20)) + "\"";
            xml += " finalshipname=\"" + xmlEscape(row.GetText(21)) + "\"";
            xml += " solarsystemname=\"" + xmlEscape(row.GetText(22)) + "\"/>\n";
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
