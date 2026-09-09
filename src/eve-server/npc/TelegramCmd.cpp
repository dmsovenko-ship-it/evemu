#include "eve-server.h"
#include "npc/TelegramCmd.h"
#include "npc/BotMgr.h"
#include "EntityList.h"
#include "EVEServerConfig.h"
#include "TelegramBot.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <atomic>    
#include <thread>    
#include <vector>    
#include <map>       
#include <string>    
#include <memory>    

/*
 * @file TelegramCmd.cpp
 *
 * Inbound Telegram commands. A background thread long-polls getUpdates for the
 * configured player bot and admin bot, recognises a small command set and
 * answers with live DB statistics. Every reply goes back to the chat the
 * command arrived from, so if both groups share one bot the role is chosen by
 * chat id rather than by token.
 *
 * curl is invoked via fork/exec with the reply captured on a pipe (same
 * pattern as BotChat::RunCurl) so the poll never blocks the game loop.
 */

namespace TelegramCmd {

namespace {

// ---- minimal HTTP GET via curl, returns response body ----
bool RunCurlCapture(const std::vector<std::string>& args, std::string& out, unsigned timeoutSec)
{
    int pipefd[2];
    if (::pipe(pipefd) != 0)
        return false;

    pid_t pid = ::fork();
    if (pid == 0) {
        ::close(pipefd[0]);
        ::dup2(pipefd[1], STDOUT_FILENO);
        ::close(pipefd[1]);
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& a : args)
            argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        ::execvp("curl", argv.data());
        _exit(127);
    }
    if (pid < 0) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        return false;
    }

    ::close(pipefd[1]);
    out.clear();
    char buf[4096];
    ssize_t n;
    while ((n = ::read(pipefd[0], buf, sizeof(buf))) > 0)
        out.append(buf, (size_t)n);
    ::close(pipefd[0]);

