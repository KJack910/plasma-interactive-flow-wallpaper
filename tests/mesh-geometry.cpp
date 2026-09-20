#include "meshgeometry.h"
#include "meshquality.h"
#include "meshvisibility.h"

#include <array>
#include <cstring>
#include <iostream>
#include <utility>

int main()
{
    std::size_t checked = 0;
    for (int quality = 0; quality <= XmbQuality::maximum; ++quality)
    {
        for (double span : {1.0, 4480.0 / 1920.0, 20.0})
        {
            for (bool overscan : {false, true})
            {
                const int nx = XmbMesh::horizontalResolution(XmbQuality::resolution(quality), span, overscan);
                const int ny = XmbQuality::verticalResolution(quality);
                const auto vertices = XmbMesh::gridVertices(nx, ny);
                if (vertices.size() != std::size_t(nx) * ny * 2)
                    return 1;
                std::vector<float> legacy;
                for (int y = 0; y < ny - 1; ++y)
                {
                    for (int x = 0; x < nx; ++x)
                    {
                        const float fx = (float(x) / float(nx - 1)) * 2.f - 1.f;
                        legacy.push_back(fx);
                        legacy.push_back((float(y + 1) / float(ny - 1)) * 2.f - 1.f);
                        legacy.push_back(fx);
                        legacy.push_back((float(y) / float(ny - 1)) * 2.f - 1.f);
                    }
                }
                for (auto [first, last] : {std::pair{0, nx - 1}, std::pair{0, 1},
                                          std::pair{nx - 2, nx - 1}, std::pair{nx / 3, nx / 2}})
                {
                    const auto indices = XmbMesh::gridIndices(nx, ny, first, last);
                    std::vector<std::uint32_t> oldIndices;
                    for (int strip = 0; strip < ny - 1; ++strip)
                    {
                        const auto base = std::uint32_t(strip * nx * 2);
                        if (strip > 0)
                        {
                            oldIndices.push_back(oldIndices.back());
                            oldIndices.push_back(base + std::uint32_t(first * 2));
                        }
                        for (int x = first; x <= last; ++x)
                        {
                            oldIndices.push_back(base + std::uint32_t(x * 2));
                            oldIndices.push_back(base + std::uint32_t(x * 2 + 1));
                        }
                    }
                    if (indices.size() != oldIndices.size())
                        return 2;
                    for (std::size_t i = 0; i < indices.size(); ++i)
                    {
                        const auto current = std::size_t(indices[i]) * 2;
                        const auto previous = std::size_t(oldIndices[i]) * 2;
                        if (current + 1 >= vertices.size() || previous + 1 >= legacy.size()
                            || std::memcmp(vertices.data() + current, legacy.data() + previous,
                                           2 * sizeof(float)) != 0)
                        {
                            std::cerr << "Mesh mismatch: " << nx << 'x' << ny << " index " << i << '\n';
                            return 3;
                        }
                        ++checked;
                    }
                }
                if (!XmbMesh::gridIndices(nx, ny, -1, -1).empty())
                    return 4;
            }
        }
    }
    const int nx = XmbMesh::horizontalResolution(440, 4480.0 / 1920.0, true);
    const int ny = XmbQuality::verticalResolution(5);
    const auto before = std::size_t(nx) * (ny - 1) * 2 * 2 * sizeof(float);
    const auto after = XmbMesh::gridVertices(nx, ny).size() * sizeof(float);
    std::cout << "Identical indexed coordinates: " << checked
              << "; Extreme+ VBO bytes/output: " << before << " -> " << after << '\n';
}
