# Contributing

## Requirements

- Linux with Qt 6.6+ and OpenGL 3.3.
- CMake 3.22+.
- Ninja.
- Qt 6 modules: Core, Gui, Quick, Qml, OpenGL and DBus.
- KDE Plasma 6 for installation and runtime testing.

On Arch/CachyOS, the helper can install the build dependencies:

```bash
./scripts/install.sh --deps
```

## Build and test

Use an out-of-source build and keep generated files out of commits:

```bash
./scripts/check-prerequisites.sh
cmake -S . -B build -G Ninja
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
./scripts/check-version.sh
```

The test suite covers zoom boundaries, mesh visibility and geometry, spline
CPU/GPU paths and output visibility.

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
- Verify the installed plugin and QML package after installation.
- Preserve a timestamped rollback archive for changes that affect rendering.

## Language and Git conventions

English is the canonical language for source comments, documentation, issues,
pull requests and commit messages. Use Conventional Commits, for example:

```text
feat: add per-output rendering pause
fix: preserve cursor pivot during zoom
docs: clarify installation requirements
```

Localized documents use the `.locale.md` suffix described in
[docs/LANGUAGE.md](docs/LANGUAGE.md).
