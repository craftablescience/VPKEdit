<div>
  <img align="left" width="44px" src="https://github.com/craftablescience/VPKEdit/blob/main/branding/logo.png?raw=true" alt="VPKEdit Logo" />
  <h1>VPKEdit</h1>
</div>

VPKEdit is an open source MIT-licensed tool that can extract from, preview the contents of and write to several pack file formats.
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

- Supported file formats:

  | Format | Description                                 | Create | Read | Write |
  |--------|---------------------------------------------|:------:|:----:|:-----:|
  | BMZ    | Bonus Map Zip (Source Engine)               |   ❌    |  ✅   |   ✅   |
  | BSP    | Source 1 Map                                |  N/A   |  ✅   |   ✅   |
  | FPX    | VPK modification (Tactical Intervention)    |   ❌    |  ✅   |   ✅   |
  | GCF    | Game Cache File (Pre-SteamPipe Steam games) |   ❌    |  ✅   |   ❌   |
  | GMA    | Garry's Mod Addon                           |   ❌    |  ✅   |   ✅   |
  | GRP    | Game Resource Package (Build Engine)        |   ❌    |  ✅   |   ✅   |
  | PAK    | PAcK file (Quake, original Half-Life, etc.) |   ❌    |  ✅   |   ✅   |
  | PCK    | Godot PaCK file (Standalone or embedded)    |   ❌    |  ✅   |   ✅   |
  | VPK    | Valve PacK file (Source Engine)             |   ✅    |  ✅   |   ✅   |
  | ZIP    | ZIP file (Uncompressed)                     |   ❌    |  ✅   |   ✅   |

- Preview files contained within the pack file without extracting them:
  - Audio
  - Text files (any encoding)
  - KeyValues files (text files with syntax highlighting)
  - Images
  - Source 1 Textures
  - Source 1 Models
- Create VPKs of any version from scratch or an existing folder
- Add and remove files and folders from writable pack files
- Extract files and folders from readable pack files
- Switch version of existing VPKs
- Native Linux compatibility
- User interface translated to the following languages ([support the translation effort here](https://poeditor.com/join/project/yxR9MLc9X2)):

  ![Bosnian](https://img.shields.io/poeditor/progress/683443/bs?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Chinese (simplified)](https://img.shields.io/poeditor/progress/683443/zh-Hans?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Dutch](https://img.shields.io/poeditor/progress/683443/nl?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Italian](https://img.shields.io/poeditor/progress/683443/it?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Japanese](https://img.shields.io/poeditor/progress/683443/ja?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Polish](https://img.shields.io/poeditor/progress/683443/pl?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Russian](https://img.shields.io/poeditor/progress/683443/ru?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Spanish](https://img.shields.io/poeditor/progress/683443/es?token=bd5e1015742373e9f14f1a98d76c1386)
  ![Swedish](https://img.shields.io/poeditor/progress/683443/sv?token=bd5e1015742373e9f14f1a98d76c1386)

Please read [this file](https://github.com/craftablescience/VPKEdit/blob/main/CONTROLS.md) to see all keyboard shortcuts.

A separate application with no external dependencies (`vpkeditcli`) provides a command-line interface.

## Planned Features

See [the open issues on this repository marked as an enhancement](https://github.com/craftablescience/VPKEdit/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement).

## Contributing

This project accepts a wide range of contributions, mostly code and translations. Code contributions are done through this GitHub repository.
Translations are open to everyone, only requiring a free POEditor account, and are hosted at https://poeditor.com/join/project/yxR9MLc9X2.
Any contributors will be added to the credits in the form of a text file shipped with the CLI application and a popup in the GUI application.

## Backend

This tool is powered by a collection of open-source C++20 Source engine parsers called [sourcepp](https://github.com/craftablescience/sourcepp).
