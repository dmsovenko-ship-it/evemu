
 /**
  * @name AgentBound.cpp
  *   agent specific code
  *    removed from AgentMgrService.cpp
  *
  * @Author:        Allan
  * @date:      26 June 2018
  *
  */


/*
 * # Agent Logging:
 * AGENT__ERROR
 * AGENT__WARNING
 * AGENT__MESSAGE
 * AGENT__DEBUG
 * AGENT__INFO
 * AGENT__TRACE
 * AGENT__DUMP
 * AGENT__RSP_DUMP
 */

#include "eve-server.h"
#include <cstdio>
#include "EntityList.h"
#include "../../eve-common/EVE_Agent.h"
#include "../../eve-common/EVE_Mail.h"
#include "../../eve-common/EVE_Missions.h"
//#include "../../eve-common/EVE_Skills.h"
#include "../../eve-common/EVE_Standings.h"

#include "StaticDataMgr.h"
#include "map/MapData.h"
#include "account/AccountService.h"
#include "corporation/LPService.h"
#include "agents/Agent.h"
#include "agents/AgentBound.h"
#include "agents/AgentMgrService.h"
#include "station/Station.h"
#include "services/ServiceManager.h"
#include "missions/EpicArcMgr.h"
#include "missions/EncounterServer.h"
#include "missions/MissionDB.h"
#include "system/SystemManager.h"
#include "system/SystemEntity.h"
#include "system/cosmicMgrs/DungeonMgr.h"
#include "POD_containers.h"

AgentBound::AgentBound(EVEServiceManager& mgr, AgentMgrService& parent, Agent *agt) :
    EVEBoundObject(mgr, parent),
    m_agent(agt)
{
    this->Add("DoAction", &AgentBound::DoAction);
    this->Add("GetAgentLocationWrap", &AgentBound::GetAgentLocationWrap);
    this->Add("GetInfoServiceDetails", &AgentBound::GetInfoServiceDetails);
    this->Add("GetMissionBriefingInfo", &AgentBound::GetMissionBriefingInfo);
    this->Add("GetMissionObjectiveInfo", &AgentBound::GetMissionObjectiveInfo);
    this->Add("GetMissionKeywords", &AgentBound::GetMissionKeywords);
    this->Add("GetMissionJournalInfo", &AgentBound::GetMissionJournalInfo);
    this->Add("GetDungeonShipRestrictions", &AgentBound::GetDungeonShipRestrictions);
    this->Add("RemoveOfferFromJournal", &AgentBound::RemoveOfferFromJournal);
    this->Add("GetOfferJournalInfo", &AgentBound::GetOfferJournalInfo);
    this->Add("GetEntryPoint", &AgentBound::GetEntryPoint);
    this->Add("GotoLocation", &AgentBound::GotoLocation);
    this->Add("WarpToLocation", &AgentBound::WarpToLocation);
    this->Add("GetMyJournalDetails", &AgentBound::GetMyJournalDetails);
}
PyResult AgentBound::GetAgentLocationWrap(PyCallArgs &call) {
    // this is detailed info on agent's location
    return m_agent->GetLocationWrap();
}

PyResult AgentBound::GetInfoServiceDetails(PyCallArgs& call) {
    // this is agents personal info... level, division, station, etc.
    return m_agent->GetInfoServiceDetails();
}


/**     ***********************************************************************
 * @note   these below are partially coded
 */

// Set agentSays[0]/[1] for a mission offer: prefer the real briefing text that
// was extracted from the EVE University wiki into missionObjectives.briefingText
// (sent as a literal string so the client shows it regardless of localization),
// otherwise fall back to the localized briefing messageID.
static void SetMissionBriefing(Client* pClient, MissionOffer& offer, PyTuple* agentSays)
{
    std::string brief;
    std::string objective;
    DBQueryResult res;
    if (sDatabase.RunQuery(res,
            "SELECT briefingText, objectiveText FROM missionObjectives WHERE missionID = %u",
            offer.missionID)) {
        DBResultRow row;
        if (res.GetRow(row)) {
            if (!row.IsNull(0)) {
                const char* t = row.GetText(0);
                if (t != nullptr)
                    brief = t;
            }
            if (!row.IsNull(1)) {
                const char* t = row.GetText(1);
                if (t != nullptr)
                    objective = t;
            }
        }
    }
    if (brief.empty() && !objective.empty())
        brief = "Objective: " + objective;      // no lore text on the wiki - show the task
    if (brief.empty() && offer.typeID == Mission::Type::Storyline)
        brief = "Objective: Deliver the mission package to the designated station.";
    if (brief.empty()) {
        // Synthetic pool missions (trade/research/fallback mining) have no
        // missionID/localization row - describe the task with the real item name.
        std::string itemName;
        if (offer.courierTypeID != 0) {
            DBQueryResult ires;
            if (sDatabase.RunQuery(ires, "SELECT typeName FROM invTypes WHERE typeID = %u", offer.courierTypeID)) {
                DBResultRow irow;
                if (ires.GetRow(irow) && !irow.IsNull(0)) {
                    const char* n = irow.GetText(0);
                    if (n != nullptr) itemName = n;
                }
            }
        }
        char buf[512];
        switch (offer.typeID) {
            case Mission::Type::Mining:
                snprintf(buf, sizeof(buf), "Objective: Mine %u units of %s and bring them to me.",
                         offer.courierAmount, itemName.empty() ? "the requested ore" : itemName.c_str());
                brief = buf; break;
            case Mission::Type::Trade:
            case Mission::Type::Data:
                snprintf(buf, sizeof(buf), "Objective: Acquire %u units of %s and bring them to me.",
                         offer.courierAmount, itemName.empty() ? "the requested goods" : itemName.c_str());
                brief = buf; break;
            case Mission::Type::Research:
                snprintf(buf, sizeof(buf), "Objective: Acquire %u units of %s for my research and bring them to me.",
                         offer.courierAmount, itemName.empty() ? "the requested materials" : itemName.c_str());
                brief = buf; break;
            case Mission::Type::Tutorial:
                brief = "Objective: Follow the tutorial instructions."; break;
            case Mission::Type::Anomic:
            case Mission::Type::Burner:
                brief = "Objective: Destroy the target ships at the designated site."; break;
            default: break;
        }
    }
    if (!brief.empty()) {
        size_t pos;
        while ((pos = brief.find("<Character>")) != std::string::npos)
            brief.replace(pos, 11, pClient->GetName());
        PySetItemRelease(agentSays, 0, new PyString(brief.c_str()));
    } else {
        PySetItemRelease(agentSays, 0, new PyInt(offer.briefingID));
    }
    PySetItemRelease(agentSays, 1, new PyInt(offer.characterID));
}

// Build (but do not persist) a tutorial/aura mission offer that tracks one tutorial
// step of the Aura chain. The mission name is the tutorial's own name, so the
// journal entry matches what the client shows during training.
static void BuildTutorialOffer(Agent* agent, uint32 charID, uint32 tutorialID, MissionOffer& offer)
{
    offer = MissionOffer();
    offer.bookmarks = RefPtr<PyList>(new PyList());
    offer.typeID = Mission::Type::Tutorial;
    offer.objectiveType = 4;             // tutorial step objective
    offer.objectiveTutorialID = tutorialID;
    offer.stateID = Mission::State::Accepted;
    offer.characterID = charID;
    offer.agentID = agent->GetID();
    offer.missionID = 1;
    offer.briefingID = 0;
    offer.name = "Aura Training";
    DBQueryResult tRes;
    if (sDatabase.RunQuery(tRes, "SELECT tutorialName FROM tutorials WHERE tutorialID = %u", tutorialID)) {
        DBResultRow tRow;
        if (tRes.GetRow(tRow)) {
            const char* n = tRow.GetText(0);
            if (n != nullptr)
                offer.name = n;
        }
    }
    offer.originID = agent->GetStationID();
    offer.originOwnerID = agent->GetCorpID();
    offer.originSystemID = agent->GetSystemID();
    offer.destinationID = offer.originID;
    offer.destinationOwnerID = offer.originOwnerID;
    offer.destinationSystemID = offer.originSystemID;
    offer.rewardISK = 10000;
    offer.dateIssued = GetFileTimeNow();
    offer.dateAccepted = GetFileTimeNow();
    offer.expiryTime = GetFileTimeNow() + EvE::Time::Day * 30;
}