    int status = 0;
    time_t start = time(nullptr);
    while (::waitpid(pid, &status, WNOHANG) == 0) {
        if (difftime(time(nullptr), start) > (double)timeoutSec) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            return false;
        }
        usleep(100000);
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::string HttpGet(const std::string& url, const std::string& proxy, unsigned timeoutSec)
{
    std::vector<std::string> args = { "curl", "-s", "--max-time", std::to_string(timeoutSec), url };
    if (!proxy.empty()) {
        // curl expects --proxy <value>
        args.insert(args.end(), { "--proxy", proxy });
    }
    std::string out;
    RunCurlCapture(args, out, timeoutSec + 5);
    return out;
}

// POST text as the value of the `text` field (--data-urlencode 'text@file',
// the same trick as TelegramBot::Notify) so user text is never shell-quoted.
void SendMessage(const std::string& endpoint, const std::string& proxy,
                 const std::string& token, const std::string& chatID,
                 const std::string& text)
{
    if (endpoint.empty() || token.empty() || chatID.empty() || text.empty())
        return;
    const std::string file = "/tmp/evemu_tg_reply.txt";
    {
        std::ofstream of(file.c_str(), std::ios::out | std::ios::trunc);
        if (!of)
            return;
        of << text;
    }
    std::vector<std::string> args = {
        "curl", "-s", "--max-time", "15",
        "-X", "POST", endpoint + "/bot" + token + "/sendMessage",
        "--data-urlencode", "chat_id=" + chatID,
        "--data-urlencode", "text@" + file,
    };
    if (!proxy.empty())
        args.insert(args.end(), { "--proxy", proxy });
    std::string out;
    RunCurlCapture(args, out, 15);
}

// Generic Bot API POST (argv-style, no shell). `params` are literal
// `key=value` pieces passed via curl --data-urlencode.
void BotPost(const std::string& endpoint, const std::string& proxy,
             const std::string& token, const std::string& method,
             const std::vector<std::string>& params)
{
    if (endpoint.empty() || token.empty())
        return;
    std::vector<std::string> args = {
        "curl", "-s", "--max-time", "15",
        "-X", "POST", endpoint + "/bot" + token + "/" + method,
    };
    for (const auto& kv : params) {
        args.push_back("--data-urlencode");
        args.push_back(kv);
    }
    if (!proxy.empty())
        args.insert(args.end(), { "--proxy", proxy });
    std::string out;
    RunCurlCapture(args, out, 15);
}

void TelegramDeleteMessage(const std::string& ep, const std::string& px,
                           const std::string& tok, const std::string& chat,
                           const std::string& msgID)
{
    BotPost(ep, px, tok, "deleteMessage",
            { "chat_id=" + chat, "message_id=" + msgID });
}

void TelegramRestrict(const std::string& ep, const std::string& px,
                      const std::string& tok, const std::string& chat,
                      const std::string& user, bool allow)
{
    std::string perms = allow
        ? "{\"can_send_messages\":true,\"can_send_media_messages\":true,"
          "\"can_send_other_messages\":true,\"can_add_web_page_previews\":true}"
        : "{\"can_send_messages\":false}";
    BotPost(ep, px, tok, "restrictChatMember",
            { "chat_id=" + chat, "user_id=" + user, "permissions=" + perms });
}

void TelegramBan(const std::string& ep, const std::string& px,
                 const std::string& tok, const std::string& chat,
                 const std::string& user)
{
    BotPost(ep, px, tok, "banChatMember",
            { "chat_id=" + chat, "user_id=" + user });
}

// message contains a link/url or forbidden/ad word → remove
bool IsForbiddenContent(const std::string& lower)
{
    static const std::string links[] = { "http://", "https://", "t.me/",
        "telegram.me", "www.", ".ru/", ".com/", ".xyz/", ".top/", "invite" };
    for (auto& l : links)
        if (lower.find(l) != std::string::npos)
            return true;
    static const std::string words[] = {
        "казино", "casino", "порно", "xxx", "крипт", "бот фарм", "продам isk",
        "куплю isk", "продам иск", "rmt", "реклам", "работа в интернете",
        "заработок", "разведу", "знакомство досуг",
        "хуй", "хуё", "хуя", "нахуй", "похуй", "бля", "бляд", "пизд", "пидор",
        "пидр", "ебал", "ебат", "ебаш", "заеб", "наеб", "разъеб", "сука", "сук",
        "гандон", "чмо", "мудак", "шлюх", "проститутк",
    };
    for (auto& w : words)
        if (lower.find(w) != std::string::npos)
            return true;
    return false;
}

// tiny helpers shared by moderation
std::string JsonNumStr(const std::string& s, size_t from, const std::string& key)
{
    std::string pat = "\"" + key + "\":";
    size_t p = s.find(pat, from);
    if (p == std::string::npos)
        return "";
    p += pat.size();
    size_t e = p;
    while (e < s.size() && (isdigit((unsigned char)s[e]) || s[e] == '-'))
        ++e;
    return s.substr(p, e - p);
}

// ---- tiny JSON field extractors (Telegram getUpdates shape is stable) ----
// Each returns the raw (unescaped) string value for "key": or "" if absent.
std::string JsonStrField(const std::string& s, size_t from, const std::string& key)
{
    std::string pat = "\"" + key + "\":\"";
    size_t p = s.find(pat, from);
    if (p == std::string::npos)
        return "";
    p += pat.size();
    std::string v;
    for (; p < s.size(); ++p) {
        char c = s[p];
        if (c == '\\') {
            if (p + 1 < s.size()) {
                char nx = s[p + 1];
                if (nx == 'n') { v += '\n'; ++p; }
                else if (nx == 't') { v += '\t'; ++p; }
                else if (nx == 'r') { v += '\r'; ++p; }
                else if (nx == '"') { v += '"'; ++p; }
                else if (nx == '\\') { v += '\\'; ++p; }
                else if (nx == 'u') {
                    auto hexv = [](char h) -> int {
                        if (h >= '0' && h <= '9') return h - '0';
                        int l = h | 32;
                        if (l >= 'a' && l <= 'f') return l - 'a' + 10;
                        return 0;
                    };
                    unsigned cp = 0;
                    if (p + 5 < s.size())
                        for (int k = 1; k <= 4; ++k)
                            cp = (cp << 4) | (unsigned)hexv(s[p + 1 + k]);
                    if (cp >= 0xD800 && cp <= 0xDBFF && p + 11 < s.size()
                        && s[p + 6] == '\\' && s[p + 7] == 'u') {
                        unsigned lo = 0;
                        for (int k = 0; k < 4; ++k)
                            lo = (lo << 4) | (unsigned)hexv(s[p + 8 + k]);
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        p += 11;   // skip low-surrogate escape
                    } else {
                        p += 4;
                    }
                    // UTF-8 encode cp
                    if (cp < 0x80) v += (char)cp;
                    else if (cp < 0x800) {
                        v += (char)(0xC0 | (cp >> 6));
                        v += (char)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        v += (char)(0xE0 | (cp >> 12));
                        v += (char)(0x80 | ((cp >> 6) & 0x3F));
                        v += (char)(0x80 | (cp & 0x3F));
                    } else {
                        v += (char)(0xF0 | (cp >> 18));
                        v += (char)(0x80 | ((cp >> 12) & 0x3F));
                        v += (char)(0x80 | ((cp >> 6) & 0x3F));
                        v += (char)(0x80 | (cp & 0x3F));
                    }
                }
                else { v += nx; ++p; }
            }
        } else if (c == '"') {
            break;
        } else {
            v += c;
        }
    }
    return v;
}

// Extracts the chat id of a message object (chat may be absent in some
// update types). Returns "" when not found.
std::string JsonChatID(const std::string& block)
{
    // ..."chat":{"id":-1001234567890,"type":"supergroup"...}
    std::string pat = "\"chat\":{\"id\":";
    size_t p = block.find(pat);
    if (p == std::string::npos)
        return "";
    p += pat.size();
    size_t e = p;
    while (e < block.size() && (isdigit((unsigned char)block[e]) || block[e] == '-'))
        ++e;
    return block.substr(p, e - p);
}

struct Update {
    int64 updateID = 0;
    std::string chatID;
    std::string text;
    std::string messageID;
    std::string fromID;
    bool fromIsBot = false;
    std::string joinUserID;   // non-empty when someone joined (new_chat_members)
    bool joinIsBot = false;
};

// Extract the numeric id of the message sender, if any.
std::string JsonFromID(const std::string& block)
{
    std::string pat = "\"from\":{\"id\":";
    size_t p = block.find(pat);
    if (p == std::string::npos) return "";
    p += pat.size();
    size_t e = p;
    while (e < block.size() && (isdigit((unsigned char)block[e]) || block[e] == '-'))
        ++e;
    return block.substr(p, e - p);
}

// Splits "result":[...] into update objects and pulls update_id/chat/text/etc.
std::vector<Update> ParseUpdates(const std::string& json)
{
    std::vector<Update> out;
    size_t pos = 0;
    for (;;) {
        size_t u = json.find("\"update_id\":", pos);
        if (u == std::string::npos)
            break;
        // start of this object; find its matching end (next update_id or end)
        size_t objStart = json.rfind('{', u);
        size_t next = json.find("\"update_id\":", u + 13);
        size_t objEnd = next == std::string::npos ? json.size() : json.rfind('}', next);
        std::string obj = json.substr(objStart, objEnd - objStart);

        Update upd;
        upd.updateID = 0;
        {
            // read number after "update_id":
            size_t n = u + 12;
            while (n < json.size() && isdigit((unsigned char)json[n])) {
                upd.updateID = upd.updateID * 10 + (json[n] - '0');
                ++n;
            }
        }
        // message → chat.id, text, from, joins; only message objects
        size_t m = obj.find("\"message\":");
        if (m != std::string::npos) {
            std::string msg = obj.substr(m);
            upd.chatID    = JsonChatID(msg);
            upd.text      = JsonStrField(msg, 0, "text");
            upd.messageID = JsonNumStr(msg, 0, "message_id");
            upd.fromID    = JsonFromID(msg);
            size_t ncm = msg.find("\"new_chat_members\":[");
            if (ncm != std::string::npos) {
                size_t close = msg.find(']', ncm);
                std::string member = close == std::string::npos
                                   ? msg.substr(ncm) : msg.substr(ncm, close - ncm);
                upd.joinUserID = JsonNumStr(member, 0, "id");
                upd.joinIsBot  = member.find("\"is_bot\":true") != std::string::npos;
            }
            if (!upd.chatID.empty()
                && (!upd.text.empty() || !upd.joinUserID.empty() || !upd.messageID.empty()))
                out.push_back(upd);
        }
        pos = u + 12;
    }
    return out;
}

} // namespace

