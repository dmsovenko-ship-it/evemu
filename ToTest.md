# Testing Checklist

Updated 2026-07-23 — build 14: 97 commits — MWD, smartbomb, warp, orbit, approach, cloak, bubbles

---

## Deployables (Mobile Warp Disruptor)

- [ ] **Deploy from cargo** — Right-click MWD in cargo → Drop. Should appear as inert object (no bubble). `posState=-2`.
- [ ] **Anchor timer** — Right-click → "Unanchor" (client sends `effectID=650`). Anchor timer starts from SDE (`AttrAnchoringDelay`): Small I=2min, II=1min; Med I=4min, II=2min; Large I=8min, II=4min.
- [ ] **Anchor complete** — After timer → immediately Online. `WarpDisruptFieldGenerating` + `StructureOnlined` effects play. Bubble active.
- [ ] **Warp scramble** — Ship enters bubble range → `AttrWarpScrambleStatus > 0` → can't warp.
- [ ] **Scramble cleanup** — Ship leaves bubble range → scramble cleared (no other sources).
- [ ] **Unanchor** — Right-click anchored MWD → "Unanchor". Timer runs (same as anchor). Bubble visual stops. Object returns to unanchored state.
- [ ] **Sec-level restriction** — Try anchoring in highsec/lowsec. Should fail (only 0.0 allowed).
- [ ] **Corp ownership** — Try anchoring another corp's MWD. Should be denied.
- [ ] **MWD attribute migration** — New MWDs after migration should use correct SDE anchor/range per type.

## Warp Disrupt Probe

- [ ] **Interdiction Sphere Launcher** — Fit launcher to Interdictor, load Warp Disrupt Probe charges.
- [ ] **Activate launcher** — Probe deploys in space. Aggression timer starts (15min).
- [ ] **Warp scramble bubble** — Ships within 20km get scram status.
- [ ] **Highsec block** — Activate in highsec → fails (can't launch).
- [ ] **Scramble cleanup on range exit** — Ship leaves probe range → scramble cleared.
- [ ] **Scramble cleanup on probe removal** — Probe expires/is destroyed → all ships in bubble get scramble cleared.
- [ ] **Smartbomb destroy** — Smartbomb can destroy probe (normal weapons can't).

## Smartbombs

- [ ] **Activate smartbomb** — Fit smartbomb, activate. `effects.SmartBomb` animation plays.
- [ ] **Crimewatch** — Hitting another player with smartbomb should set weapon+aggression timer (docking blocked, CONCORD in highsec).
- [ ] **Kill right** — Killing a player with smartbomb should grant kill right to victim.
- [ ] **Killmail** — Killmail should be sent correctly.
- [ ] **Damage** — Ships in range take EM/Thermal/Kinetic/Explosive damage with falloff.
- [ ] **Capacitor drain** — Each cycle consumes capacitor (`AttrCapacitorNeed`).
- [ ] **Cycle time** — Module cycles at correct speed (per `AttrSpeed`/`AttrDuration`).

## Warp / Movement

- [ ] **Early warp start** — Ship should start warp when within 30° of target after half align time (no need for full 6° alignment).
- [ ] **Snap stop** — Ship stops immediately on command, no 200m drift.
- [ ] **Bubble hopping** — Check server logs for `[BubbleTrace]` — NPCs should NOT hop bubbles every tick.
- [ ] **Empty bubble cleanup** — Empty bubbles should be removed within 5 seconds.

## Jump Cloak

- [ ] **Cloak after jump** — After jumping, enemy should NOT see you (check `[BubbleTrace] AddBallExclusive: ... is cloaked — skipping`).
- [ ] **Cloak duration** — After 60s, cloak should drop automatically.

## Timer Display (Crucible — only self-visible)

- [ ] **Session change timer** — Jump through gate. Timer icon shows ~20s (self only).
- [ ] **Weapon timer** — Activate weapon on target. Timer appears in top-left (self only).
- [ ] **Criminal timer** — Crime in highsec. Timer shows (self only).
- [ ] **MWD anchor timer** — No visual timer (Crucible): timer runs server-side, no client countdown.

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