PyResult AgentBound::DoAction(PyCallArgs &call, std::optional <PyInt*> actionID) {
    // this is first call when initiating agent convo
    _log(AGENT__DUMP,  "AgentBound::Handle_DoAction() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    // this actually returns a complicated tuple depending on other variables involving this agent and char.
    /*
        agentSays, dialogue, extraInfo = self.__GetConversation(agentDialogueWindow, actionID)

    def __GetConversation(self, wnd, actionID):
       tmp = wnd.sr.agentMoniker.DoAction(actionID)
        ret, wnd.sr.oob = tmp
        agentSays, wnd.sr.dialogue = ret
        firstActionID = wnd.sr.dialogue[0][0]
        firstActionDialogue = wnd.sr.dialogue[0][1]
        wnd.sr.agentSays = self.ProcessMessage(agentSays, wnd.sr.agentID)
        return (wnd.sr.agentSays, wnd.sr.dialogue, wnd.sr.oob)
     *
     */

    Character* pchar = call.client->GetChar().get();
    float charStanding = StandingDB::GetStanding(m_agent->GetID(), pchar->itemID());
    float quality = EvEMath::Agent::EffectiveQuality(m_agent->GetQuality(), pchar->GetSkillLevel(EvESkill::Connections), charStanding);
    float bonus = EvEMath::Agent::GetStandingBonus(charStanding, m_agent->GetFactionID(), pchar->GetSkillLevel(EvESkill::Connections), pchar->GetSkillLevel(EvESkill::Diplomacy), pchar->GetSkillLevel(EvESkill::CriminalConnections));
    float standing = EvEMath::Agent::EffectiveStanding(charStanding, bonus);

    std::string response = "";
    bool missionQuit = false, missionCompleted = false, missionDeclined = false;

    PyTuple* agentSays = new PyTuple(2);
    // dialog is button info
    PyList* dialog = new PyList();

    using namespace Dialog::Button;

    // to set 'admin dialog options' (which i dont know wtf they are yet), i *think* you add a tuple of *something* that is NOT dict or int.
    if (false /*admin options*/) {
        PyTuple* adminButton = new PyTuple(2);
        PySetItemRelease(adminButton, 0, new PyInt(Admin));
        PySetItemRelease(adminButton, 1, new PyString("Admin Options"));
        dialog->AddItem(adminButton);
    }

                // branch-choice links (epic arcs) arrive as actionID 200+N, where N
                // is the branchID of the chosen mission
                int actionVal = actionID.has_value() ? actionID.value()->value() : 0;
                if (actionVal >= 200 && actionVal < 300) {
                    EpicArcData* bArc = sEpicArcMgr.GetArcByAgent(m_agent->GetID());
                    if (bArc != nullptr)
                        sEpicArcMgr.SetBranchChoice(pchar->itemID(), bArc->arcID, (int8)(actionVal - 200));
                    actionVal = EpicArcStart;
                }

                if (m_agent->CanUseAgent(call.client)) {
        switch (actionVal) {
            case 0: {
                //  if char has current mission with this agent, add this one.
                MissionOffer offer = MissionOffer();
                if (m_agent->HasMission(pchar->itemID(), offer)) {
                    PyTuple* button1 = new PyTuple(2);
                        PySetItemRelease(button1, 0, new PyInt(ViewMission));
                        PySetItemRelease(button1, 1, new PyInt(ViewMission));
                    dialog->AddItem(button1);
                    if (call.client->IsMissionComplete(offer))  {
                        PyTuple* button2 = new PyTuple(2);
                            PySetItemRelease(button2, 0, new PyInt(Complete));
                            PySetItemRelease(button2, 1, new PyInt(Complete));
                        dialog->AddItem(button2);
                    }
                    SetMissionBriefing(call.client, offer, agentSays);
                } else {
                    // Check for epic arc
                    bool hasArc = false;
                    EpicArcData* arc = sEpicArcMgr.GetArcByAgent(m_agent->GetID());
                    if (arc != nullptr) {
                        if (sEpicArcMgr.IsOnArc(pchar->itemID(), arc->arcID)) {
                            PyTuple* arcButton = new PyTuple(2);
                                PySetItemRelease(arcButton, 0, new PyInt(Dialog::Button::EpicArcStart));
                                PySetItemRelease(arcButton, 1, new PyString("Continue Epic Arc"));
                            dialog->AddItem(arcButton);
                            hasArc = true;
                        } else if (sEpicArcMgr.CanStartArc(pchar->itemID(), arc->arcID)) {
                            PyTuple* arcButton = new PyTuple(2);
                                PySetItemRelease(arcButton, 0, new PyInt(Dialog::Button::EpicArcStart));
                                PySetItemRelease(arcButton, 1, new PyString("Epic Arc"));
                            dialog->AddItem(arcButton);
                            hasArc = true;
                        }
                    }
                    // Show RequestMission only for agent types that can give missions
                    if (m_agent->GetTypeID() == Agents::Type::Basic or
                        m_agent->GetTypeID() == Agents::Type::None or
                        m_agent->GetTypeID() == Agents::Type::Research or
                        m_agent->GetTypeID() == Agents::Type::GenericStoryLine or
                        m_agent->GetTypeID() == Agents::Type::StoryLine or
                        m_agent->GetTypeID() == Agents::Type::FacWar or
                        m_agent->GetTypeID() == Agents::Type::Cosmos or
                        m_agent->IsCosmos() or
                        m_agent->GetTypeID() == Agents::Type::Tutorial or
                        m_agent->GetTypeID() == Agents::Type::Aura) {
                        PyTuple* button2 = new PyTuple(2);
                            PySetItemRelease(button2, 0, new PyInt(RequestMission));
                            PySetItemRelease(button2, 1, new PyInt(RequestMission));
                        dialog->AddItem(button2);
                    }
                    response = "Why the fuck am I looking at you again, ";
                    response += call.client->GetName();
                    response += "?";
                    PySetItemRelease(agentSays, 0, new PyString(response));
                    agentSays->SetItem(1, PyStatic.NewNone());
                }

                // if agent does location, add this one...
                if (m_agent->IsLocator()) {
                    PyTuple* button3 = new PyTuple(2);
                        PySetItemRelease(button3, 0, new PyInt(LocateCharacter));
                        PySetItemRelease(button3, 1, new PyInt(LocateCharacter));
                    dialog->AddItem(button3);
                }

                // if agent does research, add this one...
                if (m_agent->IsResearch()) {
                    PyTuple* button4 = new PyTuple(2);
                        PySetItemRelease(button4, 0, new PyInt(StartResearch));
                        PySetItemRelease(button4, 1, new PyInt(StartResearch));
                    dialog->AddItem(button4);
                }
            } break;
            case EpicArcStart: { //100
                EpicArcData* arc = sEpicArcMgr.GetArcByAgent(m_agent->GetID());
                if (arc != nullptr) {
                    uint32 charID = pchar->itemID();
                    if (!sEpicArcMgr.IsOnArc(charID, arc->arcID)) {
                        if (sEpicArcMgr.CanStartArc(charID, arc->arcID))
                            sEpicArcMgr.StartArc(charID, arc->arcID, m_agent->GetID());
                    }
                    // branch point: let the player choose the story line (the client
                    // renders the dict entries as mission-title links)
                    std::vector<EpicArcMissionData> arcOptions = sEpicArcMgr.GetNextMissionOptions(charID, arc->arcID);
                    if (arcOptions.size() > 1) {
                        PySetItemRelease(agentSays, 0, new PyString("Choose your path."));
                        agentSays->SetItem(1, PyStatic.NewNone());
                        for (auto& opt : arcOptions) {
                            std::string optBrief;
                            DBQueryResult bRes;
                            if (sDatabase.RunQuery(bRes,
                                    "SELECT briefingText FROM missionObjectives WHERE missionID = %u", opt.missionID)) {
                                DBResultRow bRow;
                                if (bRes.GetRow(bRow) && !bRow.IsNull(0))
                                    if (const char* t = bRow.GetText(0))
                                        optBrief = t;
                            }
                            PyDict* choice = new PyDict();
                                choice->SetItemString("ContentID", new PyInt(charID));
                                choice->SetItemString("Mission Keywords", new PyDict());
                                choice->SetItemString("Mission Title ID", new PyString(opt.missionName));
                                if (!optBrief.empty())
                                    choice->SetItemString("Mission Briefing ID", new PyString(optBrief.c_str()));
                                else
                                    choice->SetItemString("Mission Briefing ID", PyStatic.NewNone());
                            PyTuple* choiceEntry = new PyTuple(2);
                                PySetItemRelease(choiceEntry, 0, new PyInt(200 + opt.branchID));
                                PySetItemRelease(choiceEntry, 1, choice);
                            dialog->AddItem(choiceEntry);
                        }
                        break;
                    }

                    MissionOffer offer = MissionOffer();
                    // the journal clones offer.bookmarks — a null RefPtr here
                    // segfaulted the server on the first journal open
                    offer.bookmarks = RefPtr<PyList>(new PyList());
                    m_agent->MakeOffer(charID, offer);
                    // Override with epic arc mission data
                    EpicArcMissionData* nextMission = sEpicArcMgr.GetNextMissionForChar(charID, arc->arcID);
                    if (nextMission != nullptr) {
                        offer.name = nextMission->missionName;
                        offer.missionID = nextMission->missionID;
                        offer.rewardISK = nextMission->rewardISK;
                        offer.bonusISK = nextMission->rewardISK * 0.2;
                        offer.typeID = Mission::Type::EpicArc;
                        offer.storyline = true;
                        offer.characterID = charID;
                        offer.agentID = m_agent->GetID();
                        offer.originID = (m_agent->GetStationID() != 0) ? m_agent->GetStationID() : m_agent->GetLocationID();
                        offer.originOwnerID = m_agent->GetCorpID();
                        offer.originSystemID = m_agent->GetSystemID();
                        offer.dateIssued = GetFileTimeNow();
                        offer.expiryTime = GetFileTimeNow() + EvE::Time::Day * 90;
                        offer.stateID = Mission::State::Allocated;
                        // resolve a destination system for the objective (arc missions are
                        // not courier/trade, so MakeOffer never picked one), then assign the
                        // objective before the offer row is written
                        offer.range = 2;
                        sMapData.GetMissionDestination(m_agent, Mission::Type::Encounter, offer);
                        if (offer.destinationSystemID == 0 || offer.destinationSystemID == offer.originSystemID) {
                            DBQueryResult jumpRes;
                            if (sDatabase.RunQuery(jumpRes, "SELECT toSolarSystemID FROM mapSolarSystemJumps WHERE fromSolarSystemID = %u LIMIT 1", offer.originSystemID)) {
                                DBResultRow jumpRow;
                                if (jumpRes.GetRow(jumpRow)) {
                                    offer.destinationSystemID = jumpRow.GetUInt(0);
                                    offer.destinationID = offer.destinationSystemID;
                                    offer.destinationTypeID = 0;
                                    offer.destinationOwnerID = 0;
                                }
                            }
                        }
                        sMissionDataMgr.AssignArcObjective(offer);
                        MissionDB::CreateOfferID(offer);
                        m_agent->AddOffer(charID, offer);
                        m_agent->SendMissionUpdate(call.client, "offered");
                        // persistent notification
                        PyDict* notifData = new PyDict();
                            notifData->SetItemString("agentID", new PyInt(m_agent->GetID()));
                            notifData->SetItemString("missionName", new PyString(offer.name));
                        sEntityList.CreateNotification(charID, Notify::Types::ResearchMissionAvailable, m_agent->GetID(), notifData);
                        PySetItemRelease(agentSays, 0, new PyInt(nextMission->missionID));
                        PySetItemRelease(agentSays, 1, new PyInt(charID));
                        PyTuple* button1 = new PyTuple(2);
                            PySetItemRelease(button1, 0, new PyInt(Accept));
                            PySetItemRelease(button1, 1, new PyInt(Accept));
                        dialog->AddItem(button1);
                        PyTuple* button2 = new PyTuple(2);
                            PySetItemRelease(button2, 0, new PyInt(Decline));
                            PySetItemRelease(button2, 1, new PyInt(Decline));
                        dialog->AddItem(button2);
                    }
                }
            } break;
            case RequestMission: {  //2
                // Aura starts tutorial chain, not a standard mission
                if (m_agent->GetTypeID() == Agents::Type::Aura) {
                    PySetItemRelease(agentSays, 0, new PyString("Welcome to EVE Online. Your training begins now."));
                    agentSays->SetItem(1, PyStatic.NewNone());
                    PyTuple* button1 = new PyTuple(2);
                        PySetItemRelease(button1, 0, new PyInt(Accept));
                        PySetItemRelease(button1, 1, new PyString("Begin Training"));
                    dialog->AddItem(button1);
                    // Notify client to open tutorial browser with first Aura tutorial
                    PyDict* args = new PyDict();
                        args->SetItemString("tutorialID", new PyInt(215));
                    PyTuple* payload = new PyTuple(1);
                        payload->SetItem(0, args);
                    call.client->SendNotification("OnServerTutorialRequest", "clientID", payload, false);
                } else {
                MissionOffer offer = MissionOffer();
                m_agent->MakeOffer(pchar->itemID(), offer);
                if (offer.name.empty()) {
                    // COSMOS chain finished (one-time missions): nothing left to offer
                    PySetItemRelease(agentSays, 0, new PyString("I have no further assignments for you right now."));
                    agentSays->SetItem(1, PyStatic.NewNone());
                    break;
                }
                m_agent->SendMissionUpdate(call.client, "offered");

                //  this one will get complicated and is based on agent/char interaction
                //   detail in /eve/client/script/ui/station/agents/agents.py

                /*  agentSays is a tuple of msgData and contentID
                *      msgData can be single integer of briefingID, a string literal, or a tuple as defined above.
                *   contentID is used for specific char's mission keywords.  we're not using it like there here....
                */

                SetMissionBriefing(call.client, offer, agentSays);

                // dialog can also contain mission data.
                //   set a dialog tuple[1] to dict and fill with MissionBriefingInfo
                PyTuple* button1 = new PyTuple(2);
                    PySetItemRelease(button1, 0, new PyInt(Accept));
                    PySetItemRelease(button1, 1, new PyInt(Accept));
                dialog->AddItem(button1);
                PyTuple* button2 = new PyTuple(2);
                    PySetItemRelease(button2, 0, new PyInt(Decline));
                    PySetItemRelease(button2, 1, new PyInt(Decline));
                dialog->AddItem(button2);
                PyTuple* button3 = new PyTuple(2);
                    PySetItemRelease(button3, 0, new PyInt(Defer));
                    PySetItemRelease(button3, 1, new PyInt(Defer));
                dialog->AddItem(button3);
                }
            } break;
            case ViewMission: { //1
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                SetMissionBriefing(call.client, offer, agentSays);
                if (offer.stateID < Mission::State::Accepted) {
                    PyTuple* button1 = new PyTuple(2);
                        PySetItemRelease(button1, 0, new PyInt(Accept));
                        PySetItemRelease(button1, 1, new PyInt(Accept));
                    dialog->AddItem(button1);
                    PyTuple* button2 = new PyTuple(2);
                        PySetItemRelease(button2, 0, new PyInt(Decline));
                        PySetItemRelease(button2, 1, new PyInt(Decline));
                    dialog->AddItem(button2);
                    PyTuple* button3 = new PyTuple(2);
                        PySetItemRelease(button3, 0, new PyInt(Defer));
                        PySetItemRelease(button3, 1, new PyInt(Defer));
                    dialog->AddItem(button3);
                } else if (offer.stateID == Mission::State::Accepted) {
                    PyTuple* button1 = new PyTuple(2);
                        PySetItemRelease(button1, 0, new PyInt(Quit));
                        PySetItemRelease(button1, 1, new PyInt(Quit));
                    dialog->AddItem(button1);
                    if (call.client->IsMissionComplete(offer))  {
                        PyTuple* button2 = new PyTuple(2);
                            PySetItemRelease(button2, 0, new PyInt(Complete));
                            PySetItemRelease(button2, 1, new PyInt(Complete));
                        dialog->AddItem(button2);
                    }
                }
            } break;
            case Accept:            //3
            case AcceptRemotely: {  //5
                // Aura: tutorial already started via OnServerTutorialRequest from RequestMission
                if (m_agent->GetTypeID() == Agents::Type::Aura) {
                    PySetItemRelease(agentSays, 0, new PyString("Good luck pilot. Follow the tutorial instructions."));
                    agentSays->SetItem(1, PyStatic.NewNone());
                    // journal mission that tracks the tutorial step the client is running
                    if (!m_agent->HasMission(pchar->itemID())) {
                        MissionOffer tut = MissionOffer();
                        BuildTutorialOffer(m_agent, pchar->itemID(), 215, tut);
                        MissionDB::CreateOfferID(tut);
                        m_agent->AddOffer(pchar->itemID(), tut);
                        m_agent->SendMissionUpdate(call.client, "offer_accepted");
                    }
                } else {
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                offer.stateID = Mission::State::Accepted;
                offer.dateAccepted = GetFileTimeNow();
                offer.expiryTime = GetFileTimeNow() + (30 * m_agent->GetLevel() * EvE::Time::Minute);
                // career-agent tutorial chain: ask the client to run this step
                if (offer.typeID == Mission::Type::Tutorial && offer.objectiveTutorialID != 0) {
                    PyDict* targs = new PyDict();
                        targs->SetItemString("tutorialID", new PyInt(offer.objectiveTutorialID));
                    PyTuple* tpayload = new PyTuple(1);
                        tpayload->SetItem(0, targs);
                    call.client->SendNotification("OnServerTutorialRequest", "clientID", tpayload, false);
                }
                // only courier missions hand the pilot a mission package; mining/trade/
                // research missions expect the pilot to mine or buy the goods themselves
                // (spawning cargo for them made every such mission instantly complete)
                bool isCourierObjective = (offer.typeID == Mission::Type::Courier)
                    || ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos) && offer.objectiveType == 2)
                    || (offer.typeID == Mission::Type::Storyline && offer.courierTypeID != 0);
                if (isCourierObjective && offer.courierTypeID) {
                    sItemFactory.SetUsingClient(call.client);
                    ItemData data(offer.courierTypeID, pchar->itemID(), locTemp, flagNone, offer.courierAmount);
                    InventoryItemRef iRef = sItemFactory.SpawnItem(data);
                    if (iRef.get() != nullptr) {
                        // in-space agent: the mission package goes to the ship's hold
                        if (sDataMgr.IsStation(offer.originID))
                            iRef->Move(offer.originID, flagHangar, true);
                        else
                            iRef->Move(pchar->itemID(), flagCargoHold, true);
                    }
                    sItemFactory.UnsetUsingClient();
                }
                // Spawn mission dungeon for encounter/mining type missions
                GPoint dungeonSite(0.0, 0.0, 0.0);   // position of the spawned dungeon (if any)

                // mission content overrides (faction / npc count / structures) from the
                // curated missionObjectives table (built from the mission descriptions)
                uint32 moFaction = 0, moCount = 0, moStruct = 0, moStructQty = 0;
                {
                    DBQueryResult moRes;
                    if (sDatabase.RunQuery(moRes,
                            "SELECT npcFactionID, npcCount, structureTypeID, structureQty FROM missionObjectives WHERE missionID = %u",
                            offer.missionID)) {
                        DBResultRow moRow;
                        if (moRes.GetRow(moRow)) {
                            moFaction = moRow.GetUInt(0);
                            moCount = moRow.GetUInt(1);
                            moStruct = moRow.GetUInt(2);
                            moStructQty = moRow.GetUInt(3);
                        }
                    }
                }
                if (offer.destinationSystemID != 0 && offer.missionID != 0) {
                    uint32 dungeonID = 0;
                    DBQueryResult dunRes;
                    if (sDatabase.RunQuery(dunRes,
                        "SELECT dungeonID FROM qstEncounter WHERE id = %u AND dungeonID > 0",
                        offer.missionID))
                    {
                        DBResultRow dunRow;
                        if (dunRes.GetRow(dunRow))
                            dungeonID = dunRow.GetUInt(0);
                    }
                    if (dungeonID > 0) {
                        SystemManager* pDestSys = sEntityList.FindOrBootSystem(offer.destinationSystemID);
                        if (pDestSys != nullptr) {
                            CosmicSignature sig;
                            sig.systemID = offer.destinationSystemID;
                            sig.ownerID = call.client->GetCharacterID();
                            sig.sigName = call.client->GetName();
                            sig.position = GPoint(
                                MakeRandomFloat(-2.0e12, 2.0e12),
                                MakeRandomFloat(-2.0e12, 2.0e12),
                                MakeRandomFloat(-2.0e12, 2.0e12)
                            );
                            pDestSys->GetDungMgr()->MakeDungeon(sig, dungeonID);
                            // Remember where the dungeon went so the encounter
                            // target cluster can be placed beside it.
                            dungeonSite = sig.position;
                            _log(AGENT__MESSAGE, "Spawned dungeon %u for mission %u in system %u.",
                                 dungeonID, offer.missionID, offer.destinationSystemID);
                        }
                    }
                }
                // Encounter missions: spawn a tracked hostile target cluster so the
                // player has something to clear and the mission can be completed.
                // Placed at the mission dungeon's site when one exists (the player
                // fights dungeon NPCs and the tracked targets together); otherwise
                // at a random point in the destination system.
                if (offer.typeID == Mission::Type::Encounter && offer.destinationSystemID != 0) {
                    EncounterSpawnServer* ess = EncounterSpawnServer::Get();
                    if (ess != nullptr) {
                        uint32 n = ess->SpawnEncounterForOffer(offer, offer.destinationSystemID, dungeonSite, moFaction, moCount);
                        if (n > 0)
                            _log(AGENT__MESSAGE, "Spawned %u encounter targets for mission %u offer %u.",
                                 n, offer.missionID, offer.offerID);
                    }
                }
                // Anomic/burner: elite deadspace pirate + escorts at a site
                if (offer.typeID == Mission::Type::Anomic && offer.destinationSystemID != 0) {
                    EncounterSpawnServer* ess = EncounterSpawnServer::Get();
                    if (ess != nullptr) {
                        GPoint anomicPos = dungeonSite;
                        if (anomicPos.x == 0 && anomicPos.y == 0 && anomicPos.z == 0)
                            anomicPos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
                        uint32 n = ess->SpawnAnomicForOffer(offer, offer.destinationSystemID, anomicPos, moFaction);
                        if (n == 0)
                            n = ess->SpawnEncounterForOffer(offer, offer.destinationSystemID, anomicPos, moFaction, 3);
                        _log(AGENT__MESSAGE, "Spawned %u anomic targets for mission %u offer %u.",
                             n, offer.missionID, offer.offerID);
                    }
                }
                // Mining missions: spawn an ore belt of the requested ore in the
                // destination system so the player always has something to mine
                // (SDE mining missions don't point at an existing belt). Registers
                // the belt position for the journal warp link.
                if (offer.typeID == Mission::Type::Mining && offer.destinationSystemID != 0
                    && offer.courierTypeID != 0) {
                    SystemManager* pDestSys = sEntityList.FindOrBootSystem(offer.destinationSystemID);
                    if (pDestSys != nullptr) {
                        GPoint beltPos = dungeonSite;
                        if (beltPos.x == 0 && beltPos.y == 0 && beltPos.z == 0)
                            beltPos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12),
                                             MakeRandomFloat(-2.0e12, 2.0e12),
                                             MakeRandomFloat(-2.0e12, 2.0e12));
                        std::vector<uint32> roids = pDestSys->GetDungMgr()->SpawnMineableAsteroids(
                            offer.courierTypeID, beltPos, 25 + MakeRandomInt(0, 15));
                        if (!roids.empty()) {
                            if (EncounterSpawnServer::Get() != nullptr)
                                EncounterSpawnServer::Get()->RegisterMiningSite(offer.offerID, beltPos);
                            _log(AGENT__MESSAGE, "Spawned mining belt (%zu roids of type %u) for mission %u offer %u.",
                                 roids.size(), offer.courierTypeID, offer.missionID, offer.offerID);
                        }
                    }
                }
                // Epic arc / COSMOS hunt objectives spawn a tracked hostile cluster in the
                // destination system (completion = every target destroyed)
                if ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos)
                    && offer.objectiveType == 3 && offer.destinationSystemID != 0) {
                    EncounterSpawnServer* huntEss = EncounterSpawnServer::Get();
                    if (huntEss != nullptr) {
                        uint32 huntSys = offer.destinationSystemID;
                        GPoint huntPos = dungeonSite;
                        if (huntPos.x == 0 && huntPos.y == 0 && huntPos.z == 0)
                            huntPos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
                        // COSMOS agent in space: the targets live at its own site, so
                        // fly to the agent instead of a random destination system
                        if (m_agent->GetStationID() == 0) {
                            SystemEntity* agSE = nullptr;
                            auto& systems = sEntityList.GetSystems();
                            auto sit = systems.find(m_agent->GetSystemID());
                            if (sit != systems.end())
                                agSE = sit->second->GetCosmosAgentSE(m_agent->GetID());
                            if (agSE != nullptr) {
                                huntSys = m_agent->GetSystemID();
                                huntPos = agSE->GetPosition();
                                huntPos.x += 30000.0;
                                dungeonSite = huntPos;
                                offer.destinationSystemID = huntSys;
                                offer.destinationID = huntSys;
                                offer.destinationOwnerID = 0;
                            }
                        }
                        // COSMOS agents docked at a station: keep the fight in their
                        // own system next to the agent's station
                        if (offer.typeID == Mission::Type::Cosmos && m_agent->GetStationID() != 0) {
                            SystemManager* pAgentSys = sEntityList.FindOrBootSystem(m_agent->GetSystemID());
                            if (pAgentSys != nullptr)
                                if (SystemEntity* stSE = pAgentSys->GetSE(m_agent->GetStationID())) {
                                    huntSys = m_agent->GetSystemID();
                                    huntPos = stSE->GetPosition();
                                    huntPos.x += 30000.0;
                                    dungeonSite = huntPos;
                                    offer.destinationSystemID = huntSys;
                                    offer.destinationID = huntSys;
                                    offer.destinationOwnerID = 0;
                                }
                        }
                        // arc hunt with a real destination station: put the target
                        // cluster next to that station (reachable + visible in the journal)
                        if (huntSys == offer.destinationSystemID && offer.destinationID != 0
                            && sDataMgr.IsStation(offer.destinationID)) {
                            SystemManager* pDestSys = sEntityList.FindOrBootSystem(huntSys);
                            if (pDestSys != nullptr)
                                if (SystemEntity* stSE = pDestSys->GetSE(offer.destinationID)) {
                                    huntPos = stSE->GetPosition();
                                    huntPos.x += 30000.0;
                                    dungeonSite = huntPos;
                                }
                        }
                        uint32 huntCount = 0;
                        if (offer.typeID == Mission::Type::Cosmos) {
                            // use the real NPC pack of the agent's COSMOS site when possible
                            uint32 siteID = 0;
                            DBQueryResult siteRes;
                            if (sDatabase.RunQuery(siteRes,
                                    "SELECT siteID FROM cosmosAgentSites WHERE agentID = %u LIMIT 1",
                                    m_agent->GetID())) {
                                DBResultRow siteRow;
                                if (siteRes.GetRow(siteRow))
                                    siteID = siteRow.GetUInt(0);
                            }
                            if (siteID != 0)
                                huntCount = huntEss->SpawnCosmosHuntForOffer(offer, huntSys, huntPos, siteID);
                        }
                        if (huntCount == 0)
                            huntCount = huntEss->SpawnEncounterForOffer(offer, huntSys, huntPos, moFaction, moCount);
                        _log(AGENT__MESSAGE, "Arc/cosmos hunt: spawned %u targets for mission %u offer %u.",
                             huntCount, offer.missionID, offer.offerID);
                    }
                }
                // encounter-style storyline missions spawn their target cluster as well
                if (offer.typeID == Mission::Type::Storyline && offer.courierTypeID == 0
                    && offer.destinationSystemID != 0) {
                    EncounterSpawnServer* slEss = EncounterSpawnServer::Get();
                    if (slEss != nullptr) {
                        GPoint slPos = dungeonSite;
                        if (slPos.x == 0 && slPos.y == 0 && slPos.z == 0)
                            slPos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
                        uint32 slCount = slEss->SpawnEncounterForOffer(offer, offer.destinationSystemID, slPos, moFaction, moCount);
                        _log(AGENT__MESSAGE, "Storyline: spawned %u targets for mission %u offer %u.",
                             slCount, offer.missionID, offer.offerID);
                    }
                }
                // build the actual mission site: faction ambience and (for structure
                // objectives) the target structures described by the mission text
                if (offer.destinationSystemID != 0 && (moFaction != 0 || moStructQty > 0)) {
                    SystemManager* pSiteSys = sEntityList.FindOrBootSystem(offer.destinationSystemID);
                    if (pSiteSys != nullptr) {
                        GPoint sitePos = dungeonSite;
                        if (sitePos.x == 0 && sitePos.y == 0 && sitePos.z == 0)
                            sitePos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
                        pSiteSys->GetDungMgr()->SpawnMissionSite(sitePos, moFaction, moStruct, moStructQty);
                    }
                }
                // fetch objectives: the requested item(s) must be retrievable at the
                // mission site, so drop a lootable container there on Accept
                if ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos)
                    && offer.objectiveType == 5 && offer.courierTypeID != 0 && offer.destinationSystemID != 0) {
                    SystemManager* pLootSys = nullptr;
                    GPoint lootPos = dungeonSite;
                    SystemEntity* agSE = nullptr;
                    {
                        auto& systems = sEntityList.GetSystems();
                        auto it = systems.find(m_agent->GetSystemID());
                        if (it != systems.end()) {
                            pLootSys = it->second;
                            agSE = pLootSys->GetCosmosAgentSE(m_agent->GetID());
                        }
                    }
                    if (agSE != nullptr) {
                        // COSMOS agent in space: the item lies next to the agent's site
                        lootPos = agSE->GetPosition();
                        lootPos.x += 20000.0;
                    } else {
                        pLootSys = sEntityList.FindOrBootSystem(offer.destinationSystemID);
                        if (pLootSys != nullptr) {
                            // put the container next to a warpable landmark so the pilot can
                            // actually find it (mission journal has no site bookmark for fetch)
                            std::vector<uint32> dstStations;
                            if (sDataMgr.GetStationList(offer.destinationSystemID, dstStations) && !dstStations.empty())
                                if (SystemEntity* stSE = pLootSys->GetSE(dstStations.front()))
                                    lootPos = stSE->GetPosition();
                            if (lootPos.x == 0 && lootPos.y == 0 && lootPos.z == 0)
                                lootPos = GPoint(MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12), MakeRandomFloat(-2.0e12, 2.0e12));
                            else
                                lootPos.MakeRandomPointOnSphere(50000.0f);
                        }
                    }
                    if (pLootSys != nullptr)
                        pLootSys->GetDungMgr()->SpawnMissionLoot(lootPos, offer.courierTypeID, offer.courierAmount);
                }
                m_agent->UpdateOffer(pchar->itemID(), offer);
                m_agent->SendMissionUpdate(call.client, "offer_accepted");
                PySetItemRelease(agentSays, 0, new PyInt(m_agent->GetAcceptRsp(pchar->itemID())));
                PySetItemRelease(agentSays, 1, new PyInt(pchar->itemID()));
                }
            } break;
            case Complete:              //6
            case CompleteRemotely: {    //7
                //  need to verify all requirements have been met.
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                offer.stateID = Mission::State::Completed;
                offer.dateCompleted = GetFileTimeNow();
                m_agent->UpdateOffer(pchar->itemID(), offer);
                m_agent->SendMissionUpdate(call.client, "completed");
                PySetItemRelease(agentSays, 0, new PyInt(m_agent->GetCompleteRsp(pchar->itemID())));
                PySetItemRelease(agentSays, 1, new PyInt(pchar->itemID()));
                // Epic arc completion handling
                if (offer.typeID == Mission::Type::EpicArc) {
                    EpicArcData* arc = sEpicArcMgr.GetArcByAgent(m_agent->GetID());
                    if (arc != nullptr) {
                        uint32 charID = pchar->itemID();
                        // Final mission (80056 = Our Man Dagan) awards faction standing
                        if (offer.missionID == 80056) {
                            uint8 socialLvl = pchar->GetSkillLevel(EvESkill::Social);
                            float standingReward = 0.7f * (1.0f + 0.05f * socialLvl);
                            StandingDB::SaveStandingChanges(charID, arc->factionID, 126,
                                standingReward, "The Blood-Stained Stars completion");
                        }
                        // advance the chapter/mission state (completes the arc when
                        // the last chapter has no further missions for this branch)
                        sEpicArcMgr.AdvanceMission(charID, arc->arcID, offer.missionID);
                    }
                }
                // COSMOS one-time completion tracking
                if (offer.typeID == Mission::Type::Cosmos) {
                    uint32 charID = pchar->itemID();
                    DBerror err;
                    sDatabase.RunQuery(err,
                        "INSERT INTO chrCosmosState (characterID, missionID, agentID, completed, dateCompleted)"
                        " VALUES (%u, %u, %u, 1, %.0f) ON DUPLICATE KEY UPDATE completed = 1, dateCompleted = %.0f",
                        charID, offer.missionID, m_agent->GetID(), GetFileTimeNow(), GetFileTimeNow());
                }
                if (offer.courierTypeID) {
                    // remove item from player possession
                    call.client->RemoveMissionItem(offer.courierTypeID, offer.courierAmount);
                }
                if (offer.rewardItemID) {
                    // add reward item: to the hangar when docked, otherwise to the
                    // ship's cargo hold (COSMOS agents live in space, no station)
                    sItemFactory.SetUsingClient(call.client);
                    ItemData data(offer.rewardItemID, pchar->itemID(), locTemp, flagNone, offer.rewardItemQty);
                    InventoryItemRef iRef = sItemFactory.SpawnItem(data);
                    if (iRef.get() != nullptr) {
                        // courier missions can be completed remotely at the
                        // destination station - deliver the item where the pilot is
                        uint32 rewardStation = call.client->GetStationID();
                        if (rewardStation == 0)
                            rewardStation = m_agent->GetStationID();
                        if (rewardStation != 0)
                            iRef->Move(rewardStation, flagHangar, true);
                        else
                            iRef->Move(pchar->itemID(), flagCargoHold, true);
                    }
                    sItemFactory.UnsetUsingClient();
                }
                /** @todo  add fleet sharing  */
                if (offer.rewardISK)
                    AccountService::TransferFunds(m_agent->GetID(), pchar->itemID(), offer.rewardISK, "Mission Reward", Journal::EntryType::AgentMissionReward, m_agent->GetID());
                if (offer.bonusTime > 0 && offer.dateAccepted > 0 && offer.bonusISK > 0
                    && (GetFileTimeNow() - offer.dateAccepted) < ((int64)offer.bonusTime * EvE::Time::Minute))
                    AccountService::TransferFunds(m_agent->GetID(), pchar->itemID(), offer.bonusISK, "Mission Bonus Reward", Journal::EntryType::AgentMissionTimeBonusReward, m_agent->GetID());
                /** @todo  add lp, etc, etc  */
                if (offer.rewardLP)
                    LPService::AddLP(pchar->itemID(), m_agent->GetCorpID(), offer.rewardLP);
                m_agent->UpdateStandings(call.client, Standings::MissionCompleted, offer.important);
                m_agent->RemoveOffer(pchar->itemID());
                // tutorial chain: hand out the next Aura step and tell the client to run it
                if (offer.typeID == Mission::Type::Tutorial) {
                    uint32 nextTut = 0;
                    DBQueryResult tRes;
                    if (sDatabase.RunQuery(tRes, "SELECT nextTutorialID FROM tutorials WHERE tutorialID = %u", offer.objectiveTutorialID)) {
                        DBResultRow tRow;
                        if (tRes.GetRow(tRow))
                            nextTut = tRow.GetUInt(0);
                    }
                    if (nextTut != 0) {
                        MissionOffer tut = MissionOffer();
                        BuildTutorialOffer(m_agent, pchar->itemID(), nextTut, tut);
                        MissionDB::CreateOfferID(tut);
                        m_agent->AddOffer(pchar->itemID(), tut);
                        m_agent->SendMissionUpdate(call.client, "offered");
                        PyDict* targs = new PyDict();
                            targs->SetItemString("tutorialID", new PyInt(nextTut));
                        PyTuple* tpayload = new PyTuple(1);
                            tpayload->SetItem(0, targs);
                        call.client->SendNotification("OnServerTutorialRequest", "clientID", tpayload, false);
                    } else {
                        call.client->SendNotifyMsg("Training complete. Welcome to New Eden, pilot.");
                    }
                }
                // every 15th completed mission for this corp unlocks a storyline offer
                if (offer.typeID != Mission::Type::Tutorial && offer.typeID != Mission::Type::Storyline)
                    m_agent->MaybeOfferStoryline(pchar->itemID(), call.client);
            } break;
            case Defer: {   //10
                // extend expiry time and close
                MissionOffer offer = MissionOffer();
                if (m_agent->HasMission(pchar->itemID(), offer)) {
                    offer.stateID = Mission::State::Allocated; //Defered
                    offer.expiryTime += EvE::Time::Day;
                    m_agent->UpdateOffer(pchar->itemID(), offer);
                    m_agent->SendMissionUpdate(call.client, "prolong");
                    PySetItemRelease(agentSays, 0, new PyString("I can give you 24 hours to think about it."));    //msgInfo  -- if tuple[0].string then return msgInfo
                    agentSays->SetItem(1, PyStatic.NewNone());    // ContentID  -- PyNone used when msgInfo is string to return without processing
                }
            } break;
            case Decline: { //9
                missionDeclined = true;
                m_agent->DeleteOffer(pchar->itemID());
                m_agent->SendMissionUpdate(call.client, "offer_declined");
                PySetItemRelease(agentSays, 0, new PyInt(m_agent->GetDeclineRsp(pchar->itemID())));
                PySetItemRelease(agentSays, 1, new PyInt(pchar->itemID()));
                /** @todo  add lp, etc, etc  */
                m_agent->UpdateStandings(call.client, Standings::MissionDeclined);
            } break;
            case Quit: {    //11
                missionQuit = true;
                MissionOffer offer = MissionOffer();
                m_agent->GetOffer(pchar->itemID(), offer);
                if (offer.courierTypeID) {
                    // remove item from player possession
                    call.client->RemoveMissionItem(offer.courierTypeID, offer.courierAmount);
                }
                // remove mission offer and set standings accordingly
                m_agent->DeleteOffer(pchar->itemID());
                m_agent->SendMissionUpdate(call.client, "quit");
                PySetItemRelease(agentSays, 0, new PyInt(m_agent->GetDeclineRsp(pchar->itemID())));
                PySetItemRelease(agentSays, 1, new PyInt(pchar->itemID()));
                /** @todo  add lp, etc, etc  */
                m_agent->UpdateStandings(call.client, Standings::MissionFailure, offer.important);
            } break;
            case Continue: {    //8
                // not sure what to do here.
            } break;
            case StartResearch: {   //12
                // Show research field choices from agent's agtResearchAgents
                if (m_agent->GetResearchFields().empty()) {
                    PySetItemRelease(agentSays, 0, new PyString("I have no research fields available for you."));
                    agentSays->SetItem(1, PyStatic.NewNone());
                } else {
                    // Check if character already researching with this agent
                    uint32 charID = pchar->itemID();
                    bool alreadyResearching = false;
                    DBQueryResult res;
                    sDatabase.RunQuery(res, "SELECT skillTypeID FROM chrResearch WHERE characterID = %u AND agentID = %u",
                        charID, m_agent->GetID());
                    if (res.GetRowCount() > 0)
                        alreadyResearching = true;

                    if (alreadyResearching) {
                        PySetItemRelease(agentSays, 0, new PyString("You are already conducting research with me."));
                        agentSays->SetItem(1, PyStatic.NewNone());
                    } else {
                        // Return the list of research fields for the client to display
                        // (built here so the "already researching" branch doesn't leak it)
                        PyList* choices = new PyList();
                        for (uint16 skillID : m_agent->GetResearchFields()) {
                            PyTuple* choice = new PyTuple(2);
                            PySetItemRelease(choice, 0, new PyInt(skillID));
                            PySetItemRelease(choice, 1, new PyInt(skillID));
                            choices->AddItem(choice);
                        }
                        PyDict* researchData = new PyDict();
                        researchData->SetItemString("skillTypeID", choices);
                        PySetItemRelease(agentSays, 0, new PyString("What field of research interests you?"));
                        agentSays->SetItem(1, researchData);
                    }
                }
              } break;
            case CancelResearch: {  //13
                uint32 charID = pchar->itemID();
                DBerror err;
                sDatabase.RunQuery(err, "DELETE FROM chrResearch WHERE characterID = %u AND agentID = %u",
                    charID, m_agent->GetID());
                PySetItemRelease(agentSays, 0, new PyString("Research connection terminated. Any unspent research points have been lost."));
                agentSays->SetItem(1, PyStatic.NewNone());
              } break;
            case BuyDatacores: {    //14
                uint32 charID = pchar->itemID();
                DBQueryResult res;
                sDatabase.RunQuery(res,
                    "SELECT skillTypeID, points FROM chrResearch WHERE characterID = %u AND agentID = %u",
                    charID, m_agent->GetID());
                DBResultRow row;
                if (res.GetRow(row)) {
                    uint16 skillID = row.GetInt(0);
                    double points = row.GetDouble(1);
                    // Each datacore costs 100 RP + 10k ISK
                    uint32 maxBuy = (uint32)(points / 100.0);
                    uint32 iskBalance = call.client->GetBalance();
                    uint32 affordable = iskBalance / 10000;
                    if (affordable < maxBuy)
                        maxBuy = affordable;

                    if (maxBuy < 1) {
                        PySetItemRelease(agentSays, 0, new PyString("You don't have enough research points or ISK to buy datacores."));
                        agentSays->SetItem(1, PyStatic.NewNone());
                    } else {
                        // Buy one datacore for simplicity
                        points -= 100.0;
                        call.client->AddBalance(-10000);
                        sDatabase.RunQuery(res,
                            "UPDATE chrResearch SET points = %.2f WHERE characterID = %u AND agentID = %u",
                            points, charID, m_agent->GetID());
                        // Create datacore item in hangar
                        sItemFactory.SetUsingClient(call.client);
                        ItemData itemData(skillID, charID, locTemp, flagHangar, 1);
                        InventoryItemRef iRef = sItemFactory.SpawnItem(itemData);
                        sItemFactory.UnsetUsingClient();
                        if (iRef.get() != nullptr) {
                            PySetItemRelease(agentSays, 0, new PyString("Here is your datacore. Thank you for your contribution to science!"));
                            agentSays->SetItem(1, PyStatic.NewNone());
                        }
                    }
                } else {
                    PySetItemRelease(agentSays, 0, new PyString("You are not currently researching with me."));
                    agentSays->SetItem(1, PyStatic.NewNone());
                }
              } break;
            case LocateCharacter:   //15
            case LocateAccept: {    //16
                // not sure what to do here.
            } break;
            case LocateReject: {    //17
                // not sure what to do here.
            } break;
            case Yes: {             //18
                // not sure what to do here.
            } break;
            case No: {              //19
                // not sure what to do here.
            } break;
            case AcceptChoice:{     //4
                // i think this is for options
            } break;
            case Admin: {           //20
                // not sure what to do here.
            } break;
            default: {
                int32 val = actionID.has_value() ? actionID.value()->value() : 0;
                // Check if this is a research field selection (skillTypeID > 1000)
                if (val > 1000) {
                    const auto& fields = m_agent->GetResearchFields();
                    if (std::find(fields.begin(), fields.end(), val) != fields.end()) {
                        // Start research with this field
                        uint32 charID = pchar->itemID();
                        int8 researchSkill = pchar->GetSkillLevel(val);
                        float agentQuality = m_agent->GetQuality();
                        float agentEffQuality = EvEMath::Agent::EffectiveQuality(agentQuality, 0, charStanding);
                        float ppd = EvEMath::RAM::ResearchPointsPerDay(1.0f, agentEffQuality, researchSkill, 5);
                        DBerror err;
                        sDatabase.RunQuery(err,
                            "INSERT INTO chrResearch (characterID, agentID, skillTypeID, points, pointsPerDay, lastUpdate)"
                            " VALUES (%u, %u, %u, 0, %.2f, %lli)"
                            " ON DUPLICATE KEY UPDATE skillTypeID = %u, pointsPerDay = %.2f, lastUpdate = %lli",
                            charID, m_agent->GetID(), val, ppd, (int64)GetFileTimeNow(),
                            val, ppd, (int64)GetFileTimeNow());
                        PySetItemRelease(agentSays, 0, new PyString("Research has begun. I will inform you of our progress."));
                        agentSays->SetItem(1, PyStatic.NewNone());
                        break;
                    }
                }
                // error
                _log(AGENT__ERROR, "AgentBound::Handle_DoAction() - unhandled buttonID %u", actionID );
                call.client->SendErrorMsg("Internal Server Error. Ref: ServerError xxxxx.");
                PySafeDecRef(dialog);
                PySafeDecRef(agentSays);
                return nullptr;
            }
        }
    } else {
        PySetItemRelease(agentSays, 0, new PyInt(m_agent->GetStandingsRsp(pchar->itemID())));
        PySetItemRelease(agentSays, 1, PyStatic.NewNone() /*new PyInt(pchar->itemID())*/);
    }

    // extraInfo data....
    PyDict* xtraInfo = new PyDict();
        xtraInfo->SetItemString("loyaltyPoints",    new PyInt(LPService::GetLPBalanceForCorp(pchar->itemID(),m_agent->GetCorpID())));  // this is char current LP
        xtraInfo->SetItemString("missionCompleted", new PyBool(missionCompleted));
        xtraInfo->SetItemString("missionQuit",      new PyBool(missionQuit));
        xtraInfo->SetItemString("missionDeclined",  new PyBool(missionDeclined));

    if (agentSays->empty()) {
        agentSays->SetItem(0, PyStatic.NewNone());  // briefingID
        agentSays->SetItem(1, PyStatic.NewNone());  // ContentID
    }
    PyTuple* inner = new PyTuple(2);
        inner->SetItem(0, agentSays);
        inner->SetItem(1, dialog);
    PyTuple* outer = new PyTuple(2);
        outer->SetItem(0, inner);
        outer->SetItem(1, xtraInfo);

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_DoAction RSP:" );
        outer->Dump(AGENT__RSP_DUMP, "    ");
    }

    PySafeDecRef(agentSays);

    return outer;
}

