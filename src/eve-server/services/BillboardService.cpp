#include "eve-server.h"
#include "EVEServerConfig.h"
#include "services/BillboardService.h"

BillboardService::BillboardService(EVEServiceManager& mgr) :
    Service("billboardMgr", eAccessLevel_Character)
{
    this->Add("GetServerMessages", &BillboardService::GetServerMessages);
    this->Add("GetBillboardData", &BillboardService::GetBillboardData);
}

PyResult BillboardService::GetServerMessages(PyCallArgs& call) {
    PyDict* result = new PyDict();
    // MOTD text — configured in server config or DB
    std::string motd = sConfig.world.Motd;
    if (!motd.empty())
        result->SetItemString("motd", new PyString(motd));
    // Optional news items
    if (!sConfig.world.NewsHeadlines.empty())
        result->SetItemString("news", new PyString(sConfig.world.NewsHeadlines));
    return new PyObject("util.KeyVal", result);
}

PyResult BillboardService::GetBillboardData(PyCallArgs& call, PyInt* locationID) {
    // Returns station-specific billboard content
    // locationID = stationID or solarSystemID
    PyDict* result = new PyDict();
    result->SetItemString("locationID", locationID);
    // Default station-specific message
    char buf[256];
    snprintf(buf, sizeof(buf), "Welcome to station %u", locationID->value());
    result->SetItemString("stationText", new PyString(buf));
    result->SetItemString("bountyText", new PyString("Top bounties: check charMgr.GetTopBounties()"));
    return new PyObject("util.KeyVal", result);
}
