
/**
 * @name Tower.h
 *   Class for POS Towers.
 *
 * @Author:         Allan
 * @date:   28 December 17
 */


#ifndef EVEMU_POS_TOWER_H_
#define EVEMU_POS_TOWER_H_


#include "pos/Structure.h"

class Misile;

class TowerSE
: public StructureSE
{
public:
    TowerSE(StructureItemRef structure, EVEServiceManager& services, SystemManager* system, const FactionData& fData);
    virtual ~TowerSE()                                  { /* do nothing here */ }

    /* class type pointer querys. */
    virtual TowerSE*            GetTowerSE()            { return this; }

    /* class type tests. */
    virtual bool                IsTowerSE()             { return true; }

    /* SystemEntity interface */
    virtual void                Process();
    virtual PyDict*             MakeSlimItem();

    /* virtual functions default to base class and overridden as needed */
    virtual void                Init();
    virtual void                InitData();

    /* virtual functions to be overridden in derived classes */
    virtual void     MissileLaunched(Missile* pMissile) { /* Do nothing here */ }


    /* basic tower methods */
    virtual void                SetOnline();
    virtual void                SetOffline();

    virtual void                Online();
    virtual void                Operating();
    virtual void                Reinforced();

    virtual void                Scoop();

    void                        ReinforceTower();

    /* tower data methods */
    PyRep* GetDeployFlags();
    PyRep* GetUsageFlagList();
    PyRep* GetProcessInfo();

    void UpdatePassword();
    void SetDeployFlags(int8 anchor=0, int8 unanchor=0, int8 online=0, int8 offline=0);
    void SetUseFlags(uint32 itemID, int8 view=0, int8 take=0, int8 use=0);

    /* general tower data methods */
    void SetStanding(float set)                         { m_tdata.standing = set; }
    void SetStatus(float set)                           { m_tdata.status = set; }
    void SetStatusDrop(bool set)                        { m_tdata.statusDrop = set; }   // shoot if their status is dropping
    void SetCorpWar(bool set)                           { m_tdata.corpWar = set; }      // shoot if we are at war with them
    void SetCorpAccess(bool set)                        { m_tdata.allowCorp = set; }
    void SetAllyAccess(bool set)                        { m_tdata.allowAlliance = set; }
    void SetUseAllyStandings(bool set)                  { m_tdata.allyStandings = set; }    // use alliance standings
    void SetStandingOwnerID(bool set)                   { m_tdata.standingOwnerID = set; }
    void SetPassword(std::string pass)                  { m_tdata.password = pass; }

    bool HasForceField()                                { return (m_hasShield? true : false); }
    bool GetStatusDrop()                                { return m_tdata.statusDrop; }
    bool GetCorpWar()                                   { return m_tdata.corpWar; }
    float GetStanding()                                 { return m_tdata.standing; }
    uint32 GetStandingOwnerID()                         { return m_tdata.standingOwnerID; }
    std::string GetPassword()                           { return m_tdata.password; }

    bool ShowInCalendar()                               { return m_tdata.showInCalendar; }
    bool SendFuelNotifications()                        { return m_tdata.sendFuelNotifications; }
    void SetShowInCalendar(bool set)                    { m_tdata.showInCalendar = set; }
    void SetSendFuelNotifications(bool set)             { m_tdata.sendFuelNotifications = set; }

    void GetTowerData(EVEPOS::TowerData& tData)         { tData = m_tdata; }

    uint16 GetSOI()                                     { return m_soi; }

    inline void AddModule(StructureSE* pSE)             { m_structs.emplace(pSE->GetID(), pSE); }
    inline void RemoveModule(StructureSE* pSE)          { m_structs.erase(pSE->GetID()); }

    // structure update methods
    void UpdateAccess()                                 { m_db.UpdateAccess(m_data.itemID, m_tdata); }
    void UpdateSentry()                                 { m_db.UpdateSentry(m_data.itemID, m_tdata); }
    void UpdateNotify()                                 { m_db.UpdateNotify(m_data.itemID, m_tdata); }
    void UpdatePermission()                             { m_db.UpdatePermission(m_data.itemID, m_tdata); }

    // fx methods
    void ApplyResist(uint16 type, float amount)         { /* do nothing here */ }
    void RemoveResist(uint16 type, float amount)        { /* do nothing here */ }

    // resource methods
    void OnlineModule(StructureSE* pSE);
    void OfflineModule(StructureSE* pSE);
    void RecalcResources();

    bool HasPG(float amount);
    bool HasCPU(float amount);

    float GetPGLoad()                                   { return m_pgUsed; }
    float GetCPULoad()                                  { return m_cpuUsed; }
    float GetPGTotal()                                  { return m_pg; }
    float GetCPUTotal()                                 { return m_cpu; }

    // fuel consumption
    void InitFuelData();
    bool CheckFuel();
    uint32 GetFuelTypeID()                              { return m_fuelTypeID; }
    uint32 GetFuelPerHour()                             { return m_fuelPerHour; }
    uint32 GetStrontPerHour()                           { return m_strontPerHour; }

protected:
    EVEPOS::TowerData m_tdata;

    SystemEntity* m_pShieldSE;
    //POS_AI* m_ai;

private:
    void CreateForceField();

    bool m_hasShield;

    int8 m_tsize;   // tower size: small, med, large

    float m_pg;
    float m_cpu;

    uint16 m_soi;   // Sphere Of Influence, 45km max

    float m_pgUsed;             // current PG usage from online modules
    float m_cpuUsed;            // current CPU usage from online modules

    uint32 m_fuelTypeID;        // typeID of fuel consumed (e.g. 4247 = Fuel Blocks)
    uint32 m_fuelPerHour;       // fuel units consumed per hour (10/20/40 for S/M/L)
    uint32 m_strontTypeID;      // typeID of strontium for reinforced (16275)
    uint32 m_strontPerHour;     // strontium consumed per reinforced hour (100/200/400)
    int64  m_lastFuelCheck;     // FileTime of last fuel consumption check

    std::map<uint32, StructureSE*> m_structs;  // structID/pSSE

};

#endif  // EVEMU_POS_TOWER_H_