PyResult AgentBound::GetMissionBriefingInfo(PyCallArgs &call) {
    // called from iniate agent convo... should be populated when mission available
    // will return PyNone if no mission avalible
    _log(AGENT__MESSAGE,  "AgentBound::Handle_GetMissionBriefingInfo()");

    MissionOffer offer = MissionOffer();
    if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    switch (offer.stateID) {
        //case Mission::State::Allocated:
        case Mission::State::Accepted:
        case Mission::State::Failed:
        case Mission::State::Completed:
        case Mission::State::Rejected:
        case Mission::State::Defered:
        case Mission::State::Expired: {
            return PyStatic.NewNone();
        }
    }

    // these are found in the client data by MessageIDs ....  i.e.  {[location]objectiveDestinationID.name}
    // contentID is the key for the keywords data on live...not used here
    PyDict* keywords = new PyDict();
        keywords->SetItemString("objectiveLocationID", new PyInt(offer.originID));
        keywords->SetItemString("objectiveLocationSystemID", new PyInt(offer.originSystemID));
        keywords->SetItemString("objectiveTypeID", new PyInt(offer.courierTypeID));
        keywords->SetItemString("objectiveQuantity", new PyInt(offer.courierAmount));
        keywords->SetItemString("objectiveDestinationID", new PyInt(offer.destinationID));
        keywords->SetItemString("objectiveDestinationSystemID", new PyInt(offer.destinationSystemID));
        if (offer.rewardISK) {
            keywords->SetItemString("rewardTypeID", new PyInt(itemTypeCredits));
            keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardISK));
        } else {
            // wouldnt these be in 'extra' or ?
            keywords->SetItemString("rewardTypeID", new PyInt(offer.rewardItemID));
            keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardItemQty));
        }
        keywords->SetItemString("dungeonLocationID", new PyInt(offer.dungeonLocationID));
        keywords->SetItemString("dungeonSolarSystemID", new PyInt(offer.dungeonSolarSystemID));
    PyDict *briefingInfo = new PyDict();
        briefingInfo->SetItemString("ContentID", new PyInt(offer.characterID));
        briefingInfo->SetItemString("Mission Keywords", keywords);
        briefingInfo->SetItemString("Mission Title ID", new PyInt(offer.missionID));
        briefingInfo->SetItemString("Mission Briefing ID", new PyInt(offer.briefingID));
        switch(offer.typeID) {
            case Mission::Type::Courier:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetCourierRes()); break;
            case Mission::Type::Mining:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetMiningRes()); break;
            case Mission::Type::Encounter:
            default:
                briefingInfo->SetItemString("Mission Image", sMissionDataMgr.GetKillRes()); break;
        }
        // decline time OR expiration time.  if not decline then expiration
        briefingInfo->SetItemString("Decline Time", PyStatic.NewNone());   // -1 is generic decline msg
        briefingInfo->SetItemString("Expiration Time", new PyLong(offer.expiryTime) );

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionBriefingInfo() RSP:" );
        briefingInfo->Dump(AGENT__RSP_DUMP, "    ");
    }

    return briefingInfo;
}