// ---- command handlers ---------------------------------------

namespace {

// Format a "name (ship type)" pilot line for /last and /topkills.
std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Is `chat` one of the ids in the comma-separated config value?
bool InChatList(const std::string& csv, const std::string& chat)
{
    if (csv.empty()) return false;
    size_t start = 0;
    while (start <= csv.size()) {
        size_t comma = csv.find(',', start);
        std::string item = csv.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        if (Trim(item) == chat)
            return true;
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return false;
}

std::string PlayerHelp()
{
    return "Доступные команды:\n"
           "/online — онлайн сервера\n"
           "/topkills — топ киллов за 24ч\n"
           "/market — топ рынка за 24ч\n"
           "/who <имя> — инфо о пилоте\n"
           "/last — последний килл\n"
           "/help — этот список";
}

std::string AdminHelp()
{
    return "Доступные команды (админ):\n"
           "/status — состояние сервера\n"
           "/flags — security-флаги (RMT/мультибокс)\n"
           "/petitions — открытые петиции\n"
           "/accounts — статистика аккаунтов\n"
           "/bans <ip> — кто логинился с IP\n"
           "/help — этот список";
}

std::string CmdOnline()
{
    DBQueryResult res;
    std::string out;
    // Everyone counts as a player — no separate "simulated pilots" figure.
    uint32 total = sEntityList.GetClientCount()
                 + sBotMgr.CountActiveBots()
                 + sBotMgr.GetDockedBotCount();
    out += "👥 Онлайн игроков: " + std::to_string(total) + "\n";
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM account"))
    { DBResultRow r; if (res.GetRow(r)) out += "Аккаунтов всего: " + std::to_string(r.GetUInt(0)) + "\n"; }
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM chrKillTable"))
    { DBResultRow r; if (res.GetRow(r)) out += "Киллов всего: " + std::to_string(r.GetUInt(0)) + "\n"; }
    return out;
}

std::string CmdTopKills()
{
    std::string d = BuildKillDigestText(5,
        "(k.killTime - 116444736000000000) / 10000000 > UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL 1 DAY))");
    return d.empty() ? "За сутки киллов нет." : d;
}

std::string CmdLastKill()
{
    std::string d = BuildKillDigestText(1, "1");
    return d.empty() ? "Киллов пока нет." : d;
}

std::string CmdMarket()
{
    // biggest trades (by ISK) and busiest traders in the last 24h
    std::string out;
    DBQueryResult res;
    std::string since = "t.transactionDate >= NOW() - INTERVAL 1 DAY";
    {
        std::string q1 =
            "SELECT MAX(iv.typeName), SUM(t.quantity*t.price), COUNT(*)"
            " FROM mktTransactions t JOIN invTypes iv ON iv.typeID = t.typeID"
            " WHERE " + since + " GROUP BY t.typeID ORDER BY 2 DESC LIMIT 5";
        if (sDatabase.RunQuery(res, q1.c_str())) {
            DBResultRow r;
            out += "🏪 Крупнейшие сделки за 24ч:\n";
            while (res.GetRow(r)) {
                const char* nm = r.GetText(0) ? r.GetText(0) : "?";
                out += "• " + std::string(nm) + " — " + HumanizeIsk(r.GetDouble(1))
                     + " (" + std::to_string(r.GetUInt(2)) + " сделок)\n";
            }
        }
    }
    {
        std::string q2 =
            "SELECT COALESCE(cc.characterName, cr.corporationName, '?'), SUM(t.quantity*t.price)"
            " FROM mktTransactions t"
            " LEFT JOIN chrCharacters cc ON cc.characterID = t.characterID"
            " LEFT JOIN crpCorporation cr ON cr.corporationID = t.characterID"
            " WHERE " + since + " GROUP BY t.characterID ORDER BY 2 DESC LIMIT 3";
        if (sDatabase.RunQuery(res, q2.c_str())) {
            DBResultRow r;
            out += "Активные трейдеры:\n";
            while (res.GetRow(r)) {
                out += "• " + std::string(r.GetText(0) ? r.GetText(0) : "?")
                     + " — " + HumanizeIsk(r.GetDouble(1)) + "\n";
            }
        }
    }
    return out.empty() ? "За 24ч сделок нет." : out;
}

std::string CmdWho(const std::string& arg)
{
    std::string name = Trim(arg);
    if (name.empty())
        return "Укажи имя: /who <пилот>";
    std::string esc;
    sDatabase.DoEscapeString(esc, name);
    // exact or prefix; prefer exact. Show up to one result.
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT c.characterID, c.characterName, c.accountID,"
        "       cr.corporationName, al.allianceName,"
        "       c.securityRating, c.title"
        " FROM chrCharacters c"
        " LEFT JOIN crpCorporation cr ON cr.corporationID = c.corporationID"
        " LEFT JOIN alnAlliance al ON al.allianceID = cr.allianceID"
        " WHERE c.characterName LIKE '%s' OR c.characterName LIKE '%s%%'"
        " ORDER BY (c.characterName = '%s') DESC, c.characterName LIMIT 5",
        esc.c_str(), esc.c_str(), esc.c_str()))   // esc is escaped (safe inside quotes)
        return "ошибка запроса";
    DBResultRow row;
    if (!res.GetRow(row))
        return "Пилот не найден.";
    std::string out = "👤 " + std::string(row.GetText(1) ? row.GetText(1) : "?");
    if (row.GetText(3)) out += "\nКорпорация: " + std::string(row.GetText(3));
    if (row.GetText(4)) out += " [" + std::string(row.GetText(4)) + "]";
    const char* sec = row.GetText(5);
    char secb[32];
    if (sec) { snprintf(secb, sizeof(secb), "%.2f", atof(sec)); out += "\nСек: " + std::string(secb); }
    if (row.GetText(6)) out += "\nТитул: " + std::string(row.GetText(6));
    // online status & ship from their active client/ship
    uint32 charID = row.GetUInt(0);
    DBQueryResult ores;
    if (sDatabase.RunQuery(ores,
        "SELECT cc.solarSystemID, ss.solarSystemName, it.typeName"
        " FROM chrCharacters cc"
        " LEFT JOIN entity e ON e.itemID = cc.shipID"
        " LEFT JOIN invTypes it ON it.typeID = e.typeID"
        " LEFT JOIN mapSolarSystems ss ON ss.solarSystemID = cc.solarSystemID"
        " WHERE cc.characterID = %u AND cc.online = 1", charID)) {
        DBResultRow orow;
        if (ores.GetRow(orow) && orow.GetText(1)) {
            out += "\n📍 " + std::string(orow.GetText(1));
            if (orow.GetText(2)) out += " на " + std::string(orow.GetText(2));
        }
    }
    return out;
}

