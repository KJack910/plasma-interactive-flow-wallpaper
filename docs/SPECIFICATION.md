# Specification

Reference version: 1.1.0
Plasma package ID: `org.xmbflow.interactive`
Repository: `plasma-interactive-flow-wallpaper`

## Purpose

Provide a native KDE Plasma 6 wallpaper inspired by the PlayStation 3 XMB
visual language, with interactive waves and particles rendered through Qt Quick
OpenGL and integrated with Wayland.

The project must install for the current user without modifying system files
and must provide a dedicated removal path.

## Functional requirements

1. Plasma must recognize the package as `Plasma/Wallpaper`.
2. The renderer must use `QQuickFramebufferObject` and OpenGL.
3. Configuration must be available through Plasma's `config.qml` page.
4. Configuration must expose mesh quality, FPS, particles, speed, brightness,
   pointer interaction and pause controls.
5. Pointer interaction must deform the field when enabled.
6. Wheel zoom must use the pointer as its pivot and obey the limits in
   `src/zoomconstraints.h`.
7. Linked multi-monitor rendering must use one global virtual surface rather
   than an independent phase for each monitor.
8. Rendering must be pausable when hidden, covered or manually paused.
9. `SystemUsage` must refresh CPU/GPU readings every 300 ms when included in
   the configuration page.
10. Removal must uninstall the Plasma package and the project's KWin script
    without deleting unrelated Plasma settings.

## Non-functional requirements

- C++20.
- CMake 3.22 or newer.
- Qt 6.4 or newer for building: Core, Gui, Quick, Qml, OpenGL and DBus.
- Qt 6.6 or newer is the verified runtime baseline.
- Out-of-source build, preferably with Ninja.
- Tests runnable through CTest.
- No dependency on Qt WebEngine, Chromium or HTML.
- No secrets, personal configuration or generated binaries in Git.

## Architecture

```text
Plasma WallpaperItem
├── package/contents/ui/main.qml
│   └── XmbRendererItem
│       └── QQuickFramebufferObject
│           ├── global mesh and projection
│           ├── CPU/GPU spline texture
│           ├── wave shaders
│           └── particle shaders
├── package/contents/ui/config.qml
│   └── Plasma settings + SystemUsage
├── src/xmbnativeplugin.cpp
│   └── native QML type registration
└── kwin-script/contents/code/main.js
    └── covered-output detection and per-output pause
```

## Multi-monitor contract

Each wallpaper instance receives its output origin and size, the virtual desktop
size and a reference size. The renderer evaluates the field in the same global
logical surface and clips the result to the local output. This preserves phase
continuity across horizontal and vertical seams, including layouts with
unequal monitor sizes.

Diagnostic mode must display a continuous global grid. The coordinates printed
by `scripts/multiscreen-diagnose.sh` are an example from development, not a
hardware requirement.

## SystemUsage contract

- CPU: aggregate `cpu` line from `/proc/stat`.
- Sampling: precise Qt timer at 300 ms.
- GPU: enumerate every `/sys/class/drm/card*` adapter.
- Preferred direct metrics: `gpu_busy_percent` and `gt_busy_percent`.
- Fallback: deltas of `engine/*/busy_time` counters.
- Multiple GPUs: report the busiest adapter when direct metrics are available.
- Non-Linux systems or drivers without DRM metrics: GPU data is not guaranteed.

## Installation and rollback contract

The build writes the native plugin directly to:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

The installer:

1. checks the required tools;
2. builds the project;
3. removes an older package with the same ID, if present;
4. installs the Plasma package for the current user;
5. restarts `plasmashell`;
6. installs and reloads the companion KWin script.

Installation changes the current Plasma session. Create a backup before testing
renderer changes. Use `uninstall.sh` to roll back the package and script, then
reinstall the previous version if required.

## Non-goals

- Plasma 5 support.
- Windows or macOS support.
- Desktop environments other than KDE Plasma.
- Authentication or encryption, which are not applicable to a local wallpaper.
- Precise energy attribution to this wallpaper alone.
