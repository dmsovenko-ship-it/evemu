
/**
 * @name Moon.cpp
 *   Specific Class for individual moons.
 * this class will hold all moon data and relative info for each moon.
 *
 * @Author:         Allan
 * @date:   30 April 2016
 */

#include "Moon.h"
#include "pos/Structure.h"
#include "StaticDataMgr.h"
#include "system/Celestial.h"
#include "system/SystemManager.h"
#include <random>

/** @note  general design notes
 * moonse will have a Moon class to hold data and call other functions/methods as needed
 * the PlanetMgr class will manage all aspects of moon data, init'd as a single instance (no reason for multiples)
 *
 *
 */

/*  moon probes, cause i dont where else to put them offhand...
 *   Probe     Travel_Time  Skill_Needed
 *   Quest     40 minutes   Survey III, Astrometrics III
 *   Discovery 10 minutes   Survey III, Astrometrics III
 *   Gaze      5 Minutes    Survey V, Astrometrics V
 *

    About 50% of moons will yield nothing.
    About 39% of moons will yield only gases.
    The rest will yield some combination of gasses and/or minerals.

    Normal skills to reduce probing time do not apply to survey probes

    The Covert Ops 10% time reduction skill per level DOES apply to survey probes when fired from a covops. If you are scanning a large number of moons, it may be better time-wise to use a covops. A covops is also highly recommended when scanning in hostile systems.


   What you need to Probe Moons

    Skills
    Survey III
    Astrometrics III
    Expanded Probe Launcher (NOT a Core Probe Launcher)
    Survey Probes
 */

Moon::Moon()
{

}


MoonSE::MoonSE(InventoryItemRef self, EVEServiceManager &services, SystemManager* system)
: StaticSystemEntity(self, services, system),
m_towerSE(nullptr)
{
    /** @todo finish moon resources...this is hacked */
    m_resources.clear();
/*
          [PyTuple 2 items]
            [PyInt 40126701]
            [PyList 20 items]
              [PyTuple 2 items]
                [PyInt 16633]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16634]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16635]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16636]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16637]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16638]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16639]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16640]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16641]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16642]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16643]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16644]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16646]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16647]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16648]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16649]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16650]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16651]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16652]
                [PyInt 4]
              [PyTuple 2 items]
                [PyInt 16653]
                [PyInt 4]
                */
}

bool MoonSE::LoadExtras() {
    if (!StaticSystemEntity::LoadExtras())
        return false;

    /** @todo use this to initialize moongoo data, create planet manager for moon, or whatever else
     * i decide is needed for moon management
     *  this is called after SE is created.
     */

    /*  moon goo data (notes for me)
     * goo rarity 4/8/16/32/64
     *
     *

Moon materials have different rarity classes, starting with R4 being the most common and R64 being the most rare.

    R4 (Gases) : Atmospheric Gases, Evaporite, Hydrocarbons, Silicates.
    R8 (Metals): Cobalt, Scandium, Titanium, Tungsten
    R16 (Metals): Cadmium, Chromium, Platinum, Vanadium
    R32 (Metals): Caesium, Hafnium, Mercury, Technetium
    R64 (Metals): Dysprosium, Neodymium, Promethium, Thulium

    Intermediate Materials include:

    Caesarium Cadmide
    Carbon Polymers
    Ceramic Powder
    Crystallite Alloy
    Dysporite
    Fernite Alloy
    Ferrofluid
    Fluxed Condensates
    Hexite
    Hyperflurite
    Neo Mercurite
    Platinum Technite
    Prometium
    Rolled Tungsten Alloy
    Silicon Diborite
    Solerium
    Sulfuric Acid
    Titanium Chromide
    Vanadium Hafnite
     */
    // Live-style moon composition, generated per moon (deterministic by moonID):
    // common R4/R8 almost everywhere, R16 uncommon, R32 rare, R64 rarest — and the
    // rare tiers are far more common in low/null sec ("rarer == farther"). Rare
    // materials are region-tied with random exceptions (e.g. Technetium mostly in
    // Guristas space).
    uint32 moonID = m_self->itemID();
    uint32 sysID = m_self->locationID();

    uint32 regionID = 0;
    float security = 0.0f;
    {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT r.regionID, s.security FROM mapSolarSystems s"
            " JOIN mapConstellations c ON c.constellationID = s.constellationID"
            " JOIN mapRegions r ON r.regionID = c.regionID"
            " WHERE s.solarSystemID = %u", sysID)) {
            DBResultRow row;
            if (res.GetRow(row)) { regionID = row.GetUInt(0); security = row.GetFloat(1); }
        }
    }

    static const uint16 R4[]  = { 16633, 16634, 16635, 16636 };
    static const uint16 R8[]  = { 16637, 16638, 16639, 16640 };
    static const uint16 R16[] = { 16641, 16642, 16643, 16644 };
    static const uint16 R32[] = { 16646, 16647, 16648, 16649 };
    static const uint16 R64[] = { 16650, 16651, 16652, 16653 };

    std::mt19937 rng(moonID ? moonID : 1);
    auto pct  = [&](int p) { return (int)(rng() % 100) < p; };
    auto pick = [&](const uint16* arr, int n) { return arr[rng() % n]; };

    int rareBonus = 0;
    if (security < 0.45f) rareBonus += 10;   // lowsec
    if (security < 0.05f) rareBonus += 15;   // nullsec
    if (security < -0.5f) rareBonus += 10;

    // R4 always (1-3), R8 usually, R16 sometimes.
    int n4 = 1 + (int)(rng() % 3);
    for (int i = 0; i < n4; ++i) {
        uint16 t = pick(R4, 4);
        if (!m_resources.count(t)) m_resources[t] = 4 + (rng() % 3);
    }
    if (pct(85)) { uint16 t = pick(R8, 4); m_resources[t] = 3 + (rng() % 3); }
    if (pct(55)) { uint16 t = pick(R16, 4); m_resources[t] = 2 + (rng() % 3); }

    // Region affinity: rare materials "live" in particular regions; elsewhere a
    // small random chance remains (the "rare exception"). Technetium -> Guristas.
    auto sameRegion = [&](const uint32* regions, int n) {
        for (int i = 0; i < n; ++i) if (regions[i] == regionID) return true;
        return false;
    };
    static const uint32 guristasRegions[] = { 10000015, 10000051, 10000058, 10000057,
                                              10000003, 10000023, 10000035, 10000045, 10000055 };

    // R32 (rare): usually one material, boosted in low/null.
    if (pct(20 + rareBonus)) { uint16 t = pick(R32, 4); m_resources[t] = 1 + (rng() % 2); }
    if (sameRegion(guristasRegions, 9)) { m_resources[16649] = 1 + (rng() % 2); }   // Technetium
    else if (pct(4)) { uint16 t = pick(R32, 4); m_resources[t] = 1; }               // rare exception

    // R64 (rarest): only in low/null normally; elsewhere very rare.
    if (pct(6 + rareBonus)) { uint16 t = pick(R64, 4); m_resources[t] = 1 + (rng() % 2); }
    else if (pct(2)) { uint16 t = pick(R64, 4); m_resources[t] = 1; }

    return true;
}