std::string CmdStatus()
{
    DBQueryResult res;
    std::string out;
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM account")) {
        DBResultRow r; if (res.GetRow(r)) out += "Аккаунтов: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM account WHERE online = 1")) {
        DBResultRow r; if (res.GetRow(r)) out += "Онлайн: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM account WHERE banned = 1")) {
        DBResultRow r; if (res.GetRow(r)) out += "Забанено: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM chrKillTable")) {
        DBResultRow r; if (res.GetRow(r)) out += "Киллы: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM mktTransactions")) {
        DBResultRow r; if (res.GetRow(r)) out += "Сделок на рынке: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res, "SELECT COUNT(*) FROM portal_petitions WHERE status = 1 AND deleted = 0")) {
        DBResultRow r; if (res.GetRow(r)) out += "Открытых петиций: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    return out.empty() ? "нет данных" : "🖥 Статус сервера:\n" + out;
}

std::string CmdFlags()
{
    DBQueryResult res;
    std::string out;
    int64 since = GetFileTimeNow() - 24LL * 3600 * 10000000;
    if (sDatabase.RunQuery(res,
        "SELECT sc.characterName, bc.characterName, COUNT(*), SUM(t.price*t.quantity)"
        " FROM mktTransactions t"
        " JOIN chrCharacters sc ON sc.characterID = t.clientID"
        " JOIN chrCharacters bc ON bc.characterID = t.characterID"
        " WHERE t.transactionType = 0 AND t.transactionDate >= %lli"
        "   AND t.clientID <> t.characterID"
        " GROUP BY t.clientID, t.characterID"
        " ORDER BY 4 DESC LIMIT 5", (long long)since)) {
        DBResultRow r;
        out += "💸 Крупные переводы (24ч):\n";
        while (res.GetRow(r)) {
            out += "• " + std::string(r.GetText(0) ? r.GetText(0) : "?") + " → "
                 + std::string(r.GetText(1) ? r.GetText(1) : "?") + " · "
                 + HumanizeIsk(r.GetDouble(3)) + "\n";
        }
    }
    if (sDatabase.RunQuery(res,
        "SELECT h.ip, COUNT(DISTINCT h.accountID), GROUP_CONCAT(DISTINCT a.accountName SEPARATOR ', ')"
        " FROM accountLoginHistory h"
        " JOIN account a ON a.accountID = h.accountID"
        " WHERE h.loginTime >= NOW() - INTERVAL 14 DAY"
        " GROUP BY h.ip HAVING COUNT(DISTINCT h.accountID) >= 2"
        " ORDER BY 2 DESC LIMIT 5")) {
        DBResultRow r;
        out += "🖥 Один IP на несколько аккаунтов:\n";
        while (res.GetRow(r)) {
            out += "• " + std::string(r.GetText(0) ? r.GetText(0) : "?") + " — "
                 + std::to_string(r.GetUInt(1)) + " акк.: "
                 + std::string(r.GetText(2) ? r.GetText(2) : "") + "\n";
        }
    }
    if (out.empty())
        out = "Флагов за период нет.";
    return out;
}

