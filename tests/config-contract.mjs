import assert from "node:assert/strict";
import { existsSync, readFileSync } from "node:fs";

const root = new URL("../", import.meta.url);
const main = readFileSync(new URL("package/contents/ui/main.qml", root), "utf8");
const config = readFileSync(new URL("package/contents/ui/config.qml", root), "utf8");
const metadata = readFileSync(new URL("package/metadata.json", root), "utf8");

const rendererProperties = [
    "flowGroupOffset",
    "flowSpacing",
    "horizontalOverscan",
    "pauseWhenCovered",
    "renderStyle",
    "renderingPaused",
    "splineBackend"
];
for (const property of rendererProperties) {
    assert.match(main, new RegExp(`\\b${property}\\s*:`),
        `main.qml must forward ${property} to Native.XmbRenderer`);
}

assert.match(main, /visible:\s*!renderer\.renderingPaused\s*&&\s*!renderer\.frozen/,
    "the renderer must leave the scene while manually paused or frozen");
assert.match(config, /i18n\(/,
    "the settings page must use KDE's QML translation helper");
assert.match(metadata, /"Name\[it\]"/,
    "the package metadata must include an Italian name");
assert.match(metadata, /"Description\[it\]"/,
    "the package metadata must include an Italian description");
assert.ok(existsSync(new URL("package/contents/locale/it/LC_MESSAGES/plasma_wallpaper_org.xmbflow.interactive.mo", root)),
    "the compiled Italian catalog must be bundled");
assert.doesNotMatch(config, /Segui puntatore:|Multischermo:|Qualità mesh:/,
    "Italian UI literals must stay in the catalog instead of the QML source");

assert.match(main, /pauseWhenCovered:\s*Boolean\(root\.cfg\.pauseWhenHidden\s*\?\?\s*true\)\s*&&\s*Boolean\(root\.cfg\.pauseWhenCovered\s*\?\?\s*true\)/,
    "energy saving must be the master switch for automatic covered-output pausing");
assert.doesNotMatch(config, /i18n\(\"Diagnostics:\"\)/,
    "the diagnostics-only control must not destabilize the normal settings layout");
assert.match(config, /Horizontal overscan:/,
    "horizontal overscan must remain available in the settings page");
assert.match(config, /checked:\s*root\.cfg_horizontalOverscan/,
    "horizontal overscan must be bound to its saved configuration value");

console.log("Configuration contract, pause binding, localization and settings controls are present");
