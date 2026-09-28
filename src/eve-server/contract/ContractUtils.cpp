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
    Author: AlTahir
*/

#include <boost/algorithm/string/replace.hpp>
#include "eve-server.h"


#include "ContractUtils.h"

// Since these queries are used in multiple GET functions, we initialize them as constants and use at will.
const std::string getContractQueryBase = "SELECT contractId as contractID, contractType as type, issuerID, issuerCorpID, forCorp, isPrivate as availability, "
                               "assigneeID, acceptorID, dateIssued, dateExpired, dateAccepted, numDays, dateCompleted, startStationID, startSolarSystemID, "
                               "startRegionID, endStationID, endSolarSystemID, endRegionID, price, reward, collateral, title, description, status, "
                               "crateID, volume, issuerAllianceID, issuerWalletKey, acceptorWalletKey "
                               "FROM ctrContracts ";
const std::string getContractItemsQueryBase = "SELECT contractId as contractID, itemID, quantity, itemTypeID, inCrate, parentID, productivityLevel, materialLevel, isCopy as copy, "
                                    "licensedProductionRunsRemaining, damage, flagID "
                                    "FROM ctrItems "
                                    "WHERE contractId IN (%s)";
const std::string getContractItemsShortQueryBase = "SELECT itemID "
                                                   "FROM ctrItems "
                                                   "WHERE contractId IN (%s) AND inCrate = true";
const std::string getContractBidsQueryBase = "SELECT amount, bidderID "
                                   "FROM ctrBids "
                                   "WHERE contractId IN (%s) "
                                   "ORDER BY bidDateTime DESC";

/**
 * Gathers and packs contract data into a util.KeyVal PyObject.
 * @param contractId - contract ID to get
 * @return - util.KeyVal PyObject (or nullptr, if contract doesn't exist or there were errors during execution)
 */
PyResult ContractUtils::GetContractEntry(int contractId)
{
    std::string contractID = std::to_string(contractId);

    DBQueryResult contractRes;
    if (!sDatabase.RunQuery(contractRes, (getContractQueryBase + "WHERE contractId IN (%s)").c_str(), contractID.c_str()))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", contractRes.error.c_str());
        return nullptr;
    }
    if (contractRes.GetRowCount() > 0) {
        DBResultRow contractRow;
        contractRes.GetRow(contractRow);

        // We only run queries for items and bids if we had something returned in contracts query - we don't need to waste time and resources on non-existent contracts.
        DBQueryResult itemsRes;
        DBQueryResult bidsRes;
        if (!sDatabase.RunQuery(itemsRes, getContractItemsQueryBase.c_str(), contractID.c_str()))
        {
            codelog(DATABASE__ERROR, "Error in query: %s", itemsRes.error.c_str());
            return nullptr;
        }
        if (!sDatabase.RunQuery(bidsRes, getContractBidsQueryBase.c_str(), contractID.c_str()))
        {
            codelog(DATABASE__ERROR, "Error in query: %s", bidsRes.error.c_str());
            return nullptr;
        }

        PyDict* response = new PyDict;
        response->SetItemString("contract", DBRowToPackedRow(contractRow));
        response->SetItemString("items", DBResultToCRowset(itemsRes));
        response->SetItemString("bids", DBResultToCRowset(bidsRes));

        return new PyObject( "util.KeyVal", response );
    } else {
        codelog(SERVICE__ERROR, "No contract with ID '%s' was found. Aborting", contractID.c_str());
        return nullptr;
    }
}

/**
 * Queries contracts and returns a PyList object, that contains data for each of them.
 * @param contractIds - Vector with all contractID's to search for
 * @return PyList with Contract KeyVal objects.
 */
