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

// Sentry gun typeIDs
static const uint32 SENTRY_TYPEID = 3740; // Caldari Sentry Gun I

// Sentry gun count by system sec
static uint32 GetSentryCount(float systemSecRating) {
    if (systemSecRating >= 0.5f) return 2;  // Highsec
    if (systemSecRating > 0.0f) return 3;   // Lowsec
    return 4; // Nullsec
}

// Sentry gun damage (100 DPS, split across 4 types)
static const float SENTRY_DPS = 100.0f;

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
  m_sentryDamageTimer(0)
{
    m_aggressionTimer.Disable();
    m_criminalTimer.Disable();
    m_weaponTimer.Disable();
    m_concordTimer.Disable();
    m_concordDamageTimer.Disable();
    m_sentryDamageTimer.Disable();
}

CrimeWatch::~CrimeWatch()
{
    ClearConcordShips();
    ClearSentryGuns();
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

    // Sentry gun DPS: 100 damage every second
    if (m_sentryDamageTimer.Enabled() and m_sentryDamageTimer.Check()) {
        ApplySentryDamage();
        m_sentryDamageTimer.Start(1000);
    }

    // Cleanup sentry guns when aggression expires
    if (!m_aggressionTimer.Enabled() and !m_sentryGuns.empty()) {
        ClearSentryGuns();
    }

    if (m_aggressionTimer.Enabled()) m_aggressionTimer.Check();
    if (m_criminalTimer.Enabled()) m_criminalTimer.Check();
    if (m_weaponTimer.Enabled()) m_weaponTimer.Check();
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
}

void CrimeWatch::OnLooting()
{
    if (!sConfig.crime.Enabled) return;
    // Suspect flag: weapon timer + aggression timer for 15 min
    m_weaponTimer.Start(60000);
    m_aggressionTimer.Start(sConfig.crime.AggFlagTime * 1000);
    // -0.2 security penalty (already applied in InventoryBound::Add)
}

void CrimeWatch::OnAggression(Client* pTarget, float systemSecRating)
{
    if (pTarget == nullptr or pTarget == m_client)
        return;
    if (!sConfig.crime.Enabled)
        return;

    // EVE: aggression timer = 15 minutes after PvP aggression
    // Prevents docking and jumping
    m_aggressionTimer.Start(sConfig.crime.AggFlagTime * 1000);

    // EVE: CONCORD only in highsec, skip if target is outlaw
    bool targetIsOutlaw = (pTarget->GetCrimeWatch() != nullptr && pTarget->GetCrimeWatch()->IsOutlaw());
    if (!targetIsOutlaw && systemSecRating >= 0.5f && !m_concordTimer.Enabled() && !m_concordDamageTimer.Enabled()) {
        if (!m_criminalTimer.Enabled()) {
            m_criminalTimer.Start(sConfig.crime.CrimFlagTime * 1000);
            m_client->SendNotifyMsg("CONCORD response initiated. You have been flagged as a criminal.");
        }

        // -0.2 penalty for aggression
        m_client->GetChar()->secStatusChange(-0.2f);

        uint32 delay = 19000;
        if (systemSecRating >= 0.9f) delay = 6000;
        else if (systemSecRating >= 0.8f) delay = 7000;
        else if (systemSecRating >= 0.7f) delay = 10000;
        else if (systemSecRating >= 0.6f) delay = 14000;
        m_concordTimer.Start(delay);
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

void CrimeWatch::ClearSentryGuns()
{
    for (auto pNPC : m_sentryGuns) {
        if (pNPC != nullptr) {
            if (pNPC->SysBubble() != nullptr)
                pNPC->SysBubble()->Remove(pNPC);
            pNPC->SystemMgr()->RemoveEntity(pNPC);
        }
    }
    m_sentryGuns.clear();
    m_sentryDamageTimer.Disable();
}

void CrimeWatch::SpawnSentryGuns(uint32 count)
{
    if (!m_client->IsInSpace()) return;
    if (m_client->GetShipSE() == nullptr) return;
    SystemManager* sysMgr = m_client->SystemMgr();
    if (sysMgr == nullptr) return;

    ClearSentryGuns();
    GPoint pos = m_client->GetShipSE()->GetPosition();
    uint32 sysID = sysMgr->GetID();

    FactionData faction;
    faction.allianceID = 0;
    faction.factionID = 500021;
    faction.ownerID = 1000125;
    faction.corporationID = 1000125;

    for (uint32 i = 0; i < count; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "Sentry Gun #%u", i + 1);
        ItemData itemData(SENTRY_TYPEID, faction.ownerID, sysID, flagNone, name, pos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
        if (iRef.get() == nullptr) continue;

        NPC* pNPC = new NPC(iRef, sysMgr->GetServiceMgr(), sysMgr, faction);
        if (pNPC == nullptr || !pNPC->Load()) {
            if (pNPC) delete pNPC;
            continue;
        }
        GPoint gunPos = pos;
        gunPos.x += (float)(MakeRandomInt(-5000, 5000));
        gunPos.z += (float)(MakeRandomInt(-5000, 5000));
        pNPC->DestinyMgr()->SetPosition(gunPos);
        sysMgr->AddNPC(pNPC);
        m_sentryGuns.push_back(pNPC);
    }
}

void CrimeWatch::ApplySentryDamage()
{
    if (!m_client->IsInSpace()) return;
    if (!m_aggressionTimer.Enabled()) return;
    SystemEntity* shipSE = m_client->GetShipSE();
    if (shipSE == nullptr) return;
    ShipItemRef ship = m_client->GetShip();
    if (ship.get() == nullptr) return;

    // 100 DPS split across 4 damage types (25 each)
    float dmg = SENTRY_DPS * 0.25f;
    Damage d(m_client->GetShipSE(), InventoryItemRef(ship.get()), dmg, dmg, dmg, dmg, 1.0f, 0);
    shipSE->ApplyDamage(d);
}
