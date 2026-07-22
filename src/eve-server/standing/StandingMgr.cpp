
 /**
  * @name StandingMgr.cpp
  *   memory object caching system for managing and saving standings data
  *   methods and functions relating to manipulation of standings
  *
  * @Author:        Allan
  * @date:      14 Novemeber 2018
  *
  */


#include "StandingMgr.h"

#include "EntityList.h"
#include "EVE_Mail.h"

/*
 * STANDING__ERROR
 * STANDING__WARNING
 * STANDING__MESSAGE
 * STANDING__DEBUG
 * STANDING__INFO
 * STANDING__TRACE
 * STANDING__DUMP
 * STANDING__RSPDUMP
 */


StandingMgr::StandingMgr()
: m_factionStandings(nullptr),
  m_decayTimer(3600000), // 1 hour default
  m_researchTimer(3600000) // 1 hour default
{

}

StandingMgr::~StandingMgr()
{

}

void StandingMgr::Clear()
{
    PySafeDecRef(m_factionStandings);
}

int StandingMgr::Initialize()
{
    Populate();
    SetDecayTimer();
    sLog.Blue("      StandingMgr", "Standings Manager Initialized.");
    return 1;
}

void StandingMgr::GetInfo()
{

}

void StandingMgr::Populate()
{
    m_factionStandings = StandingDB::GetFactionStandings();
    if (m_factionStandings == nullptr)
        sLog.Error("      StandingMgr", "m_factionStandings is null");

}

void StandingMgr::UpdateStandings(uint32 fromID, uint32 toID, uint16 eventType, double amount, std::string msg)
{
    StandingDB::UpdateStanding(fromID, toID, amount);
    StandingDB::SaveStandingChanges(fromID, toID, eventType, amount, msg);

    // Send notification for meaningful standing changes (skip decay, combat, property damage)
    switch (eventType) {
        case Standings::Decay:
        case Standings::CombatAggression:
        case Standings::CombatShipKill:
        case Standings::CombatPodKill:
        case Standings::CombatOther:
        case Standings::CombatAssistance:
        case Standings::PropertyDamage:
        case Standings::CombatShipKillOwnFaction:
        case Standings::CombatPodKillOwnFaction:
        case Standings::CombatAggressionOwnFaction:
        case Standings::CombatAssistanceOwnFaction:
        case Standings::CombatOtherOwnFaction:
            break; // skip — these are too frequent
        default: {
            PyDict* data = new PyDict();
            data->SetItemString("fromID", new PyInt(fromID));
            data->SetItemString("toID", new PyInt(toID));
            data->SetItemString("amount", new PyFloat(amount));
            data->SetItemString("eventType", new PyInt(eventType));
            if (!msg.empty())
                data->SetItemString("msg", new PyString(msg));
            sEntityList.CreateNotification(toID, Notify::Types::ContactEdit, fromID, data);
        }
    }
}

void StandingMgr::SetDecayTimer()
{
    m_decayTimer.Start(3600000, true); // 1 hour
}

void StandingMgr::SetResearchTimer()
{
    m_researchTimer.Start(3600000, true); // 1 hour
}

void StandingMgr::ProcessResearch()
{
    if (!m_researchTimer.Check(false))
        return;

    _log(STANDING__INFO, "StandingMgr::ProcessResearch() - processing research point accumulation.");

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT characterID, agentID, points, pointsPerDay, lastUpdate"
        " FROM chrResearch WHERE pointsPerDay > 0 AND lastUpdate > 0");

    DBResultRow row;
    while (res.GetRow(row)) {
        double points = row.GetDouble(2);
        double ppd = row.GetDouble(3);
        int64 lastUpdate = row.GetInt64(4);

        double hoursElapsed = (double)(GetFileTimeNow() - lastUpdate) / (double)EvE::Time::Hour;
        if (hoursElapsed < 1.0)
            continue;

        double earned = ppd / 24.0 * hoursElapsed;
        points += earned;

        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE chrResearch SET points = %.2f, lastUpdate = %lli"
            " WHERE characterID = %u AND agentID = %u",
            points, (int64)GetFileTimeNow(), row.GetUInt(0), row.GetUInt(1));
    }

    _log(STANDING__INFO, "StandingMgr::ProcessResearch() - research point accumulation complete.");
    SetResearchTimer();
}