PyResult AgentBound::GetMissionKeywords(PyCallArgs &call, PyInt* contentID) {
    // thse are the variables embedded in the messageIDs
    //self.missionArgs[contentID] = self.GetAgentMoniker(agentID).GetMissionKeywords(contentID)
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMissionKeywords() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    /*   none of this really matters as we're not using 'contentID' like live does
    Call_SingleArg args;
    if (!args.Decode(&call.tuple)) {
        _log(SERVICE__ERROR, "%s: Failed to decode arguments.", GetName());
        return nullptr;
    }

    uint32 contentID = PyRep::IntegerValueU32(args.arg);
    if (contentID == 0)
        return PyStatic.NewNone();
    */
    MissionOffer offer = MissionOffer();
    if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    PyDict* keywords = new PyDict();
    keywords->SetItemString("objectiveLocationID", new PyInt(offer.originID));
    keywords->SetItemString("objectiveLocationSystemID", new PyInt(offer.originSystemID));
    keywords->SetItemString("objectiveTypeID", new PyInt(offer.courierTypeID));
    keywords->SetItemString("objectiveQuantity", new PyInt(offer.courierAmount));
    keywords->SetItemString("objectiveDestinationID", new PyInt(offer.destinationID));
    keywords->SetItemString("objectiveDestinationSystemID", new PyInt(offer.destinationSystemID));
    if (offer.rewardISK) {
        keywords->SetItemString("rewardTypeID", new PyInt(itemTypeCredits));
        keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardISK));
    } else {
        // wouldnt these be in 'extra' or ?
        keywords->SetItemString("rewardTypeID", new PyInt(offer.rewardItemID));
        keywords->SetItemString("rewardQuantity", new PyInt(offer.rewardItemQty));
    }
    keywords->SetItemString("dungeonLocationID", new PyInt(offer.dungeonLocationID));
    keywords->SetItemString("dungeonSolarSystemID", new PyInt(offer.dungeonSolarSystemID));

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionKeywords() RSP:" );
        keywords->Dump(AGENT__RSP_DUMP, "    ");
    }

    return keywords;
}

