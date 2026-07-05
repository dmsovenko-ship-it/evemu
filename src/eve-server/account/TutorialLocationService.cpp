#include "eve-server.h"
#include "account/TutorialLocationService.h"
#include "account/AccountService.h"

TutorialLocationService::TutorialLocationService()
: Service("tutorialLocationSvc")
{
    this->Add("GiveTutorialGoodies", &TutorialLocationService::GiveTutorialGoodies);
}

PyResult TutorialLocationService::GiveTutorialGoodies(PyCallArgs& call, PyInt* tutorialID, PyInt* pageID, PyInt* pageNo)
{
    sLog.White("TutorialLocationService::GiveTutorialGoodies()", "tutID=%u pageID=%u pageNo=%u",
               tutorialID->value(), pageID->value(), pageNo->value());

    Client* pClient = call.client;
    uint32 charID = pClient->GetCharacterID();

    // Look up reward from tutorial_rewards table
    DBQueryResult res;
    bool hasReward = sDatabase.RunQuery(res,
        "SELECT iskAmount, typeID, quantity, skillTypeID"
        " FROM tutorial_rewards"
        " WHERE tutorialID = %u AND pageID = %u",
        tutorialID->value(), pageID->value());

    DBResultRow row;
    if (hasReward && res.GetRow(row)) {
        int64 iskAmount = row.GetInt64(0);
        uint32 typeID = row.GetUInt(1);
        uint32 quantity = row.GetUInt(2);
        uint32 skillTypeID = row.GetUInt(3);

        if (iskAmount > 0) {
            AccountService::TransferFunds(charID, charID, iskAmount, "Tutorial reward",
                                          Journal::EntryType::MissionReward, tutorialID->value());
            sLog.White("TutorialLocationService", "Gave %lld ISK to char %u for tutorial %u",
                       iskAmount, charID, tutorialID->value());
        }

        if (typeID > 0 && quantity > 0) {
            // Spawn item in station hangar
            ItemData idata(typeID, charID, pClient->GetStationID(), flagHangar, quantity);
            InventoryItemRef iRef = sItemFactory.SpawnItem(idata);
            if (iRef.get() != nullptr) {
                iRef->Move(pClient->GetStationID(), flagHangar, true);
                iRef->SaveItem();
                sLog.White("TutorialLocationService", "Gave item %u x%u to char %u", typeID, quantity, charID);
            }
        }
    }

    // Return nodeID and timestamp (same as other service methods)
    return new PyLong(Win32TimeNow());
}
