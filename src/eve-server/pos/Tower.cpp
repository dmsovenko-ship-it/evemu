
/**
 * @name Tower.cpp
 *   Class for POS Towers.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */

/*
 * POS__ERROR
 * POS__WARNING
 * POS__MESSAGE
 * POS__DUMP
 * POS__DEBUG
 * POS__DESTINY
 * POS__SLIMITEM
 * POS__TRACE
 */


#include "Client.h"
#include "EntityList.h"
#include "EVE_Mail.h"
#include "EVEServerConfig.h"
#include "inventory/Inventory.h"
#include "planet/Moon.h"
#include "pos/Module.h"
#include "pos/Tower.h"
#include "system/Container.h"
#include "system/Damage.h"
#include "system/BubbleManager.h"
#include "system/SystemBubble.h"
#include "system/SystemManager.h"

/** @todo (Allan) this class needs more research to finish
 * see pics in ::GamePC/G/games/EvE/misc/POS
 * flagStructureActive             = 144,
 * flagStructureInactive           = 145,
 * AttrOperationConsumptionRate = 687,
 * AttrReinforcedConsumptionRate = 688,
 * AttrResourceReinforced1Type = 694,
 * AttrResourceReinforced2Type = 695,
 * AttrResourceReinforced3Type = 696,
 * AttrResourceReinforced4Type = 697,
 * AttrResourceReinforced5Type = 698,
 * AttrResourceReinforced1Quantity = 699,
 * AttrResourceReinforced2Quantity = 700,
 * AttrResourceReinforced3Quantity = 701,
 * AttrResourceReinforced4Quantity = 703,
 * AttrResourceReinforced5Quantity = 704,
 * AttrResourceOnline1Type = 705,
 * AttrResourceOnline2Type = 706,
 * AttrResourceOnline3Type = 707,
 * AttrResourceOnline4Type = 708,
 ***  many other attributes for towers and their modules.....
 * AttrControlTowerMissileVelocityBonus = 792,
 * AttrControlTowerSize = 1031,
 * AttrAnchoringSecurityLevelMax = 1032,
 * AttrAnchoringRequiresSovereignty = 1033,
 * AttrControlTowerMinimumDistance = 1165,
 * AttrPosPlayerControlStructure = 1167,
 * AttrIsIncapacitated = 1168,
 * AttrPosStructureControlAmount = 1174,
 * AttrOnliningRequiresSovereigntyLevel = 1185,
 * AttrPosAnchoredPerSolarSystemAmount = 1195,
 * AttrPosStructureControlDistanceMax = 1214,
 * AttrAnchoringRequiresSovereigntyLevel = 1215,
 * AttrHarvesterType = 709,
 * AttrHarvesterQuality = 710,
 * AttrMoonAnchorDistance = 711,
 * AttrUsageDamagePercent = 712,
 * AttrConsumptionType = 713,
 * AttrConsumptionQuantity = 714,
 * AttrMaxOperationalDistance = 715,    for sma, hangar, etc
 * AttrMaxOperationalUsers = 716,    for sma, hangar, etc
 * AttrRefiningYieldMultiplier = 717,
 * AttrOperationalDuration = 719,
 * AttrRefineryCapacity = 720,
 * AttrRefiningDelayMultiplier = 721,
 * AttrPosControlTowerPeriod = 722,
 * AttrMoonMiningAmount = 726,
 * AttrControlTowerLaserDamageBonus = 728,
 * AttrControlTowerLaserOptimalBonus = 750,
 * AttrControlTowerHybridOptimalBonus = 751,
 * AttrControlTowerProjectileOptimalBonus = 752,
 * AttrControlTowerProjectileFallOffBonus = 753,
 * AttrControlTowerProjectileROFBonus = 754,
 * AttrControlTowerMissileROFBonus = 755,
 * AttrControlTowerMoonHarvesterCPUBonus = 756,
 * AttrControlTowerSiloCapacityBonus = 757,
 * AttrControlTowerLaserProximityRangeBonus = 760,
 * AttrControlTowerProjectileProximityRangeBonus = 761,
 * AttrControlTowerHybridProximityRangeBonus = 762,
 * AttrMaxGroupActive = 763,
 * AttrControlTowerEwRofBonus = 764,
 * AttrScanRange = 765,
 * AttrControlTowerHybridDamageBonus = 766,
 * AttrTrackingSpeedBonus = 767,
 * AttrMaxRangeBonus2 = 769,
 * AttrControlTowerEwTargetSwitchDelayBonus = 770,
 * AttrAmmoCapacity = 771,
 * AttrActivationBlocked = 1349,
 * AttrActivationBlockedStrenght = 1350,
 * AttrPosCargobayAcceptType = 1351,
 * AttrPosCargobayAcceptGroup = 1352,
 */

TowerSE::TowerSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& fData)
: StructureSE(structure, services, system, fData),
m_pShieldSE(nullptr),
m_manualTargetID(0),
m_botFuelled(false),
m_lastPlayerCount(0),
m_lastFieldAnnounce(0)
{
    m_hasShield = false;
    m_structs.clear();
    for (int i = 0; i < 4; ++i)
        m_hardenerApplied[i] = 0.0f;

    // create AI object for tower here....not written yet.
    //m_ai = new POS_AI(this);

    m_pg = m_self->GetAttribute(AttrPowerOutput).get_int();
    m_cpu = m_self->GetAttribute(AttrCpuOutput).get_int();

    m_tsize = m_self->GetAttribute(AttrControlTowerSize).get_int();
    if ((m_tsize < 1) or (m_tsize > 3))
        m_tsize = 1;  // do something constructive here cause size is wrong
    m_soi = m_self->GetAttribute(AttrPosStructureControlDistanceMax).get_int() * m_tsize;

    m_tdata = EVEPOS::TowerData();

    m_fuelTypeID = 4247;        // Fuel Blocks
    m_fuelPerHour = m_tsize * 10;  // 10/20/40 for S/M/L
    m_strontTypeID = 16275;     // Strontium Clathrates
    m_strontPerHour = m_tsize * 100; // 100/200/400 for S/M/L
    m_lastFuelCheck = GetFileTimeNow();
    m_lastFuelPct = 100.0f;

    m_pgUsed = 0.0f;
    m_cpuUsed = 0.0f;

    /** @note these are defined, but i dunno what they are
     * AttrControlTowerMinimumDistance
     *
     *
     *
     *
     *
     */
}

