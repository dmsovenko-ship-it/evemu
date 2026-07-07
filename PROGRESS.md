# EVEmu Crucible — Fork Progress

> **Overall: ~82%** (upstream: 59.5%)  
> Last updated: 2026-07-08  
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

## Summary Table

| System | % | System | % | System | % |
|--------|---|--------|---|--------|---|
| Account & Character | 97% | Skills & Certificates | 99% | Ship Navigation | 95% |
| Combat | 99% | Modules | 92% | **Drones** | **90%** |
| **NPC AI & Spawning** | **80%** | Agents | 85% | Missions | 70% |
| **Wormholes** | **80%** | Fleet | 98% | Market | 55% |
| Contracts | 50% | Corporation | 70% | Alliance | 30% |
| Science & Industry | 40% | POS | 65% | Planetary Interaction | 72% |
| **Incursions** | **75%** | Scanning | 99% | LSC (Chat) | 70% |
| EvE Mail | 45% | Calendar | 65% | Standings | 40% |
| Effects System | 88% | Anomaly Manager | 85% | Spawn Manager | 75% |
| Dungeon Manager | 70% | Civilian Manager | 10% | Memory Management | 20% |

## Key Enhancements Over Upstream

| Feature | Status | Notes |
|---------|--------|-------|
| NPC Ship-category typeIDs | ✅ | 33500-33523 for crosshair rendering |
| Warp-to-0 / Fleet warp | ✅ | Desync fix, autopilot chain |
| Crimewatch timers | ✅ | Weapon/aggression/criminal/combat logoff |
| CONCORD & Sentry | ✅ | Police ×25 HP, delay by sec, sec loss |
| Drones: full AI, skills, subtypes | ✅ | Combat/EWAR/logistics/mining/fighters |
| Kill rights | ✅ | Grant, auto-activation, Limited Engagement |
| Contraband / Customs police | ✅ | Gate scan, fine, confiscation, faction NPCs |
| Incursion system | ✅ | State machine, wave spawning, contest rewards |
| Jump clones + per-clone implants | ✅ | Clone jump, install/destroy, implant assignment |
| War decay timer | ✅ | Auto-end wars on unpaid bills |
| Contract auctions | ✅ | PlaceBid/FinishAuction + ISK transfer |
| Ship fittings manager | ✅ | Full CRUD (character + corporation) |
| Invention | ✅ | Chance calculation + T2 BPC creation |
| Mailing lists | ✅ | Join/Leave/Delete/Kick/EntityAccess |
| GM commands | ✅ | dogma get/list/set, colored help outputs |
| SDE verification vs live ESI | ✅ | Graphics, sounds, effects, groups confirmed |
| PyInt memory cache | ✅ | -10..255 cached in PyStatic::NewInt() |
| Agent portrait fallback | ✅ | images.evetech.net redirect |
| Image server | ✅ | Built-in HTTP, portrait/logo serving |
