# Dungeon Descent

A story-driven fantasy adventure about riddles, choices and the truth beneath Aethoria. Choose a hero and enter a magical dungeon to rescue a princess believed to have been captured by the sorcerer Malvel.

Play through **all three connected floors**, from the animated menu and hero selection to Malvel's domain and four branching endings. The same hero, health, stats, reputation and choices carry through the entire descent.

The game starts in **fullscreen**. Press **F11** to switch between fullscreen and a resizable window.

![The first-floor guardian battle](docs/images/floor-one-combat.jpg)

## Features

- Warrior, Ranger, Mage and Rogue, with balanced 1.5x primary-stat and Health reward multipliers, starting class bonuses, a 6-stat hero selection breakdown, and unique per-battle tactical abilities.
- Five biomes: **Ice / Jungle** on floor 1, **Desert / Ghost** on floor 2 and **Lava** on floor 3.
- Twenty-seven rooms, with shuffled openings, battles, libraries, shops, respite, encounters, guardians and Malvel. Choice buttons show the narrative choice without spoiling exact stat rewards.
- Eight timed riddles per floor, drawn without replacement from the original 30 easy, 37 medium and 35 hard riddles. No new riddles were added. Answer choices are shuffled; high Observation eliminates one incorrect option.
- Sealed chest rewards, the second-floor vision, the princess's diary, the lava orb, healing, reputation and branching encounter dialogue.
- Turn-based tactical combat with Standard Strike, Heavy Strike, Guard Strike, and class-specific active abilities, pre-battle combat math forecasts, critical hits, limited strikes, enemy health, synthesized sound effects, and visual combat/stat-change feedback.
- **Easy, Medium, Hard and Impossible** combat difficulties. Choose after your hero; the level is fixed for the descent. Easy preserves the previous enemy stats. Higher levels change enemy health and counterattack damage, with separate profiles for opening fights, later fights and guardians.
- **Custom Seeds & Daily Runs**: Play with a random seed, enter a custom seed on the biome selection screen, or challenge the deterministic Daily Run seed. The active seed is displayed in the pause menu and journey results.
- **Save & Quit (Suspend Run)**: Suspend an active descent between timed riddles to a single-slot atomic save file and resume later from the main menu.
- The original narrated lore intro over the candle animation, followed by the animated main menu. The voice-over plays once above the background music; any key or click skips to the menu.
- A floor map and room progress, HUD stat tooltips (`I` or mouse hover), hero lore, paginated story text, biome music, procedural combat/UI SFX, and sound toggle.
- The original good, true and bad endings, plus **A Crown in Ashes**, a fourth ending for a perfect run: win all **9 battles**, solve all **24 riddles** and earn **9 reputation**. Each ending has a full story, artwork and journey results.
- **Local leaderboards for Easy, Medium, Hard and Impossible.** All submitted records are saved and displayed 10 per page with `<` and `>` page navigation. Browse high scores directly from the main menu, view them in read-only mode after a game over, or enter three arcade-style initials after any ending. Boards persist between launches and rebuilds.
- Mouse and keyboard controls, fullscreen startup, resizable windows and pause between riddles.

Defeat the Yeti or Jungle Warden to unlock the second-floor door choice. Defeat Ahmose the Pharaoh or Count Dracula to enter Malvel's domain. Floor transitions continue the current descent; starting again is available only after an ending or game over. See the [floor-one](docs/FLOOR_ONE.md), [floor-two](docs/FLOOR_TWO.md) and [floor-three](docs/FLOOR_THREE.md) guides for routes and rewards.

Difficulty leaves hero stats, rewards, riddles, timers and ending conditions unchanged. Enemy health, counterattack damage, and your combat forecast are shown before a fight. See the [combat balance guide](docs/COMBAT_BALANCE.md) for the stat tables, simulation results and reproduction commands.

![Combat difficulty selection](docs/images/difficulty-selection.jpg)

![The second-floor guardian](docs/images/floor-two-desert.jpg)

![Malvel in the final lava chamber](docs/images/floor-three-lava.jpg)