void TowerSE::Init()
{    StructureSE::Init();

    if (!m_db.GetTowerData(m_tdata, m_data)) {
        _log(SE__TRACE, "TowerSE %s(%u) has no saved data.  Initializing default set.", m_self->name(), m_self->itemID());
        // invalid data....init to 0 as this will only hit for currently-launching items (or errors)
        InitData();
    }

    // if password is already set and tower online, then we can online (create) the forcefield
    m_harmonic = m_tdata.harmonic;

    // The shield IS the force field. Control Towers carry AttrShieldCapacity but
    // NOT a persisted AttrShieldCharge, so a fresh load reads 0 — which the
    // damage-driven reinforcement check would mistake for "below 25%" and put
    // every tower straight into reinforced mode. Treat a missing/zero charge as
    // a full shield.
    {
        double cap = m_self->GetAttribute(AttrShieldCapacity).get_float();
        if (cap > 0.0 && m_self->GetAttribute(AttrShieldCharge).get_float() <= 0.0)
            m_self->SetAttribute(AttrShieldCharge, cap, false);
    }

    if ((m_harmonic > EVEPOS::Harmonic::Offline)
    and (!m_tdata.password.empty())
    and (m_data.state > EVEPOS::StructureState::Anchored))
        CreateForceField();

    // if tower anchored, tell moon this is its tower
    if (m_data.state > EVEPOS::StructureState::Unanchored)
        m_moonSE->SetTower(this);

    // set tower in bubble
    if (m_bubble == nullptr)
        assert(0);
    m_bubble->SetTowerSE(this);

    // initialize fuel data
    InitFuelData();

    // recalculate PG/CPU load from all existing modules
    RecalcResources();

    // re-apply shield hardener bonuses from online hardening arrays (loaded POS)
    ApplyHardeners();

    // if we were online/operating when server went down, calculate elapsed fuel
    if ((m_data.state >= EVEPOS::StructureState::Online) and (m_data.state <= EVEPOS::StructureState::Operating)) {
        // fuel will be consumed on next Process() tick
        m_lastFuelCheck = GetFileTimeNow();
    }
}

void TowerSE::InitData() {
    // init base data first
    StructureSE::InitData();
    m_tdata.harmonic = m_harmonic;     // set during base SE creation
    m_tdata.standingOwnerID = 0;    /** @todo  get sov holder here. */

    m_db.SaveTowerData(m_tdata, m_data);
}

// Called by StructureSE::BotDeployAndAnchor after the common anchored/online
// state is set: link the tower to its moon, register it in the bubble and
// persist the tower data so a reload brings the force field back.
void TowerSE::OnBotAnchorComplete()
{
    if (m_moonSE != nullptr)
        m_moonSE->SetTower(this);
    if (m_bubble != nullptr)
        m_bubble->SetTowerSE(this);

    m_harmonic = EVEPOS::Harmonic::Offline;
    m_tdata.harmonic = m_harmonic;
    // Bot towers need a password: the force field is only created when one is
    // set (SetOnline/Init), so password-less bot towers never showed a field.
    // UpdateTowerData, not SaveTowerData — the row already exists from Init().
    if (m_tdata.password.empty())
        m_tdata.password = std::to_string(MakeRandomInt(100000, 999999));
    m_db.UpdateTowerData(m_tdata, m_data);
    InitFuelData();
    BotEnsureFuel(720);   // bots fuel the tower before launch so it never drops to reinforced
}

// Bot POS fuel: top the tower up to `hours` of fuel in its cargo hold (default
// 30 days) and, if it had already run dry and gone reinforced/offline, bring it
// back online now that it has fuel. Idempotent — safe to call every docked cycle.
void TowerSE::BotEnsureFuel(uint32 hours)
{
    if (hours == 0)
        hours = 720;
    if (m_fuelPerHour == 0)
        m_fuelPerHour = m_tsize * 10;

    // Self-heal a bot tower that was persisted Unanchored: pre-fix deploys set
    // the anchored/online state in memory but saved it with an INSERT that
    // failed on the existing row, so every reload came up unanchored (no field,
    // no defences) while the modules self-healed online around it. Anchor where
    // it sits, link the moon and bubble, then continue to online below.
    if (m_data.state <= EVEPOS::StructureState::Unanchored) {
        InitData();   // resolves m_moonSE (the anchor point)
        if (m_moonSE != nullptr)
            m_moonSE->SetTower(this);
        if (m_bubble != nullptr)
            m_bubble->SetTowerSE(this);
        m_self->SetFlag(flagStructureActive);
        m_data.state = EVEPOS::StructureState::Anchored;
        m_db.UpdateBaseData(m_data);
        _log(POS__MESSAGE, "TowerSE::BotEnsureFuel() - %s(%u) was unanchored — re-anchored at moon %u.",
             GetName(), m_self->itemID(), m_data.anchorpointID);
    }

    Inventory* inv = m_self->GetMyInventory();
    if (inv == nullptr)
        return;

    uint32 have = 0;
    {
        std::vector<InventoryItemRef> items;
        inv->GetItemsByFlag(flagCargoHold, items);
        for (auto& it : items)
            if (it->typeID() == m_fuelTypeID)
                have += it->quantity();
    }

    uint32 target = hours * m_fuelPerHour;
    if (have < target) {
        uint32 add = target - have;
        ItemData idata((uint16)m_fuelTypeID, m_self->ownerID(), m_self->itemID(), flagCargoHold, add);
        InventoryItemRef fuel = sItemFactory.SpawnItem(idata);
        if (fuel.get() != nullptr) {
            inv->AddItem(fuel);
            _log(POS__MESSAGE, "TowerSE::BotEnsureFuel() - %s(%u) fuelled with %u x %u (had %u).",
                 GetName(), m_self->itemID(), add, m_fuelTypeID, have);
        }
    }

    // The force field needs a password (self-heal towers deployed before this).
    if (m_tdata.password.empty()) {
        m_tdata.password = std::to_string(MakeRandomInt(100000, 999999));
        m_db.UpdateTowerData(m_tdata, m_data);
    }

    // A bot tower's force field must be ONLINE: harmonic Offline(0)/Inactive(-1)
    // means "field down", and FieldSE::EncodeDestiny then sends the ball in STOP
    // mode so the client draws no sphere at all.  Older deploys persisted
    // harmonic 0 while the tower was already Online (Init only re-creates the
    // field for harmonic > Offline), so heal it here - before the field fallback
    // below, so the freshly created ball goes out in FIELD mode.
    if (m_tdata.harmonic <= EVEPOS::Harmonic::Offline) {
        m_harmonic = EVEPOS::Harmonic::Online;
        m_tdata.harmonic = m_harmonic;
        m_db.UpdateTowerData(m_tdata, m_data);
    }

    // If the tower dropped offline (e.g. it ran dry before the bot started
    // topping it up), bring it back online now that it has fuel. A DAMAGE
    // reinforcement is NOT auto-cleared — the tower stays reinforced until its
    // timer expires (otherwise the self-heal would fight the 25% rule).
    if (m_data.state > EVEPOS::StructureState::Unanchored
        && m_data.state != EVEPOS::StructureState::Online
        && m_data.state != EVEPOS::StructureState::Operating
        && m_data.state != EVEPOS::StructureState::Reinforced
        && m_data.state != EVEPOS::StructureState::SheildReinforced
        && m_data.state != EVEPOS::StructureState::ArmorReinforced) {
        _log(POS__MESSAGE, "TowerSE::BotEnsureFuel() - %s(%u) re-onlining (state was %u).",
             GetName(), m_self->itemID(), (unsigned)m_data.state);
        SetOnline();
    }

    // Field fallback: covers towers already Online whose password was just set
    // above (SetOnline creates the field only when a password is present).
    if (m_data.state >= EVEPOS::StructureState::Online && !m_hasShield)
        CreateForceField();

    // Tower is up — now anchor+online its modules (same sequence a real pilot
    // follows: tower first, then each module).
    BotOnlineModules();
}

