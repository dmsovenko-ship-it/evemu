/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:             Zhur
    Implementation:     Positron96
    Rewrite:            Allan
*/

/** @todo  go thru and update/optimize this class */

#include "eve-server.h"

#include "../../eve-common/EVE_Calendar.h"
#include "../../eve-common/EVE_Mail.h"


#include "EntityList.h"
#include "StatisticMgr.h"
#include "StaticDataMgr.h"
#include "account/AccountService.h"
#include "manufacturing/Blueprint.h"
#include "manufacturing/RamMethods.h"
#include "manufacturing/RamProxyService.h"
#include "station/StationDataMgr.h"
#include "system/CalendarDB.h"

RamProxyService::RamProxyService() :
    Service("ramProxy")
{
    this->Add("AssemblyLinesGet", &RamProxyService::AssemblyLinesGet);
    this->Add("AssemblyLinesSelect", &RamProxyService::AssemblyLinesSelect);
    this->Add("AssemblyLinesSelectPublic", &RamProxyService::AssemblyLinesSelectPublic);
    this->Add("AssemblyLinesSelectPrivate", &RamProxyService::AssemblyLinesSelectPrivate);
    this->Add("AssemblyLinesSelectCorp", &RamProxyService::AssemblyLinesSelectCorp);
    this->Add("AssemblyLinesSelectAlliance", &RamProxyService::AssemblyLinesSelectAlliance);
    this->Add("GetJobs2", &RamProxyService::GetJobs2);
    this->Add("InstallJob", &RamProxyService::InstallJob);
    this->Add("CompleteJob", &RamProxyService::CompleteJob);
    this->Add("GetRelevantCharSkills", &RamProxyService::GetRelevantCharSkills);
    this->Add("UpdateAssemblyLineConfigurations", &RamProxyService::UpdateAssemblyLineConfigurations);
}

/*
 * # Manufacturing Logging:
 * MANUF__ERROR
 * MANUF__WARNING
 * MANUF__MESSAGE
 * MANUF__INFO
 * MANUF__DEBUG
 * MANUF__TRACE
 * MANUF__DUMP
 */

PyResult RamProxyService::GetRelevantCharSkills(PyCallArgs &call) {
    return call.client->GetChar()->GetRAMSkills();
}

PyResult RamProxyService::AssemblyLinesSelectPublic(PyCallArgs &call) {
    return FactoryDB::AssemblyLinesSelectPublic(call.client->GetRegionID());
}

PyResult RamProxyService::AssemblyLinesSelectPrivate(PyCallArgs &call) {
    return FactoryDB::AssemblyLinesSelectPrivate(call.client->GetCharacterID());
}

PyResult RamProxyService::AssemblyLinesSelectCorp(PyCallArgs &call) {
    /** @todo  this needs to search db for POS arrays based on corp */
    return FactoryDB::AssemblyLinesSelectCorporation(call.client->GetCorporationID());
}

PyResult RamProxyService::AssemblyLinesSelectAlliance(PyCallArgs &call) {
    /** @todo  this needs to search db for POS arrays based on alliance */
    return FactoryDB::AssemblyLinesSelectAlliance(call.client->GetAllianceID());
}

PyResult RamProxyService::AssemblyLinesGet(PyCallArgs &call, PyInt* stationID) {
    return FactoryDB::AssemblyLinesGet(stationID->value());
}

PyResult RamProxyService::AssemblyLinesSelect(PyCallArgs &call, PyString* filter) {
    if (filter->content() == "region") {
        return FactoryDB::AssemblyLinesSelectPublic(call.client->GetRegionID());
    } else if (filter->content() == "char") {
        return FactoryDB::AssemblyLinesSelectPersonal(call.client->GetCharacterID());
    } else if (filter->content() == "corp") {
        return FactoryDB::AssemblyLinesSelectCorporation(call.client->GetCorporationID());
    } else if (filter->content() == "alliance") {
        return FactoryDB::AssemblyLinesSelectAlliance(call.client->GetAllianceID());
    }

    _log(SERVICE__ERROR, "Unknown filter '%s'.", filter->content().c_str());
    return nullptr;
}

PyResult RamProxyService::GetJobs2(PyCallArgs &call, PyInt* ownerID, PyBool* completed) {
    if (ownerID->value() == call.client->GetCorporationID())
        if ((call.client->GetCorpRole() & Corp::Role::FactoryManager) != Corp::Role::FactoryManager) {
            // what other roles (if any) can view corp factory jobs?
            call.client->SendInfoModalMsg("You cannot view your corporation's jobs because you are not a Factory Manager.");
            return nullptr;
        }

    return FactoryDB::GetJobs2(ownerID->value(), completed->value());
}

