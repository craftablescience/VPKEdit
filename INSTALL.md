# Installing VPKEdit

This document covers the different ways you can install VPKEdit on its supported
platforms:

- Windows 10/11 (64-bit)
- Linux (64-bit)

Note: if you wish to *update* the application, simply repeat these steps. This works for
both the standalone method and installer method.

## Windows

1. You will need to install the VS2015-2022 runtime located at https://aka.ms/vs/16/release/vc_redist.x64.exe.
2. When that is installed, download either the standalone version or the installer version from [the latest
   GitHub release](https://github.com/craftablescience/VPKEdit/releases/latest), under the `Assets` dropdown.
3. If you downloaded the standalone files, you're done, just unzip the files. If you downloaded the installer,
   unzip the installer application and run it. Windows will give you a safety warning, ignore it and hit `More Info` → `Run Anyway`.

## Linux

Installation on Linux will vary depending on your distro. On all distros you should be able to run the standalone
application without any issues. There are two ways of *installing* VPKEdit
specific to the following distros:

#### Debian-based:

1. Download the installer from the GitHub releases section, and extract the `.deb` file from inside.
2. Run `sudo apt install ./<name of deb file>.deb` in the directory you extracted it to.

##### installing cli

if you want to be able to use the CLI without needing to specify the path to the CLI application do the following
1. unzip the standalone CLI zip where ever
2. set ```vpkeditcli``` to be an executable using ```chmod +x vpkeditcli```
3. then move ```vpkeditcli``` to ```usr/local/bin``` with the command ```mv ./vpkeditcli /usr/local/bin/```

You can now use the CLI by just typing ```vpkeditcli``` in your Linux terminal

#### Arch-based:

VPKEdit is on the Arch User Repository thanks to [@HurricanePootis](https://github.com/HurricanePootis).
Please consult the [Arch Wiki](https://wiki.archlinux.org/title/Arch_User_Repository) for more information,
as I don't use Arch Linux personally and don't know how that system works. (I use Debian btw)