PyResult AgentBound::GetMissionObjectiveInfo(PyCallArgs &call, std::optional <PyInt*> characterID, std::optional <PyInt*> contentID)
{
    // sends charID, contentID (although there's another call without any parameters, hence the optionals used)
    // returns PyDict loaded with mission info  or PyNone
    //  returning mission info sets double-pane view, where PyNone sets single-pane view
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMissionObjectiveInfo() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    MissionOffer offer = MissionOffer();
    if (call.tuple->size() == 0)
        if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
            return PyStatic.NewNone();

        switch (offer.stateID) {
            //case Mission::State::Allocated:
            case Mission::State::Accepted:
            //case Mission::State::Failed:
            case Mission::State::Completed:
            case Mission::State::Rejected:
            case Mission::State::Defered:
            case Mission::State::Expired: {
                return PyStatic.NewNone();
            }
        }

    return GetMissionObjectiveInfo(call.client, offer);
}

PyResult AgentBound::GetMyJournalDetails(PyCallArgs &call) {
    //parallelCalls.append((sm.GetService('agents').GetAgentMoniker(agentID).GetMyJournalDetails, ()))
    //missionState, importantMission, missionType, missionName, agentID, expirationTime, bookmarks, remoteOfferable, remoteCompletable = each
    // this is to update ONLY info with this agent....
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMyJournalDetails() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    PyTuple *tuple = new PyTuple(2);
    //missions:
    PyList* missions = new PyList();
    MissionOffer offer = MissionOffer();
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        if (offer.stateID < Mission::State::Completed) {
            PyTuple* mData = new PyTuple(9);
                PySetItemRelease(mData, 0, new PyInt(offer.stateID)); //missionState  .. these may be wrong also.
                PySetItemRelease(mData, 1, new PyInt(offer.important?1:0)); //importantMission  -- integer boolean
                PySetItemRelease(mData, 2, new PyString(sMissionDataMgr.GetTypeLabel(offer.typeID))); //missionTypeLabel
                PySetItemRelease(mData, 3, new PyString(offer.name)); //missionName
                PySetItemRelease(mData, 4, new PyInt(offer.agentID)); //agentID
                PySetItemRelease(mData, 5, new PyLong(offer.expiryTime)); //expirationTime
                mData->SetItem(6, offer.bookmarks->Clone()); //bookmarks -- if populated, this is PyList of PyDicts as defined below...
                PySetItemRelease(mData, 7, new PyBool(offer.remoteOfferable)); //remoteOfferable
                PySetItemRelease(mData, 8, new PyBool(offer.remoteCompletable)); //remoteCompletable
            missions->AddItem(mData);
        }
    }
    tuple->SetItem(0, missions);

    //research: query chrResearch for this character + agent
    PyList* research = new PyList();
    {
        uint32 charID = call.client->GetCharacterID();
        uint32 agentID = m_agent->GetID();
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
        "SELECT r.agentID, r.skillTypeID, r.pointsPerDay, r.points, "
        "  a.level, a.quality, a.locationID "
            "FROM chrResearch r "
            "JOIN agtAgents a ON r.agentID = a.agentID "
            "WHERE r.characterID = %u AND r.agentID = %u", charID, agentID))
        {
            DBResultRow row;
            while (res.GetRow(row)) {
                PyTuple* rData = new PyTuple(7);
                PySetItemRelease(rData, 0, new PyInt(row.GetUInt(0)));   // agentID
                PySetItemRelease(rData, 1, new PyInt(row.GetUInt(1)));   // skillTypeID
                PySetItemRelease(rData, 2, new PyFloat(row.GetFloat(2)));// ppd
                PySetItemRelease(rData, 3, new PyFloat(row.GetFloat(3)));// points
                PySetItemRelease(rData, 4, new PyInt(row.GetUInt(4)));   // level
                PySetItemRelease(rData, 5, new PyInt(row.GetInt(5)));    // quality
                PySetItemRelease(rData, 6, new PyInt(row.GetUInt(6)));   // stationID
                research->AddItem(rData);
            }
        }
    }
    tuple->SetItem(1, research);

    if (is_log_enabled(AGENT__RSP_DUMP))
        tuple->Dump(AGENT__RSP_DUMP, "   ");
    return tuple;
}

