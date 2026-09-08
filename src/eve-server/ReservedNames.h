#ifndef EVEMU_EVESERVER_RESERVEDNAMES_H_
#define EVEMU_EVESERVER_RESERVEDNAMES_H_

#include <string>
#include <vector>
#include <cctype>

// Shared list of forbidden player-facing names (account + character). Kept in
// one place so portal registration and in-game character creation enforce the
// same rules.
inline bool IsReservedName(const std::string& name)
{
    std::string n;
    n.reserve(name.size());
    for (unsigned char c : name)
        n += (char)std::tolower(c);

    static const std::vector<std::string> reserved = {
        "admin", "administrator", "gm", "gamemaster", "ceo", "owner", "moderator",
        "support", "ccp", "concord", "eve", "system", "server", "test", "root",
        "god", "hitler", "adolf", "nazi", "fascist", "ss", "kike", "faggot",
        "nigger", "putin", "lenin", "stalin",
    };
    for (const auto& r : reserved)
        if (n == r) return true;

    static const std::vector<std::string> bannedParts = {
        "гитлер", "адольф", "нацист", "фашист", "хуй", "пизд", "бляд", "ебал",
        "ебат", "сука", "соси",
    };
    for (const auto& b : bannedParts)
        if (n.find(b) != std::string::npos) return true;

    return false;
}

#endif
