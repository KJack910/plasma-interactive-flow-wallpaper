#include "meshvisibility.h"
#include "meshquality.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    static_assert(XmbQuality::maximum == 7);
    static_assert(XmbQuality::resolution(5) == 440);
    static_assert(XmbQuality::verticalResolution(5) == 320);
    static_assert(XmbQuality::resolution(6) == 560);
    static_assert(XmbQuality::verticalResolution(6) == 384);
    static_assert(XmbQuality::resolution(7) == 720);
    static_assert(XmbQuality::verticalResolution(7) == 480);
    const int previousResolutions[] = {96, 140, 180, 220, 320};
    const int previousVertical[] = {96, 140, 180, 220, 256};
    for (int quality = 0; quality < 5; ++quality) {
        assert(XmbQuality::resolution(quality) == previousResolutions[quality]);
        assert(XmbQuality::verticalResolution(quality) == previousVertical[quality]);
    }
    constexpr double world = 4480.0;
    constexpr double nearFactor = 1.0 / (1.0 + 1.2 * 0.095);
    constexpr double farFactor = 1.0 / (1.0 - 1.2 * 0.095);
    for (double zoom : {0.40, 1.0, 2.0, 3.50})
    {
        for (double offset : {world * (1.0 - zoom) * 0.5,
                              -zoom * 1.5 * world,
                              world - zoom * 2.5 * world})
        {
            for (const auto viewport : {std::pair{0.0, 1920.0},
                                        std::pair{1920.0, 2560.0}})
            {
                for (double translation : {-world * 0.5, 0.0, world * 0.5})
                {
                    const auto visible = XmbMesh::visibleWorldWindow(
                        zoom, offset, viewport.first, viewport.second,
                        world, 1920.0, 2.5, 3.28, translation);
                    assert(visible.left < visible.right);
                    for (int i = 0; i <= 2000; ++i)
                    {
                        const double source = -1.5 * world + 4.0 * world * i / 2000.0;
                        for (double perspective : {nearFactor, 1.0, farFactor})
                        {
                            const double projected = world * 0.5
                                + (source - world * 0.5 + translation) * perspective;
                            const double screen = projected * zoom + offset;
                            if (screen >= viewport.first &&
                                screen <= viewport.first + viewport.second)
                            {
                                assert(source >= visible.left - 1e-7);
                                assert(source <= visible.right + 1e-7);
                            }
                        }
                    }
                }
            }
        }
    }
    // At ordinary strength the new perspective-aware range must be narrower
    // than the former fixed 0.40-world-width guard on both sides.
    const auto typical = XmbMesh::visibleWorldWindow(
        2.0, -world * 0.5, 1920.0, 2560.0, world, 1920.0, 1.0, 0.82);
    const double oldWidth = 2560.0 / 2.0 + 0.80 * world;
    assert(typical.right - typical.left < oldWidth);
    for (int quality : {96, 140, 180, 220, 320, 440, 560, 720}) {
        for (double span : {1.0, world / 1920.0, 20.0}) {
            const int legacy = std::clamp(int(std::ceil(quality * span * 2.0)), quality, 4096);
            assert(XmbMesh::horizontalResolution(quality, span) == legacy);
            assert(XmbMesh::horizontalResolution(quality, span, true) == legacy);
            const int limited = XmbMesh::horizontalResolution(quality, span, false);
            assert(limited - 1 >= (legacy - 1) / 4.0);
            assert(limited - 1 < (legacy - 1) / 4.0 + 1.0);
        }
    }
    for (bool overscan : {true, false}) {
        const auto domain = XmbMesh::sourceDomain(overscan);
        assert(domain.left == (overscan ? -1.5 : 0.0));
        assert(domain.right == (overscan ? 2.5 : 1.0));
        const int resolution = XmbMesh::horizontalResolution(220, world / 1920.0, overscan);
        for (double zoom : {0.4, 0.65, 1.0, 2.0, 3.5}) {
            for (double offset : {0.0, world * (1.0 - zoom) * 0.5, world * (1.0 - zoom)}) {
                for (const auto viewport : {std::pair{0.0, 1920.0}, std::pair{1920.0, 2560.0}}) {
                    for (double translation : {-world * 0.5, 0.0, world * 0.5}) {
                        const auto visible = XmbMesh::visibleWorldWindow(zoom, offset,
                            viewport.first, viewport.second, world, 1920.0, 2.5, 3.28, translation);
                        const auto columns = XmbMesh::visibleColumns(visible, world, resolution, overscan);
                        if (columns.first < 0) {
                            assert(visible.right < domain.left * world || visible.left > domain.right * world);
                            continue;
                        }
                        assert(columns.first >= 0 && columns.last < resolution);
                        assert(columns.first < columns.last);
                        const double firstX = (domain.left + double(columns.first) / (resolution - 1)
                            * (domain.right - domain.left)) * world;
                        const double lastX = (domain.left + double(columns.last) / (resolution - 1)
                            * (domain.right - domain.left)) * world;
                        assert(firstX <= std::max(domain.left * world, visible.left) + 1e-7);
                        assert(lastX >= std::min(domain.right * world, visible.right) - 1e-7);
                        for (int i = 0; i < resolution; ++i) {
                            const double source = (domain.left + double(i) / (resolution - 1)
                                * (domain.right - domain.left)) * world;
                            assert(source >= domain.left * world - 1e-7 && source <= domain.right * world + 1e-7);
                            for (double perspective : {nearFactor, 1.0, farFactor}) {
                                const double projected = (world * 0.5
                                    + (source - world * 0.5 + translation) * perspective) * zoom + offset;
                                if (projected >= viewport.first && projected <= viewport.first + viewport.second)
                                    assert(i >= columns.first && i <= columns.last);
                            }
                        }
                    }
                    for (double dpr : {1.0, 1.25, 2.0}) {
                        const int pixels = int(viewport.second * dpr);
                        const auto clip = XmbMesh::horizontalClip(zoom, offset, world,
                            viewport.first, viewport.second, pixels);
                        assert(clip.left >= 0 && clip.right <= pixels && clip.left <= clip.right);
                        for (int pixel = int(clip.left); pixel < int(clip.right); ++pixel) {
                            const double screen = viewport.first + (pixel + 0.5) / dpr;
                            assert(screen >= offset && screen <= offset + zoom * world);
                        }
                    }
                }
            }
        }
        const auto empty = XmbMesh::visibleColumns({world * 3, world * 4}, world, resolution, overscan);
        assert(empty.first == -1 && empty.last == -1);
    }
    const auto clip = XmbMesh::horizontalClip(0.4, 1344, world, 1920, 2560, 3200);
    assert(clip.left == 0 && clip.right == 1520);
    std::cout << "Mesh: perspective visibility, source domains, density and horizontal-only clipping OK\n";
}