// Bot POS: bring the tower's modules online once the tower itself is online.
// Modules are found by scanning the system for POS structures within the force
// field that belong to the same owner (the tower does not track its modules).
void TowerSE::BotOnlineModules()
{
    if (m_system == nullptr)
        return;
    if (m_data.state < EVEPOS::StructureState::Online)
        return;   // no online tower — modules cannot run (and must not show online)

    double r = GetShieldRadius();
    GPoint tp = GetPosition();

    for (auto& [id, se] : m_system->GetEntities()) {
        if (se == nullptr || se == this)
            continue;
        StructureSE* mod = se->GetPOSSE();
        if (mod == nullptr || mod == this)
            continue;
        if (mod->IsTowerSE() || mod->IsTCUSE() || mod->IsSBUSE() || mod->IsIHubSE())
            continue;
        if (mod->GetSelf().get() == nullptr)
            continue;
        if (mod->GetSelf()->ownerID() != m_self->ownerID())
            continue;
        if (tp.distance(mod->GetPosition()) > r)
            continue;
        if (mod->GetState() >= EVEPOS::StructureState::Online)
            continue;   // already online/operating
        _log(POS__MESSAGE, "TowerSE::BotOnlineModules() - onlining %s(%u) at tower %s(%u).",
             mod->GetName(), mod->GetID(), GetName(), m_self->itemID());
        mod->SetOnline();
    }

    // newly-onlined hardening arrays change the shield resistances
    ApplyHardeners();
}

void TowerSE::Scoop() {
    StructureSE::Scoop();
    m_moonSE->SetTower(nullptr);
    m_tdata = EVEPOS::TowerData();
    m_self->ChangeSingleton(false);
    m_self->SaveItem();
}

void TowerSE::Process()
{
    /* called by EntityList::Process on every loop */

    // Bot-owned POS (customInfo "botpos"): fuel it on the first tick after a
    // load/boot and re-online it if it had run dry — so an industrialist's tower
    // never sits reinforced/invisible. Done here (not in Init) so m_destiny is
    // ready for the state/effect updates.
    if (!m_botFuelled && m_self.get() != nullptr && m_self->customInfo() == "botpos") {
        m_botFuelled = true;
        BotEnsureFuel(720);
    }

    // starbase charter checks for empire space

    // tower-specific tests here

    /*  Enable base call to Process Anchoring, Targeting and Movement  */
    StructureSE::Process();

    // Force-field ball delivery — DISABLED. Both periodic re-announce variants
    // broke the client's destiny parse and took down the whole grid (overview
    // never loads, space "jitters", client logs 100s of "Unknown ball mode in
    // dump" + "BallNotInPark" for every static slim):
    //   - packet_type=0 (full state)  -> client re-inits its park every 30s ("flicker" loop)
    //   - packet_type=1 (balls only)  -> stream desync ("Unknown ball mode")
    // The field ball is IsGlobal and is delivered by the normal static/bubble
    // paths on grid entry; do not re-send it here.
    m_lastPlayerCount = 0;
    m_lastFieldAnnounce = 0;

    // Damage-driven reinforcement (EVE): the tower's shield IS its force field.
    // When the shield is reduced to 25% the tower enters reinforced mode — the
    // field collapses and the tower is invulnerable until the timer expires
    // (ReinforceTower consumes strontium to size the window).
    if (m_data.state == EVEPOS::StructureState::Online
        || m_data.state == EVEPOS::StructureState::Operating) {
        double cap = m_self->GetAttribute(AttrShieldCapacity).get_float();
        double cur = m_self->GetAttribute(AttrShieldCharge).get_float();
        if (cap > 0.0 && cur <= cap * 0.25)
            ReinforceTower();
    }

    // consume fuel while online or operating
    if ((m_data.state >= EVEPOS::StructureState::Online)
    and (m_data.state <= EVEPOS::StructureState::Operating)) {
        CheckFuel();
    }
}

void TowerSE::InitFuelData()
{
    // Load fuel requirements from invControlTowerResources for this tower type.
    // Fall back to defaults (Fuel Blocks, sized-based) if no DB data found.
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
            "SELECT resourceTypeID, quantity, purpose"
            " FROM invControlTowerResources"
            " WHERE controlTowerTypeID = %u AND (purpose = 1 OR purpose = 4)"
            " ORDER BY purpose",
            m_self->typeID()))
    {
        // query failed; keep defaults set in constructor
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 purpose = row.GetInt(2);
        if (purpose == 1) {
            // online fuel
            m_fuelTypeID = row.GetInt(0);
            m_fuelPerHour = row.GetInt(1);
        } else if (purpose == 4) {
            // reinforced fuel (strontium)
            m_strontTypeID = row.GetInt(0);
            m_strontPerHour = row.GetInt(1);
        }
    }
}

