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
        particleCount: Number(root.cfg.particleCount ?? 4200)
        waveSpeed: Number(root.cfg.waveSpeed ?? 1.0)
        particleSpeed: Number(root.cfg.particleSpeed ?? 1.0)
        particleStyle: Number(root.cfg.particleStyle ?? 0)
        particleSimulation: Number(root.cfg.particleSimulation ?? 0)
        mouseStrength: Number(root.cfg.mouseStrength ?? 1.0)
        interactionRadius: Number(root.cfg.interactionRadius ?? 1.0)
        brightness: Number(root.cfg.brightness ?? 1.0)
        zoom: Number(root.cfg.zoom ?? 1.0)
        zoomSensitivity: Number(root.cfg.zoomSensitivity ?? 0.60)
        upperFlowCount: Number(root.cfg.upperFlowCount ?? 1)
        centerFlowCount: Number(root.cfg.centerFlowCount ?? 0)
        lowerFlowCount: Number(root.cfg.lowerFlowCount ?? 1)
        linkAcrossScreens: Boolean(root.cfg.linkAcrossScreens ?? false)
        multiscreenDebug: Boolean(root.cfg.multiscreenDebug ?? false)
        interactionEnabled: Boolean(root.cfg.interactionEnabled ?? true)
        pointerFollowMode: Number(root.cfg.pointerFollowMode ?? 1)
        interactionDirection: Number(root.cfg.interactionDirection ?? 0)
        pauseWhenHidden: Boolean(root.cfg.pauseWhenHidden ?? true)
    }
}
