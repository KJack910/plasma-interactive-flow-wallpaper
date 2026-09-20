#include "zoomconstraints.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {
void same(const QPointF &a, const QPointF &b)
{
    assert(std::abs(a.x() - b.x()) < 1e-7);
    assert(std::abs(a.y() - b.y()) < 1e-7);
}
void bounded(qreal z, const QPointF &offset, const QSizeF &surface)
{
    const qreal left = -offset.x() / z;
    const qreal right = (surface.width() - offset.x()) / z;
    const qreal top = -offset.y() / z;
    const qreal bottom = (surface.height() - offset.y()) / z;
    assert(left >= -1.5 * surface.width() - 1e-7);
    assert(right <= 2.5 * surface.width() + 1e-7);
    assert(top >= -surface.height() - 1e-7);
    assert(bottom <= 2.0 * surface.height() + 1e-7);
}
}

int main()
{
    const QSizeF dual(4480, 1080);
    const QRectF first(0, 0, 1920, 1080);
    const QRectF second(1920, 0, 2560, 1080);
    same(XmbZoom::focusAtCursor(0.4, {960, 540}, dual, first), {576, 324});
    same(XmbZoom::focusAtCursor(0.4, {3200, 540}, dual, second), {1920, 324});
    same(XmbZoom::focusAtCursor(0.4, {0, 540}, dual, first), {2688, 324});
    same(XmbZoom::focusAtCursor(0.4, {4480, 540}, dual, second), {0, 324});
    for (const QSizeF surface : {QSizeF(1920, 1080), dual, QSizeF(1920, 2160)}) {
        const QRectF full(QPointF(), surface);
        for (qreal z : {0.4, 0.65, 1.0, 2.0, 3.5}) {
            const QPointF centered(surface.width() * (1.0 - z) * 0.5,
                                   surface.height() * (1.0 - z) * 0.5);
            for (const QRectF requested : {full, first, second, QRectF(0, 1080, 1920, 1080)}) {
                const QRectF view = XmbZoom::navigationViewport(surface, requested);
                for (int i = 0; i <= 20; ++i) {
                    const QPointF cursor(view.left() + view.width() * i / 20.0,
                                         view.top() + view.height() * i / 20.0);
                    const auto pan = XmbZoom::wheelTarget(z, z, centered, cursor, surface, view, true, true);
                    same(pan, XmbZoom::focusAtCursor(z, cursor, surface, view));
                    bounded(z, pan, surface);
                    const qreal next = std::min(z + 0.01, 3.5);
                    const auto after = XmbZoom::wheelTarget(z, next, centered, cursor, surface, view, true);
                    same((cursor - after) / next, (cursor - centered) / z);
                    bounded(next, after, surface);
                    same(after, XmbZoom::wheelTarget(z, next, centered, cursor, surface, full, true));
                    same(XmbZoom::wheelTarget(z, z, centered, cursor, surface, view, true), centered);
                    const auto zoomedOut = XmbZoom::wheelTarget(z, 0.4, pan, cursor, surface, view, false);
                    const QPointF expectedZoomedOut = z <= XmbZoom::minimum
                        ? QPointF(surface.width() * (1.0 - 0.4) * 0.5,
                                  surface.height() * (1.0 - 0.4) * 0.5)
                        : XmbZoom::zoomAt(z, 0.4, pan, cursor, surface);
                    same(zoomedOut, expectedZoomedOut);
                    same(XmbZoom::wheelTarget(0.4, 0.4, zoomedOut, cursor, surface, view, false),
                         QPointF(surface.width() * (1.0 - 0.4) * 0.5,
                                 surface.height() * (1.0 - 0.4) * 0.5));
                }
            }
            for (const QPointF extreme : {QPointF(-1e9, -1e9), QPointF(1e9, 1e9)}) {
                const auto offset = XmbZoom::constrainNavigation(z, extreme, surface);
                bounded(z, offset, surface);
                same(XmbZoom::constrainNavigation(z, offset, surface), offset);
            }
        }
    }
    for (qreal z : {0.4, 1.0, 3.5}) {
        const auto left = XmbZoom::focusAtCursor(z, {0, 540}, dual, first);
        const auto right = XmbZoom::focusAtCursor(z, {4480, 540}, dual, second);
        assert(std::abs(-left.x() / z + 1.5 * 4480) < 1e-7);
        assert(std::abs((4480 - right.x()) / z - 2.5 * 4480) < 1e-7);
    }
    const QPointF cursor(2240, 540);
    const QPointF drift(100, 30);
    const QPointF zoomOutPivot = XmbZoom::zoomAt(1.0, 0.70, drift, cursor, dual);
    same(XmbZoom::wheelTarget(1.0, 0.70, drift, cursor, dual, second, false), zoomOutPivot);
    const QPointF minimumCenter(dual.width() * (1.0 - 0.4) * 0.5,
                                dual.height() * (1.0 - 0.4) * 0.5);
    for (int i = 0; i < 20; ++i) {
        same(XmbZoom::wheelTarget(0.4, 0.4, drift, cursor, dual, second, false), minimumCenter);
    }
    for (qreal next : {0.99, 0.85, 0.70, 0.55, 0.40}) {
        const QPointF pivoted = XmbZoom::zoomAt(1.0, next, drift, cursor, dual);
        const QPointF result = XmbZoom::wheelTarget(1.0, next, drift, cursor, dual, second, false);
        same(result, pivoted);
        bounded(next, result, dual);
    }
    same(XmbZoom::wheelTarget(1.0, 0.70, drift, cursor, dual, second, false),
         XmbZoom::zoomAt(1.0, 0.70, drift, cursor, dual));
    const QPointF preserved = {1800, 400};
    same(XmbZoom::wheelTarget(0.4, 0.4, preserved, cursor, dual, second, false), minimumCenter);
    for (int i = 0; i < 20; ++i) {
        same(XmbZoom::wheelTarget(0.4, 0.4, preserved, cursor, dual, second, false), minimumCenter);
    }
    for (const QSizeF surface : {QSizeF(1920, 1080), dual, QSizeF(1920, 2160)}) {
        const QRectF full(QPointF(), surface);
        for (qreal z : {0.4, 0.65, 0.99, 1.0, 1.01, 2.0, 3.5}) {
            const qreal edge = surface.width() * (1.0 - z);
            const QPointF centered(edge * 0.5, surface.height() * (1.0 - z) * 0.5);
            for (const QPointF extreme : {QPointF(-1e9, -1e9), QPointF(1e9, 1e9)}) {
                const auto legacy = XmbZoom::constrainNavigation(z, extreme, surface);
                same(legacy, XmbZoom::constrainNavigation(z, extreme, surface, true));
                const auto limited = XmbZoom::constrainNavigation(z, extreme, surface, false);
                assert(limited.x() == (extreme.x() < 0 ? std::min(0.0, edge) : std::max(0.0, edge)));
                assert(limited.y() == legacy.y());
                same(limited, XmbZoom::constrainNavigation(z, limited, surface, false));
                same(limited, XmbZoom::constrain(z, extreme, surface, false));
                same(limited, XmbZoom::constrainNavigation(z, extreme, surface, full, false));
                same(limited, XmbZoom::constrainNavigation(z,
                    XmbZoom::constrainNavigation(z, limited, surface, true), surface, false));
            }
            for (const QRectF requested : {full, first, second, QRectF(0, 1080, 1920, 1080)}) {
                const QRectF view = XmbZoom::navigationViewport(surface, requested);
                for (int i = 0; i <= 20; ++i) {
                    const QPointF cursor(view.left() + view.width() * i / 20.0,
                                         view.top() + view.height() * i / 20.0);
                    const auto legacy = XmbZoom::focusAtCursor(z, cursor, surface, view);
                    same(legacy, XmbZoom::focusAtCursor(z, cursor, surface, view, true));
                    const auto focus = XmbZoom::focusAtCursor(z, cursor, surface, view, false);
                    assert(focus.x() >= std::min(0.0, edge) - 1e-7);
                    assert(focus.x() <= std::max(0.0, edge) + 1e-7);
                    assert(focus.y() == legacy.y());
                    if (z < 1.0)
                        assert(std::abs((cursor.x() - focus.x()) / z - cursor.x()) < 1e-7);
                    same(focus, XmbZoom::wheelTarget(z, z, centered, cursor, surface, view, true, true, false));
                    same(XmbZoom::focusAtCursor(z, cursor, surface, false),
                         XmbZoom::focusAtCursor(z, cursor, surface, full, false));
                    for (bool inward : {false, true}) {
                        const qreal next = std::clamp(z + (inward ? 0.01 : -0.01), 0.4, 3.5);
                        const auto wheel = XmbZoom::wheelTarget(z, next, focus, cursor, surface,
                                                                view, inward, false, false);
                        const QPointF expected = !inward && z <= XmbZoom::minimum
                            ? QPointF(surface.width() * (1.0 - next) * 0.5,
                                      surface.height() * (1.0 - next) * 0.5)
                            : XmbZoom::zoomAt(z, next, focus, cursor, surface, false);
                        same(wheel, expected);
                        same(wheel, XmbZoom::constrainNavigation(next, wheel, surface, false));
                    }
                    const QPointF expectedMinimum = z <= XmbZoom::minimum
                        ? QPointF(surface.width() * (1.0 - 0.4) * 0.5,
                                  surface.height() * (1.0 - 0.4) * 0.5)
                        : XmbZoom::zoomAt(z, 0.4, focus, cursor, surface, false);
                    same(XmbZoom::wheelTarget(z, 0.4, focus, cursor, surface, view,
                                               false, false, false), expectedMinimum);
                }
            }
            const auto left = XmbZoom::focusAtCursor(z, {0, 540}, surface, false);
            const auto right = XmbZoom::focusAtCursor(z, {surface.width(), 540}, surface, false);
            assert(std::abs(left.x()) < 1e-7);
            assert(std::abs(right.x() - edge) < 1e-7);
        }
    }
    same(XmbZoom::focusAtCursor(0.4, {960, 540}, dual, first, false), {576, 324});
    same(XmbZoom::focusAtCursor(0.4, {3200, 540}, dual, second, false), {1920, 324});
    same(XmbZoom::focusAtCursor(0.4, {1920, 540}, dual, first, false),
         XmbZoom::focusAtCursor(0.4, {1920, 540}, dual, second, false));
    std::cout << "Zoom: legacy/default true unchanged; overscan off real X edges, monitor focus, Y parity and smooth return OK\n";
}
