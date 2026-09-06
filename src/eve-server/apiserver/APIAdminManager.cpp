#include "eve-server.h"
#include "apiserver/APIAdminManager.h"

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

std::string APIAdminManager::ProcessCall(const std::string& handler,
                                         const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    // account management
    if (handler == "AccountList.xml.aspx" || handler == "BanAccount.xml.aspx" ||
        handler == "UnbanAccount.xml.aspx")
        return ProcessAccounts(handler, params);

    // petitions
    if (handler == "PetitionList.xml.aspx" || handler == "PetitionClose.xml.aspx" ||
        handler == "PetitionReply.xml.aspx" || handler == "PetitionCreate.xml.aspx" ||
        handler == "PetitionMine.xml.aspx")
        return ProcessPetitions(handler, params);

    // timecodes
    if (handler == "TimecodeList.xml.aspx" || handler == "GrantTimecode.xml.aspx")
        return ProcessTimecodes(handler, params);

    // items
    if (handler == "GiveItem.xml.aspx")
        return ProcessItems(handler, params);

    // roles
    if (handler == "SetRole.xml.aspx")
        return ProcessRoles(handler, params);

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}

std::string APIAdminManager::ProcessAccounts(const std::string& handler,
                                             const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "AccountList.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT accountID, accountName, email, role, online, banned, logonCount, lastLogin "
            "FROM account ORDER BY accountID"))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <accounts>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            const char* email = row.GetText(2);
            xml += "      <row accountid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountname=\"" + xmlEscape(row.GetText(1)) + "\"";
            xml += " email=\"" + xmlEscape(email) + "\"";
            xml += " role=\"" + std::to_string(row.GetInt64(3)) + "\"";
            xml += " online=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " banned=\"" + std::to_string(row.GetInt(5)) + "\"";
            xml += " logoncount=\"" + std::to_string(row.GetUInt(6)) + "\"/>\n";
        }
        xml += "    </accounts>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "BanAccount.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty()) return BuildErrorXML("105", "Missing accountid.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE account SET banned = 1 WHERE accountID = %u", std::stoul(aid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    if (handler == "UnbanAccount.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty()) return BuildErrorXML("105", "Missing accountid.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE account SET banned = 0 WHERE accountID = %u", std::stoul(aid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    return BuildErrorXML("9999", "Unknown handler");
}

std::string APIAdminManager::ProcessPetitions(const std::string& handler,
                                              const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    // ensure table exists
    {
        DBerror err;
        sDatabase.RunQuery(err,
            "CREATE TABLE IF NOT EXISTS portal_petitions ("
            "petitionID INT UNSIGNED NOT NULL AUTO_INCREMENT, "
            "accountID INT UNSIGNED NOT NULL DEFAULT 0, "
            "authorName VARCHAR(40) NOT NULL DEFAULT '', "
            "subject VARCHAR(200) NOT NULL DEFAULT '', "
            "body TEXT, "
            "status TINYINT NOT NULL DEFAULT 1, "
            "createDate DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "PRIMARY KEY (petitionID)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    }

    if (handler == "PetitionList.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT petitionID, accountID, authorName, subject, body, status, createDate "
            "FROM portal_petitions ORDER BY petitionID DESC"))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <petitions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row petitionID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " authorName=\"" + xmlEscape(row.GetText(2)) + "\"";
            xml += " subject=\"" + xmlEscape(row.GetText(3)) + "\"";
            xml += " body=\"" + xmlEscape(row.GetText(4)) + "\"";
            xml += " status=\"" + std::to_string(row.GetInt(5)) + "\"";
            xml += " createDate=\"" + std::string(row.GetText(6)) + "\"/>\n";
        }
        xml += "    </petitions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    // A player's own petitions (list + statuses). The portal knows the logged-in
    // account; only that account's rows are returned.
    if (handler == "PetitionMine.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty()) return BuildErrorXML("105", "Missing accountid.");
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT petitionID, authorName, subject, body, status, createDate "
            "FROM portal_petitions WHERE accountID = %u ORDER BY petitionID DESC",
            std::stoul(aid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <petitions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row petitionID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " authorName=\"" + xmlEscape(row.GetText(1)) + "\"";
            xml += " subject=\"" + xmlEscape(row.GetText(2)) + "\"";
            xml += " body=\"" + xmlEscape(row.GetText(3)) + "\"";
            xml += " status=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " createDate=\"" + std::string(row.GetText(5)) + "\"/>\n";
        }
        xml += "    </petitions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    // A player submits a support petition from the portal. accountid/author are
    // taken from POST (the portal knows the logged-in account). body may span
    // several lines — escape it before embedding in the SQL string.
    if (handler == "PetitionCreate.xml.aspx") {
        std::string aid   = get("accountid");
        std::string author = get("author");
        std::string subject = get("subject");
        std::string body   = get("body");
        if (aid.empty() || subject.empty() || body.empty())
            return BuildErrorXML("105", "Missing accountid, subject or body.");
        std::string sEsc, bEsc, aEsc;
        sDatabase.DoEscapeString(sEsc, subject);
        sDatabase.DoEscapeString(bEsc, body);
        sDatabase.DoEscapeString(aEsc, author);
        DBerror err;
        if (!sDatabase.RunQuery(err,
            "INSERT INTO portal_petitions (accountID, authorName, subject, body, status)"
            " VALUES (%u, '%s', '%s', '%s', 1)",
            std::stoul(aid), aEsc.c_str(), sEsc.c_str(), bEsc.c_str()))
            return BuildErrorXML("999", "Insert failed.");
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    if (handler == "PetitionClose.xml.aspx") {
        std::string pid = get("petitionid");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET status = 0 WHERE petitionID = %u", std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    if (handler == "PetitionReply.xml.aspx") {
        std::string pid = get("petitionid");
        std::string reply = get("reply");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");
        std::string rEsc;
        sDatabase.DoEscapeString(rEsc, reply);
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE portal_petitions SET body = CONCAT(COALESCE(body,''), '\n--- Admin reply ---\n', '%s') WHERE petitionID = %u",
            rEsc.c_str(), std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    return BuildErrorXML("9999", "Unknown handler");
}

std::string APIAdminManager::ProcessTimecodes(const std::string& handler,
                                              const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    {
        DBerror err;
        sDatabase.RunQuery(err,
            "CREATE TABLE IF NOT EXISTS portal_timecodes ("
            "id INT UNSIGNED NOT NULL AUTO_INCREMENT, "
            "accountID INT UNSIGNED NOT NULL DEFAULT 0, "
            "days INT UNSIGNED NOT NULL DEFAULT 0, "
            "grantDate DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "PRIMARY KEY (id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    }

    if (handler == "TimecodeList.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT id, accountID, days, grantDate FROM portal_timecodes ORDER BY id DESC LIMIT 500"))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <timecodes>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row id=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " days=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " grantDate=\"" + std::string(row.GetText(3)) + "\"/>\n";
        }
        xml += "    </timecodes>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "GrantTimecode.xml.aspx") {
        std::string aid = get("accountid");
        std::string days = get("days");
        if (aid.empty() || days.empty()) return BuildErrorXML("105", "Missing accountid or days.");
        DBerror err;
        sDatabase.RunQuery(err, "INSERT INTO portal_timecodes (accountID, days) VALUES (%u, %u)",
            std::stoul(aid), std::stoul(days));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    return BuildErrorXML("9999", "Unknown handler");
}

std::string APIAdminManager::ProcessItems(const std::string& handler,
                                          const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "GiveItem.xml.aspx") {
        std::string cid = get("characterid");
        std::string tid = get("typeid");
        std::string qty = get("quantity");
        if (cid.empty() || tid.empty()) return BuildErrorXML("105", "Missing characterid or typeid.");

        uint32 charID = std::stoul(cid);
        uint32 typeID = std::stoul(tid);
        uint32 quantity = qty.empty() ? 1 : std::stoul(qty);

        // get the character's location (station or solar system)
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT locationID, corporationID FROM chrCharacters WHERE characterID = %u", charID))
            return BuildErrorXML("999", "Character not found.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1004", "Character not found.");

        uint32 locationID = row.GetUInt(0);

        // create the item in the character's hangar (flag 5 = Hangar)
        DBerror err;
        uint32 itemID = 0;
        if (!sDatabase.RunQueryLID(err, itemID,
            "INSERT INTO entity (typeID, ownerID, locationID, flag, quantity, customName) "
            "VALUES (%u, %u, %u, 5, %u, '')",
            typeID, charID, locationID, quantity))
            return BuildErrorXML("999", "Failed to create item.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <itemid>" + std::to_string(itemID) + "</itemid>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler");
}

std::string APIAdminManager::ProcessRoles(const std::string& handler,
                                          const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    if (handler == "SetRole.xml.aspx") {
        std::string aid = get("accountid");
        std::string role = get("role");
        if (aid.empty() || role.empty()) return BuildErrorXML("105", "Missing accountid or role.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE account SET role = %lld WHERE accountID = %u",
            std::stoll(role), std::stoul(aid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    return BuildErrorXML("9999", "Unknown handler");
}
