# Testing Checklist

Updated 2026-07-24 — build 15: 130+ commits — MWD range, warp-to-0 surface, collision, visibility fixes

---

## Deployables (Mobile Warp Disruptor)

- [ ] **Deploy from cargo** — Right-click MWD in cargo → Drop. Should appear as inert object (no bubble). `posState=-2`.
- [ ] **Anchor timer** — Right-click → "Unanchor". Timer starts from SDE per type.
- [ ] **Anchor complete** — After timer → immediately Online. `WarpDisruptFieldGenerating` + `StructureOnlined` effects play. Bubble active.
- [ ] **MWD range by type** — Verify per SDE: Small 5/7.5km, Medium 11.5/17.5km, Large 26.5/40km, Jump 48km. NOT from DB attribute.
- [ ] **Warp scramble** — Ship within MWD range → `AttrWarpScrambleStatus > 0` → can't warp.
- [ ] **Scramble cleanup** — Ship leaves MWD range → scramble cleared. Ship leaves bubble → cleared on `Bubble::Remove`.
- [ ] **MWD field visible** — All players in bubble see `WarpDisruptFieldGenerating` effect (including late-joiners).
- [ ] **Warp intercept** — Warping INTO MWD bubble → pulled out of warp (HasWarpBubble check, no 1s timer delay).
- [ ] **After intercept** — Ship sees all bubble entities (MWD, other ships, NPCs). Others see the ship.
- [ ] **Unanchor** — Right-click anchored MWD → "Unanchor". Timer runs. Bubble stops. Returns to unanchored.
- [ ] **Sec-level restriction** — Highsec/lowsec block (only 0.0 allowed).
- [ ] **Corp ownership** — Another corp's MWD denied.
- [ ] **Transient** — MWD deleted from DB on server restart.

## Warp Disrupt Probe

- [ ] **Interdiction Sphere Launcher** — Fit to Interdictor, load charges.
- [ ] **Activate launcher** — Probe deploys in space. Aggression (15min). Check highsec block.
- [ ] **Warp scramble bubble** — Ships within 20km get scram status.
- [ ] **Probe removal cleanup** — All ships in bubble get scramble cleared when probe expires/destroyed.

## Warp / Movement / Collision

- [ ] **Warp-to-0 gate** — Land at gate surface + ~100m (not 5km inside). Can't fly through gate model.
- [ ] **Warp-to-0 station** — Land at station surface + ~100m (not 2.5km min distance).
- [ ] **Warp-to-N** — Warp to N km from gate/station center. Correct distance (not offset by radius).
- [ ] **Collision detection** — Fly towards large static entity (gate, station, planet). Ship bumps off at surface.
- [ ] **Early warp start** — Warp starts at <30° after half align time.
- [ ] **Snap stop** — Ship stops immediately on command, no drift.
- [ ] **Bubble hopping** — Check `[BubbleTrace]` — NPCs NOT hopping every tick.
- [ ] **Empty bubble cleanup** — Empty bubbles removed within 5s.
- [ ] **JumpIn effect** — When someone jumps through gate, players in destination bubble see `effects.JumpIn`.
- [ ] **Overlapping bubbles** — Only entities within 300km of player position are visible (not all entities from nearby bubbles).

## Visibility After Warp

- [ ] **Normal warp to gate** — Ship visible to others from ~300km (grid edge), not just after WarpStop.
- [ ] **Fleet warp** — All fleet members visible to each other during and after warp.
- [ ] **Intercepted warp** — After MWD pull-out, all bubble entities visible (ship, MWD, NPCs, drones).

## Jump Cloak

- [ ] **Cloak after jump** — Enemy does NOT see you after gate jump (`AddBallExclusive — is cloaked — skipping`).
- [ ] **Cloak duration** — 60s (or remaining from previous cloak if <30s).
- [ ] **Uncloak** — After timer expires, ship is added to bubble. Other players see you. You see other players.

## Warp Capacitor

- [ ] **Capacitor drain on warp** — Warp consumes capacitor per `AttrWarpCapacitorNeed × mass × AU`. Minimum 0.00001.
- [ ] **Insufficient capacitor** — Warp distance limited by available cap. If 0 cap → can't warp.

## MWD / Bracket / Crash Fixes

- [ ] **No Unknown packet type crash** — MWD in SetState/AddBalls does not crash client (DataSector fix).
- [ ] **No client bracket crash** — Hover over brackets shows no `AttributeError: 'NoneType' object has no attribute 'lower'`.

## Ship Modules

- [ ] **Warp Scrambler** — Activate on target. Target can't warp.
- [ ] **Stasis Web** — Activate on target. Target speed reduces.
- [ ] **ECM** — Activate on target. Target loses lock.
- [ ] **Fitting in space** — Subcap denied. Capital allowed. T3 subsystem swap allowed.

## /giveallskills

- [ ] All skills to 5, persists on relog.
