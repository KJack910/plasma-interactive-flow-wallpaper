#include "xmbrendereritem.h"
#include "zoomconstraints.h"
#include "meshvisibility.h"
#include "meshquality.h"
#include "meshgeometry.h"
#include "outputvisibilitycontroller.h"
#include "splinetexture.h"
#include "splinetexturegpu.h"

#include <QEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QHash>
#include <QString>
#include <QOpenGLContext>
#include <QPointer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVersionFunctionsFactory>
#include <QRandomGenerator>
#include <QScreen>
#include <QQuickWindow>
#include <QVector3D>
#include <QVector4D>
#include <QDebug>
#include <QtMath>

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <limits>
#include <vector>
#include <memory>

namespace
{

    constexpr int kSplineTexW = XmbSpline::width;
    constexpr int kSplineTexH = XmbSpline::height;

    static constexpr const char *kBgVertex = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 vUvYDown;
void main() {
    vUvYDown = vec2(aPos.x * 0.5 + 0.5, 1.0 - (aPos.y * 0.5 + 0.5));
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

    static constexpr const char *kBgFragment = R"GLSL(
#version 330 core
in vec2 vUvYDown;
out vec4 oColor;
uniform vec3 uColorStart;
uniform vec3 uColorEnd;
uniform vec2 uDir;
uniform float uTMin;
uniform float uTSpan;
uniform vec2 uViewportOriginPx;
uniform vec2 uViewportSizePx;
uniform vec2 uVirtualSizePx;
uniform vec2 uReferenceSizePx;
uniform float uMultiscreenDebug;

float gridLine(float worldX, float spacing, float widthPx) {
    float m = mod(worldX, spacing);
    float d = min(m, spacing - m);
    return 1.0 - smoothstep(widthPx, widthPx + 1.0, d);
}

void main() {
    vec2 world = uViewportOriginPx + vUvYDown * uViewportSizePx;
    vec2 worldUv = world / max(uVirtualSizePx, vec2(1.0));
    float t = dot(worldUv, uDir);
    float u = clamp((t - uTMin) / max(uTSpan, 1e-6), 0.0, 1.0);
    float g = u * u * (3.0 - 2.0 * u);
    vec3 base = mix(uColorStart, uColorEnd, g);

    if (uMultiscreenDebug > 0.5) {
        float minor = max(
            gridLine(world.x, max(uReferenceSizePx.x / 4.0, 1.0), 1.0),
            gridLine(world.y, max(uReferenceSizePx.y / 4.0, 1.0), 1.0));
        float major = max(
            gridLine(world.x, max(uReferenceSizePx.x, 1.0), 2.0),
            gridLine(world.y, max(uReferenceSizePx.y, 1.0), 2.0));
        float virtualEdge = max(max(
            1.0 - smoothstep(0.0, 2.0, abs(world.x)),
            1.0 - smoothstep(0.0, 2.0, abs(world.x - uVirtualSizePx.x))), max(
            1.0 - smoothstep(0.0, 2.0, abs(world.y)),
            1.0 - smoothstep(0.0, 2.0, abs(world.y - uVirtualSizePx.y))));
        base = mix(base, vec3(0.62), minor * 0.22);
        base = mix(base, vec3(0.92), major * 0.62);
        base = mix(base, vec3(1.0), virtualEdge * 0.85);
    }

    oColor = vec4(base, 1.0);
}
)GLSL";

    static constexpr const char *kWaveVertex = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
uniform sampler2D uSplineTex;
uniform float uTime;
uniform float flowSpeed;
uniform float tension;
uniform float damping;
uniform float uSplineLength;
uniform float spacing;
uniform float perturbation;
uniform float perturbationScale;
uniform float timeStep;
uniform float waveCosAmp;
uniform float waveBias;
uniform float waveHeightScale;
uniform float waveSoftClip;
uniform float ffdYAmp;
uniform float ffdZAmp;
uniform float zDetailScale;
uniform vec3 ffdScale1;
uniform vec3 ffdScale2;
uniform vec3 ffdOffset;
uniform vec2 uPointer;
uniform vec2 uInteractionCenter;
uniform float uInteractionRadius;
uniform vec2 uInteractionSign;
uniform vec2 uFlow;
uniform vec2 uViewport;
uniform float uPointerStrength;
uniform vec4 uSafeRect;
uniform float uProtected;
uniform float uWaveVariant;
uniform float uWaveYOffset;
uniform float uZoom;
uniform vec2 uHorizontalDomain;
uniform vec2 uZoomOffsetPx;
uniform vec2 uViewportOriginPx;
uniform vec2 uViewportSizePx;
uniform vec2 uVirtualSizePx;
uniform vec2 uReferenceSizePx;
uniform int uPointerFollowMode;
uniform int uRenderStyle;
uniform int uParticleStyle;
out vec3 vPos;
out float vFilament;
out float vDepth;
void main() {
    vec2 uv = (aPos + 1.0) * 0.5;

    // Build one complete virtual-desktop mesh in every renderer. Projection is
    // performed in this shared scene before the result is converted to the
    // local output NDC, so perspective and zoom cannot restart at a seam.
    // Keep a horizontal overscan so waves continue beyond both screen edges.
    // Wide source domain prevents dynamic zoom from exhausting the mesh at
    // either horizontal edge (the visible viewport still performs clipping).
    float worldX = (uv.x * (uHorizontalDomain.y - uHorizontalDomain.x) + uHorizontalDomain.x) * uVirtualSizePx.x;
    float desktopU = worldX / max(uVirtualSizePx.x, 1.0);
    float phaseU = worldX / max(uReferenceSizePx.x, 1.0);
    float waveX = phaseU * 2.0 - 1.0;
    float sceneX = (worldX - uVirtualSizePx.x * 0.5)
        * 2.0 / max(uReferenceSizePx.x, 1.0);
    vec3 p = vec3(sceneX, 0.0, aPos.y);
    // Wrap the spline outside the virtual desktop instead of clamping its
    // last texel; clamping was the source of the stretched outer mesh.
    vec2 splineUv = vec2(fract(desktopU), uv.y);

    p.y = texture(uSplineTex, splineUv).r;
    vec3 waveP = vec3(waveX, p.y, p.z);
    vec3 ffd1 = waveP * ffdScale1 + ffdOffset;
    vec3 ffd2 = waveP * ffdScale2 + ffdOffset;
    p.y += sin(ffd1.x + uTime * flowSpeed * mix(1.2, 1.34, uWaveVariant) + uWaveVariant * 0.8) * ffdYAmp;
    p.z += cos(ffd2.z + uTime * flowSpeed * mix(1.1, 1.27, uWaveVariant) + uWaveVariant * 1.1) * ffdZAmp;
    float baseWave = cos(waveX * 2.0 + uTime * mix(0.62, 0.78, uWaveVariant) * timeStep + uWaveVariant * 1.2) * waveCosAmp + waveBias;
    baseWave *= (1.0 - damping);
    baseWave += tension * sin(waveX * uSplineLength + uTime * flowSpeed * timeStep * mix(0.42, 0.58, uWaveVariant) + uWaveVariant * 1.4);
    float structured = perturbation * perturbationScale * (
        sin((waveX * uSplineLength * 6.0 + p.z * 0.5) * spacing * 0.01 + uTime * flowSpeed * timeStep * mix(0.92, 1.06, uWaveVariant) + uWaveVariant * 0.9) * 0.5 +
        sin((waveX * uSplineLength * 10.0 - p.z * 0.8) * spacing * 0.005 + uTime * flowSpeed * timeStep * mix(0.44, 0.61, uWaveVariant) + uWaveVariant * 1.3) * 0.25
    );
    float totalWave = (baseWave + structured) * waveHeightScale;
    totalWave = waveSoftClip * tanh(totalWave / max(waveSoftClip, 1e-4));
    p.y -= totalWave;
    vec2 uv2 = splineUv;
    uv2.x = fract(uv2.x + uTime * flowSpeed * 0.06 * timeStep);
    p.z -= texture(uSplineTex, uv2).r * zDetailScale;
    p.y += sin(waveX * 3.1 + uTime * flowSpeed * 0.8 + uWaveVariant * 1.6) * 0.012 * uWaveVariant;
    p.y += uWaveYOffset;

    float renderYSign = (uRenderStyle == 1) ? -1.0 : 1.0;
    if (uPointerFollowMode == 2 && uPointerStrength > 0.0001) {
        vec2 cursorScene = vec2(
            (uPointer.x - uVirtualSizePx.x * 0.5) * 2.0 / max(uReferenceSizePx.x, 1.0),
            renderYSign * (uVirtualSizePx.y * 0.5 - uPointer.y) * 2.0 / max(uReferenceSizePx.y, 1.0));
        p.xy += cursorScene;
    }

    float basePerspective = 1.0 / (1.0 + clamp(p.z, -1.1, 1.1) * 0.085);
    // Interaction space restored exactly to the v1.0.5/v1.0.2 centered-zoom
    // formulation. Dynamic zoom pivoting is applied only to final rendering.
    // Interaction is evaluated in the unzoomed global scene. One global
    // affine zoom transform is applied below; its offset is chosen from the
    // monitor under the cursor on the GUI thread.
    vec2 projectedBase = p.xy * basePerspective;
    vec2 interactionBaseWorld = vec2(
        uVirtualSizePx.x * 0.5 + projectedBase.x * uReferenceSizePx.x * 0.5,
        uVirtualSizePx.y * 0.5 - renderYSign * projectedBase.y * uReferenceSizePx.y * 0.5);
    vec2 screen = interactionBaseWorld * uZoom + uZoomOffsetPx;
    // Qt reports the pointer from the top-left, while the FBO interaction
    // surface is bottom-left based. Convert only the pointer position here.
    vec2 pointerFramebuffer = uPointer;
    vec2 offset = screen - pointerFramebuffer;
    float radius = clamp(uReferenceSizePx.x * 0.09 * uInteractionRadius, 40.0, 320.0);
    vec2 contactOffset = offset / vec2(1.25, 1.0);
    float distanceToPointer = sqrt(dot(contactOffset, contactOffset));
    float influence = exp(-dot(contactOffset, contactOffset) / (radius * radius))
        * (1.0 - smoothstep(radius * 0.65, radius * 1.5, distanceToPointer));
    vec2 outside = max(max(uSafeRect.xy - screen, screen - uSafeRect.zw), vec2(0.0));
    float safeDistance = sqrt(dot(outside, outside));
    float safeMask = mix(1.0, smoothstep(0.0, 80.0, safeDistance), uProtected);
        float filamentPhase = uv.y * 24.0 + sin(phaseU * 7.5 - uTime * flowSpeed * 0.7) * 0.09;
    float filament = pow(0.5 + 0.5 * cos(filamentPhase * 6.28318530718), 4.0);
    float filamentResponse = 0.92 + filament * 0.28;
    vec2 stretch = (offset * 0.36 + uFlow * 0.04)
        * influence * uPointerStrength * safeMask * filamentResponse;
    float travel = sqrt(dot(stretch, stretch));
    float guard = mix(1.0, min(1.0, safeDistance * 0.75 / max(travel, 0.001)), uProtected);
    stretch *= guard;

    // Intentionally kept identical to v1.0.2/v1.0.5 mouse behavior.
    // X already uses the screen-space direction. Y is also kept in the
    // screen-space sign here: the previous negation mirrored vertical motion.
    p.xy += vec2(stretch.x, -renderYSign * stretch.y) * uInteractionSign * 2.0
        / (uReferenceSizePx * sqrt(clamp(uZoom, 0.40, 3.50)));

    float depthPolarity = sin((filamentPhase + phaseU * 0.31) * 6.28318530718);
    float gestureDepth = 0.045 + min(sqrt(dot(uFlow, uFlow)) / 42.0, 1.0) * 0.085;
    p.z += depthPolarity * gestureDepth * influence * uPointerStrength * safeMask * guard;
    float perspective = 1.0 / (1.0 + clamp(p.z, -1.2, 1.2) * 0.095);
    vec2 projected = p.xy * perspective;
    vec2 projectedBaseWorld = vec2(
        uVirtualSizePx.x * 0.5 + projected.x * uReferenceSizePx.x * 0.5,
        uVirtualSizePx.y * 0.5 - renderYSign * projected.y * uReferenceSizePx.y * 0.5);
    vec2 projectedWorld = projectedBaseWorld * uZoom + uZoomOffsetPx;
    vec2 localUv = (projectedWorld - uViewportOriginPx) / uViewportSizePx;
    vec2 localNdc = vec2(localUv.x * 2.0 - 1.0, 1.0 - localUv.y * 2.0);
    gl_Position = vec4(localNdc, clamp(p.z * 0.22, -0.95, 0.95), 1.0);
    vPos = p;
    vFilament = filament;
    vDepth = p.z;
}
)GLSL";

    static constexpr const char *kWaveFragment = R"GLSL(
