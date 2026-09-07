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
        handler == "PetitionMine.xml.aspx" || handler == "PetitionMessages.xml.aspx" ||
        handler == "PetitionAddMessage.xml.aspx" || handler == "PetitionCancel.xml.aspx" ||
        handler == "PetitionCategories.xml.aspx")
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

    // Language used for category names on the portal (defaults to Russian).
    std::string lang = get("language");
    if (lang.empty()) lang = "ru";
    // whitelist — never interpolate arbitrary client text into SQL
    if (lang != "ru" && lang != "en-us" && lang != "RU" && lang != "EN")
        lang = "ru";

    // Shared SELECT: one row per petition + its localized category name.
    // `lang` is whitelisted above, safe to splice.
    std::string petitionBase =
        "SELECT p.petitionID, p.accountID, p.characterID, p.authorName, p.categoryID,"
        " COALESCE(cat.categoryName, '') AS categoryName,"
        " p.subject, p.status, p.claimedBy, p.updated, p.deleted,"
        " p.createDate, p.touchDate"
        " FROM portal_petitions p"
        " LEFT JOIN portal_petition_categories cat"
        "   ON cat.categoryID = p.categoryID AND cat.languageID = '" + lang + "' ";

    // All petitions (admin).
    if (handler == "PetitionList.xml.aspx") {
        DBQueryResult res;
        std::string q = petitionBase + " ORDER BY p.petitionID DESC";
        if (!sDatabase.RunQuery(res, q.c_str()))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <petitions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row petitionid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " characterid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " authorname=\"" + xmlEscape(row.GetText(3)) + "\"";
            xml += " categoryid=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " categoryname=\"" + xmlEscape(row.GetText(5)) + "\"";
            xml += " subject=\"" + xmlEscape(row.GetText(6)) + "\"";
            xml += " status=\"" + std::to_string(row.GetInt(7)) + "\"";
            xml += " claimedby=\"" + std::to_string(row.GetUInt(8)) + "\"";
            xml += " updated=\"" + std::to_string(row.GetInt(9)) + "\"";
            xml += " deleted=\"" + std::to_string(row.GetInt(10)) + "\"";
            xml += " createdate=\"" + std::string(row.GetText(11)) + "\"";
            xml += " touchdate=\"" + std::string(row.GetText(12)) + "\"/>\n";
        }
        xml += "    </petitions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    // A player's own petitions (accountID-based; covers both portal rows and
    // in-game rows filed by characters of the account).  Players never see the
    // rows of other accounts.
    if (handler == "PetitionMine.xml.aspx") {
        std::string aid = get("accountid");
        if (aid.empty() || aid.find_first_not_of("0123456789") != std::string::npos)
            return BuildErrorXML("105", "Missing accountid.");
        DBQueryResult res;
        std::string q = petitionBase + " WHERE p.accountID = " + aid + " AND p.deleted = 0"
                      + " ORDER BY p.petitionID DESC";
        if (!sDatabase.RunQuery(res, q.c_str()))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <petitions>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row petitionid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " accountid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " characterid=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " authorname=\"" + xmlEscape(row.GetText(3)) + "\"";
            xml += " categoryid=\"" + std::to_string(row.GetUInt(4)) + "\"";
            xml += " categoryname=\"" + xmlEscape(row.GetText(5)) + "\"";
            xml += " subject=\"" + xmlEscape(row.GetText(6)) + "\"";
            xml += " status=\"" + std::to_string(row.GetInt(7)) + "\"";
            xml += " updated=\"" + std::to_string(row.GetInt(9)) + "\"";
            xml += " createdate=\"" + std::string(row.GetText(11)) + "\"";
            xml += " touchdate=\"" + std::string(row.GetText(12)) + "\"/>\n";
        }
        xml += "    </petitions>\n  </result>\n</eveapi>\n";
        return xml;
    }

    // A player submits a petition from the portal. accountid is the session
    // account; author is the display name (prefer first character).  A category
    // is optional; body becomes the first message in the shared thread so the
    // in-game client sees the same conversation.
    if (handler == "PetitionCreate.xml.aspx") {
        std::string aid     = get("accountid");
        std::string author  = get("author");
        std::string subject = get("subject");
        std::string body    = get("body");
        std::string category= get("categoryid");
        if (aid.empty() || subject.empty() || body.empty())
            return BuildErrorXML("105", "Missing accountid, subject or body.");
        uint32 categoryID = category.empty() ? 0 : std::stoul(category);

        std::string sEsc, bEsc, aEsc;
        sDatabase.DoEscapeString(sEsc, subject);
        sDatabase.DoEscapeString(bEsc, body);
        sDatabase.DoEscapeString(aEsc, author);
        uint32 charID = 0;
        std::string sid = get("senderid");
        if (!sid.empty()) charID = std::stoul(sid);
        DBerror err;
        uint32 petitionID = 0;
        if (!sDatabase.RunQueryLID(err, petitionID,
            "INSERT INTO portal_petitions"
            " (accountID, characterID, authorName, categoryID, subject, body, status, updated, createDate, touchDate)"
            " VALUES (%u, 0, '%s', %u, '%s', '%s', 1, 0, NOW(), NOW())",
            std::stoul(aid), aEsc.c_str(), categoryID, sEsc.c_str(), bEsc.c_str()))
            return BuildErrorXML("999", "Insert failed.");

        // First message = the petition body, so the conversation is uniform.
        sDatabase.RunQuery(err,
            "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
            " VALUES (%u, %u, '%s', 0, 0, '%s', NOW())",
            petitionID, charID, aEsc.c_str(), bEsc.c_str());

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <ok/>\n    <petitionid>" + std::to_string(petitionID) + "</petitionid>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    // Full message thread of one petition.  Admin may read any; a player must
    // pass accountid and only sees rows owned by that account.
    if (handler == "PetitionMessages.xml.aspx") {
        std::string pid = get("petitionid");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");

        if (!get("accountid").empty()) {
            DBQueryResult own;
            if (!sDatabase.RunQuery(own,
                "SELECT accountID FROM portal_petitions WHERE petitionID = %u", std::stoul(pid)))
                return BuildErrorXML("999", "Query failed.");
            DBResultRow orow;
            if (own.GetRow(orow) && orow.GetUInt(0) != std::stoul(get("accountid")))
                return BuildErrorXML("1004", "Not your petition.");
        }

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT messageID, senderID, senderName, isGM, comment, text, sentDate"
            " FROM portal_petition_messages WHERE petitionID = %u ORDER BY sentDate, messageID",
            std::stoul(pid)))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <messages>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row messageid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " senderid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " sendername=\"" + xmlEscape(row.GetText(2)) + "\"";
            xml += " isgm=\"" + std::to_string(row.GetInt(3)) + "\"";
            xml += " comment=\"" + std::to_string(row.GetInt(4)) + "\"";
            xml += " text=\"" + xmlEscape(row.GetText(5)) + "\"";
            xml += " sentdate=\"" + std::string(row.GetText(6)) + "\"/>\n";
        }
        xml += "    </messages>\n  </result>\n</eveapi>\n";
        return xml;
    }

    // Player adds a message to one of their own open petitions.
    if (handler == "PetitionAddMessage.xml.aspx") {
        std::string pid = get("petitionid");
        std::string aid = get("accountid");
        std::string msg = get("message");
        if (pid.empty() || aid.empty() || msg.empty())
            return BuildErrorXML("105", "Missing petitionid, accountid or message.");
        if (!PetitionOwnedBy(std::stoul(pid), std::stoul(aid)))
            return BuildErrorXML("1004", "Not your petition.");
        if (!PetitionIsOpen(std::stoul(pid)))
            return BuildErrorXML("1005", "Petition is closed.");

        std::string mEsc;
        sDatabase.DoEscapeString(mEsc, msg);
        std::string name = get("sendername");
        std::string nEsc;
        sDatabase.DoEscapeString(nEsc, name);
        uint32 charID = 0;
        std::string sid = get("senderid");
        if (!sid.empty()) charID = std::stoul(sid);
        DBerror err;
        sDatabase.RunQuery(err,
            "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
            " VALUES (%u, %u, '%s', 0, 0, '%s', NOW())",
            std::stoul(pid), charID, nEsc.c_str(), mEsc.c_str());
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET updated = 1, touchDate = NOW() WHERE petitionID = %u",
            std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    // Admin replies into the thread (isGM = 1 so the player sees it in game).
    if (handler == "PetitionReply.xml.aspx") {
        std::string pid = get("petitionid");
        std::string reply = get("reply");
        std::string gmName = get("adminname");
        if (pid.empty() || reply.empty()) return BuildErrorXML("105", "Missing petitionid or reply.");
        std::string rEsc, nEsc;
        sDatabase.DoEscapeString(rEsc, reply);
        sDatabase.DoEscapeString(nEsc, gmName);
        uint32 charID = 1;   // Eve System by default; real GM char via senderid if given
        std::string sid = get("senderid");
        if (!sid.empty()) charID = std::stoul(sid);
        DBerror err;
        sDatabase.RunQuery(err,
            "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
            " VALUES (%u, %u, '%s', 1, 0, '%s', NOW())",
            std::stoul(pid), charID, nEsc.c_str(), rEsc.c_str());
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET updated = 1, touchDate = NOW() WHERE petitionID = %u",
            std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    // Admin closes a petition.
    if (handler == "PetitionClose.xml.aspx") {
        std::string pid = get("petitionid");
        if (pid.empty()) return BuildErrorXML("105", "Missing petitionid.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET status = 0, touchDate = NOW() WHERE petitionID = %u",
            std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    // Player cancels (closes) one of their own open petitions.
    if (handler == "PetitionCancel.xml.aspx") {
        std::string pid = get("petitionid");
        std::string aid = get("accountid");
        if (pid.empty() || aid.empty())
            return BuildErrorXML("105", "Missing petitionid or accountid.");
        if (!PetitionOwnedBy(std::stoul(pid), std::stoul(aid)))
            return BuildErrorXML("1004", "Not your petition.");
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET status = 0, touchDate = NOW() WHERE petitionID = %u",
            std::stoul(pid));
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><result><ok/></result></eveapi>\n";
    }

    // Category tree for the portal "submit" form.  Emits groups (parent 0) and
    // leaf categories; the portal groups leaves under their group.
    if (handler == "PetitionCategories.xml.aspx") {
        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT categoryID, parentCategoryID, categoryName, description"
            " FROM portal_petition_categories WHERE languageID = '%s'"
            " ORDER BY parentCategoryID, sortOrder, categoryID",
            lang.c_str()))
            return BuildErrorXML("999", "Query failed.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <categories>\n";
        DBResultRow row;
        while (res.GetRow(row)) {
            xml += "      <row categoryid=\"" + std::to_string(row.GetUInt(0)) + "\"";
            xml += " parentcategoryid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " categoryname=\"" + xmlEscape(row.GetText(2)) + "\"";
            xml += " description=\"" + xmlEscape(row.GetText(3)) + "\"/>\n";
        }
        xml += "    </categories>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler");
}

// Portal ownership/state helpers ----------------------------------------------
bool APIAdminManager::PetitionOwnedBy(uint32 petitionID, uint32 accountID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT accountID FROM portal_petitions WHERE petitionID = %u", petitionID))
        return false;
    DBResultRow row;
    return res.GetRow(row) && row.GetUInt(0) == accountID;
}

bool APIAdminManager::PetitionIsOpen(uint32 petitionID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT status FROM portal_petitions WHERE petitionID = %u", petitionID))
        return false;
    DBResultRow row;
    return res.GetRow(row) && row.GetInt(0) == 1;
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
            xml += " accountid=\"" + std::to_string(row.GetUInt(1)) + "\"";
            xml += " days=\"" + std::to_string(row.GetUInt(2)) + "\"";
            xml += " grantdate=\"" + std::string(row.GetText(3)) + "\"/>\n";
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