std::string CmdPetitions()
{
    DBQueryResult res;
    std::string out;
    if (sDatabase.RunQuery(res,
        "SELECT p.petitionID, p.authorName, p.subject, c.categoryName, p.createDate"
        " FROM portal_petitions p"
        " LEFT JOIN portal_petition_categories c ON c.categoryID = p.categoryID AND c.languageID = 'ru'"
        " WHERE p.status = 1 AND p.deleted = 0"
        " ORDER BY p.petitionID DESC LIMIT 8")) {
        DBResultRow r;
        out += "📮 Открытые петиции:\n";
        bool any = false;
        while (res.GetRow(r)) {
            any = true;
            out += "• #" + std::to_string(r.GetUInt(0)) + " "
                 + std::string(r.GetText(1) ? r.GetText(1) : "?") + " — "
                 + std::string(r.GetText(2) ? r.GetText(2) : "");
            if (r.GetText(3)) out += " [" + std::string(r.GetText(3)) + "]";
            out += "\n";
        }
        if (!any) out += "нет открытых\n";
    }
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM portal_petitions WHERE status = 1 AND deleted = 0 AND categoryID IN (601,602)")) {
        DBResultRow r; if (res.GetRow(r)) out += "⚠️ Бот/RMT: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    return out.empty() ? "нет данных" : out;
}

