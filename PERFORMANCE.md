# OpenGL/Vulkan performance assessment — September 16, 2026

## System and measured load

- Plasma on two monitors, 4480×1080 virtual surface; Qt 6.11.2.
- AMD Radeon RX 5700 XT, `amdgpu`/RADV (Mesa 26.2.2); Vulkan 1.4 available.
- Preset measured on both monitors: quality 4, 9 flows (3+3+3), 30,000 3D
  particles, 60 FPS and linked global surface.
- Six-second samples were taken in active → fully paused → active order. Pause
  was temporarily applied to both outputs through the wallpaper's D-Bus bridge;
  the KWin script was then reloaded to restore the actual coverage state. No
  user setting was changed.

| Phase | plasmashell graphics engine | plasmashell CPU | Total GPU | RX 5700 XT power |
|---|---:|---:|---:|---:|
| Active A | 232 ms/s | 0.59 core | 34.1% | 41.7 W |
| Paused | 0.006 ms/s | ~0 core | 5.7% | 33.9 W |
| Active B | 212 ms/s | 0.63 core | 26.6% | 41.3 W |

The `drm-engine-gfx` counter comes from the DRM `fdinfo` file for the
`plasmashell` process. Its descriptors share the same client ID and must not be
summed. CPU is calculated from `utime+stime` deltas divided by `CLK_TCK=100`.
GPU utilization and power are seven one-second averages from `amdgpu` counters.
The active/paused comparison attributes most of the difference to the
wallpaper, but `plasmashell` also includes panels and other UI. GPU utilization
and power are whole-GPU values, not XMB-only values. The sample is short and
does not measure stabilized temperature or energy over many hours.

## Work derived from the code

At quality 4 and the current surface, the grid has 1494×256 samples and
762,448 indices for a full flow. Without viewport clipping, nine flows drawn by
two instances would produce about 13.7 million vertex references per frame, or
823 million/s at 60 FPS. This is a comparison limit, not the typical clipped
load. Each instance also recalculates a 1024×128 texture on the CPU and uploads
it every frame: together, 15.7 million texels/s or 60 MiB/s of float data.
These are work estimates, not measured frame times.

## Decision

Do not replace OpenGL with Vulkan immediately. The renderer uses
`QQuickFramebufferObject`, which Qt supports through OpenGL; the migration
would require a new scene-graph node/QRhi path and converted shaders. The Qt
Quick backend is selected by `plasmashell`, not by an individual wallpaper
instance. The current data does not show a saturated GPU and points first to
procedural CPU work and repeated mesh work.

## Reduced overscan work — geometric estimate

The renderer keeps a source four virtual desktops wide for the 0.40× minimum
zoom and edge navigation, but each monitor sends only columns whose projected
interval can enter its viewport. Additional margin starts at 128 screen pixels
and grows with deformation strength; it is rounded to blocks of 16 columns to
avoid uploads for every small movement. Pointer-follow mode uses the same
selection and includes the known translation in the inverse calculation.

Particles and the spline texture are not clipped yet: the current saving
concerns mesh vertices only and is not a measured reduction in GPU time or
power.

With 1494 columns and two outputs of 1920+2560 over a 4480-pixel virtual width,
centered camera and normal interaction, the approximate columns sent per output
are 529+673 at 0.40×, 225+289 at 1× and 81+97 at 3.50×, compared with
1494+1494 without clipping. These are geometric counts, not GPU timings, and
particles are excluded.

## Next optimization order

1. Measure the actual saving from viewport-column selection.
2. Generate/share the spline texture once per global frame or move its dynamic
   calculation to the GPU; measure CPU, GPU and power.
3. Only if driver or draw-call CPU remains a bottleneck, prototype a QRhi
   OpenGL/Vulkan path with identical shaders, FPS and monitor layout. Compare
   CPU frame time, GPU time, FPS percentiles and power at equal visual output.

Reducing spline or mesh resolution is not the first step because it may change
filaments and motion.

## September 17, 2026 — Extreme+ preset and deduplication

Measured configuration: quality 5 (Extreme+, base grid 440×320, extended
resX 2054), 30,000 3D particles, 9 flows, 60 FPS and two linked outputs of
1920+2560 on a 4480×1080 virtual desktop.

1. **Spline** (`src/splinetexture.h`): the static hash component of the
   1024×128 texture is generated once per process; each frame calculates only
   dynamic terms. Bit-level equivalence was verified over 8,519,680 texels by
   `xmb_splineequivalence`; the CPU calculation improved by approximately
   35–36% (about 3.0 ms to 1.95 ms per frame).
2. **Mesh deduplication** (`src/meshgeometry.h`): each grid vertex is stored
   once and indices rebuild the same strips and joins. The
   `xmb_meshgeometry` test verifies 20.9 million identical indexed coordinates.
   Extreme+ VBO per output decreased from 10,483,616 to 5,258,240 bytes
   (−49.8%). Triangles, clipping, perspective and zoom are unchanged.
3. **Post-installation measurement**: after restarting `plasmashell` and loading
   the new libraries, the `plasmashell` GFX engine from DRM `fdinfo` was 12.3%
   stable across three 6–10 second samples. The earlier 35–40% value was total
   GPU usage, including KWin and other applications, and is not directly
   comparable. At the same metric, the reduction in vertices and CPU work is
   included in this value.

## References

- [QQuickFramebufferObject](https://doc.qt.io/qt-6/qquickframebufferobject.html)
- [QRhi](https://doc.qt.io/qt-6/qrhi.html)
- [Qt Quick scene-graph renderer selection](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html)
