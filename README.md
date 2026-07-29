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
- Supports cross-worldspace fast travel through ordinary map markers.
- Provides a dependency-free in-game map chooser using Skyrim's built-in UI.
- Marks the player's current world in the chooser.
- Supports paginated map lists and duplicate-name disambiguation.
- Can open the chooser while MapMenu is already visible.
- Supports persistent or one-shot map selections.
- Requires no ESP, Papyrus scripts, or SkyUI.

## Compatibility

The currently validated environment is:

- Skyrim Special Edition on Steam
- Runtime `1.6.1170`
- SKSE `2.2.6`
- Address Library database for runtime `1.6.1170`

The project currently builds for the Anniversary Edition runtime family.
Pre-1.6 Special Edition runtimes, Skyrim VR, GOG, and other executable versions
have not been tested and should not be assumed compatible.

## Requirements

- [SKSE64](https://skse.silverlock.org/) matching the installed Skyrim runtime
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- Microsoft Visual C++ 2015-2022 Redistributable (x64)

## Installation

Install a release archive with Mod Organizer 2 or Vortex, or copy the packaged
files into the following locations:

```text
Data/
└── SKSE/
    └── Plugins/
        ├── WorldMapSelector.dll
        └── WorldMapSelector.ini
```

Launch Skyrim through SKSE.

## Usage

Press `F10` to open the map chooser.

- `Default` restores Skyrim's normal map selection.
- The current map is marked `[Current]`.
- Selecting a map can open it immediately.
- Pressing `F10` while MapMenu is open displays the chooser over the map.
- Selecting a different map while MapMenu is open closes and rebuilds MapMenu
  with the new world.

The hotkey uses a DirectInput keyboard scan code and can be changed in the INI.

## Configuration

`WorldMapSelector.ini`:

```ini
[General]
; Valid levels: Trace, Debug, Info, Warn, Error, Critical, Off
LogLevel=Info

[Controls]
; DirectInput keyboard scan code. 0x44 is F10.
; Set to 0 to disable the built-in chooser hotkey.
OpenSelectorKey=0x44

[Behavior]
; Open the selected map immediately when choosing outside MapMenu.
OpenMapAfterSelection=true

; Keep the selected remote map after MapMenu closes.
; When false, the selection applies to one map session.
PersistSelection=true

; Permit the chooser while MapMenu is already visible.
AllowChooserWhileMapOpen=true
```

Behavior settings reload whenever the chooser opens. Changing
`OpenSelectorKey` requires restarting Skyrim.

## Known Issues

- Custom map destinations can be created, moved, and removed on a remote map,
  but their display does not consistently refresh while that map remains open.
- Only Steam runtime `1.6.1170` has received substantial testing so far.
- Compatibility with heavily modified map interfaces needs broader testing.

Please include `WorldMapSelector.log`, the Skyrim runtime version, and a mod
list when reporting a problem.

The log is normally written to:

```text
Documents/My Games/Skyrim Special Edition/SKSE/WorldMapSelector.log
```

## Building

### Prerequisites

- Visual Studio with the x64 C++ desktop toolchain
- CMake 3.21 or newer
- Ninja
- Git
- vcpkg, with `VCPKG_ROOT` set

Clone recursively so the CommonLib submodule is present:

```powershell
git clone --recurse-submodules <repository-url>
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

The `x64-Release` preset currently uses `RelWithDebInfo`. Dependencies are
resolved through the checked-in `vcpkg.json` manifest using the
`x64-windows-static-md` triplet.

If the repository was cloned without submodules:

```powershell
git submodule update --init --recursive
```
