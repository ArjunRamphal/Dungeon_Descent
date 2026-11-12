# Dungeon Descent

A story-driven fantasy adventure about riddles, choices and the truth beneath Aethoria. Choose a hero and enter a magical dungeon to rescue a princess believed to have been captured by the sorcerer Malvel.

The **SFML remake now has a playable first floor**, from the animated menu and hero selection to the guardian battle and floor-clear summary. The original COMP315 Group 7 game has three floors; floors 2 and 3 have not yet been ported to the SFML interface.

![The first-floor guardian battle](docs/images/floor-one-combat.jpg)

## Playable in the SFML remake

- Warrior, Ranger, Mage and Rogue, with their original starting stats, automatic class abilities and reward multipliers.
- Both first-floor paths: the **Ice** and **Jungle** biomes.
- Nine rooms: three shuffled opening rooms, the prisoner, a second battle, respite, a library, Surtees' shop and the guardian.
- Eight timed riddles, drawn without replacement from all 30 original easy riddles. Answer choices are shuffled.
- Sealed chest rewards, book choices, healing, reputation and branching prisoner dialogue.
- Turn-based combat, critical hits, limited strikes, enemy health and visible battle feedback.
- A map and room progress, hero lore, paginated story text, biome music and sound toggle.
- Mouse and keyboard controls, resizable windows, fullscreen, pause and room retry.

The first floor ends after defeating the Yeti or Jungle Warden. A failed guardian fight cannot count as a clear. See the [floor-one guide](docs/FLOOR_ONE.md) for rules, deliberate combat adjustments and implementation notes.

## Build and run on Windows

1. Install Visual Studio 2022 with **Desktop development with C++**, the MSVC v143 toolset and a Windows SDK.
2. Open `Dungeon_Descent.sln`.
3. Select **Debug | x64** or **Release | x64** and build.
4. Run from Visual Studio or launch `build/x64/Release/Dungeon_Descent.exe`.

The active game is native C++17/SFML 3.0.0. It no longer requires C++/CLI, Windows Forms or the .NET Framework Developer Pack. The bundled SFML SDK is for MSVC x64.

For CMake, Linux, tests, asset paths and troubleshooting, see [development setup](docs/SETUP.md).

## Controls

| Input | Action |
| --- | --- |
| Left click | Select a button. |
| `1`–`4` | Select a displayed numbered choice. |
| `Tab`, `Shift+Tab`, Up/Down | Move between controls. |
| `Enter` / `Space` | Activate the selected control; attack when selected. |
| Left/Right, Page Up/Down, mouse wheel | Turn story pages. |
| `M` | Open/close the floor map. |
| `Esc` | Pause/resume, close an overlay, or go back. |
| `V` | Toggle music. |
| `F11` | Toggle fullscreen. |

Riddle timers stop while paused, viewing the map, reading hero lore or after losing window focus. Runs and room checkpoints are held in memory; closing the game ends the current run.

## Repository layout

| Path | Contents |
| --- | --- |
| `Dungeon_Descent.sln` | Native SFML Visual Studio solution. |
| `CMakeLists.txt` | Portable game and test build. |
| `src/DungeonDescent/StartScreen.cpp` | Entry point, asset discovery and startup errors. |
| `src/DungeonDescent/sfml/` | SFML interface and independent first-floor rules. |
| Other files in `src/DungeonDescent/` | Preserved original Windows Forms game and classes, used as references for the remaining port. They are not compiled by the active project. |
| `assets/` | Original artwork, fonts, animation frames, music and story/riddle text. |
| `tests/` | Rules tests and optional SFML render/input tests. |
| `third_party/SFML-3.0.0/` | Bundled Windows SDK and original notices. |
| `docs/` | Setup, first-floor rules and screenshots. |
| `build/` | Ignored generated output. |

## Authors

Developed as a **COMP315 Group 7** project.

- Lokadi Naicker
- Arjun Ramphal
- Darian Robert
- Ashutosh Maraj
- Zaid Kajee
- Sikhulile Dlamini
- Keolin Naicker
- Zaheera Ganie
- Nafees Abdulla

SFML and its bundled files retain their original notices. See [third-party information](third_party/README.md).
