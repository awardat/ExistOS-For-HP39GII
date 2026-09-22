 ___This English readme may not be up to date with the [Chinese version](./README.md).___

# ExistOS-For-HP39GII (Fork)

> **This project is a fork by [awardcat](https://github.com/awardcat) from [ExistOS-Team/ExistOS-For-HP39GII](https://github.com/ExistOS-Team/ExistOS-For-HP39GII).**

This fork is developed using Vibe Coding with AI-assisted code review, bug fixes, and requirement management. Key improvements:

- Comprehensive code audit and security hardening
- KhiCAS Chinese localization
- Ongoing bug fixes and quality improvements

Issues and suggestions: [GitHub Issues](https://github.com/awardcat/ExistOS-For-HP39GII/issues).

---

An open source HP39GII firmware project

## Project Overview

[![GPL Licence](https://badges.frapsoft.com/os/gpl/gpl.png?v=103)](https://opensource.org/licenses/GPL-3.0/)
[![Build Status](../../actions/workflows/build.yml/badge.svg)](../../actions/workflows/build.yml)

This firmware project is created by a group of calculator enthusiasts, using libraries such as [FreeRTOS kernel](https://github.com/FreeRTOS/FreeRTOS), [TinyUSB](https://github.com/hathach/tinyusb), [FatFs](http://elm-chan.org/fsw/ff/00index_e.html), [dhara](https://github.com/dlbeer/dhara), [giac](http://www-fourier.ujf-grenoble.fr/~parisse/giac.html), etc. Like-minded friends are more than welcome to try out and improve the code of this project. We'd love to hear your voices!

Refer to the [Install Guide](#only-installing) for installing procedures.

# Note: The following features mainly applies to Build 70 and older versions.

## Contents

| | User Guide | |
| :---: | :---: | :---: |
| [Current Development Status](#current-developping-status) | | |
| | **[Install Guide](#only-installing)** | |
| For Windows 10/11 | [ExistOS Updater](#for-windows-10--11-existos-updater) | Automated tools for beginers |
| Win/Linux | [OS Loader & EDB](#for-windows--linux) | |
| | **[Usage](#basic-usage-of-the-firmware)** | |
| [Setup](#booting-for-the-first-time) | [Shortcuts](#shortcuts) | [Accessing Internal Storage](#accessing-internal-storage) |
| **[KhiCAS user manual](docs/KhiCAS-manual.md)** | [Keys and menus](docs/KhiCAS-manual.md) | [Function reference](docs/KhiCAS-functions.md) |
| **[Uninstalling and Flashing Back](#uninstalling-existos-and-flashing-back-to-the-hp-firmware)** | **[Contributors](#contributors)** | **[License](#license)** |

| | Developer Guide | |
| :---: | :---: | :---: |
| [Current Development Status](#current-developping-status) | | |
| | **[Comiling and Installing](#compiling-and-installing)** | |
| [Prerequisites](#prerequisites) | [Compiling ExistOS](#compiling-existos) | [Flash firmware](#installing) |
| | **Code contribution** | |
| Documents (To do) | [Code submission standard](#code-submission-standard) |
| **[Uninstalling and Flashing Back](#uninstalling-existos-and-flashing-back-to-the-hp-firmware)** | **[Contributors](#contributors)** | **[License](#license)** |

## Current Development Status (latest only, see CHANGELOG.md)

### build 144 (2026-09-22 released, [Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-144))
- [x] **Python app input experience**: prompts (first line `>>> `, continuation lines `... `, shown immediately after ENT); `heap` info line moved between version info and `>>>`; input line visible after clear/reset; **Reset interpreter** reprints the startup info (version -> heap -> `>>>`)
- [x] **Python app menu partial redraw**: File menu up/down and Help pages left/right redraw only the menu area (selection bar follows), no full-screen redraw, no console flash

### build 143 (2026-09-22 released, [Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-143))
- [x] **Stock firmware reverse engineering** (research notes): unpacked firmware.sb; source-level stock NAND structures (NCB/LDLB/DBBT formats and block layout); CP15 histogram shows stock uses cache lockdown + TTB/FCSE; stock PPL is compiled, CAS grammar present but not exposed
- [x] **Bad-block RAM cache** (stock master BB table idea) — faster boot, faster KhiCAS exit/save
- [x] **Python app**: symbol panel partial refresh (no more full-screen redraw on arrows) with its own bottom bar; run entry moved to **F6 File -> Run script**; files moved to **/python** (auto-migrated on first start); GC heap tries large sizes first (512K, configurable 16-512K via `/python/pyheap.cfg`)
- [x] **Config persistence fix**: JSON parser did not stop at newline, so MEM SWAP was always read back as false (saved fine, lost on reboot)

## Memory and MEM SWAP (important)

On-chip RAM is limited (malloc heap ~160KB). Enabling **Settings -> MEM SWAP** merges the 3MB FTL swap area into the malloc heap (about **3.16MB** total); KhiCAS heavy computations and the Python app need it.

- **MEM SWAP defaults to OFF after every firmware flash** (config reset): enable it in **Settings** and reboot
- With it off: a long KhiCAS session can exhaust the on-chip heap and the serial port prints `EXT HEAP NOMEM !` (since build 141 the computation aborts early instead of crashing)
- Cost: swap uses NAND page swapping, so heavy paging causes **wear**; light everyday use may keep it off
- Usage: the Settings page shows `used/total` (about 3.16MB when enabled)

### Flash storage layout (128MB NAND)

| Area | Location | Size | Notes |
|------|----------|------|-------|
| Stock firmware/boot | blocks 0-21 | 2.75 MB | Reserved (stock system/recovery) |
| OSLoader | block 22 | 128 KB | Bootloader |
| Config/reserved | blocks 23-30 | ~1 MB | System configuration |
| ExistOS firmware partition | blocks 31-159 | 16.5 MB | `ExistOS.sys` is ~5.6MB, plenty of headroom |
| FTL volume | blocks 160-1023 | 108 MB | dhara flash translation layer |
| - FTL front | sectors 0-4095 | 8 MB | **MEM SWAP 3MB** + unused headroom |
| - User data (USB drive) | sectors 4096+ | ~80 MB | Visible over USB |
| - dhara reserve/bad blocks | | ~20 MB | GC/checkpoints/bad-block management (do not touch) |

### Why MEM SWAP is fixed at 3MB

The OSLoader has 512KB of RAM (34KB heap), and **every 1MB of virtual address space costs 4KB of L2 page tables**. Growing the swap from 3MB to 6MB adds 12KB of page tables, shrinking the heap from 34KB to 22KB, which makes the OSLoader run out of memory at boot (`!!!!OOM!!!` + reset loop) and shows a **white screen** (measured 2026-09-21, reverted).

So the swap stays at 3MB (heap limit ~3.16MB); the OSLoader RAM budget (512KB / 34KB heap) is a hard constraint.

## Only Installing

### For Windows 10 / 11: ExistOS Updater

Requires:

- Firmware: Download from [here](https://github.com/awardat/ExistOS-For-HP39GII/releases) (this repository).
  - Download `OSLoader.sb` and `ExistOS.sys`.
- ExistOS Updater: Download from [here](https://github.com/ExistOS-Team/ExistOS_Updater_v2/releases).
  - Only supports Windows 10 / 11.

Then follow the [instructions](https://github.com/ExistOS-Team/ExistOS_Updater_v2#readme) to flash the firmware.

### Migrating from the stock firmware / unbricking (no command line)

The **HostLink mode** of ExistOS Updater works with **any** HP39GII, including devices still running the stock firmware or unable to boot:

1. Remove the batteries (make sure the device is powered off)
2. **Hold `ON/C`** and connect USB to the computer (enters the ROM-level USB mode)
3. Choose HostLink mode in the Updater and flash `OSLoader.sb` and `ExistOS.sys`

This is equivalent to the command-line `sbloader` + `edb` flow. If storage fails to mount on the first boot after flashing, press **`ON`+`F5`** in the OSLoader to enter the erase-data menu and clear the partitions (this deletes all user files on the device), then flash again.

### For Windows / Linux

Requires:

- Firmware: Download from [here](https://github.com/awardat/ExistOS-For-HP39GII/releases) (this repository).
  - Download `OSLoader.sb` and `ExistOS.sys`.
- sb_loader: Used to send OSLoader to the RAM of your calculator if you haven't installed it before.
  - Windows: Download binary file from [here](../../raw/main/tools/sbtools_win/sb_loader.exe).
  - Linux: Download zip file from [here](../../archive/refs/heads/main.zip) and extract it. Then run the following commands:
    - Install `crypto++` library. Following the "Install dependencies" section [here](#Prerequisites)
    - `cd tools/sbtools/`
    - `make`
    - Then you will get the executable file `sb_loader`
- EDB (ExistOS Debug Brige): Used to flash firmwares.
  - Windows: Download binary file from [here](../../raw/main/tools/edb.exe).
  - Linux: Download zip file from [here](https://github.com/ExistOS-Team/edb-unix/archive/refs/heads/master.zip) and extract it. Then run the following commands:
    - `mkdir build`
    - `cmake -B build`
    - `cmake --build build`
    - Then you will get the executable file `edb`

Put the mentioned files to a directory.

If you haven't installed ExistOS on your calculator, please go through this to-do list first:
1. Remove all batteries from your calculator.
2. Hold `ON/C` key and connect your calculator to computer via USB cable.
3. Run command `sbloader OSLoader.sb`
  - Normally your calculator will boot into the OSLoader, and then a message will show up on the screen. There is no need to unplug the USB cable now. Just continue to do the following steps.
  - ![OSL Boot](Image/1.png)

If ExistOS has already been installed on your device:
1. Connect your calculator to computer via USB cable.
2. Run command `edb -r -f OSLoader.sb 1408 b` to flash the OSLoader.
  - Your calculator will reboot automatically.
3. Run command `edb -r -f ExistOS.sys 1984` to flash the System.
  - Your calculatr will reboot automatically.
1. Enjoy ExistOS!
  - If you are in trouble with the installation or anything else, open an issue or join our Discord server to seek for help.

## Note on KhiCAS Computation Speed

Some symbolic operations take seconds to tens of seconds on the HP39GII. This is inherent to symbolic algorithms (not a hang; the busy indicator lights up while computing):

- Hardware: ARM926EJ-S @240MHz (480MHz in boost mode), no hardware FPU; giac uses libtommath software bignum arithmetic
- One symbolic evaluation costs ~85ms; solving equations/inequalities/systems needs hundreds to thousands of evaluations
- Measured (standard 240MHz / boost 480MHz): `solve(exp(x)=x^2,x)` 17.4s / 8.3s; a small nonlinear system 15.2s; `integrate(sin(x)/x,x)` 12.6s; a cubic inequality 8.7s
- Tip: switch to **boost mode** (Shift+Symb config menu -> speed) for roughly 2x speedup
- Compared with the old snapshot (~1.4/1.5), the giac 2.0.0 solver is more complete (multiple roots, inequalities, RUR systems) at some speed cost

## Compiling and Installing

### Prerequisites

Clone the git repo first:
```bash
git clone https://github.com/ExistOS-Team/ExistOS-For-HP39GII.git # https
git clone git@github.com:ExistOS-Team/ExistOS-For-HP39GII.git # ssh
```
Then enter the directory:
```bash
cd ExistOS-For-HP39GII
```
Switch to the root directory of this project first.

Note:
- `gcc-arm-none-eabi` v10.3 is tested OK. Download binary executable files from [here](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2).
  - Using other versions of GCC may cause OSLoader to not run.

|System|Install|
|----|----|
|Windows|Download from [here](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.exe?rev=29bb46cfa0434fbda93abb33c1d480e6&hash=B2C5AAE07841929A0D0BF460896D6E52) and install `gcc-arm-none-eabi`|
||Do not forget to add directory to the PATH environment variable|
|Debian & Ubuntu|`apt-get install gcc-arm-none-eabi`|
|Arch Linux|`pacman -Syu arm-none-eabi-gcc`|
|Other|Lookup if there are binary packages provided. Or you can build from [source code](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/downloads)|
||The link above is the new version. You can download the old version (V10.3) from [here](https://developer.arm.com/downloads/-/gnu-rm#panel1a)|

Add `udev` rule:
|System|Install|
|----|----|
|Windows|N/A|
|Linux|`sudo cp 99-hp39gii.rules /etc/udev/rules.d/`|
||Then restart `udev` to load the new rule:|
||`sudo service udev restart`|
||If commands above didn't work:|
||`sudo udevadm control --reload-rules`|
||`sudo udevadm trigger`|
|Other distros using `udev`|Copy `99-hp39gii.rules` in the project to the directory containing `udev` rules, then restart `udev`|

Install compiler:
|System|Install|
|----|----|
|Windows|Download binary executable of [Ninja](https://github.com/ninja-build/ninja/releases), extract it to a directory and add it your PATH|
|Debian & Ubuntu|`apt-get install cmake make`|
|Arch Linux|`pacman -Syu cmake make`|

Install dependencies:
|System|Install|
|----|----|
|Windows|N/A|
|Debian|`apt-get install libcrypto++-dev libusb-1.0.0-dev`|
|Ubuntu|`apt-get install libcrypto++6 libcrypto++-dev libusb-1.0.0-dev`|
|Arch Linux|`pacman -Syu libusb crypto++`|
|Others|Install libusb 1.0, [libcrypto++](https://cryptopp.com/wiki/Linux#Distribution_Package)|
||Check installtion with `pkg-config`|

_Tips: `pkg-config` will search for libraries according to `/usr/lib/pkgconfig/*.pc`. If you would like to manually add libraries, please modify `CMakeLists.txt` to correct paths._

Build `sbtool`:
|System|Install|
|----|----|
|Windows|Pre-compiled executable in `tools/`|
|Linux|`cd tools/sbtools/ && make`|
||`cp sb_loader ../`|
||`cp elftosb ../`|
||`cd ../../`|
||`cd Libs/src/micropython-master/ports/eoslib/ && make`|
||`cd ../../../../../`|

Build `EDB`:
|System|Install|
|----|----|
|Windows|Pre-compiled executable in `tools/`|
|Linux|`cd tools/`|
||`git clone https://github.com/ExistOS-Team/edb-unix.git`|
||`cd edb-unix/`|
||`mkdir build`|
||`cmake -B build/`|
||`cmake --build build/`|
||`cp build/edb ../`|
||`cd ../../`|

Build sys_signer:
|System|Install|
|----|----|
|Windows|Pre-compiled executable in `tools/`|
|Linux|`cd tools/sys_signer/`|
||`mkdir build`|
||`cmake -B build/`|
||`cmake --build build/`|
||`cp build/sys_signer ../`
||`cd ../../`|

### Compiling ExistOS

Create a new directory to store binary files and caches:
```bash
mkdir build
cd build
```

Preparing to compile:
|System|Install|Note|
|----|----|----|
|Windows|`cmake .. -G Ninja`|Specifies `Ninja` as complier|
|Linux|`cmake ..`||

Compiling:
|System|Install|
|----|----|
|Windows|`ninja`|
|Linux|`make`|

### Installing

#### OSLoader in RAM (temporarily)

Note: Please install drivers for HP39GII on your own.

You can skip this step if the OSLoader has been installed on your device (unless it's bricked.)

OSLoader boots ExistOS and provides low-level APIs and virtual memory service. Run commands below to load the OSLoader temporarily.

Before flashing, power off your calculator completely by removing the batteries, and then plug in USB cable while holding down the `ON/C` key. Then your calculator will enter the flashing mode.

An HID device named "USB Input Device" with the ID of 066F:3770 will show up in the Device Manager under Windows.

![USBID](Image/0.png)

|System|Install|
|---|---|
|Windows|`ninja sb_flash`|
|Linux|`make sb_flash`|

#### OSLoader

|System|Install|
|----|----|
|Windows|`ninja edb_flash_loader`|
|Linux|`make edb_flash_loader`|

This will flash OSLoader to the calculator.

Your calculator will reboot automatically.

#### ExistOS

|System|Install|
|----|----|
|Windows|`ninja edb_flash_sys`|
|Linux|`make edb_flash_sys`|

This will install ExistOS on your calculator.

Your calculator will reboot automatically.

## KhiCAS Calculator

KhiCAS user manual (startup / keys / menus / scripts / Python compat mode / FAQ): [docs/KhiCAS-manual.md](./docs/KhiCAS-manual.md).

KhiCAS built-in function reference (355 commands in 22 categories with purpose/parameters/examples, Chinese): [docs/KhiCAS-functions.md](./docs/KhiCAS-functions.md).

## Python App

Built-in standalone **MicroPython 1.29** app (4th icon on the apps page): full Python language (classes/generators/exceptions/comprehensions), script runner (`/xcas/pyNN_*.py`), file I/O, Chinese menus and help.

- **User manual** (startup / key tables / menus / syntax differences / samples / benchmarks / FAQ): [docs/Python-app-manual.md](./docs/Python-app-manual.md)
- **Samples**: `samples/py01_basics.py`, `samples/py07_nqueens.py` (N-Queens 8x8: 1221 ms standard / 703 ms boost)

## Basic Usage of the Firmware

### Booting for the first time

During the first boot after the installation you will see the following dialog, prompting you to format the data section of the flash as FAT16. Press `ENTER` to confirm the operation, which usually takes around 30 seconds.

![Sys1](Image/2.png)

This screen indicates a successful formatting attempt. Select OK to enter the main menu.

![Sys1](Image/3.png)

This system only comes with a KhiCAS application for now. Press `←` `→` `↑` `↓` to navigate and `ENTER` to confirm.

![Sys1](Image/4.png)

The Files tab is the file explorer that is capable of viewing jpg pictures, playing mjpeg encoded avi videos and executing .exp ExistOS applications. No other file managing functions are implemented now.

![Sys1](Image/4-1.png)

The Status tab shows system status and related settings.

![Sys1](Image/5.png)

![Sys1](Image/5-1.png)

### Shortcuts

`ON` + `F5`: Enter the maintenance menu

`ON` + `[+]` / `[-]`: Adjust the contrast

### Accessing internal storage

Holding down the `F2` key while booting (or immediately after pressing the `ON/C` key) will bring up the following interface:

![Sys1](Image/38.png)

An 80 MB USB drive, the data section of the onboard flash, will show up on your computer. The `System` directory stores the assets, for example, fonts and pictures (Unused right now). The `xcas` directory stores KhiCAS user scripts, sessions (history) and other datas.

![Sys1](Image/39.png)

![Sys1](Image/40.png)

## Uninstalling ExistOS and Flashing Back to the HP Firmware

You need to erase the whole flash before flashing back to the HP Firmware, otherwise you'll get stuck at the formatting progress when using the official update tool.

How to erase the whole flash:  
After flashing OSLoader or while ExistOS is running, press `ON`+`F5` to enter the maintenance menu, and then press `F2` to erase the flash. ___This operation cannot be undone.___ When the screen shows "Flash Cleared", connect the calculator to a computer and launch the official update tool under 7 / XP to flash your calculator back to the HP firmware.

## Code Submission Standard

**If you want to contribute to this project, please follow the standards below**

1. Variable

    * Variables are named using lower camel case. For example, `windowHeight`
    * Function parameter naming is the same as variable.
    * Don't name using a single letter except for temporary or loop variable. 
    * It is prefered to add some meaningful prefix. For example, `p` means pointer.
    * Don't define variable and pointer at the same time on one line, such as `char *p, q;`

2. Function

    * Functions are named using underline. For example, `get_window_width`。
    * Function names should follow the Verb - object relationship.
    * It is prefered to add some meaningful prefix. For example, `is` means the type of return value is bool.
    * Short function can be defined as inline, function parameters and return values should be pointer instead of variable.
    * Avoid using recursion, consider refactoring to cycling.

3. Constant, macro and hardware-related

    * Constants and macros are named using underline to divide upper-case letters. For example, `MAX_WIDTH`。

4. Custom type

    * Custom type names are named using underline (tentative).
    * Must use typedef to define a custom type before using struct to define non-single-instance object.

5. Operator and other symbol

    * Unary operators should appress the variable, such as `c++`, `*p`.
    * Binary operators shold have spaces on both sides, such as `i == 1`, `a += 3`, except for `->`
    * Ternary are the same as binary, such as `isLeft ? 1 : 0`.
    * commas should be followed by a space.
    * Parentheses should be added appropriately where it is not easy to understand.

6. Pseudo-class

    If object-oriented is necessary, you can consider using `typedef struct` as pseudo-class.

    * Pseudo classes should be named by the upper camel case (Pascal) name.

    * For properties inside the class, its name is the same as the general variable.

    * Pseudo-class methods are not saved in pseudo-classes, but are global functions. Methods should be named using underline.

      * Normal method should be named as `ClassName_method_name`, the first parameter should always be a pointer to the instance and be named `this` (even unnecessary).

      * Static method should be named as`ClassName_static_method_name`。
      * General method named `ClassName_initializer` should be called right after an instance is defined.

7. Coding

    7.1

    ```c
    if (a == 1) {                          // There should be spaces between keywords and brackets, and spaces between brackets and curly brackets
        // code here
    }else{                                 // Use Java style
        // code here
    }
    if (b == 1) return;                    // When there is only one sentence of code in the block, you can leave a blank space without curly braces
    ```

    7.2

    ```c
    while (true)
        ;                                  // When using empty loops, you should wrap the semicolon and indent it
    ```
    7.3

    ​	(a) The loop variable of a general for loop is defined in the for loop:

    ```c
        for (int i = 0; i < l; i++) {
            // code here
        }
    ```

    ​	(b) In cases where circular variables are used externally, an initial value should also be given here:

    ```c
        int i;
        for (i = 0; i < l; i++) {
                // code here
        }
        return i;
    ```

    ​	(c) Do not leave any of the three of for loops empty: ` for ( ; ; ) `, otherwise use the while loop.

    7.4 Don't use assignment where a sentence needs to be judged, such as `if (a = 1), (a = 1) ? a : 0 `.

    7.5 The goto statement should be avoided as much as possible.

    7.6 Switch should be used more than else if.In a switch statement, it is best to have a break/return statement in each case, except when multiple cases share exactly the same piece of code.Care should be taken when using switch traversal and it is best to comment on it.

For VSCode users, `clang-format` extension is available to format the code conveniently.

## Contributors

<a href = "https://github.com/ExistOS-Team/ExistOS-For-HP39GII/graphs/contributors">
  <img src = "https://contrib.rocks/image?repo=ExistOS-Team/ExistOS-For-HP39GII"/>
</a>

Special thanks to:
- [parisseb](https://github.com/parisseb)

## License

[GPL-3.0](./LICENSE)
