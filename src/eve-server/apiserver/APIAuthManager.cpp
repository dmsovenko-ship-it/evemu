#include "eve-server.h"
#include "EVEServerConfig.h"
#include "auth/PasswordModule.h"
#include "apiserver/APIAuthManager.h"

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

std::string APIAuthManager::ProcessCall(const std::string& handler,
                                        const std::map<std::string, std::string>& params)
{
    auto get = [&](const std::string& k) -> std::string {
        auto it = params.find(k);
        return it != params.end() ? it->second : "";
    };

    // Register: creates a new account
    if (handler == "Register.xml.aspx") {
        std::string name = get("name");
        std::string pass = get("password");
        std::string email = get("email");

        if (name.empty() || pass.empty())
            return BuildErrorXML("105", "Name and password required.");
        if (name.length() < 3 || name.length() > 40)
            return BuildErrorXML("106", "Account name must be 3-40 characters.");
        if (pass.length() < 6)
            return BuildErrorXML("107", "Password must be at least 6 characters.");

        // check if name exists
        DBQueryResult res;
        if (sDatabase.RunQuery(res, "SELECT accountID FROM account WHERE accountName = '%s'", name.c_str())) {
            DBResultRow row;
            if (res.GetRow(row))
                return BuildErrorXML("108", "Account name already exists.");
        }

        // store plain password (matches server's auth model)
        std::string escapedPass;
        sDatabase.DoEscapeString(escapedPass, pass);

        uint32 role = sConfig.account.autoAccountRole;
        if (role == 0) role = 1; // ROLE_PLAYER if no auto-role configured

        DBerror err;
        uint32 accountID = 0;
        if (!sDatabase.RunQueryLID(err, accountID,
            "INSERT INTO account (accountName, password, hash, role, type) VALUES ('%s', '%s', '', %u, 23)",
            name.c_str(), escapedPass.c_str(), role))
            return BuildErrorXML("999", "Failed to create account.");

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <accountid>" + std::to_string(accountID) + "</accountid>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    // Login: validate credentials, return account info
    if (handler == "Login.xml.aspx") {
        std::string name = get("name");
        std::string pass = get("password");

        if (name.empty() || pass.empty())
            return BuildErrorXML("105", "Name and password required.");

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT accountID, HEX(hash), role, banned, password, accountName FROM account WHERE accountName = '%s'",
            name.c_str()))
            return BuildErrorXML("999", "Database error.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1001", "Account not found.");

        if ((int)row.GetInt(3) != 0)
            return BuildErrorXML("1002", "Account is banned.");

        // CCP-style auth: hash = PasswordHash(username, password) (SHA1, 1000 iterations).
        // Account hash column holds the raw 20-byte digest the game client sent (HEX here).
        // If hash is empty (portal-created account), fall back to plaintext compare.
        const char* storedHashHex = row.GetText(1);
        bool ok = false;
        if (storedHashHex != nullptr && strlen(storedHashHex) == 40) {
            std::string computed;
            if (PasswordModule::GeneratePassHash(name, pass, computed)) {
                // hex-encode computed 20-byte digest
                std::string hex;
                hex.reserve(40);
                static const char* hx = "0123456789ABCDEF";
                for (unsigned char c : computed) {
                    hex += hx[c >> 4];
                    hex += hx[c & 0xF];
                }
                ok = (hex == storedHashHex);
            }
        } else {
            // plaintext fallback for portal-registered accounts
            const char* storedPass = row.GetText(4);
            ok = (storedPass != nullptr) && (pass == storedPass);
        }
        if (!ok)
            return BuildErrorXML("1003", "Invalid password.");

        // update login stats
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE account SET logonCount = logonCount + 1, lastLogin = NOW() WHERE accountID = %u", row.GetUInt(0));

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <accountid>" + std::to_string(row.GetUInt(0)) + "</accountid>\n";
        xml += "    <accountname>" + xmlEscape(row.GetText(5)) + "</accountname>\n";
        xml += "    <role>" + std::to_string(row.GetInt64(2)) + "</role>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
