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

            // Apply immediate security penalty
            double penalty = -6.0 * systemSecRating;
            m_client->GetChar()->secStatusChange(penalty);
        }

        // Start CONCORD response: 5 second delay, then ship destruction
        m_concordTimer.Start(5000);
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

    // CONCORD: zero HP and trigger destruction via Damage (self as source, no weapon).
    // Uses the NPC constructor signature to avoid weaponRef dereference.
    ShipRef ship = m_client->GetShip();
    if (ship.get() == nullptr) {
        sLog.Debug("CrimeWatch", "ApplyConcordPenalty() - %s(%u) no ship item, skipping.",
            m_client->GetName(), m_client->GetCharacterID());
        return;
    }

    ship->SetAttribute(AttrShieldCharge, 0.0);
    ship->SetAttribute(AttrHP, 0.0);
    // Armor HP uses AttrArmorDamage to represent damage taken vs max AttrArmorHP
    double armorMax = ship->GetAttribute(AttrArmorHP).get_float();
    ship->SetAttribute(AttrArmorDamage, armorMax);

    // Trigger fatal blow - this calls the NPC damage path (no weapon)
    Damage d(m_client->GetShipSE(), 1.0f, 0);
    shipSE->ApplyDamage(d);

    sLog.Log("CrimeWatch", "CONCORD destroyed %s(%u).",
        m_client->GetName(), m_client->GetCharacterID());

    m_client->SendNotifyMsg("CONCORD has destroyed your ship.");
}
