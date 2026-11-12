# Floor 1 — Ice and Jungle

## Scope

The first floor includes introduction, hero selection, biome choice, nine rooms, eight riddles, combat and the first guardian. It reuses the original art, music, riddle bank and story files. The original Windows Forms game remains as a reference; the active SFML game continues through [floor 2](FLOOR_TWO.md) and [floor 3](FLOOR_THREE.md).

## Route

| Rooms | Content |
| --- | --- |
| 1–3 | Library, chest and ordinary battle, shuffled once per new run. |
| 4 | Captive warrior: free him for reputation and story, or take his vitality for stats. |
| 5 | Second ordinary battle; confront or sneak past. |
| 6 | Respite: drink for health and a story clue, or search for stat rewards. |
| 7 | Second library: choose a stat book. |
| 8 | Surtees' shop: choose one potion or the Book of Knowledge. |
| 9 | Yeti (Ice) or Jungle Warden (Jungle). Must be defeated to clear the floor. |

An ordinary room's outcome is followed by one riddle. A chest starts with its riddle and allows its treasure to be claimed only after a correct answer. The boss has no riddle. After the first guardian, **Descend the stairway** opens the Desert/Ghost doors and preserves the entire player state; backing out returns to the stairway. A new run is only available after game over or an ending.

## Hero stats and rewards

The combat numbers in this guide describe **Easy**. See [combat balance](COMBAT_BALANCE.md) for Medium, Hard and Impossible enemy stats. Hero stats, rewards and riddle rules are the same on every level.

Starting stats below include the original automatic +3 class ability. This bonus is applied once per run.

| Hero | STR | WIS | OBS | AGI | HP | ACC | Combat damage stat |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Warrior | 7 | 1 | 0 | 3 | 35 | 2 | Strength |
| Ranger | 2 | 2 | 4 | 2 | 30 | 8 | Accuracy |
| Mage | 1 | 7 | 3 | 1 | 40 | 1 | Wisdom |
| Rogue | 2 | 0 | 4 | 8 | 30 | 5 | Agility |

Original reward rules are retained: Warrior Strength, Ranger Accuracy and Rogue Agility gains have a 1.5 multiplier. Their all-stat rewards also multiply Health by 1.5. Mage individual stat rewards have a 1.5 multiplier; all-stat rewards give 1.5 for non-health stats and 3 for Health. Health is the original growing stat, rather than a capped heal meter. Observation remains a tracked progression stat; the original later-game answer-elimination threshold is not reached in this floor.

Riddle time is `20 + floor(Wisdom / 2)` seconds. Correct answers give `+0.75` to all stats before class multipliers; wrong answers remove `1.5`, and timeouts remove `0.5`. Penalties cannot make stats negative. A correct chest seal additionally allows one all-stat `+2` treasure reward. Riddles and answer positions are shuffled using one seeded generator; no question repeats within a run.

Libraries offer Strength, Agility, Wisdom or Observation `+2`. The shop offers Health `+5`, all stats `+1`, or a reputation/story choice. Drinking at respite gives Health `+10` and reputation; searching gives all stats `+2`. Reputation has three first-floor sources: freeing the prisoner, drinking at respite and choosing the shop's book.

## Combat decisions and corrections

The original attack function used Strength for every hero. The remake deliberately uses each class's primary stat, shown above, so selecting a wand, bow or knife changes combat meaningfully. Original enemy health is retained: ordinary enemies have 32 HP, guardians 144 HP. Strikes are a random base of 3–5 plus `floor(Agility / 3)`. Critical-hit probability is Accuracy out of 35, capped naturally at 100%, with double damage. Minimum damage is 1.

Enemies retaliate for 3 HP in opening battles, 4 HP in room 5 and 5 HP in guardian fights. A killing blow prevents retaliation. Ordinary victory gives an all-stat `+1` reward; running out of strikes forces a retreat and a `0.75` all-stat penalty. Sneaking skips combat and its reward, but still leads to the room's riddle.

A guardian must be defeated. Zero Health or exhausted guardian strikes ends the descent. View the results and return to the main menu to begin again. There are no room/floor retries or restart options during a run.

## Implementation

- `sfml/DungeonRun` owns the run, state transitions, random generator and stat rules. It has no SFML or Windows dependencies.
- `sfml/GameApp` owns drawing, input, paginated UTF-8 text, sound and overlays. Buttons send actions to the rules; drawing never edits player stats.
- `StartScreen.cpp` handles portable startup, explicit asset paths and useful errors.
- The Visual Studio project and CMake build the same game, run rules and local leaderboard implementation. Legacy forms are not compiled.
- A 1440×900 canvas scales with aspect-preserving margins. Mouse hit testing uses the inverse transform, including letterbox offsets.
- Timed riddles cannot be paused. Pause, map and lore controls are unavailable during a question; the countdown keeps running through focus loss and quit confirmation, using elapsed time before accepting new input. The attack delay prevents a burst of queued clicks from consuming an entire fight.
- Animation frames load on demand into an eight-texture cache. Large intro/menu sequences are not all resident in video memory.
- Completed, defeated and already-resolved states ignore incompatible input, preventing duplicate rewards and transitions past the chapter boundary.

## Verification and next chapter

800 seeded complete first-floor routes cover every hero/biome pairing, alongside 1,600 full three-floor runs. Focused checks cover chest failure/success, timeout idempotence, stat floors, unique questions, failed fights and terminal states. Optional SFML tests drive both biome routes, hero-lore highlighting, resized click coordinates and unpausable riddle timers. Screenshots are inspected for layout.

Windows compilation and audible music playback still require verification on Windows. There is no disk save/load. All three floors and the original branching endings are playable. The game starts in fullscreen and supports F11 to switch to a window.
