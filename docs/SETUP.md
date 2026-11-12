# Development setup

## Windows / existing Visual Studio solution

Install Visual Studio 2022 with **Desktop development with C++**, MSVC v143 and a Windows SDK. Open `Dungeon_Descent.sln`, select **x64**, build and run. The bundled SFML 3.0.0 SDK supplies the headers, import libraries and runtime DLLs. Its x86 configuration is unsupported.

To distribute **one EXE**, select **Standalone | x64**. This compiles static dependencies and embeds the assets into `dist/Dungeon_Descent.exe`. The first build also requires Git, CMake 3.24+ (available through **C++ CMake tools for Windows**) and internet access. See [single-executable builds](STANDALONE.md).

The game starts in fullscreen; press **F11** to switch to a resizable window.

Choose a combat difficulty after your hero. Easy preserves the previous version; Medium, Hard and Impossible change enemy Health and counterattack damage. The level stays fixed across all floors. See [combat balance](COMBAT_BALANCE.md) for the exact stats, measured results and optional simulator.

The active project is native C++ and compiles `StartScreen.cpp`, `sfml/GameApp.cpp`, `sfml/DungeonRun.cpp` and `sfml/Leaderboards.cpp`. Standalone also builds the portable asset helpers. C++/CLI, .NET and Windows Forms are not required. The original forms and classes remain in source as references.

From Developer PowerShell:

```powershell
msbuild Dungeon_Descent.sln /m /p:Configuration=Release /p:Platform=x64
.\build\x64\Release\Dungeon_Descent.exe
```

| Configuration | Output |
| --- | --- |
| Debug x64 | `build/x64/Debug/Dungeon_Descent.exe` |
| Release x64 | `build/x64/Release/Dungeon_Descent.exe` |
| Standalone x64 | `dist/Dungeon_Descent.exe` |

For Debug and Release, MSBuild copies the original runtime folders (`fonts`, `images`, `music`, `textfiles`, `candle`) and SFML DLLs beside the executable. Keep these together when sharing those builds. Standalone needs only its EXE; it prepares embedded assets in the user's cache. Rebuild after asset changes.

### Visual Studio launch settings

The x64 project explicitly selects the Local Windows Debugger, starts `$(TargetPath)`, uses `$(TargetDir)` as its working directory, and selects the native debugger. These settings are evaluated after the C++ targets define the output paths.

Standalone overrides the launch command with `$(NMakeOutput)`, pointing to `dist/Dungeon_Descent.exe`, and uses `dist` as the working directory. Its makefile configuration invokes the PowerShell/CMake build through Visual Studio's normal Build command.

If Visual Studio displays **Unable to start debugging. Check your debugger settings**, first confirm that this solution is open from the updated checkout. In Solution Explorer, right-click the **Dungeon_Descent project** and select **Set as Startup Project**. Select **Release | x64**, then use **Build > Rebuild Solution** and check the Build output for errors.

You can inspect the launch profile under the project's **Properties > Configuration Properties > Debugging**:

| Setting | Value |
| --- | --- |
| Debugger to launch | Local Windows Debugger |
| Command | `$(TargetPath)` |
| Working Directory | `$(TargetDir)` |
| Debugger Type | Native Only |
| Attach | No |

