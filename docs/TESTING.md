# Testing strategy

## Automated checks

The CTest suite covers:

- zoom limits;
- mesh visibility;
- mesh geometry and deduplication;
- CPU/GPU spline equivalence when an OpenGL 3.3 context is available;
- the GPU spline path (marked skipped by CTest when the headless environment
  cannot create an OpenGL 3.3 context);
- output visibility and pause behavior;
- the QML-to-native configuration contract;
- the KWin power-save bridge and render-pause contract;
- dynamic KScreen topology parsing, including negative coordinates.

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
package/translate/build.sh
bash -n install.sh uninstall.sh scripts/*.sh package/translate/*.sh
./scripts/check-version.sh
python3 tests/test-kscreen-topology.py
```

## Manual post-installation checks

1. Install the package in a Plasma session.
2. Open the wallpaper configuration page.
3. Confirm that the settings page is visible.
4. Check the complete page under an English Plasma locale; all labels and options must be English.
5. Check the complete page under an Italian Plasma locale; the shipped Italian catalog must be used.
6. Check pointer hover, press and release behavior.
7. Check wheel zoom, minimum/maximum limits and sensitivity.
8. Check mesh and particle presets.
9. On multiple monitors, enable the `Multiscreen` and `Diagnostics` options.
10. Run `scripts/multiscreen-diagnose.sh` and verify that it reports the actual output names, rectangles and virtual bounds.
11. Confirm grid continuity at monitor seams, including unequal and vertically offset outputs.
12. Cover one monitor completely with a window and verify its pause.
13. Run `scripts/diagnose.sh` and inspect the `plasmashell` log.

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
