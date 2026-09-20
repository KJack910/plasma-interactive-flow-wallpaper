#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace XmbMesh
{
inline std::vector<float> gridVertices(int resolutionX, int resolutionY)
{
    std::vector<float> vertices;
    vertices.reserve(std::size_t(resolutionX) * resolutionY * 2);
    for (int y = 0; y < resolutionY; ++y)
    {
        for (int x = 0; x < resolutionX; ++x)
        {
            vertices.push_back((float(x) / float(resolutionX - 1)) * 2.f - 1.f);
            vertices.push_back((float(y) / float(resolutionY - 1)) * 2.f - 1.f);
        }
    }
    return vertices;
}

inline std::vector<std::uint32_t> gridIndices(int resolutionX, int resolutionY,
                                            int first, int last)
{
    std::vector<std::uint32_t> indices;
    if (first < 0 || last <= first || last >= resolutionX || resolutionY < 2)
        return indices;
    const int strips = resolutionY - 1;
    indices.reserve(std::size_t(strips) * ((last - first + 1) * 2 + 2) - 2);
    for (int strip = 0; strip < strips; ++strip)
    {
        const auto upper = std::uint32_t((strip + 1) * resolutionX);
        const auto lower = std::uint32_t(strip * resolutionX);
        if (strip > 0)
        {
            indices.push_back(indices.back());
            indices.push_back(upper + std::uint32_t(first));
        }
        for (int x = first; x <= last; ++x)
        {
            indices.push_back(upper + std::uint32_t(x));
            indices.push_back(lower + std::uint32_t(x));
        }
    }
    return indices;
}
}
