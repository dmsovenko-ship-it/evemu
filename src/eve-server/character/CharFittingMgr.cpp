#include "eve-server.h"
#include "character/CharFittingMgr.h"

CharFittingMgr::CharFittingMgr() :
    Service("charFittingMgr")
{
    this->Add("GetFittings", &CharFittingMgr::GetFittings);
    this->Add("SaveFitting", &CharFittingMgr::SaveFitting);
    this->Add("SaveManyFittings", &CharFittingMgr::SaveManyFittings);
    this->Add("DeleteFitting", &CharFittingMgr::DeleteFitting);
    this->Add("UpdateNameAndDescription", &CharFittingMgr::UpdateNameAndDescription);
}

PyResult CharFittingMgr::GetFittings(PyCallArgs &call, PyInt* ownerID) {
    uint32 charID = ownerID->value();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT id, shipID, shipDNA, fittingName, description FROM chrShipFittings WHERE characterID = %u",
        charID))
    {
        return new PyList();
    }

    PyList* fittings = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* entry = new PyDict();
        entry->SetItemString("fittingID", PyStatic.NewInt(row.GetUInt(0)));
        entry->SetItemString("shipID", PyStatic.NewInt(row.GetUInt(1)));
        entry->SetItemString("shipDNA", new PyString(row.GetText(2)));
        entry->SetItemString("fittingName", new PyString(row.GetText(3)));
        entry->SetItemString("description", new PyString(row.GetText(4)));
        fittings->AddItem(new PyObject("util.KeyVal", entry));
    }
    return fittings;
}

PyResult CharFittingMgr::SaveFitting(PyCallArgs &call, PyInt* ownerID, PyObject* fitting) {
    uint32 charID = ownerID->value();
    // fitting is util.KeyVal with: shipID, shipDNA, name, description, items
    PyDict* fDict = fitting->AsDict();
    if (fDict == nullptr) return PyStatic.NewZero();

    PyRep* shipID_r = fDict->GetItemString("shipID");
    PyRep* shipDNA_r = fDict->GetItemString("shipDNA");
    PyRep* name_r = fDict->GetItemString("fittingName");
    PyRep* desc_r = fDict->GetItemString("description");
    if (shipID_r == nullptr || shipDNA_r == nullptr) return PyStatic.NewZero();

    uint32 shipID = PyRep::IntegerValueU32(shipID_r);
    std::string shipDNA = PyRep::StringContent(shipDNA_r);
    std::string fName = name_r ? PyRep::StringContent(name_r) : "";
    std::string fDesc = desc_r ? PyRep::StringContent(desc_r) : "";

    std::string dnaEscaped, nameEscaped, descEscaped;
    sDatabase.DoEscapeString(dnaEscaped, shipDNA);
    sDatabase.DoEscapeString(nameEscaped, fName);
    sDatabase.DoEscapeString(descEscaped, fDesc);

    uint32 newID = 0;
    DBerror err;
    if (sDatabase.RunQueryLID(err, newID,
        "INSERT INTO chrShipFittings (characterID, shipID, shipDNA, fittingName, description) "
        "VALUES (%u, %u, '%s', '%s', '%s')",
        charID, shipID, dnaEscaped.c_str(), nameEscaped.c_str(), descEscaped.c_str()))
    {
        return PyStatic.NewInt(newID);
    }
    return PyStatic.NewZero();
}

PyResult CharFittingMgr::SaveManyFittings(PyCallArgs &call, PyInt* ownerID, PyDict* fittingsToSave) {
    uint32 charID = ownerID->value();
    // fittingsToSave is dict mapping tempID -> fitting KeyVal
    PyList* result = new PyList();
    auto itr = fittingsToSave->begin();
    for (; itr != fittingsToSave->end(); ++itr) {
        PyRep* tempID = itr->first;
        PyObject* fitting = itr->second->AsObject();
        if (fitting == nullptr) continue;

        PyDict* fDict = fitting->AsDict();
        if (fDict == nullptr) continue;

        PyRep* shipID_r = fDict->GetItemString("shipID");
        PyRep* shipDNA_r = fDict->GetItemString("shipDNA");
        PyRep* name_r = fDict->GetItemString("fittingName");
        if (shipID_r == nullptr || shipDNA_r == nullptr) continue;

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
            charID, shipID, dnaEscaped.c_str(), nameEscaped.c_str()))
        {
            PyTuple* pair = new PyTuple(2);
            pair->SetItem(0, tempID);
            pair->SetItem(1, PyStatic.NewInt(newID));
            result->AddItem(new PyObject("util.KeyVal", pair));
        }
    }
    return result;
}

PyResult CharFittingMgr::DeleteFitting(PyCallArgs &call, PyInt* ownerID, PyInt* fittingID) {
    DBerror err;
    sDatabase.RunQuery(err,
        "DELETE FROM chrShipFittings WHERE id = %u AND characterID = %u",
        fittingID->value(), ownerID->value());
    return PyStatic.NewTrue();
}

PyResult CharFittingMgr::UpdateNameAndDescription(PyCallArgs &call, PyInt* fittingID, PyInt* ownerID, PyWString* name, PyWString* description) {
    std::string nameEscaped, descEscaped;
    sDatabase.DoEscapeString(nameEscaped, name->content());
    sDatabase.DoEscapeString(descEscaped, description->content());

    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE chrShipFittings SET fittingName = '%s', description = '%s' WHERE id = %u AND characterID = %u",
        nameEscaped.c_str(), descEscaped.c_str(), fittingID->value(), ownerID->value());
    return PyStatic.NewTrue();
}
