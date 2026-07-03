# Testing Checklist

Things to verify after recent changes (2026-07-02 session). Check off each item as you confirm it works.

---

## Drones

- [ ] **Launch drones** — Undock, open drone bay, launch drones. They should immediately begin orbiting your ship.
- [ ] **Engage target** — Lock a target, right-click drones → Engage. Drones should fly to and attack the target.
- [ ] **Return to drone bay** — With drones engaged, click Return to Drone Bay. Drones should fly all the way back and disappear into the bay (not stop at orbit range).
- [ ] **Return and orbit** — Right-click → Return and Orbit. Drones should fly back and resume orbiting the ship (not enter the bay).
- [ ] **Abandon drone** — Right-click a drone → Abandon. It should go neutral in space.
- [ ] **Reconnect to drones** — Log out with drones in space, log back in, use Reconnect to Drones. They should re-enter your control.
- [ ] **Sentry drones** — Launch sentry drones, they should orbit at position (not move). Damage should include SentryDroneInterfacing bonus.
- [ ] **Target Paint drones** — Launch a target painter drone, it should apply signature radius bonus to target.
- [ ] **ECM drones** — Launch ECM drones against a target. ECM should have a chance-based success (not always break locks).
- [ ] **Drone Assist** — Right-click a drone → Assist <player>. Drone should follow that player and attack their NPC target.
- [ ] **Drone Guard** — Right-click a drone → Guard <player>. Drone should orbit that player and engage anyone targeting them.
- [ ] **Drone Damage Amplifier** — Fit a DDA, launch drones. Damage should be higher than without the module.
- [ ] **Advanced Drone Interfacing** — Train to lvl 5, drone damage should include +2%/lvl bonus.
- [ ] **Mining Drone Specialization** — Train to lvl 5, mining drone yield should include +2%/lvl bonus.

---

## Warp / Movement

- [ ] **Warp to object at 0km** — Warp to a stargate, planet, or station at 0km. Ship should decelerate and stop at or within a few hundred meters of the target — not 10+ km past it.
// Landed within object, got flung out to 30KM. Re-evaluate, when warping at zero, should be landing within 2 to 2.5km away from the object.
- [ ] **No 180-degree flip** — During warp deceleration, the ship should slow down facing forward. It should NOT rotate 180 degrees or visually snap/reverse as it drops out of warp.
- [ ] **Immediate re-warp** — As soon as you drop out of warp, immediately warp somewhere else. Should work cleanly without freezing or desyncing.
- [ ] **Orbit entry** — Lock a target and set orbit. The ship should smoothly enter orbit without a one-tick direction jerk at the start.
- [ ] **Orbit at range** — Start orbit from far away (ship approaches first) and from close up (ship backs off). Both should transition correctly.

---

## Wormholes

- [ ] **Fit probe launcher** — Fit a Scan Probe Launcher I to a ship and load probes. The module should show as fittable and loadable.
- [ ] **Launch probes** — Activate the launcher. Probes should appear in space. Open the probe scanner window — probes should be visible and moveable.
- [ ] **Scan down a wormhole** — Position probes around a wormhole signature and scan. Result should reach 100% and appear in the probe results list.
- [ ] **Warp to wormhole** — Right-click the scan result → Warp To. Ship should warp to the wormhole object in space.
- [ ] **Jump through wormhole** — Right-click the wormhole → Jump. Ship should appear in the destination system near the K162 exit.
- [ ] **Oversized ship blocked** — Attempt to jump a ship heavier than the wormhole's max jump mass. Should receive an error message and not jump.
- [ ] **Collapsed wormhole blocked** — After enough jumps deplete mass, attempt another jump. Should receive "This wormhole has collapsed" and not jump.
- [ ] **Mass depletion visual** — After several heavy ship jumps, the wormhole should change visually: Full → Reduced → Disrupted size, Adolescent → Decaying → Closing age.
- [ ] **Wormhole collapse** — Jump enough mass through (or wait for the timer). Both entrance and K162 should disappear from space.

---

## Anomalies

- [ ] **Anomalies appear on scanner** — Open the probe scanner without launching probes. Cosmic Anomalies (combat sites) should appear automatically.
- [ ] **Signatures appear on scanner** — Probe scan a system. Gravimetric, Magnetometric, Radar, and Ladar signatures should show up and be scannable to 100%.
- [ ] **Warp to anomaly** — Right-click a cosmic anomaly → Warp To. Ship warps to the site and NPCs/objects are present in space.
- [ ] **Site respawn after expiry** — After ~2 hours an anomaly site should disappear from the scanner and a new site of the same type should appear shortly after. *(To test faster, temporarily reduce the expiry in `DungeonMgr::MakeDungeon()` from `2 * EvE::Time::Hour` to a smaller value and rebuild.)*
- [ ] **Multiple site types per system** — Systems should populate with a variety of site types (Grav, Mag, Radar, Ladar, Anomaly) up to the system's max, not just one type repeating.

---

## Market

- [ ] **Market browser speed** — Open the market browser and navigate categories. Should be fast with no multi-second lag between clicks.
- [ ] **Seeded regions — empire** — Characters in Caldari (The Forge/Citadel/Lonetrek), Amarr (Domain/Kador/Kor-Azor), Gallente (Essence/Sinq Laison/Placid), and Minmatar (Heimatar/Metropolis/Molden Heath) space should all find buy/sell orders in the market.
- [ ] **Seeded regions — NPC null-sec** — Stations in Curse, Stain, Venal, Great Wildlands, Syndicate, and Outer Ring should have market orders available.