std::string CmdAccounts()
{
    DBQueryResult res;
    std::string out;
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*), SUM(online), SUM(banned),"
        "       SUM(lastLogin >= NOW() - INTERVAL 1 DAY)"
        " FROM account")) {
        DBResultRow r;
        if (res.GetRow(r))
            out += "Аккаунты: всего " + std::to_string(r.GetUInt(0))
                 + ", онлайн " + std::to_string(r.GetUInt(1))
                 + ", бан " + std::to_string(r.GetUInt(2))
                 + ", заходили за сутки " + std::to_string(r.GetUInt(3)) + "\n";
    }
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM chrCharacters WHERE characterID >= 90000000 AND characterID < 98000000")) {
        DBResultRow r; if (res.GetRow(r)) out += "Челоботов: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    if (sDatabase.RunQuery(res,
        "SELECT COUNT(*) FROM chrCharacters WHERE characterID < 90000000")) {
        DBResultRow r; if (res.GetRow(r)) out += "Реальных персонажей: " + std::to_string(r.GetUInt(0)) + "\n";
    }
    return out.empty() ? "нет данных" : out;
}

std::string CmdBans(const std::string& arg)
{
    std::string ip = Trim(arg);
    if (ip.empty())
        return "Укажи IP: /bans <ip>";
    std::string esc;
    sDatabase.DoEscapeString(esc, ip);
    DBQueryResult res;
    std::string out = "🖥 Логины с " + ip + ":\n";
    if (!sDatabase.RunQuery(res,
        "SELECT h.accountID, a.accountName, MAX(h.loginTime), a.banned"
        " FROM accountLoginHistory h"
        " JOIN account a ON a.accountID = h.accountID"
        " WHERE h.ip = '%s'"
        " GROUP BY h.accountID, a.accountName, a.banned"
        " ORDER BY 3 DESC LIMIT 10", esc.c_str()))
        return "ошибка запроса";
    DBResultRow r;
    bool any = false;
    while (res.GetRow(r)) {
        any = true;
        out += "• " + std::string(r.GetText(1) ? r.GetText(1) : "?") + " (id "
             + std::to_string(r.GetUInt(0)) + "), последний "
             + std::string(r.GetText(2) ? r.GetText(2) : "?");
        if (r.GetUInt(3) == 1) out += " — забанен";
        out += "\n";
    }
    if (!any)
        out += "логинов не найдено.\n";
    return out;
}

} // namespace