PyResult AgentBound::GetMissionJournalInfo(PyCallArgs &call, std::optional <PyInt*> characterID, std::optional <PyInt*> contentID) {
    //called on rclick in journal to "read details"
    //ret = self.GetAgentMoniker(agentID).GetMissionJournalInfo(charID, contentID)
    _log(AGENT__DUMP,  "AgentBound::Handle_GetMissionJournalInfo() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    MissionOffer offer = MissionOffer();
    if (!m_agent->HasMission(call.client->GetCharacterID(), offer))
        return PyStatic.NewNone();

    PyDict* journalInfo = new PyDict();
    journalInfo->SetItemString("contentID", new PyInt(offer.characterID));
    journalInfo->SetItemString("missionNameID", new PyInt(offer.missionID));
    journalInfo->SetItemString("briefingTextID", new PyInt(offer.briefingID));
    journalInfo->SetItemString("missionState", new PyInt(offer.stateID));
    journalInfo->SetItemString("expirationTime", new PyLong(offer.expiryTime) );
    journalInfo->SetItemString("objectives", GetMissionObjectiveInfo(call.client, offer));
    switch(offer.typeID) {
        case Mission::Type::Courier:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetCourierRes()); break;
        case Mission::Type::Mining:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetMiningRes()); break;
        case Mission::Type::Encounter:
        default:
            journalInfo->SetItemString("missionImage", sMissionDataMgr.GetKillRes()); break;
    }

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionJournalInfo() RSP:" );
        journalInfo->Dump(AGENT__RSP_DUMP, "    ");
    }

    return journalInfo;
}

PyDict* AgentBound::GetMissionObjectiveInfo(Client* pClient, MissionOffer& offer)
{
    PyDict* objectiveData = new PyDict();
    objectiveData->SetItemString("missionTitleID", new PyInt(offer.missionID));
    objectiveData->SetItemString("contentID", new PyInt(offer.characterID));
    objectiveData->SetItemString("importantStandings", new PyInt(offer.important));     // boolean integer
    // will need to test for this to set correctly.....
    if (pClient->IsMissionComplete(offer)) {     // Mission::Status:: data here 0=no, 1=yes, 2=cheat
        objectiveData->SetItemString("completionStatus", new PyInt(Mission::Status::Complete));
    } else {
        objectiveData->SetItemString("completionStatus", new PyInt(Mission::Status::Incomplete));
    }
    objectiveData->SetItemString("missionState", new PyInt(offer.stateID /*Mission::State::Offered*/));   // Mission::State:: data here for agentGift populating.  Accepted/failed to display gift items as accepted
    objectiveData->SetItemString("loyaltyPoints", new PyInt(offer.rewardLP));
    objectiveData->SetItemString("researchPoints", PyStatic.NewInt(0));

    /*  this puts title/msg at bottom of right pane
    if (offer.stateID == Mission::State::Accepted)
        if (offer.typeID == Mission::Type::Courier) {
            PyTuple* missionExtra = new PyTuple(2);  // this is tuple(2)  headerID, bodyID    -- std locale msgIDs
                PySetItemRelease(missionExtra, 0, new PyString("Reminder...."));   // this should be separate title from mission name
                PySetItemRelease(missionExtra, 1, new PyString("Remember to get the %s from your hangar before you leave.", call.client->GetCourierItemRef(m_agent->GetID())->name()));   // this is additional info about mission, etc.
            objectiveData->SetItemString("missionExtra", missionExtra);
        } */

    PyList* locList = new PyList();    // tuple of list of locationIDs (pickup and dropoff)
        locList->AddItem(new PyInt(offer.originSystemID));
        locList->AddItem(new PyInt(offer.destinationSystemID));
    objectiveData->SetItemString("locations", locList);

    PyList* giftList = new PyList();    // this is list of tuple(3)  typeID, quantity, extra
    /*
    PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
        extra->SetItemString("specificItemID", PyStatic.NewNone());
        extra->SetItemString("blueprintInfo", PyStatic.NewNone());
    PyTuple* agentGift = new PyTuple(3);
        agentGift->SetItem(0, PyStatic.NewNone());
        agentGift->SetItem(1, PyStatic.NewNone());
        agentGift->SetItem(2, extra);
        giftList->AddItem(agentGift);
    */
    objectiveData->SetItemString("agentGift", giftList);

    PyList* normList = new PyList();    // this is list of tuple(3)  typeID, quantity, extra
    if (offer.rewardISK) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
            //extra->SetItemString("specificItemID", PyStatic.NewNone());
            //extra->SetItemString("blueprintInfo", PyStatic.NewNone());
        PyTuple* normalRewards = new PyTuple(3);
            PySetItemRelease(normalRewards, 0, new PyInt(itemTypeCredits));
            PySetItemRelease(normalRewards, 1, new PyInt(offer.rewardISK));
            normalRewards->SetItem(2, extra);
        normList->AddItem(normalRewards);
    }
    if (offer.rewardItemID) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
            //extra->SetItemString("specificItemID", PyStatic.NewNone());
            //extra->SetItemString("blueprintInfo", PyStatic.NewNone());
        PyTuple* normalRewards = new PyTuple(3);
            PySetItemRelease(normalRewards, 0, new PyInt(offer.rewardItemID));
            PySetItemRelease(normalRewards, 1, new PyInt(offer.rewardItemQty));
            normalRewards->SetItem(2, extra);
        normList->AddItem(normalRewards);
    }
    objectiveData->SetItemString("normalRewards", normList);

    PyList* collateralList = new PyList(); // this is list of tuple(3)  typeID, quantity, extra
    /*
    PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
        extra->SetItemString("specificItemID", PyStatic.NewNone());
        extra->SetItemString("blueprintInfo", PyStatic.NewNone());
    PyTuple* collateral = new PyTuple(3);
        collateral->SetItem(0, PyStatic.NewNone());
        collateral->SetItem(1, PyStatic.NewNone());
        collateral->SetItem(2, extra);
        */
    objectiveData->SetItemString("collateral", collateralList);

    PyList* bonusList = new PyList();   // this is list of tuple(4)  timeRemaining, typeID, quantity, extra
    if (offer.bonusTime > 0) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
            //extra->SetItemString("specificItemID", PyStatic.NewNone());
            //extra->SetItemString("blueprintInfo", PyStatic.NewNone());
        PyTuple* bonusRewards = new PyTuple(4);
        int64 bonusLeft = (int64)offer.bonusTime * EvE::Time::Minute;
        if (offer.dateAccepted > 0) {
            bonusLeft -= (GetFileTimeNow() - offer.dateAccepted);
            if (bonusLeft < 0)
                bonusLeft = 0;
        }
        PySetItemRelease(bonusRewards, 0, new PyLong(bonusLeft));
            PySetItemRelease(bonusRewards, 1, new PyInt(itemTypeCredits));
            PySetItemRelease(bonusRewards, 2, new PyInt(offer.bonusISK ? offer.bonusISK : offer.rewardISK));
            bonusRewards->SetItem(3, extra);
        bonusList->AddItem(bonusRewards);
    }
    // bonusList can be multiple items, usualy only item or isk for time bonus
    if (false/*bonus2*/) {
        PyDict* extra = new PyDict();    // 'extra' is either specificItemID or blueprint data.
            //extra->SetItemString("specificItemID", PyStatic.NewNone());
            //extra->SetItemString("blueprintInfo", PyStatic.NewNone());
        PyTuple* bonusRewards2 = new PyTuple(4);
            PySetItemRelease(bonusRewards2, 0, new PyLong(12000000000)); //20m
            PySetItemRelease(bonusRewards2, 1, new PyInt(itemTypeTrit));
            PySetItemRelease(bonusRewards2, 2, new PyInt(offer.rewardISK));
            bonusRewards2->SetItem(3, extra);
        bonusList->AddItem(bonusRewards2);
    }
    objectiveData->SetItemString("bonusRewards", bonusList);
    /*  for collateral and rewards, as follows...
    typeID, quantity, extra in objectiveData['normalRewards']
    typeID, quantity, extra in objectiveData['collateral']
    typeID, quantity, extra in objectiveData['agentGift']
    or
    timeRemaining, typeID, quantity, extra in objectiveData['bonusRewards']

        specificItemID = extra.get('specificItemID', 0)
        blueprintInfo = extra.get('blueprintInfo', None)
        */

    objectiveData->SetItemString("objectives", GetMissionObjectives(pClient, offer));
    PyList* dunList = new PyList();  // this is a list of dunData dicts
    // Encounter missions spawn a tracked hostile cluster at a known point; give
    // the client a dungeon objective with those coordinates so it can build the
    // warp-to-site link ("Objective: <location>"). Without this the player has no
    // way to reach the spawned targets.
    bool isSiteMission = (offer.typeID == Mission::Type::Encounter || offer.typeID == Mission::Type::Mining
        || offer.typeID == Mission::Type::Anomic
        || ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos) && offer.objectiveType == 3)
        || (offer.typeID == Mission::Type::Storyline && offer.courierTypeID == 0));
    if (isSiteMission && EncounterSpawnServer::Get() != nullptr) {
        GPoint tgt;
        if (EncounterSpawnServer::Get()->GetTargetPosition(offer.offerID, tgt)) {
            _log(AGENT__ERROR, "Mission objective: offer %u (%s) site at %.0f,%.0f,%.0f",
                 offer.offerID, sMissionDataMgr.GetTypeLabel(offer.typeID).c_str(), tgt.x, tgt.y, tgt.z);
            uint32 destSys = offer.destinationSystemID != 0 ? offer.destinationSystemID : offer.dungeonSolarSystemID;
            if (destSys != 0) {
                PyDict* dunData = new PyDict();
                    dunData->SetItemString("dungeonID", new PyInt(1000));
                    dunData->SetItemString("completionStatus", new PyInt(
                        pClient->IsMissionComplete(offer) ? 1 : 0));
                    dunData->SetItemString("optional", new PyInt(0));
                    dunData->SetItemString("objectiveCompleted", new PyBool(false));
                    dunData->SetItemString("ownerID", new PyInt(m_agent->GetID()));
                    dunData->SetItemString("shipRestrictions", PyStatic.NewInt(0));   // 0=normal
                PyDict* loc = new PyDict();
                    loc->SetItemString("typeID", new PyInt(offer.destinationTypeID ? offer.destinationTypeID : m_agent->GetLocTypeID()));
                    loc->SetItemString("locationID", new PyInt(destSys));
                    loc->SetItemString("locationType", new PyString("dungeon"));
                    loc->SetItemString("solarsystemID", new PyInt(destSys));
                    loc->SetItemString("agentID", new PyInt(m_agent->GetID()));
                    loc->SetItemString("referringAgentID", new PyInt(m_agent->GetID()));
                    loc->SetItemString("shipTypeID", new PyInt(m_agent->GetTypeID()));
                    PyTuple* coords = new PyTuple(3);
                        PySetItemRelease(coords, 0, new PyFloat(tgt.x));
                        PySetItemRelease(coords, 1, new PyFloat(tgt.y));
                        PySetItemRelease(coords, 2, new PyFloat(tgt.z));
                    loc->SetItemString("coords", coords);
                dunData->SetItemString("location", loc);
                dunList->AddItem(dunData);
            }
        } else {
            _log(AGENT__ERROR, "Mission objective: offer %u has NO tracked site (nothing spawned).",
                 offer.offerID);
        }
    }
    objectiveData->SetItemString("dungeons", dunList);
    /* dunData data....
     * dungeonID
     * completionStatus
     * optional
     * briefingMessage
     * objectiveCompleted
     * ownerID
     * location
        location['locationID']
        location['locationType']
        location['solarsystemID']
        location['coords']
        location['agentID']
        ?location['referringAgentID']
        ?location['shipTypeID']
     * shipRestrictions  0=normal 1=special with link to *something else*
     */

    if (is_log_enabled(AGENT__RSP_DUMP)) {
        _log(AGENT__RSP_DUMP, "AgentBound::Handle_GetMissionObjectiveInfo() RSP:" );
        objectiveData->Dump(AGENT__RSP_DUMP, "    ");
    }

    return objectiveData;
}

