# Testing Checklist

Things to verify after recent changes. Check off each item as you confirm it works.

---

## Drones

- [x] **Launch drones** — Undock, open drone bay, launch drones. They should immediately begin orbiting your ship.
- [x] **Engage target** — Lock a target, right-click drones → Engage. Drones should fly to and attack the target.
- [x] **Return to drone bay** — With drones engaged, click Return to Drone Bay. Drones should fly all the way back and disappear into the bay (not stop at orbit range).
- [x] **Return and orbit** — Right-click → Return and Orbit. Drones should fly back and resume orbiting the ship (not enter the bay).
- [x] **Abandon drone** — Right-click a drone → Abandon. It should go neutral in space.
- [x] **Reconnect to drones** — Log out with drones in space, log back in, use Reconnect to Drones. They should re-enter your control.
- [x] **Sentry drones** — Launch sentry drones, they should orbit at position (not move). Damage should include SentryDroneInterfacing bonus.
- [x] **Target Paint drones** — Launch a target painter drone, it should apply signature radius bonus to target.
- [x] **ECM drones** — Launch ECM drones against a target. ECM should have a chance-based success (not always break locks).
- [x] **Drone Assist** — Right-click a drone → Assist <player>. Drone should follow that player and attack their NPC target.
- [x] **Drone Guard** — Right-click a drone → Guard <player>. Drone should orbit that player and engage anyone targeting them.
- [x] **Drone Damage Amplifier** — Fit a DDA, launch drones. Damage should be higher than without the module.
- [x] **Advanced Drone Interfacing** — Train to lvl 5, drone damage should include +2%/lvl bonus.
- [x] **Mining Drone Specialization** — Train to lvl 5, mining drone yield should include +2%/lvl bonus.

---

## Warp / Movement

- [x] **Warp to object at 0km** — Warp to a stargate, planet, or station at 0km. Ship should decelerate and stop at or within a few hundred meters of the target — not 10+ km past it.
// Landed within object, got flung out to 30KM. Re-evaluate, when warping at zero, should be landing within 2 to 2.5km away from the object.
- [x] **No 180-degree flip** — During warp deceleration, the ship should slow down facing forward. It should NOT rotate 180 degrees or visually snap/reverse as it drops out of warp.
- [x] **Immediate re-warp** — As soon as you drop out of warp, immediately warp somewhere else. Should work cleanly without freezing or desyncing.
- [x] **Orbit entry** — Lock a target and set orbit. The ship should smoothly enter orbit without a one-tick direction jerk at the start.
- [x] **Orbit at range** — Start orbit from far away (ship approaches first) and from close up (ship backs off). Both should transition correctly.

---

## Wormholes

- [x] **Fit probe launcher** — Fit a Scan Probe Launcher I to a ship and load probes. The module should show as fittable and loadable.
- [x] **Launch probes** — Activate the launcher. Probes should appear in space. Open the probe scanner window — probes should be visible and moveable.
- [x] **Scan down a wormhole** — Position probes around a wormhole signature and scan. Result should reach 100% and appear in the probe results list.
- [x] **Warp to wormhole** — Right-click the scan result → Warp To. Ship should warp to the wormhole object in space.
- [x] **Jump through wormhole** — Right-click the wormhole → Jump. Ship should appear in the destination system near the K162 exit.
- [x] **Oversized ship blocked** — Attempt to jump a ship heavier than the wormhole's max jump mass. Should receive an error message and not jump.
- [x] **Collapsed wormhole blocked** — After enough jumps deplete mass, attempt another jump. Should receive "This wormhole has collapsed" and not jump.
- [x] **Mass depletion visual** — After several heavy ship jumps, the wormhole should change visually: Full → Reduced → Disrupted size, Adolescent → Decaying → Closing age.
- [x] **Wormhole collapse** — Jump enough mass through (or wait for the timer). Both entrance and K162 should disappear from space.

---

## Anomalies

