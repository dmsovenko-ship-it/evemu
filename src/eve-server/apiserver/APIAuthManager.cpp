#include "eve-server.h"
#include "EVEServerConfig.h"
#include "auth/PasswordModule.h"
#include "apiserver/APIAuthManager.h"
#include "TelegramBot.h"
#include "ReservedNames.h"

// Reserved / forbidden account names (shared list).
static bool IsBannedAccountName(const std::string& name)
{
    return IsReservedName(name);
}

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

static bool IsValidEmail(const std::string& s) {
    if (s.empty() || s.length() > 60)
        return false;
    for (char c : s) {
        if ((unsigned char)c < 0x20 || c == ' ' || c == '\'' || c == '"' || c == '<' || c == '>' || c == '\\')
            return false;
    }
    size_t at = s.find('@');
    if (at == std::string::npos || at == 0 || at + 1 >= s.length())
        return false;
    if (s.find('@', at + 1) != std::string::npos)
        return false;
    std::string domain = s.substr(at + 1);
    return domain.find('.') != std::string::npos;
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
        std::string ip = get("ip");   // client IP passed by the portal

        if (name.empty() || pass.empty() || email.empty())
            return BuildErrorXML("105", "Name, password and email required.");
        if (name.length() < 3 || name.length() > 40)
            return BuildErrorXML("106", "Account name must be 3-40 characters.");
        if (pass.length() < 6)
            return BuildErrorXML("107", "Password must be at least 6 characters.");
        if (!IsValidEmail(email))
            return BuildErrorXML("110", "Invalid email address.");

        std::string nameEsc, emailEsc;
        sDatabase.DoEscapeString(nameEsc, name);
        sDatabase.DoEscapeString(emailEsc, email);

        // Attempts with reserved/offensive names are refused and reported to the
        // admin group (possible troll/RMT/impersonation).
        if (IsBannedAccountName(name)) {
            TelegramBot::NotifyAdmin("⛔ Запрещённое имя при регистрации: " + name
                + (ip.empty() ? "" : ", IP " + ip));
            return BuildErrorXML("109", "This account name is not allowed.");
        }

        // check if name exists
        DBQueryResult res;
        if (sDatabase.RunQuery(res, "SELECT accountID FROM account WHERE accountName = '%s'", nameEsc.c_str())) {
            DBResultRow row;
            if (res.GetRow(row)) {
                TelegramBot::NotifyAdmin("⚠️ Неудачная регистрация (имя занято): " + name
                    + (ip.empty() ? "" : ", IP " + ip));
                return BuildErrorXML("108", "Account name already exists.");
            }
        }
        // check if email already used by another account
        DBQueryResult em;
        if (sDatabase.RunQuery(em, "SELECT accountID FROM account WHERE email = '%s' AND email IS NOT NULL", emailEsc.c_str())) {
            DBResultRow emRow;
            if (em.GetRow(emRow)) {
                TelegramBot::NotifyAdmin("⚠️ Неудачная регистрация (email занят): " + email
                    + (ip.empty() ? "" : ", IP " + ip));
                return BuildErrorXML("111", "This email is already registered.");
            }
        }

        // store plain password (matches server's auth model)
        std::string escapedPass;
        sDatabase.DoEscapeString(escapedPass, pass);

        uint32 role = sConfig.account.autoAccountRole;
        if (role == 0) role = 1; // ROLE_PLAYER if no auto-role configured

        DBerror err;
        uint32 accountID = 0;
        if (!sDatabase.RunQueryLID(err, accountID,
            "INSERT INTO account (accountName, password, hash, role, type, email) VALUES ('%s', '%s', '', %u, 23, '%s')",
            nameEsc.c_str(), escapedPass.c_str(), role, emailEsc.c_str()))
            return BuildErrorXML("999", "Failed to create account.");

        TelegramBot::NotifyAdmin("✅ Новый аккаунт: " + name
            + " (id " + std::to_string(accountID) + ")"
            + ", email " + email
            + (ip.empty() ? "" : ", IP " + ip));

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

        std::string nameEsc;
        sDatabase.DoEscapeString(nameEsc, name);

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT accountID, HEX(hash), role, banned, password, accountName, email FROM account WHERE accountName = '%s'",
            nameEsc.c_str()))
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
        const char* emailCol = row.GetText(6);
        xml += "    <email>" + xmlEscape(emailCol != nullptr ? emailCol : "") + "</email>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    // SetEmail: let an existing account (e.g. created before email became
    // mandatory) attach a contact e-mail. Verifies the password first.
    if (handler == "SetEmail.xml.aspx") {
        std::string name = get("name");
        std::string pass = get("password");
        std::string email = get("email");
        std::string ip = get("ip");

        if (name.empty() || pass.empty() || email.empty())
            return BuildErrorXML("105", "Name, password and email required.");
        if (!IsValidEmail(email))
            return BuildErrorXML("110", "Invalid email address.");

        std::string nameEsc, emailEsc;
        sDatabase.DoEscapeString(nameEsc, name);
        sDatabase.DoEscapeString(emailEsc, email);

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT accountID, HEX(hash), banned, password FROM account WHERE accountName = '%s'",
            nameEsc.c_str()))
            return BuildErrorXML("999", "Database error.");
        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1001", "Account not found.");
        if ((int)row.GetInt(2) != 0)
            return BuildErrorXML("1002", "Account is banned.");

        const char* storedHashHex = row.GetText(1);
        bool ok = false;
        if (storedHashHex != nullptr && strlen(storedHashHex) == 40) {
            std::string computed;
            if (PasswordModule::GeneratePassHash(name, pass, computed)) {
                std::string hex;
                hex.reserve(40);
                static const char* hx = "0123456789ABCDEF";
                for (unsigned char c : computed) { hex += hx[c >> 4]; hex += hx[c & 0xF]; }
                ok = (hex == storedHashHex);
            }
        } else {
            const char* storedPass = row.GetText(3);
            ok = (storedPass != nullptr) && (pass == storedPass);
        }
        if (!ok)
            return BuildErrorXML("1003", "Invalid password.");

        DBQueryResult em;
        if (sDatabase.RunQuery(em,
            "SELECT accountID FROM account WHERE email = '%s' AND email IS NOT NULL AND accountID <> %u",
            emailEsc.c_str(), row.GetUInt(0))) {
            DBResultRow emRow;
            if (em.GetRow(emRow))
                return BuildErrorXML("111", "This email is already registered.");
        }

        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE account SET email = '%s' WHERE accountID = %u", emailEsc.c_str(), row.GetUInt(0));

        TelegramBot::NotifyAdmin("✉️ Email установлен: " + name
            + " → " + email
            + (ip.empty() ? "" : ", IP " + ip));

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n    <ok>1</ok>\n  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
