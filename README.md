<div>
  <img align="left" width="44px" src="https://github.com/craftablescience/VPKEdit/blob/main/src/gui/res/icon-128.png?raw=true" alt="VPKEdit Logo" />
  <h1>VPKEdit</h1>
</div>

VPKEdit is an open source MIT-licensed tool that can extract from, preview the contents of and write to several pack file formats, including VPK, ZIP, and BSP.
It also supports creating new VPKs.

<div>
  <a href="https://github.com/craftablescience/VPKEdit/blob/main/LICENSE" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/github/license/craftablescience/VPKEdit?label=license" alt="License" /></a>
  <a href="https://github.com/craftablescience/VPKEdit/actions" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/github/actions/workflow/status/craftablescience/VPKEdit/build.yml?branch=main&label=builds" alt="Workflow Status" /></a>
  <a href="https://discord.gg/ASgHFkX" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/discord/678074864346857482?label=discord&logo=Discord&logoColor=%23FFFFFF" alt="Discord" /></a>
  <a href="https://ko-fi.com/craftablescience" target="_blank" rel="noopener noreferrer"><img src="https://img.shields.io/badge/donate-006dae?label=ko-fi&logo=ko-fi" alt="Ko-Fi" /></a>
</div>

<div>
  <img width="400px" src="https://github.com/craftablescience/VPKEdit/blob/main/branding/screenshot1.png?raw=true" alt="A screenshot of VPKEdit with a VPK and an MDL preview open in wireframe mode." />
  <img width="400px" src="https://github.com/craftablescience/VPKEdit/blob/main/branding/screenshot2.png?raw=true" alt="A screenshot of VPKEdit with a VPK and an MDL preview open in shaded textured mode." />
</div>
<div>
  <img width="400px" src="https://github.com/craftablescience/VPKEdit/blob/main/branding/screenshot3.png?raw=true" alt="A screenshot of VPKEdit with a BSP and a VTF preview open." />
  <img width="400px" src="https://github.com/craftablescience/VPKEdit/blob/main/branding/screenshot4.png?raw=true" alt="A screenshot of the command-line version of VPKEdit." />
</div>

## Installing

Please read [this file](https://github.com/craftablescience/VPKEdit/blob/main/INSTALL.md) for step-by-step installation instructions.

## Features

- Create VPKs of any version from scratch or an existing folder
- Add and remove files and folders from VPKs, basic ZIP files, BSP paklumps
- Extract files and folders from VPKs, basic ZIP files, BSP paklumps
- Switch version of existing VPKs
- Native Linux compatibility

It can also preview certain file types without needing to extract them:
- Text files
- KeyValues files (with basic syntax highlighting)
- Images
- Source 1 Textures
- Source 1 Models

Please read [this file](https://github.com/craftablescience/VPKEdit/blob/main/CONTROLS.md) to see all keyboard shortcuts.

A separate application with no external dependencies (`vpkeditcli`) provides a command-line interface.

## Planned Features

See [the open issues on this repository marked as an enhancement](https://github.com/craftablescience/VPKEdit/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement).

## Contributing

This project accepts a wide range of contributions, mostly code and translations. Code contributions are done through this GitHub repository.
Translations are open to everyone, only requiring a free POEditor account, and are hosted at https://poeditor.com/join/project/yxR9MLc9X2.
Any contributors will be added to the credits in the form of a text file shipped with the CLI application and a popup in the GUI application.

## Backend

This tool is powered by an open-source pack file editing library, `libvpkedit`. This library's code is stored in this same repository,
written in C++20 and also under the MIT license. Its code was initially based off of [ValvePak](https://github.com/SteamDatabase/ValvePak)
and the Valve Developer Wiki (see [the credits](https://github.com/craftablescience/VPKEdit/blob/main/CREDITS.md) for more information).

A C wrapper library, `libvpkeditc`, is also present in this repository, and its compilation can be enabled in the CMake options. It translates the C++ library
into a C interface with largely minimal to no overhead.
