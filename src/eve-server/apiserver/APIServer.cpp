#include "eve-server.h"
#include "EVEServerConfig.h"
#include "apiserver/APIServer.h"
#include "apiserver/APIAccountManager.h"
#include "apiserver/APICharacterManager.h"
#include "apiserver/APICorporationManager.h"
#include "apiserver/APIServerManager.h"
#include "apiserver/APIServiceManager.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <thread>

using boost::asio::ip::tcp;

APIServer::APIServer() = default;
APIServer::~APIServer() = default;

void APIServer::CreateServices()
{
    if (!_runOnce) {
        _serviceManagers["account"]  = std::make_unique<APIAccountManager>();
        _serviceManagers["char"]     = std::make_unique<APICharacterManager>();
        _serviceManagers["corp"]     = std::make_unique<APICorporationManager>();
        _serviceManagers["server"]   = std::make_unique<APIServerManager>();
    }
    _runOnce = true;
}

std::string APIServer::ProcessRequest(const std::string& service, const std::string& handler,
                                      const std::map<std::string, std::string>& params)
{
    auto it = _serviceManagers.find(service);
    if (it == _serviceManagers.end())
        return "<?xml version='1.0' encoding='UTF-8'?>\n<eveapi version=\"2\"><error>Unknown service: " + service + "</error></eveapi>";

    return it->second->ProcessCall(handler, params);
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
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), sConfig.net.imageServerPort));

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
