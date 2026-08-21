# EVEmu TODO

## Current Milestone — Warp & Bubbles

### Crashes (client-side, confirmed from decompiled `.py` files)
- [x] `graphicInfo=None` → fxSequencer `GetBalls()` crash — skip SmartBomb/MicroWarpDrive, route to OnSpecialFX10
- [x] `bracket None.lower()` → add `"name"` field to all `MakeSlimItem()`
- [x] `AddBalls2 KeyError` → add `DataSector` for `IsFree` in DeployableSE EncodeDestiny
- [x] `WarpLoop crash` → skip SendAddBalls for warping ships, keep velocity on force-warp
- [x] `Bump nullptr` → null-check pilots in `Bump()`
- [x] `Missile MakeSlimItem` → add `sourceShipID`
- [ ] `SceneManager NoneType.vx` — secondary to WarpLoop crash (should be fixed now)
- [ ] `tabgroup UnicodeDecodeError` — CP1252 encoding, client-side issue

### Mail
- [x] SelfEveMail — insert mailMessage + mailStatus
- [x] GetMailBody — return raw compressed data (client decompresses)
- [x] MailingListGetInfo/GetSettings — fix leading spaces
- [x] SendMail overloads — all 8 PyInt/PyBool/PyString/PyWString combos
- [x] MarkAsUnreadByList — fix signature (was PyList*, client sends int)
- [x] MarkAsReadByList/MoveToTrashByList — fix listID vs messageID bug
- [x] SyncMail — use range filter (second param)

### Warp Disrupt / MWD Bubble
- [x] Remove dynamic bubble toggle (always visible)
- [x] Server-side `AttrWarpScrambleStatus` check in `WarpTo()`
- [x] Bubble::Add — send OnSpecialFX14 with graphicInfo(range)
- [x] warpScrambleTimer — periodic range checks every 1000ms
- [x] EncodeDestiny — DataSector for IsFree in DeployableSE

### Warp Physics
- [x] Accel formula fix — removed divide-by-3 bug (was only traveling 1/3 dist)
- [x] Capacitor fix — use `m_massMKg` instead of `m_mass` (was 1000x too expensive)
- [x] Decel exit — use `m_radius` instead of hardcoded 100m
- [x] Catch-all/30° warp — don't zero velocity
- [x] Gate arrival — JumpIn + GateActivity effects (implemented, needs testing)

### Combat
- [x] Defender missile system — ShipSE::MissileLaunched, interception, Countermeasure_Launcher
- [x] Bump collision formula — `r1+r2` instead of `r1-r2`
- [x] SmartBomb/MicroWarpDrive crash fix

### Notifications
- [x] OnMailSent — added to LSCService/SendMail
- [x] Standing updates — CreateNotification in StandingMgr
- [x] JumpIn arrival effects — implemented (70c55303 fixed turret fire guids; JumpIn/GateActivity via StargateJump flow)

### Sovereignty
- [x] militaryPoints/industrialPoints default 5→0 (client needs raw values)

## Testing Required
- [ ] Warp out of MWD bubble — should be blocked
- [ ] Warp near gate — JumpIn + GateActivity effects
- [ ] Mail — send to multiple recipients with Cyrillic text
- [ ] Defender missiles — active launcher + defender charge intercepts incoming missiles
- [ ] Bump — collision between moving ships
- [ ] Long warp (5+ AU) — no position jump at accel→decel transition

## Future Work
- Full `WarpDisruptFieldGenerating` effect classification for Crucible
- SceneManager crash hardening (`NoneType.vx`)
- Wormhole jump effects (`JumpInWormhole`)
- Skill-injected attribute recalculation on the fly
- **Large systems not implemented**: FW Plex capture, ship clone bay (AcceptShipInstallation stub), jump drive details, Planetary Interaction (stubs), Memory Mgmt (RefPtr→shared_ptr, ~400 files), PyRep leak fixes

## Known open issues (2026-08-21)
- Right-click on sleeper loot in ship cargo hold at station — client-side `menusvc._InvItemMenu`, needs decompile analysis
- MWD after anchoring doesn't scramble — verify
- FxError fxID=0 Rocket Launcher II — verify `4ee3d397`
- DB Error #1366 notificationText — binary data in utf8
- Bracket crash — `'NoneType' object has no attribute 'lower'` on hover
