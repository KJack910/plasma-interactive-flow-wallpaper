# Testing strategy

## Automated checks

The CTest suite covers:

- zoom limits;
- mesh visibility;
- mesh geometry and deduplication;
- CPU/GPU spline equivalence when an OpenGL 3.3 context is available;
- the GPU spline path (marked skipped by CTest when the headless environment
  cannot create an OpenGL 3.3 context);
- output visibility and pause behavior.

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
./scripts/check-version.sh
```

## Manual post-installation checks

1. Install the package in a Plasma session.
2. Open the wallpaper configuration page.
3. Confirm that the settings page is visible.
4. Check pointer hover, press and release behavior.
5. Check wheel zoom, minimum/maximum limits and sensitivity.
6. Check mesh and particle presets.
7. On multiple monitors, enable the `Multiscreen` and `Diagnostic` options.
8. Confirm grid continuity at monitor seams.
9. Cover one monitor completely with a window and verify its pause.
10. Run `scripts/diagnose.sh` and inspect the `plasmashell` log.

## Acceptance criteria

A release is ready only when:

- CMake configuration succeeds;
- all CTest cases pass or are explicitly skipped when the host lacks the
  required OpenGL context;
- `qmllint` reports no errors;
- all Bash scripts pass `bash -n`;
- Plasma recognizes and installs the package;
- the wallpaper remains available after restarting `plasmashell`;
- no build artifact or local backup is tracked by Git;
- rollback through `uninstall.sh` is available.

Automated checks do not replace visual verification of the renderer or manual
multi-monitor testing.
