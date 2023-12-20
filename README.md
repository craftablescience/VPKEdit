<div>
  <img align="left" width="44px" src="https://github.com/craftablescience/VPKEdit/blob/main/src/gui/res/icon-128.png?raw=true" alt="VPKEdit Logo" />
  <h1>VPKEdit</h1>
</div>

VPKEdit is an open source MIT-licensed tool that can create, extract from, preview the contents of and write to VPK archives.

<img src="https://github.com/craftablescience/VPKEdit/blob/main/branding/readme_promo.png?raw=true" alt="A screenshot of VPKEdit with a VTF preview open." />

## Installing

Please read [this file](https://github.com/craftablescience/VPKEdit/blob/main/INSTALL.md) for step-by-step installation instructions.

## Features

- Create VPKs of any version from scratch or an existing folder
- Add and remove files and folders from VPKs
- Extract files and folders from VPKs
- Switch version of existing VPKs
- Native Linux compatibility

It can also preview certain file types without needing to extract them from the VPK:
- Text files
- KeyValues files (with basic syntax highlighting)
- Images
- Source 1 Textures
- Source 1 Models

## Planned Features

- CLI interface
- Preview audio files
- Add more features to the model preview
- Edit VPK file data such as archive index and preload byte count without extracting and re-packing the file
- Rename packed files/folders
- Automatically break up large VPKs

## Backend

This tool is powered by an open-source vpk editing library, libvpkedit. This library's code is stored in this same repository,
written in C++17 and also under the MIT license. Its code was initially based off of https://github.com/SteamDatabase/ValvePak
and the Valve Developer Wiki.
