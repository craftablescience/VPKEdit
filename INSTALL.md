# Installing VPKEdit

This document covers the different ways you can install VPKEdit on its supported
platforms:

- Windows 10/11 (64-bit)
- Linux (64-bit)

## Windows

#### Automatic

VPKEdit is on the Windows package registry, meaning you only need to run one command to install or update to the latest version of VPKEdit:
1. Press `Win + R`
2. In the popup window, enter `winget install vpkedit` into the prompt and press "Ok".

Note that this will not install the standalone version. If you wish to use that, follow the next section instead.

#### Manual

If you'd prefer not to use WinGet, VPKEdit is relatively easy to install manually.
1. You will need to install the VS2015-2022 runtime located at https://aka.ms/vs/16/release/vc_redist.x64.exe.
2. When that is installed, download either the standalone version or the installer version from [the latest
   GitHub release](https://github.com/craftablescience/VPKEdit/releases/latest), under the `Assets` dropdown.
3. If you downloaded the standalone files, you're done, just unzip the files. If you downloaded the installer,
   unzip the installer application and run it. When running the application Windows will give you a safety warning,
   ignore it and hit `More Info` → `Run Anyway`.

## Linux

Installation on Linux will vary depending on your distro. On all distros you should be able to run the standalone
application without any issues.

> [!TIP]
> The installer will symlink the CLI and GUI applications to `/usr/bin` automatically,
> but if you prefer or need to use a standalone version, you can run one or both of the following commands to add them to the PATH:
>
> ```sh
> # CLI:
> ln -s /path/to/standalone/vpkeditcli /usr/bin/vpkeditcli
> # GUI:
> ln -s /path/to/standalone/vpkedit /usr/bin/vpkedit
> ```
>
> With the symlink in place, updating your standalone install will automatically update the binary on the PATH.

There are two ways of *installing* VPKEdit specific to the following distros:

#### Debian-based:

1. Download the installer from the GitHub releases section, and extract the `.deb` file from inside.
2. Run `sudo apt install ./<name of deb file>.deb` in the directory you extracted it to.

#### Arch-based:

VPKEdit is on the Arch User Repository thanks to [@HurricanePootis](https://github.com/HurricanePootis).
Please consult the [Arch Wiki](https://wiki.archlinux.org/title/Arch_User_Repository) for more information,
as I don't use Arch Linux personally and don't know how that system works. (I use Debian btw)
