import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "xmbnative" as Native

Item {
    id: root
    anchors.fill: parent
    clip: true
    focus: true
    activeFocusOnTab: true
    Component.onCompleted: scrollView.forceActiveFocus()
    onVisibleChanged: if (visible) scrollView.forceActiveFocus()

    property var configDialog: null
    property var wallpaperConfiguration: (configDialog && configDialog.wallpaperConfiguration)
        ? configDialog.wallpaperConfiguration
        : ((typeof wallpaper !== "undefined" && wallpaper && wallpaper.configuration)
            ? wallpaper.configuration
            : null)

    signal configurationChanged()
    function saveConfig() {}

    property int cfg_quality: wallpaperConfiguration ? (wallpaperConfiguration.quality ?? 3) : 3
    property int cfg_targetFps: wallpaperConfiguration ? (wallpaperConfiguration.targetFps ?? 120) : 120
    property int cfg_particleCount: wallpaperConfiguration ? (wallpaperConfiguration.particleCount ?? 12000) : 12000
    property double cfg_waveSpeed: wallpaperConfiguration ? (wallpaperConfiguration.waveSpeed ?? 1.0) : 1.0
    property double cfg_particleSpeed: wallpaperConfiguration ? (wallpaperConfiguration.particleSpeed ?? 1.0) : 1.0
    property double cfg_mouseStrength: wallpaperConfiguration ? (wallpaperConfiguration.mouseStrength ?? 1.0) : 1.0
    property double cfg_interactionRadius: wallpaperConfiguration ? (wallpaperConfiguration.interactionRadius ?? 1.0) : 1.0
    property double cfg_brightness: wallpaperConfiguration ? (wallpaperConfiguration.brightness ?? 1.0) : 1.0
    property double cfg_zoom: wallpaperConfiguration ? (wallpaperConfiguration.zoom ?? 1.0) : 1.0
    property double cfg_zoomSensitivity: wallpaperConfiguration ? (wallpaperConfiguration.zoomSensitivity ?? 0.60) : 0.60
    property double cfg_flowGroupOffset: wallpaperConfiguration ? (wallpaperConfiguration.flowGroupOffset ?? 1.28) : 1.28
    property double cfg_flowSpacing: wallpaperConfiguration ? (wallpaperConfiguration.flowSpacing ?? 0.68) : 0.68
    property int cfg_upperFlowCount: wallpaperConfiguration ? (wallpaperConfiguration.upperFlowCount ?? 1) : 1
    property int cfg_centerFlowCount: wallpaperConfiguration ? (wallpaperConfiguration.centerFlowCount ?? 0) : 0
    property int cfg_lowerFlowCount: wallpaperConfiguration ? (wallpaperConfiguration.lowerFlowCount ?? 1) : 1
    property bool cfg_linkAcrossScreens: wallpaperConfiguration ? (wallpaperConfiguration.linkAcrossScreens ?? false) : false
    property bool cfg_horizontalOverscan: wallpaperConfiguration ? (wallpaperConfiguration.horizontalOverscan ?? true) : true
    property bool cfg_multiscreenDebug: wallpaperConfiguration ? (wallpaperConfiguration.multiscreenDebug ?? false) : false
    property bool cfg_interactionEnabled: wallpaperConfiguration ? (wallpaperConfiguration.interactionEnabled ?? true) : true
    property int cfg_pointerFollowMode: wallpaperConfiguration ? (wallpaperConfiguration.pointerFollowMode ?? 1) : 1
    property int cfg_interactionDirection: wallpaperConfiguration ? (wallpaperConfiguration.interactionDirection ?? 0) : 0
    property int cfg_splineBackend: wallpaperConfiguration ? (wallpaperConfiguration.splineBackend ?? 1) : 1
    property int cfg_renderStyle: wallpaperConfiguration ? (wallpaperConfiguration.renderStyle ?? 0) : 0
    property int cfg_particleStyle: wallpaperConfiguration ? (wallpaperConfiguration.particleStyle ?? 0) : 0
    property int cfg_particleSimulation: wallpaperConfiguration ? (wallpaperConfiguration.particleSimulation ?? 0) : 0
    property bool cfg_pauseWhenHidden: wallpaperConfiguration ? (wallpaperConfiguration.pauseWhenHidden ?? true) : true
    property bool cfg_pauseWhenCovered: wallpaperConfiguration ? (wallpaperConfiguration.pauseWhenCovered ?? true) : true
    property bool cfg_renderingPaused: wallpaperConfiguration ? (wallpaperConfiguration.renderingPaused ?? false) : false

    Native.SystemUsage {
        id: usage
        visible: false
        width: 0
        height: 0
    }

    ScrollView {
        id: scrollView
        anchors.fill: root
        clip: true
        focus: true
        activeFocusOnTab: true
        contentWidth: availableWidth
        LayoutMirroring.enabled: false
        // Qt Quick Controls normally reparents an attached ScrollBar to the
        // ScrollView's internal Flickable.  That internal item is smaller than
        // the configuration window when Plasma shows the monitor preview.
        // Reparent the attached bar explicitly to the page root, as done by
        // the Qt/Kirigami ScrollView templates, and size it in root coordinates.
        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            parent: root
            policy: ScrollBar.AsNeeded
            x: root.width - width
            y: 0
            height: root.height
            width: Math.max(18, Math.min(28, root.width * 0.04))
            z: 100
        }
        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOff }

        ColumnLayout {
            id: content
            width: scrollView.availableWidth
            implicitHeight: childrenRect.height
            height: implicitHeight
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            text: i18n("XMB Interactive Flow")
            level: 2
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing
            Label {
                text: i18n("CPU: %1%", usage.cpuPercent.toFixed(1))
                font.bold: true
            }
            Label {
                text: i18n("GPU: %1%", usage.gpuPercent.toFixed(1))
                font.bold: true
            }
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            ComboBox {
                Kirigami.FormData.label: i18n("Pointer following:")
                model: [i18n("Disabled"), i18n("Interaction only"), i18n("Global flow + interaction")]
                currentIndex: root.cfg_pointerFollowMode
                onActivated: index => {
                    root.cfg_pointerFollowMode = index
                    root.configurationChanged()
                }
            }
            ComboBox {
                Kirigami.FormData.label: i18n("Particle profile:")
                model: [i18n("Small"), i18n("Large"), i18n("Mixed")]
                currentIndex: root.cfg_particleStyle
                onActivated: { root.cfg_particleStyle = index; root.configurationChanged() }
            }
            ComboBox {
                Kirigami.FormData.label: i18n("Particle simulation:")
                model: [i18n("Pseudo-2D"), i18n("Volumetric 3D")]
                currentIndex: root.cfg_particleSimulation
                onActivated: { root.cfg_particleSimulation = index; root.configurationChanged() }
            }
            ComboBox {
                Kirigami.FormData.label: i18n("Rendering structure:")
                model: [i18n("Original"), i18n("Altered")]
                currentIndex: root.cfg_renderStyle
                onActivated: { root.cfg_renderStyle = index; root.configurationChanged() }
            }

            ComboBox {
                Kirigami.FormData.label: i18n("Interaction X/Y signs:")
                model: ["+X / +Y", "−X / +Y", "+X / −Y", "−X / −Y"]
                currentIndex: root.cfg_interactionDirection
                onActivated: index => {
                    root.cfg_interactionDirection = index
                    root.configurationChanged()
                }
            }

            ComboBox {
                Kirigami.FormData.label: i18n("Spline calculation:")
                model: [i18n("CPU"), i18n("GPU (automatic CPU fallback)")]
                currentIndex: root.cfg_splineBackend
                onActivated: index => {
                    root.cfg_splineBackend = index
                    root.configurationChanged()
                }
            }

            ComboBox {
                Kirigami.FormData.label: i18n("Mesh quality:")
                model: [i18n("Soft (96)"), i18n("Crisp (140)"), i18n("Velvet (180)"), i18n("PS3 nostalgia (220)"), i18n("Creamy (320)"), i18n("Definitely too much (440 × 320)"), i18n("The GPU is crying (560 × 384)"), i18n("Cosmically irresponsible (720 × 480)")]
                currentIndex: root.cfg_quality
                onActivated: index => {
                    root.cfg_quality = index
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Target FPS:")
                from: 15; to: 240
                value: root.cfg_targetFps
                editable: true
                onValueModified: {
                    root.cfg_targetFps = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Particles:")
                from: 100; to: 30000; stepSize: 100
                value: root.cfg_particleCount
                editable: true
                onValueModified: {
                    root.cfg_particleCount = value
                    root.configurationChanged()
                }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Wave speed:")
                Slider {
                    id: waveSlider
                    Layout.preferredWidth: 260
                    from: 0.05; to: 4.0; stepSize: 0.05
                    value: root.cfg_waveSpeed
                    onMoved: {
                        root.cfg_waveSpeed = value
                        root.configurationChanged()
                    }
                }
                Label { text: waveSlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Particle speed:")
                Slider {
                    id: particleSpeedSlider
                    Layout.preferredWidth: 260
                    from: 0.05; to: 4.0; stepSize: 0.05
                    value: root.cfg_particleSpeed
                    onMoved: {
                        root.cfg_particleSpeed = value
                        root.configurationChanged()
                    }
                }
                Label { text: particleSpeedSlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Mouse strength:")
                Slider {
                    id: mouseSlider
                    Layout.preferredWidth: 260
                    from: 0.0; to: 4.0; stepSize: 0.05
                    value: root.cfg_mouseStrength
                    onMoved: {
                        root.cfg_mouseStrength = value
                        root.configurationChanged()
                    }
                }
                Label { text: mouseSlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Interaction radius:")
                Slider {
                    Layout.preferredWidth: 260
                    from: 0.40; to: 2.50; stepSize: 0.05
                    value: root.cfg_interactionRadius
                    onMoved: {
                        root.cfg_interactionRadius = value
                        root.configurationChanged()
                    }
                }
                Label { text: root.cfg_interactionRadius.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Brightness:")
                Slider {
                    id: brightnessSlider
                    Layout.preferredWidth: 260
                    from: 0.2; to: 1.8; stepSize: 0.05
                    value: root.cfg_brightness
                    onMoved: {
                        root.cfg_brightness = value
                        root.configurationChanged()
                    }
                }
                Label { text: brightnessSlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Base zoom:")
                Slider {
                    id: zoomSlider
                    Layout.preferredWidth: 260
                    from: 0.40; to: 3.50; stepSize: 0.05
                    value: root.cfg_zoom
                    onMoved: {
                        root.cfg_zoom = value
                        root.configurationChanged()
                    }
                }
                Label { text: zoomSlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Wheel sensitivity:")
                Slider {
                    id: zoomSensitivitySlider
                    Layout.preferredWidth: 260
                    from: 0.20; to: 2.00; stepSize: 0.05
                    value: root.cfg_zoomSensitivity
                    onMoved: {
                        root.cfg_zoomSensitivity = value
                        root.configurationChanged()
                    }
                }
                Label { text: zoomSensitivitySlider.value.toFixed(2) + "×" }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Flow group separation:")
                Slider {
                    Layout.preferredWidth: 260
                    from: 0.45; to: 1.60; stepSize: 0.05
                    value: root.cfg_flowGroupOffset
                    onMoved: { root.cfg_flowGroupOffset = value; root.configurationChanged() }
                }
                Label { text: root.cfg_flowGroupOffset.toFixed(2) }
            }

            RowLayout {
                Kirigami.FormData.label: i18n("Internal flow separation:")
                Slider {
                    Layout.preferredWidth: 260
                    from: 0.10; to: 1.20; stepSize: 0.05
                    value: root.cfg_flowSpacing
                    onMoved: { root.cfg_flowSpacing = value; root.configurationChanged() }
                }
                Label { text: root.cfg_flowSpacing.toFixed(2) }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Upper flows:")
                from: 0; to: 4
                value: root.cfg_upperFlowCount
                editable: true
                onValueModified: {
                    root.cfg_upperFlowCount = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Center flows:")
                from: 0; to: 4
                value: root.cfg_centerFlowCount
                editable: true
                onValueModified: {
                    root.cfg_centerFlowCount = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: i18n("Lower flows:")
                from: 0; to: 4
                value: root.cfg_lowerFlowCount
                editable: true
                onValueModified: {
                    root.cfg_lowerFlowCount = value
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: i18n("Multiscreen:")
                text: i18n("Use one virtual surface across all monitors")
                checked: root.cfg_linkAcrossScreens
                onToggled: {
                    root.cfg_linkAcrossScreens = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: i18n("Horizontal overscan:")
                text: i18n("Extend waves and particles beyond the desktop edges")
                checked: root.cfg_horizontalOverscan
                onToggled: {
                    root.cfg_horizontalOverscan = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: i18n("Interaction:")
                text: i18n("Mouse hover + click + wheel zoom; Shift+wheel moves the view without changing zoom")
                checked: root.cfg_interactionEnabled
                onToggled: {
                    root.cfg_interactionEnabled = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: i18n("Energy saving:")
                text: i18n("Enable automatic pausing when the wallpaper is hidden or covered")
                checked: root.cfg_pauseWhenHidden
                onToggled: {
                    root.cfg_pauseWhenHidden = checked
                    root.configurationChanged()
                }
            }
            CheckBox {
                Kirigami.FormData.label: i18n("Covered outputs:")
                text: i18n("Pause outputs fully covered by applications")
                checked: root.cfg_pauseWhenCovered
                enabled: root.cfg_pauseWhenHidden
                onToggled: {
                    root.cfg_pauseWhenCovered = checked
                    root.configurationChanged()
                }
            }
            CheckBox {
                Kirigami.FormData.label: i18n("Rendering:")
                text: i18n("Completely pause the wallpaper and GPU rendering (disable to resume)")
                checked: root.cfg_renderingPaused
                onToggled: {
                    root.cfg_renderingPaused = checked
                    root.configurationChanged()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.7
            text: i18n("The Ultra preset uses the v24 220×220 mesh with 32-bit indices. Left-click gradually attenuates the deformation; after release, the effect returns progressively. Particles follow the active flows. The wheel performs smooth zoom around the pointer up to 3.50×. Upper, center and lower flows are independent; with Multiscreen enabled, waves, perspective, zoom and particles belong to one global space cropped for each monitor.")
        }
    }
    }
}