PyTuple* AgentBound::GetMissionObjectives(Client* pClient, MissionOffer& offer)
{
    // set mission objectiveData based on mission type.
    uint32 destSysID = offer.destinationSystemID;
    if (destSysID == 0) {
        destSysID = m_agent->GetSystemID();
    }
    // fetch-type missions (mining/trade/research) hand the goods to the agent:
    // the drop-off is the agent's own station, not the mission's site system.
    uint32 dropoffID = offer.destinationID;
    if ((offer.typeID == Mission::Type::Mining || offer.typeID == Mission::Type::Trade || offer.typeID == Mission::Type::Research || offer.typeID == Mission::Type::Data
         || ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos) && offer.objectiveType == 5))
        && m_agent->GetStationID() != 0 && sDataMgr.IsStation(m_agent->GetStationID())) {
        dropoffID = m_agent->GetStationID();
        destSysID = m_agent->GetSystemID();
    } else if ((offer.typeID == Mission::Type::EpicArc || offer.typeID == Mission::Type::Cosmos)
               && offer.objectiveType == 5 && m_agent->GetStationID() == 0) {
        // agent in space: hand the goods in at the agent's location (system)
        dropoffID = m_agent->GetLocationID();
        destSysID = m_agent->GetSystemID();
    }
    PyDict* dropoffLocation = new PyDict();
    dropoffLocation->SetItemString("typeID", new PyInt(offer.destinationTypeID) );
    dropoffLocation->SetItemString("locationID", new PyInt(dropoffID) );
    dropoffLocation->SetItemString("locationType", new PyNone());
    dropoffLocation->SetItemString("solarsystemID", new PyInt(destSysID) );
    // If destination is not a station (e.g. in-space agent), add extra fields
    if (!sDataMgr.IsStation(dropoffID)) {
        dropoffLocation->SetItemString("shipTypeID", new PyInt(offer.destinationTypeID) );
        dropoffLocation->SetItemString("agentID", new PyInt(offer.destinationOwnerID) );
        PyTuple* coords = new PyTuple(3);
            PySetItemRelease(coords, 0, new PyFloat(0));
            PySetItemRelease(coords, 1, new PyFloat(0));
            PySetItemRelease(coords, 2, new PyFloat(0));
        dropoffLocation->SetItemString("coords", coords);
        dropoffLocation->SetItemString("referringAgentID", new PyInt(offer.agentID) );
        // COSMOS/arc agent in space: use the spawned agent's real position and id so
        // the journal objective can offer a warp link to the agent
        if (m_agent->GetStationID() == 0) {
            SystemEntity* agSE = nullptr;
            auto& systems = sEntityList.GetSystems();
            auto sit = systems.find(m_agent->GetSystemID());
            if (sit != systems.end())
                agSE = sit->second->GetCosmosAgentSE(m_agent->GetID());
            if (agSE != nullptr) {
                dropoffLocation->SetItemString("typeID", new PyInt(agSE->GetTypeID()) );
                dropoffLocation->SetItemString("agentID", new PyInt(m_agent->GetID()) );
                PyTuple* agCoords = new PyTuple(3);
                    PySetItemRelease(agCoords, 0, new PyFloat(agSE->GetPosition().x));
                    PySetItemRelease(agCoords, 1, new PyFloat(agSE->GetPosition().y));
                    PySetItemRelease(agCoords, 2, new PyFloat(agSE->GetPosition().z));
                dropoffLocation->SetItemString("coords", agCoords);
            }
        }
    }

    PyTuple* objectives = new PyTuple(1);
    switch (offer.typeID) {
        case Mission::Type::Courier:
        case Mission::Type::Storyline: {
            PyDict* pickupLocation = new PyDict();
                pickupLocation->SetItemString("typeID", new PyInt(m_agent->GetLocTypeID()) );
                pickupLocation->SetItemString("locationID", new PyInt(offer.originID) );
                pickupLocation->SetItemString("locationType", new PyNone());
                pickupLocation->SetItemString("solarsystemID", new PyInt(offer.originSystemID) );
            PyDict* cargo = new PyDict();
                cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));
            PyTuple* objData = new PyTuple(5);
                PySetItemRelease(objData, 0, new PyInt(offer.originOwnerID));
                objData->SetItem(1, pickupLocation);
                PySetItemRelease(objData, 2, new PyInt(offer.destinationOwnerID));
                objData->SetItem(3, dropoffLocation);
                objData->SetItem(4, cargo);
            PyTuple* objType = new PyTuple(2);
                PySetItemRelease(objType, 0, new PyString("transport"));
                objType->SetItem(1, objData);
            objectives->SetItem(0, objType);
        } break;
        case Mission::Type::Encounter: {
            // Encounter missions: the objective is the site itself (a dungeon
            // entry with a warp link, filled in GetMissionObjectiveInfo). No
            // 'fetch' objective — that rendering is wrong for a kill/clear site.
            PyTuple* objType = new PyTuple(2);
                PySetItemRelease(objType, 0, new PyString("none"));
                PySetItemRelease(objType, 1, new PyDict());
            objectives->SetItem(0, objType);
        } break;
        case Mission::Type::Trade:
        case Mission::Type::Research:
        case Mission::Type::Mining:
        case Mission::Type::Data: {
            PyDict* cargo = new PyDict();
                cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));
            PyTuple* objData = new PyTuple(3);
                PySetItemRelease(objData, 0, new PyInt(offer.destinationOwnerID));
                objData->SetItem(1, dropoffLocation);
                objData->SetItem(2, cargo);
            PyTuple* objType = new PyTuple(2);
                PySetItemRelease(objType, 0, new PyString("fetch"));
                objType->SetItem(1, objData);
            objectives->SetItem(0, objType);
        } break;
        case Mission::Type::EpicArc:
        case Mission::Type::Cosmos: {
            // procedural arc/cosmos objectives: "report to <agent>" is the native
            // 'agent' objective (portrait + location + warp link)
            uint32 daID = offer.destinationAgentID;
            if (daID == 0 && offer.missionID != 0) {
                DBQueryResult dres;
                if (sDatabase.RunQuery(dres,
                        "SELECT destinationAgentID FROM missionObjectives WHERE missionID = %u",
                        offer.missionID)) {
                    DBResultRow drow;
                    if (dres.GetRow(drow))
                        daID = drow.GetUInt(0);
                }
            }
            if (offer.objectiveType == 1 && daID != 0) {
                Agent* dstAg = sEntityList.GetAgent(daID);
                PyDict* locWrap = (dstAg != nullptr) ? dstAg->GetLocationWrap() : nullptr;
                if (locWrap == nullptr) {
                    locWrap = new PyDict();
                    locWrap->SetItemString("typeID", new PyInt(0));
                    locWrap->SetItemString("locationID", new PyInt(offer.destinationSystemID));
                    locWrap->SetItemString("solarsystemID", new PyInt(offer.destinationSystemID));
                }
                PyTuple* objData = new PyTuple(2);
                    PySetItemRelease(objData, 0, new PyInt(daID));
                    objData->SetItem(1, locWrap);
                PyTuple* objType = new PyTuple(2);
                    PySetItemRelease(objType, 0, new PyString("agent"));
                    objType->SetItem(1, objData);
                objectives->SetItem(0, objType);
            } else if (offer.objectiveType == 2 && offer.courierTypeID != 0) {
                PyDict* pickupLocation = new PyDict();
                    pickupLocation->SetItemString("typeID", new PyInt(m_agent->GetLocTypeID()) );
                    pickupLocation->SetItemString("locationID", new PyInt(offer.originID) );
                    pickupLocation->SetItemString("locationType", PyStatic.NewNone());
                    pickupLocation->SetItemString("solarsystemID", new PyInt(offer.originSystemID) );
                PyDict* cargo = new PyDict();
                    cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                    cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                    cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                    cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));
                PyTuple* objData = new PyTuple(5);
                    PySetItemRelease(objData, 0, new PyInt(offer.originOwnerID));
                    objData->SetItem(1, pickupLocation);
                    PySetItemRelease(objData, 2, new PyInt(offer.destinationOwnerID));
                    objData->SetItem(3, dropoffLocation);
                    objData->SetItem(4, cargo);
                PyTuple* objType = new PyTuple(2);
                    PySetItemRelease(objType, 0, new PyString("transport"));
                    objType->SetItem(1, objData);
                objectives->SetItem(0, objType);
            } else if (offer.objectiveType == 5 && offer.courierTypeID != 0) {
                // fetch: bring the item to the agent's station (same rendering as
                // the mining/trade/research hand-in objectives)
                PyDict* cargo = new PyDict();
                    cargo->SetItemString("hasCargo", new PyBool(pClient->ContainsTypeQty(offer.courierTypeID, offer.courierAmount)));
                    cargo->SetItemString("typeID", new PyInt(offer.courierTypeID));
                    cargo->SetItemString("quantity", new PyInt(offer.courierAmount));
                    cargo->SetItemString("volume", new PyFloat(offer.courierItemVolume * offer.courierAmount));
                PyTuple* objData = new PyTuple(3);
                    PySetItemRelease(objData, 0, new PyInt(offer.destinationOwnerID));
                    objData->SetItem(1, dropoffLocation);
                    objData->SetItem(2, cargo);
                PyTuple* objType = new PyTuple(2);
                    PySetItemRelease(objType, 0, new PyString("fetch"));
                    objType->SetItem(1, objData);
                objectives->SetItem(0, objType);
            } else {
                PyTuple* objType = new PyTuple(2);
                    PySetItemRelease(objType, 0, new PyString("none"));
                    objType->SetItem(1, new PyDict());
                objectives->SetItem(0, objType);
            }
        } break;
        case Mission::Type::Anomic:
        case Mission::Type::Burner:
        case Mission::Type::Tutorial:
        default: {
            // safety net for unknown/invalid mission types
            PyTuple* objType = new PyTuple(2);
                PySetItemRelease(objType, 0, new PyString("none"));
                PySetItemRelease(objType, 1, new PyDict());
            objectives->SetItem(0, objType);
        } break;
    }

    // cleanup: clear() releases the dict's own refs to its items — when the dict
    // was stored (Courier/Trade/Mining) the items survive via the tuple's refs;
    // when it wasn't (Encounter/default) this actually frees them instead of leaking.
    dropoffLocation->clear();
    PySafeDecRef(dropoffLocation);

    return objectives;

    /*  objectives data...
    if objType == 'agent':      -- report to agent
        agentID, agentLocation = objData
        agentLocation['locationID']
        agentLocation['locationType']
        agentLocation['solarsystemID']
        if not in station
            agentLocation['coords']
            agentLocation['agentID']
            ?agentLocation['referringAgentID']
            ?agentLocation['shipTypeID']

    elif objType == 'transport':        -- courier and trade missions
        pickupOwnerID, pickupLocation, dropoffOwnerID, dropoffLocation, cargo = objData
        pickupLocation['locationID']
        pickupLocation['locationType']
        pickupLocation['solarsystemID']
        dropoffLocation['locationID']
        dropoffLocation['locationType']
        dropoffLocation['solarsystemID']
        if not in station
            dropoffLocation['coords']
            dropoffLocation['agentID']
            ?dropoffLocation['referringAgentID']
            ?dropoffLocation['shipTypeID']
        cargo['hasCargo']
        cargo['typeID']
        cargo['volume']
        cargo['quantity']

    elif objType == 'fetch':            -- encounter and mining missions
        dropoffOwnerID, dropoffLocation, cargo = objData
        dropoffLocation['locationID']
        dropoffLocation['locationType']
        dropoffLocation['solarsystemID']
        if not in station
            dropoffLocation['coords']
            dropoffLocation['agentID']
            ?dropoffLocation['referringAgentID']
            ?dropoffLocation['shipTypeID']
        cargo['hasCargo']
        cargo['typeID']
        cargo['volume']
        cargo['quantity']
        */
}