/** @todo update this for corp usage */
/** @todo  add missing/unhandled indy types (RE, invention, ??)  */
PyResult RamProxyService::InstallJob(PyCallArgs &call, PyRep* locationData, PyRep* itemLocationData, PyRep* bomLocationData, PyRep* flagOutput, PyRep* buildRuns, PyRep* activityID, PyRep* licensedProductionRuns, PyRep* ownerFlag, PyRep* blah) {
    //job = sm.ProxySvc('ramProxy').InstallJob(installationLocationData, installedItemLocationData, bomLocationData, flagOutput, quoteData.buildRuns, quoteData.activityID, quoteData.licensedProductionRuns, not quoteData.ownerFlag, 'blah', quoteOnly=1, installedItem=quoteData.blueprint, maxJobStartTime=quoteData.assemblyLine.nextFreeTime + 1 * MIN, inventionItems=quoteData.inventionItems, inventionOutputItemID=quoteData.inventionItems.outputType)

    _log(MANUF__DUMP, "RamProxyService::Handle_InstallJob() - size=%lli", call.tuple->size());
    call.Dump(MANUF__DUMP);

    Call_InstallJob args;
    if (!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "%s: Failed to decode arguments.", GetName());
        return nullptr;
    }

    _log(MANUF__INFO, "RamProxyService::Handle_InstallJob() - %s", sRamMthd.GetActivityName(args.activityID));

    // check character job count - will throw if fail
    sRamMthd.JobsCheck(call.client->GetChar().get(), args);

    // load job Blueprint
    // bp in pos can be in hangar or array.  need to check both
    InventoryItemRef installedItem = sItemFactory.GetItemRef( args.bpItemID );
    if (installedItem.get() == nullptr) {
        // For remote jobs, the BP is at another station — load from byname data
        if (call.byname.find("installedItem") != call.byname.end()) {
            PyDict* dict = call.byname["installedItem"]->AsDict();
            uint32 itemID = PyRep::IntegerValueU32(dict->GetItemString("itemID"));
            installedItem = sItemFactory.GetBlueprintRef(itemID);
            if (installedItem.get() == nullptr)
                throw UserError ("RamActivityRequiresABlueprint");
        } else {
            _log(MANUF__ERROR, "Remote job: installedItem dict not found");
            throw UserError ("RamActivityRequiresABlueprint");
            return nullptr;
        }
    }
    if (installedItem->categoryID() != EVEDB::invCategories::Blueprint)
        throw UserError ("RamActivityRequiresABlueprint");

    // installedItem is bp.  change ref to bpRef
    BlueprintRef bpRef = BlueprintRef::StaticCast(installedItem);

    // check assy line activity
    sRamMthd.ActivityCheck(call.client, args, bpRef);

    // if output flag not set, put it where it came from
    if (args.outputFlag == flagNone)
        args.outputFlag = bpRef->flag();

    // check permissions
    sRamMthd.LinePermissionCheck(call.client, args);
    sRamMthd.ItemOwnerCheck(call.client, args, bpRef);

    // this is a bit funky, but works quite well....
    // decode path to BP and BOM location
    // i am populating this for corp and personal to have common call to mat'l checks that follow
    PathElement bomLocPath;
    if (args.isCorpJob) {
        // this will get messy...
        /** @todo this is incomplete....
         * get location (all, base, hq, other)
         * get roles at location
         * verify access at location
         */
        CorpPathElement bpPath;
        if (!bpPath.Decode(args.bpLocPath)) {
            _log(SERVICE__ERROR, "Failed to decode Corp BP path.");
            return nullptr;
        }
        // check bp location access
        sRamMthd.HangarRolesCheck(call.client, bpPath.pathFlagID);
        sRamMthd.LocationRolesCheck(call.client, bpPath);


        // bp passed..check bomData
        CorpPathElement bomPath;
        if (!bomPath.Decode(args.bomLocPath)) {
            _log(SERVICE__ERROR, "Failed to decode Corp BOM path.");
            return nullptr;
        }
        // check bp location access
        sRamMthd.HangarRolesCheck(call.client, bomPath.officeFlagID);
        sRamMthd.LocationRolesCheck(call.client, bomPath);

        bomLocPath.locationID   = bomPath.officeID;
        bomLocPath.ownerID      = bomPath.officeCorpID;
        bomLocPath.flagID       = bomPath.pathFlagID;
    } else {
        // get path data for player
        if (!bomLocPath.Decode(args.bpLocPath->GetItem(0))) {
            _log(SERVICE__ERROR, "Failed to decode BOM path.");
            return nullptr;
        }
    }

    // calculate time and cost
    //   Rsp_InstallJob is used as container while building response to job quote.
    Rsp_InstallJob rsp;
    if (!sRamMthd.Calculate(args, bpRef, call.client->GetChar().get(), rsp)){
        _log(MANUF__ERROR, "Could not Calculate() on %s for %s(%u)", bpRef->name(), call.client->GetName(), call.client->GetCharacterID());
        return nullptr;
    }

    // sent as assy line.nextFreeTime + 1m  (a previous call asks for assy line nextFreeTime, displayed in window)
    if (call.byname.find("maxJobStartTime") != call.byname.end())
        if (rsp.maxJobStartTime > PyRep::IntegerValue(call.byname["maxJobStartTime"]))
            throw UserError ("RamProductionTimeExceedsLimits");

    //RamCannotGuaranteeStartTime  // timeslot taken by another char while installing this one

    // get required items for activity
    std::vector<EvERam::RequiredItem> reqItems;
    sDataMgr.GetRamRequiredItems(bpRef->typeID(), (int8)args.activityID, reqItems);

    // quoteOnly is sent for all jobs before installation to approve price and timeframe
    if (PyRep::IntegerValueU32(call.byname["quoteOnly"])) {
        _log(MANUF__INFO, "quoteOnly = true");
        sRamMthd.EncodeBillOfMaterials(reqItems, rsp.materialMultiplier, rsp.charMaterialMultiplier, args.runs, rsp.bom);
        sRamMthd.EncodeMissingMaterials(reqItems, bomLocPath, call.client, rsp.materialMultiplier, rsp.charMaterialMultiplier, args.runs, rsp.missingMaterials);

        // this value is halved in client code. (removed in client update patch 5Nov20)
        //rsp.charTimeMultiplier *= 2;
        return rsp.Encode();
    }

    // verify installer has skills and all needed materials are present in proper location
    sRamMthd.MaterialSkillsCheck(call.client, args.runs, bomLocPath, rsp, reqItems);


    // at this point, it is a real job installation.  check everything else
    sRamMthd.ProductionTimeCheck(rsp.productionTime);

    // get proper location data
    /** @todo  this will need work for pos */
    uint32 locationID(0);
    switch (args.lineLocationGroupID) {
        case EVEDB::invGroups::Station:  {
            // this should be same location as client....unless remote
            locationID = args.lineLocationID;
        } break;
        case EVEDB::invGroups::Solar_System: {
            // this will be pos or ship in space (not sure how ore compression works yet)
            locationID = args.lineLocationID;
        } break;
        default: {
            locationID = args.lineLocationID;
            if (sDataMgr.IsStation(args.lineContainerID)) {
                locationID = args.lineContainerID;
            } else {
                sLog.Warning("InstallJob", "Location is not Station.  Needs work.");
            }
        } break;
    }

    // approved job cost from quote
    float cost(PyRep::IntegerValue(call.byname["authorizedCost"]));

    // pay for assembly lines...take the money, send wallet blink event record the transaction in journal.
    std::string reason = "DESC: Installing ";
    reason += sRamMthd.GetActivityName(args.activityID);
    reason += " job in ";

    if (sDataMgr.IsStation(locationID)) {
        reason += stDataMgr.GetStationName(locationID);
    } else {    // test for POS after that system is more complete...
        reason += "Unknown Location";
    }

    if (args.isCorpJob) {
        reason += " by ";
        reason += call.client->GetName();
    }

    AccountService::TransferFunds(
        call.client->GetCharacterID(),
        stDataMgr.GetOwnerID(locationID),
        cost,
        reason.c_str(),
        Journal::EntryType::FactorySlotRentalFee,
        locationID,    // shows rental location (stationID)
        Account::KeyType::Cash
    );

    int64 beginTime(GetFileTimeNow());
    if (beginTime < rsp.maxJobStartTime)
        beginTime = rsp.maxJobStartTime;

    // register/save job to chosen assy line.
    uint32 jobID = FactoryDB::InstallJob((args.isCorpJob ? call.client->GetCorporationID() : call.client->GetCharacterID()),
                                         call.client->GetCharacterID(), args, beginTime, beginTime + rsp.productionTime * EvE::Time::Second, call.client->GetSystemID());

    if (jobID < 1) {
        _log(MANUF__ERROR, "Could not InstallJob for %s using %s", call.client->GetName(), bpRef->name());
        // make client error here...
        return nullptr;
    }

    if (bpRef->quantity() > 1) {
        BlueprintRef iRef = bpRef->SplitBlueprint(1);
        if (iRef.get() == nullptr) {
            _log(MANUF__WARNING, "Failed to split %s for %s.", bpRef->name(), sRamMthd.GetActivityName(args.activityID));
            throw UserError ("RamActivityRequiresABlueprint");
        }
        bpRef = iRef;
    }

    // unpackage bpo and move to factory
    bpRef->ChangeSingleton(true);
    bpRef->Move(locRAMItems, flagFactoryBlueprint, true);

    // take required items
    std::vector<InventoryItemRef> items;
    sRamMthd.GetBOMItems( bomLocPath, items );

    std::vector<EvERam::RequiredItem>::iterator itemItr = reqItems.begin();
    for (; itemItr != reqItems.end(); ++itemItr) {
        if (itemItr->isSkill)
            continue;       // not interested

        // calculate needed quantity
        uint32 qtyNeeded;
        if (itemItr->extra) {
            qtyNeeded = itemItr->quantity * args.runs;
        } else {
            qtyNeeded = (uint32)round(((itemItr->quantity * rsp.materialMultiplier) + (itemItr->quantity * rsp.charMaterialMultiplier - itemItr->quantity)) * args.runs);
        }

        // consume required materials
        std::vector<InventoryItemRef>::iterator refItr = items.begin();
        for (; refItr != items.end(); ++refItr)
            if ((*refItr)->typeID() == itemItr->typeID) {
                if (qtyNeeded >= (*refItr)->quantity()) {
                    qtyNeeded -= (*refItr)->quantity();
                    (*refItr)->Delete();
                } else {
                    (*refItr)->AlterQuantity(-qtyNeeded, true);
                    break;  // we are done, stop searching
                }
            }
    }

    // update runs and do other shit where applicable
    switch (args.activityID) {
        case EvERam::Activity::Manufacturing: {
            // we just need to update runs here
            bpRef->UpdateRuns(-args.runs);
        } break;
        case EvERam::Activity::ReverseEngineering: {
            bpRef->UpdateRuns(-args.runs);
            // there is more to this, but not implemented yet
        } break;
        case EvERam::Activity::Invention: {
            bpRef->UpdateRuns(-args.runs);
        // im sure there is more to do here......

        /** @todo do something constructive with this data...
        // this is populated for t2 bpc
        //     inventionItems=quoteData.inventionItems
        uint16 outputType(0), baseItemType(0), decryptorType(0);
        if (call.byname.find("inventionItems") != call.byname.end()) {
            PyDict* dict = call.byname["inventionItems"]->AsDict();
            outputType = PyRep::IntegerValueU32(dict->GetItemString("outputType"));
            baseItemType = PyRep::IntegerValueU32(dict->GetItemString("baseItemType"));
            decryptorType = PyRep::IntegerValueU32(dict->GetItemString("decryptorType"));
        }
        // this is populated for t2 bpc
        //    inventionOutputItemID=quoteData.inventionItems.outputType
        if (call.byname.find("inventionOutputItemID") != call.byname.end()) {
            // this is the bp typeID to create....should we test to see if they're the same?
            outputType = PyRep::IntegerValueU32(call.byname["inventionOutputItemID"]);
        }
        */
        } break;
    }

    if (sConfig.ram.AutoEvent) {
        std::string title;
        if (args.isCorpJob) {
            title += "Corporate ";
        } else {
            title += "Personal ";
        }
        title += sRamMthd.GetActivityName(args.activityID);
        title += " Job";

        std::string description;
        if (args.isCorpJob) {
            description += "The ";
        } else {
            description += "Your ";
        }
        description += sRamMthd.GetActivityName(args.activityID);
        description += " job in ";
        /** @todo update this for pos names when that system is working */
        if (sDataMgr.IsStation(locationID)) {
            description += stDataMgr.GetStationName(locationID);
        } else {  // POS installation
            description += "Unknown Location";
        }

        description += " is scheduled to complete at the time noted.<br><br>";
        switch (args.activityID) {
            case EvERam::Activity::Manufacturing: {
                description += "This job is for <color=red>";
                description += std::to_string(args.runs);
                description += "</color> runs of <color=yellow>";
                description += bpRef->productType().groupName();
                description += "</color>,<br>producing <color=green>";
                description += std::to_string(args.runs * bpRef->productType().portionSize());
                description += "</color> units of <color=green>";
                description += bpRef->productType().name();
                description += "</color>.";
            } break;
            case EvERam::Activity::ResearchMaterial: {
                description += "Upon completion, this job will increase the Material Efficiency of the ";
                description += "<color=yellow>";
                description += bpRef->itemName();
                description += "</color> from <color=red>";
                description += std::to_string(bpRef->mLevel());
                description += "</color> to <color=green>";
                description += std::to_string(bpRef->mLevel() + args.runs);
                description += "</color>.";
            } break;
            case EvERam::Activity::ResearchTime: {
                description += "Upon completion, this job will increase the Production Efficiency of the ";
                description += "<color=yellow>";
                description += bpRef->itemName();
                description += "</color> from <color=red>";
                description += std::to_string(bpRef->pLevel());
                description += "</color> to <color=green>";
                description += std::to_string(bpRef->pLevel() + args.runs);
                description += "</color>.";
            } break;
            case EvERam::Activity::Copying: {
                description += "<color=green>";
                description += std::to_string(args.runs);
                description += "</color> Copies of the ";
                description += "<color=yellow>";
                description += bpRef->itemName();
                description += "</color> will be made.";
            } break;
            case EvERam::Activity::Invention: {
                // this isnt implemented yet, so not sure what to do here
                description += "";
            } break;
            case EvERam::Activity::ReverseEngineering: {
                // this isnt implemented yet, so not sure what to do here
                description += "";
            } break;
                // others are not used (or shouldnt be)
        }

        if (args.isCorpJob) {
            description += "<br><br>This ";
            description += sRamMthd.GetActivityName(args.activityID);
            description += " job was installed by ";
            description += call.client->GetName();
            description += " on ";
            description += currentDateTime();
            description += " CST.";
        }

        // random sayings/quotes here?  sure, why not?  lol
        if (IsEven(MakeRandomInt(0, 10)))
            description += "<br><br><br>And a good time shall be had by all!";
        //description += "Don't wish it was easier; instead, wish you were better.";
        //description += "Endure and survive.";
        //description += "Fly safe.";

        // should this be important?
        uint32 eventID = CalendarDB::SaveSystemEvent(args.isCorpJob?call.client->GetCorporationID():call.client->GetCharacterID(),
                                                     stDataMgr.GetOwnerID(locationID),
                                                     beginTime + rsp.productionTime * EvE::Time::Second,
                                                     Calendar::AutoEvent::RAMJob, title, description);

        FactoryDB::SetJobEventID(jobID, eventID);

        //force calendar reload (if corp job, update all online members, also)
        if (args.isCorpJob) {
            sEntityList.CorpNotify(call.client->GetCorporationID(), Notify::Types::FactoryJob, "OnReloadCalendar", "charid", new PyTuple(0));
        } else {
            call.client->SendNotification("OnReloadCalendar", "charid", new PyTuple(0), false);  // this is not sequenced
        }
    }

    // we may need a separate table for invention jobs to store it's specific data....

    // increment statistic counter
    sStatMgr.Increment(Stat::ramJobs);

    return nullptr;
}

