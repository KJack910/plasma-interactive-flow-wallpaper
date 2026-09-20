# Development notes

## Renderer baseline

The native renderer preserves the principal values of the web v24 reference:

- wave flow speed: `0.125`;
- particle flow speed: `0.16`;
- default particle count: `4200` in the renderer baseline;
- wave opacity: `0.74`;
- second wave opacity: `0.74 * 0.62`;
- second wave time offset: `+0.95 s`;
- second wave Y offset: `-0.070`;
- wave height scale: `0.68`;
- FFD Y/Z amplitudes: `0.075 / 0.078`;
- z detail: `0.095`;
- Ultra mesh: `220 × 220`, indexed with `uint32`.

The dual-wave and particle GLSL equations are native ports of the v24 WebGL
shaders. The browser-only reverse-carrier generator is replaced by a native
procedural float texture, so no JavaScript runtime is required inside Plasma.

## Pointer state

The GUI thread uses the same state model as the reference implementation:

```text
hover present + button released -> target = 0.82 * mouseStrength
left button held               -> target = 0
pointer absent                  -> target = 0
```

Fade-out rate is 11/s and fade-in rate is 3/s. Pointer motion does not reset
the recovery state.

## Build layout

The shared library is built directly into:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

The wallpaper package therefore contains its native QML plugin when installed
with `kpackagetool6`.

## Version history summary

Detailed release notes belong in `CHANGELOG.md`; this section only records the
architectural milestones to avoid duplicating release descriptions:

- 1.0.x: native renderer foundation, Qt 6 registration fixes and baseline
  interaction behavior.
- 1.1.0: cursor-pivot zoom, flow groups, linked virtual-desktop mapping,
  per-output pause, optimized mesh/spline paths and system-usage monitoring.

## Virtual-surface contract

The renderer does not project an independent mesh on every output. Each
wallpaper instance receives `viewportOriginPx`, `viewportSizePx`, `virtualSizePx`
and `referenceSizePx`. It builds the same virtual mesh, applies perspective,
interaction and zoom in shared scene coordinates, then converts the projected
world position into local output NDC. The framebuffer clips the result to that
output.

Particles use the same world-to-viewport transform. Cursor and wheel pivots are
expressed in virtual-desktop pixels, and linked instances share the application
wheel event. `QScreen::virtualSiblings()` and `virtualGeometry()` define the
shared logical-pixel coordinate system.

Horizontal mesh resolution scales with
`virtualWidth/referenceWidth`, capped at 768 columns, so spanning additional
outputs does not reduce tessellation density at seams.
