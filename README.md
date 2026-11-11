# Dungeon Descent

A story-driven fantasy adventure combining riddles, character progression and dungeon battles. Choose a hero and descend into a magical dungeon to rescue a princess believed to have been captured by the sorcerer Malvel.

Riddles and choices shape the hero's stats and the unfolding story. Explore the dungeon's history, confront its bosses and discover one of three endings.

Developed as a **COMP315 Group 7** project.

## Features

- Four playable heroes: Warrior, Ranger, Mage and Rogue.
- Five dungeon biomes: ice, jungle, desert, ghost and lava.
- Riddles, battles, chests, libraries, shops, encounters and respite rooms.
- Character stats, abilities and branching story outcomes.
- An animated SFML introduction and menu, with Windows Forms game screens.
- Biome music, character artwork and story text.

## Technology

- C++ and C++/CLI
- Windows Forms on .NET Framework 4.7.2
- SFML 3.0.0 for graphics, windowing and audio
- Visual Studio 2022 with the MSVC v143 toolset

## Repository layout

| Path | Contents |
| --- | --- |
| `Dungeon_Descent.sln` | Visual Studio solution. |
| `src/DungeonDescent/` | Game classes, forms, resources and C++ project. |
| `assets/` | Canonical images, fonts, music, story text and animation frames. |
| `third_party/SFML-3.0.0/` | The bundled SFML SDK, including headers, libraries and runtime DLLs. |
| `docs/SETUP.md` | Development setup, build instructions and troubleshooting. |
| `build/` | Generated executable, runtime assets and intermediate files; ignored by Git. |

Source assets are stored once. Builds copy the asset folders and SFML runtime DLLs beside the executable. The game resolves relative asset paths from its executable directory.

## Build and run

1. On Windows, install Visual Studio 2022 with the C++ desktop tools, C++/CLI support for v143, a Windows SDK and the .NET Framework 4.7.2 Developer Pack.
2. Open `Dungeon_Descent.sln`.
3. Select **Debug | x64** or **Release | x64** and build the solution.
4. Start the game from Visual Studio or run `build/x64/Release/Dungeon_Descent.exe` after a Release build.

See the [setup guide](docs/SETUP.md) for full instructions. The bundled SFML SDK is x64; x86 is not a supported build configuration for this bundle.

## Authors

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
