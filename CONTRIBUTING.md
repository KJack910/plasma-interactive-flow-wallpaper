# Contributing

## Requirements

- Linux with Qt 6.6+ and OpenGL 3.3
- CMake 3.22+
- Ninja
- Qt 6 modules: Core, Gui, Quick, Qml, OpenGL and DBus
- KDE Plasma 6 is required for installing and exercising the wallpaper

On CachyOS/Arch Linux, the helper can install the build dependencies:

```bash
./scripts/install.sh --deps
```

## Build and test

Use an out-of-source build and keep generated files out of commits:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
```

The test suite covers zoom boundaries, mesh visibility/geometry, spline
equivalence/GPU paths and output visibility.

## Local installation

Installation changes the current user's Plasma wallpaper and restarts
`plasmashell`:

```bash
./scripts/install.sh
```

Only install after the build, tests and QML lint pass. Keep a backup of the
currently installed package before testing renderer or interaction changes.

## Change policy

- Keep renderer, zoom, mesh, shader, overscan and particle changes isolated.
- Add a regression test before changing behavior.
- Do not commit generated build directories, native plugin binaries, local
  backups, Plasma configuration or credentials.
- Verify the installed plugin and QML package after an installation.
- Preserve a timestamped rollback archive for changes that affect rendering.
