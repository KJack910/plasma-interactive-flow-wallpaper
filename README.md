# Plasma Interactive Flow Wallpaper

Version 1.1.0

A native interactive wallpaper for KDE Plasma 6, inspired by the visual
language of the PlayStation 3 XMB but implemented as an independent Qt/C++
project. Waves and particles are rendered by a native OpenGL scene instead of
HTML, Chromium, or Qt WebEngine.

The project targets Linux desktops running KDE Plasma 6. It is not a
Windows/macOS wallpaper and it does not currently target Plasma 5.

## Features

- Native Qt 6 / C++20 renderer using `QQuickFramebufferObject` and OpenGL.
- Configurable wave mesh quality, FPS target, particle count and particle
  style.
- Cursor interaction, click easing and cursor-pivot wheel zoom.
- Global virtual-desktop mapping for linked multi-monitor layouts.
- Per-output pause when a monitor is fully covered by windows.
- Manual pause and pause-when-hidden controls.
- CPU/GPU readings in the settings page with a 300 ms refresh interval.
- CPU readings from Linux `/proc/stat`.
- GPU readings from DRM sysfs, with direct metrics and engine-counter fallback.
- CTest coverage for zoom, mesh, spline and output-visibility behavior.
- User-local installation and removal scripts.

## Requirements

- Linux with KDE Plasma 6.
- Wayland is the verified session type; X11 is not the primary target.
- Qt 6.6 or newer: Core, Gui, Quick, Qml, OpenGL and DBus.
- CMake 3.22 or newer.
- Ninja and a C++20 compiler.
- OpenGL 3.3 or newer.
- KDE tools such as `kpackagetool6`, `kwriteconfig6` and `qdbus6` for
  installation and runtime integration.

The automatic dependency option currently supports Arch/CachyOS through
`pacman`. Other Linux distributions can use the same source with equivalent
packages installed manually. See [the portability audit](docs/PORTABILITY.md).

## Quick start

```bash
git clone https://github.com/KJack910/plasma-interactive-flow-wallpaper.git
cd plasma-interactive-flow-wallpaper
./scripts/check-prerequisites.sh
./scripts/build.sh
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
./install.sh
```

After installation, open:

```text
Desktop context menu → Configure Desktop and Wallpaper → XMB Interactive Flow
```

The internal Plasma package ID remains `org.xmbflow.interactive` for
compatibility with existing installations.

## Build and test manually

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
./scripts/check-version.sh
```

The generated native plugin is written to:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

Generated files are excluded from Git by `.gitignore`.

## Installation and removal

Install for the current Plasma user:

```bash
./install.sh
```

On Arch/CachyOS, the build dependencies can be requested with:

```bash
./scripts/install.sh --deps
```

Remove the wallpaper package and the companion KWin script with:

```bash
./uninstall.sh
```

Installation restarts `plasmashell` and changes the current user's Plasma
session. Keep a backup before testing renderer changes.

## Diagnostics

```bash
./scripts/diagnose.sh
./scripts/multiscreen-diagnose.sh
```

For live Plasma logs:

```bash
journalctl --user -u plasma-plasmashell.service -f
```

## Project layout

```text
src/                         C++ renderer and native QML types
package/                     Plasma wallpaper package and QML UI
kwin-script/                 Per-output coverage/pause integration
scripts/                     Build, install, removal and diagnostics
tests/                       C++ regression tests
docs/                        Specifications, installation and portability docs
.github/                     GitHub Actions CI and contribution templates
```

## Documentation

- [Specification](docs/SPECIFICATION.md)
- [Installation](docs/INSTALLATION.md)
- [Portability audit](docs/PORTABILITY.md)
- [Testing strategy](docs/TESTING.md)
- [Versioning](docs/VERSIONING.md)
- [Language and Git conventions](docs/LANGUAGE.md)
- [Development notes](DEVELOPMENT.md)
- [Performance notes](PERFORMANCE.md)
- [Contributing](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)

## Versioning

The project follows Semantic Versioning. The current release is documented once
in [VERSIONING.md](docs/VERSIONING.md); release changes are recorded in
[CHANGELOG.md](CHANGELOG.md). Git tags use the `vMAJOR.MINOR.PATCH` format.

## License and attribution

The project is released under the MIT License. The visual and spline approach
is partly derived from the MIT-licensed
[PlayStation-3-XMB](https://github.com/linkev/PlayStation-3-XMB) project.
See [ATTRIBUTION.md](ATTRIBUTION.md) for the project-specific changes.
