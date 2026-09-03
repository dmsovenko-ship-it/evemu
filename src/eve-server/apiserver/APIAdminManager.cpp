#include "eve-server.h"
#include "apiserver/APIAdminManager.h"

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
        handler == "PetitionReply.xml.aspx")
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
            xml += "      <row accountID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountName=\"" + std::string(row.GetText(1)) + "\"";
            xml += " email=\"" + std::string(email ? email : "") + "\"";
            xml += " role=\"" + std::to_string(row.GetInt64(3)) + "\"";
            xml += " online=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " banned=\"" + std::to_string(row.GetInt(5)) + "\"";
            xml += " logonCount=\"" + std::to_string(row.GetUInt(6)) + "\"/>\n";
        }
        xml += "    </accounts>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "BanAccount.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty()) return BuildErrorXML("105", "Missing accountid.");
        sDatabase.RunQuery("UPDATE account SET banned = 1 WHERE accountID = %u", std::stoul(aid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    if (handler == "UnbanAccount.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty()) return BuildErrorXML("105", "Missing accountid.");
        sDatabase.RunQuery("UPDATE account SET banned = 0 WHERE accountID = %u", std::stoul(aid));
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
    sDatabase.RunQuery(
        "CREATE TABLE IF NOT EXISTS portal_petitions ("
        "petitionID INT UNSIGNED NOT NULL AUTO_INCREMENT, "
        "accountID INT UNSIGNED NOT NULL DEFAULT 0, "
        "authorName VARCHAR(40) NOT NULL DEFAULT '', "
        "subject VARCHAR(200) NOT NULL DEFAULT '', "
        "body TEXT, "
        "status TINYINT NOT NULL DEFAULT 1, "
        "createDate DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "PRIMARY KEY (petitionID)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

    if (handler == "PetitionList.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT petitionID, accountID, authorName, subject, status, createDate "
            "FROM portal_petitions ORDER BY petitionID DESC"))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <petitions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row petitionID=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountID=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " authorName=\"" + std::string(row.GetText(2)) + "\"";
            xml += " subject=\"" + std::string(row.GetText(3)) + "\"";
            xml += " status=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " createDate=\"" + std::string(row.GetText(5)) + "\"/>\n";
        }
        xml += "    </petitions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    if (handler == "PetitionClose.xml.aspx") {
        std::string pid = get("petitionid");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");
        sDatabase.RunQuery("UPDATE portal_petitions SET status = 0 WHERE petitionID = %u", std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    if (handler == "PetitionReply.xml.aspx") {
        std::string pid = get("petitionid");
        std::string reply = get("reply");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");
        // store reply as body update for now
        sDatabase.RunQuery("UPDATE portal_petitions SET body = CONCAT(COALESCE(body,''), '\n--- Admin reply ---\n', '%s') WHERE petitionID = %u",
            reply.c_str(), std::stoul(pid));
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

    sDatabase.RunQuery(
        "CREATE TABLE IF NOT EXISTS portal_timecodes ("
        "id INT UNSIGNED NOT NULL AUTO_INCREMENT, "
        "accountID INT UNSIGNED NOT NULL DEFAULT 0, "
        "days INT UNSIGNED NOT NULL DEFAULT 0, "
        "grantDate DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "PRIMARY KEY (id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

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
        sDatabase.RunQuery("INSERT INTO portal_timecodes (accountID, days) VALUES (%u, %u)",
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
        uint32 corpID = row.GetUInt(1);

        // create the item in the character's hangar (flag 4 = Cargo, or flag 5 = Hangar)
        uint32 itemID = 0;
        if (!sDatabase.RunQuery(itemID,
            "INSERT INTO entity (typeID, ownerID, locationID, flag, quantity, customName) "
            "VALUES (%u, %u, %u, 5, %u, '')",
            typeID, charID, locationID, quantity))
            return BuildErrorXML("999", "Failed to create item.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <itemID>" + std::to_string(itemID) + "</itemID>\n";
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
        sDatabase.RunQuery("UPDATE account SET role = %lld WHERE accountID = %u",
            std::stoll(role), std::stoul(aid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    return BuildErrorXML("9999", "Unknown handler");
}