#version 330 core
in vec3 vPos;
in float vFilament;
in float vDepth;
out vec4 oColor;
uniform float opacity;
uniform float brightness;
uniform float fresnelPower;
uniform float fresnelScale;
void main() {
    vec3 dx = dFdx(vPos);
    vec3 dy = dFdy(vPos);
    vec3 N = normalize(cross(dx, dy));
    float F = fresnelScale * pow(1.0 + dot(vec3(0.0, 0.0, -1.0), N), fresnelPower);
    float filamentBody = F * mix(0.92, 1.18, vFilament) + 0.032 * vFilament;
    float depthLight = mix(0.82, 1.14, clamp(0.5 - vDepth * 0.32, 0.0, 1.0));
    oColor = vec4(vec3(depthLight), min(1.0, filamentBody * opacity * brightness * depthLight));
}
)GLSL";

    static constexpr const char *kParticleVertex = R"GLSL(
#version 330 core
layout(location=0) in vec3 aSeed;
layout(location=1) in float aTrailAge;
layout(location=2) in float aTrailEnabled;
uniform float uTime;
uniform float flowSpeed;
uniform float ratio;
uniform float ptSizeBase;
uniform float ptSizeVar;
uniform vec2 uPointer;
uniform vec2 uInteractionCenter;
uniform float uInteractionRadius;
uniform vec2 uInteractionSign;
uniform vec2 uFlow;
uniform vec2 uViewport;
uniform float uPointerStrength;
uniform vec4 uSafeRect;
uniform float uProtected;
uniform float uZoom;
uniform vec2 uHorizontalDomain;
uniform vec2 uZoomOffsetPx;
uniform int uUpperCount;
uniform int uCenterCount;
uniform int uLowerCount;
uniform vec2 uViewportOriginPx;
uniform vec2 uViewportSizePx;
uniform vec2 uVirtualSizePx;
uniform vec2 uReferenceSizePx;
uniform int uPointerFollowMode;
uniform int uRenderStyle;
uniform int uParticleStyle;
uniform int uParticleSimulation;
uniform int uTrailPass;
uniform float uFlowGroupOffset;
uniform float uFlowSpacing;
out float vAlpha;
out float vTrail;
out vec2 vTrailAxis;
out float vTrailDepth;
out float vTrailLine;

void laneParameters(int lane, out float laneY, out float phase, out float variant)
{
    float upperBase = uFlowGroupOffset;
    float lowerBase = -uFlowGroupOffset;
    float regionalSpacing = uFlowSpacing;
    float centerSpacing = uFlowSpacing;

    if (lane < uUpperCount) {
        int i = lane;
        float centered = (uUpperCount > 1) ? (float(i) / float(uUpperCount - 1) - 0.5) : 0.0;
        laneY = upperBase + centered * regionalSpacing;
        float seed = fract(sin(float(11 + i) * 12.9898) * 43758.5453);
        phase = 0.20 + seed * 3.80;
        variant = 0.25 + seed * 2.40;
        return;
    }

    lane -= uUpperCount;
    if (lane < uCenterCount) {
        int i = lane;
        float centered = (uCenterCount > 1) ? (float(i) / float(uCenterCount - 1) - 0.5) : 0.0;
        laneY = centered * centerSpacing;
        float seed = fract(sin(float(101 + i) * 12.9898) * 43758.5453);
        phase = 0.20 + seed * 3.80;
        variant = 0.25 + seed * 2.40;
        return;
    }

    lane -= uCenterCount;
    int i = max(lane, 0);
    float centered = (uLowerCount > 1) ? (float(i) / float(uLowerCount - 1) - 0.5) : 0.0;
    laneY = lowerBase + centered * regionalSpacing;
    float seed = fract(sin(float(211 + i) * 12.9898) * 43758.5453);
    phase = 0.20 + seed * 3.80;
    variant = 0.25 + seed * 2.40;
}

