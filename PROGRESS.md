# EVEmu Crucible — Fork Progress Status

> Last updated: 2026-07-06
> Based on [upstream EVEmu Project Status](https://wiki.evemu.dev/wiki/Crucible_Project_Status) format
> Fork: [dmsovenko-ship-it/evemu](https://github.com/dmsovenko-ship-it/evemu) of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)
>
> Each system below follows the upstream checklist. **Bold** items mark additions/enhancements made in this fork beyond upstream.

**Estimated Overall Progress**

| Upstream | This Fork |
|----------|-----------|
| 59.5%    | **~75%**  |

---

## 1. System Foundation Fundamentals

**Upstream: 62.5% — This Fork: 70%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Stable, Efficient Server Code | 65% | 70% | Eliminated duplicated code, tracking/fixing segfaults, C++0x/11 |
| Memory Management | 15% | 20% | Improved over upstream but still needs work |
| Efficient, Accurate Packet Deciphering | 80% | 85% | AddBalls2, Fleet packets, OnMultiEvent all understood |
| Mutexes | 100% | 100% | All MT systems correctly lock/unlock |
| Eliminate all compiler warnings | 50% | 55% | Incremental cleanup |
| **GM commands** | **65%** | **75%** | `/online me`, `/spawn`, faction-sec, warp-to-player, `/giveallskills` |

---

## 2. Item Information Windows

**Upstream: 90% — This Fork: 90%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Get Item Info | 90% | 90% | |
| Get Ship Info | 90% | 90% | |
| Get Skill Info | 90% | 90% | |
| Get Certification Info | 90% | 90% | |
| Get Character Info | 90% | 90% | |
| Get NPC Corporation Info | 90% | 90% | |
| Get Player Corporation Info | 90% | 90% | |

---

## 3. Account and Character Management

**Upstream: 96.3% — This Fork: 97%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Account Login | 100% | 100% | CRAM-MC auth |
| Account Banning/Kicking | 60% | 60% | |
| Character Creation | 100% | 100% | Paperdoll, portrait upload |
| Character Deletion | 70% | 70% | |
| Character entrance to last location | 100% | 100% | |
| Character Window | 100% | 100% | |
| Open Skills Window | 100% | 100% | |
| Open Certificate Window and Planner | 100% | 100% | |
| Open Decorations Window | 100% | 100% | |
| Open Attributes Window | 100% | 100% | |
| Neural Remap | 100% | 100% | |
| Open Augmentations Window | 100% | 100% | **Implants now work** (ProcessEffects) |
| Open Jump Clones Window | 100% | 100% | Jump clones not implemented |
| Open Bio Window | 100% | 100% | |
| Open Employment History Window | 100% | 100% | |
| Open Standings Window | 100% | 100% | |
| Open Security Status Window | 100% | 100% | SecStatus saved to DB |
| Open Kill Rights Window | 100% | 100% | **Kill Rights implemented** |
| Open Combat Log Window | 100% | 100% | |
| KillMail | 10% | **95%** | XML blob, push notification, corpus |
| **Image server** | **—** | **✅** | Built-in HTTP server, portraits, cacheOwners gender fix |
| **Portrait upload** | **—** | **✅** | Via PhotoUploadService + liveupdate |

---

## 4. Skills & Certificates

**Upstream: 98.8% — This Fork: 99%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Browse Skills to train | 100% | 100% | |
| Add skills to queue | 100% | 100% | |
| Filter skills that don't fit in queue | 100% | 100% | |
| Apply changes to skill queue | 100% | 100% | |
| Pause skill queue | 100% | 100% | |
| Re-Start skill queue after pause | 90% | 95% | |
| Skill training time | 100% | 100% | |
| Right-click add skill to queue | 100% | 100% | |
| Certificate Awarding | 100% | 100% | |
| **Implant processing** | **0%** | **✅** | Character::ProcessEffects iterates flagImplant |
| **Booster processing** | **0%** | **✅** | Character::ProcessEffects iterates flagBooster |

---

## 5. Standings

**Upstream: 18.8% — This Fork: 40%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Faction Standings (NPC Alliances) | 100% | 100% | |
| CONCORD Standings (Security Rating) | 10% | **80%** | Changes saved to DB, -0.2 penalty on kills |
| Agent to Character (missions) | 15% | **80%** | Mission completions adjust standings |
| Agent to Player Corp (missions) | 10% | **40%** | |
| Agents respond to Character standings | 15% | **50%** | |
| NPC Corp to Character (missions/faction kills) | 15% | **60%** | |
| NPC Corp to Player Corp | 15% | **40%** | |
| Character to Character (PnP) | 15% | **30%** | |
| Character to Player Corp | 10% | **20%** | |
| Player Corp to Character | 10% | **20%** | |
| Player Corp to Player Corp | 10% | **20%** | |
| Alliance to Player Corp | 10% | **20%** | |
| Alliance to Alliance | 10% | **20%** | |

---

## 6. NPC Station Services

**Upstream: 82.7% — This Fork: 85%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Right-click Menu in Station | 100% | 100% | |
| In-station Guests List | 100% | 100% | |
| Can repackage ships/items | 100% | 100% | |
| Can assemble ships and items | 100% | 100% | |
| Can eject from ship to pod | 100% | 100% | |
| Can make active ships | 100% | 100% | |
| Agent Services | 15% | **80%** | Full mission offer/accept/complete flow |
| Trade Services | 70% | 75% | |
| Repair Services | 90% | 90% | |
| Insurance Services | 100% | 100% | |
| Bounty Services | 100% | 100% | |
| Market Services | 70% | **80%** | Multi-region seeding |
| Medical Services | 80% | 85% | |
| LP Services | 10% | **50%** | LP Store data added |
| Clone upgrade | 100% | 100% | |
| Clone transfer | 10% | 10% | |
| Jump clone installation | 10% | 10% | |
| Items window | 100% | 100% | |
| Can merge/stack/split items | 100% | 100% | |
| Can trash items | 100% | 100% | |
| Can open/close containers | 100% | 100% | |
| Can move items into/out of containers | 100% | 100% | |
| Can inject skills from items | 100% | 100% | |

---

## 7. Agents

**Upstream: 63.1% — This Fork: 85%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Agents Tab | 80% | 90% | |
| Can initiate conversation with agent | 80% | **95%** | Includes Epic Arc Start button (ID=100) |
| Agents offer Missions | 80% | **95%** | Courier/Mining/Encounter/Storyline/EpicArc/COSMOS/Career |
| Can View Mission Offer | 80% | 95% | |
| Can Accept Mission Offer | 80% | 95% | |
| Can Complete Mission Offer | 80% | 95% | Reward ISK + standing + LP |
| Can Search Agents | 15% | **40%** | |
| Can Search for character by agent | 10% | **20%** | |
| **Research Agents** | **0%** | **✅** | 21 agents with tech fields, research levels/quality |
| **Career Agents** | **0%** | **✅** | 29 career agents, 4 factions, sequential chains |
| **COSMOS Agents** | **0%** | **✅** | 42 agents across all factions + pirates |

---

## 8. Missions

**Upstream: 13.5% — This Fork: 70%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Missions Give Rewards (ISK/standings) | 95% | 95% | |
| Missions Give LP | 95% | 95% | |

### Basic Mission Types

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Courier Missions | 60% | **85%** | Cargo delivery between stations |
| Mining Missions | 15% | **70%** | Mine specific ore at location, or supply agent |
| Security Missions (Encounter) | 0% | **80%** | Destroy ships at location, NPC spawning via EncounterSpawnServer |
| Trade Missions | 0% | 0% | Not implemented |

### Special Mission Types

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Tutorial Missions | 0% | **70%** | Aura tutorial, station navigation steps, tutorial goodies |
| Storyline Missions | 0% | **80%** | 144 missions, derived standings, faction selection |
| Research Missions | 0% | 0% | Research agent data exists, mission flow not coded |
| Data Missions | 0% | 0% | Pirate tag turn-in not implemented |
| COSMOS Missions | 0% | **75%** | Caldari/Amarr/Gallente arcs, pirate agents, artifact missions |
| Anomic Missions | 0% | 0% | Not implemented |
| Arc Missions (Epic Arcs) | 0% | **70%** | Blood-Stained Stars (58 missions, 7 chapters), branching choices, 90-day cooldown |
| Unsorted Missions | 10% | 10% | |

---

## 9. LSC — Large Scale Chat System

**Upstream: 68.1% — This Fork: 70%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Open/View Static Server Channels | 100% | 100% | |
| Join Static Server Channels | 100% | 100% | |
| rClick Chat Menu | 100% | 100% | |
| Characters show up in Local | 90% | 90% | |
| Chat in Local | 100% | 100% | |
| Characters show up in Corp | 90% | 90% | |
| Chat in Corp | 90% | 90% | |
| Initiate Private Conversation Chat | 5% | 5% | |
| Create Private Chat Channels | 5% | 5% | |
| Joining Created Private Chat Channels | 5% | 5% | |
| Chat in Private Chat Channels | 5% | 5% | |
| Load Subscribed Chat Channels Upon Login | 100% | 100% | |
| Configure Private Chat Channels | 5% | 5% | |
| Leave Chat Channels | 100% | 100% | |
| Unsubscribe from Private Chat Channel | 5% | 5% | |
| Character and Corporation Lookup (via Search) | 100% | 100% | |

---

## 10. EvE Mail

**Upstream: 40% — This Fork: 45%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| System-Generated mail | 0% | **80%** | Killmail push, corp notifications |

### Characters

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Can Send Eve-mails | 80% | 85% | Still buggy |
| Can Receive Eve-mails | 80% | 85% | |
| Can View Eve-mails | 80% | 85% | |
| Can Reply-to Eve-mails | 80% | 85% | |
| Can Send Eve-mails to Groups (like Corp) | 0% | 0% | |
| Can Create new Private Mailing Lists | 0% | 0% | |
| Private Mailing Lists Saved/Restored | 0% | 0% | |
| Send/Receive in new Private Mailing Lists | 0% | 0% | |

### Corporation Mail

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| All corp mail features | 0% | 0% | |

---

## 11. Calendar

**Upstream: 64.3% — This Fork: 65%**

No significant changes in fork.

---

## 12. Market Details

**Upstream: 48.8% — This Fork: 55%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Price History | 70% | 75% | |

### Characters

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| View Items on Market | 100% | 100% | |
| View Item Details | 100% | 100% | |
| Buy Items (auto-pick seller) | 100% | 100% | |
| Buy Items in Specific Location | 80% | 85% | |
| Placing Buy Orders | 85% | 85% | |
| Cancelling Buy Orders | 70% | 75% | |
| Modify Buy Orders | 70% | 75% | |
| Placing Sell Orders | 85% | 85% | |
| Cancelling Sell Orders | 70% | 75% | |
| Modify Sell Orders | 70% | 75% | |

### Corporation Market — unchanged from upstream (all ~10%)

---

## 13. Assets Window

**Upstream: 85% — This Fork: 85%**

No significant changes in fork.

---

## 14. Wallet

**Upstream: 97.5% — This Fork: 98%**

No significant changes in fork.

---

## 15. Contracts

**Upstream: 16.6% — This Fork: 16.6%**

No changes in fork.

---

## 16. Map System

**Upstream: 88.7% — This Fork: 90%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Can Get Active Cyno Fields | 100% | **100%** | Cyno Generator **implemented** |
| **Wormhole map data** | **—** | **✅** | WH connections displayed |

---

## 17. Bookmark System

**Upstream: 94.4% — This Fork: 95%**

No significant changes in fork.

---

## 18. Effects System

**Upstream: 83.1% — This Fork: 88%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Parsing Effects | 95% | 95% | |
| Process Self Effects | 95% | 95% | |
| Process Skill Effects | 70% | **90%** | Implants and Boosters processed |
| Process Ship Effects (T3) | 0% | 0% | |
| Process Group Effects | 95% | 95% | |
| Apply Processed Effects to Self | 95% | 95% | |
| Apply Processed Effects to Character | 95% | 95% | |
| Apply Processed Effects to Ship | 95% | 95% | |
| Apply Processed Effects to Target | 85% | 90% | |
| Apply Processed Effects to Other | 95% | 95% | |
| Apply Processed Effects to Charge | 95% | 95% | |

---

## 19. Ship Management

**Upstream: 79.7% — This Fork: 82%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| All upstream items | 79.7% | ~82% | |
| **Cloak systems** | **—** | **✅** | Cloak activation/deactivation, visual states, stay-active, under-jump |
| **Heat systems** | **70%** | **75%** | |

### Capital Ship Management

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Can store/retrieve items from Corporate Hangar | 80% | 80% | |
| **Jump drives** | **0%** | **✅** | Fuel calc, JDC range, CynoJump, BridgeJump |

---

## 20. Ship Fittings Manager

**Upstream: 10% — This Fork: 10%**

No changes in fork.

---

## 21. Ship Navigation (Destiny)

**Upstream: 90.4% — This Fork: 95%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Stargate Jump | 100% | 100% | 5km offset spawn, 12s align timeout |
| Orbit Object | 90% | **95%** | Smooth transition |
| Follow Object | 100% | 100% | Autopilot chain preserved |
| Approach Object | 100% | 100% | |
| Keep Object at Distance | 100% | 100% | |
| Aligning to Object | 100% | 100% | |
| Warp to object on-grid | 100% | 100% | |
| Warp to object off-grid | 100% | 100% | No desync (position before GOTO), NaN halt |
| Dock to Station | 100% | 100% | No oscillation/wiggle |
| Undock from Station (with velocity) | 100% | 100% | |
| Can travel routes using AutoPilot | 40% | **95%** | AP state preserved across jumps, follow resumes |
| Warp-in from random location at login | 100% | **100%** | **Login warp with 3s delay, station proximity detection** |
| Warp-out to random location at logoff | 70% | **90%** | Combat logoff (15min ghost), emergency warp (60s), safe logoff (immediate) |
| **Warp-to-0** | **—** | **✅** | ~1000m landing offset |
| **Fleet warp (WarpToMember)** | **—** | **✅** | `CmdWarpToStuff` type="char" |
| **Fleet warp (WarpFleetToMember)** | **—** | **✅** | fleet=1 flag, warps whole system |
| **Fleet Regroup** | **—** | **✅** | Warps all members to boss |
| **Pending destiny update queue** | **—** | **✅** | Defers updates until SetStateSent |

---

## 22. Combat

**Upstream: 99.2% — This Fork: 99.5%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Can Lock Target | 100% | 100% | |
| Can Activate Modules | 100% | 100% | |
| Modules affect target | 100% | 100% | |
| Can Orbit target | 90% | **95%** | |
| Can Follow target | 100% | 100% | |
| Can Keep target at distance | 100% | 100% | |
| Pilot moved to pod when ship destroyed | 100% | 100% | |
| Pilot moved to clone when pod destroyed | 100% | 100% | |
| **Crimewatch / aggression timers** | **—** | **✅** | Weapon (60s), aggression (15min), criminal (15min highsec) |
| **CanDock/CanJump with timer** | **—** | **✅** | Blocks dock/jump when timers active |
| **Combat logoff** | **—** | **✅** | 15min ghost ship, emergency warp 60s, safe logoff |
| **Warp scramble blocks logoff** | **—** | **✅** | Scrambled → cannot safe logoff, ghost ship does not emergency warp |

---

## 23. Module Management

**Upstream: 70.7% — This Fork: 80%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Can drop modules on ship (auto-insert) | 100% | 100% | |
| Can drop modules on specific slots | 100% | 100% | |
| Can Online/Offline modules | 80% | **95%** | Online sends OnMultiEvent broadcast |
| Can remove modules from Fitting | 80% | 85% | |
| Can add/remove charges | 90% | 95% | |
| Can move modules between slots | 90% | 90% | |
| Can Activate modules (in space) | 80% | **95%** | |
| Activated modules produce desired effect | 90% | **95%** | All module groups working |
| Can De-activate modules | 80% | **95%** | |
| De-activated modules remove effect | 90% | 95% | |
| Can Overload modules | 15% | 20% | |
| Can De-activate Overloaded modules | 15% | 15% | |
| Overloaded modules damage bank | 20% | 20% | |
| Can repair damaged modules | 10% | 10% | |
| **Cyno Module** | **—** | **✅** | Fleet check, POS shield, jammer, sec check, high-sec block |
| **Covert Cyno Module** | **—** | **✅** | No fleet req, high-sec allowed, cloak-compatible |
| **Jump Portal Module (titan bridge)** | **—** | **✅** | Portal effect, fleet notification, bridge-to-member |
| **Covert Jump Portal (Black Ops bridge)** | **—** | **✅** | OpenBridge() on demand |
| **Cloak activation/deactivation** | **—** | **✅** | Full cycle via ModuleManager |
| **Gang Coordinator modules (warfare links)** | **—** | **✅** | Doubles fleet boost effectiveness |

---

## 24. Ship Module Groups

**Upstream: 87.3% — This Fork: 92%**

| Module Group | Upstream | Fork | Notes |
|---|---|---|---|
| Engineering | 100% | 100% | |
| Electronic | 100% | 100% | |
| Weapons — Turrets | 100% | 100% | |
| Weapons — Missiles | 100% | 100% | |
| Weapons — Other | 20% | **60%** | Cyno, cloak, jump portal |
| Shields | 100% | 100% | |
| Armor | 100% | 100% | |
| Hull | 100% | 100% | |
| EWAR | 100% | 100% | |
| Propulsion | 95% | 95% | |
| Mining | 100% | 100% | |
| Gang Assist | 20% | **80%** | Warfare links, fleet boosters, specialist skills |
| Rigs | 100% | 100% | |
| **Drone Modules** | **—** | **✅** | DDA, Drone Nav Computer, Drone Tracking, Drone Range (groups 586, 644-647) |
| **Probe Launcher** | **—** | **✅** | Scan Probe Launcher |

---

## 25. Drones

**Upstream: 18.1% — This Fork: 90%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Launch Drones | 65% | **100%** | Full lifecycle |
| Scoop to Cargo | 40% | **100%** | |
| Scoop to Drone Bay | 40% | **100%** | |

### Drone AI

| State | Upstream | Fork | Notes |
|---|---|---|---|
| Passive/Aggressive toggle | 15% | **100%** | Attack on command vs auto-engage |
| Attack and Follow (fighters) | 0% | **100%** | Chase in warp, ammo (10/5 shots), auto-reload |
| Assist | 10% | **100%** | `CmdAssist` — follows player, attacks their NPC target |
| Guard | 0% | **100%** | `CmdGuard` — orbits player, engages anyone targeting them |
| Mine | 10% | **100%** | `MiningAttack()`, ore transfer to ship |
| Focus Fire | 0% | **100%** | All drones engage same target |
| **Approach state** | **—** | **✅** | Cruise speed approach to target |
| **Idle → orbit** | **—** | **✅** | Launch → 500m orbit around owner |
| **Return and orbit** | **15%** | **100%** | Fly back → orbit at 500m |
| **Return to Drone Bay** | **15%** | **100%** | Fly all the way into bay |

### Drone Commands

| Command | Upstream | Fork | Notes |
|---|---|---|---|
| Attack | 15% | **100%** | Engage target, chase, orbit at optimal |
| Assist | 10% | **100%** | Target of assisted player |
| Guard | 0% | **100%** | Anyone targeting guarded player |
| Mine | 0% | **100%** | Mining drones mine asteroid |
| Mine Repeatedly | 0% | **100%** | Mine until depleted |
| Abandon | 80% | **100%** | Relinquish control, goes neutral |
| Return and orbit | 15% | **100%** | |
| Return To Drone Bay | 15% | **100%** | |
| Reconnect to Lost Drones | 0% | **100%** | Reconnect after logout |

### Drone Skills

| Skill | Upstream | Fork | Notes |
|---|---|---|---|
| Drones (+5%/lvl) | — | ✅ | |
| Drone Interfacing (+10%/lvl) | — | ✅ | |
| Heavy Drone Operation (+5%/lvl) | — | ✅ | |
| Sentry Drone Interfacing (+2%/lvl) | — | ✅ | **Added** |
| Racial specialization (+2%/lvl) | — | ✅ | |
| Advanced Drone Interfacing (+2%/lvl) | — | ✅ | **Added** |
| Combat Drone Operation (+2% tracking/lvl) | — | ✅ | |
| Drone Sharpshooting (+10% optimal/falloff/lvl) | — | ✅ | |
| Drone Navigation (+5% speed/lvl) | — | ✅ | |
| Drone Durability (+5% HP/lvl) | — | ✅ | |
| Scout Drone Operation (+5km range/lvl) | — | ✅ | |
| Electronic Warfare Drone Interfacing (+3km range, +10% ECM/paint) | — | ✅ | **Added** |
| Propulsion Jamming Drone Interfacing (+10% web/scramble) | — | ✅ | **Added** |
| Mining Drone Operation (+20%/lvl) | — | ✅ | |
| Mining Drone Specialization (+2%/lvl) | — | ✅ | **Added** |

### Drone Subtypes

| Subtype | Upstream | Fork | Notes |
|---|---|---|---|
| Combat | — | ✅ | Full damage cycle |
| Sentry | — | ✅ | Stationary orbit, SentryDroneInterfacing |
| EWAR — ECM | — | ✅ | Chance-based vs sensor strength |
| EWAR — Web | — | ✅ | Speed reduction |
| EWAR — Warp Scramble | — | ✅ | AttrWarpScrambleStatus |
| Target Painting | — | ✅ | **Added** — sig radius bonus |
| Logistics (shield/armor) | — | ✅ | Repair cycle |
| Cap Drain | — | ✅ | Energy transfer to owner |
| Mining | — | ✅ | Ore transfer |
| Fighter | — | ✅ | Ammo, auto-reload |
| Fighter Bomber | — | ✅ | AoE bomb, auto-reload |

### Drone Modules

| Module | Upstream | Fork | Notes |
|---|---|---|---|
| Drone Control Unit | — | ✅ | Bandwidth bonus |
| Drone Damage Amplifier (645) | — | ✅ | **Added** as PassiveModule |
| Drone Nav Computer (644) | — | ✅ | Velocity bonus |
| Drone Tracking Modules (646) | — | ✅ | Tracking/falloff bonus |
| Drone Control Range Module (647) | — | ✅ | Range bonus |
| Drone Modules (586) | — | ✅ | **Added** as PassiveModule |

---

## 26. NPC AI, Combat & Spawning

**Upstream: 65.6% — This Fork: 80%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| NPC look for targets | 100% | 100% | |
| NPC engage when targeted | 100% | 100% | |
| NPC approach found targets | 100% | 100% | |
| NPC lock/engage/orbit targets | 100% | 100% | |
| NPC notice new targets, engage weakest | 40% | **80%** | AI state machine, aggro logic |
| NPC special actions/call for help/retreat | 10% | **30%** | Basic special actions |
| Rats find/harass Characters in space | 0% | **60%** | Belt/ gate rats aggro on sight |
| Rat Wreck Creation | 95% | 95% | |
| Rat Loot | 95% | 95% | |
| **NPC repair visual** | **—** | **✅** | Shield/armor repair beams |
| **Sentry guns** | **—** | **✅** | Corp inheritance, null-sec standing check |
| **CONCORD** | **—** | **✅** | Police ×25 HP, delay by sec, −0.2 penalty |
| **Customs police NPC** | **—** | **✅** | Faction-specific, contraband scan enforcement |
| **NPC drone AI** | **—** | **✅** | Full state machine, combat/EWAR/logistics/cap drain |

---

## 27. Science & Industry

**Upstream: 38.1% — This Fork: 40%**

### Players

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Research | 95% | 95% | |
| Invention | 10% | 10% | |
| Reverse Engineering | 10% | 10% | |
| Ore Mining | 95% | 95% | |
| Gas Mining | 30% | 30% | |
| Refining and Reprocessing | 100% | 100% | |
| Manufacturing | 95% | 95% | |
| R&D using Agents | 0% | **10%** | Research agent data added |

### Corporations — unchanged from upstream

---

## 28. Scanning & Probing

**Upstream: 99.4% — This Fork: 99.5%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| System Scanning using Ship Sensors | 100% | 100% | |
| Directional Scanning | 15% | 15% | |
| Launch Probes from ship | 100% | 100% | |
| Move Probes in Space | 100% | 100% | |
| Change Probe Range | 100% | 100% | |
| System Scanning with Probes | 95% | 95% | |
| Bookmark Scanned Result | 100% | 100% | |
| Warp to Scanned Result | 100% | 100% | |

---

## 29. Fleet System

**Upstream: 94% — This Fork: 98%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Create Fleet | 100% | 100% | |
| Fleet management window | 100% | 100% | |
| Set Fleet MOTD | 95% | 95% | |
| Fleet Warp | 15% | **98%** | WarpToMember, WarpFleetToMember, FleetRegroup, FleetWarp broadcast |
| View Fleet Composition | 100% | 100% | |
| Create/Delete Wings and Squads | 100% | 100% | |
| Name Wings and Squads | 100% | 100% | |
| Invite to Fleet | 100% | 100% | |
| Accept Fleet Invite | 100% | 100% | |
| Reject Fleet Invite | 100% | 100% | |
| Leave Fleet | 85% | 90% | Can rejoin after leave |
| Kick Member | 85% | 90% | |
| Move fleet members | 100% | 100% | |
| Set commanders | 100% | 100% | |
| Set boosters | 100% | 100% | |
| Commanders provide bonuses to members | 95% | 95% | |
| Boosters provide bonuses to members | 95% | **98%** | **Specialist skills, Mining Director, Gang Coordinator modules** |
| Broadcast to fleet | 100% | 100% | |
| Broadcast to system | 100% | 100% | |
| Broadcast to bubble | 100% | 100% | |
| Add Member to watch list | 95% | 95% | |
| Remove Member from watch list | 95% | 95% | |
| Fleet chat window | 100% | 100% | |
| Advertise Fleet | 100% | 100% | |
| View Available Fleets | 100% | 100% | |
| Apply to Advertised Fleet | 100% | 100% | |
| View Fleet Applications | 95% | 95% | |

### Fleet Boosts (added in fork)

| Feature | Status | Notes |
|---|---|---|
| BoostData structure | ✅ | armored, info, siege, skirmish, mining, leader |
| Fleet/Wing/Squad booster boost | ✅ | Max stacking |
| ShipSE::ApplyBoost | ✅ | 2%/lvl per skill |
| Armored Warfare Specialist | ✅ | +2%/lvl to armored |
| Information Warfare Specialist | ✅ | +2%/lvl to info |
| Siege Warfare Specialist | ✅ | +2%/lvl to siege |
| Skirmish Warfare Specialist | ✅ | +2%/lvl to skirmish |
| Mining Director | ✅ | +10%/lvl to mining yield |
| Mining Foreman | ✅ | +3%/lvl from skill |
| Gang Coordinator modules | ✅ | Doubles boost effectiveness |

---

## 30. Planetary Interaction System

**Upstream: 71.6% — This Fork: 72%**

No significant changes in fork.

---

## 31. Corporation Management

**Upstream: 67.7% — This Fork: 70%**

No significant changes in fork.

---

## 32. Alliance Management

**Upstream: 11% — This Fork: 30%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Creating Alliance | 95% | 95% | |
| Join Alliance | 95% | 95% | |
| Alliance Bulletins | 10% | **40%** | Create/Read/Delete via AllianceBound, Edit/Delete fixed in AllianceDB, label CRUD |
| Alliance History (Corp Employment) | 15% | 20% | |
| Alliance Wars | 10% | 10% | |
| **Alliance bridge jump (CmdJumpThroughAlliance)** | **—** | **✅** | Portal lookup, alliance validation, fuel calc |

---

## 33. POS — Player Owned Structures

**Upstream: 61.4% — This Fork: 65%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| **POS cyno jammer check** | **—** | **✅** | Verifies jammer exists in system via GetSE() |
| **POS bridge/jump** | **—** | **✅** | CmdJumpThroughCorporationStructure, distance-based fuel |

Otherwise no significant changes from upstream.

---

## 34. Cosmic Management System

**Upstream: 54.8% — This Fork: 70%**

| Subsystem | Upstream | Fork | Notes |
|---|---|---|---|
| Anomaly Manager | 20% | **70%** | All site types, FW anomaly visibility, QueueRespawn |
| Dungeon Manager | 40% | **70%** | Anomaly/Mission dungeons, unrated sites |
| Belt Manager | 81% | 85% | |
| Spawn Manager | 86% | **90%** | Dynamic/static spawning, gate guards |
| Scan Manager | 67% | 80% | |
| WormHole Manager | 24% | **80%** | **Full WH lifecycle: creation, tracking, mass, lifetime, visual states, jump** |
| Civilian Manager | 7% | 10% | |

---

## 35. Anomaly Manager

**Upstream: 19.3% — This Fork: 70%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Gravimetric Site Creation | 90% | 100% | |
| Gravimetric Site Destruction | 65% | 95% | |
| Magnetometric Site Creation | 0% | **90%** | |
| Magnetometric Site Destruction | 0% | **90%** | |
| Radar Site Creation | 0% | **90%** | |
| Radar Site Destruction | 0% | **90%** | |
| Ladar Site Creation | 0% | **90%** | |
| Ladar Site Destruction | 0% | **90%** | |
| **Unrated sites** | **—** | **✅** | Combat anomalies visible on scanner |
| **FW mission anomalies** | **—** | **✅** | AddFWAnomaly/RemoveFWAnomaly for militia missions |
| **QueueRespawn** | **—** | **✅** | Re-queue type for respawn after cleanup |

---

## 36. Dungeon Manager

**Upstream: 40% — This Fork: 70%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Anomaly Dungeon Creation | 95% | 95% | |
| Anomaly Dungeon Destruction | 95% | 95% | |
| Mission Dungeon Creation | 95% | 95% | |
| Mission Dungeon Destruction | 95% | 95% | |
| Unrated Dungeon Creation | 95% | 95% | |
| Unrated Dungeon Destruction | 75% | 80% | |
| Unrated Escalation Creation | 10% | 10% | |
| Unrated Escalation Destruction | 10% | 10% | |
| DED Complex Creation | 10% | 10% | |
| DED Complex Destruction | 10% | 10% | |

---

## 37. Belt Manager

**Upstream: 81% — This Fork: 85%**

No significant changes in fork.

---

## 38. Spawn Manager

**Upstream: 65% — This Fork: 75%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| Dynamic spawning | 75% | **85%** | Belt, Mission, Deadspace |
| Static spawning (gate) | 15% | **50%** | Gate rats, customs NPCs |
| Spawn Destruction/Removal | 75% | **85%** | Clean removal |
| Spawns Warp into Belts | 75% | 85% | |
| Spawns Warp out of Belts | 75% | 85% | |
| Spawns guard Gates | 75% | **80%** | |

---

## 39. Scan Manager

**Upstream: 80% — This Fork: 80%**

No significant changes in fork.

---

## 40. Wormhole Manager

**Upstream: 24% — This Fork: 80%**

| Service/Action | Upstream | Fork | Notes |
|---|---|---|---|
| WormHole Creation | 25% | **90%** | Generate entrances, K162 exits |
| WormHole Tracking | 15% | **80%** | Mass tracking, lifetime |
| WormHole Destruction | 10% | **90%** | Collapse on mass/time depletion |
| View WormHole Details | 60% | **90%** | Mass, lifetime, visual states |
| Jumping thru WormHole | 0% | **90%** | Full jump, position sync, oversized ship blocked |

---

## 41. Civilian Manager

**Upstream: 7% — This Fork: 10%**

No significant changes in fork.

---

## Features Added in This Fork (Not in Upstream Checklist)

### Image Server

| Feature | Status |
|---|---|
| Built-in HTTP image server (port 26001) | ✅ |
| Character portrait serving | ✅ |
| Alliance/Corp logo serving | ✅ |
| InventoryType/Render images | ✅ |
| Fallback to CCP image server | ✅ |
| Image URL auto-resolution | ✅ |
| `imageServerURL` config override | ✅ |
| Portrait gender in `cacheOwners` | ✅ Fixed |

### Crimewatch & Combat Relogin

| Feature | Status |
|---|---|
| Weapon timer (60s) | ✅ |
| Aggression timer (15min) | ✅ |
| CanDock / CanJump enforcement | ✅ |
| Combat logoff — ghost ship 15min | ✅ |
| Emergency warp-out 60s | ✅ |
| Safe logoff (stationary → immediate) | ✅ |
| Ghost ship cleanup (ProcessGhostShips) | ✅ |
| CmdSafeLogoff | ✅ |
| Warp scramble blocks emergency warp | ✅ |
| Scramble cleanup on NPC clear target | ✅ |
| Outlaw status (SS ≤ −5.0) — no dock/jump | ✅ |
| Suspect (looting) | ✅ |
| Criminal (15min highsec) | ✅ |
| Kill Rights (grant, auto-activation, Limited Engagement) | ✅ |

### Charge Compatibility

| Feature | Status |
|---|---|
| SDE chargeGroup check (dgmTypeAttributes) | ✅ |
| Fallback IsChargeCompatible | ✅ |
| T2 ammo loading | ✅ |
| Projectile / Hybrid / Energy weapons | ✅ |
| Capacitor boosters | ✅ |

### Warp Speed

| Feature | Status |
|---|---|
| AttrWarpSpeedMultiplier (base) | ✅ |
| AttrWarpSpeedBonus (601) — rigs/implants | ✅ |

### CONCORD

| Feature | Status |
|---|---|
| Police Battleships (×25 HP) | ✅ |
| Delay by sec security level | ✅ |
| −0.2 security penalty on kill | ✅ |

### Contraband / Customs

| Feature | Status |
|---|---|
| ContrabandScan (gate jump) | ✅ |
| Highsec-only scanning | ✅ |
| System-wide broadcast on detection | ✅ |
| 60s penalty timer | ✅ |
| Standing loss on expiry | ✅ |
| Item confiscation (confiscateMinSec) | ✅ |
| ISK fine (basePrice × fineByValue × qty) | ✅ |
| Customs police NPC spawn | ✅ |
| NPC AI engagement of runner | ✅ |
| Faction-specific police NPCs | ✅ |
| Smuggling skill (−10%/lvl detection) | ✅ |

### EPIC Arcs

| Feature | Status |
|---|---|
| EpicArcMgr singleton | ✅ |
| The Blood-Stained Stars (58 missions, 7 chapters) | ✅ |
| Branching (Chapter 4 — Queen vs Blood) | ✅ |
| Branching (Chapter 7 — 4 faction commanders) | ✅ |
| Agent dialog — EpicArcStart button | ✅ |
| Mission chain auto-advance | ✅ |
| Final reward (+0.7 standing, scaled by Social) | ✅ |
| 90-day cooldown | ✅ |
| GetMyEpicJournalDetails | ✅ |

### Missions (additional)

| Feature | Status |
|---|---|
| Storyline missions (144) | ✅ |
| COSMOS (Caldari/Amarr/Gallente/Angel/Blood/Pirate arcs) | ✅ |
| Career agents (29, 4 factions, 4 paths each) | ✅ |
| Distribution missions | ✅ |
| Aura tutorial + Tutorial goodies | ✅ |
| Research agents (21, tech fields) | ✅ |
| Faction warfare data + FW anomalies | ✅ |
| LP Store data | ✅ |

### Tutorial

| Feature | Status |
|---|---|
| Aura tutorial (station navigation steps) | ✅ |
| TutorialLocationService (GiveTutorialGoodies) | ✅ |
| Career agents — all factions | ✅ |

---

*Generated from upstream checklist at https://wiki.evemu.dev/wiki/Crucible_Project_Status (archived 2025-10-02)*