bool TowerSE::CheckFuel()
{
    int64 now = GetFileTimeNow();
    int64 elapsed = now - m_lastFuelCheck;

    // check roughly once per minute
    if (elapsed < EvE::Time::Minute)
        return true;

    m_lastFuelCheck = now;

    double hoursPassed = (double)elapsed / (double)EvE::Time::Hour;
    uint32 fuelNeeded = (uint32)(hoursPassed * m_fuelPerHour);
    if (fuelNeeded < 1)
        return true;

    Inventory* inv = m_self->GetMyInventory();
    if (inv == nullptr)
        return false;

    // collect all fuel items from cargo
    std::vector<InventoryItemRef> items;
    inv->GetItemsByFlag(flagCargoHold, items);

    uint32 fuelFound = 0;
    std::vector<InventoryItemRef> fuelItems;
    for (auto& item : items) {
        if (item->typeID() == m_fuelTypeID) {
            fuelFound += item->quantity();
            fuelItems.push_back(item);
        }
    }

    if (fuelFound >= fuelNeeded) {
        // consume fuel, oldest stacks first
        uint32 toConsume = fuelNeeded;
        for (auto& item : fuelItems) {
            if (toConsume == 0)
                break;
            uint32 qty = item->quantity();
            if (qty <= toConsume) {
                toConsume -= qty;
                item->Delete();
            } else {
                item->SetQuantity(qty - toConsume);
                item->SaveItem();
                toConsume = 0;
            }
        }

        // check fuel percentage for low-fuel notifications
        uint32 fuelRemaining = fuelFound - fuelNeeded;
        float fuelPct = (float)fuelRemaining / (float)(fuelRemaining + m_fuelPerHour);
        if (fuelPct > 1.0f) fuelPct = 1.0f;

        if (m_tdata.sendFuelNotifications and (fuelPct < m_lastFuelPct)) {
            // send notification at thresholds: 50%, 25%, 10%, 5%
            if ((fuelPct < 0.05f and m_lastFuelPct >= 0.05f)
            or  (fuelPct < 0.10f and m_lastFuelPct >= 0.10f)
            or  (fuelPct < 0.25f and m_lastFuelPct >= 0.25f)
            or  (fuelPct < 0.50f and m_lastFuelPct >= 0.50f)) {
                _log(POS__MESSAGE, "TowerSE::CheckFuel() - Tower %s(%u) fuel at %.0f%%.", GetName(), m_self->itemID(), fuelPct * 100.0f);
                /** @todo send EVE mail notification to corp members */
            }
        }

        // create calendar event for fuel expiry if enabled
        if (m_tdata.showInCalendar and (m_lastFuelPct < 0.90f) and (fuelPct > 0.0f)) {
            // calculate when fuel runs out
            double hoursRemaining = (double)fuelRemaining / (double)m_fuelPerHour;
            double expiryTime = GetFileTimeNow() + (hoursRemaining * EvE::Time::Hour);
            // CalendarDB::SaveSystemEvent(m_corpID, m_self->itemID(), (int64)expiryTime,
            //    Calendar::AutoEvent::PosFuel, "Fuel Expiration", "...", true);
        }

        m_lastFuelPct = fuelPct;
        return true;
    }

    // not enough fuel - enter reinforced mode
    _log(POS__MESSAGE, "TowerSE::CheckFuel() - Tower %s(%u) has run out of fuel!  Entering reinforced mode.",
            GetName(), m_self->itemID());

    // notify corp
    PyDict* fuelData = new PyDict();
        fuelData->SetItemString("towerID", new PyInt(m_self->itemID()));
        fuelData->SetItemString("solarSystemID", new PyInt(m_system->GetID()));
    sEntityList.CreateNotification(m_corpID, Notify::Types::TowerAlert, m_self->itemID(), fuelData);

    ReinforceTower();
    return false;
}

void TowerSE::RecalcResources()
{
    m_pgUsed = 0.0f;
    m_cpuUsed = 0.0f;

    for (auto& cur : m_structs) {
        StructureSE* pSE = cur.second;
        // only count online/operating modules
        if (pSE->GetState() < EVEPOS::StructureState::Online)
            continue;

        InventoryItemRef itemRef = pSE->GetSelf();
        if (itemRef->HasAttribute(AttrPower))
            m_pgUsed += itemRef->GetAttribute(AttrPower).get_float();
        if (itemRef->HasAttribute(AttrCpu))
            m_cpuUsed += itemRef->GetAttribute(AttrCpu).get_float();
    }

    _log(POS__DEBUG, "TowerSE::RecalcResources() - Tower %s(%u): PG %.0f/%.0f, CPU %.0f/%.0f",
            GetName(), m_self->itemID(), m_pgUsed, m_pg, m_cpuUsed, m_cpu);
}

void TowerSE::ToggleProcessCycle()
{
    // toggle active flag on all moon miners and reactors
    for (auto& cur : m_structs) {
        StructureSE* pSE = cur.second;
        if (pSE->IsReactorSE()) {
            ReactorSE* pReactor = pSE->GetReactorSE();
            if (pReactor != nullptr) {
                pReactor->SetActive(!pReactor->IsActive());
                _log(POS__MESSAGE, "TowerSE::ToggleProcessCycle() - Reactor %s(%u) is now %s.",
                        pSE->GetName(), pSE->GetID(), pReactor->IsActive() ? "active" : "inactive");
            }
        }
        // future: handle moon miners here
    }
}

void TowerSE::OnlineModule(StructureSE* pSE)
{
    InventoryItemRef itemRef = pSE->GetSelf();
    if (itemRef->HasAttribute(AttrPower))
        m_pgUsed += itemRef->GetAttribute(AttrPower).get_float();
    if (itemRef->HasAttribute(AttrCpu))
        m_cpuUsed += itemRef->GetAttribute(AttrCpu).get_float();

    _log(POS__MESSAGE, "TowerSE::OnlineModule() - %s(%u) online on Tower %s(%u).  PG %.0f/%.0f, CPU %.0f/%.0f",
            pSE->GetName(), pSE->GetID(), GetName(), m_self->itemID(),
            m_pgUsed, m_pg, m_cpuUsed, m_cpu);
}

void TowerSE::OfflineModule(StructureSE* pSE)
{
    InventoryItemRef itemRef = pSE->GetSelf();
    if (itemRef->HasAttribute(AttrPower))
        m_pgUsed -= itemRef->GetAttribute(AttrPower).get_float();
    if (itemRef->HasAttribute(AttrCpu))
        m_cpuUsed -= itemRef->GetAttribute(AttrCpu).get_float();

    // prevent negative values from rounding errors
    if (m_pgUsed < 0.0f) m_pgUsed = 0.0f;
    if (m_cpuUsed < 0.0f) m_cpuUsed = 0.0f;

    _log(POS__MESSAGE, "TowerSE::OfflineModule() - %s(%u) offline on Tower %s(%u).  PG %.0f/%.0f, CPU %.0f/%.0f",
            pSE->GetName(), pSE->GetID(), GetName(), m_self->itemID(),
            m_pgUsed, m_pg, m_cpuUsed, m_cpu);
}

bool TowerSE::HasPG(float amount)
{
    return ((m_pg - m_pgUsed) >= amount);
}

bool TowerSE::HasCPU(float amount)
{
    return ((m_cpu - m_cpuUsed) >= amount);
}