void main() {
    int totalFlows = uUpperCount + uCenterCount + uLowerCount;
    if (totalFlows <= 0) {
        vAlpha = 0.0;
        gl_PointSize = 0.0;
        gl_Position = vec4(3.0, 3.0, 0.0, 1.0);
        return;
    }

    float sizeScale = (uParticleStyle == 1) ? 2.2 : (uParticleStyle == 2 ? (0.65 + aSeed.z * 1.8) : 0.8);
    float pointSize = (aSeed.z * ptSizeVar + ptSizeBase) * sizeScale;
    float trailAge = (uParticleSimulation == 1) ? aTrailAge : 0.0;
    float time = uTime * flowSpeed - trailAge * 0.42;
    float interactionAge = mix(0.45, 1.0, exp(-trailAge * 1.8));

    // Every particle belongs to one of the active XMB flow lanes.
    int lane = min(totalFlows - 1,
        int(floor(fract(aSeed.x * 17.713 + aSeed.y * 31.117) * float(totalFlows))));
    float laneY;
    float lanePhase;
    float laneVariant;
    laneParameters(lane, laneY, lanePhase, laneVariant);

    // The particle field also lives once in virtual-desktop world space.
    // Scale normalized travel speed so pixels/second remain close to the
    // original single-monitor behavior as virtual desktop width grows.
    float travelScale = uReferenceSizePx.x / max(uVirtualSizePx.x, 1.0);
    float drift = fract(aSeed.y * 50.0 + time * (0.055 + aSeed.x * 0.035) * travelScale);
    float trailLife = 1.0 - smoothstep(0.02, 0.99, drift);
    float trailMask = aTrailEnabled;
    vTrail = trailMask;
    vTrailLine = trailMask * exp(-trailAge * 2.4);
    // The wrapped transport is discontinuous at the desktop edges. Do not
    // connect history samples across that teleport.
    float wrapGuard = step(0.06, drift) * step(drift, 0.94);
    vTrailLine *= wrapGuard;
    pointSize *= mix(1.0, mix(0.45, 5.50, pow(trailLife, 0.72)), trailMask);
    // Symmetric horizontal overscan, matching the wave mesh domain.
    // Symmetric coverage around the virtual-desktop center. The extra span
    // keeps the left edge populated when zoom reaches its minimum.
    float globalU = uHorizontalDomain.y - drift * (uHorizontalDomain.y - uHorizontalDomain.x);
    globalU += sin(time * 0.9 * travelScale + aSeed.y * 15.0 + aSeed.x * 9.0) * 0.0175;
    if (uHorizontalDomain.x == 0.0)
        globalU = clamp(globalU, 0.0, 1.0);
    float particleWorldX = globalU * uVirtualSizePx.x;
    float x = (particleWorldX - uVirtualSizePx.x * 0.5)
        * 2.0 / max(uReferenceSizePx.x, 1.0);
    float phaseU = particleWorldX / max(uReferenceSizePx.x, 1.0);
    float waveX = phaseU * 2.0 - 1.0;

    // Follow the selected flow instead of occupying an unrelated independent
    // band. Jitter keeps the particles around the ribbon rather than exactly
    // on one mathematical line.
    float carrier =
        sin(waveX * 3.1 + (uTime + lanePhase) * flowSpeed * 0.8 + laneVariant * 1.6)
        * (0.020 + 0.006 * laneVariant);
    carrier += sin(waveX * 6.4 + (uTime + lanePhase) * flowSpeed * 0.43)
        * 0.018;
    float ribbonSpread = (aSeed.z - 0.55) * 0.30
        + sin(time * 0.31 + aSeed.x * 31.0 + aSeed.y * 17.0) * 0.055;
    float y = laneY + carrier + ribbonSpread;

    float depthSeed = aSeed.z * 2.0 - 1.0;
    float z = depthSeed * 0.26
        + sin(time * 0.21 + aSeed.x * 31.0 + aSeed.y * 17.0) * 0.055;
    // The pseudo-2D field travels left, therefore its tail points right.
    // In 3D this velocity also carries the depth component of the orbit.
    vec3 trailVelocity = vec3(-0.24 - aSeed.x * 0.15, 0.0, 0.0);
    if (uParticleSimulation == 1) {
        // Stable volumetric orbit: deterministic seeds and the shared clock
        // keep all virtual-screen renderers on the same 3D particle state.
        float depthPhase = time * (0.22 + aSeed.x * 0.13)
            + aSeed.x * 19.7 + aSeed.y * 13.1;
        float orbit = sin(depthPhase);
        z = depthSeed * 0.78 + orbit * 0.22;
        y += cos(depthPhase * 0.83 + laneVariant) * (0.035 + abs(z) * 0.075);
        x += sin(depthPhase * 0.61 + lanePhase) * (0.018 + abs(z) * 0.045);

        // Derivative of the volumetric orbit. Combine it with the dominant
        // right-to-left transport, then reverse it to obtain the tail axis.
        float depthRate = flowSpeed * (0.22 + aSeed.x * 0.13);
        float orbitDx = cos(depthPhase * 0.61 + lanePhase)
            * 0.61 * depthRate * (0.018 + abs(z) * 0.045);
        float orbitDy = -sin(depthPhase * 0.83 + laneVariant)
            * 0.83 * depthRate * (0.035 + abs(z) * 0.075);
        float orbitDz = cos(depthPhase) * depthRate * 0.22;
        trailVelocity = vec3(-0.24 - aSeed.x * 0.15 + orbitDx, orbitDy, orbitDz);
    }

    float opVar = mix(
        sin(time * (aSeed.x + 0.5) * 12.0 + aSeed.y * 10.0),
        sin(time * (aSeed.y + 1.5) * 6.0 + aSeed.x * 4.0),
        clamp(y * 0.5 + 0.5, 0.0, 1.0)) * aSeed.x + aSeed.y;
    vAlpha = opVar * opVar * (1.0 - fract(aSeed.x + time * 0.00285));
    vAlpha *= mix(1.0, min(1.0, 1.75 * pow(trailLife, 0.62)), trailMask);
    vAlpha *= exp(-trailAge * 2.4);
    if (uTrailPass == 0 && trailAge > 0.0001) {
        vAlpha = 0.0;
    }

    vec3 p = vec3(x, y, z);
    float renderYSign = (uRenderStyle == 1) ? -1.0 : 1.0;
    vTrailAxis = vec2(1.0, 0.0);
    vTrailDepth = 0.0;
    if (uPointerFollowMode == 2 && uPointerStrength > 0.0001) {
        vec2 cursorScene = vec2(
            (uPointer.x - uVirtualSizePx.x * 0.5) * 2.0 / max(uReferenceSizePx.x, 1.0),
            renderYSign * (uVirtualSizePx.y * 0.5 - uPointer.y) * 2.0 / max(uReferenceSizePx.y, 1.0));
        p.xy += cursorScene * interactionAge;
    }
    float basePerspective = 1.0 / (1.0 + clamp(p.z, -1.1, 1.1)
        * (uParticleSimulation == 1 ? 0.24 : 0.085));
    // Same interaction projection used by v1.0.5.
    // Match the wave path: do not apply dynamic zoom before interaction.
    vec2 projectedBase = p.xy * basePerspective;
    vec2 interactionBaseWorld = vec2(
        uVirtualSizePx.x * 0.5 + projectedBase.x * uReferenceSizePx.x * 0.5,
        uVirtualSizePx.y * 0.5 - renderYSign * projectedBase.y * uReferenceSizePx.y * 0.5);
    vec2 screen = interactionBaseWorld * uZoom + uZoomOffsetPx;
    vec2 pointerFramebuffer = uPointer;
    // The directional interaction source points from the particle to the
    // cursor. Keeping this orientation makes positive X/Y signs carry the
    // trail through the pointer instead of pushing it into an inner bend.
    vec2 offset = pointerFramebuffer - screen;
    float radius = clamp(uReferenceSizePx.x * 0.07 * uInteractionRadius, 32.0, 260.0);
    vec2 contactOffset = offset / vec2(1.25, 1.0);
    float distanceToPointer = sqrt(dot(contactOffset, contactOffset));
    float influence = exp(-dot(contactOffset, contactOffset) / (radius * radius))
        * (1.0 - smoothstep(radius * 0.65, radius * 1.5, distanceToPointer));
    vec2 outside = max(max(uSafeRect.xy - screen, screen - uSafeRect.zw), vec2(0.0));
    float safeDistance = sqrt(dot(outside, outside));
    float safeMask = mix(1.0, smoothstep(0.0, 80.0, safeDistance), uProtected);
    // Reactivity is intrinsic: larger particles receive a stronger impulse;
    // mixed particles scale continuously with their individual size.
    float particleResponse = (0.90 + aSeed.z * 0.20)
        * (uParticleStyle == 1 ? 1.65 : (uParticleStyle == 2 ? 0.85 + aSeed.z * 0.95 : 0.75));
    float agedInfluence = influence * interactionAge;
    vec2 stretch = (offset * 0.38 + uFlow * 0.06)
        * agedInfluence * uPointerStrength * safeMask * particleResponse;
    if (uParticleSimulation == 1) {
        float inverseMass = mix(1.30, 0.58, aSeed.z);
        float depthResponse = mix(1.38, 0.62, clamp(p.z * 0.5 + 0.5, 0.0, 1.0));
        vec2 tangent = vec2(-offset.y, offset.x)
            / max(sqrt(dot(offset, offset)), 1.0);
        stretch = stretch * inverseMass * depthResponse
            + tangent * agedInfluence * uPointerStrength * (8.0 + 11.0 * aSeed.x);
    }
    float travel = sqrt(dot(stretch, stretch));
    float guard = mix(1.0, min(1.0, safeDistance * 0.75 / max(travel, 0.001)), uProtected);

    // Intentionally kept identical to v1.0.2/v1.0.5 mouse behavior.
    p.xy += vec2(stretch.x, -renderYSign * stretch.y) * uInteractionSign * 2.0
        / (uReferenceSizePx * sqrt(clamp(uZoom, 0.40, 3.50))) * guard;

    float particleDepth = sin(aSeed.x * 47.0 + aSeed.y * 29.0);
    float gestureDepth = 0.04 + min(sqrt(dot(uFlow, uFlow)) / 42.0, 1.0) * 0.08;
    float depthImpulse = particleDepth * gestureDepth * agedInfluence * uPointerStrength * safeMask * guard;
    p.z += depthImpulse;
    if (uParticleSimulation == 1) {
        float volumetricImpulse = sin(aSeed.x * 47.0 + aSeed.y * 29.0)
            * agedInfluence * uPointerStrength * 0.24 * guard;
        p.z += volumetricImpulse;
    }
    float perspective = 1.0 / (1.0 + clamp(p.z, -1.2, 1.2)
        * (uParticleSimulation == 1 ? 0.28 : 0.095));
    if (uParticleSimulation == 1) {
        // Project the tail sample as a real 3D point. This lets Z motion leave
        // a visible mark through perspective instead of flattening the trail
        // to the XY plane.
        float velocityLength = max(length(trailVelocity), 0.0001);
        vec3 tailSample = p - trailVelocity / velocityLength * 0.22;
        float tailPerspective = 1.0 / (1.0 + clamp(tailSample.z, -1.2, 1.2) * 0.28);
        vec2 projectedHead = p.xy * perspective;
        vec2 projectedTail = tailSample.xy * tailPerspective;
        vec2 projectedTrail = vec2(
            projectedTail.x - projectedHead.x,
            -renderYSign * (projectedTail.y - projectedHead.y));
        if (length(projectedTrail) > 0.0001) {
            vTrailAxis = normalize(projectedTrail);
        }
        vTrailDepth = clamp(trailVelocity.z * 8.0, -1.0, 1.0);
    }
    vec2 projected = p.xy * perspective;
    vec2 projectedBaseWorld = vec2(
        uVirtualSizePx.x * 0.5 + projected.x * uReferenceSizePx.x * 0.5,
        uVirtualSizePx.y * 0.5 - renderYSign * projected.y * uReferenceSizePx.y * 0.5);
    vec2 projectedWorld = projectedBaseWorld * uZoom + uZoomOffsetPx;
    vec2 localUv = (projectedWorld - uViewportOriginPx) / uViewportSizePx;
    vec2 localNdc = vec2(localUv.x * 2.0 - 1.0, 1.0 - localUv.y * 2.0);
    float zoomParticleScale = clamp(pow(max(uZoom, 0.40), 0.65), 0.85, 2.40);
    gl_PointSize = pointSize * zoomParticleScale
        * clamp(perspective, uParticleSimulation == 1 ? 0.55 : 0.72,
            uParticleSimulation == 1 ? 1.75 : 1.28);
    gl_Position = vec4(localNdc, clamp(p.z * 0.22, -0.95, 0.95), 1.0);
}
)GLSL";

    static constexpr const char *kParticleFragment = R"GLSL(
#version 330 core
in float vAlpha;
in float vTrail;
in vec2 vTrailAxis;
in float vTrailDepth;
out vec4 oColor;
uniform float particleOpacity;
uniform int uTrailPass;
in float vTrailLine;
void main() {
    if (uTrailPass == 1) {
        float lineAlpha = vTrailLine * particleOpacity * 0.55;
        if (lineAlpha < 0.002) discard;
        oColor = vec4(vec3(lineAlpha), lineAlpha);
        return;
    }

    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float d = dot(c, c);
    float normalSparkle = d < 1.0 ? (1.0 - d) * (1.0 - d) : 0.0;

    // Particles travel right-to-left: the bright head is on the left and the
    // tail fades behind it toward the right edge of the point sprite.
    // Rotate point-sprite coordinates into the projected direction of travel.
    // Local +X points from the head towards the tail. This preserves the old
    // horizontal appearance in pseudo-2D and follows curved 3D trajectories.
    vec2 centered = gl_PointCoord - vec2(0.5);
    vec2 tailAxis = vTrailAxis;
    vec2 normalAxis = vec2(-tailAxis.y, tailAxis.x);
    vec2 uv = vec2(dot(centered, tailAxis), dot(centered, normalAxis))
        + vec2(0.5);
    float head = exp(-dot(vec2((uv.x - 0.16) * 7.0, (uv.y - 0.5) * 10.0),
                          vec2((uv.x - 0.16) * 7.0, (uv.y - 0.5) * 10.0)));
    float depthShear = vTrailDepth * 0.24 * (1.0 - uv.x);
    float depthCenter = 0.5 + depthShear;
    float depthAmount = abs(vTrailDepth);
    float tailWidth = exp(-abs(uv.y - depthCenter) * mix(18.0, 8.0, depthAmount));
    float tailDecayRate = mix(3.8, 2.0, depthAmount);
    float tailDecay = uv.x >= 0.16 ? exp(-(uv.x - 0.16) * tailDecayRate) : 0.0;
    float depthTrace = exp(-dot(vec2((uv.x - 0.30) * 4.5,
                                    (uv.y - depthCenter) * 12.0),
                                vec2((uv.x - 0.30) * 4.5,
                                    (uv.y - depthCenter) * 12.0)))
        * depthAmount * 0.55;
    float shootingStar = max(head, max(tailWidth * tailDecay * 0.82, depthTrace));
    float sparkle = mix(normalSparkle, shootingStar, vTrail);
    if (sparkle < 0.002) discard;
    float a = vAlpha * particleOpacity * sparkle;
    oColor = vec4(vec3(a), 1.0);
}
)GLSL";

    struct GpuGrid
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ibo = 0;
        GLsizei indexCount = 0;
        int resolutionX = 0;
        int resolutionY = 0;
        int firstColumn = -1;
        int lastColumn = -1;
    };

    static qreal sharedAnimationSeconds()
    {
        using Clock = std::chrono::steady_clock;
        static const auto start = Clock::now();
        return std::chrono::duration<qreal>(Clock::now() - start).count();
    }

} // namespace

class XmbRenderer final : public QQuickFramebufferObject::Renderer
{
public:
    XmbRenderer() = default;
    ~XmbRenderer() override
    {
        if (m_context && QOpenGLContext::currentContext() == m_context.data())
            cleanup();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GL_RGBA8);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        auto *src = static_cast<XmbRendererItem *>(item);
        m_horizontalOverscan = src->m_horizontalOverscan;
        m_quality = src->m_quality;
        m_meshResolution = src->meshResolution();
        m_particleCount = src->m_particleCount;
        m_waveTime = src->m_waveTime;
        m_particleTime = src->m_particleTime;
        m_frozen = src->m_frozen;
        m_renderingPaused = src->m_renderingPaused;
        m_waveSpeedMul = src->m_waveSpeed;
        m_particleSpeedMul = src->m_particleSpeed;
        m_flow = src->m_flow;
        m_pointerStrength = src->m_interactionEnabled && src->m_pointerFollowMode != 0
                                ? src->m_interactionStrength
                                : 0.0;
        m_viewportLogical = QSizeF(std::max<qreal>(1.0, src->width()), std::max<qreal>(1.0, src->height()));
        m_brightness = src->m_brightness;
        m_zoom = src->m_zoom;
        m_zoomOffsetPx = QVector2D(float(src->m_zoomOffset.x()), float(src->m_zoomOffset.y()));

        const qreal w = std::max<qreal>(1.0, src->m_referenceSizePx.width());
        const qreal h = std::max<qreal>(1.0, src->m_referenceSizePx.height());
        const qreal virtualW = std::max<qreal>(1.0, src->m_virtualSizePx.width());
        const qreal virtualH = std::max<qreal>(1.0, src->m_virtualSizePx.height());
        // uPointer and screen are both expressed as virtual-desktop pixels
        // with an origin at the top-left. Keep the sampled cursor position
        // untouched; applying the old legacy NDC conversion here mirrored the
        // interaction a second time after the global-surface conversion.
        m_pointer = src->m_pointer;
        const QVector2D pointerScene(
            float((src->m_pointer.x() - virtualW * 0.5) * 2.0 / w),
            float((virtualH * 0.5 - src->m_pointer.y()) * 2.0 / h));
        // The interaction center is the raw cursor scene position. The
        // projected vertex is zoomed separately, so comparing both in world
        // pixels keeps the field anchored to the actual cursor.
        m_interactionCenter = pointerScene;
        m_interactionRadius = src->m_interactionRadius;