- [x] **Anomalies appear on scanner** — Open the probe scanner without launching probes. Cosmic Anomalies (combat sites) should appear automatically.
- [x] **Signatures appear on scanner** — Probe scan a system. Gravimetric, Magnetometric, Radar, and Ladar signatures should show up and be scannable to 100%.
- [x] **Warp to anomaly** — Right-click a cosmic anomaly → Warp To. Ship warps to the site and NPCs/objects are present in space.
- [x] **Site respawn after expiry** — After ~2 hours an anomaly site should disappear from the scanner and a new site of the same type should appear shortly after. *(To test faster, temporarily reduce the expiry in `DungeonMgr::MakeDungeon()` from `2 * EvE::Time::Hour` to a smaller value and rebuild.)*
- [x] **Multiple site types per system** — Systems should populate with a variety of site types (Grav, Mag, Radar, Ladar, Anomaly) up to the system's max, not just one type repeating.

---

## Market

- [x] **Market browser speed** — Open the market browser and navigate categories. Should be fast with no multi-second lag between clicks.
- [x] **Seeded regions — empire** — Characters in Caldari (The Forge/Citadel/Lonetrek), Amarr (Domain/Kador/Kor-Azor), Gallente (Essence/Sinq Laison/Placid), and Minmatar (Heimatar/Metropolis/Molden Heath) space should all find buy/sell orders in the market.
- [x] **Seeded regions — NPC null-sec** — Stations in Curse, Stain, Venal, Great Wildlands, Syndicate, and Outer Ring should have market orders available.

---

## GM Commands

- [x] **`/giveallskills me`** — Run the command, open the character sheet Skills tab. All skills should appear at level 5.
- [x] **Skills persist after restart** — After running `/giveallskills me`, restart the server and log back in. All skills should still be level 5 (not reset).

---

## Docking / Logout

- [x] **Dock at station** — Warp to a station and request dock. Ship should approach and complete the dock — not oscillate or wiggle in place.
- [x] **Position saved on logout** — Warp to a location in space, log out, log back in. Ship should appear at or near where you logged out, not the previous system/location.

---

## NPC Combat

- [x] **NPC repair visual** — Attack an NPC rat until its health drops. If it has repair modules (common on cruisers/battlecruisers), it should show a shield glow or armor repair beam animation on the NPC model.
- [x] **NPC health bar updates on repair** — With an NPC targeted, its health bar should visibly rise when it activates a repair module, not stay frozen at a low value.
- [x] **NPC damage numbers** — When an NPC lands a hit on you, a combat log message like "[NPC name] hits you, doing X damage." should appear immediately. Messages should not be delayed or arrive in batches after you click something.

---

## Fleet

- [x] **Fleet Regroup** — Form a fleet, fly to a different location, use Fleet Regroup. All members should warp to the boss.
- [x] **Warp to Member** — Right-click a fleet member → Warp To. Should warp to that member's position.
- [x] **Warp Fleet to Member** — As fleet boss, right-click a member → Warp Fleet to Member. Entire fleet should warp.
- [x] **Armored Warfare Specialist** — Train to lvl 5, set as fleet booster with module active. Fleet members should get +2%×5×2 armor HP.
- [x] **Information Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 targeting range.
- [x] **Siege Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 shield capacity.
- [x] **Skirmish Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 agility.
- [x] **Mining Director** — Train to lvl 5, set as fleet booster. Fleet miners should get +10%×5 mining yield.
- [x] **Gang Coordinator module** — Fit and activate on fleet booster ship. Fleet boost values should double.
- [x] **CanDock with timer** — Fire weapons, then try to dock. Should be blocked while weapon timer is active.
- [x] **CanJump with timer** — Fire weapons, then try to jump gate. Should be blocked while weapon timer is active.

## Combat Relogin

