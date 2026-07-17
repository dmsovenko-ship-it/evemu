# Testing Checklist

Updated 2026-07-17 — build 11: deployables, smartbombs, warp disrupt probes, timers

---

## Deployables (Mobile Warp Disruptor)

- [ ] **Deploy from cargo** — Right-click Mobile Warp Disruptor I in cargo → Drop. Should appear in space.
- [ ] **Anchor timer** — Right-click → Anchor. Should start timer (reads `AttrAnchoringDelay` from item).
- [ ] **Anchor complete** — After timer, status changes to Anchored.
- [ ] **Online** — Right-click → Online. Timer runs, then status changes to Online.
- [ ] **Warp scramble** — Ship enters bubble range. Should get `AttrWarpScrambleStatus > 0` → can't warp.
- [ ] **Unanchor** — Right-click → Unanchor. Timer runs, then object returns to unanchored state.
- [ ] **Sec-level restriction** — Try anchoring in highsec/lowsec. Should fail (only 0.0 allowed).
- [ ] **Corp ownership** — Try anchoring another corp's disruptor. Should be denied.
- [ ] **Destroy** — Shoot the disruptor. Should take damage and eventually pop.

## Warp Disrupt Probe

- [ ] **Interdiction Sphere Launcher** — Fit launcher to an Interdictor, load Warp Disrupt Probe charges.
- [ ] **Activate launcher** — Activate module. Probe should deploy in space.
- [ ] **Warp scramble bubble** — Ships in range (20km) should get scram status.
- [ ] **Highsec block** — Try activating in highsec. Should fail.
- [ ] **Aggression timer** — After launch, weapon timer should show (blocks docking/gate jump).

## Smartbombs

- [ ] **Activate smartbomb** — Fit a smartbomb, activate it. Should play `effects.SmartBomb` animation.
- [ ] **Damage** — Ships in range should take EM/Thermal/Kinetic/Explosive damage with falloff.
- [ ] **Capacitor drain** — Each cycle should consume capacitor (`AttrCapacitorNeed`).
- [ ] **Cycle time** — Module should cycle at correct speed (per `AttrSpeed`/`AttrDuration`).
- [ ] **No crash** — Activate without charge loaded. Should not crash (uses module's own `AttrDamage`).

## Timer Display

- [ ] **Session change timer** — Jump through gate. Timer icon should show ~20s.
- [ ] **Weapon timer** — Activate a weapon on a target. Timer should appear in top-left.
- [ ] **Criminal timer** — Commit a crime in highsec. Timer should show.

## Ship Modules

- [ ] **Warp Scrambler** — Activate on target. Target should not be able to warp.
- [ ] **Stasis Web** — Activate on target. Target speed should reduce.
- [ ] **ECM** — Activate on target. Target should lose lock.
- [ ] **Fitting in space** — Try to fit a module while in space in a subcap. Should be denied.
- [ ] **Fitting at station** — Fit the same module at station. Should work.
- [ ] **Capitals in space** — Fit a module on a carrier in space. Should work.
- [ ] **Subsystem swap in space** — Swap a T3 subsystem while in space. Should work.

## T3 Subsystems

- [ ] **Assemble T3 ship** — Assemble a T3 cruiser with 5 subsystems. Ship should appear with correct stats.
- [ ] **Subsystem bonuses** — Board the T3 ship. Passive bonuses from subsystems should apply.
- [ ] **Subsystem swap** — Replace a subsystem in station. New bonuses should apply on reboard.

## /giveallskills

- [ ] **All skills to 5** — Run `/giveallskills me`. All skills should become level 5.
- [ ] **Persistence** — Relog. Skills should still be level 5 (was not saving to DB).