        m_upperFlowCount = src->m_upperFlowCount;
        m_centerFlowCount = src->m_centerFlowCount;
        m_lowerFlowCount = src->m_lowerFlowCount;
        m_multiscreenDebug = src->m_multiscreenDebug;
        m_pointerFollowMode = src->m_pointerFollowMode;
        m_interactionDirection = src->m_interactionDirection;
        m_renderStyle = src->m_renderStyle;
        m_splineBackend = src->m_splineBackend;
        m_particleStyle = src->m_particleStyle;
        m_particleSimulation = src->m_particleSimulation;
        m_flowGroupOffset = float(src->m_flowGroupOffset);
        m_flowSpacing = float(src->m_flowSpacing);
        m_viewportOriginPx = src->m_viewportOriginPx;
        m_viewportSizePx = src->m_viewportSizePx;
        m_virtualSizePx = src->m_virtualSizePx;
        m_referenceSizePx = src->m_referenceSizePx;

        // Diagnostic mode isolates multiscreen geometry. It intentionally does
        // not change saved settings; it only suppresses interaction/zoom in the
        // render snapshot so seam validation is unambiguous.
        if (m_multiscreenDebug)
        {
            m_pointerStrength = 0.0;
            m_zoom = 1.0;
        }
    }

    void render() override
    {
        if (m_frozen || m_renderingPaused)
            return;

        if (!ensureInitialized())
        {
            return;
        }

        auto *f = m_gl;
        const QSize fbSize = framebufferObject()->size();
        f->glViewport(0, 0, fbSize.width(), fbSize.height());
        f->glDisable(GL_DEPTH_TEST);
        f->glDisable(GL_CULL_FACE);

        renderBackground();
        updateSplineTexture();
        ensureGrid();
        updateGridVisibleRange();
        // The FBO already clips to the current output. Do not add a second
        // zoom-derived scissor when overscan is disabled: that makes the
        // linked virtual surface appear as a smaller centered island. The
        // source domain controls overscan, while the output framebuffer
        // provides the only required screen-edge clipping.
        renderWave();
        ensureParticles();
        renderParticles();

        f->glBindVertexArray(0);
        f->glUseProgram(0);

        // Only the GUI-thread frame timer requests another frame. Scheduling
        // one here keeps the render thread/GPU busy after that timer stops.
    }