- [x] **Safe logoff** — In space, stationary, no timers → disconnect. Ship should disappear immediately on relogin check.
- [x] **Unsafe logoff (moving)** — In space, moving, no timers → disconnect. Ship should stay 60s then warp out.
- [x] **Combat logoff** — In space with aggression → disconnect. Ship should stay 15 minutes.
- [x] **Warp scramble blocks emergency warp** — In space, moving, no timers → disconnect (60s emergency warp). Have an NPC or player warp-scramble you during those 60s. Ship should NOT initiate warp. After scrambler wears off, emergency warp should proceed.
- [x] **Timer reset on hit** — After combat logoff, have someone hit the ghost ship. 15min timer should reset.
- [x] **Warp scramble blocks CmdSafeLogoff** — In space with a warp scrambler active on you, try CmdSafeLogoff. Should get "cannot safe logoff while warp scrambled" message.
- [x] **NPC scramble clears on target change** — Get scrambled by an NPC, then have the NPC switch targets. Your warp status should clear.
- [x] **Drone scramble clears on drone return** — Get scrambled by a drone, then recall the drone. Your warp status should clear.
- [x] **Ghost ship visible** — After combat logoff, other players in the system should still see the ship.

## Image Server

- [x] **Remote portrait fetch** — Log in from a different machine. Character portrait should load (not 404).
- [x] **Fleet portrait** — Join a fleet, open fleet window. All character portraits should display (no crash).

## Covert Cloak

- [x] **Cloak fitting** — Fit Covert Ops Cloaking Device II to a Black Ops battleship. Client should NOT show "10000 tf" error (cpuNeed=0 in DB).
- [x] **Cloak online** — Click the power button on the cloak module. It should go online (cpu=100 in log).
- [x] **Cloak activate** — Right-click → Activate (or use the activation button). Should log "Cloak activated" and ship should visually cloak.
- [x] **Cloak button animation** — Module button should show active/pulsing state.
- [x] **Cloak deactivate** — Right-click → Deactivate (or power button). Should log "Cloak deactivated" and button state should return to normal (no red flash).
- [x] **Cloak re-activate** — After deactivation, should be able to activate again.
- [x] **Cloak offline** — Power button while active → cloak turns off, module goes offline.
- [x] **Cloak under jump** — Activate cloak, then warp/jump. Cloak should deactivate automatically (optional: expected EVE behavior).

## Cyno / Jump Bridge / Titan Bridge

- [x] **Covert cyno** — Fit a covert cyno gen to a covert ops ship. Activate it. Should create a **Covert** Cynosural Field (not regular). No fleet required, should work in high-sec.
- [x] **Covert cyno under cloak** — Activate cloak first, then activate covert cyno. Should work (no "DeniedActivateCloaked" error).
- [x] **Covert cyno visual** — Field should appear on overview as "Covert Cynosural Field", not regular.
- [x] **Regular cyno** — Fit a cyno gen to any ship. Must be in fleet and NOT in high-sec. Activate to create a regular cyno field.
- [x] **Titan bridge (cross-system)** — Titan pilot lights cyno, then activates Jump Portal Generator targeting it. Fleet members should see "Jump through" option on the titan.
- [x] **Covert bridge (cross-system)** — Black Ops ship has Covert Jump Portal Generator online. Fleet member right-clicks the Black Ops ship → should see "Bridge to..." option. Clicking it should open the portal (log "Bridge opened").
- [x] **CmdJumpThroughFleet** — Fleet member clicks "Jump through" on titan. Should pay jump fuel and arrive at the cyno.
- [x] **CmdJumpThroughAlliance** — Alliance bridge: same flow but alliance-wide.
- [x] **Hyperjump to cyno (cross-system)** — Fleet member right-clicks cyno pilot in fleet → hyperjump. Works with fuel consumption.
- [x] **Fuel consumption** — Jump consumes isotope fuel from fuel bay based on distance. Verified.
- [ ] **POS Jump Bridge** — Install a jump bridge link between two POS towers. Jump through using CorpStructure option. Fuel should scale by distance.
- [x] **Portal broadcast** — When any Jump Portal Generator goes online/offline, other ships in the same bubble should receive the module state change notification.
- [x] **OnSpecialFX for cyno** — Module activation effect visible on source ship (guid empty error fixed via GetEffectGuid fallback).
- [x] **Cyno field global visibility** — Cyno entity visible across bubbles (AttrIsGlobal fix).

