# Testing Gödel's Sky Consequence Systems

This document describes how to manually test the consequence systems in Gödel's Sky.

## Quick Start

```bash
# Build and run
cd /home/geeknik/dev/endless-sky
cmake -B build/linux -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF
ninja -C build/linux
./build/linux/godels-sky
```

## Condition Variables Reference

Use the in-game console to check condition values. The following conditions are available:

### ActionLog Conditions
| Condition | Description | Values |
|-----------|-------------|--------|
| `actionlog: pattern` | Dominant behavior pattern | 0=Unknown, 1=Trader, 2=Pirate, 3=Bounty Hunter, 4=Protector, 5=Warmonger |
| `actionlog: hostility` | Hostility score vs current system's government | 0-1000+ (scaled from 0.0-1.0+) |
| `actionlog: escalation` | True if escalating violence pattern detected | 0 or 1 |
| `actionlog: warmonger` | True if attacked 3+ distinct governments recently | 0 or 1 |
| `actionlog: witnessed` | Ratio of witnessed actions | 0-1000 (representing 0%-100%) |

### Encounter Conditions
| Condition | Description | Values |
|-----------|-------------|--------|
| `encounter: count` | Total NPCs in encounter log | Integer |
| `encounter: count disposition <type>` | Count of NPCs with specific disposition | Integer |
| `encounter: has disposition <type>` | True if any NPCs with disposition exist | 0 or 1 |

Disposition types: `friendly`, `hostile`, `wary`, `grateful`, `indebted`, `nemesis`

### Reputation Conditions
| Condition | Description | Values |
|-----------|-------------|--------|
| `reputation: threshold <level>` | True if current reputation matches level | 0 or 1 |
| `reputation: atrocity` | True if committed unforgiven atrocity | 0 or 1 |

Threshold levels: `war`, `hostile`, `unfriendly`, `neutral`, `friendly`, `allied`, `honored`

### Witness Conditions
| Condition | Description | Values |
|-----------|-------------|--------|
| `witness: unwitnessed` | True if <30% of recent actions witnessed | 0 or 1 |
| `witness: ratio <threshold>` | True if witnessed ratio below threshold | 0 or 1 |
| `witness: crime leaked` | Random: True if unwitnessed crime was leaked | 0 or 1 |

### Crew Conditions
| Condition | Description | Values |
|-----------|-------------|--------|
| `crew: trustworthiness` | Crew loyalty/morale score | 0-1000 |
| `crew: leak chance` | Probability of information leaks | 0-500 |
| `ship: compromised` | Random: True if ship security compromised | 0 or 1 |

## Testing Scenarios

### Scenario 1: Building a Trader Reputation

**Goal**: Establish yourself as a peaceful trader and see positive consequences.

**Steps**:
1. Start a new game
2. Complete only cargo delivery and passenger transport missions
3. Avoid all combat (flee from pirates)
4. After 10-15 successful deliveries, check `actionlog: pattern`
5. Look for "Merchant Recognition" missions to appear
6. News items should mention your trading reputation

**Expected Results**:
- `actionlog: pattern` = 1 (TRADER)
- "Merchant Recognition: Reliable Trader" mission appears
- NPCs comment positively on your trading in news
- Eventually unlock "Merchant Guild Access"

### Scenario 2: Creating Enemies and Nemeses

**Goal**: Create hostile NPCs through combat and see memory effects.

