import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const source = readFileSync(new URL("../src/xmbrendereritem.cpp", import.meta.url), "utf8");
const qml = readFileSync(new URL("../package/contents/ui/main.qml", import.meta.url), "utf8");
const render = source.split("    void render() override\n    {")[1]?.split("\nprivate:")[0];

assert.ok(render, "renderer OpenGL non trovato");
assert.doesNotMatch(render, /\bupdate\s*\(/,
    "il render thread non deve richiedere continuamente nuovi frame");
assert.match(render, /m_frozen\s*\|\|\s*m_renderingPaused/,
    "il render thread deve rispettare entrambe le pause");
assert.match(qml, /visible:\s*!renderer\.renderingPaused\s*&&\s*!renderer\.frozen/,
    "il framebuffer in pausa deve essere escluso dalla scena Qt Quick");
assert.match(source, /if \(!m_frozen && !m_renderingPaused\s*\n\s*&& \(!m_pauseWhenHidden \|\| isVisible\(\)\)\)/,
    "il timer deve fermarsi anche in modalità multischermo");

console.log("Pausa rendering: nessun ciclo GPU autonomo e item nascosto OK");
