#include "eve-server.h"
#include "EVEServerConfig.h"
#include "apiserver/APIServer.h"
#include "apiserver/APIAccountManager.h"
#include "apiserver/APICharacterManager.h"
#include "apiserver/APICorporationManager.h"
#include "apiserver/APIServerManager.h"
#include "apiserver/APIAuthManager.h"
#include "apiserver/APIAdminManager.h"
#include "apiserver/APIServiceManager.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <thread>
#include <map>
#include <mutex>

using boost::asio::ip::tcp;

// ---- response cache for the API server -----------------------------------
// Heavy portal read endpoints (killboard lists, activity, market tops) run
// expensive aggregate queries over chrKillTable / mktOrders on every request.
// Cache the rendered XML in memory for a short TTL so the first browser load
// doesn't hang on a cold full-scan and repeat loads are instant.
namespace {

// Windows FILETIME units are 100ns ticks; EvE::Time::Second == 10000000L.
static const int64 kFiletimeSecond = 10000000LL;

struct ApiCacheEntry { int64 expireFiletime; std::string body; };

std::mutex g_apiCacheMutex;
std::map<std::string, ApiCacheEntry> g_apiCache;

// seconds of TTL per handler; 0 = never cache (mutating / live endpoints)
int g_apiCacheTtl(const std::string& service, const std::string& handler)
{
    std::string key = service + "/" + handler;
    static const std::map<std::string, int> ttl = {
        { "server/TopKills.xml.aspx",         30 },
        { "server/TopValuables.xml.aspx",     60 },
        { "server/Activity.xml.aspx",         30 },
        { "server/MarketTops.xml.aspx",       30 },
        { "server/MarketStats.xml.aspx",      30 },
        { "server/KillStats.xml.aspx",        30 },
        { "server/ActiveSystems.xml.aspx",    60 },
        { "server/CourierContracts.xml.aspx", 20 },
        { "server/MapData.xml.aspx",         300 },
        { "server/SovChanges.xml.aspx",       30 },
        { "char/AllKills.xml.aspx",           15 },
        { "char/KillMails.xml.aspx",          20 },
        { "char/KillMail.xml.aspx",          300 },
        { "char/KillDetail.xml.aspx",        120 },
        { "char/Resolve.xml.aspx",           300 },
        { "char/CharacterInfo.xml.aspx",     300 },
        { "corp/KillMails.xml.aspx",          20 },
        { "corp/CorporationSheet.xml.aspx",  300 },
    };
    auto it = ttl.find(key);
    return it != ttl.end() ? it->second : 0;
}

} // namespace