PyList* ContractUtils::GetContractEntries(std::vector<int> contractIDList) {
    if (contractIDList.empty()) {
        // Empty input is legitimate (courier contracts have no item entries the
        // client would query) - return an empty list instead of erroring out.
        return new PyList();
    }

    // A search can hand us thousands of ids; a single "IN (a,b,c,...)" that large
    // can exceed the DB limits and drop the connection (#2006). Query in bounded
    // batches and merge the results.
    const size_t BATCH = 500;

    std::map<int, CRowSet*> itemsMap;
    std::map<int, CRowSet*> bidsMap;
    DBRowDescriptor* itemsHeader = nullptr;
    DBRowDescriptor* bidsHeader = nullptr;
    PyList* contractsList = new PyList;
    bool anyContracts = false;

    for (size_t start = 0; start < contractIDList.size(); start += BATCH) {
        size_t end = std::min(start + BATCH, contractIDList.size());
        std::string contractIDs;
        for (size_t i = start; i < end; ++i)
            contractIDs.append(std::to_string(contractIDList[i]) + ",");
        contractIDs.pop_back();

        DBQueryResult contractRes;
        if (!sDatabase.RunQuery(contractRes, (getContractQueryBase + "WHERE contractId IN (%s)").c_str(), contractIDs.c_str())) {
            codelog(DATABASE__ERROR, "Error in query: %s", contractRes.error.c_str());
            continue;
        }
        if (contractRes.GetRowCount() == 0)
            continue;

        DBQueryResult itemsRes;
        DBQueryResult bidsRes;
        if (!sDatabase.RunQuery(itemsRes, getContractItemsQueryBase.c_str(), contractIDs.c_str())) {
            codelog(DATABASE__ERROR, "Error in query: %s", itemsRes.error.c_str());
            continue;
        }
        if (!sDatabase.RunQuery(bidsRes, getContractBidsQueryBase.c_str(), contractIDs.c_str())) {
            codelog(DATABASE__ERROR, "Error in query: %s", bidsRes.error.c_str());
            continue;
        }
        anyContracts = true;

        // The schema is identical for every batch, so one descriptor is enough.
        if (itemsHeader == nullptr)
            itemsHeader = new DBRowDescriptor(itemsRes);
        if (bidsHeader == nullptr)
            bidsHeader = new DBRowDescriptor(bidsRes);

        // Every CRowSet created from one header must own its own header ref
        // (the ctor's keyword dict steals one); the creator ref is released below.
        auto makeRowset = [](DBRowDescriptor* hdr) {
            PyIncRef(hdr);
            DBRowDescriptor* h = hdr;
            return new CRowSet(&h);
        };

        DBResultRow itemRow;
        while (itemsRes.GetRow(itemRow)) {
            int contractID = itemRow.GetInt(0);
            auto pos = itemsMap.find(contractID);
            CRowSet* rowset = (pos == itemsMap.end()) ? (itemsMap[contractID] = makeRowset(itemsHeader)) : pos->second;
            PyPackedRow* into = rowset->NewRow();
            FillItemData(&itemRow, into);
        }

        DBResultRow bidRow;
        while (bidsRes.GetRow(bidRow)) {
            int contractID = bidRow.GetInt(0);
            auto pos = bidsMap.find(contractID);
            CRowSet* rowset = (pos == bidsMap.end()) ? (bidsMap[contractID] = makeRowset(bidsHeader)) : pos->second;
            PyPackedRow* into = rowset->NewRow();
            FillBidData(&bidRow, into);
        }

        DBResultRow contractRow;
        while (contractRes.GetRow(contractRow)) {
            int contractID = contractRow.GetInt(0);

            PyDict* contract = new PyDict;
            contract->SetItemString("contract", DBRowToPackedRow(contractRow));
            contract->SetItemString("items", itemsMap.find(contractID) == itemsMap.end() ? makeRowset(itemsHeader) : itemsMap.find(contractID)->second);
            contract->SetItemString("bids", bidsMap.find(contractID) == bidsMap.end() ? makeRowset(bidsHeader) : bidsMap.find(contractID)->second);

            contractsList->AddItem(new PyObject("util.KeyVal", contract));
        }
    }

    // release the creator refs - the rowsets hold their own now
    if (itemsHeader != nullptr)
        PySafeDecRef(itemsHeader);
    if (bidsHeader != nullptr)
        PySafeDecRef(bidsHeader);

    if (!anyContracts) {
        codelog(SERVICE__ERROR, "No contracts in range were found. Aborting");
        PySafeDecRef(contractsList);
        return nullptr;
    }
    return contractsList;
}

/**
 * Queries and composes a response object for GetContractListForOwner call. Placed in utils class so that we can re-use query strings
 * @param call - Call instance
 * @return - Response obj
 */
