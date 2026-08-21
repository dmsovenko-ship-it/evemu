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
    Author:        Allan
*/

#include "eve-server.h"

#include "Client.h"


#include "Probes.h"
#include "exploration/ScanMgrService.h"
#include "system/DestinyManager.h"
#include "system/SystemManager.h"
#include "system/SystemBubble.h"
#include "services/ServiceManager.h"

/* SCAN__ERROR
 * SCAN__WARNING
 * SCAN__MESSAGE
 * SCAN__DEBUG
 * SCAN__INFO
 * SCAN__TRACE
 * SCAN__DUMP
 */

ScanMgrService::ScanMgrService(EVEServiceManager& mgr) :
    Service("scanMgr", eAccessLevel_SolarSystem2),
    m_manager (mgr)
{
    this->Add("GetSystemScanMgr", &ScanMgrService::GetSystemScanMgr);
}
void ScanMgrService::BoundReleased (ScanBound* bound) {
    auto it = this->m_instances.find (bound->GetClient());

    if (it == this->m_instances.end ())
        return;

    this->m_instances.erase (it);
}

PyResult ScanMgrService::GetSystemScanMgr(PyCallArgs& call) {
    DestinyManager* pDestiny = call.client->GetShipSE()->DestinyMgr();
    if (pDestiny == nullptr) {
        codelog(CLIENT__ERROR, "%s: Client has no destiny manager!", call.client->GetName());
        return PyStatic.NewNone();
    }

    ScanBound* pSB;
    auto it = this->m_instances.find (call.client);

    if (it == this->m_instances.end ()) {
        pSB = new ScanBound(m_manager, *this, call.client);
    } else {
        pSB = it->second;
    }

    pSB->NewReference (call.client);

    return new PySubStruct(new PySubStream(pSB->GetOID()));
}

ScanBound::ScanBound(EVEServiceManager& mgr, ScanMgrService& parent, Client* client) :
    EVEBoundObject(mgr, parent),
    m_client(client)
{
    this->Add("ConeScan", &ScanBound::ConeScan);
    this->Add("RequestScans", &ScanBound::RequestScans);
    this->Add("RecoverProbes", &ScanBound::RecoverProbes);
    this->Add("DestroyProbe", &ScanBound::DestroyProbe);
    this->Add("ReconnectToLostProbes", &ScanBound::ReconnectToLostProbes);
}

PyResult ScanBound::ConeScan( PyCallArgs& call, PyRep* ignored1, PyRep* ignored2, PyRep* ignored3, PyRep* ignored4, PyRep* ignored5) {
    //result = sm.GetService('scanSvc').ConeScan(self.scanangle, rnge * 1000, vec.x, vec.y, vec.z)
    //return sm.RemoteSvc('scanMgr').GetSystemScanMgr().ConeScan(scanangle, scanRange, x, y, z)
    //_log(SCAN__TRACE, "ScanBound::Handle_ConeScan() - size=%lli", call.tuple->size());
    //call.Dump(SCAN__DUMP);

    // TODO: for this one we're keeping the old mechanism for now as it requires changes on more than just this service
    Call_ConeScan args;
    if (!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "ScanBound: Failed to decode arguments.");
        //TODO: throw exception
        return PyStatic.NewNone();
    }

    DestinyManager* pDestiny = m_client->GetShipSE()->DestinyMgr();
    if (pDestiny == nullptr) {
        codelog(CLIENT__ERROR, "%s: Client has no destiny manager!", m_client->GetName());
        return PyStatic.NewNone();
    } else if (pDestiny->IsWarping()) {
        call.client->SendNotifyMsg( "You can't scan while warping");
        return PyStatic.NewNone();
    }

    if (m_client->GetShipSE()->SysBubble() == nullptr)
        sBubbleMgr.Add(m_client->GetShipSE());

    if (m_client->scan() == nullptr)
        m_client->SetScan(new Scan(m_client));

    /** @todo  2s reset time on dscan */

    return m_client->scan()->ConeScan(args);
}

