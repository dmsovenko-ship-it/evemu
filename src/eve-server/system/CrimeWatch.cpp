#include "CrimeWatch.h"
#include "Client.h"
#include "EVEServerConfig.h"
#include "ship/Ship.h"
#include "system/SystemManager.h"
#include "system/Damage.h"
#include "mail/MailDB.h"
#include "npc/NPC.h"
#include "inventory/ItemFactory.h"
#include "system/SystemBubble.h"
#include "standing/KillRightDB.h"

// Character attribute IDs for HUD timers (from EVE static data)
static const uint16 ATTR_AGGRESSION_TIMER = 258;
static const uint16 ATTR_WEAPON_TIMER = 261;
static const uint16 ATTR_NPC_TIMER = 264;

// CONCORD ship typeIDs from EVE static data
static const uint32 CONCORD_TYPEIDS[] = {
    1912, // Concord Police Battleship
    1914, // Concord Special Ops Battleship
    1918, // Concord Army Battleship
};

CrimeWatch::CrimeWatch(Client* pClient)
: m_client(pClient),
  m_aggressionTimer(sConfig.crime.AggFlagTime * 1000),
  m_criminalTimer(sConfig.crime.CrimFlagTime * 1000),
  m_weaponTimer(sConfig.crime.WeaponFlagTime * 1000),
  m_concordTimer(0),
  m_concordDamageTimer(0),
  m_limitedEngagementTimer(900000) // 15 minutes
{
    m_aggressionTimer.Disable();
    m_criminalTimer.Disable();
    m_weaponTimer.Disable();
    m_concordTimer.Disable();
    m_concordDamageTimer.Disable();
    m_limitedEngagementTimer.Disable();
}

bool CrimeWatch::IsOutlaw() const
{
    return m_client->GetSecurityRating() <= -5.0f;
}

CrimeWatch::~CrimeWatch()
{
    ClearConcordShips();
}

void CrimeWatch::Process()
{
    if (m_concordTimer.Enabled() and m_concordTimer.Check()) {
        m_concordTimer.Disable();
        SpawnConcordShips();
        m_concordDamageTimer.Start(4000);
    }
    if (m_concordDamageTimer.Enabled() and m_concordDamageTimer.Check()) {
        m_concordDamageTimer.Disable();
        ApplyConcordPenalty();
        ClearConcordShips();
    }

    if (m_aggressionTimer.Enabled() and m_aggressionTimer.Check(false)) {
        m_aggressionTimer.Disable();
        if (m_client->GetChar())
            m_client->GetChar()->SetAttribute(ATTR_AGGRESSION_TIMER, int64(0), true);
    }
    if (m_criminalTimer.Enabled()) m_criminalTimer.Check();
    if (m_weaponTimer.Enabled() and m_weaponTimer.Check(false)) {
        m_weaponTimer.Disable();
        if (m_client->GetChar())
            m_client->GetChar()->SetAttribute(ATTR_WEAPON_TIMER, int64(0), true);
    }
    if (m_limitedEngagementTimer.Enabled()) m_limitedEngagementTimer.Check();
}

void CrimeWatch::ClearConcordShips()
{
    for (auto pNPC : m_concordShips) {
        if (pNPC != nullptr) {
            if (pNPC->SysBubble() != nullptr)
                pNPC->SysBubble()->Remove(pNPC);
            pNPC->SystemMgr()->RemoveEntity(pNPC);
        }
    }
    m_concordShips.clear();
}

void CrimeWatch::OnWeaponFired()
{
    // EVE: weapon timer = 60 seconds after ANY weapon use (NPC or player)
    // Prevents docking and jumping during this time
    m_weaponTimer.Start(60000);
    if (m_client->GetChar()) {
        int64 endTime = static_cast<int64>(GetFileTimeNow()) + 60LL * EvE::Time::Second;
        m_client->GetChar()->SetAttribute(ATTR_WEAPON_TIMER, int64(endTime), true);
    }
}

