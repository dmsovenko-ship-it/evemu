#include "eve-server.h"
#include "services/BillboardService.h"

BillboardService::BillboardService(EVEServiceManager& mgr) :
    Service("billboardMgr", eAccessLevel_Character)
{
    this->Add("GetServerMessages", &BillboardService::GetServerMessages);
    this->Add("GetBillboardData", &BillboardService::GetBillboardData);
}

PyResult BillboardService::GetServerMessages(PyCallArgs& call) {
    PyDict* result = new PyDict();
    result->SetItemString("motd", new PyString("Welcome to EVEmu"));
    result->SetItemString("news", new PyString("Server is online"));
    return new PyObject("util.KeyVal", result);
}

PyResult BillboardService::GetBillboardData(PyCallArgs& call, PyInt* locationID) {
    PyDict* result = new PyDict();
    result->SetItemString("locationID", locationID);
    char buf[256];
    snprintf(buf, sizeof(buf), "Welcome to station %u", locationID->value());
    result->SetItemString("stationText", new PyString(buf));
    return new PyObject("util.KeyVal", result);
}
