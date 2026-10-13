#ifndef __PREFETCH_CACHE_H__
#define __PREFETCH_CACHE_H__

/*
 * Background DB prefetch for system boot.
 *
 * Booting a system runs synchronous SQL on the GAME thread: the static and
 * dynamic entity LIST queries (SystemDB::LoadSystemStaticEntities /
 * LoadSystemDynamicEntities) plus per-item ItemFactory loads. The per-item
 * ItemFactory work touches non-thread-safe singletons and MUST stay on the game
 * thread, but the two LIST queries are plain reads into POD vectors and can be
 * run ahead of time.
 *
 * This cache runs those two queries on a worker thread (sDatabase is
 * mutex-protected, so concurrent RunQuery is safe) for the systems a player is
 * about to jump into; SystemManager::LoadSystemStatics/Dynamics then CONSUME
 * the prepared vectors instead of querying, so the jump boot no longer pays for
 * them on the game thread.
 *
 * Safety:
 *  - the worker only calls SystemDB static read methods -> plain vectors;
 *  - the cache is guarded by a mutex;
 *  - entries are single-use (moved out at boot) and expire (TTL) so a stale
 *    list is never used.
 */

#include "POD_containers.h"
#include "inventory/ItemType.h"   // ItemData (boot item preload)
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class PrefetchCache {
public:
    static PrefetchCache& get();

    void Start();
    void Stop();

    // Ask the worker to prepare a system's entity lists (dedup; skips already
    // prepared, non-stale entries).
    void Enqueue(uint32 systemID);

    // Consume a prepared list (moved out; the entry is cleared). false if the
    // system was not prefetched or the entry went stale.
    bool TakeStatic(uint32 systemID, std::vector<DBSystemEntity>& out);
    bool TakeDynamic(uint32 systemID, std::vector<DBSystemDynamicEntity>& out);

    // Worker-built per-item preloads (ItemDB ItemData map + AttributeMap rows),
    // consumed on the game thread (see ItemDB::InstallPreload /
    // AttributeMap::InstallPreload). false if not prepared / stale.
    bool TakeItemPreload(uint32 systemID, std::map<uint32, ItemData>& out);
    bool TakeAttrPreload(uint32 systemID, std::map<uint32, std::vector<PrefetchedAttr> >& out);

private:
    PrefetchCache() = default;
    void Run();

    struct Entry {
        bool staticReady = false;
        bool dynamicReady = false;
        bool itemReady = false;
        bool attrReady = false;
        std::vector<DBSystemEntity> statics;
        std::vector<DBSystemDynamicEntity> dynamics;
        std::map<uint32, ItemData> items;                              // ItemDB boot preload
        std::map<uint32, std::vector<PrefetchedAttr> > attrs;          // AttributeMap boot preload
        uint32 gen = 0;              // bumped on Enqueue; worker stamps it done
        int64 stamp = 0;             // filetime when prepared
    };

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::map<uint32, Entry> m_cache;   // prepared entries
    std::map<uint32, uint32> m_queue;  // systemID -> gen requested
    uint32 m_gen = 0;                  // monotonically increasing request gen
    std::atomic<bool> m_run{ false };
    std::thread m_thread;
};

#define sPrefetchCache \
    ( PrefetchCache::get() )

#endif  // __PREFETCH_CACHE_H__