> ⚠️ **Known limitation — same-system beacon jump**: When the cyno/covert-cyno field is in the **same solar system** as the jumping ship, the fleet right-click menu does **not** show the hyperjump/bridge option. Only inter-system jumps work through the fleet menu. This is a client-side limitation (`menusvc.py` / fleet service beacon filtering).  
> *Цино/коверт-цино в одной системе с кораблём — меню прыжка во флоте не появляется. Только межсистемные прыжки.*

## Customs / Contraband

- [ ] **Customs NPCs at gates** — Jump into a highsec system. Check if customs NPCs (2 Commissioner + 2 Agent) are orbiting some gates.
- [ ] **Contraband detection (mild)** — Load mild contraband (standingLoss < 0.05, qty < 10) into cargo. Jump a gate with customs NPCs. Should get "detected" message + 60s timer.
- [ ] **60s timer → penalty** — After detection, stay in highsec with contraband for 60s. Timer fires → "engaged" + standing loss + ISK fine + item confiscation. Customs NPCs at the gate should start attacking.
- [ ] **Jettison avoids penalty** — After detection, jettison all contraband within 60s. Timer fires → no penalty (no contraband in cargo).
- [ ] **Severe contraband (immediate)** — Load heavy drugs (standingLoss >= 0.05) or large quantity (>= 10 units). Jump a gate with customs NPCs. Should get "detected" + "engaged" immediately + NPC attacks. No 60s timer.
- [ ] **No customs NPCs → no scan** — Jump a gate WITHOUT customs NPCs (80% of gates). Should NOT trigger scan even with contraband.
- [ ] **Lowsec/nullsec immunity** — Jump a gate with customs NPCs but in lowsec/nullsec. No scan.
- [ ] **Smuggling skill** — Train Smuggling to lvl 5. Detection chance should be visibly lower.
- [ ] **Standing loss** — After penalty, check faction standings. Should have decreased.
- [ ] **ISK fine** — After penalty, wallet should show a deduction of `basePrice × fineByValue × quantity`.

## Login Warp

- [ ] **Login in space** — Log out in space, log back in. Ship should appear at login position with no kick.
- [ ] **Login near station** — Log out near a station, log back in. Ship should not get kicked 100km away.
- [ ] **Multiple logins** — Repeat login/logout cycle 5+ times. Should be stable each time.

## Missions

- [ ] **Courier mission** — Accept a courier mission from an agent. Deliver cargo to destination station. Complete mission for reward.
- [ ] **Encounter mission** — Accept a combat encounter mission. Activate it, fly to the encounter location. Kill the spawned NPCs. Complete mission.
- [ ] **Mining mission** — Accept a mining mission. Mine required ore. Deliver to agent.
- [ ] **Distribution mission** — Accept a distribution (delivery) mission. Deliver items to target station in the same region. Complete mission.
- [ ] **Storyline mission** — Complete enough standard missions to trigger a storyline offer. Accept it, complete it. Should get derived standing adjustments.
- [ ] **Mission completion flow** — After mission objectives are met, agent window should show "Complete" button. Clicking it should give reward (ISK + standing) and allow accepting next mission.
- [ ] **Decline mission** — Agent offers a mission, click decline. Should not damage standing (Crucible behavior).
- [ ] **Agent dialog** — Right-click an agent in station → Conversation. Should show mission offer if available. "Epic Arc Start" button should appear for epic arc agents.

## Epic Arcs

- [ ] **Start epic arc** — Go to the starting agent for "The Blood-Stained Stars" (e.g., Sister Alitura in Arnon). Conversation should show "Epic Arc Start" button. Click it to begin.
- [ ] **Chapter progression** — Complete chapter missions. Next chapter should auto-offer (GetPendingMissions should show next agent).
- [ ] **Branching choice (Ch 4)** — In Chapter 4, choose between tracking and Blood route. Chapter 5 missions should differ based on choice.
- [ ] **Branching choice (Ch 7)** — In Chapter 7, choose a faction commander. Final mission should match.
- [ ] **Final reward** — Complete the final arc mission. Should receive +0.7 faction standing (scaled by Social skill).
- [ ] **Epic journal** — Open Epic Journal window. Should show arc progress, chapter status, mission counts.
- [ ] **90-day cooldown** — After completing the arc, attempting to start again should fail with "cooldown active" message.
- [ ] **Mission state persistence** — Log out mid-arc, log back in. Arc state should persist (GetMyEpicJournalDetails returns correct data).

