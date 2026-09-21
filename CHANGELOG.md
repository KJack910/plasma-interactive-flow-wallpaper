# Changelog

All notable changes are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow
Semantic Versioning.

## [Unreleased]

- Reserved for changes after 1.1.0.
- Localized settings with English source strings and an Italian catalog.
- Complete configuration-property forwarding from Plasma QML to the native renderer.
- Headless-safe render-pause behavior and registered KWin/Node/Python integration tests.
- Runtime-derived multiscreen diagnostics without DP-1/DP-2 topology assumptions.
- Removed the layout-sensitive Diagnostics checkbox from the normal wallpaper settings page.
- Made Energy saving the master switch for automatic hidden/covered-output pausing.
- Unified hidden and covered-output pausing into one Energy saving setting.
- Removed zoom-derived overscan-off scissoring so disabled overscan still fills each output and clips only at the framebuffer boundary.
- Clamped sub-1 base zoom to 1.0 while overscan is disabled, preventing a centered virtual-desktop island from being mistaken for incomplete output rendering.
- Rebuilt the Italian catalog during installation and removed absolute local paths from the translation template.

## [1.1.0] - 2026-09-20

### Added

- Native KDE Plasma 6 wallpaper package with Qt/C++ OpenGL rendering.
- Cursor interaction, cursor-pivot wheel zoom and configurable flow groups.
- Linked virtual-desktop rendering for multi-monitor layouts.
- Automatic per-output pause through the companion KWin script.
- CPU/GPU readings in the configuration UI.
- CTest coverage for zoom, mesh, spline and output-visibility behavior.
- User-local build, installation, diagnostics and removal scripts.

### Changed

- Rendering uses a shared logical desktop surface before per-output clipping.
- Mesh and spline paths include the optimized implementations documented in
  `PERFORMANCE.md`.

### Fixed

- Qt 6 QML type registration compatibility.
- Multi-monitor phase continuity and viewport mapping.
- Mesh index handling for high-quality presets.

[Unreleased]: https://github.com/KJack910/plasma-interactive-flow-wallpaper/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/KJack910/plasma-interactive-flow-wallpaper/releases/tag/v1.1.0