private:
    bool compileProgram(QOpenGLShaderProgram &program, const char *vs, const char *fs)
    {
        if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vs))
        {
            qWarning() << "XMB vertex shader:" << program.log();
            return false;
        }
        if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fs))
        {
            qWarning() << "XMB fragment shader:" << program.log();
            return false;
        }
        if (!program.link())
        {
            qWarning() << "XMB shader link:" << program.log();
            return false;
        }
        return true;
    }

    bool ensureInitialized()
    {
        if (m_initialized)
            return true;
        auto *ctx = QOpenGLContext::currentContext();
        if (!ctx)
            return false;
        m_gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(ctx);
        if (!m_gl)
        {
            qWarning() << "XMB wallpaper requires an OpenGL 3.3 context";
            return false;
        }
        m_context = ctx;
        m_gl->initializeOpenGLFunctions();

        if (!compileProgram(m_bgProgram, kBgVertex, kBgFragment) ||
            !compileProgram(m_waveProgram, kWaveVertex, kWaveFragment) ||
            !compileProgram(m_particleProgram, kParticleVertex, kParticleFragment))
        {
            return false;
        }

        const std::array<float, 8> quad = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
        m_gl->glGenVertexArrays(1, &m_bgVao);
        m_gl->glGenBuffers(1, &m_bgVbo);
        m_gl->glBindVertexArray(m_bgVao);
        m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_bgVbo);
        m_gl->glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(quad)), quad.data(), GL_STATIC_DRAW);
        m_gl->glEnableVertexAttribArray(0);
        m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        m_gl->glBindVertexArray(0);

        m_splineData.resize(kSplineTexW * kSplineTexH);
        m_gl->glGenTextures(1, &m_splineTex);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_splineTex);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, kSplineTexW, kSplineTexH, 0, GL_RED, GL_FLOAT, nullptr);
        m_splineGpu = std::make_unique<XmbSpline::GpuGenerator>();
        if (!m_splineGpu->initialize(m_gl, m_splineTex))
        {
            m_splineGpu->cleanup();
            m_splineGpu.reset();
            qWarning() << "XMB GPU spline unavailable, using CPU path";
        }

        m_initialized = true;
        return true;
    }

    void cleanupGrid()
    {
        if (!m_gl)
            return;
        if (m_grid.ibo)
            m_gl->glDeleteBuffers(1, &m_grid.ibo);
        if (m_grid.vbo)
            m_gl->glDeleteBuffers(1, &m_grid.vbo);
        if (m_grid.vao)
            m_gl->glDeleteVertexArrays(1, &m_grid.vao);
        m_grid = {};
    }

    void cleanupParticles()
    {
        if (!m_gl)
            return;
        if (m_particleTrailIbo)
            m_gl->glDeleteBuffers(1, &m_particleTrailIbo);
        if (m_particleVbo)
            m_gl->glDeleteBuffers(1, &m_particleVbo);
        if (m_particleVao)
            m_gl->glDeleteVertexArrays(1, &m_particleVao);
        m_particleTrailIbo = m_particleVbo = m_particleVao = 0;
        m_builtParticleCount = 0;
        m_builtParticleBaseCount = 0;
        m_builtParticleSimulation = -1;
        m_particleTrailIndexCount = 0;
    }

    void cleanup()
    {
        if (!m_gl)
            return;
        cleanupGrid();
        cleanupParticles();
        if (m_splineGpu)
        {
            m_splineGpu->cleanup();
            m_splineGpu.reset();
        }
        if (m_splineTex)
            m_gl->glDeleteTextures(1, &m_splineTex);
        if (m_bgVbo)
            m_gl->glDeleteBuffers(1, &m_bgVbo);
        if (m_bgVao)
            m_gl->glDeleteVertexArrays(1, &m_bgVao);
        m_splineTex = m_bgVbo = m_bgVao = 0;
    }

    void ensureGrid()
    {
        const qreal horizontalSpan = m_virtualSizePx.width() / std::max<qreal>(1.0, m_referenceSizePx.width());
        const int resX = XmbMesh::horizontalResolution(m_meshResolution, horizontalSpan, m_horizontalOverscan);
        const int resY = XmbQuality::verticalResolution(m_quality);
        if (m_grid.resolutionX == resX && m_grid.resolutionY == resY && m_grid.vao)
            return;
        cleanupGrid();

        const auto verts = XmbMesh::gridVertices(resX, resY);

        m_gl->glGenVertexArrays(1, &m_grid.vao);
        m_gl->glGenBuffers(1, &m_grid.vbo);
        m_gl->glGenBuffers(1, &m_grid.ibo);
        m_gl->glBindVertexArray(m_grid.vao);
        m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_grid.vbo);
        m_gl->glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);
        m_gl->glEnableVertexAttribArray(0);
        m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_grid.ibo);
        m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        m_gl->glBindVertexArray(0);
        m_grid.indexCount = 0;
        m_grid.resolutionX = resX;
        m_grid.resolutionY = resY;
    }

    void updateGridVisibleRange()
    {
        if (!m_grid.vao)
            return;
        const int resX = m_grid.resolutionX;
        const qreal width = std::max<qreal>(1.0, m_virtualSizePx.width());
        const qreal zoom = std::max<qreal>(XmbZoom::minimum, m_zoom);
        // Scene-follow mode adds cursorScene to p.x before projection. In
        // virtual pixels this is exactly mouseX minus the desktop center.
        const qreal sceneTranslationX =
            m_pointerFollowMode == 2 && m_pointerStrength > 0.0001
                ? m_pointer.x() - width * 0.5 : 0.0;
        // Invert the exact perspective bounds of the wave shader and reserve
        // only the interaction displacement required for this output. The
        // previous +/-0.40 virtual-width guard shaded large invisible strips.
        const XmbMesh::HorizontalWindow visible = XmbMesh::visibleWorldWindow(
            zoom, m_zoomOffsetPx.x(), m_viewportOriginPx.x(),
            m_viewportSizePx.width(), width, m_referenceSizePx.width(),
            m_interactionRadius, m_pointerStrength, sceneTranslationX);
        const auto columns = XmbMesh::visibleColumns(visible, width, resX, m_horizontalOverscan);
        const int first = columns.first;
        const int last = columns.last;
        if (first < 0)
        {
            m_grid.indexCount = 0;
            m_grid.firstColumn = m_grid.lastColumn = -1;
            return;
        }
        if (first == m_grid.firstColumn && last == m_grid.lastColumn)
            return;
        const auto indices = XmbMesh::gridIndices(resX, m_grid.resolutionY, first, last);
        m_gl->glBindVertexArray(m_grid.vao);
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_grid.ibo);
        m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                           GLsizeiptr(indices.size() * sizeof(quint32)),
                           indices.data(), GL_DYNAMIC_DRAW);
        m_gl->glBindVertexArray(0);
        m_grid.indexCount = GLsizei(indices.size());
        m_grid.firstColumn = first;
        m_grid.lastColumn = last;
    }

    void ensureParticles()
    {
        // The configured value is the actual rendered population.
        const int count = std::clamp(m_particleCount, 100, 30000);
        constexpr int trailSegments = 32;
        const bool volumetric = m_particleSimulation == 1;
        if (m_particleVao && m_builtParticleBaseCount == count && m_builtParticleSimulation == m_particleSimulation)
            return;
        cleanupParticles();

        std::vector<float> seeds;
        seeds.reserve(size_t(count) * (volumetric ? 34u : 5u));
        std::vector<quint32> trailIndices;
        QRandomGenerator rng(0x584D4236u);
        for (int i = 0; i < count; ++i)
        {
            const float a = float(rng.generateDouble());
            const float b = float(rng.generateDouble());
            const float c = std::pow(float(rng.generateDouble()), 8.f) + 0.1f;
            const bool hasTrail = volumetric && (i % 200 == 0);
            const int segments = hasTrail ? trailSegments : 1;
            const quint32 firstVertex = quint32(seeds.size() / 5u);
            for (int segment = 0; segment < segments; ++segment)
            {
                seeds.push_back(a);
                seeds.push_back(b);
                seeds.push_back(c);
                seeds.push_back(segments > 1
                                    ? float(segment) / float(segments - 1)
                                    : 0.0f);
                seeds.push_back(hasTrail ? 1.0f : 0.0f);
            }
            if (hasTrail)
            {
                for (int segment = 0; segment < segments; ++segment)
                {
                    trailIndices.push_back(firstVertex + quint32(segment));
                }
                trailIndices.push_back(std::numeric_limits<quint32>::max());
            }
        }

        m_gl->glGenVertexArrays(1, &m_particleVao);
        m_gl->glGenBuffers(1, &m_particleVbo);
        m_gl->glBindVertexArray(m_particleVao);
        m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
        m_gl->glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(seeds.size() * sizeof(float)), seeds.data(), GL_STATIC_DRAW);
        m_gl->glEnableVertexAttribArray(0);
        m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
        m_gl->glEnableVertexAttribArray(1);
        m_gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                    reinterpret_cast<const void *>(3 * sizeof(float)));
        m_gl->glEnableVertexAttribArray(2);
        m_gl->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                    reinterpret_cast<const void *>(4 * sizeof(float)));
        if (!trailIndices.empty())
        {
            m_gl->glGenBuffers(1, &m_particleTrailIbo);
            m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_particleTrailIbo);
            m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                               GLsizeiptr(trailIndices.size() * sizeof(quint32)),
                               trailIndices.data(), GL_STATIC_DRAW);
            m_particleTrailIndexCount = GLsizei(trailIndices.size());
        }
        m_gl->glBindVertexArray(0);
        m_builtParticleCount = int(seeds.size() / 5u);
        m_builtParticleBaseCount = count;
        m_builtParticleSimulation = m_particleSimulation;
    }

    void generateSplineTexture()
    {
        // Native procedural approximation of the v24 reverse spline source.
        // The visible geometry continues to use the original v24 vertex shader;
        // this texture supplies the low-frequency PS3-like carrier/detail field.
        const float t = float(m_waveTime);
        const float flow = t * 0.125f * float(m_waveSpeedMul);
        XmbSpline::generate(flow, m_splineData.data());
    }

    void updateSplineTexture()
    {
        const float t = float(m_waveTime);
        const float flow = t * 0.125f * float(m_waveSpeedMul);
        if (m_splineBackend == 1 && m_splineGpu && m_splineGpu->render(flow))
            return;
        generateSplineTexture();
        m_gl->glBindTexture(GL_TEXTURE_2D, m_splineTex);
        m_gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        m_gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSplineTexW, kSplineTexH,
                              GL_RED, GL_FLOAT, m_splineData.data());
    }

    void renderBackground()
    {
        m_gl->glDisable(GL_BLEND);
        m_bgProgram.bind();
        m_gl->glBindVertexArray(m_bgVao);

        const float cr = 37.f / 255.f;
        const float cg = 89.f / 255.f;
        const float cb = 179.f / 255.f;
        const float b = float(m_brightness);
        m_bgProgram.setUniformValue("uColorStart", QVector3D(cr * 0.09f * b, cg * 0.09f * b, cb * 0.09f * 1.2f * b));
        m_bgProgram.setUniformValue("uColorEnd", QVector3D(cr * 0.62f * b, cg * 0.62f * b, cb * 0.62f * b));
        m_bgProgram.setUniformValue("uDir", QVector2D(0.f, 1.f));
        m_bgProgram.setUniformValue("uTMin", 0.f);
        m_bgProgram.setUniformValue("uTSpan", 1.f);
        m_bgProgram.setUniformValue("uViewportOriginPx", QVector2D(m_viewportOriginPx));
        m_bgProgram.setUniformValue("uViewportSizePx", QVector2D(m_viewportSizePx.width(), m_viewportSizePx.height()));
        m_bgProgram.setUniformValue("uVirtualSizePx", QVector2D(m_virtualSizePx.width(), m_virtualSizePx.height()));
        m_bgProgram.setUniformValue("uReferenceSizePx", QVector2D(m_referenceSizePx.width(), m_referenceSizePx.height()));
        m_bgProgram.setUniformValue("uMultiscreenDebug", m_multiscreenDebug ? 1.f : 0.f);
        m_gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_gl->glBindVertexArray(0);
        m_bgProgram.release();
    }

    void setCommonWaveUniforms()
    {
        m_waveProgram.setUniformValue("flowSpeed", 0.125f * float(m_waveSpeedMul));
        m_waveProgram.setUniformValue("tension", 0.12f);
        m_waveProgram.setUniformValue("damping", 0.0001f);
        m_waveProgram.setUniformValue("uSplineLength", 0.306001f);
        m_waveProgram.setUniformValue("spacing", 407.658f);
        m_waveProgram.setUniformValue("perturbation", 0.0998587f);
        m_waveProgram.setUniformValue("perturbationScale", 0.07f);
        m_waveProgram.setUniformValue("timeStep", 1.0f);
        m_waveProgram.setUniformValue("waveCosAmp", 0.11f);
        m_waveProgram.setUniformValue("waveBias", -0.1f);
        m_waveProgram.setUniformValue("waveHeightScale", m_renderStyle == 1 ? 1.05f : 0.68f);
        m_waveProgram.setUniformValue("waveSoftClip", 0.22f);
        m_waveProgram.setUniformValue("ffdScale1", QVector3D(5.67726f, 1.00077f, 1.f));
        m_waveProgram.setUniformValue("ffdScale2", QVector3D(2.82755f, 1.27579f, 2.88782f));
        m_waveProgram.setUniformValue("ffdOffset", QVector3D(0.f, -0.469999f, 0.f));
        m_waveProgram.setUniformValue("ffdYAmp", m_renderStyle == 1 ? 0.13f : 0.075f);
        m_waveProgram.setUniformValue("ffdZAmp", m_renderStyle == 1 ? 0.14f : 0.078f);
        m_waveProgram.setUniformValue("zDetailScale", 0.095f);
        m_waveProgram.setUniformValue("brightness", 1.0f);
        m_waveProgram.setUniformValue("fresnelPower", 4.0f);
        m_waveProgram.setUniformValue("fresnelScale", m_renderStyle == 1 ? 0.72f : 0.5f);
        m_waveProgram.setUniformValue("uPointer", QVector2D(float(m_pointer.x()), float(m_pointer.y())));
        m_waveProgram.setUniformValue("uFlow", m_flow);
        m_waveProgram.setUniformValue("uViewport", QVector2D(float(m_viewportLogical.width()), float(m_viewportLogical.height())));
        m_waveProgram.setUniformValue("uPointerStrength", float(m_pointerStrength));
        m_waveProgram.setUniformValue("uInteractionCenter", m_interactionCenter);
        m_waveProgram.setUniformValue("uInteractionRadius", float(m_interactionRadius));
        m_waveProgram.setUniformValue("uInteractionSign", interactionSign());
        m_waveProgram.setUniformValue("uPointerFollowMode", m_pointerFollowMode);
        m_waveProgram.setUniformValue("uRenderStyle", m_renderStyle);
        m_waveProgram.setUniformValue("uSafeRect", QVector4D(0.f, 0.f, 0.f, 0.f));
        m_waveProgram.setUniformValue("uProtected", 0.f);
        const auto domain = XmbMesh::sourceDomain(m_horizontalOverscan);
        m_waveProgram.setUniformValue("uHorizontalDomain", QVector2D(float(domain.left), float(domain.right)));
        m_waveProgram.setUniformValue("uZoom", float(m_zoom));
        m_waveProgram.setUniformValue("uZoomOffsetPx", m_zoomOffsetPx);
        m_waveProgram.setUniformValue("uViewportOriginPx", QVector2D(m_viewportOriginPx));
        m_waveProgram.setUniformValue("uViewportSizePx", QVector2D(m_viewportSizePx.width(), m_viewportSizePx.height()));
        m_waveProgram.setUniformValue("uVirtualSizePx", QVector2D(m_virtualSizePx.width(), m_virtualSizePx.height()));
        m_waveProgram.setUniformValue("uReferenceSizePx", QVector2D(m_referenceSizePx.width(), m_referenceSizePx.height()));
        m_waveProgram.setUniformValue("uSplineTex", 0);
    }

    void renderWave()
    {
        m_gl->glEnable(GL_BLEND);
        m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_waveProgram.bind();
        m_gl->glActiveTexture(GL_TEXTURE0);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_splineTex);
        m_waveProgram.setUniformValue("uSplineTex", 0);
        setCommonWaveUniforms();
        m_gl->glBindVertexArray(m_grid.vao);

        const int upperCount = std::clamp(m_upperFlowCount, 0, 4);
        const int centerCount = std::clamp(m_centerFlowCount, 0, 4);
        const int lowerCount = std::clamp(m_lowerFlowCount, 0, 4);
        const auto laneRandom = [](int key)
        {
            const float raw = std::sin(float(key) * 12.9898f) * 43758.5453f;
            return raw - std::floor(raw);
        };

        // Three real screen regions, not small offsets around the center.
        // OpenGL NDC: +Y is the upper part of the screen, -Y the lower part.
        // Divide the rendered surface into three horizontal bands of 30%.
        // NDC Y is +1 at the top and -1 at the bottom; the centers of the
        // outer bands therefore remain anchored near their respective edges.
        const float upperBase = m_flowGroupOffset;
        const float lowerBase = -m_flowGroupOffset;
        const float regionalSpan = m_flowSpacing;
        const float centerSpan = m_flowSpacing;

        // Upper section. Multiple flows are distributed around the upper third.
        for (int i = 0; i < upperCount; ++i)
        {
            const float centered = (upperCount > 1)
                                       ? (float(i) / float(upperCount - 1) - 0.5f)
                                       : 0.0f;
            const float seed = laneRandom(11 + i);
            m_waveProgram.setUniformValue("uTime", float(m_waveTime + 0.20 + seed * 3.80));
            m_waveProgram.setUniformValue("uWaveVariant", 0.25f + seed * 2.40f);
            m_waveProgram.setUniformValue("uWaveYOffset", upperBase + centered * regionalSpan);
            m_waveProgram.setUniformValue("opacity", 0.68f * std::pow(0.84f, float(i)));
            m_gl->glDrawElements(GL_TRIANGLE_STRIP, m_grid.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        // Center section. Values are symmetric around the visual center.
        for (int i = 0; i < centerCount; ++i)
        {
            const float centered = (centerCount > 1)
                                       ? (float(i) / float(centerCount - 1) - 0.5f)
                                       : 0.0f;
            const float seed = laneRandom(101 + i);
            m_waveProgram.setUniformValue("uTime", float(m_waveTime + 0.20 + seed * 3.80));
            m_waveProgram.setUniformValue("uWaveVariant", 0.25f + seed * 2.40f);
            m_waveProgram.setUniformValue("uWaveYOffset", centered * centerSpan);
            m_waveProgram.setUniformValue("opacity", 0.74f * std::pow(0.84f, float(i)));
            m_gl->glDrawElements(GL_TRIANGLE_STRIP, m_grid.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        // Lower section. Symmetric to the upper section.
        for (int i = 0; i < lowerCount; ++i)
        {
            const float centered = (lowerCount > 1)
                                       ? (float(i) / float(lowerCount - 1) - 0.5f)
                                       : 0.0f;
            const float seed = laneRandom(211 + i);
            m_waveProgram.setUniformValue("uTime", float(m_waveTime + 0.20 + seed * 3.80));
            m_waveProgram.setUniformValue("uWaveVariant", 0.25f + seed * 2.40f);
            m_waveProgram.setUniformValue("uWaveYOffset", lowerBase + centered * regionalSpan);
            m_waveProgram.setUniformValue("opacity", 0.64f * std::pow(0.84f, float(i)));
            m_gl->glDrawElements(GL_TRIANGLE_STRIP, m_grid.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        m_gl->glBindVertexArray(0);
        m_waveProgram.release();
    }

    void renderParticles()
    {
        m_gl->glEnable(GL_BLEND);
        m_gl->glBlendFunc(GL_ONE, GL_ONE);
        m_particleProgram.bind();
        const float aspect = float(m_viewportLogical.width() / std::max<qreal>(1.0, m_viewportLogical.height()));
        const float ratio = std::max(1.f, std::min(aspect, 2.f)) * 0.375f;
        m_particleProgram.setUniformValue("uTime", float(m_particleTime));
        m_particleProgram.setUniformValue("flowSpeed", 0.16f * float(m_particleSpeedMul));
        m_particleProgram.setUniformValue("ratio", ratio);
        m_particleProgram.setUniformValue("ptSizeBase", 2.8f);
        m_particleProgram.setUniformValue("ptSizeVar", 1.7f);
        m_particleProgram.setUniformValue("particleOpacity", 0.88f);
        m_particleProgram.setUniformValue("uPointer", QVector2D(float(m_pointer.x()), float(m_pointer.y())));
        m_particleProgram.setUniformValue("uFlow", m_flow);
        m_particleProgram.setUniformValue("uViewport", QVector2D(float(m_viewportLogical.width()), float(m_viewportLogical.height())));
        m_particleProgram.setUniformValue("uPointerStrength", float(m_pointerStrength));
        m_particleProgram.setUniformValue("uInteractionCenter", m_interactionCenter);
        m_particleProgram.setUniformValue("uInteractionRadius", float(m_interactionRadius));
        m_particleProgram.setUniformValue("uInteractionSign", interactionSign());
        m_particleProgram.setUniformValue("uPointerFollowMode", m_pointerFollowMode);
        m_particleProgram.setUniformValue("uRenderStyle", m_renderStyle);
        m_particleProgram.setUniformValue("uParticleStyle", m_particleStyle);
        m_particleProgram.setUniformValue("uParticleSimulation", m_particleSimulation);
        m_particleProgram.setUniformValue("uTrailPass", 0);
        m_particleProgram.setUniformValue("uSafeRect", QVector4D(0.f, 0.f, 0.f, 0.f));
        m_particleProgram.setUniformValue("uProtected", 0.f);
        const auto domain = XmbMesh::sourceDomain(m_horizontalOverscan);
        m_particleProgram.setUniformValue("uHorizontalDomain", QVector2D(float(domain.left), float(domain.right)));
        m_particleProgram.setUniformValue("uZoom", float(m_zoom));
        m_particleProgram.setUniformValue("uZoomOffsetPx", m_zoomOffsetPx);
        m_particleProgram.setUniformValue("uUpperCount", std::clamp(m_upperFlowCount, 0, 4));
        m_particleProgram.setUniformValue("uCenterCount", std::clamp(m_centerFlowCount, 0, 4));
        m_particleProgram.setUniformValue("uLowerCount", std::clamp(m_lowerFlowCount, 0, 4));
        m_particleProgram.setUniformValue("uFlowGroupOffset", m_flowGroupOffset);
        m_particleProgram.setUniformValue("uFlowSpacing", m_flowSpacing);
        m_particleProgram.setUniformValue("uViewportOriginPx", QVector2D(m_viewportOriginPx));
        m_particleProgram.setUniformValue("uViewportSizePx", QVector2D(m_viewportSizePx.width(), m_viewportSizePx.height()));
        m_particleProgram.setUniformValue("uVirtualSizePx", QVector2D(m_virtualSizePx.width(), m_virtualSizePx.height()));
        m_particleProgram.setUniformValue("uReferenceSizePx", QVector2D(m_referenceSizePx.width(), m_referenceSizePx.height()));
        m_gl->glBindVertexArray(m_particleVao);
        m_gl->glDrawArrays(GL_POINTS, 0, m_builtParticleCount);

        if (m_particleSimulation == 1 && m_particleTrailIndexCount > 0)
        {
            m_particleProgram.setUniformValue("uTrailPass", 1);
            m_gl->glEnable(GL_PRIMITIVE_RESTART);
            m_gl->glPrimitiveRestartIndex(std::numeric_limits<GLuint>::max());
            m_gl->glLineWidth(2.0f);
            m_gl->glDrawElements(GL_LINE_STRIP, m_particleTrailIndexCount,
                                 GL_UNSIGNED_INT, nullptr);
            m_gl->glDisable(GL_PRIMITIVE_RESTART);
        }
        m_gl->glBindVertexArray(0);
        m_particleProgram.release();
    }

private:
    QVector2D interactionSign() const
    {
        const float sx = (m_interactionDirection == 1 || m_interactionDirection == 3) ? -1.f : 1.f;
        const float sy = (m_interactionDirection == 2 || m_interactionDirection == 3) ? -1.f : 1.f;
        return QVector2D(sx, sy);
    }

    QPointer<QOpenGLContext> m_context;
    QOpenGLFunctions_3_3_Core *m_gl = nullptr;
    bool m_initialized = false;
    QOpenGLShaderProgram m_bgProgram;
    QOpenGLShaderProgram m_waveProgram;
    QOpenGLShaderProgram m_particleProgram;

    GLuint m_bgVao = 0;
    GLuint m_bgVbo = 0;
    GLuint m_splineTex = 0;
    GpuGrid m_grid;
    std::vector<float> m_splineData;
    std::unique_ptr<XmbSpline::GpuGenerator> m_splineGpu;

    GLuint m_particleVao = 0;
    GLuint m_particleVbo = 0;
    GLuint m_particleTrailIbo = 0;
    int m_builtParticleCount = 0;
    int m_builtParticleBaseCount = 0;
    int m_builtParticleSimulation = -1;
    GLsizei m_particleTrailIndexCount = 0;

    int m_quality = 2;
    int m_meshResolution = 180;
    int m_particleCount = 12000;
    qreal m_waveTime = 0.0;
    qreal m_particleTime = 0.0;
    bool m_frozen = false;
    bool m_renderingPaused = false;
    qreal m_waveSpeedMul = 1.0;
    qreal m_particleSpeedMul = 1.0;
    QPointF m_pointer;
    QVector2D m_flow;
    qreal m_pointerStrength = 0.0;
    qreal m_interactionRadius = 1.0;
    QVector2D m_interactionCenter = QVector2D(0.f, 0.f);
    QSizeF m_viewportLogical = QSizeF(1.0, 1.0);
    qreal m_brightness = 1.0;
    qreal m_zoom = 1.0;
    QVector2D m_zoomOffsetPx;
    QPointF m_viewportOriginPx;
    QSizeF m_viewportSizePx = QSizeF(1.0, 1.0);
    QSizeF m_virtualSizePx = QSizeF(1.0, 1.0);
    QSizeF m_referenceSizePx = QSizeF(1.0, 1.0);
    bool m_horizontalOverscan = true;
    bool m_multiscreenDebug = false;
    int m_upperFlowCount = 1;
    int m_centerFlowCount = 0;
    int m_lowerFlowCount = 1;
    int m_pointerFollowMode = 1;
    int m_interactionDirection = 0;
    int m_renderStyle = 0;
    int m_splineBackend = 1;
    int m_particleStyle = 0;
    int m_particleSimulation = 0;
    float m_flowGroupOffset = 1.28f;
    float m_flowSpacing = 0.68f;
};

XmbRendererItem::XmbRendererItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);

    // Input-transparent by design. Plasma desktop icons/menus keep all events;
    // the renderer observes cursor state by polling Qt globally instead.
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptHoverEvents(false);
    setAcceptTouchEvents(false);
    if (QGuiApplication::instance())
    {
        QGuiApplication::instance()->installEventFilter(this);
    }
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_frameTimer, &QTimer::timeout, this, &XmbRendererItem::tick);
    connect(this, &QQuickItem::visibleChanged, this, &XmbRendererItem::updateTimerInterval);
    connect(OutputVisibilityController::instance(), &OutputVisibilityController::outputFrozenChanged,
            this, [this](const QString &outputName, bool frozen) {
        QScreen *screen = window() ? window()->screen() : nullptr;
        if (screen && screen->name() == outputName)
            setFrozen(m_pauseWhenCovered && frozen);
    });
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *newWindow) {
        QObject::disconnect(m_screenChangedConnection);
        if (newWindow)
            m_screenChangedConnection = connect(newWindow, &QQuickWindow::screenChanged,
                                                this, &XmbRendererItem::updateOutputVisibility);
        updateOutputVisibility();
    });
    m_clock.start();
    m_lastNs = m_clock.nsecsElapsed();
    updateTimerInterval();
}