/*
 * 473     prototypingBonus    250000  NULL
 * 556     anchoringDelay  1800000     NULL
 * 650     maxStructureDistance    50000   NULL
 * 676     unanchoringDelay    NULL    3600000
 * 677     onliningDelay   1800000     NULL
 * 680     shieldRadius    30000   NULL
 * 711     moonAnchorDistance  100000  NULL
 * 1214    posStructureControlDistanceMax  NULL    15000
 */

void TowerSE::SetOnline()
{
    //StructureSE::SetOnline();

    m_data.timestamp = GetFileTimeNow();
    m_self->SetFlag(flagStructureActive);
    m_procState = EVEPOS::ProcState::Online;
    m_data.state = EVEPOS::StructureState::Online;
    m_harmonic = EVEPOS::Harmonic::Online;
    m_tdata.harmonic = m_harmonic;
    SetTimer(m_self->GetAttribute(AttrOnliningDelay).get_int());

    // A control tower's force field only exists once it has a password (both
    // Init and SetOnline gate CreateForceField on it), so a tower anchored
    // without one came online shieldless — the player could fly right up to it.
    // Assign one at anchoring/online time so the field is always generated; the
    // owner can change it in-game later.
    if (m_tdata.password.empty())
        m_tdata.password = std::to_string(MakeRandomInt(100000, 999999));

    if ((m_harmonic > EVEPOS::Harmonic::Offline)
    and (!m_tdata.password.empty()))
        CreateForceField();

    SendSlimUpdate();
    m_destiny->SendSpecialEffect(m_self->itemID(),m_self->itemID(),m_self->typeID(),0,0,"effects.StructureOnline",0,1,1,-1,0);

    m_db.UpdateBaseData(m_data);
    // persist harmonic + password so a reload re-creates the force field in Init()
    m_db.UpdateTowerData(m_tdata, m_data);
    // online hardeners (re)shape the shield resistances
    ApplyHardeners();

    /** @todo determine fuel supply and make calendar event for expiration

    std::string title = "Expiration of Tower fuel";

    std::string description = "The Control Tower ";
    description += ** get tower name here **;
    description += " in ";
    description += ** get tower location name here **;
    description += " will exhaust it's current fuel supply at this time.";      // change this to eve datetime?
    CalendarDB::SaveSystemEvent(call.client->GetCorporationID(), [towerID here?], expiryTime,
                                Calendar::AutoEvent::PosFuel, title, description, true);
     */
}

void TowerSE::SetOffline()
{
    if (m_hasShield) {
        m_pShieldSE->Delete();
        SafeDelete(m_pShieldSE);
        m_hasShield = false;
    }

    StructureSE::SetOffline();
}


void TowerSE::Online()
{
    // structure online, but not operating
    // take resources or whatever needs to be done
    /*
     * 1031    controlTowerSize    3   NULL
     *
     * POS fuel usage per hour
     *  Fuel blocks    s:10   m:20   l:40
     */

    // if fuel has run out, start reinforced mode.


    // reset timers
    StructureSE::Online();
}

void TowerSE::Operating()
{
    // structure operating
    // take resources or whatever needs to be done

    //1031    controlTowerSize    3

    // if fuel has run out, start reinforced mode.

    // reset timers
    StructureSE::Operating();
}

void TowerSE::ReinforceTower()
{
    // shut down force field
    if (m_hasShield) {
        m_pShieldSE->Delete();
        SafeDelete(m_pShieldSE);
        m_hasShield = false;
    }

    // check strontium in cargo to calculate reinforced duration
    Inventory* inv = m_self->GetMyInventory();
    uint32 strontHours = 0;
    if (inv != nullptr) {
        std::vector<InventoryItemRef> items;
        inv->GetItemsByFlag(flagCargoHold, items);
        for (auto& item : items) {
            if (item->typeID() == m_strontTypeID) {
                strontHours += item->quantity() / m_strontPerHour;
                // consume all stront
                item->Delete();
            }
        }
    }

    // minimum 1 hour, maximum 48 hours
    if (strontHours < 1)
        strontHours = 1;
    if (strontHours > 48)
        strontHours = 48;

    _log(POS__MESSAGE, "TowerSE::ReinforceTower() - Tower %s(%u) reinforced for %u hours with %u stront units.",
            GetName(), m_self->itemID(), strontHours, strontHours * m_strontPerHour);

    // set state reinforced
    m_self->SetFlag(flagStructureInactive);
    m_data.state = EVEPOS::StructureState::Reinforced;
    m_procState = EVEPOS::ProcState::Reinforcing;
    // convert hours to ms for timer
    SetTimer(strontHours * 3600000);

    SendSlimUpdate();
    m_db.UpdateBaseData(m_data);
    m_destiny->SendSpecialEffect(m_self->itemID(), m_self->itemID(), m_self->typeID(), 0, 0, "effects.StructureOffline", 0, 0, 0, -1, 0);
}

void TowerSE::Reinforced()
{
    // dunno what to do here yet.
}

void TowerSE::UpdatePassword()
{
    if (m_tdata.password.empty()) {
        m_harmonic =EVEPOS::Harmonic::Offline;
        m_tdata.harmonic = m_harmonic;
        if (m_pShieldSE == nullptr)
            return;

        m_pShieldSE->SetHarmonic(m_harmonic);

        //  this is for UPDATING forcefield ONLY...do not send on creation.
        std::vector<PyTuple*> updates;
        //  'massive' enables client-side bounce
        SetBallMassive sbm;
            sbm.entityID = m_pShieldSE->GetSelf()->itemID();
            sbm.is_massive = false;         // disable client-side bump checks
        updates.push_back(sbm.Encode());
        // harmonic for ForceField
        SetBallHarmonic sbh;
            sbh.itemID = m_pShieldSE->GetSelf()->itemID();
            sbh.corpID = m_corpID;
            sbh.allianceID = m_allyID;
            sbh.mass = -1;      // always -1
            sbh.harmonic = m_harmonic;
        updates.push_back(sbh.Encode());
        m_destiny->SendDestinyUpdate(updates); //consumed
    } else {
        m_harmonic = EVEPOS::Harmonic::Online;
        m_tdata.harmonic = m_harmonic;

        if (m_data.state > EVEPOS::StructureState::Anchored)
            CreateForceField();
    }

    m_db.UpdateHarmonicAndPassword(m_data.itemID, m_tdata);
}

void TowerSE::SetDeployFlags(int8 anchor/*0*/, int8 unanchor/*0*/, int8 online/*0*/, int8 offline/*0*/)
{
    m_tdata.anchor = anchor;
    m_tdata.unanchor = unanchor;
    m_tdata.online = online;
    m_tdata.offline = offline;

    m_db.UpdateDeployFlags(m_data.itemID, m_tdata);
}

