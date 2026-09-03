#ifndef __APISERVER__H__INCL__
#define __APISERVER__H__INCL__

#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>

class APIServiceManager;

class APIServer
{
public:
    static APIServer& get() {
        static APIServer instance;
        return instance;
    }

    void CreateServices();
    void Run();
    void Stop();
    std::string& url() { return _url; }

    std::string ProcessRequest(const std::string& service, const std::string& handler,
                               const std::map<std::string, std::string>& params);

    APIServer(const APIServer&) = delete;
    APIServer& operator=(const APIServer&) = delete;
    ~APIServer();

private:
    APIServer();
    void RunInternal();

    std::thread _ioThread;
    std::string _url;
    bool _runOnce = false;
    std::map<std::string, std::unique_ptr<APIServiceManager>> _serviceManagers;
};

#define sAPIServer ( APIServer::get() )

#endif // __APISERVER__H__INCL__
