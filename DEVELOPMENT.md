# Development notes

## Mapping from web v24 to native renderer

The native renderer intentionally preserves these v24 values:

- wave flow speed: `0.125`
- particle flow speed: `0.16`
- particles: `4200`
- wave opacity: `0.74`
- second wave opacity: `0.74 * 0.62`
- second wave time offset: `+0.95 s`
- second wave Y offset: `-0.070`
- wave height scale: `0.68`
- FFD Y/Z amplitudes: `0.075 / 0.078`
- z detail: `0.095`
- Ultra mesh: `220 × 220`, indexed with `uint32`

The dual wave and particle GLSL equations are ports of the v24 WebGL shaders.
The browser-only reverse carrier generator is replaced with a native 256×64
procedural float texture so no JavaScript runtime is needed inside Plasma.

## Input state

The GUI thread runs the same click state model used in v24:

```text
hover present + button released -> target = 0.82 * mouseStrength
left button held               -> target = 0
pointer absent                  -> target = 0
```

Fade-out rate is 11/s and fade-in rate is 3/s. Pointer motion does not reset
that recovery state.

## Build layout

The shared library is built directly into:

`package/contents/ui/xmbnative/libxmbnativeplugin.so`

The wallpaper package therefore contains its native QML plugin when
`kpackagetool6` installs it.


- v1.0.9 aggiunge zoom con rotella, interazione del mouse non invertita e flussi multipli configurabili sopra/sotto.


- v1.0.9: mouse effect restored to v1.0.2 behavior; wheel zoom and multi-flow controls are retained.


### v1.0.9 flow zones
Upper, center and lower flow counts are now independent. Upper/lower bands are placed in the actual upper/lower thirds of the screen; center bands remain around the center. Mouse deformation logic remains the v1.0.2/v1.0.4 behavior.


### v1.0.9 renderer changes

The wave shader can use normalized virtual-desktop X coordinates. Particle seeds are assigned to active flow lanes and, when multiscreen linking is enabled, are generated in the shared virtual X domain so they are not duplicated per monitor. Zoom uses a cursor-space pivot passed to both wave and particle shaders.


### v1.0.9

- same mouse deformation formula, but its source offset points toward the cursor;
- wheel zoom step reduced and interpolation slowed; configurable wheel sensitivity added;
- multiscreen X mapping now uses the actual global bounds of each wallpaper item, with a QScreen fallback, to remove phase disagreement at monitor seams.


### v1.0.9 interaction rollback

The interaction equations are intentionally frozen to the v1.0.5 form. Any
future pointer-orientation correction should happen before the shader input,
not by changing the deformation equations themselves.


## v1.1.0 virtual-surface multiscreen contract

The renderer no longer projects an independent mesh on every output. Each
wallpaper instance receives `viewportOriginPx`, `viewportSizePx`,
`virtualSizePx`, and `referenceSizePx`. It builds the same complete virtual
mesh, applies perspective, interaction and zoom in shared scene coordinates,
then converts the projected world position into the local output NDC. The
framebuffer clips the result to that output.

Particles use the same final world-to-viewport transform. The cursor and wheel
pivot are expressed in virtual-desktop pixels, and every linked instance sees
the application-wide wheel event. `QScreen::virtualSiblings()` and
`virtualGeometry()` define the shared 2D logical-pixel coordinate system.

The horizontal mesh resolution is multiplied by
`virtualWidth/referenceWidth` (capped at 768 columns) so spanning more outputs
does not reduce tessellation density at seams.