PyRep* TowerSE::GetDeployFlags()
{
    PyList* header = new PyList(4);
        header->SetItemString(0, "anchor");
        header->SetItemString(1, "unanchor");
        header->SetItemString(2, "online");
        header->SetItemString(3, "offline");
    PyList* line = new PyList(4);           // these are structure permissions for this tower
        PySetItemRelease(line, 0, new PyInt(m_tdata.anchor));
        PySetItemRelease(line, 1, new PyInt(m_tdata.unanchor));
        PySetItemRelease(line, 2, new PyInt(m_tdata.online));
        PySetItemRelease(line, 3, new PyInt(m_tdata.offline));

    PyDict* dict = new PyDict();
    dict->SetItemString("header", header);
    dict->SetItemString("line", line);

    return new PyObject("util.Row", dict);
}

void TowerSE::SetUseFlags(uint32 itemID, int8 view, int8 take, int8 use/*0*/)
{
    std::map<uint32, StructureSE*>::iterator itr = m_structs.find(itemID);
    if (itr != m_structs.end()) {
        itr->second->SetUsageFlags(view, take, use);
        itr->second->UpdateUsageFlags();
    }
}

PyRep* TowerSE::GetUsageFlagList()
{
    /*
                [PyObject Name: eve.common.script.sys.rowset.Rowset]
                    [PyDict 3 kvp]
                        Key:[PyString "header"]
                        ==Value:[PyList 4 items]
                                    [PyString "structureID"]
                                    [PyString "viewput"]
                                    [PyString "viewputtake"]
                                    [PyString "use"]
                        Key:[PyString "RowClass"]
                        ==Value:[PyToken carbon.common.script.sys.row.Row]
                        Key:[PyString "lines"]
                        ==Value:[PyList 1 items]
                                    [PyList 4 items]
                                        [PyIntegerVar 1010759458081]
                                        [PyInt 3]
                                        [PyInt 0]
                                        [PyInt 0]
                                        */

    PyList* header = new PyList(4);
        header->SetItemString(0, "structureID");
        header->SetItemString(1, "viewput");
        header->SetItemString(2, "viewputtake");
        header->SetItemString(3, "use");
    PyList* lines = new PyList();
    for (auto cur : m_structs) {
        PyList* line = new PyList(4);
            PySetItemRelease(line, 0, new PyInt(cur.first));
            PySetItemRelease(line, 1, new PyInt(cur.second->CanView()));
            PySetItemRelease(line, 2, new PyInt(cur.second->CanTake()));
            PySetItemRelease(line, 3, new PyInt(cur.second->CanUse()));
        lines->AddItem(line);
    }

    PyDict* dict = new PyDict();
    dict->SetItemString("header", header);
    dict->SetItemString("RowClass", new PyToken("util.Row"));
    dict->SetItemString("lines", lines);

    return new PyObject("util.Rowset", dict);
}

PyRep* TowerSE::GetProcessInfo()
{
    /*
            info = self.posMgr.GetMoonProcessInfoForTower(self.slimItem.itemID)
            itemID, active, reaction, connections, demands, supplies in info:

            if reaction and rec.groupID == const.groupMobileReactor and not demands and not supplies:
                demands = [ (row.typeID, row.quantity) for row in cfg.invtypereactions[reaction[1]] if row.input == 1 ]
                supplies = [ (row.typeID, row.quantity) for row in cfg.invtypereactions[reaction[1]] if row.input == 0 ]
                demand = {}
                demands = demands or []
                for tID, quant in demands:
                    demand[tID] = quant

                    supply = {}
                    supplies = supplies or []
                    for tID, quant in supplies:
                        supply[tID] = quant

                for sourceID, tID in connections:
                    self.sr.structureConnections[tID, sourceID] = itemID

        [PyList 2 items]
          [PyTuple 6 items]
            [PyIntegerVar 1002332982210]
            [PyBool False]
            [PyNone]
            [PyList 0 items]
            [PyList 0 items]
            [PyList 0 items]
          [PyTuple 6 items]
            [PyIntegerVar 1002331680835]
            [PyBool False]
            [PyNone]
            [PyList 0 items]
            [PyList 0 items]
            [PyList 0 items]
            */

    PyList* list = new PyList();
    for (auto cur : m_structs) {
        StructureSE* sSE = cur.second;
        if (sSE == nullptr) continue;
        uint32 gID = sSE->GetSelf()->groupID();
        if (gID != EVEDB::invGroups::Moon_Mining &&
            gID != EVEDB::invGroups::Silo &&
            gID != EVEDB::invGroups::Mobile_Reactor)
            continue;

        PyTuple* tuple = new PyTuple(6);
        PySetItemRelease(tuple, 0, new PyInt(cur.first));

        // Active state
        bool active = false;
        if (sSE->IsReactorSE()) {
            ReactorSE* rSE = sSE->GetReactorSE();
            active = rSE->IsActive();
        }
        PySetItemRelease(tuple, 1, new PyBool(active));

        // Reaction type
        if (sSE->IsReactorSE()) {
            int32 reactionType = sSE->GetReactorSE()->GetReactorData()->GetReaction();
            if (reactionType > 0)
                PySetItemRelease(tuple, 2, new PyInt(reactionType));
            else
                tuple->SetItem(2, PyStatic.NewNone());
        } else {
            tuple->SetItem(2, PyStatic.NewNone());
        }

        // Connections, demands, supplies
        if (sSE->IsReactorSE()) {
            ReactorData* rData = sSE->GetReactorSE()->GetReactorData();

            // Connections: tuple (sourceID, toID)
            PyList* connList = new PyList();
            for (auto& [connItemID, conn] : rData->GetConnections()) {
                PyTuple* connTuple = new PyTuple(2);
                PySetItemRelease(connTuple, 0, new PyInt(conn.sourceID));
                PySetItemRelease(connTuple, 1, new PyInt(conn.toID));
                connList->AddItem(connTuple);
            }
            tuple->SetItem(3, connList);

            // Demands (inputs needed)
            PyList* demList = new PyList();
            for (auto& [resID, res] : rData->GetDemands()) {
                PyTuple* demTuple = new PyTuple(2);
                PySetItemRelease(demTuple, 0, new PyInt(res.typeID));
                PySetItemRelease(demTuple, 1, new PyInt(res.quantity));
                demList->AddItem(demTuple);
            }
            tuple->SetItem(4, demList);

            // Supplies (outputs available)
            PyList* supList = new PyList();
            for (auto& [resID, res] : rData->GetSupplies()) {
                if (res.quantity > 0) {
                    PyTuple* supTuple = new PyTuple(2);
                    PySetItemRelease(supTuple, 0, new PyInt(res.typeID));
                    PySetItemRelease(supTuple, 1, new PyInt(res.quantity));
                    supList->AddItem(supTuple);
                }
            }
            tuple->SetItem(5, supList);
        } else {
            PySetItemRelease(tuple, 3, new PyList());
            PySetItemRelease(tuple, 4, new PyList());
            PySetItemRelease(tuple, 5, new PyList());
        }

        list->AddItem(tuple);
    }

    if (is_log_enabled(POS__RSP_DUMP))
        list->Dump(POS__RSP_DUMP, "   ");
    return list;
}