void CrimeWatch::OnLooting()
{
    if (!sConfig.crime.Enabled) return;
    // Suspect flag: weapon timer + aggression timer for 15 min
    m_weaponTimer.Start(60000);
    m_aggressionTimer.Start(sConfig.crime.AggFlagTime * 1000);
    if (m_client->GetChar()) {
        int64 now = static_cast<int64>(GetFileTimeNow());
        m_client->GetChar()->SetAttribute(ATTR_WEAPON_TIMER, int64(now + 60LL * EvE::Time::Second), true);
        m_client->GetChar()->SetAttribute(ATTR_AGGRESSION_TIMER, int64(now + sConfig.crime.AggFlagTime * EvE::Time::Second), true);
    }
    // -0.2 security penalty (already applied in InventoryBound::Add)
}

void CrimeWatch::OnAggression(Client* pTarget, float systemSecRating)
{
    if (pTarget == nullptr or pTarget == m_client)
        return;
    if (!sConfig.crime.Enabled)
        return;

    // Check legal attack conditions FIRST — before aggression timer or any flags
    // (sentries check IsAggressed/IsCriminal, so these must be checked first)
    {
        // Check if target has Limited Engagement (Kill Right activated on them)
        if (pTarget->GetCrimeWatch() != nullptr && pTarget->GetCrimeWatch()->IsLimitedEngagement())
            return;
        // Check if target is permaflashy (outlaw)
        if (pTarget->GetCrimeWatch() != nullptr && pTarget->GetCrimeWatch()->IsOutlaw())
            return;
        // Check if attacker has a free Kill Right against the target
        DBQueryResult krRes;
        uint32 attackerID = m_client->GetCharacterID();
        uint32 victimID = pTarget->GetCharacterID();
        if (sDatabase.RunQuery(krRes,
            " SELECT rightID, price FROM chrKillRights "
            " WHERE ownerID = %u AND targetID = %u AND used = 0 AND expiryDate > %lli",
            victimID, attackerID, static_cast<int64>(GetFileTimeNow())))
        {
            DBResultRow krRow;
            if (krRes.GetRow(krRow)) {
                int64 price = krRow.GetInt64(1);
                if (price == 0) {
                    // free kill right: set limited engagement, no flags at all
                    if (pTarget->GetCrimeWatch() != nullptr)
                        pTarget->GetCrimeWatch()->SetLimitedEngagement();
                    return;
                }
            }
        }
    }

    // EVE: aggression timer = 15 minutes after PvP aggression
    m_aggressionTimer.Start(sConfig.crime.AggFlagTime * 1000);
    if (m_client->GetChar()) {
        int64 endTime = static_cast<int64>(GetFileTimeNow()) + sConfig.crime.AggFlagTime * EvE::Time::Second;
        m_client->GetChar()->SetAttribute(ATTR_AGGRESSION_TIMER, int64(endTime), true);
    }

    // Highsec: criminal act + CONCORD response + kill right grant
    if (systemSecRating >= 0.5f) {
        if (!m_criminalTimer.Enabled()) {
            m_criminalTimer.Start(sConfig.crime.CrimFlagTime * 1000);
            m_client->SendNotifyMsg("CONCORD response initiated. You have been flagged as a criminal.");
        }

        // grant Kill Right to victim
        KillRightDB kdb;
        if (kdb.GrantKillRight(pTarget->GetCharacterID(), m_client->GetCharacterID()) > 0)
            pTarget->SendNotifyMsg("Kill Right granted against %s for criminal aggression.", m_client->GetName());

        // -0.2 penalty for aggression
        m_client->GetChar()->secStatusChange(-0.2f);

        uint32 delay = 19000;
        if (systemSecRating >= 0.9f) delay = 6000;
        else if (systemSecRating >= 0.8f) delay = 7000;
        else if (systemSecRating >= 0.7f) delay = 10000;
        else if (systemSecRating >= 0.6f) delay = 14000;
        m_concordTimer.Start(delay);
    }
    // Lowsec: pod kill also grants kill right
    else if (systemSecRating > 0.0f && pTarget->InPod()) {
        KillRightDB kdb;
        if (kdb.GrantKillRight(pTarget->GetCharacterID(), m_client->GetCharacterID()) > 0)
            pTarget->SendNotifyMsg("Kill Right granted against %s for pod kill.", m_client->GetName());
    }
}

