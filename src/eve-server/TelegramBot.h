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

    // Nicer formatting: HTML parse mode with the first line (the message title)
    // rendered bold. Everything is HTML-escaped first so special chars are safe.
    std::string title = text;
    std::string rest;
    size_t nl = text.find('\n');
    if (nl != std::string::npos) {
        title = text.substr(0, nl);
        rest  = text.substr(nl + 1);
    }
    std::string html = "<b>" + HtmlEscape(title) + "</b>";
    if (!rest.empty())
        html += "\n" + HtmlEscape(rest);

    const std::string file = "/tmp/evemu_tg_msg.txt";
    {
        std::ofstream of(file.c_str(), std::ios::out | std::ios::trunc);
        if (!of)
            return;
        of << html;
    }

    std::string cmd = "curl -s --max-time 10";
    if (!proxy.empty())
        cmd += " --proxy '" + proxy + "'";
    cmd += " -X POST '" + endpoint + "/bot" + botToken + "/sendMessage'";
    cmd += " --data-urlencode 'chat_id=" + chatID + "'";
    cmd += " --data-urlencode 'text@" + file + "'";
    cmd += " --data-urlencode 'parse_mode=HTML'";
    cmd += " >/dev/null 2>&1 &";   // fire & forget (detached background shell)
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
