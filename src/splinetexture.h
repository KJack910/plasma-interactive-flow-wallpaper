#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace XmbSpline
{
constexpr int width = 1024;
constexpr int height = 128;
constexpr std::size_t texelCount = width * height;
using Texture = std::array<float, texelCount>;

inline float hash01(float x)
{
    const float s = std::sin(x) * 43758.5453123f;
    return s - std::floor(s);
}

inline const Texture &descriptors()
{
    static const Texture values = [] {
        Texture result;
        for (int row = 0; row < height; ++row)
        {
            const float z = (float(row) / float(height - 1)) * 2.f - 1.f;
            for (int x = 0; x < width; ++x)
            {
                const float u = float(x) / float(width - 1);
                result[std::size_t(row) * width + x] =
                    (hash01(u * 173.0f + z * 47.0f + 13.37f) * 2.f - 1.f) * 0.018f;
            }
        }
        return result;
    }();
    return values;
}

inline void generate(float flow, float *output)
{
    const auto &staticDescriptors = descriptors();
    for (int row = 0; row < height; ++row)
    {
        const float z = (float(row) / float(height - 1)) * 2.f - 1.f;
        for (int x = 0; x < width; ++x)
        {
            const float u = float(x) / float(width - 1);
            const float band = std::sin(flow * 0.25f + z * 1.7f + u * 6.2f) * 0.200f;
            const float secondary = std::cos(z * 7.0f + u * 4.8f + flow * 0.09f) * 0.025f;
            const float travel =
                std::sin((u * 3.14159265358979323846f * 1.3f + z * 0.8f) - flow * 0.25f) * 0.014f * 0.12f +
                std::sin((u * 3.14159265358979323846f * 2.8f - z * 1.2f) + flow * 0.15f) * 0.008f;
            const float perturb = 0.0998587f * 0.07f *
                                  std::sin((u * (4.0f + 0.306001f * 2.0f) + z * 4.0f - flow * 0.6f) * 4.07658f);
            const std::size_t index = std::size_t(row) * width + x;
            output[index] = 0.45f * (band + secondary + staticDescriptors[index])
                + 0.55f * (travel + perturb);
        }
    }
}
}