// The tower's force field radius (metres).  shieldRadius (AttrShieldRadius) is
// stored in dgmTypeAttributes.valueInt, and the per-item copy may be missing
// (temp items skip the entity_attributes load), so resolve defensively:
// item attribute -> type attribute -> 20 km default.
double TowerSE::GetShieldRadius() {
    double r = 0.0;
    if (m_self.get() != nullptr && m_self->HasAttribute(AttrShieldRadius))
        r = m_self->GetAttribute(AttrShieldRadius).get_float();

    if (r <= 0.0 && m_self.get() != nullptr) {
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
                "SELECT COALESCE(valueInt, valueFloat) FROM dgmTypeAttributes"
                " WHERE typeID = %u AND attributeID = %u",
                m_self->typeID(), (uint16)AttrShieldRadius)) {
            DBResultRow row;
            if (res.GetRow(row) && !row.IsNull(0))
                r = row.GetDouble(0);
        }
    }

    if (r <= 0.0)
        r = 20000.0;    // sane fallback so the sphere is never zero-sized
    return r;
}

void TowerSE::CreateForceField()
{
    if (m_hasShield)
        return;
    // create and add force field to tower
    // NOTE: the 5th arg must be a NAME — passing m_ownerID here matched the
    // quantity ctor, so the field item had an empty name and its bracket
    // killed the client's right-click menu near the POS.
    ItemData idata(EVEDB::invTypes::ForceField, m_corpID, m_system->GetID(), flagNone, "Force Field", GetPosition());
    InventoryItemRef ifRef = sItemFactory.SpawnItem(idata);
    if (ifRef.get() == nullptr)
        return;  // we'll get over it
    double shieldRadius = GetShieldRadius();
    ifRef->SetPosition(GetPosition());
    ifRef->SetAttribute(AttrRadius, shieldRadius, false);
    ifRef->SaveItem();
    FactionData data = FactionData();
        data.allianceID = m_allyID;
        data.corporationID = m_corpID;
        data.factionID = m_warID;
        data.ownerID = m_ownerID;
    FieldSE* iSE = new FieldSE(ifRef, m_services, m_system, data);
    // the client draws the sphere from the ball radius — the ForceField type's
    // own radius is ~0, so the field must carry the tower's shield radius
    iSE->SetRadius(shieldRadius);
    // set shield harmonic to tower harmonic
    iSE->SetHarmonic(m_harmonic);
    m_system->AddEntity(iSE);
    m_pShieldSE = iSE;
    m_hasShield = true;

    // Delivery is now identical to POS modules: plain RIGID bubble ball via the
    // normal SendAddBalls path on bubble entry (no AddBallExclusive - both the
    // one-shot full-state and the periodic re-announce variants of it broke the
    // client's destiny stream and took the whole grid down).

    _log(POS__MESSAGE, "TowerSE::CreateForceField() - %s(%u): field %u created, radius %.0f m, harmonic %i, pos(%.0f,%.0f,%.0f), bubble %u.",
         GetName(), m_self->itemID(), ifRef->itemID(), shieldRadius, m_harmonic,
         iSE->GetPosition().x, iSE->GetPosition().y, iSE->GetPosition().z,
         (iSE->SysBubble() != nullptr ? iSE->SysBubble()->GetID() : 0));
}

// Force field access (EVE): the owning corp's ships, the owning alliance's ships
// (when allowAlliance is set) and any ship that supplied the tower password may
// cross into the shield; everyone else is stopped at the barrier by
// DestinyManager::ProcessState().
bool TowerSE::CanEnterField(SystemEntity* se)
{
    if (se == nullptr)
        return false;

    // pilots carry the authoritative char/corp/alliance ids
    Client* pc = se->HasPilot() ? se->GetPilot() : nullptr;
    uint32 ownerID = (pc != nullptr) ? pc->GetCharacterID() : se->GetOwnerID();
    uint32 corpID  = (pc != nullptr) ? pc->GetCorporationID() : se->GetCorporationID();
    int32  allyID  = (pc != nullptr) ? pc->GetAllianceID() : se->GetAllianceID();

    if (ownerID != 0 && ownerID == m_ownerID)
        return true;
    if (corpID != 0 && corpID == m_corpID)
        return true;    // the tower's corp — its owners
    if (m_tdata.allowAlliance && allyID != 0 && allyID == m_allyID)
        return true;

    // password supplied through the ship's force-field password field
    if (!m_tdata.password.empty() && se->IsShipSE()) {
        std::string pass = se->GetShipSE()->GetTowerPassword();
        if (!pass.empty() && pass == m_tdata.password)
            return true;
    }
    return false;
}

// Recompute the tower's shield resonances from ONLINE Shield Hardening Arrays
// (group 444) inside the field. Each online hardener adds its resistance bonus
// (attrs 1489-1492), lowering the matching shield damage resonance (271-274) —
// i.e. raising the tower's shield resistances. Idempotent: the previously
// applied bonus is undone before the new one is applied, so this can run on
// every module online/offline without drifting the base value.
void TowerSE::RegisterAggressor(uint32 charID)
{
    if (charID == 0)
        return;
    m_aggressors[charID] = GetFileTimeNow() + (int64)EvE::Time::Minute * 15;
}

bool TowerSE::IsAggressor(uint32 charID)
{
    auto it = m_aggressors.find(charID);
    if (it == m_aggressors.end())
        return false;
    if (it->second < GetFileTimeNow()) {
        m_aggressors.erase(it);
        return false;
    }
    return true;
}

uint32 TowerSE::GetRecentAggressor()
{
    int64 now = GetFileTimeNow();
    uint32 best = 0;
    int64 bestExp = 0;
    for (auto it = m_aggressors.begin(); it != m_aggressors.end(); ) {
        if (it->second < now) {
            it = m_aggressors.erase(it);
            continue;
        }
        if (it->second > bestExp) {
            bestExp = it->second;
            best = it->first;
        }
        ++it;
    }
    return best;
}

