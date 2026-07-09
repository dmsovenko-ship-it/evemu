/**
 * @name AllianceDB.h
 *      database methods for alliance data
 *
 * @author Allan
 * updates James
 * @date 24 May 2019
 *
 */


#ifndef EVE_ALLIANCE_ALLIANCEDB_H
#define EVE_ALLIANCE_ALLIANCEDB_H


#include "ServiceDB.h"

#include "alliance/AllianceData.h"
#include "packets/CorporationPkts.h"


struct OfficeData;

class Client;
class PyRep;
class PyObject;

class AllianceDB
{
public:

    PyRep* GetEmploymentRecord(uint32 corpID);
    bool AddEmployment(uint32 allyID, uint32 corpID);

    bool CreateAlliance(std::string name, std::string shortName, std::string description, std::string url, Client* pClient, uint32& allyID, uint32& corpID);

    PyRep* GetBulletins(uint32 allyID);
    void AddBulletin(uint32 allyID, uint32 ownerID, uint32 cCharID, const std::string& title, const std::string& body);
    static void EditBulletin(uint32 bulletinID, uint32 eCharID, int64 eDataTime, std::string& title, std::string& body);
    static void DeleteBulletin(uint32 bulletinID);

    static PyRep *GetMyApplications(uint32 allyID);
    PyRep *GetApplications(uint32 allyID);

    PyRep *GetAlliance(uint32 allyID);

    PyRep* GetContacts(uint32 allyID);
    void AddContact(uint32 ownerID, int32 contactID, int32 relationshipID);
    void UpdateContact(int32 relationshipID, uint32 contactID, uint32 ownerID);
    void RemoveContact(uint32 contactID, uint32 ownerID);
    void UpdateContactLabelMask(uint32 ownerID, uint32 contactID, uint32 mask, bool add);

    PyRep* GetLabels(uint32 allyID);
    void SetLabel(uint32 allyID, uint32 color, std::string name);
    void EditLabel(uint32 allyID, uint32 labelID, uint32 color, std::string name);
    void DeleteLabel(uint32 allyID, uint32 labelID);

    bool InsertApplication(Alliance::ApplicationInfo& aInfo);
    bool UpdateApplication(const Alliance::ApplicationInfo& aInfo);
    bool DeleteApplication(const Alliance::ApplicationInfo& aInfo);
    bool GetCurrentApplicationInfo(uint32 allyID, uint32 corpID, Alliance::ApplicationInfo& aInfo);

    bool IsShortNameTaken(std::string shortName);
    bool UpdateCorpAlliance(uint32 allyID, uint32 corpID);
    void DeleteMember(uint32 allyID, uint32 corpID);
    void DeclareExecutorSupport(uint32 corpID, uint32 chosenExecutor);
    void UpdateAlliance(uint32 allyID, std::string description, std::string url);
    void SetTaxRate(uint32 allyID, double taxRate);

    PyRep* GetMembers(uint32 allyID);
    PyRep* GetAllianceMembers(uint32 allyID);
    PyRep* GetRankedAlliances();

    static uint32 GetExecutorID(uint32 allyID);
    bool CreateAllianceChangePacket(OnAllianceChanged &ac, uint32 oldAllyID, uint32 newAllyID);

    // Alliance voting
    bool AddVoteCase(uint32 allyID, const std::string& voteCaseText, const std::string& description,
                     uint32 voteType, int64 startDateTime, int64 endDateTime, PyRep* options);
    PyRep* GetVoteItems(uint32 allyID, uint32 status, uint32 maxLen);
    PyRep* GetVoteOptions(uint32 voteCaseID);
    PyRep* GetVotes(uint32 voteCaseID);
    bool CastVote(uint32 corpID, uint32 allyID, uint32 voteCaseID, uint8 optionID);
    bool ResolveVote(uint32 voteCaseID);
    void ProcessVoteExpiry(uint32 allyID);
};

#endif  // EVE_ALLIANCE_ALLIANCEDB_H