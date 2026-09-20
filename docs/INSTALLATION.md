# Installation

## Prerequisites

Supported environment:

- Linux with KDE Plasma 6;
- Wayland is the verified session type;
- CMake 3.22+;
- Ninja;
- a C++20 compiler;
- Qt 6.6+ with Core, Gui, Quick, Qml, OpenGL and DBus;
- `kpackagetool6`, `kwriteconfig6`, `qdbus6` and `plasmashell`;
- OpenGL 3.3+ for the renderer.

Automatic per-output pause also requires the KWin script integration. The
multi-monitor diagnostic script requires `kscreen-doctor`.

## Arch Linux, CachyOS and derivatives

The helper can install build dependencies with:

```bash
./scripts/install.sh --deps
```

Without dependency installation:

```bash
./scripts/build.sh
```

## Manual build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
```

The native plugin is generated at:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

## Tests and lint

```bash
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
./scripts/check-version.sh
```

## Installation in the current Plasma session

After the build and checks:

```bash
./install.sh
```

The script installs the package for the current user, restarts `plasmashell`
and installs the `xmbfullscreenbridge` KWin script.

Then open:

```text
Desktop context menu → Configure Desktop and Wallpaper → XMB Interactive Flow
```

## Diagnostics

```bash
./scripts/diagnose.sh
./scripts/multiscreen-diagnose.sh
```

For Plasma logs:

```bash
journalctl --user -u plasma-plasmashell.service -f
```

## Removal

```bash
./uninstall.sh
```

Removal uninstalls the package and disables the project's KWin script.

## Other distributions

The source can build on other Linux distributions when equivalent dependencies
are available. The `--deps` option is currently specific to Arch/pacman and
does not guess package names for Debian, Fedora or other distributions.

Install the following equivalents manually:

- CMake and Ninja;
- a C++20 compiler;
- Qt 6 Core, Gui, Quick, Qml, OpenGL and DBus;
- KDE Plasma 6 tools for KPackage, KWin and plasmashell.

After manual dependency installation, use the build and test commands above.
Installation still requires an active Plasma 6 session and the KDE commands
listed in the prerequisites.