XmbRendererItem::~XmbRendererItem()
{
    if (QGuiApplication::instance())
    {
        QGuiApplication::instance()->removeEventFilter(this);
    }
}

QQuickFramebufferObject::Renderer *XmbRendererItem::createRenderer() const
{
    return new XmbRenderer();
}

void XmbRendererItem::updateOutputVisibility()
{
    QScreen *screen = window() ? window()->screen() : nullptr;
    setFrozen(m_pauseWhenCovered && screen
        && OutputVisibilityController::instance()->isFrozen(screen->name()));
}

int XmbRendererItem::meshResolution() const
{
    return XmbQuality::resolution(m_quality);
}

void XmbRendererItem::updateTimerInterval()
{
    const int interval = std::max(1, int(std::round(1000.0 / double(std::clamp(m_targetFps, 15, 240)))));
    m_frameTimer.setInterval(interval);
    if (!m_frozen && !m_renderingPaused
        && (!m_pauseWhenHidden || isVisible()))
        m_frameTimer.start();
    else
        m_frameTimer.stop();
}

void XmbRendererItem::updateDesktopGeometry()
{
    const qreal localWidth = std::max<qreal>(1.0, width());
    const qreal localHeight = std::max<qreal>(1.0, height());

    if (!m_linkAcrossScreens)
    {
        m_viewportOriginPx = QPointF();
        m_viewportSizePx = QSizeF(localWidth, localHeight);
        m_virtualSizePx = m_viewportSizePx;
        m_referenceSizePx = m_viewportSizePx;
        return;
    }

    QScreen *current = window() ? window()->screen() : nullptr;
    if (!current)
    {
        m_viewportOriginPx = QPointF();
        m_viewportSizePx = QSizeF(localWidth, localHeight);
        m_virtualSizePx = m_viewportSizePx;
        m_referenceSizePx = m_viewportSizePx;
        return;
    }

    // virtualSiblings() is the set of outputs that share one desktop
    // coordinate system. virtualGeometry() is their exact logical-pixel union.
    const QList<QScreen *> siblings = current->virtualSiblings();
    const QRect virtualRect = current->virtualGeometry();
    const QRect currentRect = current->geometry();

    if (siblings.isEmpty() || virtualRect.width() <= 0 || virtualRect.height() <= 0 || currentRect.width() <= 0 || currentRect.height() <= 0)
    {
        m_viewportOriginPx = QPointF();
        m_viewportSizePx = QSizeF(localWidth, localHeight);
        m_virtualSizePx = m_viewportSizePx;
        m_referenceSizePx = m_viewportSizePx;
        return;
    }

    // Normalize the compositor's possibly negative global origin to (0,0).
    // This is a complete 2D rectangle, so vertically offset outputs are also
    // crops of the same virtual surface rather than independent wallpapers.
    m_viewportOriginPx = QPointF(
        qreal(currentRect.x() - virtualRect.x()),
        qreal(currentRect.y() - virtualRect.y()));
    m_viewportSizePx = QSizeF(currentRect.size());
    m_virtualSizePx = QSizeF(virtualRect.size());

    // Preserve the spatial frequency in pixels instead of stretching one wave
    // to fill the combined desktop. The phase reference is selected entirely
    // from the active QScreen topology; connector names and development
    // resolutions are never used.
    QScreen *reference = current;
    for (QScreen *screen : siblings)
    {
        if (!screen)
            continue;
        const QRect g = screen->geometry();
        const QRect rg = reference->geometry();
        if (g.x() < rg.x() || (g.x() == rg.x() && g.width() < rg.width()))
        {
            reference = screen;
        }
    }
    m_referenceSizePx = QSizeF(reference->geometry().size());

    // Logical geometries are integer-valued in this topology. Rounding here
    // prevents sub-pixel noise from ever creating two slightly different seam
    // uniforms if the platform reports a floating conversion internally.
    m_viewportOriginPx = QPointF(
        std::round(m_viewportOriginPx.x()), std::round(m_viewportOriginPx.y()));
    m_viewportSizePx = QSizeF(
        std::round(m_viewportSizePx.width()), std::round(m_viewportSizePx.height()));
    m_virtualSizePx = QSizeF(
        std::round(m_virtualSizePx.width()), std::round(m_virtualSizePx.height()));
    m_referenceSizePx = QSizeF(
        std::max<qreal>(1.0, std::round(m_referenceSizePx.width())),
        std::max<qreal>(1.0, std::round(m_referenceSizePx.height())));

    static QHash<const XmbRendererItem *, QString> lastGeometry;
    const QString signature = QStringLiteral("%1|%2,%3|%4x%5|%6x%7|%8x%9")
                                  .arg(current->name())
                                  .arg(m_viewportOriginPx.x(), 0, 'f', 0)
                                  .arg(m_viewportOriginPx.y(), 0, 'f', 0)
                                  .arg(m_viewportSizePx.width(), 0, 'f', 0)
                                  .arg(m_viewportSizePx.height(), 0, 'f', 0)
                                  .arg(m_virtualSizePx.width(), 0, 'f', 0)
                                  .arg(m_virtualSizePx.height(), 0, 'f', 0)
                                  .arg(m_referenceSizePx.width(), 0, 'f', 0)
                                  .arg(m_referenceSizePx.height(), 0, 'f', 0);
    if (lastGeometry.value(this) != signature)
    {
        lastGeometry.insert(this, signature);
        qInfo().noquote()
            << "XMB multiscreen world:" << current->name()
            << "viewportOrigin=" << m_viewportOriginPx
            << "viewportSize=" << m_viewportSizePx
            << "virtualSize=" << m_virtualSizePx
            << "referenceSize=" << m_referenceSizePx;
    }
}

