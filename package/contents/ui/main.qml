import QtQuick
import org.kde.plasma.plasmoid
import "xmbnative" as Native

WallpaperItem {
    id: root

    readonly property var cfg: {
        if (typeof wallpaper !== "undefined" && wallpaper && wallpaper.configuration)
            return wallpaper.configuration
        return configuration
    }

    Native.XmbRenderer {
        id: renderer
        anchors.fill: parent

        quality: Number(root.cfg.quality ?? 3)
        targetFps: Number(root.cfg.targetFps ?? 120)
        particleCount: Number(root.cfg.particleCount ?? 12000)
        waveSpeed: Number(root.cfg.waveSpeed ?? 1.0)
        particleSpeed: Number(root.cfg.particleSpeed ?? 1.0)
        particleStyle: Number(root.cfg.particleStyle ?? 0)
        particleSimulation: Number(root.cfg.particleSimulation ?? 0)
        mouseStrength: Number(root.cfg.mouseStrength ?? 1.0)
        interactionRadius: Number(root.cfg.interactionRadius ?? 1.0)
        brightness: Number(root.cfg.brightness ?? 1.0)
        zoom: Math.max(Boolean(root.cfg.horizontalOverscan ?? true) ? 0.40 : 1.0, Number(root.cfg.zoom ?? 1.0))
        zoomSensitivity: Number(root.cfg.zoomSensitivity ?? 0.60)
        flowGroupOffset: Number(root.cfg.flowGroupOffset ?? 1.28)
        flowSpacing: Number(root.cfg.flowSpacing ?? 0.68)
        upperFlowCount: Number(root.cfg.upperFlowCount ?? 1)
        centerFlowCount: Number(root.cfg.centerFlowCount ?? 0)
        lowerFlowCount: Number(root.cfg.lowerFlowCount ?? 1)
        linkAcrossScreens: Boolean(root.cfg.linkAcrossScreens ?? false)
        horizontalOverscan: Boolean(root.cfg.horizontalOverscan ?? true)
        multiscreenDebug: Boolean(root.cfg.multiscreenDebug ?? false)
        interactionEnabled: Boolean(root.cfg.interactionEnabled ?? true)
        pointerFollowMode: Number(root.cfg.pointerFollowMode ?? 1)
        interactionDirection: Number(root.cfg.interactionDirection ?? 0)
        renderStyle: Number(root.cfg.renderStyle ?? 0)
        splineBackend: Number(root.cfg.splineBackend ?? 1)
        pauseWhenHidden: Boolean(root.cfg.pauseWhenHidden ?? true)
        pauseWhenCovered: Boolean(root.cfg.pauseWhenHidden ?? true)
        renderingPaused: Boolean(root.cfg.renderingPaused ?? false)
        visible: !renderer.renderingPaused && !renderer.frozen
    }
}
