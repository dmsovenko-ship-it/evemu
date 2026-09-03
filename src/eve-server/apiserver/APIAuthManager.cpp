#include "eve-server.h"
#include "EVEServerConfig.h"
#include "apiserver/APIAuthManager.h"

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
        xml += "    <accountID>" + std::to_string(accountID) + "</accountID>\n";
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
            "SELECT accountID, password, role, banned FROM account WHERE accountName = '%s'",
            name.c_str()))
            return BuildErrorXML("999", "Database error.");

        DBResultRow row;
        if (!res.GetRow(row))
            return BuildErrorXML("1001", "Account not found.");

        if ((int)row.GetInt(3) != 0)
            return BuildErrorXML("1002", "Account is banned.");

        // compare plain passwords (matches server's auth model)
        std::string storedPass = row.GetText(1);
        if (storedPass != pass)
            return BuildErrorXML("1003", "Invalid password.");

        // update login stats
        DBerror err;
        sDatabase.RunQuery(err, "UPDATE account SET logonCount = logonCount + 1, lastLogin = NOW() WHERE accountID = %u", row.GetUInt(0));

        std::string xml = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\">\n";
        xml += "  <currentTime>" + Win32TimeToString(GetFileTimeNow()) + "</currentTime>\n";
        xml += "  <result>\n";
        xml += "    <accountID>" + std::to_string(row.GetUInt(0)) + "</accountID>\n";
        xml += "    <accountName>" + std::string(row.GetText(0)) + "</accountName>\n";
        xml += "    <role>" + std::to_string(row.GetInt64(2)) + "</role>\n";
        xml += "  </result>\n</eveapi>\n";
        return xml;
    }

    return BuildErrorXML("9999", "Unknown handler: " + handler);
}
