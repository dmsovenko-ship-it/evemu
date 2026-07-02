# EVEmu Crucible — Project Status

> Last updated: 2026-07-02 (end of session)
> Based on codebase analysis of [dmsovenko-ship-it/evemu](https://github.com/dmsovenko-ship-it/evemu) (fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible))

---

## Warp & Movement

| Feature | Status | Notes |
|---------|--------|-------|
| Warp to object at range | ✅ Working | Stop distance, landing offset |
| Warp to 0 | ✅ Working | ~1000m landing offset |
| Warp deceleration | ✅ Working | No 180-degree flip |
| Immediate re-warp | ✅ Working | No freeze or desync |
| Autopilot chain | ✅ Working | State preserved across gate jumps |
| Gate jump | ✅ Working | 5km offset spawn, 12s align timeout |
| Align-to | ✅ Working | GOTO mode via DestinyManager |
| Orbit entry | ✅ Working | Smooth transition |
| MWD/Afterburner | ✅ Working | ActiveModule handling |
| Fleet warp (WarpToMember) | ✅ Added | `CmdWarpToStuff` type="char" |
| Fleet warp (WarpFleetToMember) | ✅ Added | fleet=1 flag, warps all members in system |
| Fleet Regroup | ✅ Added | `CmdFleetRegroup` — warps all members to boss |
| Formation flight | ❌ Not implemented | `FORMATION` mode (12) exists but unused |

### Login Warp

| Feature | Status | Notes |
|---------|--------|-------|
| Pending destiny update queue | ✅ Added | Defers updates until SetState sent — fixes `No ballpark` |
| 3s delay before login warp | ✅ Added | Client ballpark initializes before warp command |
| Station proximity detection | ✅ Added | Offsets target outside station sphere to prevent collision kick |
| Safe logoff detection | ✅ Added | Stationary disconnect = immediate removal, no 60s warp |

---

## Drones

### Combat Drones

| Feature | Status | Notes |
|---------|--------|-------|
| Launch / Scoop | ✅ Working | Full lifecycle |
| Engage / Return / Orbit | ✅ Working | AI state machine |
| Fighter ammo (10/5 shots) | ✅ Working | Auto-reload on carrier |
| Fighter bomber AoE | ✅ Working | Bomb attack, return when empty |
| Drone bandwidth / control | ✅ Working | Ship bandwidth check |
| Control range | ✅ Working | `AttrDroneControlDistance` + skill bonuses |
| Incapacitated (out of range) | ✅ Working | Disables drone, re-enables on return |

### Damage & Skills

| Feature | Status | Notes |
|---------|--------|-------|
| Drones (+5%/lvl) | ✅ Working | All combat drones |
| Drone Interfacing (+10%/lvl) | ✅ Working | All combat drones |
| Heavy Drone Operation (+5%/lvl) | ✅ Working | Non-sentry combat drones |
| Sentry Drone Interfacing (+2%/lvl) | ✅ Added | Sentry drones only |
| Racial specialization (+2%/lvl) | ✅ Working | Caldari/Minmatar/Amarr/Gallente |
| Advanced Drone Interfacing (+2%/lvl) | ✅ Added | All drones (CombatAttack, Fighter, FighterBomber) |
| Combat Drone Operation (+2% tracking/lvl) | ✅ Working | `GetDroneToHit()` |
| Drone Sharpshooting (+10% optimal/falloff/lvl) | ✅ Working | `GetDroneToHit()` |
| Drone Navigation (+5% speed/lvl) | ✅ Working | `SetEngaged()` / `SetApproaching()` |
| Drone Durability (+5% HP/lvl) | ✅ Working | `DroneSE::Online()` |
| Scout Drone Operation (+5km range/lvl) | ✅ Working | `GetControlRange()` |
| Electronic Warfare Drone Interfacing (+3km range/lvl) | ✅ Working | `GetControlRange()` |
| Drone Damage Amplifier modules | ✅ Added | `PassiveModule` + `AttrDroneDamageBonus` from ship |
| Drone Navigation Computer | ✅ Added | `AttrDroneMaxVelocityBonus` from ship |
| Drone Control Range Module | ✅ Added | `AttrDroneRangeBonus` from ship |
| Drone Tracking Computer | ✅ Added | `AttrTrackingSpeedBonus`, `AttrFalloffBonus` from ship in `GetDroneToHit()` |

### EWAR Drones

| Feature | Status | Notes |
|---------|--------|-------|
| Web drones | ✅ Working | `WebAttack()` via `WebbedMe()` |
| Web: Propulsion Jamming Drone Interfacing (+10%/lvl) | ✅ Added | Web strength bonus |
| Warp scramble drones | ✅ Working | `ScrambleAttack()` sets `AttrWarpScrambleStatus` |
| Scramble: Propulsion Jamming Drone Interfacing (+10%/lvl) | ✅ Added | Scramble strength bonus |
| ECM drones | ✅ Working | `ECMAttack()` breaks locks |
| ECM: chance calc vs sensor strength | ✅ Added | Reads Gravimetric/Radar/Magnetometric/Ladar, rolls chance |
| ECM: Electronic Warfare Drone Interfacing (+10%/lvl) | ✅ Added | ECM strength bonus |
| Cap drain drones | ✅ Working | `CapDrainAttack()` drains + transfers to owner |
| Target Painting drones | ✅ Added | `SubType_TargetPaint`, `PaintAttack()`, sig radius bonus |
| Target Paint: Electronic Warfare Drone Interfacing (+10%/lvl) | ✅ Added | Paint strength bonus |

### Support Drones

| Feature | Status | Notes |
|---------|--------|-------|
| Mining drones | ✅ Working | `MiningAttack()`, ore transfer to ship |
| Mining: skill bonus (+5%/lvl) | ✅ Working | |
| Mining: Mining Drone Operation (+20%/lvl) | ✅ Working | |
| Mining: Mining Drone Specialization (+2%/lvl) | ✅ Added | |
| Logistics (shield/armor) drones | ✅ Working | `LogisticsRepair()` |
| Logistics: Repair Drone Operation (+5%/lvl) | ✅ Working | |
| Salvage drones | ❌ Not in Crucible | Skill exists but no AI/subtype |

### Assist / Guard

| Feature | Status | Notes |
|---------|--------|-------|
| CmdAssist | ✅ Added | Sets `m_assistTargetID` on drone |
| AI Assisting state | ✅ Added | Orbits assisted player, follows their NPC target |
| CmdGuard | ✅ Added | Sets `m_guardTargetID` on drone |
| AI Guarding state | ✅ Added | Orbits guarded player, auto-engages anyone targeting them |
| Delegate Control / Relinquish | ✅ Working | `CmdDelegateControl`, `CmdRelinquishControl` |

### Drone Modules

| Feature | Status | Notes |
|---------|--------|-------|
| Drone Control Unit | ✅ Working | PassiveModule |
| Drone Damage Modules (group 645) | ✅ Added | Now instantiated as PassiveModule |
| Drone Modules (group 586) | ✅ Added | Now instantiated as PassiveModule |
| Drone Navigation Computer (644) | ✅ Working | PassiveModule |
| Drone Tracking Modules (646) | ✅ Working | PassiveModule |
| Drone Control Range Module (647) | ✅ Working | PassiveModule |

---

## Fleet

| Feature | Status | Notes |
|---------|--------|-------|
| Create / Join / Leave fleet | ✅ Working | Full lifecycle |
| Fleet hierarchy (wings/squads) | ✅ Working | |
| Fleet invitations | ✅ Working | Invite/Join requests |
| Fleet roles (boss/wing/squad) | ✅ Working | |
| Fleet advert | ✅ Working | Public fleet listing |
| Fleet broadcast | ✅ Working | Bubble/System/Universe scope |
| Fleet MOTD | ✅ Working | |
| Fleet Regroup | ✅ Added | Warps all fleet members to boss |

### Fleet Warp

| Feature | Status | Notes |
|---------|--------|-------|
| Warp to Member | ✅ Added | `CmdWarpToStuff` type="char" resolves target in system |
| Warp Fleet to Member | ✅ Added | fleet=1 flag warps entire fleet in system |
| Fleet Regroup | ✅ Added | `CmdFleetRegroup` warps all members to boss position |
| FleetWarp broadcast | ✅ Added | Notification sent via FleetBroadcast |

### Fleet Boosts

| Feature | Status | Notes |
|---------|--------|-------|
| BoostData structure | ✅ Working | armored, info, siege, skirmish, mining, leader |
| Fleet booster (FC) boost | ✅ Working | Based on warfare skills |
| Wing booster boost | ✅ Working | Max(fleet, wing) stacking |
| Squad booster boost | ✅ Working | Max(fleet, wing, squad) stacking |
| ShipSE::ApplyBoost (armor, shield, scan, range, inertia) | ✅ Working | 2%/lvl per skill |
| Mining Foreman boost (3%/lvl) | ✅ Working | Applied in MiningLaser.cpp |
| **Armored Warfare Specialist** (+2%/lvl) | ✅ Added | Adds to armored bonus |
| **Information Warfare Specialist** (+2%/lvl) | ✅ Added | Adds to info bonus |
| **Siege Warfare Specialist** (+2%/lvl) | ✅ Added | Adds to siege bonus |
| **Skirmish Warfare Specialist** (+2%/lvl) | ✅ Added | Adds to skirmish bonus |
| **Mining Director** (+10%/lvl) | ✅ Added | Applied in MiningLaser.cpp |
| **Gang Coordinator modules** (warfare links) | ✅ Added | Active module doubles boost effectiveness |
| Warfare Link Specialist | ❌ Not implemented | Module-based link bonuses |
| OnFleetBoost push notification | ❌ Not implemented | Client recalculates on location change (timer-based) |

### Crimewatch & Combat Relogin

| Feature | Status | Notes |
|---------|--------|-------|
| Weapon timer (60s) | ✅ Working | Set on `OnWeaponFired()`, blocks dock/jump |
| Aggression timer (15min) | ✅ Working | Set on `OnAggression()`, resets on each hit |
| CanDock / CanJump enforcement | ✅ Added | `CmdDock` / `CmdStargateJump` check `CrimeWatch` timers |
| Combat logoff — ghost ship 15min | ✅ Added | Ship stays in system after disconnect with aggression |
| Emergency warp-out 60s | ✅ Added | No aggression → 60s wait → warp to random point → disappear |
| Safe logoff (stationary disconnect) | ✅ Added | Speed=0 on disconnect → immediate ship removal |
| Ghost ship cleanup | ✅ Added | `SystemManager::ProcessGhostShips()` removes expired ships |
| CmdSafeLogoff | ✅ Added | BeyonceService handler for client safe logoff request |
| Warp scramble blocks emergency warp | ❌ Not connected to logoff | `AttrWarpScrambleStatus` checked in `CmdWarpToStuff` only |

---

## NPC & Combat

| Feature | Status | Notes |
|---------|--------|-------|
| NPC AI (basic) | ✅ Working | Agression, orbit, attack |
| NPC rat fleets | ✅ Working | Group warp, engagement |
| NPC repair visual | ✅ Working | Shield/armor repair beams |
| Sentry guns | ✅ Working | |
| CONCORD | ✅ Working | |
| Bounty / sec status | ✅ Working | |
| NPC spawn / respawn | ✅ Working | Anomaly and belt spawners |

### Drones (NPC)

| Feature | Status | Notes |
|---------|--------|-------|
| NPC drone AI | ✅ Working | Full state machine |
| NPC drone subtypes | ✅ Working | Combat, EWAR, Logistics, CapDrain |
| NPC drone skills/attr | ✅ Working | Reads NPC attributes |

---

## Modules & Fitting

| Feature | Status | Notes |
|---------|--------|-------|
| Turret modules | ✅ Working | ActiveModule |
| Missile launchers | ✅ Working | All sizes |
| Shield / Armor repairers | ✅ Working | |
| Afterburner / MWD | ✅ Working | |
| Capacitor booster | ✅ Working | |
| Energy vampire / destab | ✅ Working | |
| Warp scrambler / stasis web | ✅ Working | 
| ECM / ECCM | ✅ Working | ActiveModule |
| Sensor booster / tracker computer | ✅ Working | ActiveModule |
| Cynosural field gen | ✅ Working | CynoModule |
| Probe launcher | ✅ Working | ProbeLauncher |
| Rigs | ✅ Working | RigModule |
| Subsystems (T3) | ✅ Working | SubSystemModule |
| Mining lasers | ✅ Working | MiningLaser |
| Gang Coordinator (warfare links) | ✅ Added | Tracks active state, doubles fleet boost |

---

## Caches & Services

| Feature | Status | Notes |
|---------|--------|-------|
| ImageServer | ✅ Working | Built-in HTTP server on port 26001 |
| ImageServer URL resolution | ✅ Added | Auto-resolve IP when configured as localhost |
| ImageServer config (`imageServerURL`) | ✅ Added | Explicit override for remote clients |
| Portrait upload | ✅ Working | Via PhotoUploadService + liveupdate |
| Portrait gender in cacheOwners | ✅ Fixed | LEFT JOIN with chrCharacters |
| ObjCacheService | ✅ Working | Cached object generation |
| Fleet cache | ✅ Working | cacheOwners, fleetID mapping |

---

## Client Protocol

| Feature | Status | Notes |
|---------|--------|-------|
| CryptoServerHandshake | ✅ Working | |
| Session management | ✅ Working | |
| Destiny updates (CmdWarpTo, etc.) | ✅ Working | |
| AddBalls envelope | ✅ Working | packet_type=1 (partial updates) |
| AddBalls2 | ✅ Working | Slim items + extra ball data |
| Fleet packets (OnFleetJoin, etc.) | ✅ Working | |
| FleetWarp notification | ✅ Working | Via FleetBroadcast |

---

## Image Server

| Feature | Status | Notes |
|---------|--------|-------|
| Built-in HTTP image server | ✅ Working | Port 26001 |
| Character portrait serving | ✅ Working | JPEG files from `image_cache/Character/` |
| Alliance / Corp logo serving | ✅ Working | PNG files |
| InventoryType / Render images | ✅ Working | PNG files, falls back to CCP redirect |
| Fallback to CCP image server | ✅ Working | For non-player items |
| Image upload (character creation) | ✅ Working | Via PhotoUploadService |
| Image URL auto-resolution | ✅ Added | Resolves server IP when configured as localhost |
| `imageServerURL` config option | ✅ Added | Explicit hostname override |
| Portrait gender in `cacheOwners` | ✅ Fixed | LEFT JOIN with `chrCharacters` prevents fleet join crash |

---

## Other Systems

| Feature | Status | Notes |
|---------|--------|-------|
| Station docking | ✅ Working | |
| Stargate jumping | ✅ Working | |
| Bookmarks | ✅ Working | |
| Agent missions | ⚠️ Partial | Basic agent service, warp-to-location not implemented |
| Wormholes | ✅ Working | Jump, mass, lifetime, visual states |
| Sovereignty | ✅ Working | NPC corp sov data |
| Market / orders | ✅ Working | Multi-region seeding |
| Planetary interaction | ⚠️ Partial | Basic infrastructure |
| POS (Player Owned Structures) | ✅ Working | Towers, modules, fuel |
| Tutorial (Career Agents) | ⚠️ Partial | Basic service |
| Skill training | ✅ Working | SP accumulation, skill queue |
| Character creation | ✅ Working | Paperdoll, portrait upload |
| Corp management | ✅ Working | |
| Alliance management | ✅ Working | |
| Chat / LSC | ✅ Working | Fleet, corp, alliance, local channels |
| Crimewatch / aggression | ✅ Working | |
| Container / loot | ✅ Working | |
| Autopilot | ✅ Working | Chain jumps, follow |