static std::string url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int val = 0;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> val) {
                result += static_cast<char>(val);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

APIServer::APIServer() = default;
APIServer::~APIServer() = default;

void APIServer::CreateServices()
{
    if (!_runOnce) {
        _serviceManagers["account"]  = std::make_unique<APIAccountManager>();
        _serviceManagers["char"]     = std::make_unique<APICharacterManager>();
        _serviceManagers["corp"]     = std::make_unique<APICorporationManager>();
        _serviceManagers["server"]   = std::make_unique<APIServerManager>();
        _serviceManagers["auth"]     = std::make_unique<APIAuthManager>();
        _serviceManagers["admin"]    = std::make_unique<APIAdminManager>();
    }
    _runOnce = true;
}

std::string APIServer::ProcessRequest(const std::string& service, const std::string& handler,
                                      const std::map<std::string, std::string>& params)
{
    // Only GET-style read endpoints are cached (whitelist above); POST / admin /
    // auth calls return TTL 0 and always execute live.
    int ttl = g_apiCacheTtl(service, handler);
    std::string cacheKey;
    if (ttl > 0) {
        // deterministic key: service/handler?k=v&k=v (params is a std::map, sorted)
        std::ostringstream ks;
        ks << service << "/" << handler << "?";
        for (auto& kv : params) ks << kv.first << "=" << kv.second << "&";
        cacheKey = ks.str();

        std::lock_guard<std::mutex> lk(g_apiCacheMutex);
        auto it = g_apiCache.find(cacheKey);
        if (it != g_apiCache.end() && it->second.expireFiletime > GetFileTimeNow())
            return it->second.body;
    }

    auto it = _serviceManagers.find(service);
    std::string body;
    if (it == _serviceManagers.end())
        body = "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><error>Unknown service: " + service + "</error></eveapi>";
    else
        body = it->second->ProcessCall(handler, params);

    if (ttl > 0 && body.find("<error>") == std::string::npos) {
        std::lock_guard<std::mutex> lk(g_apiCacheMutex);
        // bound memory: drop expired entries, and if still huge, clear the map
        if (g_apiCache.size() > 4096) {
            int64 now = GetFileTimeNow();
            for (auto it2 = g_apiCache.begin(); it2 != g_apiCache.end();) {
                if (it2->second.expireFiletime < now) it2 = g_apiCache.erase(it2);
                else ++it2;
            }
            if (g_apiCache.size() > 8192) g_apiCache.clear();
        }
        g_apiCache[cacheKey] = { static_cast<int64>(GetFileTimeNow() + double(ttl) * kFiletimeSecond), body };
    }
    return body;
}

/* ── minimal HTTP server ──────────────────────────────────────── */

static void HandleSession(tcp::socket socket, APIServer& srv)
{
    try {
        boost::asio::streambuf buf;
        boost::system::error_code ec;
        boost::asio::read_until(socket, buf, "\r\n\r\n", ec);
        if (ec) return;

        std::istream req(&buf);
        std::string method, uri, version;
        req >> method >> uri >> version;

        // parse path: /<service>/<handler>?key=val&key2=val2
        std::string service, handler;
        std::map<std::string, std::string> params;

        size_t qPos = uri.find('?');
        std::string path = (qPos != std::string::npos) ? uri.substr(0, qPos) : uri;
        if (qPos != std::string::npos) {
            std::string qs = uri.substr(qPos + 1);
            // replace &amp; with &
            boost::replace_all(qs, "&amp;", "&");
            std::istringstream ss(qs);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    std::string key = pair.substr(0, eq);
                    boost::to_lower(key);
                    params[key] = pair.substr(eq + 1);
                }
            }
        }

        // read POST body if present
        if (method == "POST") {
            // find Content-Length from what's left in the buffer
            std::string remaining(
                (std::istreambuf_iterator<char>(req)),
                 std::istreambuf_iterator<char>());

            size_t clPos = remaining.find("Content-Length:");
            if (clPos == std::string::npos)
                clPos = remaining.find("content-length:");
            if (clPos != std::string::npos) {
                size_t start = clPos + 15;
                size_t end = remaining.find("\r\n", start);
                std::string clStr = remaining.substr(start, end - start);
                size_t contentLength = std::stoul(clStr);

                // skip to body (after double CRLF)
                size_t bodyStart = remaining.find("\r\n\r\n");
                if (bodyStart != std::string::npos) {
                    bodyStart += 4;
                    std::string body = remaining.substr(bodyStart, contentLength);
                    boost::replace_all(body, "&amp;", "&");
                    boost::replace_all(body, "+", " ");
                    std::istringstream bs(body);
                    std::string pair;
                    while (std::getline(bs, pair, '&')) {
                        size_t eq = pair.find('=');
                        if (eq != std::string::npos) {
                            std::string key = pair.substr(0, eq);
                            boost::to_lower(key);
                            params[key] = url_decode(pair.substr(eq + 1));
                        }
                    }
                }
            }
        }

        // strip leading /
        if (!path.empty() && path[0] == '/') path = path.substr(1);
        size_t slash = path.find('/');
        if (slash != std::string::npos) {
            service = path.substr(0, slash);
            handler = path.substr(slash + 1);
        } else {
            handler = path;
        }

        std::string xml = srv.ProcessRequest(service, handler, params);

        std::ostringstream resp;
        resp << "HTTP/1.0 200 OK\r\n"
             << "Content-Type: text/xml\r\n"
             << "Content-Length: " << xml.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << xml;

        std::string out = resp.str();
        boost::asio::write(socket, boost::asio::buffer(out));
    } catch (std::exception& e) {
        sLog.Error("APIServer", "Connection error: %s", e.what());
    }
}

void APIServer::RunInternal()
{
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), sConfig.net.imageServerPort + 1));

        sLog.Green("API Server", "Listening on port %u", sConfig.net.imageServerPort);

        for (;;) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(HandleSession, std::move(socket), std::ref(*this)).detach();
        }
    } catch (std::exception& e) {
        sLog.Error("APIServer", "Fatal: %s", e.what());
    }
}

void APIServer::Run()
{
    _ioThread = std::thread([this]() { RunInternal(); });
}

void APIServer::Stop()
{
    // best-effort: the io_context is inside RunInternal's stack
}
