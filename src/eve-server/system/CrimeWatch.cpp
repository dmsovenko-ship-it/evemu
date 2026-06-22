#include "CrimeWatch.h"
#include "Client.h"
#include "EVEServerConfig.h"
#include "ship/Ship.h"
#include "system/SystemManager.h"
#include "system/Damage.h"

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
        ApplyConcordPenalty();
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

#include <cmath>

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

    // CONCORD applies massive damage split across all 4 damage types.
    // Using the explicit-damage constructor so weaponRef is stored but NOT
    // dereferenced (the NPC constructor reads from weaponRef, the explicit one doesn't).
    ShipItemRef ship = m_client->GetShip();
    if (ship.get() == nullptr) {
        sLog.Debug("CrimeWatch", "ApplyConcordPenalty() - %s(%u) no ship item, skipping.",
            m_client->GetName(), m_client->GetCharacterID());
        return;
    }

    double totalHP = ship->GetAttribute(AttrShieldCapacity).get_float()
                   + ship->GetAttribute(AttrArmorHP).get_float()
                   + ship->GetAttribute(AttrHP).get_float();
    double concordDmg = totalHP * 25.0; // 2500% per type × 4 types = 10000% total

    // Notify: CONCORD destruction  
    m_client->SendNotifyMsg("CONCORD destroyed your %s in %s.",
        ship->itemName(), m_client->SystemMgr()->GetName());

    // Send killmail from EVE System (senderID=1)
    LSCService* lsc = m_client->GetLSC();
    if (lsc != nullptr) {
        std::vector<int32> recipients;
        recipients.push_back(m_client->GetCharacterID());
        std::string subject = "CONCORD Destruction Notice";
        std::string body = "Your ship ";
        body += ship->itemName();
        body += " was destroyed by CONCORD forces in ";
        body += m_client->SystemMgr()->GetName();
        body += ".\n\nYour vessel engaged in illegal activity in a high-security system. "
               "CONCORD has enforced the standard security protocol.";
        lsc->SendMail(1, recipients, subject, body);
    }

    Damage d(shipSE, InventoryItemRef(ship.get()), concordDmg, concordDmg, concordDmg, concordDmg, 1.0f, 0);
    shipSE->ApplyDamage(d);

    sLog.Log("CrimeWatch", "CONCORD destroyed %s(%u).",
        m_client->GetName(), m_client->GetCharacterID());

    m_client->SendNotifyMsg("CONCORD has destroyed your ship.");
}
