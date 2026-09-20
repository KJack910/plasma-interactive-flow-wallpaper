# Portability and generality audit

Audit date: September 20, 2026.

## Summary

The system is general within this target boundary:

```text
Linux + KDE Plasma 6 + Qt 6.4+ build / Qt 6.6+ verified runtime + OpenGL 3.3+ + user Plasma session
```

It is not a cross-platform Windows/macOS wallpaper. The C++ renderer uses Qt,
but installation, desktop integration and hardware monitoring use Linux/KDE
interfaces.

## Compatibility matrix

| Area | Status | Verified boundary |
|---|---|---|
| C++ build | General on Linux with Qt 6 | CMake 3.22+, Qt 6.4+, Ninja, C++20 |
| Rendering | GPU-dependent | OpenGL 3.3+ |
| Plasma package | KDE-specific | Plasma 6, `kpackagetool6` |
| Session | Desktop-specific | KDE Plasma, Wayland verified |
| Multi-monitor | General logical model | Rectangular and offset layouts |
| CPU usage | Linux-specific | `/proc/stat` |
| GPU usage | Linux/DRM-specific | direct metrics and `busy_time` fallback |
| Per-output pause | KDE/KWin-specific | QtDBus + KWin script |
| Automatic dependencies | Limited | Arch/pacman currently |
| Windows/macOS | Unsupported | no package or installer target |
| Plasma 5 | Unsupported | Plasma 6 APIs and tools |

## Local verification

The current development system verified:

- CMake, Ninja and a C++ compiler;
- Qt 6 and `qmllint`;
- `kpackagetool6`, `plasmashell`, `systemctl` and `qdbus6`;
- successful CMake configuration;
- successful Release build;
- six passing CTest cases;
- QML lint without errors;
- Bash syntax for build, installation, removal and diagnostic scripts.

This demonstrates reproducible behavior in the local Linux/KDE target. It does
not prove a build on Debian, Fedora, Windows or macOS.

## Why the system is not fully generic

1. `/proc/stat` and `/sys/class/drm` are not available on Windows/macOS.
2. `kpackagetool6`, `kwriteconfig6`, `qdbus6` and `plasmashell` are KDE tools.
3. Restarting the shell uses `systemctl --user` and assumes a user systemd
   session.
4. `scripts/install.sh --deps` uses Arch package names through pacman.
5. The renderer uses the OpenGL `QQuickFramebufferObject` path; Vulkan is not
   selected by an individual wallpaper instance.
6. Package metadata and installation are specific to `Plasma/Wallpaper`.

## Reusable parts outside the development system

- the Qt/C++ renderer and most shader code;
- mathematical tests for mesh, spline, zoom and visibility;
- the global virtual-desktop model;
- the CMake structure;
- the Plasma package on other Linux distributions with Plasma 6;
- the DRM GPU fallback on drivers exposing the expected counters.

## Work required for broader support

For more Linux distributions:

- document equivalent packages;
- separate build from Plasma installation;
- provide an installer that does not require pacman;
- test at least one Debian/Ubuntu and one Fedora/KDE environment;
- make KWin and SystemUsage integrations optional.

Windows/macOS support would require separate ports for package installation,
CPU/GPU metrics, desktop lifecycle and wallpaper integration. It is not a
consequence of changing only the CMake generator.
