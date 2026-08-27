# Release Sources

This document identifies the source and dependency revisions corresponding to
published WorldMapSelector binaries. It supplements the normal build instructions
without changing the license that applied to any release.

## Published releases

| Release | Source tag | CommonLibSSE NG | vcpkg baseline |
| --- | --- | --- | --- |
| 1.0.0 | `v1.0.0` | `9ea71c54883e65fcafeda3040ccc2bc02a5c9cc6` | `ee12231b20c95013c6638d845d04c91559a1d1ff` |
| 1.1.0 | `v1.1.0` | `9ea71c54883e65fcafeda3040ccc2bc02a5c9cc6` | `ee12231b20c95013c6638d845d04c91559a1d1ff` |
| 1.2.0 | `v1.2.0` | `9ea71c54883e65fcafeda3040ccc2bc02a5c9cc6` | `ee12231b20c95013c6638d845d04c91559a1d1ff` |
| 1.3.0 | `v1.3.0` | `9ea71c54883e65fcafeda3040ccc2bc02a5c9cc6` | `ee12231b20c95013c6638d845d04c91559a1d1ff` |
| 1.3.1 | `v1.3.1` | `9ea71c54883e65fcafeda3040ccc2bc02a5c9cc6` | `ee12231b20c95013c6638d845d04c91559a1d1ff` |

These releases use CommonLibSSE NG 4.39.0 under its MIT license. WorldMapSelector's
own source is released under the Unlicense. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for the other licenses and acknowledgements applicable to these builds.

## Retrieve an exact release

Clone the repository, check out the desired release tag, and initialize its pinned
submodules:

```powershell
git clone https://github.com/Apocrypher00/WorldMapSelector.git
cd WorldMapSelector
git checkout vX.Y.Z
git submodule update --init --recursive
```

Replace `vX.Y.Z` with the desired tag from the table.
The tag fixes the WorldMapSelector source and build files. Its Git tree also records
the exact CommonLibSSE NG submodule commit.

GitHub's automatically generated source archives do not contain the contents of Git
submodules. A recursive clone or the explicit `git submodule update` command is
therefore required to retrieve the complete CommonLibSSE NG source used by the build.

## Restore vcpkg dependencies

The checked-in `vcpkg.json` manifest records a builtin baseline. That baseline fixes
the vcpkg port definitions used to resolve MinHook, SimpleIni, and the other linked or
header-only dependencies. CMake restores those dependencies through vcpkg when the
project is configured:

```powershell
cmake --preset x64-Release
cmake --build --preset x64-Release
```

The project uses the `x64-windows-static-md` triplet. `VCPKG_ROOT` must point to a
working vcpkg checkout, as described in the main README.

The baseline identifies the selected port revisions and their upstream source hashes.
The resulting package sources and license files are populated through vcpkg during
dependency restoration; generated build directories and compiled packages are not
stored in this repository.

## What is and is not part of the source

The corresponding project and dependency source consists of:

- The WorldMapSelector release tag.
- The CommonLibSSE NG revision recorded by that tag's submodule entry.
- The dependency port revisions selected by that tag's `vcpkg.json` baseline.
- The checked-in CMake presets, CMake project file, configuration, and packaging rules.

Skyrim, SKSE, Address Library, Windows, Visual Studio, CMake, Ninja, Git, and vcpkg are
runtime requirements, development tools, or independently distributed software. They
are not copied into the WorldMapSelector source repository.

## Future dependency changes

A future release may use a different CommonLibSSE NG revision, vcpkg baseline, or
license. Its release tag and a new entry in the table above will identify that boundary.
Historical releases retain the source and licensing recorded for their own tags.