## Build and run on Windows

1. Install Visual Studio 2022 with **Desktop development with C++**, the MSVC v143 toolset and a Windows SDK.
2. Open `Dungeon_Descent.sln`.
3. Select **Standalone | x64** and build to make a single executable. The first build requires Git, CMake 3.24+ (or Visual Studio's **C++ CMake tools for Windows**) and an internet connection to compile the pinned dependencies.
4. Run with **Ctrl+F5**, or share just **`dist/Dungeon_Descent.exe`**.

Standalone includes SFML, the Visual C++ runtime, images, fonts, music and story files. No additional DLLs or asset folders need to accompany the EXE. On first launch it prepares a private asset cache under `%LOCALAPPDATA%\DungeonDescent\assets`; later launches verify and reuse it. See [standalone builds](docs/STANDALONE.md) for details and the PowerShell build command.

For faster development builds, **Debug | x64** and **Release | x64** still use the bundled SDK and copy assets and DLLs beside the game in `build/x64/<configuration>/`. Keep that entire output together when running those builds.

The active game is native C++17/SFML 3.0.0. The legacy C++/CLI Windows Forms sources have been removed; only the native SFML game and portable engine remain. The bundled SFML SDK is for MSVC x64.

For CMake, Linux, tests, asset paths and troubleshooting, see [development setup](docs/SETUP.md).

## Controls

| Input | Action |
| --- | --- |
| Left click | Select a button or stat tooltip target. |
| `1`–`4` | Select a displayed numbered choice or tactical combat action. |
| `Tab`, `Shift+Tab`, Up/Down | Move between controls. |
| `Enter` / `Space` | Activate the selected control; quick Standard Strike in combat. |
| Left/Right, Page Up/Down, mouse wheel | Turn story pages or leaderboard pages. |
| `I` | Cycle through hero stat tooltips on the HUD. |
| `M` | Open/close the floor map between timed riddles. |
| `Esc` | Pause/resume between timed riddles, close an overlay, or go back. |
| `V` | Mute/unmute music, sound effects and intro narration. |
| `F11` | Toggle fullscreen. |

Timed riddles cannot be paused. Pause, save & quit, map and hero-lore controls are disabled while the countdown runs; switching windows or opening the quit confirmation does not stop it. Between timed riddles, open the pause menu (`Esc`) to **Save & quit** and suspend your run to disk, then resume from the main menu later.

After reading an ending, **View your journey** (or `Esc`) opens the matching difficulty leaderboard alongside your final results. Type **three letters**, use **Backspace** to edit and **Enter** to save, or skip entry with `Esc`. You can also browse all four difficulty boards anytime from **High scores** on the main menu or in read-only mode after a game over. See [leaderboards](docs/LEADERBOARDS.md) for storage and ranking details.

## Repository layout

| Path | Contents |
| --- | --- |
| `Dungeon_Descent.sln` | Native SFML Visual Studio solution. |
| `CMakeLists.txt` | Portable game and test build. |
| `src/DungeonDescent/StartScreen.cpp` | Entry point, asset discovery and startup errors. |
| `src/DungeonDescent/sfml/` | SFML interface, three-floor run rules (`DungeonRun`) and local leaderboards (`Leaderboards`). |
| `src/DungeonDescent/portable/` | Embedded assets (`AssetBundle`), OS file locking (`FileLock`) and atomic suspend-save persistence (`SuspendSave`). |
| `assets/` | Original artwork, fonts, animation frames, music and story/riddle text. |
| `tests/` | Rules, balance, suspend-save, asset-bundle, leaderboard and optional SFML render/input tests. |
| `tools/CombatBalance.cpp` | Reproducible simulation of the real combat/run rules. |
| `tools/Build-Standalone.ps1` | Windows single-executable build and dependency checks. |
| `third_party/SFML-3.0.0/` | Bundled Windows SDK and original notices. |
| `docs/` | Setup, routes and rules for each floor, and screenshots. |
| `build/` | Ignored generated output. |
| `dist/` | Ignored standalone executable, ready to share. |

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
