# EVEmu Testing Scenarios

## 1. Warp Scramble (MWD Bubble)

**Setup**: Drop and online a Mobile Large Warp Disruptor I (typeID 12200, range 26.5km).

**Expected**:
- [ ] Bubble visual appears (red sphere, correctly scaled)
- [ ] Ship inside bubble cannot warp — "Warp drive is disrupted." error
- [ ] Ship outside bubble (>26.5km) CAN warp
- [ ] Ship moving in/out of bubble updates scramble status within 1-2 seconds
- [ ] Multiple ships in bubble — all affected
- [ ] Bubble persists after ship leaves and re-enters

**Edge cases**:
- [ ] Shuttle inside bubble — immune (warpBubbleImmune check)
- [ ] NPC inside bubble — immune (no pilot)
- [ ] MWD offlined — scramble removed, warp allowed
- [ ] MWD unanchored — scramble removed

## 2. Warp Mechanics

**Setup**: Any ship, warp to bookmark at various distances.

**Expected**:
- [ ] Warp initiation: ship aligns, enters warp animation smoothly
- [ ] Warp acceleration: smooth ramp-up, no position jump
- [ ] Warp deceleration: smooth slowdown, ship arrives at target
- [ ] Short warp (<5 AU): complete accel+decel without cruise phase
- [ ] Long warp (>10 AU): accel → cruise → decel phases
- [ ] Arrival effects at gate: JumpIn on ship + GateActivity on gate model

**Edge cases**:
- [ ] Warp-to-0: arrives precisely at target
- [ ] Warp with insufficient capacitor: partial warp (as far as cap allows) or error
- [ ] Warp interrupted by MWD bubble entering target grid: pull out of warp
- [ ] Force-warp (catch-all): ships stuck in align eventually warp

## 3. Capacitor Usage

**Setup**: Check capacitor drain when initiating warp.

**Expected**:
- [ ] Frigate (1M kg, 5 AU warp): ~3-5 GJ drain
- [ ] Battleship (100M kg, 5 AU warp): ~300-500 GJ drain
- [ ] Warp Drive Operation skill reduces drain (10%/level)

## 4. Mail

**Setup**: Use in-game mail system.

**Expected**:
- [ ] Send mail to single character
- [ ] Send mail to multiple recipients
- [ ] Send mail with Cyrillic/Russian subject/body
- [ ] Reply/forward (isReplyTo/isForwardedFrom flags)
- [ ] Receive mail notification (OnMessage)
- [ ] Read mail body
- [ ] Mark as read/unread
- [ ] Move to/from trash
- [ ] Create/edit/delete labels
- [ ] Assign/remove labels on messages
- [ ] Mailing list management
- [ ] SyncMail returns correct message range
- [ ] Delete mail

**Edge cases**:
- [ ] Send to blocked contact — contact doesn't receive it
- [ ] Rate limit — max 5 messages/minute
- [ ] Too many recipients — error message

## 5. Defender / Anti-Missile

**Setup**: Fit defender launcher + Defender Missile charge on a ship.

**Expected**:
- [ ] Activate defender launcher (module cycles)
- [ ] When missile fired at ship, launcher auto-fires defender
- [ ] Defender missile intercepts incoming missile (both destroyed)
- [ ] Countermeasure_Launcher also works
- [ ] DefenderMissiles skill increases chance (5%/level)
- [ ] NPC defender missiles work (AttrEntityDefenderChance)

## 6. Bump / Collision

**Setup**: Two ships, one stationary, one approaching at speed.

**Expected**:
- [ ] Ships bump at correct surface distance (r1 + r2 + BUMP_DISTANCE)
- [ ] Bump notification messages appear
- [ ] Ship bounces off large structures (gates, stations)

## 7. Bracket / Slim Items

**Setup**: Check overview and bracket display for all entity types.

**Expected**:
- [ ] Ships show name, type, corp, alliance in brackets
- [ ] NPC ships show name and type
- [ ] Structures (gates, stations) show name
- [ ] Deployables (MWD) show name and state
- [ ] Anomalies, wormholes show name
- [ ] Missiles show name and source info
- [ ] No `AttributeError` in bracketMgr/michelle logs
- [ ] Right-click menu shows correct character/ corp/ alliance entries

## 8. Sovereignty

**Setup**: Claim a system via sovereignty mechanics.

**Expected**:
- [ ] Sov dashboard loads without errors
- [ ] militaryPoints/industrialPoints display correctly (0 for unclaimed)
- [ ] Development index map coloring works

## 9. Jump / Gate Transition

**Setup**: Use stargate to jump between systems.

**Expected**:
- [ ] JumpOut effect plays on departure gate
- [ ] Ship transitions to destination system
- [ ] JumpIn effect plays on arrival
- [ ] GateActivity effect on both gates
- [ ] Session change completes without errors
- [ ] No `NoneType.vx` crash in SceneManager

## 10. System Stress

**Setup**: Multiple clients in same grid.

**Expected**:
- [ ] 10+ clients in bubble, no crashes
- [ ] All clients see each other's balls
- [ ] Warp loop does not crash any client
- [ ] Bubble effects reach all clients
- [ ] Mail sent to multiple online recipients — all receive OnMessage