## Career Agents

- [ ] **Career agent dialog** — Visit a career agent (e.g., Caldari Military). Should offer career path missions.
- [ ] **Career progression** — Complete first career mission; next mission should auto-offer. Complete all 4 career paths.
- [ ] **Career rewards** — After finishing career path, should receive rewards (ISK + items).
- [ ] **Aura tutorial** — New character logs in for first time. Aura should prompt with tutorial steps.
- [ ] **Tutorial goodies** — Complete tutorial steps. Should receive starting items/ships at milestones.

## COSMOS Missions

- [ ] **COSMOS agent availability** — Fly to a COSMOS agent station. Agent should offer missions.
- [ ] **COSMOS Caldari arc** — Complete all 10 Caldari COSMOS missions in sequence.
- [ ] **COSMOS Amarr arc** — Complete all 10 Amarr COSMOS missions.
- [ ] **COSMOS Gallente arc** — Start with Basic, then proceed through 3 advanced missions.
- [ ] **COSMOS pirate agents** — Find Serpentis/Guristas/Sansha COSMOS agents. Should offer missions.
- [ ] **COSMOS artifact (Ani) missions** — Accept artifact recovery missions. Use exploration items to complete.

## Research Agents

- [ ] **Research agent dialog** — Visit a research agent. Conversation should show research offer.
- [ ] **Start research** — Accept research. Should start accumulating research points per field.
- [ ] **Research fields** — Agent should offer research in their assigned tech fields.

## Faction Warfare

- [ ] **Join militia** — Enlist in a faction militia. Should see FW standings update.
- [ ] **FW mission anomalies** — After joining, FW mission anomalies should appear on scanner.

## LP Store

- [ ] **LP Store access** — Open LP Store (Nexus → Loyalty Point Store or via corp station service). Should show offers.
- [ ] **LP Store items** — Items should display correct LP cost and ISK cost. (Cannot purchase — store is data-only without the client-side LP store service.)

## Anomalies

- [ ] **Site respawn after expiry** — Wait for anomaly to expire (~2 hours). New anomaly of the same type should respawn automatically. *(For testing: can temporarily reduce expiry timer in DungeonMgr::MakeDungeon.)*

## New Features (2026-07-08)

- [ ] **Incursion contest** — Join an incursion site, deal damage, complete site. Rewards should be proportional to damage dealt (not equal split). Check wallet for varying reward amounts.
- [ ] **Incursion waves** — Clear NPCs in an incursion site. Next wave should spawn with harder NPCs. After final wave, site should be marked complete.
- [ ] **Ship Fittings** — Open fitting window, save a fitting. Log out, log back in. Fitting should appear in saved fittings list. Rename and delete should work.
- [ ] **Civilian traffic** — Enter a system with 2+ gates/stations. Civilian ships should appear and move between them. Leaving system → civilians despawn.
- [ ] **Invention** — Place a T1 BPC in a lab with datacores. Start invention job. On completion, receive T2 BPC (or failure message).
- [ ] **Contract auctions** — Create an auction contract. Place a bid. Finish the auction. Winner should receive item, seller should receive ISK (minus tax).
- [ ] **Mailing lists** — Create a mailing list. Join it. Send mail to the list. All members should receive it. Leave the list.
- [ ] **CONCORD LP store** — Check LP store for CONCORD (corpID 1000125). Should show implants and CONCORD BPCs with correct LP/ISK costs.

## Stability

- [ ] **No crash on combat** — Engage NPCs with modules active and drones out. Killing NPCs or having modules deactivate mid-combat should not crash the server.
- [ ] **Multiple players** — If possible, have two clients in the same system performing actions simultaneously. No crashes from concurrent module/target activity.
