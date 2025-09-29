# Installing VPKEdit

This document covers the different ways you can install VPKEdit on its supported
platforms:

- Windows 7, 8, 8.1, 10, 11 (`x86_64`)
- macOS (`arm64`)
- Linux (`x86_64`)

## Windows

###### Automatic (Windows 10+):

VPKEdit is on the Windows package registry, so you only need to run one command to install VPKEdit or update to the latest version:
1. Press `Win + R`
2. In the popup window, enter `winget install vpkedit` into the prompt and press "Ok".

> [!WARNING]
> - The version on WinGet is usually slightly out of date. If you want to make sure you are installing the very latest stable release,
>   continue to the Manual section.
> - This will not install the standalone version. If you wish to use that, continue to the Manual section.

###### Manual:

1. You will need to install the VS2015-2022 runtime located at https://aka.ms/vs/17/release/vc_redist.x64.exe.
2. When that is installed, download either the standalone version or the installer version from [the latest
   GitHub release](https://github.com/craftablescience/VPKEdit/releases/latest), under the `Assets` dropdown.
   - Note that if you are using Windows 7, 8, or 8.1, you will need to download the "Compatibility" version of the GUI for
     it to work. The CLI will work on any OS.
3. If you downloaded the standalone files, you're done, just unzip the files. If you downloaded the installer,
   unzip the installer application and run it. When running the application Windows will give you a safety warning,
   ignore it and hit `More Info` → `Run Anyway`.

## macOS

Install VPKEdit through the DMG installer in [the latest GitHub release](https://github.com/craftablescience/VPKEdit/releases/latest),
under the `Assets` dropdown. No standalone build or `x86_64` builds are provided for this platform.

Note that since builds are not signed, you will need to run the following command in the terminal once after
installation:

```sh
xattr -rd com.apple.quarantine /path/to/installed/VPKEdit.app
```

## Linux

Installation on Linux will vary depending on your distro. On all distros you should be able to run the standalone
application without any issues.

[![Packaging Status](https://repology.org/badge/vertical-allrepos/vpkedit.svg?header=Packaging%20Status)](https://repology.org/project/vpkedit/versions)

> [!TIP]
> If you prefer or need to use a standalone version, you can run one or both of the following commands to add the standalone binaries to the PATH:
>
> ```sh
> # CLI:
> ln -s /path/to/standalone/vpkeditcli ~/.local/bin/vpkeditcli
> # GUI:
> ln -s /path/to/standalone/vpkedit ~/.local/bin/vpkedit
> ```
>
> With the symlink in place, updating your standalone install will automatically update the binaries on the PATH.

There are two ways of *installing* VPKEdit specific to the following distros:

#### Debian-based:

###### Automatic:

1. Visit https://craftablescience.info/ppa/ and follow the instructions.
2. VPKEdit should now be installable and upgradable from `apt` (the package name being `vpkedit`).

###### Manual:

1. Download the installer from the GitHub releases section, and extract the `.deb` file from inside.
2. Run `sudo apt install ./<name of deb file>.deb` in the directory you extracted it to.

#### Fedora-based:

###### Automatic:

1. Install the Terra third-party package repository at https://terra.fyralabs.com/.
2. VPKEdit should now be installable and upgradeable from `dnf` and `yum` (the package name being `vpkedit`).

###### Manual:

1. Download the installer from the GitHub releases section, and extract the `.rpm` file from inside.
2. Run `sudo dnf install ./<name of rpm file>.rpm` in the directory you extracted it to.

#### Arch-based:

VPKEdit is on the Arch User Repository thanks to [@HurricanePootis](https://github.com/HurricanePootis).
Please consult the [Arch Wiki](https://wiki.archlinux.org/title/Arch_User_Repository) for more information,
as I don't use Arch Linux personally and don't know how that system works. (I use Fedora btw)

#### NixOS:

VPKEdit is available in NixOS 25.05 and nixpkgs unstable thanks to [@Seraphim Pardee](https://github.com/SeraphimRP).
Add it to your system by placing `vpkedit` in the appropriate `environment.systemPackages = with pkgs; [];` section 
in your Nix configuration. If you want to use it temporarily, run `nix-shell -p vpkedit` then `vpkedit` (for gui) or `vpkeditcli`.