These are [Visual Studio's C++ launch settings](https://learn.microsoft.com/en-us/visualstudio/debugger/project-settings-for-a-cpp-debug-configuration?view=vs-2022). After a successful Release x64 build, the game can also be started directly at `build/x64/Release/Dungeon_Descent.exe`. If no executable was produced, resolve the first Build output error before trying to launch again.

## CMake / Windows

CMake 3.24 or later can use the same bundled MSVC x64 SDK:

```powershell
cmake -S . -B build/cmake -A x64
cmake --build build/cmake --config Release
ctest --test-dir build/cmake -C Release --output-on-failure
.\build\cmake\Release\Dungeon_Descent.exe
```

CMake copies assets into an `assets` folder beside the executable and copies the required runtime DLLs. Use `-DDD_FETCH_SFML=ON` to build the pinned SFML 3.0.0 source instead, for example with another supported compiler. This needs Git and network access for SFML's dependencies.

## CMake / Linux

The bundled DLLs and `.lib` files are Windows binaries. On Debian/Ubuntu, install build tools and SFML's development dependencies:

```sh
sudo apt install build-essential cmake ninja-build git \
  libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
  libfreetype-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DDD_FETCH_SFML=ON
cmake --build build/linux
ctest --test-dir build/linux --output-on-failure
./build/linux/Dungeon_Descent
```

Alternatively, use an installed SFML 3.x package and pass `-DSFML_DIR=/path/to/lib/cmake/SFML`. The game targets the 3.0.0 API. See SFML's [CMake guide](https://www.sfml-dev.org/tutorials/3.0/getting-started/cmake/) for platform prerequisites.

`DD_COPY_ASSETS=OFF` avoids copying assets during local development. Asset discovery can find the repository's `assets` directory when the executable is built below the repository root.

## Assets and launch options

No working-directory change is required. Startup searches for an `assets` folder beside the executable, the MSBuild runtime layout, and nearby repository asset folders. Paths use `std::filesystem`, including Windows Unicode executable paths.

```sh
Dungeon_Descent --assets /path/to/assets
Dungeon_Descent --skip-intro
Dungeon_Descent --no-audio
Dungeon_Descent --smoke-test --screenshot DungeonDescent.png
```

`--smoke-test` opens a first-floor scene in fullscreen without audio, renders five frames, optionally saves its canvas and exits. It requires a graphical display. Missing required images, fonts, story text or riddle data produces a startup/error message and `DungeonDescent-error.log`; optional music failures do not end gameplay.

Normal startup plays the original `music/intro.wav` lore narration once over the looping `DD.wav` soundtrack and candle animation. Narration uses the original 60% volume, above the 25% background music. Any key or click skips to the menu and stops narration; `V` mutes/unmutes both streams and `F11` changes the window mode without skipping or restarting the voice-over. `--skip-intro` opens the menu directly, and `--no-audio` keeps the visual intro without audio playback. The standalone EXE includes the original recording in its embedded assets.

Completed endings open a local leaderboard beside the final results. Each difficulty retains its own top ten using the displayed overall score and three-letter initials. Scores are stored at `%LOCALAPPDATA%\DungeonDescent\leaderboards.tsv` on Windows, independently of the EXE and asset cache. `--data-dir <folder>` or `DUNGEON_DESCENT_DATA_DIR` selects another profile. See [leaderboard controls and persistence](LEADERBOARDS.md).

## Tests

Rules tests need only a C++17 compiler and CMake, without SFML or a display:

```sh
cmake -S . -B build/rules -DDD_BUILD_GAME=OFF
cmake --build build/rules
ctest --test-dir build/rules --output-on-failure
```

They exercise 800 first-floor regression routes and 1,600 complete three-floor routes covering every hero and Ice/Jungle-to-Desert/Ghost-to-Lava pairing, plus earned perfect endings for every pairing. Checks cover all four endings, exact thresholds and near misses, carryover, opening-room permutations, assets, 24 riddles without repetition within each floor, scaled rewards and penalties, chest/diary/minion story gating, Observation hints, descent guards and terminal states. This checks a successful strategy; it does not promise victory for every choice or random strike roll.

Enable the optional render/input checks with `-DDD_TEST_GRAPHICS=ON` in a game build. Run CTest with a graphical display, or `xvfb-run -a ctest --test-dir build/linux --output-on-failure` on headless Linux. They traverse Ice–Desert–Lava and Jungle–Ghost–Lava with the real SFML renderer and event handler, checking fullscreen/F11, continuous-run navigation, door transitions, carryover, scaled clicks, keyboard choices, lore-tab highlights, hints, maps, story/ending pagination, final results and countdowns through focus loss and quit confirmation. Other ending and game-over screens are also rendered. Screenshots are saved under `ui-captures` in the build directory.

The combat difficulty checks add 16,384 seeded attempts across all four levels. They verify enemy profiles and actual damage, unchanged non-combat rules, floor carryover and increasing challenge. Render/input tests also select every level and check the locked mode, battle previews and difficulty-labelled results.

Intro checks exercise the actual narration/background streams, mute/unmute, focus and F11 changes, completion without looping, keyboard/mouse skip, quit confirmation, and the silent/skip launch options. Linux CTest uses an empty ALSA configuration for the render tests so headless runners can use SFML's null playback backend. These checks validate playback state and transitions; speaker/headphone output still needs a listening check on the target computer.

Validation during this implementation: GCC 14 / SFML 3.0.0 build, all rules tests and the SFML render/input tests passed on Linux with software OpenGL. The Windows MSVC project paths and XML were checked; Windows compilation and audible playback still require verification on Windows.

## Troubleshooting

| Problem | Check |
| --- | --- |
| Build asks for x64 | Select the x64 platform for the bundled SDK. |
| A header, import library or DLL is missing | Restore `third_party/SFML-3.0.0` and rebuild. |
| Want to share only the EXE | Build **Standalone x64** and share `dist/Dungeon_Descent.exe`. |
| Standalone cannot download dependencies | Check Git and internet access; the first build fetches the pinned SFML sources. |
| Standalone cannot prepare assets | Ensure `%LOCALAPPDATA%` is writable and has about 280 MiB free, or set `DUNGEON_DESCENT_CACHE` to another writable directory. |
| Assets cannot be located | Keep the copied runtime folders together or supply `--assets`. |
| Asset edits do not appear | Rebuild to rerun the copy step. |
| No music | Press `V`, check system volume, and check the original `music` folder. Use `--no-audio` for environments without an audio device. |
| Pause/map/lore is unavailable during a riddle | This is intentional: timed questions cannot be paused, including by switching windows or opening quit confirmation. |
| Display cannot be opened on Linux | Run in a desktop session or use Xvfb for verification. |
| Visual Studio cannot start debugging | Set the game as the startup project and check the native launch settings above; confirm that Rebuild Solution succeeds. |
| Error during launch | Read the message and `DungeonDescent-error.log`. |

Build output, logs and personal Visual Studio settings should stay out of Git. The bundled SDK and historical assets retain their notices.
