# Decompiled EVE Crucible Client Scripts

## Extraction tools

- `evedec_crucible.py` — decrypts compiled.code (3DES key from blue.dll via exponent-of-one RSA trick) + zlib decompress + marshal
- `extract_starmap.py` — extracts specific targets from compiled.code
- `extract_service.py` — extracts service.py and serviceManager.py
- `extract_pathfinder.py` — extracts pathfinder.py
- `decompile_dbutil.py` — extracts and decompiles dbutil.py
- `dump_cache2.py` — scans bulkdata/*.cache2 for human-readable schema

## Extracted files in C:\EVE_unpacked\extracted\

### Autopilot system
| File | Purpose |
|------|---------|
| `autopilot.py` | Main AutoPilot service — Update() every 2s, warp/approach/jump logic |
| `spaceMgr.py` | WarpDestination, IndicateWarp, StopWarpIndication |
| `michelle.py` | Ballpark management, destiny update processing, OnBallparkCall scatter |
| `states.py` | State flags (threat, targeting, etc.) |

### Session & services framework
| File | Purpose |
|------|---------|
| `sessions.py` | CoreSession, ApplyRemoteAttributeChanges, SessionMgr |
| `eveSessions.py` | EVE Session class, SessionMgr.PerformSessionChange, DependantAttributes |
| `SessionChangeGPCS.py` | Session change notification handler |
| `service.py` | CoreService base class, service lifecycle |
| `serviceManager.py` | StartService, StopService, CreateServiceInstance |
| `machoNet.py` | Networking layer, ProcessSessionChange |

### Navigation & routing
| File | Purpose |
|------|---------|
| `starMapSvc.py` | Starmap service — UpdateRoute, GetWaypoints, SetWaypoints, GetDestinationPath |
| `pathfinder.py` | Pathfinder service — GetWaypointPath, GetPathBetween |
| `eveNavigationService.py` | In-captain-quarters navigation (incarna, not space) |
| `inflight_navigation.py` | Inflight navigation UI |
| `map_navigation.py` | Map navigation UI |

### Effects & space objects
| File | Purpose |
|------|---------|
| `Warp.py` | Warp effect (client-side visual) |
| `Jump.py` | Jump effect |
| `stargate.py` | Stargate space object |
| `warpgate.py` | Warpgate space object |
| `ship.py` | Ship space object |
| `shipui.py` | Ship UI (speed controls, modules) |
| `jumpMonitor.py` | Jump timing/monitoring service |
| `jumpQueue.py` | Jump queue service |
| `eveCfg.py` | EVE configuration, constants |

### Cache/misc
| File | Purpose |
|------|---------|
| `dbutil.py` | DB utility functions (SQLStringify, TuplesToCSVStrings) |

## Complete file inventory (1082 entries)
See `inventory.txt` for full list

## Key findings — Autopilot flow

### Pre-jump
1. AP Update() → finds stargate matching destinationPath[0]
2. CmdWarpToStuffAutopilot() → server WarpTo with autoPilot=true
3. WarpStop() → snap position, Follow() called → CmdFollowBall sent
4. Cloud Folow() tick → speed control → rawDist decreases

### Auto-jump (server-side)
1. rawDist <= followDistance (2500m) → SetSpeedFraction(0)
2. StargateJump() → MoveToLocation() → SendSessionChange()

### Post-jump (the problem area)
1. SessionChangeNotification arrives
2. C++ framework processes session params
3. OnSessionChanged fired:
   - KillTimer(), ignoreTimerCycles=3, StartTimer()
   - UpdateRoute(fakeUpdate=True)
4. UpdateRoute():
   - If waypoints[0] == solarsystemid2 → shift waypoints → **SetOff('waypoint reached')**
   - This disables autopilot!
5. If session.autopilot=1 in change, C++ framework may re-enable it

### Starmap.UpdateRoute() logic (starMapSvc.py:2101)
```python
for each in [session.stationid2, session.solarsystemid2, session.constellationid, session.regionid]:
    if waypoints[0] == each:
        waypoints = waypoints[1:]
        settings.char.ui.Set('autopilot_waypoints', waypoints)
        if settings.user.ui.Get('autopilot_stop_at_each_waypoint', 0) == 0:
            if sm.GetService('autoPilot').GetState():
                sm.GetService('autoPilot').SetOff('  - waypoint reached')
        break
```

### OnBallparkCall triggers (michelle.py:599)
Destiny events that scatter OnBallparkCall:
- Orbit, GotoDirection, WarpTo, SetBallRadius, GotoPoint,
- SetBallInteractive, SetBallFree, SetBallHarmonic, FollowBall

Autopilot.OnBallparkCall disables on: GotoDirection, GotoPoint
(when gotoCount == 0 and final waypoint is not a station)
