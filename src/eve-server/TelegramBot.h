#ifndef EVEMU_EVESERVER_TELEGRAMBOT_H_
#define EVEMU_EVESERVER_TELEGRAMBOT_H_

#include <string>
#include <cstdlib>
#include <cctype>

#include "EVEServerConfig.h"

/*
 * Minimal Telegram notifier. Runs curl in a detached background shell so the
 * game loop is never blocked on the network call (same pattern as BotChat's
 * DeepSeek calls). Two audiences:
 *   - player group: public events (server news, downtime, ...)
 *   - admin group : closed channel (security / RMT / bot flags, priority)
 * Each is gated by its own Enabled/token/chat in the <telegram> config block.
 */

namespace TelegramBot {

inline std::string UrlEncode(const std::string& s)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += (char)c;
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

inline void Notify(const std::string& endpoint, const std::string& proxy,
                   const std::string& botToken, const std::string& chatID,
                   const std::string& text)
{
    if (endpoint.empty() || botToken.empty() || chatID.empty() || text.empty())
        return;
    std::string cmd = "curl -s -X POST '" + endpoint + "/bot" + botToken
                    + "/sendMessage' --data-urlencode 'chat_id=" + chatID
                    + "' --data-urlencode 'text=" + UrlEncode(text)
                    + "' >/dev/null 2>&1";
    if (!proxy.empty())
        cmd = "curl -s --proxy '" + proxy + "' -X POST '" + endpoint
            + "/bot" + botToken
            + "/sendMessage' --data-urlencode 'chat_id=" + chatID
            + "' --data-urlencode 'text=" + UrlEncode(text)
            + "' >/dev/null 2>&1";
    cmd += " &";   // fire & forget (detached background shell)
    ::system(cmd.c_str());
}

// Public events → the player group.
inline void NotifyPlayer(const std::string& text)
{
    auto& tg = EVEServerConfig::get().telegram;
    if (tg.PlayerEnabled)
        Notify(tg.Endpoint, tg.Proxy, tg.PlayerBotToken, tg.PlayerChatID, text);
}

// Security/priority alerts → the closed admin group.
inline void NotifyAdmin(const std::string& text)
{
    auto& tg = EVEServerConfig::get().telegram;
    if (tg.AdminEnabled)
        Notify(tg.Endpoint, tg.Proxy, tg.AdminBotToken, tg.AdminChatID, text);
}

} // namespace TelegramBot

#endif