namespace {

// Route one command to its handler.
std::string RunCommand(const std::string& cmdLower, const std::string& arg, bool isAdmin)
{
    if (cmdLower == "/help")
        return isAdmin ? AdminHelp() : PlayerHelp();
    if (!isAdmin) {
        if (cmdLower == "/online")   return CmdOnline();
        if (cmdLower == "/topkills") return CmdTopKills();
        if (cmdLower == "/market")   return CmdMarket();
        if (cmdLower == "/who")      return CmdWho(arg);
        if (cmdLower == "/last")     return CmdLastKill();
    } else {
        if (cmdLower == "/status")   return CmdStatus();
        if (cmdLower == "/flags")    return CmdFlags();
        if (cmdLower == "/petitions")return CmdPetitions();
        if (cmdLower == "/accounts") return CmdAccounts();
        if (cmdLower == "/bans")     return CmdBans(arg);
    }
    return "";
}

// --- moderator state (in-memory, resets on server restart) ---
// pending anti-spam verification: user id -> group chat id
static std::map<std::string, std::string> g_pendingVerify;
// spam strikes per chat:user
static std::map<std::string, int> g_spamStrikes;

// temporary debug log for moderator behaviour
static void ModLog(const std::string& line)
{
    std::ofstream f("/tmp/evemu_mod.log", std::ios::app);
    if (f) f << line << "\n";
}

// poll once per bot token; keep the last update id per token so a shared bot
// that serves both groups doesn't re-deliver
void PollOnce(const std::string& endpoint, const std::string& proxy,
              const std::string& token,
              const std::string& playerChat, const std::string& adminChat,
              int64& offset)
{
    std::string url = endpoint + "/bot" + token + "/getUpdates?offset=";
    url += offset > 0 ? std::to_string(offset) : "-1";
    // long-poll a bit so we don't busy-spin
    url += "&timeout=20";

    std::string body = HttpGet(url, proxy, 25);
    if (body.empty() || body.find("\"ok\":true") == std::string::npos)
        return;

    std::vector<Update> ups = ParseUpdates(body);
    for (const auto& u : ups) {
        // Always acknowledge the update (advance offset) so Telegram doesn't
        // re-deliver it forever, even when it is not for our groups.
        if (u.updateID >= offset)
            offset = u.updateID + 1;

        // role by chat list (ids may be comma-separated)
        bool isAdmin  = !adminChat.empty() && InChatList(adminChat, u.chatID);
        bool inPlayer = !playerChat.empty() && InChatList(playerChat, u.chatID);
        bool isPrivate = !u.chatID.empty() && u.chatID[0] != '-'
                      && u.chatID == u.fromID;

        // private chat with the bot → anti-spam verification
        if (isPrivate && !u.text.empty()) {
            std::string t = Trim(u.text);
            for (auto& c : t) c = (char)tolower((unsigned char)c);
            bool verify = t.find("/verify") != std::string::npos;
            if (verify) {
                auto it = g_pendingVerify.find(u.fromID);
                if (it != g_pendingVerify.end()) {
                    TelegramRestrict(endpoint, proxy, token, it->second, u.fromID, true);
                    SendMessage(endpoint, proxy, token, it->second,
                                "✅ Проверка пройдена, добро пожаловать!");
                    SendMessage(endpoint, proxy, token, u.chatID,
                                "✅ Вы разблокированы в чате.");
                    g_pendingVerify.erase(it);
                } else {
                    SendMessage(endpoint, proxy, token, u.chatID,
                                "Ожидающей проверки нет — вы уже в чате.");
                }
            }
            continue;
        }

        if (!inPlayer && !isAdmin)
            continue;   // some other group that added the bot → ignore

        ModLog("update chat=" + u.chatID + " player=" + std::to_string(inPlayer)
             + " admin=" + std::to_string(isAdmin) + " text='" + u.text + "'");

        // --- moderation (player chats only) ---
        if (inPlayer) {
            if (!u.joinUserID.empty()) {
                if (u.joinIsBot) {
                    TelegramBan(endpoint, proxy, token, u.chatID, u.joinUserID);
                    continue;
                }
                g_pendingVerify[u.joinUserID] = u.chatID;
                TelegramRestrict(endpoint, proxy, token, u.chatID, u.joinUserID, false);
                SendMessage(endpoint, proxy, token, u.chatID,
                            "👋 Добро пожаловать! Для защиты от спама вы временно в муте — напишите боту в личку /verify, чтобы разблокироваться.");
                continue;
            }
            if (!u.text.empty() && u.text[0] != '/') {
                std::string lower = u.text;
                for (auto& c : lower) c = (char)tolower((unsigned char)c);
                if (IsForbiddenContent(lower)) {
                    ModLog("FORBIDDEN chat=" + u.chatID + " from=" + u.fromID
                         + " msg=" + u.messageID + " text='" + u.text + "'");
                    if (!u.messageID.empty())
                        TelegramDeleteMessage(endpoint, proxy, token, u.chatID, u.messageID);
                    if (!u.fromID.empty()) {
                        std::string key = u.chatID + ":" + u.fromID;
                        int st = ++g_spamStrikes[key];
                        if (st >= 2) {
                            TelegramRestrict(endpoint, proxy, token, u.chatID, u.fromID, false);
                            SendMessage(endpoint, proxy, token, u.chatID,
                                        "⛔ Спам/реклама — участник замучен.");
                        } else {
                            SendMessage(endpoint, proxy, token, u.chatID,
                                        "🚫 В чате запрещены ссылки, реклама и запрещённые слова.");
                        }
                    }
                    continue;
                }
            }
        }

        std::string text = Trim(u.text);
        if (text.empty() || text[0] != '/')
            continue;
        // split "/cmd@botname arg..." → cmdLower + arg (strip the @suffix)
        std::string cmd = text;
        std::string arg;
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos) {
            arg = cmd.substr(sp + 1);
            cmd = cmd.substr(0, sp);
        }
        size_t at = cmd.find('@');
        if (at != std::string::npos)
            cmd = cmd.substr(0, at);
        for (auto& c : cmd) c = (char)tolower((unsigned char)c);

        std::string reply = RunCommand(cmd, arg, isAdmin);
        if (!reply.empty())
            SendMessage(endpoint, proxy, token, u.chatID, reply);
    }
}

