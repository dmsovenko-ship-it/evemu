# EVEmu Session Context

## Current State
All fixes committed and pushed to `master` (commit `f59a77be`). Server building now.

## Git Log
```
f59a77be fix: add server-side warp scramble check in WarpTo, fix Bubble::Add OnSpecialFX14 with range, add JumpIn/GateActivity on warp arrival
d34308cc fix: warp accel formula divide-by-3 bug (1/3 dist traveled), fix warp cap using m_massMKg
b9133a99 fix: add mixed PyInt/PyBool SendMail overloads
3bc4d2ad feat: defender/anti-missile system (ShipSE::MissileLaunched, interception, Countermeasure_Launcher)
7cb2fb3d fix: add sourceShipID to Missile MakeSlimItem
bc17d8db fix: null check pilots in Bump() (crash when bumping NPC/structure without pilot)
cb82e950 fix: add corpID/allianceID/charID to base SystemEntity MakeSlimItem
4813a739 fix: mail service — fix MarkAsUnreadByList signature, add list-based DB methods, fix SyncMail range filter
4d326741 fix: add SendMail overloads with PyInt* for isReplyTo/isForwardedFrom
e2349964 fix: skip SendAddBalls for warping ships (prevents WarpLoop crash), keep velocity on force-warp
d58859ea chore: remove __pycache__ from tracking, add to gitignore
d3cf152d fix: remove dynamic bubble toggle, fix bump collision formula, fix warp decel exit (use ship radius)
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

### Known Remaining Issues
- Full `WarpDisruptFieldGenerating` effect classification missing in Crucible
- SceneManager crash (`NoneType.vx`) after warp crash is a secondary effect — primary warp crash now fixed
- `tabgroup UnicodeDecodeError` (CP1252) — client-side, needs `errors='replace'` in editplaintext.py
