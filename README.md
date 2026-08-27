# WorldMapSelector

WorldMapSelector is an SKSE plugin that lets players view and fast travel from
world maps other than the world they currently occupy.

For example, a player standing in Skyrim can open the Solstheim map, select a
Solstheim marker, and fast travel there without first travelling to Solstheim
by some other means.

## Features

- Discovers worldspaces with usable world-map data, including maps added by
  other mods.
- Switches the world-map scene, camera, and marker collection together.
- Displays ordinary, quest, and custom destination markers on the selected
  world map.
- Supports cross-worldspace fast travel through ordinary map markers.
- Provides an in-game map chooser using Skyrim's built-in UI, without requiring
  SkyUI or another UI framework.
- Adds a native-style `Select Map` key hint when SkyUI's compatible MapMenu
  button panel is available.
- Marks the player's current world in the chooser.
- Supports paginated map lists and duplicate-name disambiguation.
- Can open the chooser while MapMenu is already visible.
- Supports persistent or one-shot map selections.
- Requires no ESP or Papyrus scripts.

## Compatibility

Supported Steam Skyrim runtimes:

| Skyrim | SKSE |
| --- | --- |
| `1.5.97` | `2.0.20` |
| `1.6.318` | `2.1.2` |
| `1.6.323` | `2.1.3` |
| `1.6.342` | `2.1.4` |
| `1.6.353` | `2.1.5` |
| `1.6.629` | `2.2.0` |
| `1.6.640` | `2.2.3` |
| `1.6.1130` | `2.2.5` |
| `1.6.1170` | `2.2.6` |
| `1.7.99`* | `2.3.0` |
| `1.7.104` | `2.3.1` |

\* Skyrim `1.7.99` is supported by exact static comparison with the tested
`1.7.104` code, but has not been tested in game because its matching SKSE
archive was unavailable.

Install the SKSE build and Address Library database matching the exact Skyrim
runtime.

Skyrim `1.6.317` is not supported because its matching official SKSE `2.1.0`
release does not load native DLL plugins. Other `1.5.x` runtimes, Skyrim VR,
GOG, and future Skyrim updates are unsupported unless explicitly listed above.

SkyUI is optional. When its compatible MapMenu interface is loaded,
WorldMapSelector adds a `Select Map` keyboard hint to the bottom button panel.
Without that interface, the hint is skipped and every other WorldMapSelector
feature remains available. Other MapMenu interface replacements may vary.

## Requirements

- [SKSE64](https://skse.silverlock.org/), using the matching version listed
  above for the installed Skyrim runtime
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444), with the database matching the installed Skyrim runtime
- [Latest Microsoft Visual C++ Redistributable (x64)](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)

CommonLibSSE NG and MinHook are linked into the plugin; users do not install
them separately.

## Installation

Install the release archive with Mod Organizer 2 or Vortex. For a manual
installation, copy the archive's contents into Skyrim's `Data` directory.

The archive is laid out relative to `Data`:

```text
SKSE/
└── Plugins/
    ├── WorldMapSelector.dll
    ├── WorldMapSelector.ini
    └── WorldMapSelector/
        └── Licenses/
            └── ...
```

Launch Skyrim through SKSE.

## Usage

Press `F10` to open the map chooser.

- `[Clear Selection]` restores Skyrim's automatic map selection.
- The map containing the player is marked `[Here]`.
- An explicitly selected map is marked `[Selected]`.
- When both are the same, the map is marked `[Here/Selected]`.
- Selecting the map marked `[Here]` stores it as an explicit selection; this
  allows that map to remain selected after the player travels elsewhere.
- By default, selecting a map outside MapMenu opens it immediately.
- Pressing `F10` while MapMenu is open displays the chooser over the map.
- Selecting a different map while MapMenu is open closes and rebuilds MapMenu
  with the new world.

The hotkey uses a DirectInput keyboard code and can be changed in the INI.

## Configuration

The included `WorldMapSelector.ini` documents every setting and its valid
values. Its defaults are:

| Setting | Default | Purpose |
| --- | --- | --- |
| `LogLevel` | `Info` | Controls plugin log verbosity. |
| `OpenSelectorKey` | `0x44` | Sets the DirectInput keyboard code for the chooser hotkey. `0x44` is F10; `0x00` disables it. |
| `OpenMapAfterSelection` | `true` | Opens the selected map immediately when choosing outside MapMenu. |
| `PersistSelection` | `true` | Keeps the selected map after MapMenu closes. |
| `AllowChooserOutsideMap` | `true` | Allows the chooser hotkey during normal gameplay. Set it to `false` to make the hotkey map-only. |
| `AllowChooserWhileMapOpen` | `true` | Allows the chooser hotkey while MapMenu is visible. |
| `ShowMapMenuKeyHint` | `true` | Shows the configured keyboard hotkey in compatible MapMenu interfaces, including while using a controller. |
| `ShowMapMenuKeyHintOnLocalMap` | `false` | Also shows the SkyUI key hint while viewing the local map. Leave disabled to avoid conflicts with local-map interface mods. |
| `MapsPerPage` | `6` | Number of worldspaces shown per chooser page. Valid values are 1 through 7. |
| `ShowCancelButton` | `true` | Shows a visible Cancel button in the chooser. Escape, Tab, and controller Cancel inputs remain available when hidden. |
| `ShowClearSelectionButton` | `true` | Shows the option that returns map selection to Skyrim. When hidden, every chooser choice explicitly selects a map. |
| `IncludedWorldspaces` | *(empty)* | Comma-separated EditorIDs allowed in the chooser. An empty value includes every valid map. |
| `ExcludedWorldspaces` | `Falskaar` | Comma-separated EditorIDs that are omitted from the map chooser. |

Worldspace lists are case-insensitive. When an EditorID appears in both lists,
`ExcludedWorldspaces` takes priority.

See the [DirectInput keyboard code reference](https://community.bistudio.com/wiki/DIK_KeyCodes)
when choosing another hotkey.

Configuration is loaded once when the plugin starts. Restart Skyrim after
changing any INI setting.

## Known Issues

- **Falskaar is incompatible with remote map selection.** Opening its remote map
  can leave Skyrim unable to complete later world transitions. Falskaar is
  excluded by default; removing it from `ExcludedWorldspaces` is not recommended.
- Compatibility with custom replacements for the MapMenu interface needs
  broader testing.

Please include `WorldMapSelector.log`, the Skyrim runtime version, and a mod
list when reporting a problem.

The log is normally written to:

```text
Documents/My Games/Skyrim Special Edition/SKSE/WorldMapSelector.log
```

## Building

### Prerequisites

- Visual Studio 2026 with the Desktop development with C++ workload
- CMake 3.21 or newer
- Ninja
- Git
- vcpkg, with `VCPKG_ROOT` set

Clone recursively so the CommonLib submodule is present:

```powershell
git clone --recurse-submodules https://github.com/Apocrypher00/WorldMapSelector.git
cd WorldMapSelector
```

Configure and build:

```powershell
cmake --preset x64-Release
cmake --build --preset x64-Release
```

The output is written beneath:

```text
out/build/x64-Release/
```

The `x64-Release` preset produces an optimized `Release` build. Dependencies
are resolved through the checked-in `vcpkg.json` manifest using the
`x64-windows-static-md` triplet.

To build the optimized plugin and generate the complete release archive:

```powershell
cmake --preset x64-Release
cmake --build --preset x64-Release-Package
```

The Nexus-ready archive is written to the release build directory as
`WorldMapSelector-<version>.zip`. The package contains the DLL, default INI,
and required project and third-party license files in their Data-relative
locations.

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```

See [SOURCES.md](SOURCES.md) for the source corresponding to each published
release and the exact dependency pins used to reproduce it.

## Dependencies and acknowledgements

WorldMapSelector depends at runtime on SKSE64 and Address Library for SKSE
Plugins. Development is built on
[CommonLibSSE NG](https://github.com/alandtse/CommonLibSSE-NG), including the work
of the original CommonLibSSE and CommonLibSSE-NG contributors, and uses
[MinHook](https://github.com/TsudaKageyu/minhook) for whole-function detours.

The pinned build also uses the following libraries through vcpkg and
CommonLibSSE NG:

- [fmt](https://github.com/fmtlib/fmt)
- [spdlog](https://github.com/gabime/spdlog)
- [Xbyak](https://github.com/herumi/xbyak)
- [rapidcsv](https://github.com/d99kris/rapidcsv)
- [SimpleIni](https://github.com/brofield/simpleini)
- [toml11](https://github.com/ToruNiina/toml11)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [DirectXMath](https://github.com/microsoft/DirectXMath)
- [DirectX Tool Kit](https://github.com/microsoft/DirectXTK)

The project is configured and built with CMake, Ninja, vcpkg, and Microsoft
Visual C++. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for license
and attribution information.

WorldMapSelector-authored source remains available under the Unlicense. Because
CommonLibSSE NG is linked statically, distributed plugin binaries are governed by
GPL-3.0-or-later with CommonLib's exceptions. See [LICENSING.md](LICENSING.md) for
the distinction and [SOURCES.md](SOURCES.md) for corresponding source information.

## Development acknowledgement

WorldMapSelector began with the author's original pre-Anniversary Edition
prototype and reverse-engineering work. The current C++ implementation was
developed primarily in collaboration with OpenAI's ChatGPT and Codex. The
author directed the design and reverse engineering, reviewed and revised the
source, and performed the in-game testing. GitHub Copilot also assisted with
parts of the early Visual Studio and CMake setup.