**Steps**:
1. Find a merchant ship
2. Attack it until disabled (don't destroy)
3. Let it escape (don't board or finish)
4. Note the ship's appearance/name
5. Travel around the same area
6. The same NPC should now be hostile and target you specifically

**Expected Results**:
- `encounter: has disposition hostile` = 1
- The disabled NPC targets you preferentially in future encounters
- "Hostile Encounter" missions may appear
- If you continue attacking the same NPC, they become NEMESIS

### Scenario 3: Unwitnessed Crimes

**Goal**: Test the witness system by committing crimes in empty systems.

**Steps**:
1. Find a system with no inhabited planets (uninhabited)
2. Verify no friendly ships are present
3. Attack and destroy a passing ship
4. Check `witness: unwitnessed`
5. Compare reputation before/after attack

**Expected Results**:
- Crimes in uninhabited systems have reduced reputation impact
- `witness: unwitnessed` = 1 if most recent actions were unwitnessed
- "player operates unseen" news may appear

### Scenario 4: Crew Trustworthiness

**Goal**: Test crew morale mechanics.

**Steps**:
1. Accumulate debt (don't pay crew salaries)
2. Check `crew: trustworthiness` - should decrease
3. Engage in piracy (lowers trust further)
4. Look for "Disgruntled Crew" missions
5. Pay off debts and do honest work
6. Check `crew: trustworthiness` recovery

**Expected Results**:
- `crew: trustworthiness` decreases with debt and piracy
- Low trustworthiness (<400) triggers crew problem missions
- High trustworthiness (>800) triggers loyalty bonus missions

### Scenario 5: Crime Leaks

**Goal**: Test the crime leak mechanics.

**Steps**:
1. Commit unwitnessed crimes (piracy in empty systems)
2. Have low crew trustworthiness (owe salaries)
3. Travel to populated systems
4. Check for "Crime Leaked" missions appearing
5. Observe if authorities become aware of your activities

**Expected Results**:
- `witness: crime leaked` can trigger randomly
- Leaks are more likely with low crew trust
- "Crime Leaked: Anonymous Tip" missions appear

### Scenario 6: AI Memory Integration

**Goal**: Verify that NPCs remember and react to past encounters.

**Steps**:
1. Save your game
2. Find and disable an NPC ship (don't destroy)
3. Save the UUID of the disabled ship (from console if possible)
4. Travel away and return after some time
5. Encounter the same NPC again

**Expected Results**:
- NPCs with HOSTILE disposition target player ships
- NPCs with WARY disposition flee when player approaches
- NPCs with NEMESIS disposition prioritize player over other targets

### Scenario 7: Pattern-Based NPC Reactions

**Goal**: Verify that NPCs react to your known behavior pattern.

**Steps**:
1. Build up a BOUNTY_HUNTER pattern by destroying pirates
2. After 10-15 pirate kills, check `actionlog: pattern` = 3
3. Travel to pirate-controlled systems
4. Observe pirate behavior when you arrive

**Expected Results**:
- Pirates flee from you at greater distances
- Pirates with low health flee immediately rather than engaging
- Weaker pirate ships avoid you entirely

**Alternative Test (Pirate Pattern)**:
1. Build up a PIRATE pattern by attacking merchants
2. Travel to trade-heavy systems
3. Observe unarmed merchant behavior

**Expected Results**:
- Unarmed merchants flee when you approach
- Armed merchants may still engage
- NPCs with "timid" personality always flee

### Scenario 8: Witness Flee Behavior

**Goal**: Verify that NPCs witnessing your crimes flee to report.

**Steps**:
1. Find a system with multiple ships
2. Attack a merchant ship while other friendly ships are nearby
3. Observe the nearby ships' behavior

**Expected Results**:
- Unarmed civilian ships flee toward the system edge
- Ships that witness the attack set "fleeing" state
- Ships with combat capability may engage instead of fleeing
- Fled witnesses contribute to reputation damage if they escape

### Scenario 9: Perceived Threat-Based Flee

**Goal**: Verify that NPCs flee based on encounter history threat level.

**Steps**:
1. Attack the same NPC type repeatedly (e.g., disable 3+ ships)
2. Travel to new systems with the same government's ships
3. Observe NPC behavior when you approach

**Expected Results**:
- NPCs with high perceived threat (>0.6) flee at greater distances
- Flee radius increases with threat level (1500 + threat * 2000)
- NPCs that have seen you attack allies are more cautious

## Console Commands for Testing

If the game has console access, use these to check conditions:

```
# Check behavior pattern
conditions "actionlog: pattern"

# Check crew trustworthiness
conditions "crew: trustworthiness"

# Check if any hostile NPCs exist
conditions "encounter: has disposition hostile"

# Check if operating unwitnessed
conditions "witness: unwitnessed"
```

## Data File Locations

Consequence-related data files:
- `data/human/consequences.txt` - Main reputation threshold missions
- `data/human/alpha-consequences.txt` - Alpha faction interactions
- `data/human/consequence-news.txt` - News items based on reputation
- `data/human/consequence-unlocks.txt` - Reputation-based unlocks
- `data/human/dispositions.txt` - NPC disposition missions
- `data/human/crew-consequences.txt` - Crew trustworthiness missions

## Known Limitations

1. **AI Integration Cannot Be Unit Tested**: AI flee behavior functions (`ShouldFleePlayerPattern`, `GetFleeUrgency`, `ShouldFleeAsWitness`) are in an anonymous namespace and cannot be unit tested. Testing requires full gameplay scenarios.

2. **Random Conditions**: `witness: crime leaked` and `ship: compromised` are probabilistic - they may not trigger every time conditions are met.

3. **Disposition Building**: Dispositions require multiple encounters to build up. Single interactions won't create strong dispositions.

4. **UUID Persistence**: NPC memory relies on UUID persistence across saves. New NPCs generated each session won't have history.

5. **Pattern Caching**: Player behavior pattern is cached per-step for performance. Pattern changes may take a few seconds to propagate to NPC behavior.

6. **Witness Flee Detection**: Witness flee only triggers when player is actively firing (Command::PRIMARY). Ships won't flee from players who are just targeting but not shooting.

## Reporting Issues

When reporting issues with the consequence system, please include:
1. Your save file
2. Steps to reproduce
3. Expected vs actual behavior
4. Values of relevant condition variables
