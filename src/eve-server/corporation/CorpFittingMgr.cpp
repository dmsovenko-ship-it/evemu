#include "eve-server.h"
#include "corporation/CorpFittingMgr.h"

CorpFittingMgr::CorpFittingMgr() :
    Service("corpFittingMgr", eAccessLevel_Corporation)
{
    this->Add("GetFittings", &CorpFittingMgr::GetFittings);
    this->Add("SaveFitting", &CorpFittingMgr::SaveFitting);
    this->Add("SaveManyFittings", &CorpFittingMgr::SaveManyFittings);
    this->Add("DeleteFitting", &CorpFittingMgr::DeleteFitting);
    this->Add("UpdateNameAndDescription", &CorpFittingMgr::UpdateNameAndDescription);
}

PyResult CorpFittingMgr::GetFittings(PyCallArgs &call, PyInt* ownerID) {
    uint32 corpID = ownerID->value();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT id, shipID, shipDNA, fittingName, description FROM chrShipFittings WHERE characterID = %u",
        corpID))
    {
        return new PyList();
    }

    PyList* fittings = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("fittingID", PyStatic.NewInt(row.GetUInt(0)));
        entry->SetItemString("shipTypeID", PyStatic.NewInt(row.GetUInt(1)));
        entry->SetItemString("shipDNA", new PyString(row.GetText(2)));
        entry->SetItemString("name", new PyString(row.GetText(3)));
        entry->SetItemString("description", new PyString(row.GetText(4)));
        fittings->AddItem(new PyObject("util.KeyVal", entry));
    }
    return fittings;
}

PyResult CorpFittingMgr::SaveFitting(PyCallArgs &call, PyInt* ownerID, PyObject* fitting) {
    uint32 corpID = ownerID->value();
    PyDict* fDict = fitting->arguments()->AsDict();
    if (fDict == nullptr) return PyStatic.NewZero();

    PyRep* shipID_r = fDict->GetItemString("shipTypeID");
    PyRep* shipDNA_r = fDict->GetItemString("shipDNA");
    PyRep* name_r = fDict->GetItemString("name");
    if (shipID_r == nullptr || shipDNA_r == nullptr) return PyStatic.NewZero();

    uint32 shipID = PyRep::IntegerValueU32(shipID_r);
    std::string shipDNA = PyRep::StringContent(shipDNA_r);
    std::string fName = name_r ? PyRep::StringContent(name_r) : "";

    std::string dnaEscaped, nameEscaped;
    sDatabase.DoEscapeString(dnaEscaped, shipDNA);
    sDatabase.DoEscapeString(nameEscaped, fName);

    uint32 newID = 0;
    DBerror err;
    if (sDatabase.RunQueryLID(err, newID,
        "INSERT INTO chrShipFittings (characterID, shipID, shipDNA, fittingName) "
        "VALUES (%u, %u, '%s', '%s')",
        corpID, shipID, dnaEscaped.c_str(), nameEscaped.c_str()))
    {
        return PyStatic.NewInt(newID);
    }
    return PyStatic.NewZero();
}

PyResult CorpFittingMgr::SaveManyFittings(PyCallArgs &call, PyInt* ownerID, PyDict* fittingsToSave) {
    uint32 corpID = ownerID->value();
    PyList* result = new PyList();
    auto itr = fittingsToSave->begin();
    for (; itr != fittingsToSave->end(); ++itr) {
        PyRep* tempID = itr->first;
        PyObject* fitting = itr->second->AsObject();
        if (fitting == nullptr) continue;

        PyDict* fDict = fitting->arguments()->AsDict();
        if (fDict == nullptr) continue;

        PyRep* shipID_r = fDict->GetItemString("shipTypeID");
        PyRep* shipDNA_r = fDict->GetItemString("shipDNA");
        if (shipID_r == nullptr || shipDNA_r == nullptr) continue;

        uint32 shipID = PyRep::IntegerValueU32(shipID_r);
        std::string shipDNA = PyRep::StringContent(shipDNA_r);

        std::string dnaEscaped;
        sDatabase.DoEscapeString(dnaEscaped, shipDNA);

        uint32 newID = 0;
        DBerror err;
        if (sDatabase.RunQueryLID(err, newID,
            "INSERT INTO chrShipFittings (characterID, shipID, shipDNA) VALUES (%u, %u, '%s')",
            corpID, shipID, dnaEscaped.c_str()))
        {
            PyTuple* pair = new PyTuple(2);
            pair->SetItem(0, tempID);
            pair->SetItem(1, PyStatic.NewInt(newID));
            result->AddItem(new PyObject("util.KeyVal", pair));
        }
    }
    return result;
}

PyResult CorpFittingMgr::DeleteFitting(PyCallArgs &call, PyInt* ownerID, PyInt* fittingID) {
    DBerror err;
    sDatabase.RunQuery(err,
        "DELETE FROM chrShipFittings WHERE id = %u AND characterID = %u",
        fittingID->value(), ownerID->value());
    return PyStatic.NewTrue();
}

PyResult CorpFittingMgr::UpdateNameAndDescription(PyCallArgs &call, PyInt* fittingID, PyInt* ownerID, PyWString* name, PyWString* description) {
    std::string nameEscaped, descEscaped;
    sDatabase.DoEscapeString(nameEscaped, name->content());
    sDatabase.DoEscapeString(descEscaped, description->content());

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE chrShipFittings SET fittingName = '%s', description = '%s' WHERE id = %u AND characterID = %u",
        nameEscaped.c_str(), descEscaped.c_str(), fittingID->value(), ownerID->value());
    return PyStatic.NewTrue();
}