PyResult ContractUtils::GetContractListForOwner(PyInt* ownerID, PyInt* contractStatus, std::optional <PyInt*> contractType, std::optional <PyBool*> issuedToBy) {
    std::string contracts_query = getContractQueryBase;
    std::string items_query = "SELECT contractId, itemTypeID, quantity, inCrate "
                              "FROM ctrItems "
                              "WHERE contractId IN (";
    std::vector<int> contractIDs;

    // First, we finish contracts query by adding filters
    if (issuedToBy.has_value() == false) {
        contracts_query.append("WHERE (issuerID = {OWNER_ID} OR assigneeID = {OWNER_ID} OR acceptorID = {OWNER_ID}) ");
    } else {
        bool issued = issuedToBy.value()->AsBool()->value();
        if (issued) {
            contracts_query.append("WHERE (assigneeID = {OWNER_ID} OR acceptorID = {OWNER_ID}) ");
        } else {
            contracts_query.append("WHERE issuerID = {OWNER_ID} ");
        }
    }
    if (contractType.has_value() == false) {
        contracts_query.append("AND contractType IN (1,2,3) ");
    } else {
        contracts_query.append("AND contractType = " + std::to_string(contractType.value()->value()) + " ");
    }
    contracts_query.append("AND status = " + std::to_string(contractStatus->AsInt()->value()));
    boost::replace_all(contracts_query, "{OWNER_ID}", std::to_string(ownerID->AsInt()->value()));

    DBQueryResult res;
    if (!sDatabase.RunQuery(res, contracts_query.c_str()))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return nullptr;
    }

    PyObjectEx* contracts = DBResultToCRowset(res);
    for (auto contract : contracts->list()) {
        // To get items, we store contractID's in separate list
        contractIDs.push_back(contract->AsPackedRow()->GetField(0)->AsInt()->value());
    }

    PyDict* items = new PyDict;
    if (!contractIDs.empty()) {
        // Batch the "IN (...)" so a huge contract list cannot kill the DB link (#2006).
        const size_t BATCH = 500;
        std::map<int, CRowSet*> itemsByContractID;
        DBResultRow row;
        for (size_t start = 0; start < contractIDs.size(); start += BATCH) {
            size_t end = std::min(start + BATCH, contractIDs.size());
            std::string batch_query = items_query;
            for (size_t i = start; i < end; ++i)
                batch_query.append(std::to_string(contractIDs[i]) + ",");
            batch_query.pop_back(); batch_query.append(")");

            if (!sDatabase.RunQuery(res, batch_query.c_str()))
            {
                codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
                continue;
            }

            while (res.GetRow(row)) {
                auto pos = itemsByContractID.find(row.GetInt(0));
                if (pos == itemsByContractID.end()) {
                    DBRowDescriptor *header = new DBRowDescriptor(res);
                    CRowSet *rowset = new CRowSet(&header);

                    PyPackedRow* packedRow = rowset->NewRow();
                    PySetFieldRelease(packedRow, "contractId", new PyInt(row.GetInt(0)));
                    PySetFieldRelease(packedRow, "itemTypeID", new PyInt(row.GetInt(1)));
                    PySetFieldRelease(packedRow, "quantity", new PyInt(row.GetInt(2)));
                    PySetFieldRelease(packedRow, "inCrate", new PyBool(row.GetBool(3)));

                    itemsByContractID[row.GetInt(0)] = rowset;
                } else {
                    CRowSet* rowset = pos->second;

                    PyPackedRow* packedRow = rowset->NewRow();
                    PySetFieldRelease(packedRow, "contractId", new PyInt(row.GetInt(0)));
                    PySetFieldRelease(packedRow, "itemTypeID", new PyInt(row.GetInt(1)));
                    PySetFieldRelease(packedRow, "quantity", new PyInt(row.GetInt(2)));
                    PySetFieldRelease(packedRow, "inCrate", new PyBool(row.GetBool(3)));
                }
            }
        }
        for (auto entry : itemsByContractID) {
            PySetItemRelease(items, new PyInt(entry.first), entry.second);
        }
    }

    PyDict* ret = new PyDict;
    ret->SetItemString("contracts", contracts);
    ret->SetItemString("items", items);

    return new PyObject("util.KeyVal", ret);
}

/**
 * Service function - populates PackedRow with values from DBResultRow for ctrItems
 * @param itemRow - DB ctrItems row
 * @param targetRow - target PyPackedRow
 */
