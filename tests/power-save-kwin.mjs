import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { runInNewContext } from "node:vm";

function signal() {
    const listeners = [];
    return {
        connect(callback) { listeners.push(callback); },
        emit(...args) { for (const callback of listeners) callback(...args); }
    };
}

function output(name, x) {
    return { name, geometry: { x, y: 0, width: 100, height: 100 }, geometryChanged: signal() };
}

function applicationWindow(x, width = 100) {
    const window = {
        x, y: 0, width, height: 100,
        frameGeometry: { x, y: 0, width, height: 100 },
        normalWindow: true, hidden: false, minimized: false,
        fullScreen: false, noBorder: false,
        desktops: [], activities: [],
        fullScreenChanged: signal(), outputChanged: signal(),
        minimizedChanged: signal(), maximizedChanged: signal(), hiddenChanged: signal(),
        desktopsChanged: signal(), activitiesChanged: signal(),
        frameGeometryChanged: signal(), readyForPaintingChanged: signal(),
        windowShown: signal(), windowHidden: signal(), closed: signal()
    };
    return window;
}

const first = output("DP-1", 0);
const second = output("DP-2", 100);
const calls = [];
const liveWindows = [];
const workspace = {
    screens: [first, second], currentDesktop: {}, currentActivity: "default",
    windowList: () => liveWindows,
    clientArea: (_type, screen) => screen === second
        ? { x: 100, y: 0, width: 100, height: 95 }
        : screen.geometry,
    windowAdded: signal(), windowRemoved: signal(), windowActivated: signal(),
    stackingOrderChanged: signal(),
    screensChanged: signal(), currentDesktopChanged: signal(),
    currentActivityChanged: signal()
};
const code = readFileSync(new URL("../kwin-script/contents/code/main.js", import.meta.url), "utf8");
runInNewContext(code, {
    workspace, KWin: { MaximizeFull: 1 },
    callDBus: (_service, _path, _interface, _method, screen, frozen) => calls.push([screen, frozen]),
    console: { info() {} }, Map, Set, Array, Math
});

assert.deepEqual(calls.splice(0), [["DP-1", false], ["DP-2", false]]);
const window = applicationWindow(0);
liveWindows.push(window);
workspace.windowAdded.emit(window);
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

window.frameGeometry.x = 100;
window.outputChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false], ["DP-2", true]]);

window.frameGeometry.x = 0;
window.frameGeometry.width = 200;
window.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

window.hidden = true;
window.hiddenChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false], ["DP-2", false]]);

window.hidden = false;
workspace.currentDesktopChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true], ["DP-2", true]]);

liveWindows.splice(liveWindows.indexOf(window), 1);
workspace.windowRemoved.emit(window);
assert.deepEqual(calls.splice(0), [["DP-1", false], ["DP-2", false]]);

const maximized = applicationWindow(100);
maximized.frameGeometry.height = 95;
liveWindows.push(maximized);
workspace.windowAdded.emit(maximized);
assert.deepEqual(calls.splice(0), [["DP-2", true]]);

maximized.desktops = [{}];
workspace.currentDesktopChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-2", false]]);

maximized.desktops = [];
maximized.activities = ["other"];
workspace.currentActivityChanged.emit();
assert.deepEqual(calls.splice(0), []);

maximized.activities = ["default"];
workspace.currentActivityChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-2", true]]);

liveWindows.splice(liveWindows.indexOf(maximized), 1);
workspace.windowRemoved.emit(maximized);
assert.deepEqual(calls.splice(0), [["DP-2", false]]);

const staged = applicationWindow(0);
staged.normalWindow = false;
staged.frameGeometry.width = 50;
liveWindows.push(staged);
workspace.windowAdded.emit(staged);
assert.deepEqual(calls.splice(0), []);

staged.normalWindow = true;
staged.frameGeometry.width = 100;
staged.maximizedChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

staged.minimized = true;
workspace.windowActivated.emit(staged);
assert.deepEqual(calls.splice(0), [["DP-1", false]]);

staged.minimized = false;
workspace.stackingOrderChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

liveWindows.splice(liveWindows.indexOf(staged), 1);
workspace.windowRemoved.emit(staged);
assert.deepEqual(calls.splice(0), [["DP-1", false]]);

const leftHalf = applicationWindow(0, 50);
const rightHalf = applicationWindow(50, 50);
liveWindows.push(leftHalf);
workspace.windowAdded.emit(leftHalf);
assert.deepEqual(calls.splice(0), []);
liveWindows.push(rightHalf);
workspace.windowAdded.emit(rightHalf);
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

rightHalf.frameGeometry.x = 53;
rightHalf.frameGeometry.width = 47;
rightHalf.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false]]);

rightHalf.frameGeometry.x = 50;
rightHalf.frameGeometry.width = 50;
rightHalf.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

liveWindows.splice(liveWindows.indexOf(leftHalf), 1);
workspace.windowRemoved.emit(leftHalf);
assert.deepEqual(calls.splice(0), [["DP-1", false]]);

liveWindows.splice(liveWindows.indexOf(rightHalf), 1);
workspace.windowRemoved.emit(rightHalf);
assert.deepEqual(calls.splice(0), []);

const borderless = applicationWindow(0, 50);
borderless.noBorder = true;
borderless.opacity = 1;
borderless.opacityChanged = signal();
borderless.noBorderChanged = signal();
assert.equal(borderless.normalWindow, true);
assert.equal(borderless.fullScreen, false);
liveWindows.push(borderless);
workspace.windowAdded.emit(borderless);
assert.deepEqual(calls.splice(0), []);

borderless.frameGeometry.width = 100;
borderless.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);
workspace.stackingOrderChanged.emit();
borderless.noBorderChanged.emit();
assert.deepEqual(calls.splice(0), []);

borderless.opacity = 0.5;
borderless.opacityChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false]]);
borderless.opacity = 1;
borderless.opacityChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

borderless.minimized = true;
borderless.minimizedChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false]]);
borderless.minimized = false;
borderless.minimizedChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", true]]);

borderless.frameGeometry.width = 50;
borderless.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-1", false]]);
borderless.noBorder = false;
borderless.noBorderChanged.emit();
borderless.noBorder = true;
borderless.noBorderChanged.emit();
assert.deepEqual(calls.splice(0), []);

borderless.frameGeometry.x = 100;
borderless.frameGeometry.width = 100;
borderless.frameGeometry.height = 95;
borderless.frameGeometryChanged.emit();
assert.deepEqual(calls.splice(0), [["DP-2", true]]);
liveWindows.splice(liveWindows.indexOf(borderless), 1);
workspace.windowRemoved.emit(borderless);
assert.deepEqual(calls.splice(0), [["DP-2", false]]);

console.log("KWin power-save: transizioni, monitor, borderless, opacità e copertura affiancata OK");