PyResult ScanBound::RequestScans(PyCallArgs& call, std::optional <PyDict*> probes) {
    _log(SCAN__TRACE, "ScanBound::Handle_RequestScans() - size=%lli", call.tuple->size());
    call.Dump(SCAN__DUMP);

    DestinyManager* pDestiny = m_client->GetShipSE()->DestinyMgr();
    if (pDestiny == nullptr) {
        codelog(CLIENT__ERROR, "%s: Client has no destiny manager!", call.client->GetName());
        return PyStatic.NewNone();
    } else if (pDestiny->IsWarping()) {
        // The client's scanSvc.RequestScans() sets scanningProbes BEFORE calling us
        // (scanningProbes = [session.shipid] for a ship scan, or the probe ids), and
        // only clears it on OnSystemScanStarted/OnSystemScanStopped. If we reject
        // with just a notify, the scanner window's CheckButtonStates keeps the
        // Analyze button disabled forever. Send an empty OnSystemScanStopped so the
        // client resets scanningProbes/currentScan and the button re-enables.
        call.client->SendNotifyMsg("You can't scan while warping");
        OnSystemScanStopped osss;
            osss.scanProbesDict = new PyDict();
            osss.systemScanResult = new PyList();
            osss.absentTargets = new PyList();
        PyTuple* ev = osss.Encode();
        call.client->SendNotification("OnSystemScanStopped", "charid", &ev);
        return PyStatic.NewNone();
    }

    if (m_client->GetShipSE()->SysBubble() == nullptr)
        sBubbleMgr.Add(m_client->GetShipSE());

    if (m_client->scan() == nullptr)
        m_client->SetScan(new Scan(m_client));

    m_client->scan()->RequestScans(probes.has_value() ? probes.value() : nullptr);

    // this call returns a PyNone
    return PyStatic.NewNone();
}

PyResult ScanBound::RecoverProbes(PyCallArgs& call, PyList* probeIDs) {
    //successProbeIDs = sm.RemoteSvc('scanMgr').GetSystemScanMgr().RecoverProbes(probeIDs)
    // list of probes successfully scooped to cargo
    // this is tested and added in Probe::RecoverProbe()
    PyList* list = new PyList();

    std::vector<int32> ints;

    PyList::const_iterator list_2_cur = probeIDs->begin();
    for (size_t list_2_index(0); list_2_cur != probeIDs->end(); ++list_2_cur, ++list_2_index) {
        if (!(*list_2_cur)->IsInt()) {
            _log(XMLP__DECODE_ERROR, "Decode Call_SingleIntList failed: Element %u in list list_2 is not an integer: %s", list_2_index, (*list_2_cur)->TypeString());
            return nullptr;
        }

        const PyInt* t = (*list_2_cur)->AsInt();
        ints.push_back(t->value());
    }

    SystemEntity* pSE(nullptr);
    for (auto cur : ints) {
        pSE = m_client->SystemMgr()->GetSE(cur);
        if (pSE == nullptr)
            continue;
        if (pSE->IsProbeSE())
            pSE->GetProbeSE()->RecoverProbe(list);
    }

    return list;
}

PyResult ScanBound::DestroyProbe(PyCallArgs& call, PyInt* probeID) {
    //scanMan = sm.RemoteSvc('scanMgr').GetSystemScanMgr()
    //scanMan.DestroyProbe(probeID)
    _log(SCAN__TRACE, "ScanBound::Handle_DestroyProbe() - size=%lli", call.tuple->size());
    call.Dump(SCAN__DUMP);

    SystemEntity* pSE(m_client->SystemMgr()->GetSE(probeID->value()));
    if (pSE != nullptr)
        pSE->Delete();
    SafeDelete(pSE);

    return nullptr;
}

PyResult ScanBound::ReconnectToLostProbes(PyCallArgs& call) {
    // Reconnect to probes that were in space before disconnect/session change.
    // Resend all probes in the current system to the client.
    _log(SCAN__TRACE, "ScanBound::ReconnectToLostProbes()");

    SystemManager* sysMgr = m_client->SystemMgr();
    if (sysMgr == nullptr)
        return nullptr;

    std::map<uint32, SystemEntity*> entities = sysMgr->GetEntities();
    for (auto& itr : entities) {
        SystemEntity* pSE = itr.second;
        if (pSE == nullptr or !pSE->IsProbeSE())
            continue;
        pSE->GetProbeSE()->SendNewProbe();
    }

    return nullptr;
}
