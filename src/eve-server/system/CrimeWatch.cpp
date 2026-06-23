#include "CrimeWatch.h"
#include "Client.h"
#include "EVEServerConfig.h"
#include "ship/Ship.h"
#include "system/SystemManager.h"
#include "system/Damage.h"
#include "mail/MailDB.h"
#include "npc/NPC.h"
#include "inventory/ItemFactory.h"

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
  m_concordTimer(0)
{
    m_aggressionTimer.Disable();
    m_criminalTimer.Disable();
    m_weaponTimer.Disable();
    m_concordTimer.Disable();
}

void CrimeWatch::Process()
{
    if (m_concordTimer.Enabled() and m_concordTimer.Check()) {
        m_concordTimer.Disable();
        SpawnConcordShips();
    }

    if (m_aggressionTimer.Enabled())
        m_aggressionTimer.Check();
    if (m_criminalTimer.Enabled())
        m_criminalTimer.Check();
    if (m_weaponTimer.Enabled())
        m_weaponTimer.Check();
}

void CrimeWatch::OnAggression(Client* pTarget, float systemSecRating)
{
    if (pTarget == nullptr or pTarget == m_client)
        return;
    if (!sConfig.crime.Enabled)
        return;

    // Start weapon timer (15s default)
    if (m_weaponTimer.Enabled())
        m_weaponTimer.Start(sConfig.crime.WeaponFlagTime * 1000);
    else
        m_weaponTimer.Start(sConfig.crime.WeaponFlagTime * 1000);

    // Start aggression timer (15 min default)
    if (!m_aggressionTimer.Enabled())
        m_aggressionTimer.Start(sConfig.crime.AggFlagTime * 1000);

    // High-sec (= true sec > 0.45) → criminal flag + CONCORD
    if (systemSecRating > 0.45f) {
        if (!m_criminalTimer.Enabled()) {
            m_criminalTimer.Start(sConfig.crime.CrimFlagTime * 1000);
            m_client->SendNotifyMsg("CONCORD response initiated. You have been flagged as a criminal.");
        }

        // -0.2 security penalty for aggression (before kill)
        m_client->GetChar()->secStatusChange(-0.2f);

        // CONCORD response time depends on system security
        uint32 concordDelay = 19000; // 0.5 = 19s
        if (systemSecRating >= 0.9f) concordDelay = 6000;
        else if (systemSecRating >= 0.8f) concordDelay = 7000;
        else if (systemSecRating >= 0.7f) concordDelay = 10000;
        else if (systemSecRating >= 0.6f) concordDelay = 14000;
        m_concordTimer.Start(concordDelay);
    }

    sLog.Debug("CrimeWatch", "OnAggression() - %s(%u) aggressed %s(%u) in sec=%.2f",
        m_client->GetName(), m_client->GetCharacterID(),
        pTarget->GetName(), pTarget->GetCharacterID(),
        systemSecRating);
}

void CrimeWatch::SpawnConcordShips()
{
    if (!m_client->IsInSpace()) return;
    if (m_client->GetShipSE() == nullptr) return;
    if (m_client->GetShip().get() == nullptr) return;

    SystemManager* sysMgr = m_client->SystemMgr();
    if (sysMgr == nullptr) return;

    GPoint criminalPos = m_client->GetShipSE()->GetPosition();
    uint32 criminalSysID = sysMgr->GetID();

    // Faction data for CONCORD
    FactionData faction;
    faction.allianceID = 0;
    faction.factionID = 500021; // CONCORD faction
    faction.ownerID = 1000125;  // CONCORD Bureau
    faction.corporationID = 1000125;

    // Spawn 2-3 CONCORD battleships
    uint32 numShips = 2 + (MakeRandomInt(0, 1) ? 1 : 0);
    for (uint32 i = 0; i < numShips; ++i) {
        // Pick a random CONCORD type
        uint32 typeID = CONCORD_TYPEIDS[MakeRandomInt(0, 2)];
        char name[64];
        snprintf(name, sizeof(name), "CONCORD Police #%u", m_client->GetCharacterID() + i);

        // Create the ship item in the system
        ItemData itemData(typeID, faction.ownerID, criminalSysID, flagNone, name, criminalPos);
        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
        if (iRef.get() == nullptr) {
            sLog.Error("CrimeWatch", "Failed to spawn CONCORD ship type %u", typeID);
            continue;
        }

        // Create NPC entity
        NPC* pNPC = new NPC(iRef, *sysMgr->GetServiceMgr(), sysMgr, faction);
        if (pNPC == nullptr || !pNPC->Load()) {
            if (pNPC) delete pNPC;
            continue;
        }

        // Position near the criminal with some spread
        GPoint pos = criminalPos;
        pos.x += (float)(MakeRandomInt(-500, 500));
        pos.y += (float)(MakeRandomInt(-500, 500));
        pos.z += (float)(MakeRandomInt(-500, 500));
        pNPC->DestinyMgr()->SetPosition(pos);

        // Add to system (appears in space)
        sysMgr->AddNPC(pNPC);

        // Warp to criminal's position
        pNPC->DestinyMgr()->WarpTo(criminalPos, 0);
    }

    // Apply damage after a brief delay via existing timer mechanism
    m_concordTimer.Start(3000);
}

void CrimeWatch::ApplyConcordPenalty()
{
    if (!m_client->IsInSpace()) {
        sLog.Debug("CrimeWatch", "ApplyConcordPenalty() - %s(%u) not in space, skipping.",
            m_client->GetName(), m_client->GetCharacterID());
        return;
    }
    SystemEntity* shipSE = m_client->GetShipSE();
    if (shipSE == nullptr) {
        sLog.Debug("CrimeWatch", "ApplyConcordPenalty() - %s(%u) no ship SE, skipping.",
            m_client->GetName(), m_client->GetCharacterID());
        return;
    }

    ShipItemRef ship = m_client->GetShip();
    if (ship.get() == nullptr) {
        sLog.Debug("CrimeWatch", "ApplyConcordPenalty() - %s(%u) no ship item, skipping.",
            m_client->GetName(), m_client->GetCharacterID());
        return;
    }

    double totalHP = ship->GetAttribute(AttrShieldCapacity).get_float()
                   + ship->GetAttribute(AttrArmorHP).get_float()
                   + ship->GetAttribute(AttrHP).get_float();
    double concordDmg = totalHP * 25.0;

    // Notify and mail
    m_client->SendNotifyMsg("CONCORD destroyed your %s in %s.",
        ship->itemName(), m_client->SystemMgr()->GetName());

    {
        MailDB mailDB;
        std::vector<int32> recipients;
        recipients.push_back(m_client->GetCharacterID());
        std::string body = "Your ship ";
        body += ship->itemName();
        body += " was destroyed by CONCORD forces in ";
        body += m_client->SystemMgr()->GetName();
        body += ".\n\nYour vessel engaged in illegal activity in a high-security system. "
               "CONCORD has enforced the standard security protocol.";
        mailDB.SendMail(1, recipients, -1, -1, "CONCORD Destruction Notice", body, 0, 0);
    }

    // Destroy the ship
    Damage d(shipSE, InventoryItemRef(ship.get()), concordDmg, concordDmg, concordDmg, concordDmg, 1.0f, 0);
    shipSE->ApplyDamage(d);

    sLog.Log("CrimeWatch", "CONCORD destroyed %s(%u).",
        m_client->GetName(), m_client->GetCharacterID());

    m_client->SendNotifyMsg("CONCORD has destroyed your ship.");
}
