#ifndef EVEMU_EVESERVER_TELEGRAMBOT_H_
#define EVEMU_EVESERVER_TELEGRAMBOT_H_

#include <string>
#include <fstream>
#include <cstdlib>

#include "EVEServerConfig.h"

/*
 * Minimal Telegram notifier. Runs curl in a detached background shell so the
 * game loop is never blocked on the network call (same pattern as BotChat's
 * DeepSeek calls). Two audiences:
 *   - player group: public events (server news, downtime, top kills, ...)
 *   - admin group : closed channel (security / RMT / bot flags, priority)
 * Each is gated by its own Enabled/token/chat in the <telegram> config block.
 * Endpoint may point at a RU-reachable Bot API mirror; Proxy is optional.
 *
 * The message body is written to a temp file and sent with curl's
 * `--data-urlencode 'text@file'` so curl URL-encodes it exactly once (hand-
 * encoding it ourselves AND using --data-urlencode would double-encode, and the
 * chat would receive a literal percent-encoded string).
 */

namespace TelegramBot {

inline std::string HtmlEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            default:   out += c;        break;
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

    if (endpoint.empty() || botToken.empty() || chatID.empty() || text.empty())
        return;

    const std::string file = "/tmp/evemu_tg_msg.txt";
    {
        std::ofstream of(file.c_str(), std::ios::out | std::ios::trunc);
        if (!of)
            return;
        of << text;
    }

    std::string cmd = "curl -s --max-time 10";
    if (!proxy.empty())
        cmd += " --proxy '" + proxy + "'";
    cmd += " -X POST '" + endpoint + "/bot" + botToken + "/sendMessage'";
    cmd += " --data-urlencode 'chat_id=" + chatID + "'";
    cmd += " --data-urlencode 'text@" + file + "'";
    cmd += " >/dev/null 2>&1 &";   // fire & forget (detached background shell)
    ::system(cmd.c_str());
}

// Send to every chat in a comma-separated chat-id list (e.g. channel + group).
inline void NotifyToChats(const std::string& endpoint, const std::string& proxy,
                          const std::string& botToken, const std::string& chatList,
                          const std::string& text)
{
    if (chatList.empty()) return;
    size_t start = 0;
    while (start <= chatList.size()) {
        size_t comma = chatList.find(',', start);
        std::string chatID = chatList.substr(start,
            comma == std::string::npos ? std::string::npos : comma - start);
        // trim
        size_t b = chatID.find_first_not_of(" \t\r\n");
        size_t e = chatID.find_last_not_of(" \t\r\n");
        if (b != std::string::npos)
            chatID = chatID.substr(b, e - b + 1);
        Notify(endpoint, proxy, botToken, chatID, text);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
}

// Public events → the player announce channel (defaults to PlayerChatID so a
// group-only setup keeps working unchanged). Group duplicates come from the
// channel's own "linked channel" posts, not from the bot.
inline void NotifyPlayer(const std::string& text)
{
    auto& tg = EVEServerConfig::get().telegram;
    if (tg.PlayerEnabled)
        NotifyToChats(tg.Endpoint, tg.Proxy, tg.PlayerBotToken,
                      tg.PlayerAnnounceChatID.empty() ? tg.PlayerChatID : tg.PlayerAnnounceChatID, text);
}

// Security/priority alerts → the closed admin group(s).
inline void NotifyAdmin(const std::string& text)
{
    auto& tg = EVEServerConfig::get().telegram;
    if (tg.AdminEnabled)
        NotifyToChats(tg.Endpoint, tg.Proxy, tg.AdminBotToken, tg.AdminChatID, text);
}

} // namespace TelegramBot

#endif