/**     ***********************************************************************
 * @note   these do absolutely nothing at this time....
 */

PyResult AgentBound::GetDungeonShipRestrictions(PyCallArgs &call, PyInt* dungeonID) {
    _log(AGENT__DUMP,  "AgentBound::Handle_GetDungeonShipRestrictions() - size=%lli", call.tuple->size());
    // Return ship restriction data for this dungeon
    // Client expects: {allowedGroups: [groupID, ...], maxShipSize: int}
    PyDict* result = new PyDict();
    PyList* allowedGroups = new PyList();
    // Allow all combat-capable ship groups by default
    allowedGroups->AddItem(new PyInt(25));   // Frigate
    allowedGroups->AddItem(new PyInt(26));   // Cruiser
    allowedGroups->AddItem(new PyInt(27));   // Battleship
    allowedGroups->AddItem(new PyInt(419));  // Battlecruiser
    allowedGroups->AddItem(new PyInt(420));  // Destroyer
    allowedGroups->AddItem(new PyInt(28));   // Industrial
    result->SetItemString("allowedGroups", allowedGroups);
    result->SetItemString("maxShipSize", new PyInt(4));  // 1=frig, 2=destroyer, 3=cruiser, 4=BC, 5=BS
    return result;
}

PyResult AgentBound::RemoveOfferFromJournal(PyCallArgs &call) {
    _log(AGENT__DUMP,  "AgentBound::Handle_RemoveOfferFromJournal() - size=%lli", call.tuple->size());
    call.Dump(AGENT__DUMP);

    Client* pClient = call.client;
    if (pClient != nullptr)
        m_agent->DeleteOffer(pClient->GetCharacterID());

    return PyStatic.NewNone();
}

PyResult AgentBound::GetOfferJournalInfo(PyCallArgs &call) {
    _log(AGENT__DUMP,  "AgentBound::Handle_GetOfferJournalInfo() - size=%lli", call.tuple->size());
    // Return active mission offer info for this agent
    MissionOffer offer;
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        PyDict* result = new PyDict();
        result->SetItemString("missionState", new PyInt(offer.stateID));
        result->SetItemString("expirationTime", new PyLong(offer.expiryTime));
        return result;
    }
    return PyStatic.NewNone();
}

PyResult AgentBound::GetEntryPoint(PyCallArgs &call) {
    _log(AGENT__DUMP,  "AgentBound::Handle_GetEntryPoint() - size=%lli", call.tuple->size());
    // Return mission entry point (solar system to go to)
    MissionOffer offer;
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        uint32 missionID = offer.missionID;
        DBQueryResult res;
        if (sDatabase.RunQuery(res,
            "SELECT solarSystemID FROM agtMissionEntryPoints WHERE missionID = %u", missionID))
        {
            DBResultRow row;
            if (res.GetRow(row)) {
                PyDict* result = new PyDict();
                result->SetItemString("solarSystemID", new PyInt(row.GetUInt(0)));
                return result;
            }
        }
    }
    // Fallback: return agent's solar system
    PyInt* sysID = sDataMgr.GetAgentSystemID(m_agent->GetID());
    if (sysID != nullptr) {
        PyDict* result = new PyDict();
        // SetItemString steals our reference — no clone, no leak
        result->SetItemString("solarSystemID", sysID);
        return result;
    }
    return PyStatic.NewNone();
}

PyResult AgentBound::GotoLocation(PyCallArgs &call, PyInt* locationType, PyInt* locationNumber, PyInt* referringAgentID) {
    _log(AGENT__DUMP,  "AgentBound::Handle_GotoLocation() - size=%lli", call.tuple->size());
    // Create a bookmark to the mission destination and add to journal
    MissionOffer offer;
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        call.client->SendNotifyMsg("Destination marked in journal.");
    }
    return PyStatic.NewNone();
}

PyResult AgentBound::WarpToLocation(PyCallArgs &call, PyWString* locationType, PyInt* locationNumber, PyFloat* warpRange, PyBool* fleet, PyInt* referringAgentID) {
    _log(AGENT__DUMP,  "AgentBound::Handle_WarpToLocation() - size=%lli", call.tuple->size());
    // Initiate warp to the mission destination. For an Encounter mission the
    // 'dungeon' location points at the tracked target cluster — warp the client
    // to the real coordinates the targets were spawned at (not a random point).
    MissionOffer offer;
    if (m_agent->HasMission(call.client->GetCharacterID(), offer)) {
        Client* pClient = call.client;
        if (pClient != nullptr && pClient->IsInSpace() && pClient->GetShipSE() != nullptr) {
            GPoint dest;
            bool havePos = false;
            if (offer.typeID == Mission::Type::Encounter && EncounterSpawnServer::Get() != nullptr) {
                havePos = EncounterSpawnServer::Get()->GetTargetPosition(offer.offerID, dest);
                if (!havePos) {
                    // Only an INTERRUPTED encounter is missing from the tracker. A cleared
                    // one is gone from it as well (the entry is dropped when the last target
                    // dies), and re-spawning there would reset the counters and turn a
                    // finished mission back into an uncompletable one.
                    bool alreadyCleared = false;
                    DBQueryResult cres;
                    DBResultRow crow;
                    if (sDatabase.RunQuery(cres,
                            "SELECT missionNPCs, missionNPCsKilled FROM agtOffers WHERE offerID = %u",
                            offer.offerID)
                        && cres.GetRow(crow))
                        alreadyCleared = (crow.GetUInt(0) > 0 && crow.GetUInt(1) >= crow.GetUInt(0));

                    // A downtime wipes the in-memory tracker, and the target cluster dies
                    // with it. On live the whole complex respawned and the encounter simply
                    // started over, so do exactly that here. The previous fallback warped
                    // the pilot to a random point in deep space with nothing in it, while
                    // the offer still tracked N targets in the database - a mission that
                    // could never be completed, with no way to re-trigger the spawn because
                    // the mission was already accepted.
                    if (!alreadyCleared) {
                        uint32 respawned = EncounterSpawnServer::Get()->SpawnEncounterForOffer(
                            offer, offer.destinationSystemID, GPoint(0.0, 0.0, 0.0));
                        if (respawned > 0) {
                            havePos = EncounterSpawnServer::Get()->GetTargetPosition(offer.offerID, dest);
                            _log(AGENT__MESSAGE, "WarpToLocation: encounter %u had no live targets, respawned %u - starting over.",
                                 offer.offerID, respawned);
                        }
                    }
                }
            }
            if (!havePos) {
                // Fallback: agent's own solar system origin (old behaviour was a
                // random deep-space point; better to go to the site's system hub).
                dest.x = MakeRandomFloat(-1.5e13, 1.5e13);
                dest.y = MakeRandomFloat(-1.5e13, 1.5e13);
                dest.z = MakeRandomFloat(-1.5e13, 1.5e13);
            }
            pClient->GetShipSE()->DestinyMgr()->WarpTo(dest, 0);
            _log(AGENT__MESSAGE, "WarpToLocation: warping to mission %u offer %u target (%.0f,%.0f,%.0f).",
                 offer.missionID, offer.offerID, dest.x, dest.y, dest.z);
        }
    }
    return PyStatic.NewNone();
}
