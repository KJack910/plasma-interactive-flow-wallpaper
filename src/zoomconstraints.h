#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <algorithm>
#include <cmath>

namespace XmbZoom
{
constexpr qreal minimum = 0.40;
constexpr qreal maximum = 3.50;
constexpr qreal sourceLeft = -1.50;
constexpr qreal sourceRight = 2.50;
constexpr qreal sourceTop = -1.00;
constexpr qreal sourceBottom = 2.00;

inline QRectF navigationViewport(const QSizeF &surface, const QRectF &activeViewport = {})
{
    const QRectF desktop(QPointF(), QSizeF(std::max<qreal>(1.0, surface.width()),
                                         std::max<qreal>(1.0, surface.height())));
    const QRectF viewport = activeViewport.intersected(desktop);
    return viewport.isEmpty() ? desktop : viewport;
}

inline QPointF constrainNavigation(qreal zoom, const QPointF &offset,
                                   const QSizeF &surface, bool horizontalOverscan = true)
{
    zoom = std::clamp(zoom, minimum, maximum);
    const qreal w = std::max<qreal>(1.0, surface.width());
    const qreal h = std::max<qreal>(1.0, surface.height());
    const qreal xMin = horizontalOverscan ? w - zoom * sourceRight * w : std::min(0.0, w * (1.0 - zoom));
    const qreal xMax = horizontalOverscan ? -zoom * sourceLeft * w : std::max(0.0, w * (1.0 - zoom));
    return QPointF(std::clamp(offset.x(), xMin, xMax),
                   std::clamp(offset.y(), h - zoom * sourceBottom * h, -zoom * sourceTop * h));
}

inline QPointF constrainNavigation(qreal zoom, const QPointF &offset,
                                   const QSizeF &surface, const QRectF &, bool horizontalOverscan = true)
{
    return constrainNavigation(zoom, offset, surface, horizontalOverscan);
}

inline QPointF constrain(qreal zoom, const QPointF &offset, const QSizeF &surface,
                        bool horizontalOverscan = true)
{
    return constrainNavigation(zoom, offset, surface, horizontalOverscan);
}

inline QPointF zoomAt(qreal previousZoom, qreal nextZoom, const QPointF &offset,
                      const QPointF &pivot, const QSizeF &surface, bool horizontalOverscan = true)
{
    const qreal ratio = std::clamp(nextZoom, minimum, maximum)
        / std::clamp(previousZoom, minimum, maximum);
    return constrainNavigation(nextZoom, pivot - (pivot - offset) * ratio, surface, horizontalOverscan);
}

inline QPointF focusAtCursor(qreal zoom, const QPointF &cursor,
                             const QSizeF &surface, const QRectF &activeViewport,
                             bool horizontalOverscan = true)
{
    zoom = std::clamp(zoom, minimum, maximum);
    const QRectF view = navigationViewport(surface, activeViewport);
    const qreal x = std::clamp(cursor.x(), view.left(), view.right());
    const qreal y = std::clamp(cursor.y(), view.top(), view.bottom());
    QPointF target = view.center() - QPointF(x, y) * zoom;
    if (!horizontalOverscan && zoom < 1.0)
    {
        target.setX(x * (1.0 - zoom));
        return constrainNavigation(zoom, target, surface, horizontalOverscan);
    }
    const qreal localX = (x - view.left()) / view.width();
    if (view.left() <= 0.0 && localX < 0.25)
    {
        const qreal blend = std::pow((0.25 - localX) / 0.25, 2.0);
        const qreal edge = horizontalOverscan ? -zoom * sourceLeft * surface.width() : 0.0;
        target.setX(target.x() + (edge - target.x()) * blend);
    }
    if (view.right() >= surface.width() && localX > 0.75)
    {
        const qreal blend = std::pow((localX - 0.75) / 0.25, 2.0);
        const qreal edge = horizontalOverscan ? surface.width() - zoom * sourceRight * surface.width()
                                              : surface.width() * (1.0 - zoom);
        target.setX(target.x() + (edge - target.x()) * blend);
    }
    return constrainNavigation(zoom, target, surface, horizontalOverscan);
}

inline QPointF focusAtCursor(qreal zoom, const QPointF &cursor, const QSizeF &surface,
                             bool horizontalOverscan = true)
{
    return focusAtCursor(zoom, cursor, surface, {}, horizontalOverscan);
}

inline QPointF wheelTarget(qreal previousZoom, qreal nextZoom, const QPointF &offset,
                           const QPointF &cursor, const QSizeF &surface,
                           const QRectF &activeViewport, bool zoomingIn, bool navigateOnly = false,
                           bool horizontalOverscan = true)
{
    const qreal previous = std::clamp(previousZoom, minimum, maximum);
    const qreal next = std::clamp(nextZoom, minimum, maximum);
    if (navigateOnly)
        return focusAtCursor(next, cursor, surface, activeViewport, horizontalOverscan);
    if (!zoomingIn && previous <= minimum && next <= minimum)
    {
        const QPointF centered(surface.width() * (1.0 - next) * 0.5,
                               surface.height() * (1.0 - next) * 0.5);
        return constrainNavigation(next, centered, surface, horizontalOverscan);
    }
    if (zoomingIn)
        return zoomAt(previous, next, offset, cursor, surface, horizontalOverscan);

    // Zoom-out remains cursor-pivoted. The old home blend forced the camera
    // toward the global desktop centroid near the minimum zoom.
    return zoomAt(previous, next, offset, cursor, surface, horizontalOverscan);
}

inline QPointF wheelTarget(qreal previousZoom, qreal nextZoom, const QPointF &offset,
                           const QPointF &cursor, const QSizeF &surface, bool zoomingIn,
                           bool horizontalOverscan = true)
{
    return wheelTarget(previousZoom, nextZoom, offset, cursor, surface, {}, zoomingIn, false, horizontalOverscan);
}
}
