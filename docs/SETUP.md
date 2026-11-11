# Development setup

## Requirements

Use a Windows development machine with:

- Visual Studio 2022 and the **Desktop development with C++** workload.
- The **MSVC v143** build tools and a Windows SDK.
- **C++/CLI support for v143 build tools**, available in the Visual Studio Installer's Individual components tab.
- The [.NET Framework 4.7.2 Developer Pack](https://dotnet.microsoft.com/en-us/download/dotnet-framework/net472) and Windows Forms targeting components.

The project combines native C++/SFML with managed C++/CLI Windows Forms. Microsoft documents [C++/CLI support](https://learn.microsoft.com/en-us/cpp/dotnet/dotnet-programming-with-cpp-cli-visual-cpp?view=msvc-170) as a separately installable component.

The existing C++20 project setting is retained. SFML remains at **3.0.0**; its SDK is included in `third_party/SFML-3.0.0`, so no dependency download or upgrade is part of this cleanup.

## Open and build

1. Open `Dungeon_Descent.sln` from the repository root.
2. Confirm that `Dungeon_Descent` is the startup project.
3. Select **Debug | x64** or **Release | x64**.
4. Build or rebuild the solution.
5. Run through Visual Studio or launch the generated executable.

From a Visual Studio Developer PowerShell, a Release build can also be requested with:

```powershell
msbuild Dungeon_Descent.sln /m /p:Configuration=Release /p:Platform=x64
```

The bundled SFML DLLs are x64 binaries. The legacy x86 solution entries remain, but the build gives a clear message to select x64 when using this SDK.

## Output locations

| Configuration | Executable |
| --- | --- |
| Debug x64 | `build/x64/Debug/Dungeon_Descent.exe` |
| Release x64 | `build/x64/Release/Dungeon_Descent.exe` |

Intermediate compiler files are written to `build/intermediate/x64/<Configuration>/`. The generated `build` directory is ignored by Git.

The previous checked-in `x64/Debug` and `x64/Release` folders were build snapshots. Create a fresh executable from the source using the instructions above.

## Assets and working directory

The authoritative game content lives in:

- `assets/candle/`
- `assets/fonts/`
- `assets/images/`
- `assets/music/`
- `assets/textfiles/`

The `CopyGameRuntimeFiles` MSBuild target copies these contents into the executable's output directory while preserving their folder structure. It also copies the SFML DLLs from the bundled SDK's `bin` directory. Unchanged files are skipped on subsequent copies.

At startup, the game sets its working directory to the executable directory before loading images, fonts, music or story text. This keeps the existing paths such as `music/DD.wav` and `images/loadscreen.png` valid after the source directory move.

Rebuild after changing assets if Visual Studio reports that the compiled project is already up to date. When sharing a locally built game, keep the executable, copied asset directories and runtime DLLs together.

## SFML configuration

The project uses these repository-relative properties:

| Property | Purpose |
| --- | --- |
| `SFMLRoot` | Defaults to `third_party/SFML-3.0.0` relative to the project. |
| `GameAssetsDir` | Points to the repository's `assets` directory. |
| `OutDir` | Selects the `build/<Platform>/<Configuration>` output folder. |
| `IntDir` | Separates compiler intermediates from runtime output. |

The x64 Debug configuration links the existing `-d` import libraries; Release links the existing non-debug libraries. Both use dynamic SFML linking, with runtime DLLs copied beside the executable. SFML's [Visual Studio guide](https://www.sfml-dev.org/tutorials/3.0/getting-started/visual-studio/) explains these settings.

The SDK's headers, libraries, DLLs, documentation, examples and debug symbols have been preserved. Its original notices remain with the files. See [third-party information](../third_party/README.md).

## Troubleshooting

| Problem | Check |
| --- | --- |
| C++/CLI or CLR compiler support is unavailable | Install C++/CLI support for the v143 toolset in Visual Studio Installer. |
| .NET Framework reference assemblies cannot be found | Install the 4.7.2 Developer Pack and targeting components. |
| Build asks for x64 | Select the x64 solution platform to match the bundled SDK. |
| `SFML/Graphics.hpp` or an SFML library cannot be found | Restore the complete `third_party/SFML-3.0.0` directory and keep the project-relative paths intact. |
| An SFML DLL is missing at launch | Rebuild and run from the generated output folder with the copied DLLs. |
| Missing images, music, fonts or text | Restore the `assets` directory and rebuild so the runtime files are copied. |
| Asset edits do not appear | Rebuild the project to rerun the asset copy target. |

## Repository maintenance

Commit game source, form resources and canonical assets. Keep the SFML SDK and its notices together. Leave generated executables, duplicated runtime assets, intermediate compiler output and personal Visual Studio settings out of commits.

The cleanup preserves Git history. Removing the old build snapshots from the current tree does not remove them from earlier commits or reduce the historical clone size.