void XmbRendererItem::pollSystemPointer()
{
    if (!m_interactionEnabled || width() <= 0.0 || height() <= 0.0)
    {
        m_pointerPresent = false;
        m_leftHeld = false;
        m_systemPointerInitialized = false;
        return;
    }

    const QPointF globalPos = QPointF(QCursor::pos());
    QPointF pointerPos;
    bool inside = false;

    if (m_linkAcrossScreens && window() && window()->screen())
    {
        QScreen *current = window()->screen();
        const QRect virtualRect = current->virtualGeometry();
        for (QScreen *screen : current->virtualSiblings())
        {
            if (screen && screen->geometry().contains(globalPos.toPoint()))
            {
                inside = true;
                break;
            }
        }
        pointerPos = globalPos - QPointF(virtualRect.topLeft());
    }
    else
    {
        pointerPos = mapFromGlobal(globalPos);
        inside = pointerPos.x() >= 0.0 && pointerPos.y() >= 0.0 && pointerPos.x() < width() && pointerPos.y() < height();
    }

    if (!inside)
    {
        m_pointerPresent = false;
        m_leftHeld = false;
        m_systemPointerInitialized = false;
        return;
    }

    if (!m_systemPointerInitialized || !m_pointerPresent)
    {
        m_pointer = pointerPos;
        m_prevPointer = pointerPos;
        m_flow = QVector2D();
        m_systemPointerInitialized = true;
    }
    else
    {
        m_pointer = pointerPos;
    }

    m_pointerPresent = true;
    const Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
    m_leftHeld = buttons.testFlag(Qt::LeftButton);
}

bool XmbRendererItem::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (!m_interactionEnabled || width() <= 0.0 || height() <= 0.0)
    {
        return false;
    }

    if (event->type() == QEvent::Wheel)
    {
        auto *wheel = static_cast<QWheelEvent *>(event);
        QPointF pivotPos;
        QRectF activeViewport;
        bool inside = false;

        if (m_linkAcrossScreens && window() && window()->screen())
        {
            QScreen *current = window()->screen();
            // Wayland does not give the wallpaper a reliable global pointer
            // position: globalPosition() can stay output-relative. The wheel
            // recipient is the desktop view of the output under the pointer,
            // so the recipient window identifies the active output and
            // position() is already local to it. Every linked instance still
            // resolves the same event to one shared camera.
            if (QQuickWindow *recipient = qobject_cast<QQuickWindow *>(watched);
                recipient && recipient->screen())
            {
                const QPoint virtualOrigin = current->virtualGeometry().topLeft();
                const QRect geometry = recipient->screen()->geometry();
                activeViewport = QRectF(
                    QPointF(geometry.topLeft() - virtualOrigin),
                    QSizeF(geometry.size()));
                pivotPos = activeViewport.topLeft() + wheel->position();
                inside = true;
            }
            else
            {
                const QPointF globalPos = wheel->globalPosition();
                for (QScreen *screen : current->virtualSiblings())
                {
                    if (screen && screen->geometry().contains(globalPos.toPoint()))
                    {
                        inside = true;
                        const QRect geometry = screen->geometry();
                        const QPoint virtualOrigin = current->virtualGeometry().topLeft();
                        activeViewport = QRectF(
                            QPointF(geometry.topLeft() - virtualOrigin),
                            QSizeF(geometry.size()));
                        break;
                    }
                }
                pivotPos = globalPos - QPointF(current->virtualGeometry().topLeft());
            }
        }
        else
        {
            pivotPos = mapFromGlobal(wheel->globalPosition());
            inside = pivotPos.x() >= 0.0 && pivotPos.y() >= 0.0 && pivotPos.x() < width() && pivotPos.y() < height();
            activeViewport = QRectF(QPointF(), QSizeF(width(), height()));
        }

        if (inside)
        {
            updateDesktopGeometry();
            qreal steps = wheel->angleDelta().y() / 120.0;
            if (qFuzzyIsNull(steps) && wheel->modifiers().testFlag(Qt::ShiftModifier))
                steps = wheel->angleDelta().x() / 120.0;
            if (qFuzzyIsNull(steps) && !wheel->pixelDelta().isNull())
            {
                steps = wheel->pixelDelta().y() / 120.0;
            }
            if (!qFuzzyIsNull(steps))
            {
                // Shift+wheel selects a camera destination without changing
                // magnification, including at the exact 0.40x minimum.
                const bool navigateOnly = wheel->modifiers().testFlag(Qt::ShiftModifier);
                const qreal limitProximity = steps > 0.0
                                                 ? std::clamp<qreal>((XmbZoom::maximum - m_targetZoom) / 0.70, 0.0, 1.0)
                                                 : std::clamp<qreal>((m_targetZoom - XmbZoom::minimum) / 0.70, 0.0, 1.0);
                const qreal softenedStep = steps * (0.20 + 0.80 * limitProximity);
                const qreal requestedZoom = navigateOnly ? m_targetZoom : std::clamp<qreal>(
                    m_targetZoom + softenedStep * (0.025 * m_zoomSensitivity),
                    XmbZoom::minimum, XmbZoom::maximum);
                // Every linked wallpaper instance observes the same global
                // wheel event. Inward zoom navigates toward the cursor even
                // before reaching the maximum; outward zoom keeps its
                // existing pivot behavior.
                const QSizeF surfaceSize(
                    m_linkAcrossScreens ? m_virtualSizePx.width() : width(),
                    m_linkAcrossScreens ? m_virtualSizePx.height() : height());
                const QPointF cursor = pivotPos;
                m_zoomViewport = activeViewport;
                const QPointF destination = XmbZoom::wheelTarget(
                    m_targetZoom, requestedZoom,
                    XmbZoom::constrainNavigation(m_targetZoom, m_targetZoomOffset, surfaceSize, m_horizontalOverscan),
                    cursor, surfaceSize, activeViewport,
                    steps > 0.0, navigateOnly, m_horizontalOverscan);
                if (qFuzzyCompare(requestedZoom, m_targetZoom)
                    && std::hypot(destination.x() - m_targetZoomOffset.x(),
                                  destination.y() - m_targetZoomOffset.y()) < 0.01)
                    return false;
                m_zoomReturningHome = !navigateOnly && steps < 0.0;
                m_targetZoomOffset = destination;
                m_baseZoomNeedsCentering = false;
                m_zoomChangeFromWheel = true;
                setZoom(requestedZoom);
                m_zoomChangeFromWheel = false;
            }
        }
    }
    return false;
}

void XmbRendererItem::tick()
{
    if (m_frozen || m_renderingPaused
        || (m_pauseWhenHidden && !isVisible()))
        return;

    const qint64 nowNs = m_clock.nsecsElapsed();
    const qreal dt = std::clamp<qreal>((nowNs - m_lastNs) / 1.0e9, 0.001, 0.05);
    m_lastNs = nowNs;

    updateDesktopGeometry();

    if (m_linkAcrossScreens && !m_frozen)
    {
        // All wallpaper instances inside plasmashell share exactly the same
        // animation clock, so a flow does not jump in phase at a monitor seam.
        const qreal shared = sharedAnimationSeconds();
        m_waveTime = shared;
        m_particleTime = shared + 137.0;
    }
    else
    {
        m_waveTime += dt;
        m_particleTime += dt;
    }

    // Keep interaction live while Plasma routes events to icons, menus, panels
    // or desktop selection instead of the wallpaper item itself.
    pollSystemPointer();

    const qreal vx = (m_pointer.x() - m_prevPointer.x()) / dt;
    const qreal vy = (m_pointer.y() - m_prevPointer.y()) / dt;
    m_prevPointer = m_pointer;
    const qreal flowEase = 1.0 - std::exp(-dt * 18.0);
    const qreal targetFx = m_pointerPresent ? std::clamp(vx * 0.006, -8.0, 8.0) : 0.0;
    const qreal targetFy = m_pointerPresent ? std::clamp(vy * 0.006, -8.0, 8.0) : 0.0;
    m_flow.setX(float(m_flow.x() + (targetFx - m_flow.x()) * flowEase));
    m_flow.setY(float(m_flow.y() + (targetFy - m_flow.y()) * flowEase));

    const qreal target = (m_interactionEnabled && m_pointerPresent)
                             ? (m_leftHeld ? 0.0 : 0.82 * m_mouseStrength)
                             : 0.0;
    const qreal rate = target < m_interactionStrength ? 11.0 : 3.0;
    const qreal oldStrength = m_interactionStrength;
    m_interactionStrength += (target - m_interactionStrength) * (1.0 - std::exp(-dt * rate));
    if (m_interactionStrength < 0.0005)
        m_interactionStrength = 0.0;
    if (!qFuzzyCompare(oldStrength, m_interactionStrength))
        emit interactionStrengthChanged();

    const qreal zoomEase = 1.0 - std::exp(-dt * (m_zoomReturningHome ? 2.5 : 5.0));
    const QSizeF surfaceSize(
        m_linkAcrossScreens ? m_virtualSizePx.width() : width(),
        m_linkAcrossScreens ? m_virtualSizePx.height() : height());
    if (m_baseZoomNeedsCentering)
    {
        const QRectF desktop(QPointF(), surfaceSize);
        m_targetZoomOffset = XmbZoom::focusAtCursor(m_targetZoom, desktop.center(), surfaceSize, m_zoomViewport, m_horizontalOverscan);
        m_baseZoomNeedsCentering = false;
    }
    m_targetZoomOffset = XmbZoom::constrainNavigation(m_targetZoom, m_targetZoomOffset, surfaceSize, m_horizontalOverscan);
    m_zoom += (m_targetZoom - m_zoom) * zoomEase;
    m_zoomOffset += (m_targetZoomOffset - m_zoomOffset) * zoomEase;
    if (std::abs(m_targetZoom - m_zoom) < 0.0005)
        m_zoom = m_targetZoom;
    if (std::hypot(m_zoomOffset.x() - m_targetZoomOffset.x(),
                   m_zoomOffset.y() - m_targetZoomOffset.y()) < 0.01)
    {
        m_zoomOffset = m_targetZoomOffset;
    }
    // Clamp after both interpolation and snapping. Clamping before the final
    // snap can leave a small but visible overshoot at the virtual edge.
    m_zoomOffset = XmbZoom::constrainNavigation(m_zoom, m_zoomOffset, surfaceSize, m_horizontalOverscan);

    update();
}

