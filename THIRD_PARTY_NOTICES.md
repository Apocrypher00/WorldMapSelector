# Third-Party Notices

WorldMapSelector itself is released under the Unlicense. That license does not
replace the licenses of the third-party projects used to build or run it.

## Runtime requirements (not redistributed)

| Project | Purpose |
| --- | --- |
| [SKSE64](https://skse.silverlock.org/) | Loads and provides runtime services to the plugin. |
| [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444) | Resolves version-specific Skyrim executable addresses. |

## Linked and build dependencies

| Project | License | Copyright / attribution |
| --- | --- | --- |
| [CommonLibSSE NG](https://github.com/alandtse/CommonLibVR) | MIT | Copyright (c) 2018 Ryan-rsm-McKenzie and subsequent contributors. Includes work from CommonLibSSE and CommonLibSSE-NG. |
| [MinHook](https://github.com/TsudaKageyu/minhook) | 2-clause BSD | Copyright (c) 2009-2017 Tsuda Kageyu. Includes Hacker Disassembler Engine portions by Vyacheslav Patkov. |
| [fmt](https://github.com/fmtlib/fmt) | MIT | Copyright (c) 2012-present Victor Zverovich and fmt contributors. |
| [spdlog](https://github.com/gabime/spdlog) | MIT | Copyright (c) 2016 Gabi Melman. |
| [Xbyak](https://github.com/herumi/xbyak) | 3-clause BSD | Copyright (c) 2007 MITSUNARI Shigeo. |
| [rapidcsv](https://github.com/d99kris/rapidcsv) | 3-clause BSD | Copyright (c) 2017 Kristofer Berggren. |
| [SimpleIni](https://github.com/brofield/simpleini) | MIT | Copyright (c) 2006-2024 Brodie Thiesfield. |
| [toml11](https://github.com/ToruNiina/toml11) | MIT | Copyright (c) 2017 Toru Niina. |
| [JSON for Modern C++](https://github.com/nlohmann/json) | MIT | Copyright (c) 2013-2025 Niels Lohmann. |
| [DirectXMath](https://github.com/microsoft/DirectXMath) | MIT | Copyright (c) Microsoft Corporation. |
| [DirectX Tool Kit](https://github.com/microsoft/DirectXTK) | MIT | Copyright (c) Microsoft Corporation. |

The exact dependency revisions used by a build are pinned by `vcpkg.json`, its
builtin baseline, and the CommonLibSSE NG Git submodule commit.

## License terms

The complete license text for CommonLibSSE NG is present in its checked-in Git
submodule at `extern/CommonLibVR/LICENSE`. Exact license texts for vcpkg
dependencies are installed under each package's `share/<package>/copyright`
path during the build.

Binary release archives must include the applicable copyright and license
texts for all linked third-party components. In particular, the MinHook and
BSD-licensed components require their notices and disclaimers to accompany
binary redistributions. Before packaging a public binary release, copy the
copyright files from the resolved vcpkg installation into the release's
license-notices directory.

Build tools such as CMake, Ninja, vcpkg, Git, and Microsoft Visual C++ are not
redistributed as part of WorldMapSelector.