---

## GM Commands

- [ ] **`/giveallskills me`** — Run the command, open the character sheet Skills tab. All skills should appear at level 5.
- [ ] **Skills persist after restart** — After running `/giveallskills me`, restart the server and log back in. All skills should still be level 5 (not reset).

---

## Docking / Logout

- [ ] **Dock at station** — Warp to a station and request dock. Ship should approach and complete the dock — not oscillate or wiggle in place.
- [ ] **Position saved on logout** — Warp to a location in space, log out, log back in. Ship should appear at or near where you logged out, not the previous system/location.

---

## NPC Combat

- [ ] **NPC repair visual** — Attack an NPC rat until its health drops. If it has repair modules (common on cruisers/battlecruisers), it should show a shield glow or armor repair beam animation on the NPC model.
- [ ] **NPC health bar updates on repair** — With an NPC targeted, its health bar should visibly rise when it activates a repair module, not stay frozen at a low value.
- [ ] **NPC damage numbers** — When an NPC lands a hit on you, a combat log message like "[NPC name] hits you, doing X damage." should appear immediately. Messages should not be delayed or arrive in batches after you click something.

---

## Fleet

- [ ] **Fleet Regroup** — Form a fleet, fly to a different location, use Fleet Regroup. All members should warp to the boss.
- [ ] **Warp to Member** — Right-click a fleet member → Warp To. Should warp to that member's position.
- [ ] **Warp Fleet to Member** — As fleet boss, right-click a member → Warp Fleet to Member. Entire fleet should warp.
- [ ] **Armored Warfare Specialist** — Train to lvl 5, set as fleet booster with module active. Fleet members should get +2%×5×2 armor HP.
- [ ] **Information Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 targeting range.
- [ ] **Siege Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 shield capacity.
- [ ] **Skirmish Warfare Specialist** — Train to lvl 5. Fleet members should get +2%×5 agility.
- [ ] **Mining Director** — Train to lvl 5, set as fleet booster. Fleet miners should get +10%×5 mining yield.
- [ ] **Gang Coordinator module** — Fit and activate on fleet booster ship. Fleet boost values should double.
- [ ] **CanDock with timer** — Fire weapons, then try to dock. Should be blocked while weapon timer is active.
- [ ] **CanJump with timer** — Fire weapons, then try to jump gate. Should be blocked while weapon timer is active.

## Combat Relogin

- [ ] **Safe logoff** — In space, stationary, no timers → disconnect. Ship should disappear immediately on relogin check.
- [ ] **Unsafe logoff (moving)** — In space, moving, no timers → disconnect. Ship should stay 60s then warp out.
- [ ] **Combat logoff** — In space with aggression → disconnect. Ship should stay 15 minutes.
- [x] **Warp scramble blocks emergency warp** — In space, moving, no timers → disconnect (60s emergency warp). Have an NPC or player warp-scramble you during those 60s. Ship should NOT initiate warp. After scrambler wears off, emergency warp should proceed.
- [ ] **Timer reset on hit** — After combat logoff, have someone hit the ghost ship. 15min timer should reset.
- [x] **Warp scramble blocks CmdSafeLogoff** — In space with a warp scrambler active on you, try CmdSafeLogoff. Should get "cannot safe logoff while warp scrambled" message.
- [ ] **NPC scramble clears on target change** — Get scrambled by an NPC, then have the NPC switch targets. Your warp status should clear.
- [ ] **Drone scramble clears on drone return** — Get scrambled by a drone, then recall the drone. Your warp status should clear.
- [ ] **Ghost ship visible** — After combat logoff, other players in the system should still see the ship.

## Image Server

- [ ] **Remote portrait fetch** — Log in from a different machine. Character portrait should load (not 404).
- [ ] **Fleet portrait** — Join a fleet, open fleet window. All character portraits should display (no crash).

## Cyno / Jump Bridge / Titan Bridge

- [ ] **Covert cyno** — Fit a covert cyno gen to a covert ops ship. Activate it. Should create a covert cyno field (no fleet required, should work in high-sec).
- [ ] **Regular cyno** — Fit a cyno gen to any ship. Must be in fleet and NOT in high-sec. Activate to create a cyno field.
- [ ] **Titan bridge** — Titan pilot lights cyno, then activates Jump Portal Generator targeting it. Fleet members should see "Jump through" option on the titan.
- [ ] **CmdJumpThroughFleet** — Fleet member clicks "Jump through" on titan. Should pay jump fuel and arrive at the cyno.
- [ ] **CmdJumpThroughAlliance** — Alliance bridge: same flow but alliance-wide.
- [ ] **POS Jump Bridge** — Install a jump bridge link between two POS towers. Jump through using CorpStructure option. Fuel should scale by distance.

## Login Warp

- [ ] **Login in space** — Log out in space, log back in. Ship should appear at login position with no kick.
- [ ] **Login near station** — Log out near a station, log back in. Ship should not get kicked 100km away.
- [ ] **Multiple logins** — Repeat login/logout cycle 5+ times. Should be stable each time.

## Stability

- [ ] **No crash on combat** — Engage NPCs with modules active and drones out. Killing NPCs or having modules deactivate mid-combat should not crash the server.
- [ ] **Multiple players** — If possible, have two clients in the same system performing actions simultaneously. No crashes from concurrent module/target activity.