PyResult RamProxyService::CompleteJob(PyCallArgs &call, PyRep* info, PyRep* jobID, PyRep* cancel) {
    /*
     * 23:35:54 [ManufDump] RamProxyService::Handle_CompleteJob() - size 3
     * 23:35:54 [ManufDump]   Call Arguments:
     * 23:35:54 [ManufDump]      Tuple: 3 elements
     * 23:35:54 [ManufDump]       [ 0]   List: 3 elements
     * 23:35:54 [ManufDump]       [ 0]   [ 0]   List: 2 elements
     * 23:35:54 [ManufDump]       [ 0]   [ 0]   [ 0]    Integer: 60014140   invLocation (station, ship, starbase)
     * 23:35:54 [ManufDump]       [ 0]   [ 0]   [ 1]    Integer: 15         invLocationGroup
     * 23:35:54 [ManufDump]       [ 0]   [ 1]   List: Empty                 path  [eve.session.locationid, eve.session.charid, const.flagHangar]
     * 23:35:54 [ManufDump]       [ 0]   [ 2]   List: 1 elements
     * 23:35:54 [ManufDump]       [ 0]   [ 2]   [ 0]    Integer: 60014140   containerID (actual itemID of location bp is in)
     * 23:35:54 [ManufDump]       [ 1]    Integer: 2                        jobID
     * 23:35:54 [ManufDump]       [ 2]    Boolean: false                    cancel
     */
    _log(MANUF__DUMP, "RamProxyService::Handle_CompleteJob() - size=%lli", call.tuple->size());
    call.Dump(MANUF__DUMP);

    Call_CompleteJob args;
    if (!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "%s: Failed to decode arguments.", GetName());
        return nullptr;
    }

    EvERam::JobProperties data = EvERam::JobProperties();
    if (!FactoryDB::GetJobProperties(args.jobID, data))
        throw UserError ("RamCompletionNoSuchJob");

    sRamMthd.VerifyCompleteJob(args, data, call.client);

    // does an aborted job return the installed item immediately or after time expiry?
    FactoryDB::CompleteJob(args.jobID, (args.cancel ? EvERam::Status::Abort : EvERam::Status::Delivered));

    // return item
    InventoryItemRef installedItem = sItemFactory.GetItemRef(data.itemID);
    if (installedItem.get() == nullptr)
        return nullptr;
    installedItem->Move(args.containerID, data.outputFlag, true);

    // return materials which weren't consumed
    std::vector<EvERam::RequiredItem> reqItems;
    sDataMgr.GetRamReturns(installedItem->typeID(), data.activity, reqItems);
    for (auto cur : reqItems) {
        // what about items where damage < 1.0?  (there are some...)
        uint32 quantity = (cur.quantity * data.jobRuns * (1 - cur.damagePerJob));
        if (quantity == 0)
            quantity = 1;

        ItemData idata(cur.typeID, data.ownerID, locTemp, flagNone, quantity);
        InventoryItemRef iRef = sItemFactory.SpawnItem( idata );
        if (iRef.get() != nullptr)
            iRef->Move(args.containerID, data.outputFlag, true);
    }

    if (args.cancel) {
        // what needs to be done to cancel a job?

        // if job event in calendar, set to deleted for canceled job.
        CalendarDB::DeleteEvent(data.eventID);
    } else {
        BlueprintRef bpRef = BlueprintRef::StaticCast( installedItem );
        switch(data.activity) {
            case EvERam::Activity::Manufacturing: {
                ItemData idata(bpRef->productTypeID(), data.ownerID, locTemp, flagFactoryOutput, (bpRef->productType().portionSize() * data.jobRuns));
                InventoryItemRef iRef = sItemFactory.SpawnItem( idata );
                if (iRef.get() != nullptr)
                    iRef->Move(args.containerID, data.outputFlag, true);
            } break;
            case EvERam::Activity::ResearchTime: {
                bpRef->UpdatePLevel(data.jobRuns);
            } break;
            case EvERam::Activity::ResearchMaterial: {
                bpRef->UpdateMLevel(data.jobRuns);
            } break;
            case EvERam::Activity::Copying: {
                ItemData idata(installedItem->typeID(), data.ownerID, locTemp, flagFactoryOutput);
                EvERam::bpData bpdata = EvERam::bpData();
                    bpdata.copy   = true;
                    bpdata.runs   = data.licensedRuns;
                    bpdata.mLevel = bpRef->mLevel();
                    bpdata.pLevel = bpRef->pLevel();
                while (data.jobRuns) {
                    BlueprintRef copy = Blueprint::Spawn(idata, bpdata);
                    //new bpc not showing in player hangar....
                    if (copy.get() != nullptr)
                        copy->Move(args.containerID, data.outputFlag, true);
                    --data.jobRuns;
                }
            } break;
            case EvERam::Activity::Invention: {
                BlueprintRef invBpRef = BlueprintRef::StaticCast(installedItem);
                uint32 outTypeID = FactoryDB::GetTech2Blueprint(invBpRef->typeID());
                bool success = false;
                int8 meBonus = -2, peBonus = -4, runs = 1;

                if (outTypeID > 0) {
                    // Read invention items from client data (passed during install)
                    uint16 baseItemType = 0, decryptorType = 0;
                    if (call.byname.find("inventionItems") != call.byname.end()) {
                        PyDict* dict = call.byname["inventionItems"]->AsDict();
                        baseItemType = PyRep::IntegerValueU32(dict->GetItemString("baseItemType"));
                        decryptorType = PyRep::IntegerValueU32(dict->GetItemString("decryptorType"));
                    }

                    // Base chance: use blueprint type's chanceOfRE if available, else per-group fallback
                    float baseChance = invBpRef->type().chanceOfRE();
                    if (baseChance < 0.01f) {
                        uint16 prodGroup = invBpRef->productType().groupID();
                        if (prodGroup >= 25 && prodGroup <= 31)
                            baseChance = 0.30f;
                        else if (prodGroup >= 26 && prodGroup <= 420)
                            baseChance = 0.26f;
                        else if (prodGroup >= 27)
                            baseChance = 0.22f;
                        else
                            baseChance = 0.34f;
                    }

                    // Read encryption and datacore skills from RAM requirements
                    uint8 encryptionLevel = 0, dataCore1Level = 0, dataCore2Level = 0;
                    std::vector<EvERam::RequiredItem> reqItems;
                    sDataMgr.GetRamRequiredItems(invBpRef->typeID(), EvERam::Activity::Invention, reqItems);
                    for (auto& req : reqItems) {
                        if (!req.isSkill) continue;
                        uint8 skillLevel = call.client->GetChar()->GetSkillLevel(req.typeID);
                        // Encryption skills are in category 34 (Science), datacore skills also Science
                        // The first skill (usually the encryption skill) gets encryptionLevel
                        if (encryptionLevel == 0)
                            encryptionLevel = skillLevel;
                        else if (dataCore1Level == 0)
                            dataCore1Level = skillLevel;
                        else if (dataCore2Level == 0)
                            dataCore2Level = skillLevel;
                    }

                    // Meta level from baseItemType (lookup invMetaTypes table)
                    uint8 metaLevel = 0;
                    if (baseItemType > 0) {
                        DBQueryResult metaRes;
                        if (sDatabase.RunQuery(metaRes,
                            "SELECT metaLevel FROM invMetaTypes WHERE typeID = %u", baseItemType))
                        {
                            DBResultRow metaRow;
                            if (metaRes.GetRow(metaRow))
                                metaLevel = metaRow.GetUInt(0);
                        }
                    }

                    // Decryptor modifier (from invMetaTypes or known typeIDs)
                    float decryptorMod = 1.0f;
                    if (decryptorType > 0) {
                        DBQueryResult decRes;
                        if (sDatabase.RunQuery(decRes,
                            "SELECT metaLevel FROM invMetaTypes WHERE typeID = %u", decryptorType))
                        {
                            DBResultRow decRow;
                            if (decRes.GetRow(decRow)) {
                                uint8 meta = decRow.GetUInt(0);
                                // decryptor meta levels: 1=basic(0.6), 2=symmetric(0.8),
                                // 3=optimized(1.1), 4=accelerant(1.2), 5=attachment(1.3)
                                static const float decMods[] = { 1.0f, 0.6f, 0.8f, 1.1f, 1.2f, 1.3f };
                                if (meta <= 5)
                                    decryptorMod = decMods[meta];
                            }
                        }
                    }

                    float chance = EvEMath::RAM::InventionChance(baseChance, encryptionLevel, dataCore1Level, dataCore2Level, metaLevel, decryptorMod);
                    success = (MakeRandomFloat() < chance);

                    if (success) {
                        runs = invBpRef->runs();
                        if (runs < 1) runs = 1;
                        if (runs > 10) runs = 10;

                        ItemData idata(outTypeID, data.ownerID, locTemp, flagFactoryOutput);
                        EvERam::bpData bpdata;
                        bpdata.copy = true;
                        bpdata.runs = runs;
                        bpdata.mLevel = std::max<int8>(0, invBpRef->mLevel() + meBonus);
                        bpdata.pLevel = std::max<int8>(0, invBpRef->pLevel() + peBonus);

                        BlueprintRef newBp = Blueprint::Spawn(idata, bpdata);
                        if (newBp.get() != nullptr) {
                            newBp->Move(args.containerID, data.outputFlag, true);
                            data.jobRuns = 0;
                        }
                    }
                }

                PyDict* dict = new PyDict();
                if (success) {
                    dict->SetItemString("messageLabel", new PyString("UI/ScienceAndIndustry/ScienceAndIndustryWindow/RamInventionJobSucceeded"));
                    dict->SetItemString("jobCompletedSuccessfully", new PyBool(true));
                    dict->SetItemString("outputME", PyStatic.NewInt(meBonus));
                    dict->SetItemString("outputPE", PyStatic.NewInt(peBonus));
                    dict->SetItemString("outputRuns", PyStatic.NewInt(runs));
                    dict->SetItemString("outputTypeID", PyStatic.NewInt(outTypeID));
                } else {
                    dict->SetItemString("messageLabel", new PyString("UI/ScienceAndIndustry/ScienceAndIndustryWindow/RamInventionJobFailed"));
                    dict->SetItemString("jobCompletedSuccessfully", new PyBool(false));
                }

                /* invention result outcomes:  (proposed in phoebe)
                 *
                 * success:
                 *      execeptional    - ME +2, PE +3
                 *      great           - ME +1, PE +2
                 *      good            - PE +1
                 *      standard        - no modifier
                 *
                 * failure:
                 *      standard        - return 50% datacores
                 *      poor            - return 25% datacores
                 *      terrible        - return 10% datacores
                 *      critical        - return no datacores
                 *
                 */

                // not sure the semantics on this...its' on both succeed and fail
                // this *could* be the part about your skills
                /*
(251331, `This job was so easy you feel you could do it again in your sleep.`)
(251332, `You have a good feeling this job is perfectly suited to someone of your talents.`)
(251333, `Completing this job was fairly comfortable for you and didn't tax your talents too much.`)
(251334, `You're happy with your success, for succeeding this job was far from certain.`)
(251335, `You succeeded, but you have a nagging feeling that you can't count on it every time.`)
(251336, `Despite valiant efforts you failed the job with success at your fingertips.`)
(251338, `You've got a good feel for this job, even if nothing of value came out of it this time.`)
(251339, `Although you have a firm understanding of the basics of this job you were never close to a solution.`)
(251340, `This is far from an impossible job, but one that might require a few tries before succeeding.`)
(251341, `This job requires lot of diligence and hard work on your part if you want to succeed.`)
(251342, `You feel a bit out of your league, succeeding this job requires a fair bit of luck.`)
(251343, `You never saw the light, this job is almost impossible for you to complete.`)

{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251331, 'label': u'InventionResultSuccessCouldDoInSleep'}(u'This job was so easy you feel you could do it again in your sleep.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251332, 'label': u'InventionResultSuccessSuitedToYourTalets'}(u'You have a good feeling this job is perfectly suited to someone of your talents.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251333, 'label': u'InventionResultSuccessDidntTaxTooMuch'}(u"Completing this job was fairly comfortable for you and didn't tax your talents too much.", None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251334, 'label': u'InventionResultSuccessFarFromCertain'}(u"You're happy with your success, for succeeding this job was far from certain.", None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251335, 'label': u'InventionResultSuccessCantCountOnIt'}(u"You succeeded, but you have a nagging feeling that you can't count on it every time.", None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251336, 'label': u'InventionResultFailedSuccessAtFingertips'}(u'Despite valiant efforts you failed the job with success at your fingertips.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251338, 'label': u'InventionResultFailedFeltGoodAboutIt'}(u"You've got a good feel for this job, even if nothing of value came out of it this time.", None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251339, 'label': u'InventionResultFailedNeverClose'}(u'Although you have a firm understanding of the basics of this job you were never close to a solution.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251340, 'label': u'InventionResultFailedRequiresAFewTries'}(u'This is far from an impossible job, but one that might require a few tries before succeeding.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251341, 'label': u'InventionResultFailedRequiresHardWork'}(u'This job requires lot of diligence and hard work on your part if you want to succeed.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251342, 'label': u'InventionResultFailedRequiresLuck'}(u'You feel a bit out of your league, succeeding this job requires a fair bit of luck.', None, None)
{'FullPath': u'UI/ScienceAndIndustry/Invention', 'messageID': 251343, 'label': u'InventionResultFailedImpossible'}(u'You never saw the light, this job is almost impossible for you to complete.', None, None)
{
*/
                /* {'messageKey': 'ProductionFailure', 'dataID': 17875197, 'suppressable': False, 'bodyID': 256420, 'messageType': 'hint', 'urlAudio': '', 'urlIcon': '', 'titleID': 256419, 'messageID': 3741}
                 * {'messageKey': 'ProductionDuplicationFailure', 'dataID': 17875202, 'suppressable': False, 'bodyID': 256422, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 256421, 'messageID': 3739}
                 * {'messageKey': 'ProductionInventionFailure', 'dataID': 17875207, 'suppressable': False, 'bodyID': 256424, 'messageType': 'info', 'urlAudio': '', 'urlIcon': '', 'titleID': 256423, 'messageID': 3740}
                 *
                 * {'FullPath': u'UI/Messages', 'messageID': 256419, 'label': u'ProductionFailureTitle'}(u'Production Job Will Fail', None, None)
                 * {'FullPath': u'UI/Messages', 'messageID': 256420, 'label': u'ProductionFailureBody'}(u'There is no chance of that action succeeding.', None, None)
                 * {'FullPath': u'UI/Messages', 'messageID': 256421, 'label': u'ProductionDuplicationFailureTitle'}(u'Production Job Will Fail', None, None)
                 * {'FullPath': u'UI/Messages', 'messageID': 256422, 'label': u'ProductionDuplicationFailureBody'}(u'"{[item]type.name}" can not be duplicated, there is no chance of it succeeding.', None, {u'{[item]type.name}': {'conditionalValues': [], 'variableType': 2, 'propertyName': 'name', 'args': 0, 'kwargs': {}, 'variableName': 'type'}})
                 * {'FullPath': u'UI/Messages', 'messageID': 256423, 'label': u'ProductionInventionFailureTitle'}(u'Production Job Will Fail', None, None)
                 * {'FullPath': u'UI/Messages', 'messageID': 256424, 'label': u'ProductionInventionFailureBody'}(u'"{[item]type.name}" can not be invented, there is no chance of it succeeding.', None, {u'{[item]type.name}': {'conditionalValues': [], 'variableType': 2, 'propertyName': 'name', 'args': 0, 'kwargs': {}, 'variableName': 'type'}})
                 *
                 */
                /*
            if hasattr(result, 'message'):
                eve.Message(result.message.msg, result.message.args)
                */
                PyDict* msg = new PyDict();
                    msg->SetItemString("msg", PyStatic.NewInt(0));
                    msg->SetItemString("args", PyStatic.NewInt(0));
                dict->SetItemString("message", msg);
                return dict;
            } break;

            case EvERam::Activity::ReverseEngineering: {
                BlueprintRef reBpRef = BlueprintRef::StaticCast(installedItem);
                float baseChance = reBpRef->type().chanceOfRE();
                if (baseChance < 0.01f) baseChance = 0.20f;

                uint8 reSkill = call.client->GetChar()->GetSkillLevel(EvESkill::ReverseEngineering);
                float skillMod = 1.0f + 0.01f * reSkill;

                // datacore skill modifier: read RAM requirements for this RE blueprint
                float dcMod = 1.0f;
                std::vector<EvERam::RequiredItem> reqItems;
                sDataMgr.GetRamRequiredItems(reBpRef->typeID(), EvERam::Activity::ReverseEngineering, reqItems);
                for (auto& req : reqItems) {
                    if (req.isSkill and req.typeID != EvESkill::ReverseEngineering) {
                        uint8 dcLevel = call.client->GetChar()->GetSkillLevel(req.typeID);
                        if (dcLevel > 0)
                            dcMod = 1.0f + dcLevel * 0.05f; // +5% per datacore skill level
                    }
                }
                float chance = baseChance * skillMod * dcMod;
                bool success = (MakeRandomFloat() < chance);

                PyDict* dict = new PyDict();
                if (success) {
                    // Determine output blueprint typeID from parentBlueprintTypeID
                    uint32 outTypeID = reBpRef->type().parentBlueprintTypeID();
                    if (outTypeID == 0)
                        outTypeID = FactoryDB::GetTech2Blueprint(reBpRef->typeID());

                    if (outTypeID > 0) {
                        int8 runs = std::max(1, reBpRef->runs() / 10);
                        if (runs > 10) runs = 10;

                        ItemData idata(outTypeID, data.ownerID, locTemp, flagFactoryOutput);
                        EvERam::bpData bpdata;
                        bpdata.copy = true;
                        bpdata.runs = runs;
                        bpdata.mLevel = 0;
                        bpdata.pLevel = 0;

                        BlueprintRef newBp = Blueprint::Spawn(idata, bpdata);
                        if (newBp.get() != nullptr) {
                            newBp->Move(args.containerID, data.outputFlag, true);
                            data.jobRuns = 0;
                        }

                        dict->SetItemString("messageLabel", new PyString("UI/ScienceAndIndustry/ScienceAndIndustryWindow/RamReverseEngineeringJobSucceeded"));
                        dict->SetItemString("jobCompletedSuccessfully", new PyBool(true));
                        dict->SetItemString("outputRuns", PyStatic.NewInt(runs));
                        dict->SetItemString("outputTypeID", PyStatic.NewInt(outTypeID));
                    } else {
                        dict->SetItemString("messageLabel", new PyString("UI/ScienceAndIndustry/ScienceAndIndustryWindow/RamReverseEngineeringTaskFailed"));
                        dict->SetItemString("jobCompletedSuccessfully", new PyBool(false));
                    }
                } else {
                    dict->SetItemString("messageLabel", new PyString("UI/ScienceAndIndustry/ScienceAndIndustryWindow/RamReverseEngineeringTaskFailed"));
                    dict->SetItemString("jobCompletedSuccessfully", new PyBool(false));
                }

                PyDict* msg = new PyDict();
                    msg->SetItemString("msg", PyStatic.NewInt(0));
                    msg->SetItemString("args", PyStatic.NewInt(0));
                dict->SetItemString("message", msg);
                return dict;
            } break;

            /* depreciated */
            case EvERam::Activity::ResearchTech:
            case EvERam::Activity::Duplicating:
            default: {
                _log(MANUF__WARNING, "Activity %u is currently unsupported.", data.activity);
                throw UserError ("RamActivityInvalid");
            } break;
        }
    }

    // Send simple completion notification
    if (data.ownerID > 0) {
        std::string actName = sRamMthd.GetActivityName(data.activity);
        std::string msg = std::string("Your ") + actName + " job has been completed.";
        DBerror mailErr;
        sDatabase.RunQuery(mailErr,
            "INSERT INTO mailMessage (senderID, title, body, created, toCharacterIDs)"
            " VALUES (1, 'Job Complete', '%s', %lli, '%%%u%%')",
            msg.c_str(), (int64)GetFileTimeNow(), data.ownerID);
    }

    return PyStatic.NewNone();
}