// background thread: poll player + admin bots (dedup token, per-token offset)
void PollLoop(std::atomic<bool>& run, std::string endpoint, std::string proxy,
              bool playerOn, std::string playerToken, std::string playerChat,
              bool adminOn, std::string adminToken, std::string adminChat)
{
    // one poller per distinct token; chat ids ride along so a shared bot can
    // serve both the player and the admin group from a single stream.
    struct Bot { std::string token, playerChat, adminChat; int64 offset = 0; };
    std::vector<Bot> bots;
    auto add = [&](const std::string& t, const std::string& p, const std::string& a) {
        if (t.empty())
            return;
        for (auto& b : bots) {
            if (b.token == t) {   // same bot in both groups → merge chat ids
                if (!p.empty()) b.playerChat = p;
                if (!a.empty()) b.adminChat = a;
                return;
            }
        }
        bots.push_back({ t, p, a, 0 });
    };
    if (playerOn) add(playerToken, playerChat, "");
    if (adminOn)  add(adminToken, "", adminChat);

    while (run.load()) {
        for (auto& b : bots)
            PollOnce(endpoint, proxy, b.token, b.playerChat, b.adminChat, b.offset);
        // small delay between cycles (long-poll already holds ~20s)
        for (int i = 0; i < 10 && run.load(); ++i)
            usleep(100000);
    }
}

std::atomic<bool> g_run{ false };
std::thread* g_thread = nullptr;

} // namespace (anonymous)

void Start()
{
    if (g_run.load())
        return;
    auto& tg = EVEServerConfig::get().telegram;
    if (!tg.PlayerEnabled && !tg.AdminEnabled)
        return;
    if (tg.Endpoint.empty() || (tg.PlayerBotToken.empty() && tg.AdminBotToken.empty()))
        return;

    g_run.store(true);
    g_thread = new std::thread(PollLoop, std::ref(g_run),
        tg.Endpoint, tg.Proxy,
        tg.PlayerEnabled, tg.PlayerBotToken, tg.PlayerChatID,
        tg.AdminEnabled,  tg.AdminBotToken, tg.AdminChatID);
}

void Stop()
{
    g_run.store(false);
    if (g_thread && g_thread->joinable())
        g_thread->join();
    delete g_thread;
    g_thread = nullptr;
}

} // namespace TelegramCmd
