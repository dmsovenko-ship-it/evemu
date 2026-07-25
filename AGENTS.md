# EVEmu Session Context

## Current State
All fixes committed and pushed to `master` (commit `f59a77be`). Server building now.

## Git Log
```
f59a77be fix: server-side warp scramble check, Bubble::Add OnSpecialFX14, JumpIn/GateActivity on arrival
d34308cc fix: warp accel formula (divide-by-3 bug), warp cap (kg→Mkg)
b9133a99 fix: mixed PyInt/PyBool SendMail overloads
3bc4d2ad feat: defender missile system (ShipSE::MissileLaunched, interception, Countermeasure_Launcher)
7cb2fb3d fix: add sourceShipID to Missile MakeSlimItem
bc17d8db fix: null check pilots in Bump()
cb82e950 fix: add corpID/allianceID/warFactionID/charID to base SystemEntity MakeSlimItem
4813a739 fix: mail — MarkAsUnreadByList signature, list-based DB, SyncMail range filter
4d326741 fix: mixed SendMail overloads with PyInt*
e2349964 fix: skip SendAddBalls for warping ships, keep velocity on force-warp
d58859ea chore: remove __pycache__, add to gitignore
d3cf152d fix: remove bubble toggle, fix bump formula, fix decel exit (m_radius)
```

## Key Decisions

### Warp Disrupt / MWD Bubble
- **No distance check in destiny.dll** — `OnActivatingWarp` only checks `m_activations[0].size() > 0` via `PyDict_GetItem`, no range check
- **Dynamic bubble toggle removed** — WarpDisruptFieldGenerating always stays visible; `AttrWarpScrambleStatus` handles range-based scramble per-ship
- **Bubble::Add** skips `SendAddBalls` for warping ships (WarpLoop crash prevention)
- `warpScrambleTimer` runs every 1000ms, sets `AttrWarpScrambleStatus` based on actual distance to bubble center

### Warp Physics
Based on decompiled `destiny.dll` (stored at `C:\opencode\projects\other\`):
- **Accel**: `distance = e^(3t)`, `speed = 3e^(3t)` ✓ (already implemented)
- **Decel**: `distance = total - e^(-t)*decelDist`, `speed = warpSpeed * e^(-t)` ✓ (already implemented)
- **Exit condition**: `distance < ball->radius` (changed from hardcoded 100m to `m_radius`)
- **Catch-all/30° warp**: no longer zeroes `m_velocity` — ship enters warp with momentum
- **PyMethodDef table**: only 4 exports — `FindShortestPath`, `Test`, `GetBoxCenter`, `SetConstant`

### Bump Physics
- **Fixed collision formula**: `distance -= (r1 + r2)` instead of `(r1 - r2)` — was computing surface distance incorrectly

### Client Crashes (all confirmed via decompiled `.py` files)
1. **graphicInfo=None** → `effects.SmartBomb`/`MicroWarpDrive` skipped in `ShowEffect`; `SendSpecialEffect` routes to `OnSpecialFX10` when graphicInfo=0
2. **bracket None.lower()** → `"name"` field added to `MakeSlimItem()` for all entity types
3. **AddBalls2 KeyError** → `EncodeDestiny()` for DeployableSE includes `DataSector` for `IsFree`
4. **WarpLoop crash** → `WarpStop()` sends no packets; `Bubble::Add()` skips `SendAddBalls` for warping ships

### Mail System
- `SelfEveMail()` in LSCService — inserts `mailMessage` + `mailStatus`
- `GetMailBody()` returns raw compressed data (client decompresses)
- `MailingListGetInfo/GetSettings` — fixed leading spaces in list names
- `OnMailSent` notifications

### Sovereignty
- `militaryPoints`/`industrialPoints` default changed from 5 to 0 (client expects raw values, no fallback)

### Ghidra Analysis
- JDK 21 + Ghidra 12.1.2 installed at `C:\Users\User\AppData\Local\Temp\ghidra_extract\`
- Decompiled `destiny.dll` output: `C:\Users\User\AppData\Local\Temp\opencode\destiny_decompile.txt` (48KB)
- Method table dump: `C:\Users\User\AppData\Local\Temp\opencode\destiny_methods.txt`
- Key functions: `OnActivatingWarp @ 0x1001b3c0`, `EntityWarpIn @ 0x10020ed0`, `OnDeactivatingWarp @ 0x100209f0`, `OnExitWarp @ 0x10004b00`
- Scripts: `ghidra_script.java`, `ghidra_full.java`, `ghidra_methods.java` in `C:\Users\User\AppData\Local\Temp\opencode\`

### Decompiled Client Python
- 1082 files decrypted from compiled.code → `C:\opencode\projects\other\all\`
- Key files: `michelle.py`, `fxSequencer.py`, `bracketMgr.py`, `godma.py`, `evemail.py`, `sovSvc.py`
- Doc string shows Python 2.4 compatibility, Crucible branch

## Progress

### Done
- **Mail**: SelfEveMail inserts mailMessage+mailStatus; GetMailBody raw compressed; mailing list leading-space fix; all 8 SendMail overloads (PyInt/PyBool/PyString/PyWString); MarkAsUnreadByList signature fix; MarkAsReadByList/TrashByList listID→messageID bug; SyncMail range filter via second param; OnMailSent notification
- **MWD deployables**: bubble toggle removed (always visible); server-side AttrWarpScrambleStatus check in WarpTo(); Bubble::Add sends OnSpecialFX14 with graphicInfo(range); warpScrambleTimer periodic 1000ms range checks; EncodeDestiny DataSector for IsFree
- **Warp physics**: accel formula divide-by-3 fix (was 1/3 dist); capacitor mass unit fix (kg→Mkg, was ×1000); decel exit m_radius instead of hardcoded 100m; catch-all/30° no longer zeroes velocity
- **Defender missiles**: ShipSE::MissileLaunched auto-fires defenders; Missile::HitTarget intercepts missiles; public Destroy() method; Countermeasure_Launcher enabled in ModuleFactory
- **Client crash fixes**: graphicInfo=None→skip SmartBomb/MicroWarpDrive; bracket "name" field in all MakeSlimItem; AddBalls2 DataSector; WarpLoop SendAddBalls skip; Bump null-check pilots; sourceShipID in Missile MakeSlimItem
- **Sovereignty**: militaryPoints/industrialPoints default 5→0

### To Test
- **Warp scramble**: ship inside MWD bubble gets "Warp drive is disrupted."; outside bubble (>range) can warp
- **Gate arrival**: JumpIn + GateActivity effects play when warping to a gate
- **Capacitor**: frigate ~3 GJ / 5 AU; battleship ~300 GJ / 5 AU; Warp Drive Operation skill reduces drain
- **Mail**: Cyrillic; reply/forward flags; labels; mailing lists; blocked-contact filter; rate limit
- **Defender**: active launcher + defender charge intercepts incoming missiles
- **Bump**: correct surface distance (r1+r2+BUMP_DISTANCE); notification messages
- **Brackets**: all entity types display name/type/corp/alliance without AttributeError
- **Sovereignty dashboard**: loads without error; index values display correctly
- **Gate jump**: JumpOut→JumpIn→GateActivity sequence; no SceneManager `NoneType.vx`

### Remaining Issues
- Full `WarpDisruptFieldGenerating` effect classification missing in Crucible
- SceneManager crash (`NoneType.vx`) is a secondary effect — primary warp crash now fixed
- `tabgroup UnicodeDecodeError` (CP1252) — client-side, needs `errors='replace'` in editplaintext.py
