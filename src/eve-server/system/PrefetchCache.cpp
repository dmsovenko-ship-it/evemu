#include "system/PrefetchCache.h"

#include "eve-server.h"
#include "system/SystemDB.h"
#include "system/SystemManager.h"
#include <chrono>

static const int64 PREFETCH_TTL = 300LL * EvE::Time::Second;   // entries older than 5 min are ignored
static const size_t PREFETCH_MAX_ENTRIES = 32;                // cap memory

PrefetchCache& PrefetchCache::get() {
    static PrefetchCache s_inst;
    return s_inst;
}

void PrefetchCache::Start() {
    if (m_run.exchange(true))
        return;
    m_thread = std::thread(&PrefetchCache::Run, this);
    _log(SERVER__INIT, "PrefetchCache: worker started.");
}

void PrefetchCache::Stop() {
    if (!m_run.exchange(false))
        return;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

void PrefetchCache::Enqueue(uint32 systemID) {
    if (systemID == 0 || !m_run)
        return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(systemID);
    if (it != m_cache.end()) {
        bool fresh = (it->second.stamp != 0)
                     && (GetFileTimeNow() - it->second.stamp) < PREFETCH_TTL;
        if (fresh && it->second.staticReady && it->second.dynamicReady)
            return;   // already prepared and fresh
    }
    m_queue[systemID] = ++m_gen;
    m_cv.notify_one();
}

void PrefetchCache::Run() {
    while (m_run) {
        uint32 systemID = 0;
        uint32 gen = 0;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::seconds(1), [this] {
                return !m_run || !m_queue.empty();
            });
            if (!m_run)
                break;
            if (m_queue.empty())
                continue;
            auto it = m_queue.begin();
            systemID = it->first;
            gen = it->second;
            m_queue.erase(it);
        }
        if (systemID == 0)
            continue;

        // Pure DB reads into POD vectors - safe off the game thread.
        std::vector<DBSystemEntity> statics;
        SystemDB::LoadSystemStaticEntities(systemID, statics);
        std::vector<DBSystemDynamicEntity> dynamics;
        SystemDB::LoadSystemDynamicEntities(systemID, dynamics);

        std::lock_guard<std::mutex> lock(m_mutex);
        // Drop the request if a newer gen was queued meanwhile.
        auto qit = m_queue.find(systemID);
        if (qit != m_queue.end() && qit->second > gen)
            continue;
        Entry& e = m_cache[systemID];
        e.statics.swap(statics);
        e.dynamics.swap(dynamics);
        e.staticReady = true;
        e.dynamicReady = true;
        e.stamp = GetFileTimeNow();

        // Cap the cache: evict the oldest prepared entries.
        while (m_cache.size() > PREFETCH_MAX_ENTRIES)
            m_cache.erase(m_cache.begin());
    }
}

bool PrefetchCache::TakeStatic(uint32 systemID, std::vector<DBSystemEntity>& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(systemID);
    if (it == m_cache.end() || !it->second.staticReady)
        return false;
    if (it->second.stamp == 0 || (GetFileTimeNow() - it->second.stamp) >= PREFETCH_TTL) {
        m_cache.erase(it);   // stale - let the caller query fresh
        return false;
    }
    out.swap(it->second.statics);
    it->second.staticReady = false;
    return true;
}

bool PrefetchCache::TakeDynamic(uint32 systemID, std::vector<DBSystemDynamicEntity>& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(systemID);
    if (it == m_cache.end() || !it->second.dynamicReady)
        return false;
    if (it->second.stamp == 0 || (GetFileTimeNow() - it->second.stamp) >= PREFETCH_TTL) {
        m_cache.erase(it);
        return false;
    }
    out.swap(it->second.dynamics);
    it->second.dynamicReady = false;
    return true;
}