void CrimeWatch::SpawnConcordShips()
{
    if (!m_client->IsInSpace()) return;
    if (m_client->GetShipSE() == nullptr) return;

    SystemManager* sysMgr = m_client->SystemMgr();
    if (sysMgr == nullptr) return;

    GPoint criminalPos = m_client->GetShipSE()->GetPosition();
    uint32 criminalSysID = sysMgr->GetID();

    FactionData faction;
    faction.allianceID = 0;
    faction.factionID = 500021;
    faction.ownerID = 1000125;
    faction.corporationID = 1000125;

    // Spawn ships at criminal's position (no warp, just appear)
    uint32 numShips = 2;
    ClearConcordShips();
    for (uint32 i = 0; i < numShips; ++i) {
        uint32 typeID = CONCORD_TYPEIDS[MakeRandomInt(0, 2)];
        char name[64];
        snprintf(name, sizeof(name), "CONCORD #%u", i + 1);

        ItemData itemData(typeID, faction.ownerID, criminalSysID, flagNone, name, criminalPos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
        if (iRef.get() == nullptr) continue;

        NPC* pNPC = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, faction);
        if (pNPC == nullptr || !pNPC->Load()) {
            if (pNPC) delete pNPC;
            continue;
        }

        GPoint pos = criminalPos;
        pos.x += (float)(MakeRandomInt(-2000, 2000));
        pos.z += (float)(MakeRandomInt(-2000, 2000));
        pNPC->DestinyMgr()->SetPosition(pos);
        sysMgr->AddNPC(pNPC);
        m_concordShips.push_back(pNPC);
    }
}

void CrimeWatch::ApplyConcordPenalty()
{
    if (!m_client->IsInSpace()) return;
    SystemEntity* shipSE = m_client->GetShipSE();
    if (shipSE == nullptr) return;
    ShipItemRef ship = m_client->GetShip();
    if (ship.get() == nullptr) return;

    double totalHP = ship->GetAttribute(AttrShieldCapacity).get_float()
                   + ship->GetAttribute(AttrArmorHP).get_float()
                   + ship->GetAttribute(AttrHP).get_float();
    double concordDmg = totalHP * 25.0;

    m_client->SendNotifyMsg("CONCORD destroyed your %s in %s.",
        ship->itemName(), m_client->SystemMgr()->GetName());

    {
        MailDB mailDB;
        std::vector<int32> recipients;
        recipients.push_back(m_client->GetCharacterID());
        float secRating = m_client->SystemMgr()->GetSystemSecurityRating();
        char secStr[16];
        snprintf(secStr, sizeof(secStr), "%.1f", secRating);
        std::string body = "Kill Report\n============\n\n";
        body += "Victim: " + std::string(m_client->GetName()) + "\n";
        body += "Ship: " + std::string(ship->itemName()) + "\n";
        body += "System: " + std::string(m_client->SystemMgr()->GetName()) + " (" + secStr + ")\n";
        body += "Damage Taken: " + std::to_string((int)totalHP) + "\n";
        body += "Destroyed By: CONCORD Police Forces\n\n";
        body += "Your vessel engaged in illegal activity in a high-security system.\n";
        body += "CONCORD has enforced the standard security protocol.\n";
        body += "Security status penalty has been applied.";
        mailDB.SendMail(1, recipients, -1, -1, "CONCORD Kill Report", body, 0, 0);
    }

    Damage d(shipSE, InventoryItemRef(ship.get()), concordDmg, concordDmg, concordDmg, concordDmg, 1.0f, 0);
    shipSE->ApplyDamage(d);

    m_client->SendNotifyMsg("CONCORD has destroyed your ship.");
}

void CrimeWatch::SetLimitedEngagement()
{
    m_limitedEngagementTimer.Start(900000); // 15 minutes
    m_client->SendNotifyMsg("Limited Engagement active for 15 minutes. You are a legal target.");
}
