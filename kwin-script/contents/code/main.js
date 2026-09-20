"use strict";

const service = "org.xmbflow.interactive";
const objectPath = "/XmbFlow";
const interfaceName = "org.xmbflow.Fullscreen";
const frozenOutputs = new Map();
const trackedWindows = new Set();
const observedOutputs = new Map();

function notify(output, frozen) {
    if (output && output.name) {
        console.info("xmbfullscreenbridge: " + output.name + " " + (frozen ? "freeze" : "resume"));
        callDBus(service, objectPath, interfaceName, "setOutputFrozen", output.name, frozen);
    }
}

function coversOutput(window, output) {
    const outputGeometry = output.geometry;
    const maximizeGeometry = workspace.clientArea(
        KWin.MaximizeFull, output, workspace.currentDesktop);
    const frame = window.frameGeometry;
    const containsGeometry = (geometry) => geometry && frame
        && frame.x <= geometry.x + 1.0
        && frame.y <= geometry.y + 1.0
        && frame.x + frame.width >= geometry.x + geometry.width - 1.0
        && frame.y + frame.height >= geometry.y + geometry.height - 1.0;
    return containsGeometry(outputGeometry) || containsGeometry(maximizeGeometry);
}

function onCurrentDesktop(window, output) {
    if (window.onAllDesktops || !window.desktops || window.desktops.length === 0)
        return true;
    const current = workspace.currentDesktopForScreen
        ? workspace.currentDesktopForScreen(output) : workspace.currentDesktop;
    for (const desktop of window.desktops) {
        if (desktop === current)
            return true;
    }
    return false;
}

function onCurrentActivity(window) {
    if (!window.activities || window.activities.length === 0)
        return true;
    for (const activity of window.activities) {
        if (activity === workspace.currentActivity)
            return true;
    }
    return false;
}

function outputCoveredByWindows(windows, output) {
    const visible = windows.filter(window => window.normalWindow && !window.deleted
        && !window.hidden && !window.minimized
        && (window.opacity === undefined || window.opacity >= 0.995)
        && onCurrentDesktop(window, output) && onCurrentActivity(window));
    if (visible.some(window => coversOutput(window, output)))
        return true;

    // Multiple tiled windows can hide the entire usable desktop even though
    // no single window is maximized. Test their geometric union, not summed
    // area (which could count overlaps while leaving a visible gap).
    const area = workspace.clientArea(KWin.MaximizeFull, output, workspace.currentDesktop);
    if (!area || area.width <= 0 || area.height <= 0)
        return false;
    const left = area.x;
    const right = area.x + area.width;
    const top = area.y;
    const bottom = area.y + area.height;
    const rectangles = visible.map(window => window.frameGeometry).filter(Boolean);
    const edges = [left, right];
    for (const rectangle of rectangles) {
        edges.push(Math.max(left, Math.min(right, rectangle.x)));
        edges.push(Math.max(left, Math.min(right, rectangle.x + rectangle.width)));
    }
    edges.sort((a, b) => a - b);
    for (let i = 1; i < edges.length; ++i) {
        if (edges[i] - edges[i - 1] <= 0.01)
            continue;
        const middle = (edges[i] + edges[i - 1]) * 0.5;
        const intervals = rectangles.filter(rectangle => rectangle.x <= middle
            && rectangle.x + rectangle.width >= middle)
            .map(rectangle => [Math.max(top, rectangle.y),
                               Math.min(bottom, rectangle.y + rectangle.height)])
            .sort((a, b) => a[0] - b[0]);
        let coveredUntil = top;
        for (const interval of intervals) {
            if (interval[0] > coveredUntil + 1.0)
                return false;
            coveredUntil = Math.max(coveredUntil, interval[1]);
            if (coveredUntil >= bottom - 1.0)
                break;
        }
        if (coveredUntil < bottom - 1.0)
            return false;
    }
    return rectangles.length > 0;
}

function recomputeOutputs() {
    // KWin can emit windowAdded before the final window type/geometry exists.
    // Refresh subscriptions from the authoritative live list on each event.
    for (const window of workspace.windowList())
        trackWindow(window);

    const currentOutputs = new Set();
    for (const output of workspace.screens) {
        currentOutputs.add(output.name);
        if (observedOutputs.get(output.name) !== output) {
            observedOutputs.set(output.name, output);
            output.geometryChanged.connect(recomputeOutputs);
        }
        const frozen = outputCoveredByWindows(Array.from(trackedWindows), output);
        // Send the initial false state too: a restarted bridge may have kept
        // an older true state for this output.
        if (!frozenOutputs.has(output.name) || frozen !== frozenOutputs.get(output.name)) {
            frozenOutputs.set(output.name, frozen);
            notify(output, frozen);
        }
    }
    for (const outputName of frozenOutputs.keys()) {
        if (!currentOutputs.has(outputName)) {
            frozenOutputs.delete(outputName);
            observedOutputs.delete(outputName);
        }
    }
}

function updateWindow() {
    recomputeOutputs();
}

function trackWindow(window) {
    if (!window || trackedWindows.has(window)) {
        return;
    }
    trackedWindows.add(window);
    window.fullScreenChanged.connect(updateWindow);
    window.outputChanged.connect(updateWindow);
    window.minimizedChanged.connect(updateWindow);
    if (window.maximizedChanged)
        window.maximizedChanged.connect(updateWindow);
    if (window.opacityChanged)
        window.opacityChanged.connect(updateWindow);
    if (window.noBorderChanged)
        window.noBorderChanged.connect(updateWindow);
    window.hiddenChanged.connect(updateWindow);
    window.desktopsChanged.connect(updateWindow);
    window.activitiesChanged.connect(updateWindow);
    window.frameGeometryChanged.connect(updateWindow);
    if (window.readyForPaintingChanged)
        window.readyForPaintingChanged.connect(updateWindow);
    if (window.surfaceChanged)
        window.surfaceChanged.connect(updateWindow);
    if (window.windowRoleChanged)
        window.windowRoleChanged.connect(updateWindow);
    if (window.windowClassChanged)
        window.windowClassChanged.connect(updateWindow);
    if (window.visibleGeometryChanged)
        window.visibleGeometryChanged.connect(updateWindow);
    if (window.windowShown)
        window.windowShown.connect(updateWindow);
    if (window.windowHidden)
        window.windowHidden.connect(updateWindow);
}

workspace.windowList().forEach(trackWindow);
workspace.windowAdded.connect(window => {
    trackWindow(window);
    recomputeOutputs();
});
workspace.windowRemoved.connect(window => {
    trackedWindows.delete(window);
    recomputeOutputs();
});
workspace.windowActivated.connect(recomputeOutputs);
if (workspace.stackingOrderChanged)
    workspace.stackingOrderChanged.connect(recomputeOutputs);
workspace.screensChanged.connect(recomputeOutputs);
workspace.currentDesktopChanged.connect(recomputeOutputs);
workspace.currentActivityChanged.connect(recomputeOutputs);
if (workspace.showingDesktopChanged)
    workspace.showingDesktopChanged.connect(recomputeOutputs);
recomputeOutputs();
