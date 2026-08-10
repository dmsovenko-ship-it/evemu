#ifndef EVEMU_PLAYERBOT_BOTCHAT_H_
#define EVEMU_PLAYERBOT_BOTCHAT_H_

#include "eve-compat.h"
#include "eve-common.h"

/**
 * @brief Minimal HTTPS client for DeepSeek chat completions used by simulated
 * players. Fire-and-forget: call Generate() from a worker thread; the result is
 * surfaced via a callback on the main thread. If the API is unavailable or the
 * key is empty, returns false and the bot stays silent (graceful degradation).
 *
 * @author bot-infrastructure
 */
class BotChat
{
public:
    BotChat();
    ~BotChat();

    // Returns the API reply text for a chat prompt, or empty string on failure.
    // Performs a synchronous HTTPS POST to the DeepSeek API.
    static std::string QueryDeepSeek(const std::string& prompt, const std::string& systemHint);

private:
    static bool HttpsPost(const std::string& url, const std::string& apiKey,
                          const std::string& jsonBody, std::string& outBody);
};

#endif  // EVEMU_PLAYERBOT_BOTCHAT_H_