void ContractUtils::FillItemData(DBResultRow *itemRow, PyPackedRow *targetRow) {
    PySetFieldRelease(targetRow, "contractID", new PyInt(itemRow->GetInt(0)));
    PySetFieldRelease(targetRow, "itemID", new PyInt(itemRow->GetInt(1)));
    PySetFieldRelease(targetRow, "quantity", new PyInt(itemRow->GetInt(2)));
    PySetFieldRelease(targetRow, "itemTypeID", new PyInt(itemRow->GetInt(3)));
    PySetFieldRelease(targetRow, "inCrate", new PyBool(itemRow->GetBool(4)));
    PySetFieldRelease(targetRow, "parentID", new PyInt(itemRow->GetInt(5)));
    PySetFieldRelease(targetRow, "productivityLevel", new PyInt(itemRow->GetInt(6)));
    PySetFieldRelease(targetRow, "materialLevel", new PyInt(itemRow->GetInt(7)));
    PySetFieldRelease(targetRow, "copy", new PyBool(itemRow->GetBool(8)));
    PySetFieldRelease(targetRow, "licensedProductionRunsRemaining", new PyInt(itemRow->GetInt(9)));
    PySetFieldRelease(targetRow, "damage", new PyInt(itemRow->GetInt(10)));
    PySetFieldRelease(targetRow, "flagID", new PyInt(itemRow->GetInt(11)));
}

/**
 * Service function - populates PackedRow with values from DBResultRow for ctrBids
 * @param itemRow - DB ctrBids row
 * @param targetRow - target PyPackedRow
 */
void ContractUtils::FillBidData(DBResultRow *bidRow, PyPackedRow *targetRow) {
    PySetFieldRelease(targetRow, "contractId", new PyInt(bidRow->GetInt(0)));
    PySetFieldRelease(targetRow, "amount", new PyInt(bidRow->GetInt(1)));
    PySetFieldRelease(targetRow, "bidderID", new PyInt(bidRow->GetInt(2)));
    PySetFieldRelease(targetRow, "timeBid", new PyLong(bidRow->GetInt64(3)));
}

/**
 * Queries the list of entityID's for a given contract. Primarily used to get the list of items to be returned to owner
 * upon contract's deletion.
 * @param contractId - Contract ID to search items in
 * @param into - Vector instance that will contain the entityID's
 */
void ContractUtils::GetContractItemIDs(int contractId, std::vector<int> *into) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, getContractItemsShortQueryBase.c_str(), std::to_string(contractId).c_str()))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        into->push_back(row.GetInt(0));
    }
}

/**
 * Queries requested items for a contract and populates provided map with itemType and quantities
 * @param contractId
 * @param into
 */
void ContractUtils::GetRequestedItems(int contractId, std::map<int32, int32> *into) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT itemTypeID, quantity FROM ctrItems WHERE contractID = %u AND inCrate = false", contractId))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        into->insert(std::pair<int, int> (row.GetInt(0), row.GetInt(1)));
    }
}

/**
 * Queries traded items and populates the map as ItemID <-> quantity
 * @param contractId - Contract ID to query
 * @param into - Target map
 */
void ContractUtils::GetContractItemIDsAndQuantities(int contractId, std::map<int, int> *into) {
    DBQueryResult res;
    if (!sDatabase.RunQuery(res, "SELECT itemID, quantity FROM ctrItems WHERE contractID = %u AND inCrate = true", contractId))
    {
        codelog(DATABASE__ERROR, "Error in query: %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        into->insert(std::pair<int, int> (row.GetInt(0), row.GetInt(1)));
    }
}

void ContractUtils::GetCrateContentsRecursive(uint32 crateID, std::map<int, int>* into)
{
    // Query all items located inside the crate (or inside containers in the crate)
    std::vector<uint32> toProcess;
    toProcess.push_back(crateID);

    while (!toProcess.empty()) {
        uint32 parentID = toProcess.back();
        toProcess.pop_back();

        DBQueryResult res;
        if (!sDatabase.RunQuery(res,
            "SELECT itemID, quantity, typeID, groupID"
            " FROM entity"
            "  LEFT JOIN invTypes USING (typeID)"
            " WHERE locationID = %u", parentID))
        {
            continue;
        }

        DBResultRow row;
        while (res.GetRow(row)) {
            uint32 itemID = row.GetUInt(0);
            uint32 qty = row.GetUInt(1);
            uint32 typeID = row.GetUInt(2);
            uint32 groupID = row.GetUInt(3);

            into->insert(std::pair<int, int>(itemID, qty));

            // If this item is a container, recurse into it
            // Container groups include: 9=CargoContainer, 12=SecureContainer,
            // 20=AuditLogSecureContainer, 340=FreightContainer, 448=AuditLogSecureContainer
            if (groupID == 9 || groupID == 12 || groupID == 20 ||
                groupID == 340 || groupID == 448 || groupID == 649) {
                toProcess.push_back(itemID);
            }
        }
    }
}


