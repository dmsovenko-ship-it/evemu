# Testing Checklist

Things to verify after recent changes (updated 2026-07-11).

---

## POS

- [ ] **POS anchoring** — Anchor a Control Tower in a moon orbit. Wait for anchoring timer.
- [ ] **POS online** — Fuel the tower, online it. Force field should appear.
- [ ] **POS modules** — Anchor, online, and configure modules (shield hardener, gun, etc.).
- [ ] **Shield reinforcement** — Attack the tower until shield < 25%. Should enter reinforced mode (invulnerable).
- [ ] **Reinforced mode exit** — After reinforcement timer, tower exits with 0% shield. Repair shield above 25% to reset cycle.
- [ ] **POS fuel** — Let fuel run out. Tower should enter reinforced mode.

## Customs / Contraband

- [ ] **Customs NPCs at gates** — Jump into a highsec system. Customs NPCs (2 Commissioner + 2 Agent) should orbit some gates.
- [ ] **Contraband detection (mild)** — Load mild contraband (standingLoss < 0.05, qty < 10) into cargo. Jump a gate with customs NPCs. Get detection message + 60s timer.
- [ ] **60s timer → penalty** — Stay in highsec with contraband for 60s. Timer fires → standing loss + ISK fine + item confiscation. Customs NPCs attack.
- [ ] **No customs NPCs → no scan** — Jump a gate WITHOUT customs NPCs (80% of gates). Should NOT trigger scan even with contraband.
- [ ] **Lowsec/nullsec immunity** — Jump a gate with customs NPCs but in lowsec/nullsec. No scan.
- [ ] **Smuggling skill** — Train Smuggling to lvl 5. Detection chance should be visibly lower.

## Incursions (new)

- [ ] **Incursion contest rewards** — Join an incursion site, deal damage. On completion, ISK/LP should be proportional to damage dealt.
- [ ] **Incursion waves** — Clear NPCs in an incursion site. Next wave should spawn. After final wave, site completes.
- [ ] **Mothership** — Clear all systems in incursion constellation. Mothership should appear in HQ system.

## Ship Fittings

- [ ] **Save fitting** — Fit a ship, open fitting window, save the fitting. It should appear in saved fittings.
- [ ] **Load fitting** — Saved fitting should be loadable on the same ship type.
- [ ] **Rename/delete** — Rename a fitting, delete a fitting. Changes should persist after relog.

## Civilian Traffic

- [ ] **Civilians visible** — Enter a system with 2+ gates/stations. Civilian ships should appear and move between them.
- [ ] **Civilians despawn on system unload** — Leave the system. Civilians should be removed.
- [ ] **Civilians respawn** — Re-enter the system. New civilians should appear.

## Invention

- [ ] **Invention job** — Place a T1 BPC in a lab, start invention job. On completion, get T2 BPC (or failure message).
- [ ] **Invention consumes BPC run** — The T1 BPC should lose 1 run after invention attempt.

## Contract Auctions

- [ ] **Place bid** — Create an auction contract. Place a bid. Verify bid is recorded.
- [ ] **Finish auction** — Finish the auction. Winner should receive the item. Seller should receive ISK (minus 1% CONCORD tax).

## Mailing Lists

- [ ] **Create list** — Create a mailing list. It should appear in your mailing list panel.
- [ ] **Join/Leave** — Another character should be able to join the list (by name). Leave the list.
- [ ] **Send mail to list** — Send mail to the mailing list. All members should receive it.

## CONCORD LP Store

- [ ] **LP store offers** — Open LP Store for CONCORD (corpID 1000125). Should show skill hardwiring implants and CONCORD BPCs.
- [ ] **Correct costs** — Verfiy LP and ISK costs match the seeded data.

## Login Warp

- [ ] **Login in space** — Log out in space, log back in. Ship should appear at login position with no kick.
- [ ] **Login near station** — Log out near a station, log back in. Ship should not get kicked 100km away.
- [ ] **Multiple logins** — Repeat login/logout cycle 5+ times. Should be stable each time.

## Missions

- [ ] **Courier mission** — Accept a courier mission from an agent. Deliver cargo to destination station. Complete mission for reward.
- [ ] **Encounter mission** — Accept a combat encounter mission. Activate it, fly to the encounter location. Kill spawned NPCs. Complete.
- [ ] **Mining mission** — Accept a mining mission. Mine required ore. Deliver to agent.
- [ ] **Storyline mission** — Complete enough standard missions to trigger a storyline offer. Accept and complete it.
- [ ] **Mission completion flow** — After mission objectives are met, should show Complete button. Clicking it gives reward (ISK + standing + LP).

## Epic Arcs

- [ ] **Start epic arc** — Go to Sister Alitura in Arnon. Show Epic Arc Start button. Click to begin.
- [ ] **Chapter progression** — Complete chapter missions. Next chapter should auto-offer.
- [ ] **Branching** — Chapter 4 (Queen vs Blood) and Chapter 7 (faction commanders) should differ based on choice.
- [ ] **Final reward** — Complete The Blood-Stained Stars. Should receive +0.7 faction standing.
- [ ] **90-day cooldown** — After completion, attempting to start again should fail with cooldown message.

## Faction Warfare

- [ ] **Join militia** — Enlist in a faction militia. Should see FW standings update.
- [ ] **FW mission anomalies** — After joining, FW mission anomalies should appear on scanner.

## DED Complexes

- [ ] **DED 3/10** — Scan and warp to a Rated sig. Clear 3 rooms. Containers should unlock after NPCs die.
- [ ] **DED loot** — Containers should contain faction modules, Overseer's Effects, and ship BPCs.
- [ ] **DED 1/10, 5/10** — Different tiers spawn correctly with appropriate NPC sizes.

## Data/Relic Sites

- [ ] **Data site (Radar)** — Use Codebreaker on container. Loot should appear in container (not cargo).
- [ ] **Relic site (Magnetometric)** — Same as data but with Analyzer module.

## CONCORD

- [ ] **CONCORD response** — Commit a crime in highsec. CONCORD should spawn, target, and kill you.
- [ ] **CONCORD despawn** — After kill, CONCORD stays 5-10 min then despawns.
- [ ] **CONCORDOKKEN** — Kill a CONCORD ship. New one should respawn with increased damage.

## NPC Behavior

- [ ] **Customs NPCs** — Orbit gates at ~15% speed, should not aggro without contraband.
- [ ] **Anomaly NPCs** — Should have targeting crosshairs (red). Should orbit, not stand still.
- [ ] **Battleships** — Should orbit at appropriate range, not be stationary.

## Scanner

- [ ] **Anomaly scanner** — Should only show anomalies and wormholes. No NPCs or billboards.
- [ ] **Probe scanner** — DED (Rated) sites should NOT appear in scan results.

## Jita

- [ ] **No anomalies** — Jita system (30000142) should have zero anomalies.

## Stability

- [ ] **No crash on combat** — Engage NPCs with modules active and drones out. No crash.
- [ ] **Multiple players** — Two clients in same system performing actions simultaneously. No crashes.
- [ ] **No warp crash** — Warp between gates/systems repeatedly. No `Unknown packet type` or crash.
