#pragma once

#include <algorithm>
#include <cmath>

namespace XmbMesh
{
struct HorizontalWindow
{
    double left;
    double right;
};

inline HorizontalWindow sourceDomain(bool horizontalOverscan = true)
{
    return horizontalOverscan ? HorizontalWindow{-1.5, 2.5} : HorizontalWindow{0.0, 1.0};
}

inline int horizontalResolution(int meshResolution, double horizontalSpan,
                                bool horizontalOverscan = true)
{
    const int extended = std::clamp(int(std::ceil(meshResolution * horizontalSpan * 2.0)),
                                    meshResolution, 4096);
    return horizontalOverscan ? extended : std::max(2, int(std::ceil((extended - 1) / 4.0)) + 1);
}

struct ColumnWindow
{
    int first;
    int last;
};

inline ColumnWindow visibleColumns(const HorizontalWindow &visible, double width, int resolution,
                                   bool horizontalOverscan = true)
{
    const auto domain = sourceDomain(horizontalOverscan);
    const double left = domain.left * width;
    const double right = domain.right * width;
    const double x0 = std::max(left, visible.left);
    const double x1 = std::min(right, visible.right);
    if (x0 > x1)
        return {-1, -1};
    const double scale = double(resolution - 1) / (right - left);
    const int first = std::clamp((int(std::floor((x0 - left) * scale)) - 2) / 16 * 16,
                                 0, resolution - 2);
    const int last = std::clamp(((int(std::ceil((x1 - left) * scale)) + 17) / 16) * 16,
                                first + 1, resolution - 1);
    return {first, last};
}

inline HorizontalWindow horizontalClip(double zoom, double offsetX, double virtualWidth,
                                       double viewportLeft, double viewportWidth, int framebufferWidth)
{
    const double scale = framebufferWidth / std::max(1.0, viewportWidth);
    const double left = std::clamp((offsetX - viewportLeft) * scale, 0.0, double(framebufferWidth));
    const double right = std::clamp((offsetX + zoom * virtualWidth - viewportLeft) * scale,
                                  0.0, double(framebufferWidth));
    return {std::ceil(left), std::max(std::ceil(left), std::floor(right))};
}

// Inverse of the wave vertex shader's projectedBaseWorld.x. The shader clamps
// depth to [-1.2, 1.2] before applying its 0.095 perspective coefficient.
// This interval contains every unmodified ribbon vertex that can enter the
// output viewport, for every depth in that range.
inline HorizontalWindow visibleWorldWindow(double zoom, double offsetX,
                                           double viewportLeft, double viewportWidth,
                                           double virtualWidth, double referenceWidth,
                                           double interactionRadius, double pointerStrength,
                                           double sceneTranslationX = 0.0)
{
    zoom = std::max(zoom, 0.40);
    virtualWidth = std::max(virtualWidth, 1.0);
    const double radius = std::clamp(referenceWidth * 0.09 * interactionRadius,
                                     40.0, 320.0);
    // For the shader's Gaussian field, max(|offsetX| * influence) is below
    // 0.536 * radius. With its 0.36 stretch factor, 1.2 filament factor and
    // 1.129 maximum perspective, the screen displacement stays below roughly
    // 0.27 * radius * strength * sqrt(zoom). Use 0.50 plus 64 px, and keep a
    // 128 px floor for rasterization and interpolation between mesh columns.
    const double screenReserve = std::max(128.0,
        64.0 + 0.50 * radius * std::max(pointerStrength, 0.0) * std::sqrt(zoom));
    const double projectedLeft = (viewportLeft - offsetX - screenReserve) / zoom;
    const double projectedRight =
        (viewportLeft + viewportWidth - offsetX + screenReserve) / zoom;
    const double center = virtualWidth * 0.5;
    constexpr double perspectiveNear = 1.0 / (1.0 + 1.2 * 0.095);
    constexpr double perspectiveFar = 1.0 / (1.0 - 1.2 * 0.095);
    const double a = center + (projectedLeft - center) / perspectiveNear - sceneTranslationX;
    const double b = center + (projectedLeft - center) / perspectiveFar - sceneTranslationX;
    const double c = center + (projectedRight - center) / perspectiveNear - sceneTranslationX;
    const double d = center + (projectedRight - center) / perspectiveFar - sceneTranslationX;
    return {std::min({a, b, c, d}), std::max({a, b, c, d})};
}
}
