# EVEmu Crucible — Progress / Прогресс

> **Our fork · game systems: `███████████████████░` ~97%**
> **Our fork · memory management: `█████████████████░░░` 85%**
> **Our fork · performance & optimization: `██████████████████░░` 88%**
> **Upstream: `████████████░░░░░░░░` ~60%**
> Fork of [EvEmu-Project/evemu_Crucible](https://github.com/EvEmu-Project/evemu_Crucible)

> Totals are the mean of the player-facing game systems (infrastructure tracked separately).
> Итог — среднее по игровым системам (инфраструктура считается отдельно).

---

## Overview / Обзор

| System | % | Bar | Δ up | System | % | Bar | Δ up |
|--------|---|-----|------|--------|---|-----|------|
| Account & Character | 97% | `███████████████████░` | +2% | Skills & Certificates | 99% | `████████████████████` | +9% |
| Ship Navigation | 99% | `████████████████████` | +29% | Combat & Crimewatch | 99% | `████████████████████` | +9% |
| Modules & Overheating | 97% | `███████████████████░` | +12% | Drones | 96% | `███████████████████░` | +21% |
| NPC AI & Spawning | 97% | `███████████████████░` | +37% | Agents & Missions | 97% | `███████████████████░` | +27% |
| **POS** | 99% | `████████████████████` | +29% | Market | 95% | `███████████████████░` | +35% |
| **Incursions** | 96% | `███████████████████░` | +96% | Fleet | 100% | `████████████████████` | +25% |
| **Wormholes** | 92% | `██████████████████░░` | +32% | Scanning | 99% | `████████████████████` | +19% |
| **Notifications** | 97% | `███████████████████░` | +37% | **Standings** | 95% | `███████████████████░` | +35% |
| **Faction Warfare** | 99% | `████████████████████` | +49% | Calendar | 93% | `███████████████████░` | +33% |
| Mail & LSC | 95% | `███████████████████░` | +35% | Contracts | 96% | `███████████████████░` | +36% |
| Corporation | 93% | `███████████████████░` | +28% | **Alliance** | 92% | `██████████████████░░` | +37% |
| **Sovereignty** | 95% | `███████████████████░` | +35% | Science & Industry | 93% | `███████████████████░` | +48% |
| Bookmark System | 95% | `███████████████████░` | +25% | **Effects System** | 97% | `███████████████████░` | +32% |
| **Planetary Interaction** | 95% | `███████████████████░` | +45% | Deployables (MWD/Probes) | 99% | `████████████████████` | +59% |
| **Petitions & Support** | 95% | `███████████████████░` | +95% | Memory Management | 85% | `█████████████████░░░` | +65% |
| **Performance & Optimization** | 88% | `██████████████████░░` | +38% | | | | |

---

## Details / Детали

> ✅ done · 🟡 partial · ❌ not implemented · **bold** = significantly improved vs upstream

### 1. Account / Character `███████████████████░` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Login (CRAM-MC), character creation, attributes | ✅ | ✅ |
| Neural remap, implants, boosters | ✅ | ✅ |
| Jump clones + per-clone implants | 🟡 | ✅ |
| Clone jump, install/destroy clones | 🟡 | ✅ |
| KillMail, image server (portraits/logos) | ✅ | ✅ |
| **Registration email** — required, validated, unique; `SetEmail` for legacy accounts | ❌ | ✅ |

### 2. Skills / Certificates `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Queue, train, certificates, implants | ✅ | ✅ |
| SP loss on T3 pod kill | ❌ | ✅ |

### 3. Ship Navigation `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Stargate jump, orbit, follow, approach | ✅ | ✅ |
| Warp-to-0, fleet warp, login warp | ✅ | ✅ |
| **Autopilot** — CmdWarpToStuffAutopilot, auto-jump, multi-hop | ❌ | ✅ |
| **AP follow** — hysteresis smoothing, gradual accel/decel | ❌ | ✅ |
| **AP post-jump** — jump cloak (60s) + invul (15s) + gate effects | ❌ | ✅ |
| **Early warp start** — begin warp at <30° + half align time | ❌ | ✅ |
| **Snap stop** — instant speed=0 on command (no decel drift) | ❌ | ✅ |
| **Bubble hopping fix** — stay in bubble if in range, 5s cleanup | ❌ | ✅ |
| **Jump cloak** — SetCloakTimer calls Cloak(), enemies can't see | ❌ | ✅ |
| **Warp-to-0 surface** — land at object surface (gates, stations) | ❌ | ✅ |
| **Warp intercept** — MWD bubble pulls ship out of warp via HasWarpBubble | ❌ | ✅ |
| **SendAddBalls after warp** — both WarpStop and intercept resend bubble entities | ❌ | ✅ |
| **JumpIn effect** — broadcast to destination bubble on gate jump | ❌ | ✅ |
| **Collision detection** — push ships out of large static entities (gates, stations) | ❌ | ✅ |
| **Warp capacitor drain** — minimum warpCapacitorNeed=0.00001 | ❌ | ✅ |
| **Missile use-after-free** — targetID check before access | ❌ | ✅ |
| **Warp stop → GOTO coast** — smooth decel on abort instead of instant snap | ❌ | ✅ |
| **Warp scramble check during align** — checked every tick, not just at WarpTo | ❌ | ✅ |
| Orbit desync fix, reduced position sync frequency | ❌ | ✅ |
| Destiny crash fixes — bubble guard, use-after-free | ❌ | ✅ |
| **Warp scramble** blocks logoff/emergency warp | 🟡 | ✅ |
| **Warp scramble cleanup** — on bubble exit (Remove) + per-player range check | ❌ | ✅ |
| **Two-phase warp decel** — linear ramp + exponential decay, formula-based remainder | ❌ | ✅ |
| **Manual gate jump animation** — JumpOut + ~4s + JumpIn, deferred jump mid-warp | ❌ | ✅ |
| **Zero residual velocity on jump** — no post-jump jitter in new system | ❌ | ✅ |
| **NPC balls re-sent after WarpStop** — no invisible attacking NPCs after warp | ❌ | ✅ |
| **No bubble creation mid-warp** — only join existing bubbles (no per-frame GetBubble) | ❌ | ✅ |
| **Orbit from structure surface** — orbit distance measured from gate/station/planet/moon surface, not centre (no inside-the-gate push-out, no speed reset) | ❌ | ✅ |
| **Jump drives** — capital jumps require an active cynosural field (CynosuralFieldI/CovertCynosuralFieldI) in the destination system; fuel type per race (Caldari→Helium, Minmatar→Hydrogen, Amarr→Nitrogen, Gallente→Oxygen) | ❌ | ✅ |
| **End-of-warp landing** — the pilot's own ball is no longer re-delivered at the snapped arrival point (the client keeps its own ball), and the client-side destination is offset to cancel the warp-loop shortfall — no end-of-warp teleport, no 4.4–5 km short-landing at stations/gates | ❌ | ✅ |

### 4. Combat & Crimewatch `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Lock, activate modules, damage, crimewatch | ✅ | ✅ |
| **CONCORD** — ×25 HP, delay by sec, −0.2 penalty, respawn, despawn | 🟡 | ✅ |
| **Security status formula** — `−2.5% × sysSec × (1 + (v−a)/100)` | ❌ | ✅ |
| **Faction police** — secStatus threshold spawn | ❌ | ✅ |
| Sentry guns vs NPC, kill rights | 🟡 | ✅ |
| Combat logoff, outlaw docking | 🟡 | ✅ |
| **Self-defence** — only the first attacker is flagged for aggression; the victim's return fire is legal (PvP and NPC-pilot initiated fights alike) | ❌ | ✅ |
| **Structure aggression** — attacking a player structure in high-sec flags the attacker and CONCORD answers (war-decced owners exempt); defending a structure is never criminal | ❌ | ✅ |

### 5. Modules / Overheating `███████████████████░` 96%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Module groups, cyno, cloak, titan jump bridge | ✅ | ✅ |
| **Overload** — Thermo check, heat damage, Nanite Paste, OverloadRack | ❌ | ✅ |
| **ECM player jam** — ActiveModule ECM compares jam strength to target's strongest sensor; on success breaks the target's lock (ClearTarget) + sends ElectronicAttributeModifyTarget | ❌ | ✅ |
| **Energy warfare** — ship neutralizer drains the target (energyDestabilizationAmount), nosferatu drains target→self, remote capacitor transfer; were reading the wrong attribute (drained 0 / added capacitor) | ❌ | ✅ |
| **Remote sensor damper** — was a stub; now applies maxTargetRange/scanResolution bonuses to the target, symmetric undo | ❌ | ✅ |
| **ECM Burst / Remote ECM Burst** — AoE jam in range (were empty stubs) | ❌ | ✅ |
| **Warp Disrupt Field Generator** — focused radius field (per-ship scramble within range while active) | ❌ | ✅ |
| **Titan Super Weapon** — Crucible doomsday: single-target capital-only strike (sub-caps immune), 2,000,000 racial damage × Doomsday Operation, 50,000 isotope cost, banned in low-sec / POS field, 10-minute mobility cooldown | ❌ | ✅ |
| **Cargo / Ship Scanner** — send OnCargoScanComplete / OnShipScanCompleted (cargo list, cap + fitted modules) | ❌ | ✅ |
| **Misc module groups mapped** — GM/test/faction groups that previously couldn't be fitted at all now load | ❌ | ✅ |
| **Charge handling** — linked weapons clear a depleted charge (no infinite ammo / zombie ref); stale charge entries removed from the manager on burnout; cap booster grants the last charge's capacitor; defender/countermeasure auto-fire consumes its charge; pre-loaded charges restored via the charge-state cache; sensor scripts reset the correct attributes | ❌ | ✅ |
| **Weapon effect GUIDs** — turret/drone/POS-weapon effects use client-valid GUIDs (firing visuals show, not only on NPCs) | ❌ | ✅ |

### 6. Drones `███████████████████░` 96%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Launch/scoop/return, AI states (10 subtypes) | 🟡 | ✅ |
| Skills, control range, bandwidth, damage bonuses | ✅ | ✅ |
| **EWAR cleanup** — web/paint/scramble released on target loss + drone death | ❌ | ✅ |
| **Fighter-bomber always hits** — AoE munitions (no tracking/falloff attrs) toHit=1.0, no false "too far away" misses | ❌ | ✅ |

### 7. NPC AI & Spawning `███████████████████░` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| NPC AI: orbit, target, engage, flee, call-for-help | ✅ | ✅ |
| Belt/gate rats, anomalies, incursions, convoys | ✅ | ✅ |
| **EWAR** — web, ECM, target paint from attributes | ❌ | ✅ |
| **Smartbomb/AoE** — splash in EmpFieldRange | ❌ | ✅ |
| **CONCORD AI** — criminal scan, full state machine | ❌ | ✅ |
| **Sentry AI vs NPC** — aggro on NPCs attacking players in highsec | ❌ | ✅ |
| **NPC module system** — weapon/EWAR modules fitted per SDE attrs, proper cycles, effect GUIDs per type | ❌ | ✅ |
| NPC spawn position fix (no gate-bubble offset) | ❌ | ✅ |
| **Negative EWAR release** — web/scramble/track-disruption cleaned on target loss, untarget, NPC death | ❌ | ✅ |
| **NPC web symmetric undo** — no ×2.5 speed stack on target switch | ❌ | ✅ |
| **Stationary sentry turrets** — sentry/turret groups (Sentry Gun, Protective, Mobile, Destructible Sentry Gun, Mobile Missile Sentry) never move; attack type by role: turret fire / web / energy neutralizer / real missiles (chargeGroup→type) | ❌ | ✅ |
| **Analytic threat assessment** — combat power judged by hull class potential (capitals may cyno a fleet, battleships assumed fitted, fighter screen in space), not a precise fit check | ❌ | ✅ |
| **Self-preservation** — non-combat hulls (industrial/barge/freighter/hauler) never fight back, they warp out; a novice misjudge only causes a panic-flee from a winnable fight, never an attack on a fight judged as lost | ❌ | ✅ |
| **EWAR stickiness fix** — warp scramble / web / paint released when the target leaves the manager, no lingering "Warp drive is disrupted" | ❌ | ✅ |

### 8. Agents & Missions `███████████████████░` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Missions (courier/mining/encounter/storyline), agents | ✅ | ✅ |
| Career agents, COSMOS, research, tutorial | 🟡 | ✅ |
| **Courier fixup** — destinationID, agentDB COALESCE | ❌ | ✅ |
| **Mission dungeon spawn** — on accept | ❌ | ✅ |
| LP store (faction + CONCORD) | 🟡 | ✅ |
| **Encounter full cycle** — accept → dungeon objectives spawn with real faction rat targets (client-lockable) → warp link → clear → mission complete & hand-in | ❌ | ✅ |
| **Standings UI fixed** — owner cache seeded for factions/corps/agents; rows toward client-unknown factions no longer written (Character Sheet no blank window) | ❌ | ✅ |

### 9. Market `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Buy/sell orders, corp market, price history | ✅ | ✅ |
| Trade skills, MarginTrading, escrow, expired auctions | ❌ | ✅ |
| **Order-limit config fields uint8→uint32** — 20000 wrapped to 32, full type list in ask queries | ❌ | ✅ |
| **Offline order execution** — resting buy/sell pairs settle without a live session; escrow, taxes, `mktTransactions` written | ❌ | ✅ |

### 10. Contracts `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Item exchange, courier, auctions with bidding | 🟡 | ✅ |
| Auction item transfer, refund, notifications, auto-finish | ❌ | ✅ |
| **Courier complete** — accepted contract marks status/acceptor, crate delivered to destination, wrap cleaned up | ❌ | ✅ |
| **Item exchange** — creation escrows the offered items (singletons included), rejects an empty contract (no duplicate rows on repeated confirm); search hardened against missing/mistyped arguments | ❌ | ✅ |

### 11. Corporation / Alliance `███████████████████░` 93% / `██████████████████░░` 92%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Corp management, roles, offices, bills, wallets | ✅ | ✅ |
| Alliance creation, wars, voting, dividends | 🟡 | ✅ |
| Corp mail role filtering, war bills recurring | ❌ | ✅ |
| Medals — CreateMedal/GiveMedalToCharacters with cost confirmation | ❌ | ✅ |
| War declarations — RetractWar/ChangeMutualWarFlag on CorpRegistry | ❌ | ✅ |

### 12. Science & Industry `██████████████████░░` 92%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Manufacturing, copying, research (ME/PE) | ✅ | ✅ |
| **Invention** — formula with skills/meta/decryptor, T2 BPC | ❌ | ✅ |
| **Reverse Engineering** — chance calc + T2 BPC | ❌ | ✅ |
| **Remote job install** — with blueprints from remote stations | ❌ | ✅ |
| **Adjusted materials** — correct extra/waste/base, neg ME handling | ❌ | ✅ |
| **POS assembly lines** — auto-create for POS structures | ❌ | ✅ |
| **Cancel job** — returns all materials on abort | ❌ | ✅ |
| **Material-chain production** — recursive build from `invTypeMaterials`, local stock then regional import, output hauled to a hub | ❌ | ✅ |

### 13. POS `███████████████████░` 98%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Towers, modules, force field, fuel | ✅ | ✅ |
| Reinforced mode (fuel → stront → auto-online), CPU/PG | ❌ | ✅ |
| Reactions, weapon AI, skill checks, fuel notifications | ❌ | ✅ |
| **Defence grid** — weapon batteries consume charges (chargeGroup1), role-based web/scram/energy-neutralizer, targeting honours standings / security status / tower war + high-sec gate | ❌ | ✅ |
| **Manual fire control** — AssumeStructureControl (Starbase Defense Management + 15 km + immobilise), AddTargetOBO/RemoveTargetOBO, POS AI prioritises the manual target | ❌ | ✅ |
| **Guards** — orbiting defence ships that assist the manual target | ❌ | ✅ |
| **Moon harvesting & reactions** — harvester → silo → reactor chain driven by the real reaction formulas (`invTypeReactions`), moved along resource links with per-module storage; the process is started from the tower window; a moon's composition is determined once and then fixed | ❌ | ✅ |
| **EWAR batteries** — energy neutralizer (1000 GJ), sensor dampening (lock range + scan resolution), ECM (jam → breaks every lock) | ❌ | ✅ |
| **Ship Maintenance Array** — store/board/scoop/launch assembled ships; refitting from the SMA in space | ❌ | ✅ |
| **Assembly Array manufacturing** — POS array recognised as an industry facility (corp/alliance lines); corporate-hangar capacity for arrays | ❌ | ✅ |
| **Shield hardener recalculation** — tower shield resonances recomputed immediately when a player onlines/offlines a Shield Hardening Array | ❌ | ✅ |
| **Force-field barrier & guard arrival** — warping into a hostile field lands on the shield surface (no inside-the-tower landing); warp-in guards have a guaranteed arrival fallback | ❌ | ✅ |
| **Structure storage access** — POS structure inventories are corp-only (owner / owner-corp); previously any client could open any structure | ❌ | ✅ |

### 14. Wormholes `██████████████████░░` 92%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Lifecycle, K162, mass/lifetime tracking | ✅ | ✅ |
| System effects (Pulsar/Magnetar/etc) | ❌ | ✅ |
| **Signature cleanup** — Collapse + LoadAnomalies prune orphan WH signatures (no scanner spam) | ❌ | ✅ |
| **Per-system WH cap** — k-space ≤1, w-space ≤2 natural spawns | ❌ | ✅ |
| **Mass in tons** — ship kg normalized to Mg for maxJump/maxStable checks | ❌ | ✅ |
| **K162 exit stability** — landing at wormhole, paired exits across reloads | ❌ | ✅ |

### 15. Fleet `████████████████████` 100%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Create/manage, wings, squads, boosts, broadcasts | ✅ | ✅ |
| Watchlist, voice chat methods | ❌ | ✅ |

### 16. Incursions `███████████████████░` 96%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| State machine, wave NPCs, influence, rewards | ❌ | ✅ |
| Gate camps, belt replacement, focus period, 5 simultaneous | ❌ | ✅ |
| **Constellation penalties** (−10/25/50%), **CONCORD LP bonus** | ❌ | ✅ |
| Client notifications — OnTaleData/OnTaleStart/OnTaleEnd/OnInfluenceUpdate | ❌ | ✅ |
| **Penalty informer HUD** — tale data sent as `{taleID: taleData}` per system, re-sent on session change | ❌ | ✅ |
| Reward data — keyed by rewardCriteria with proper entries | ❌ | ✅ |
| **Scanner compatibility** — sites carry sigID, no scanner freeze, scan button doesn't stick during warp | ❌ | ✅ |
| **Client rendering of Sansha** — groupID remap (1051-1056 → known ship groups), NPC balls re-sent after WarpStop | ❌ | ✅ |
| **Real targets in the first pocket** — incursion placeholder groups replaced by real Sansha combat hulls, faction forced | ❌ | ✅ |
| **Acceleration gates between pockets** — placed ~30 km past the anomaly (behind the last structure), wave chain resilient to bubble changes, gate log on placement | ❌ | ✅ |

### 17. Scanning `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Probes, scan signatures, anomalies, D-scan | ✅ | ✅ |
| **Combat vs Core probes** — combat probes find ships/structures/drones, core probes signatures only | ❌ | ✅ |

### 18. Faction Warfare `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Join/leave, militia stats (char/corp/alliance/faction) | 🟡 | ✅ |
| **Plex spawning** (Scout/Small/Medium/Large), LP from NPC/PvP/plex | ❌ | ✅ |
| **Faction patrols**, **LP exchange rates** | ❌ | ✅ |
| **System flip** — plex capture accumulates flip points, occupier switches at threshold + notification | ❌ | ✅ |

### 19. Sovereignty `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| TCU 8h claim timer, vulnerable window | 🟡 | ✅ |
| IHub 2-cycle reinforcement, reinforce hour | ❌ | ✅ |
| Sov level (weeks), dev indices, upgrade effects | ❌ | ✅ |
| Outpost capture framework | ❌ | ✅ |
| **Alliance conflict zones** — SBU contested flag, ProcessSovStatusChanged, map display | ❌ | ✅ |
| **Change journal** — `sovChangeLog` records every owner flip (faction/alliance) with old/new owner | ❌ | ✅ |

### 20. Planetary Interaction `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Colonies, command center, pins, links, routes, programs | 🟡 | ✅ |
| Extractor → processor chains (P1→P4 by `schematicsTypeMap`) | ❌ | ✅ |
| **Customs offices** — anchored to the nearest planet, launch pad tied to `customInfo=planetID`, taxes | ❌ | ✅ |
| **NPC customs offices** — InterBus offices seeded on all high-sec planets | ❌ | ✅ |
| Orbital launch from colony to office | ❌ | ✅ |

### 21. Deployables (Mobile Warp Disruptor + Probes) `████████████████████` 99%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Deploy from cargo, space entity | ❌ | ✅ |
| Anchor/online via DogmaIM + PosMgr routing | ❌ | ✅ |
| Offlining timer + offline effect cleanup | ❌ | ✅ |
| **dgmTypeAttributes migration** — anchor/online/unanchor per type (Small/Medium/Large I/II) | ❌ | ✅ |
| **WarpDisruptFieldGenerating** visual effect on bubble active (sent to all players, including late-joiners) | ❌ | ✅ |
| **StructureOnlined** effect on anchor complete | ❌ | ✅ |
| **SendSlimUpdate** — groupID/categoryID/flag + real posTimestamp | ❌ | ✅ |
| Sec-level restriction (`AttrAnchoringSecurityLevelMax`) | ❌ | ✅ |
| **MWD range** — hardcoded per SDE typeID (5k–48k), not from DB attribute | ❌ | ✅ |
| Warp scramble bubble when online | ❌ | ✅ |
| **Scramble cleanup** — on range exit (per-player), on bubble exit (Remove), on last source removed | ❌ | ✅ |
| **Transient** — deleted from DB on server restart (Crucible behavior) | ❌ | ✅ |
| **Warp Disrupt Probe** — Interdiction Sphere Launcher, bubble, aggression (15min), highsec block | ❌ | ✅ |
| **Probe range** from `AttrWarpScrambleRange` (fallback 20km) | ❌ | ✅ |
| **Smartbombs** — AoE splash, capacitor drain, **crimewatch (OnWeaponFired+OnAggression)** | ❌ | ✅ |
| **Warp intercept** — bubble-flag based pull-out via GOTO transition (no 1s timer delay) | ❌ | ✅ |
| **Immediate scramble on bubble entry** — set AttrWarpScrambleStatus in Bubble::Add | ❌ | ✅ |
| **Server-side scramble check in WarpTo()** — blocks warp when AttrWarpScrambleStatus > 0 | ❌ | ✅ |
| **Invulnerability** — immune except to smartbombs/bombs | ❌ | ✅ |
| **Shuttle immunity** — group 31 hardcoded | ❌ | ✅ |
| **AttrWarpBubbleImmune** (Interdiction Nullifier) — all 6 check paths | ❌ | ✅ |
| **Immediate online after anchor** — MWD active right after anchor timer (bubble + scramble live) | ❌ | ✅ |
| **Stacked deployable drop** — Split(1) on drop/jettison, rest stays in cargo | ❌ | ✅ |
| **Warp scramble prevents jumps** — MWD bubble after anchoring scrambles the ship (no dock/jump until aggression timer cools); scramble active immediately on anchor completion | ❌ | ✅ |

### 22. Ship Module Restrictions

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Block fitting in space (subcaps) | ❌ | ✅ |
| Allow capitals to fit in space | ❌ | ✅ |
| Allow T3 subsystem swap in space | ❌ | ✅ |

### 23. Effects System `███████████████████░` 96%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Passive/online/active effects, implants/subsystems | ✅ | ✅ |
| Wormhole system effects, sov upgrade effects | ❌ | ✅ |
| **Dreadnought bonus expressions** — missing `dgmExpressions` rebuilt so capital hulls fit/activate | ❌ | ✅ |

### 24. Notifications `███████████████████░` 97%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Persistent DB notifications + live push | 🟡 | ✅ |
| Bill / tower / agent / corp sources, unread tracking | ❌ | ✅ |

### 25. Mail & LSC `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Private conversations, channels, mailing lists | ✅ | ✅ |
| Contact online notifications, corp mail role filter | ❌ | ✅ |
| **EVE-mail API** — inbox/sent listing, get, send, read/unread, notifications, unread counts; live push on send | ❌ | ✅ |

### 26. Standings `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| NPC/corp/faction standings, skills | 🟡 | ✅ |
| **Owner cache seeding** — factions, NPC corps, NPC characters present in the client owner table | ❌ | ✅ |
| **Safe standing writes** — deltas toward client-unknown factions skipped | ❌ | ✅ |

### 27. Bookmark System `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Personal/corp bookmarks, folders, coordinates | ✅ | ✅ |

### 28. Calendar `███████████████████░` 93%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Events, responses, notifications | 🟡 | ✅ |

### 29. Petitions & Support `███████████████████░` 95%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| In-game petition window (F12) — category tree per language, file/list/messages | ❌ | ✅ |
| GM workflow — reply, claim/unclaim, close, escalation | ❌ | ✅ |
| Shared thread backend across web and in-game views, FILETIME timestamps | ❌ | ✅ |
| **Category safety** — only client-known factions get standing rows; Character Sheet never blanks | ❌ | ✅ |
| Admin monitoring — login IP history, large human↔human flow audit, shared-IP multiboxing, approved account transfers, dual alert channels (public/admins) | ❌ | ✅ |
| **Ban UX** — persisted ban reason shown verbatim on banned login (unicode-safe); reserved/offensive account+character names refused with admin alert | ❌ | ✅ |
| **Account tooling** — per-account admin comment, ban reason, accounts grouped by IP/e-mail, ban-all-by-IP | ❌ | ✅ |

### 30. Memory Management `█████████████████░░░` 85%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Reference-count hardening — 32-bit refcount, separated diagnostics, opt-in hard-fail | ❌ | ✅ |
| **Sanitizer audit** — XMLParser virtual dtor, hash signed-shift overflow, aligned assign, bound-service cast | ❌ | ✅ |
| **PyRep ownership audit — outgoing path** — SendException double-release, Multicast/Broadcast consumption, CorpRegistryBound UAF, DroneSE scoop, probes outliving Client, MissionDataMgr/EncounterServer/AutoPay sweeps | ❌ | ✅ |
| **PyRep ownership audit — Agent/Contract/Calendar/Map/Cache** — 12 fixes incl. epic-arc journal crash, UpdateCacheFromSS UAF, hot-path leaks, GiveCache null-guard | ❌ | ✅ |
| **Cached CRowSet trees freed** — DeepClearRep PyObjectEx branch + per-row header refs (kills the per-login cache leak) | ❌ | ✅ |
| **Missile registry** — wrapper self-frees (was ~200B per launch, forever) | ❌ | ✅ |
| **malloc_trim every 5 min** — freed heap returned to the OS, RSS stays flat | ❌ | ✅ |
| DB reconnect only on connection errors (no retry storm on query errors) | ❌ | ✅ |
| **Field-level temps fixed** — central SetItem/SetField release helpers (~20 DB row-creators) + 964 call sites migrated (PyDict/packed-row overloads) | ❌ | ✅ |

### 31. Performance & Optimization `██████████████████░░` 88%

| Feature | Upstream | Fork |
|---------|:--------:|:----:|
| Load diagnosis playbook — docker stats, live processlist sampling, index audit | ❌ | ✅ |
| **Spawn churn control** — global ≤1 spawn/6 s, 60/40 direct/inbound mix, leave-chance 0.33→0.11% | ❌ | ✅ |
| **Per-spawn SQL diet** — pool pick by PK (no pool-wide ORDER BY RAND), PickCorp cached 5 min, skill top-up gated to 20% of respawns | ❌ | ✅ |
| **malloc_trim every 5 min** — keeps RSS flat after frees | ❌ | ✅ |
| **Hot-path indexes** — mktOrders(typeID), entity(ownerID,flag), entity(locationID,flag), botKillmailLegends(character_name), sovChangeLog | ❌ | ✅ |
| **Time dilation (TiDi)** — /tidi off\|50\|25\|10: per-system slow-mo for planned battles, official SetBallSpeed client sync | ❌ | ✅ |
| **Serpentis faction normalization** — no more wrong-faction dungeons from the random fallback | ❌ | ✅ |
| **System-boot prefetch** — neighbours of player systems pre-booted and held loaded (idle-tick; no worker thread — ItemFactory/SystemManager are not thread-safe) | ❌ | ✅ |

---

## Key Enhancements vs Upstream / Ключевые улучшения

- **Time Dilation (TiDi)** — official-style per-system slow-mo (/tidi off|50|25|10): speeds, module cycles, missile/NPC timers scale; client sync via SetBallSpeed; docks/skills/production unaffected
- **Memory campaign** — full PyRep ownership audits of every service path (agents/contracts/calendar/map/cache/network), cached rowsets now freed (per-login leak killed), missile wrapper registry, malloc_trim, refcount hardening + sanitizer fixes
- **Performance campaign** — spawn-churn control, per-spawn SQL diet, hot-path indexes, idle-time neighbour system-boot prefetch, load diagnosis playbook
- **Autopilot** — complete rewrite: auto-jump via `.tr` teleport, multi-hop via CmdStop → gate follow, 60s jump cloak, gate animation effects
- **POS** — reinforced mode, CPU/PG, reactions, weapon AI, skills, fuel notifications, role-based defence grid, manual fire control
- **POS industry & defence** — moon harvester→silo→reactor chain on the live reaction formulas (resource links, per-module storage, moon composition fixed once); working force-field barrier (warp lands on the shield); guaranteed warp-in guards; EWAR batteries (energy neutralizer / sensor damper / ECM); Ship Maintenance Array (store/board/scoop/refit); Assembly Array manufacturing; shield-hardener recalculation
- **Module EWAR & charges** — energy neutralizer / nosferatu / capacitor transfer corrected; remote sensor damper; ECM Burst & Remote ECM Burst; Warp Disrupt Field Generator; Titan doomsday; Cargo/Ship scanners; linked-weapon and charge-manager cleanup; pre-loaded charges restored
- **Item-exchange contracts** — offered items escrowed (singletons included), empty duplicates rejected, search hardened
- **Incursions** — full state machine, 5 simultaneous, named NPCs, gate camps, constellation penalties, penalty informer HUD, acceleration gates between pockets
- **Faction Warfare** — plex spawn, 3 LP channels, militia stats, patrols, system flip
- **Sovereignty** — TCU 8h claim + IHub 2-cycle reinforce + levels + upgrades + outpost capture + change journal
- **Science & Industry** — invention formula, reverse engineering, remote install, POS lines, recursive material chains
- **Planetary Interaction** — colonies, P1→P4 chains, planet-bound customs offices, NPC offices on high-sec planets
- **SDE validation** — all NPC types verified against the live API
- **~400 dungeon definitions** — anomaly, incursion, DED, data/relic, mission
- **Faction content** — full content for all 6 NPC factions (Sansha/Guristas/Angel/Blood/Serpentis/Rogue Drones): decor, turrets, anomalies, DED complexes, named NPCs
- **W-space / Sleeper block restored** — SleeperAI (remote rep, energy neut, capital escalation), sleeper combat sites by WH class, guaranteed WH in w-space, sleeper loot
- **PvE expeditions** — escalation system with faction/stage-specific DED sites, Journal integration, warp to site
- **Market full price list** — order-limit config fields uint8→uint32, full station asks returned to the client
- **Dungeon decor & accel gates** — faction-lore decoration tiers, precise warp-to-next-room gates, asteroid spacing
- **Warp hardening** — two-phase decel, deferred jump mid-warp, zero residual velocity, NPC balls re-sent after WarpStop, no bubble creation mid-warp, end-of-warp landing compensation (no teleport, no short-landing)
- **Orbit from structure surface** — gates/stations/planets/moons: orbit distance from surface, no inside-the-gate push-out
- **Stationary sentry turrets + role-based attack** — sentry/turret groups never move; attack matches role (turret fire / web / energy neutralizer / missiles)
- **Fighter-bomber to-hit** — AoE munitions always connect, no false distance misses
- **Analytic NPC threat assessment** — hull-class potential (capitals can cyno a fleet, battleships assumed fitted, fighter screen) instead of a precise fit guess
- **Self-defence crimewatch** — only the first attacker flagged for aggression; the victim's return fire is legal
- **Jump drives** — capital jumps require an active cyno in the destination; fuel type per race
- **ECM player jam** — ActiveModule ECM breaks target lock + sends ElectronicAttributeModifyTarget
- **Warp scramble prevents jumps** — MWD bubble after anchoring scrambles the ship (no dock/jump until the aggression timer cools)
- **Encounter missions full cycle** — accept → dungeon objectives spawn with real faction rat targets (lockable by the client) → warp link → clear → complete & hand-in
- **Character Sheet standings fixed** — client owner cache seeded with factions/NPC corps/NPC characters; standing deltas toward client-unknown factions no longer written; blank-standings crash gone
- **In-game petitions (F12)** — DB-backed `petitioner` service over a shared thread/category backend: category tree per language, create/list/messages, GM reply/claim/close queue, FILETIME timestamps
- **Admin monitoring & notifications** — login IP history, periodic audit (large human↔human ISK flows, shared-IP multiboxing), legitimate account-transfer approvals, dual alert channels with RU-friendly endpoint/proxy
- **Ban UX** — ban reason persisted (`banReason`) and shown verbatim on the banned login (single line, unicode-safe); reserved/offensive account & character names refused with admin alert
- **Account admin notes** — per-account free-form comment + ban reason exposed through the admin API; accounts grouped by IP/e-mail, ban-by-IP (all accounts from one address)
- **EVE-mail** — inbox/sent listing, read/unread, notifications and unread counts with live delivery on send
- **Memory hardening** — 32-bit refcount, split diagnostics, opt-in hard-fail, sanitizer-driven fixes (XMLParser virtual dtor, hash overflow, aligned assign, bound-service cast); PyRep ownership audits closed every practical leak path (cache rowsets and missile wrappers now freed); field-level temp leaks closed centrally (release helpers + 964 call sites); malloc_trim keeps RSS flat