PyResult RamProxyService::UpdateAssemblyLineConfigurations(PyCallArgs &call, PyRep* installationLocationData, PyRep* rowset) {
    _log(MANUF__DUMP, "RamProxyService::Handle_UpdateAssemblyLineConfigurations() - size=%lli", call.tuple->size());

    // Parse installation location to get station ID
    uint32 stationID = 0;
    if (installationLocationData->IsInt()) {
        stationID = installationLocationData->AsInt()->value();
    } else if (installationLocationData->IsDict()) {
        PyDict* locDict = installationLocationData->AsDict();
        // Client may send a dict with location info — extract stationID
        PyRep* locVal = locDict->GetItemString("locationID");
        if (locVal != nullptr && locVal->IsInt())
            stationID = locVal->AsInt()->value();
    }

    if (stationID == 0) {
        _log(MANUF__ERROR, "UpdateAssemblyLineConfigurations: no valid stationID found");
        return PyStatic.NewNone();
    }

    // Parse rowset — client sends updated assembly line type quantities
    if (rowset->IsList()) {
        PyList* rows = rowset->AsList();
        for (size_t i = 0; i < rows->size(); ++i) {
            PyRep* row = rows->GetItem(i);
            if (!row->IsDict()) continue;
            PyDict* dict = row->AsDict();

            PyRep* typeID = dict->GetItemString("assemblyLineTypeID");
            PyRep* qty = dict->GetItemString("quantity");
            if (typeID == nullptr || !typeID->IsInt()) continue;

            uint8 lineTypeID = typeID->AsInt()->value();
            uint8 quantity = (qty != nullptr && qty->IsInt()) ? qty->AsInt()->value() : 0;

            DBerror err;
            if (quantity > 0) {
                sDatabase.RunQuery(err,
                    "INSERT INTO ramAssemblyLineStations (stationID, assemblyLineTypeID, quantity) "
                    "VALUES (%u, %u, %u) ON DUPLICATE KEY UPDATE quantity = %u",
                    stationID, lineTypeID, quantity, quantity);
            } else {
                sDatabase.RunQuery(err,
                    "DELETE FROM ramAssemblyLineStations WHERE stationID = %u AND assemblyLineTypeID = %u",
                    stationID, lineTypeID);
            }
        }
    }

    return PyStatic.NewNone();
}