void TowerSE::ApplyHardeners()
{
    // Crucible hardeners store their resist as a damage-resonance MULTIPLIER on
    // the module (em=133, explosive=132, kinetic=131, thermal=130) — e.g. a
    // Ballistic Deflection Array has kineticDamageResonanceMultiplier = 0.75
    // (25% kinetic resist). The old code read the 1489-1492 "resistance bonus"
    // attributes, which Crucible rows do not carry — so hardeners did nothing.
    static const EveAttrEnum s_resonance[4] = {
        AttrShieldEmDamageResonance, AttrShieldExplosiveDamageResonance,
        AttrShieldKineticDamageResonance, AttrShieldThermalDamageResonance };
    static const EveAttrEnum s_mult[4] = {
        AttrEmDamageResonanceMultiplier, AttrExplosiveDamageResonanceMultiplier,
        AttrKineticDamageResonanceMultiplier, AttrThermalDamageResonanceMultiplier };

    if (m_system == nullptr)
        return;

    double radius = GetShieldRadius();
    GPoint tp = GetPosition();

    double mult[4] = { 1.0, 1.0, 1.0, 1.0 };
    bool have[4] = { false, false, false, false };
    for (auto& [id, se] : m_system->GetEntities()) {
        if (se == nullptr || se == this)
            continue;
        StructureSE* mod = se->GetPOSSE();
        if (mod == nullptr || mod == this)
            continue;
        if (mod->IsTowerSE())
            continue;
        if (mod->GetSelf().get() == nullptr)
            continue;
        if (mod->GetSelf()->ownerID() != m_self->ownerID())
            continue;
        if (mod->GetSelf()->groupID() != EVEDB::invGroups::Shield_Hardening_Array)
            continue;
        if (mod->GetState() < EVEPOS::StructureState::Online)
            continue;   // offline hardeners do nothing
        if (tp.distance(mod->GetPosition()) > radius)
            continue;   // hardener must sit inside the field it protects

        for (int i = 0; i < 4; ++i) {
            if (!mod->GetSelf()->HasAttribute(s_mult[i]))
                continue;
            double m = mod->GetSelf()->GetAttribute(s_mult[i]).get_float();
            if (m <= 0.0 || m >= 1.0)
                continue;   // 1.0 = no effect (and >1 would be a weakness)
            mult[i] *= m;   // multiplicative stacking
            have[i] = true;
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (!have[i])
            mult[i] = 1.0;
        if (mult[i] < 0.1)
            mult[i] = 0.1;   // 90% resist cap
        double cur = m_self->GetAttribute(s_resonance[i]).get_float();
        double prev = (m_hardenerApplied[i] > 0.0001f) ? (double)m_hardenerApplied[i] : 1.0;
        double base = cur / prev;            // undo the previous pass
        m_self->SetAttribute(s_resonance[i], (float)(base * mult[i]));
        m_hardenerApplied[i] = (float)mult[i];
    }
}

PyDict* TowerSE::MakeSlimItem()
{
    _log(SE__SLIMITEM, "MakeSlimItem for TowerSE %u", m_self->itemID());
    _log(POS__SLIMITEM, "MakeSlimItem for TowerSE %u", m_self->itemID());

    PyDict *slim = new PyDict();
    slim->SetItemString("name",                     new PyString(m_self->itemName()));
    slim->SetItemString("itemID",                   new PyLong(m_self->itemID()));
    slim->SetItemString("typeID",                   new PyInt(m_self->typeID()));
    slim->SetItemString("ownerID",                  new PyInt(m_ownerID));
    slim->SetItemString("corpID",                   IsCorp(m_corpID) ? new PyInt(m_corpID) : PyStatic.NewNone());
    slim->SetItemString("allianceID",               IsAlliance(m_allyID) ? new PyInt(m_allyID) : PyStatic.NewNone());
    slim->SetItemString("warFactionID",             IsFaction(m_warID) ? new PyInt(m_warID) : PyStatic.NewNone());
    slim->SetItemString("posTimestamp",             new PyLong(m_data.timestamp));
    slim->SetItemString("posState",                 new PyInt(m_data.state));
    slim->SetItemString("incapacitated",            new PyInt((m_data.state == EVEPOS::StructureState::Incapacitated) ? 1 : 0));
    slim->SetItemString("posDelayTime",             new PyInt(m_delayTime));
    slim->SetItemString("controllerID",             (m_controllerID > 0) ? new PyInt(m_controllerID) : PyStatic.NewNone());

    if (is_log_enabled(POS__SLIMITEM)) {
        _log( POS__SLIMITEM, "TowerSE::MakeSlimItem() - %s(%u)", GetName(), GetID());
        slim->Dump(POS__SLIMITEM, "     ");
    }
    return slim;
}
/*
                      [PyString "OnSlimItemChange"]
                      [PyTuple 2 items]
                        [PyIntegerVar 1006120578679]
                        [PyObjectData Name: foo.SlimItem]
                          [PyDict 11 kvp]
                            [PyString "itemID"]
                            [PyIntegerVar 1006120578679]
                            [PyString "typeID"]
                            [PyInt 20060]               <<--  Amarr Control Tower Small
                            [PyString "name"]
                            [PyString "Pix0r monto la torre de al lado"]
                            [PyString "incapacitated"]
                            [PyFloat 0]
                            [PyString "posTimestamp"]
                            [PyIntegerVar 129773067243437304]
                            [PyString "posState"]
                            [PyInt 4]
                            [PyString "warFactionID"]
                            [PyNone]
                            [PyString "allianceID"]
                            [PyInt 99001691]
                            [PyString "corpID"]
                            [PyInt 717154310]
                            [PyString "ownerID"]
                            [PyInt 717154310]
                            [PyString "nameID"]
                            [PyNone]
                    */

/*
 *    def IsShipInRangeOfStructureControlTower(self, shipID, structureID):
 *        """
 *        Returns True if structureID is associated with a control tower (or is itself a tower), and shipID is in range of that tower.
 *        'In range of the tower' means within of the force-field radius.
 *        This function should mirror the behaviour of the equivalent server-side function in park.py
 *        """
 *        structureSlim = self.slimItems.get(structureID)
 *        if structureSlim is None:
 *            return False
 *        controlTowerID = None
 *        if structureSlim.groupID == const.groupControlTower:
 *            controlTowerID = structureID
 *        elif structureSlim.controlTowerID is not None:
 *            controlTowerID = structureSlim.controlTowerID
 *        if controlTowerID is None:
 *            return False
 *        towerSlim = self.slimItems.get(controlTowerID)
 *        if towerSlim is None:
 *            return False
 *        towerShieldRadius = self.broker.godma.GetStateManager().GetType(towerSlim.typeID).shieldRadius
 *        return self.GetCenterDist(controlTowerID, shipID) < towerShieldRadius
 */