void XmbRendererItem::setQuality(int value)
{
    value = std::clamp(value, 0, XmbQuality::maximum);    if (m_quality == value)
        return;
    m_quality = value;
    emit qualityChanged();
    update();
}
void XmbRendererItem::setTargetFps(int value)
{
    value = std::clamp(value, 15, 240);
    if (m_targetFps == value)
        return;
    m_targetFps = value;
    emit targetFpsChanged();
    updateTimerInterval();
}
void XmbRendererItem::setParticleCount(int value)
{
    value = std::clamp(value, 100, 30000);
    if (m_particleCount == value)
        return;
    m_particleCount = value;
    emit particleCountChanged();
    update();
}
void XmbRendererItem::setWaveSpeed(qreal value)
{
    value = std::clamp<qreal>(value, 0.05, 4.0);
    if (qFuzzyCompare(m_waveSpeed, value))
        return;
    m_waveSpeed = value;
    emit waveSpeedChanged();
}
void XmbRendererItem::setParticleSpeed(qreal value)
{
    value = std::clamp<qreal>(value, 0.05, 4.0);
    if (qFuzzyCompare(m_particleSpeed, value))
        return;
    m_particleSpeed = value;
    emit particleSpeedChanged();
}
void XmbRendererItem::setMouseStrength(qreal value)
{
    value = std::clamp<qreal>(value, 0.0, 4.0);
    if (qFuzzyCompare(m_mouseStrength, value))
        return;
    m_mouseStrength = value;
    emit mouseStrengthChanged();
}
void XmbRendererItem::setInteractionRadius(qreal value)
{
    value = std::clamp<qreal>(value, 0.40, 2.50);
    if (qFuzzyCompare(m_interactionRadius, value))
        return;
    m_interactionRadius = value;
    emit interactionRadiusChanged();
    update();
}
void XmbRendererItem::setBrightness(qreal value)
{
    value = std::clamp<qreal>(value, 0.2, 1.8);
    if (qFuzzyCompare(m_brightness, value))
        return;
    m_brightness = value;
    emit brightnessChanged();
    update();
}
void XmbRendererItem::setZoom(qreal value)
{
    // Previous maximum was 1.85x; v1.0.6 raises it exactly threefold.
    value = std::clamp<qreal>(value, 0.40, 3.50);
    if (qFuzzyCompare(m_targetZoom, value))
        return;
    m_targetZoom = value;
    if (!m_zoomChangeFromWheel)
    {
        m_zoomReturningHome = false;
        // A saved/base zoom change is not a navigation gesture. In particular
        // 0.40x must start in the same centered frame as zooming out to it.
        // Defer until tick(), when virtual geometry has been initialized.
        m_baseZoomNeedsCentering = true;
    }
    emit zoomChanged();
}
void XmbRendererItem::setZoomSensitivity(qreal value)
{
    value = std::clamp<qreal>(value, 0.20, 2.00);
    if (qFuzzyCompare(m_zoomSensitivity, value))
        return;
    m_zoomSensitivity = value;
    emit zoomSensitivityChanged();
}
void XmbRendererItem::setFlowGroupOffset(qreal value)
{
    value = std::clamp<qreal>(value, 0.45, 1.60);
    if (qFuzzyCompare(m_flowGroupOffset, value))
        return;
    m_flowGroupOffset = value;
    emit flowGroupOffsetChanged();
    update();
}
void XmbRendererItem::setFlowSpacing(qreal value)
{
    value = std::clamp<qreal>(value, 0.10, 1.20);
    if (qFuzzyCompare(m_flowSpacing, value))
        return;
    m_flowSpacing = value;
    emit flowSpacingChanged();
    update();
}
void XmbRendererItem::setUpperFlowCount(int value)
{
    value = std::clamp(value, 0, 4);
    if (m_upperFlowCount == value)
        return;
    m_upperFlowCount = value;
    emit upperFlowCountChanged();
    update();
}
void XmbRendererItem::setCenterFlowCount(int value)
{
    value = std::clamp(value, 0, 4);
    if (m_centerFlowCount == value)
        return;
    m_centerFlowCount = value;
    emit centerFlowCountChanged();
    update();
}
void XmbRendererItem::setLowerFlowCount(int value)
{
    value = std::clamp(value, 0, 4);
    if (m_lowerFlowCount == value)
        return;
    m_lowerFlowCount = value;
    emit lowerFlowCountChanged();
    update();
}
void XmbRendererItem::setLinkAcrossScreens(bool value)
{
    if (m_linkAcrossScreens == value)
        return;
    m_linkAcrossScreens = value;
    updateDesktopGeometry();
    emit linkAcrossScreensChanged();
    update();
}

void XmbRendererItem::setHorizontalOverscan(bool value)
{
    if (m_horizontalOverscan == value)
        return;
    m_horizontalOverscan = value;
    updateDesktopGeometry();
    m_zoomOffset = XmbZoom::constrainNavigation(m_zoom, m_zoomOffset, m_virtualSizePx, m_horizontalOverscan);
    m_targetZoomOffset = XmbZoom::constrainNavigation(m_targetZoom, m_targetZoomOffset, m_virtualSizePx, m_horizontalOverscan);
    emit horizontalOverscanChanged();
    update();
}

void XmbRendererItem::setMultiscreenDebug(bool value)
{
    if (m_multiscreenDebug == value)
        return;
    m_multiscreenDebug = value;
    emit multiscreenDebugChanged();
    update();
}

void XmbRendererItem::setInteractionEnabled(bool value)
{
    if (m_interactionEnabled == value)
        return;
    m_interactionEnabled = value;
    if (!m_interactionEnabled)
    {
        m_pointerPresent = false;
        m_leftHeld = false;
        m_systemPointerInitialized = false;
        m_flow = QVector2D();
    }
    emit interactionEnabledChanged();
}
void XmbRendererItem::setPointerFollowMode(int value)
{
    value = std::clamp(value, 0, 2);
    if (m_pointerFollowMode == value)
        return;
    m_pointerFollowMode = value;
    emit pointerFollowModeChanged();
    update();
}
void XmbRendererItem::setInteractionDirection(int value)
{
    value = std::clamp(value, 0, 3);
    if (m_interactionDirection == value)
        return;
    m_interactionDirection = value;
    emit interactionDirectionChanged();
    update();
}

void XmbRendererItem::setRenderStyle(int value)
{
    value = std::clamp(value, 0, 1);
    if (m_renderStyle == value)
        return;
    m_renderStyle = value;
    emit renderStyleChanged();
    update();
}

void XmbRendererItem::setSplineBackend(int value)
{
    value = std::clamp(value, 0, 1);
    if (m_splineBackend == value)
        return;
    m_splineBackend = value;
    emit splineBackendChanged();
    update();
}

void XmbRendererItem::setParticleStyle(int value)
{
    value = std::clamp(value, 0, 2);
    if (m_particleStyle == value)
        return;
    m_particleStyle = value;
    emit particleStyleChanged();
    update();
}
void XmbRendererItem::setParticleSimulation(int value)
{
    value = std::clamp(value, 0, 1);
    if (m_particleSimulation == value)
        return;
    m_particleSimulation = value;
    emit particleSimulationChanged();
    update();
}
void XmbRendererItem::setPauseWhenHidden(bool value)
{
    if (m_pauseWhenHidden == value)
        return;
    m_pauseWhenHidden = value;
    emit pauseWhenHiddenChanged();
    updateTimerInterval();
}

void XmbRendererItem::setPauseWhenCovered(bool value)
{
    if (m_pauseWhenCovered == value)
        return;
    m_pauseWhenCovered = value;
    emit pauseWhenCoveredChanged();
    updateOutputVisibility();
}

void XmbRendererItem::setRenderingPaused(bool value)
{
    if (m_renderingPaused == value)
        return;
    m_renderingPaused = value;
    if (!value)
    {
        m_lastNs = m_clock.nsecsElapsed();
        if (m_linkAcrossScreens)
        {
            const qreal shared = sharedAnimationSeconds();
            m_waveTime = shared;
            m_particleTime = shared + 137.0;
        }
    }
    updateTimerInterval();
    emit renderingPausedChanged();
    if (!value && !m_frozen)
        update();
}

void XmbRendererItem::setFrozen(bool value)
{
    if (m_frozen == value)
        return;

    m_frozen = value;
    if (!m_frozen)
    {
        m_lastNs = m_clock.nsecsElapsed();
        if (m_linkAcrossScreens)
        {
            const qreal shared = sharedAnimationSeconds();
            m_waveTime = shared;
            m_particleTime = shared + 137.0;
        }
    }
    updateTimerInterval();
    emit frozenChanged();
    if (!m_frozen && !m_renderingPaused)
        update();
}

void XmbRendererItem::pointerMoved(qreal x, qreal y, bool present)
{
    m_pointer = QPointF(x, y);
    m_pointerPresent = present;
}
void XmbRendererItem::pointerEntered(qreal x, qreal y)
{
    m_pointer = m_prevPointer = QPointF(x, y);
    m_pointerPresent = true;
}
void XmbRendererItem::pointerLeft()
{
    m_pointerPresent = false;
    m_leftHeld = false;
    m_systemPointerInitialized = false;
}
void XmbRendererItem::leftPressed(qreal x, qreal y)
{
    m_pointer = QPointF(x, y);
    m_pointerPresent = true;
    m_leftHeld = true;
}
void XmbRendererItem::leftReleased(qreal x, qreal y)
{
    m_pointer = QPointF(x, y);
    m_pointerPresent = true;
    m_leftHeld = false;
}
void XmbRendererItem::resetAnimation()
{
    m_zoomReturningHome = false;
    m_waveTime = 0.0;
    m_particleTime = 137.0;
    m_interactionStrength = 0.0;
    m_flow = QVector2D();
    const QSizeF surfaceSize(
        m_linkAcrossScreens ? m_virtualSizePx.width() : width(),
        m_linkAcrossScreens ? m_virtualSizePx.height() : height());
    const QPointF center = QPointF(surfaceSize.width() * 0.5, surfaceSize.height() * 0.5);
    m_zoomOffset = XmbZoom::focusAtCursor(m_zoom, center, surfaceSize, m_zoomViewport, m_horizontalOverscan);
    m_targetZoomOffset = XmbZoom::focusAtCursor(m_targetZoom, center, surfaceSize, m_zoomViewport, m_horizontalOverscan);
    m_baseZoomNeedsCentering = false;
    update();
}
