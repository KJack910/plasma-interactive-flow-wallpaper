#pragma once

#include <algorithm>

namespace XmbQuality
{
constexpr int maximum = 7;

constexpr int resolution(int quality)
{
    switch (quality)
    {
    case 0: return 96;
    case 1: return 140;
    case 3: return 220;
    case 4: return 320;
    case 5: return 440;
    case 6: return 560;
    case 7: return 720;
    default: return 180;
    }
}

constexpr int verticalResolution(int quality)
{
    // Horizontal samples beyond 4096 columns would overflow the legacy
    // density test, so horizontalResolution() keeps its own cap. Vertically
    // 480 rows is the practical ceiling before the per-frame vertex shader
    // cost outweighs any visible detail gain on 1080p-class outputs.
    switch (quality)
    {
    case 5: return 320;
    case 6: return 384;
    case 7: return 480;
    default: return std::clamp(resolution(quality), 64, 256);
    }
}
}
