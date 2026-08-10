#include "eve-server.h"
#include "npc/BotChat.h"
#include "EVEServerConfig.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include <vector>

/*
 * @file BotChat.cpp
 *
 * DeepSeek chat-completions client for simulated players. Uses the system `curl`
 * binary (present in the server image) so we don't need to link an HTTP/TLS
 * library. Calls are made via fork/exec so a hung API can't block the game loop;
 * a timeout kills the child.
 */

static std::string ReadAllFromFd(int fd)
{
    std::string out;
    char buf[4096];
    ssize_t n;
    while ((n = ::read(fd, buf, sizeof(buf))) > 0)
        out.append(buf, (size_t)n);
    return out;
}

static bool RunCurl(const std::vector<std::string>& args, std::string& out, unsigned timeoutSec = 20)
{
    int pipefd[2];
    if (::pipe(pipefd) != 0)
        return false;

    pid_t pid = ::fork();
    if (pid == 0) {
        // child: exec curl, stdout -> pipe
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

    // parent: read output
    ::close(pipefd[1]);
    out = ReadAllFromFd(pipefd[0]);
    ::close(pipefd[0]);

    // wait with timeout
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

bool BotChat::HttpsPost(const std::string& url, const std::string& apiKey,
                        const std::string& jsonBody, std::string& outBody)
{
    // Write JSON body to a temp file to avoid shell quoting issues.
    char tmpl[] = "/tmp/botchat_XXXXXX";
    int fd = ::mkstemp(tmpl);
    if (fd < 0)
        return false;
    if (::write(fd, jsonBody.c_str(), jsonBody.size()) < 0) {
        ::close(fd);
        ::unlink(tmpl);
        return false;
    }
    ::close(fd);

    std::vector<std::string> args = {
        "curl", "-sS", "--max-time", "20",
        "-X", "POST", url,
        "-H", "Content-Type: application/json",
        "-H", "Authorization: Bearer " + apiKey,
        "--data", "@" + std::string(tmpl),
    };

    bool ok = RunCurl(args, outBody);
    ::unlink(tmpl);
    return ok;
}

std::string BotChat::QueryDeepSeek(const std::string& prompt, const std::string& systemHint)
{
    if (!sConfig.playerBots.ChatEnabled || sConfig.playerBots.DeepSeekKey.empty())
        return "";

    // Escape the prompt for JSON (basic: quotes and backslashes).
    auto jsonEscape = [](const std::string& s) {
        std::string o;
        for (char ch : s) {
            switch (ch) {
                case '"': o += "\\\""; break;
                case '\\': o += "\\\\"; break;
                case '\n': o += "\\n"; break;
                case '\r': o += "\\r"; break;
                case '\t': o += "\\t"; break;
                default: o += ch;
            }
        }
        return o;
    };

    std::string body = "{"
        "\"model\":\"deepseek-chat\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"" + jsonEscape(systemHint) + "\"},"
        "{\"role\":\"user\",\"content\":\"" + jsonEscape(prompt) + "\"}"
        "],"
        "\"max_tokens\":60,"
        "\"temperature\":0.9"
        "}";

    std::string resp;
    if (!HttpsPost(sConfig.playerBots.DeepSeekURL, sConfig.playerBots.DeepSeekKey, body, resp))
        return "";
    if (resp.empty())
        return "";

    // Extract "content":"..." from the JSON response (DeepSeek OpenAI-compatible).
    // Look for the last occurrence of "content":" (the assistant's message).
    std::string key = "\"content\":\"";
    size_t pos = resp.rfind(key);
    if (pos == std::string::npos)
        return "";
    pos += key.size();
    std::string text;
    for (; pos < resp.size(); ++pos) {
        char ch = resp[pos];
        if (ch == '\\') {
            if (pos + 1 < resp.size()) {
                char nx = resp[pos + 1];
                if (nx == 'n') { text += ' '; pos += 1; }
                else if (nx == '"') { text += '"'; pos += 1; }
                else if (nx == '\\') { text += '\\'; pos += 1; }
                else { text += nx; pos += 1; }
            }
        } else if (ch == '"') {
            break;
        } else {
            text += ch;
        }
    }
    // Trim whitespace.
    size_t b = text.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = text.find_last_not_of(" \t\r\n");
    return text.substr(b, e - b + 1);
}