void StandingMgr::ProcessDecay()
{
    if (!m_decayTimer.Check(false))
        return;

    _log(STANDING__INFO, "StandingMgr::ProcessDecay() - processing standings decay.");

    // decay formula: new_standing = standing * (1 - decay_rate)^hours_since_modification
    // decay_rate = 0.02 (2% per 30 days = ~0.0028% per hour)
    // skip standings modified less than 24h ago
    // skip characters not logged in for more than 60 days

    DBQueryResult res;
    sDatabase.RunQuery(res,
        "SELECT r.fromID, r.toID, r.standing, r.lastModified, c.online"
        " FROM repStandings r"
        " LEFT JOIN chrCharacters c ON c.characterID = r.toID"
        " WHERE r.toID >= 90000000"      // player characters (not NPCs)
        "   AND r.lastModified > 0"
        "   AND r.lastModified < %lli",   // only rows modified before now
        (int64)(GetFileTimeNow() - EvE::Time::Day)); // at least 24h old

    DBResultRow row;
    while (res.GetRow(row)) {
        uint32 fromID = row.GetUInt(0);
        uint32 toID = row.GetUInt(1);
        float standing = row.GetFloat(2);
        int64 lastModified = row.GetInt64(3);
        bool isOnline = row.GetInt(4) > 0;

        // skip inactive players (not logged in for 60+ days)
        if (!isOnline) {
            // check last login time
            DBQueryResult loginRes;
            sDatabase.RunQuery(loginRes,
                "SELECT lastLogin FROM account WHERE accountID ="
                " (SELECT accountID FROM chrCharacters WHERE characterID = %u)", toID);
            DBResultRow loginRow;
            if (loginRes.GetRow(loginRow)) {
                double lastLogin = loginRow.GetDouble(0);
                if ((GetFileTimeNow() - lastLogin) > (EvE::Time::Day * 60)) {
                    // skip — player has been inactive too long
                    continue;
                }
            }
        }

        if (fabs(standing) < 0.01f)
            continue;

        // calculate hours since last modified
        double hoursElapsed = (double)(GetFileTimeNow() - lastModified) / (double)EvE::Time::Hour;
        if (hoursElapsed < 24.0)
            continue; // only decay standings older than 24h

        // decay rate: ~2% toward 0 per 30 days = ~0.0000278 per hour
        // new = old * (1 - 0.0000278)^hours
        double decayFactor = pow(1.0 - 0.0000278, hoursElapsed);
        float newStanding = standing * decayFactor;

        // clamp: if standing is very close to 0, set to 0
        if (fabs(newStanding) < 0.001f)
            newStanding = 0.0f;

        // only update if change is significant
        float delta = newStanding - standing;
        if (fabs(delta) < 0.001f)
            continue;

        // save new standing
        DBerror err;
        sDatabase.RunQuery(err,
            "UPDATE repStandings SET standing = %f, lastModified = %lli"
            " WHERE fromID = %u AND toID = %u",
            newStanding, (int64)GetFileTimeNow(), fromID, toID);

        // log change
        StandingDB::SaveStandingChanges(fromID, toID, Standings::Decay, delta,
            "Standing decay.");

        _log(STANDING__TRACE, "StandingMgr::ProcessDecay() - %u -> %u: %.4f -> %.4f (%.4f)",
                fromID, toID, standing, newStanding, delta);
    }

    _log(STANDING__INFO, "StandingMgr::ProcessDecay() - decay complete.");

    // re-arm timer
    SetDecayTimer();
}

