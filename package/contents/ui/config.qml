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
            text: "XMB Interactive Flow"
            level: 2
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing
            Label {
                text: "CPU: " + usage.cpuPercent.toFixed(1) + "%"
                font.bold: true
            }
            Label {
                text: "GPU: " + usage.gpuPercent.toFixed(1) + "%"
                font.bold: true
            }
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            ComboBox {
                Kirigami.FormData.label: "Segui puntatore:"
                model: ["Disattivato", "Solo interazione", "Flusso globale + interazione"]
                currentIndex: root.cfg_pointerFollowMode
                onActivated: index => {
                    root.cfg_pointerFollowMode = index
                    root.configurationChanged()
                }
            }
            ComboBox {
                Kirigami.FormData.label: "Profilo particellari:"
                model: ["Piccoli", "Grandi", "Misti"]
                currentIndex: root.cfg_particleStyle
                onActivated: { root.cfg_particleStyle = index; root.configurationChanged() }
            }
            ComboBox {
                Kirigami.FormData.label: "Simulazione particellari:"
                model: ["Pseudo-2D", "3D volumetrico"]
                currentIndex: root.cfg_particleSimulation
                onActivated: { root.cfg_particleSimulation = index; root.configurationChanged() }
            }
            ComboBox {
                Kirigami.FormData.label: "Struttura rendering:"
                model: ["Originale", "Alterata"]
                currentIndex: root.cfg_renderStyle
                onActivated: { root.cfg_renderStyle = index; root.configurationChanged() }
            }

            ComboBox {
                Kirigami.FormData.label: "Segni interazione X/Y:"
                model: ["+X / +Y", "−X / +Y", "+X / −Y", "−X / −Y"]
                currentIndex: root.cfg_interactionDirection
                onActivated: index => {
                    root.cfg_interactionDirection = index
                    root.configurationChanged()
                }
            }

            ComboBox {
                Kirigami.FormData.label: "Calcolo spline:"
                model: ["CPU", "GPU (fallback CPU automatico)"]
                currentIndex: root.cfg_splineBackend
                onActivated: index => {
                    root.cfg_splineBackend = index
                    root.configurationChanged()
                }
            }

            ComboBox {
                Kirigami.FormData.label: "Qualità mesh:"
                model: ["Soffice (96)", "Croccante (140)", "Velluto (180)", "Nostalgia PS3 (220)", "Cremoso (320)", "Definitivamente Troppo (440 × 320)", "Il GPU Piange (560 × 384)", "Cosmicamente Irresponsabile (720 × 480)"]
                currentIndex: root.cfg_quality
                onActivated: index => {
                    root.cfg_quality = index
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: "FPS target:"
                from: 15; to: 240
                value: root.cfg_targetFps
                editable: true
                onValueModified: {
                    root.cfg_targetFps = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: "Particelle:"
                from: 100; to: 30000; stepSize: 100
                value: root.cfg_particleCount
                editable: true
                onValueModified: {
                    root.cfg_particleCount = value
                    root.configurationChanged()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Velocità onda:"
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
                Kirigami.FormData.label: "Velocità particelle:"
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
                Kirigami.FormData.label: "Forza mouse:"
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
                Kirigami.FormData.label: "Raggio interazione:"
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
                Kirigami.FormData.label: "Luminosità:"
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
                Kirigami.FormData.label: "Zoom base:"
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
                Kirigami.FormData.label: "Sensibilità rotella:"
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
                Kirigami.FormData.label: "Distanza macroclassi:"
                Slider {
                    Layout.preferredWidth: 260
                    from: 0.45; to: 1.60; stepSize: 0.05
                    value: root.cfg_flowGroupOffset
                    onMoved: { root.cfg_flowGroupOffset = value; root.configurationChanged() }
                }
                Label { text: root.cfg_flowGroupOffset.toFixed(2) }
            }

            RowLayout {
                Kirigami.FormData.label: "Distanza flussi interni:"
                Slider {
                    Layout.preferredWidth: 260
                    from: 0.10; to: 1.20; stepSize: 0.05
                    value: root.cfg_flowSpacing
                    onMoved: { root.cfg_flowSpacing = value; root.configurationChanged() }
                }
                Label { text: root.cfg_flowSpacing.toFixed(2) }
            }

            SpinBox {
                Kirigami.FormData.label: "Flussi superiori:"
                from: 0; to: 4
                value: root.cfg_upperFlowCount
                editable: true
                onValueModified: {
                    root.cfg_upperFlowCount = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: "Flussi centrali:"
                from: 0; to: 4
                value: root.cfg_centerFlowCount
                editable: true
                onValueModified: {
                    root.cfg_centerFlowCount = value
                    root.configurationChanged()
                }
            }

            SpinBox {
                Kirigami.FormData.label: "Flussi inferiori:"
                from: 0; to: 4
                value: root.cfg_lowerFlowCount
                editable: true
                onValueModified: {
                    root.cfg_lowerFlowCount = value
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: "Multischermo:"
                text: "Usa un'unica superficie virtuale su tutti i monitor"
                checked: root.cfg_linkAcrossScreens
                onToggled: {
                    root.cfg_linkAcrossScreens = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: "Estensione orizzontale:"
                text: "Estendi onde e particelle oltre i bordi del desktop"
                checked: root.cfg_horizontalOverscan
                onToggled: {
                    root.cfg_horizontalOverscan = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: "Diagnostica:"
                text: "Mostra la griglia globale 2D e isola il multischermo"
                checked: root.cfg_multiscreenDebug
                enabled: root.cfg_linkAcrossScreens
                onToggled: {
                    root.cfg_multiscreenDebug = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: "Interazione:"
                text: "Mouse hover + click + rotella zoom; Shift+rotella sposta la vista senza cambiare zoom"
                checked: root.cfg_interactionEnabled
                onToggled: {
                    root.cfg_interactionEnabled = checked
                    root.configurationChanged()
                }
            }

            CheckBox {
                Kirigami.FormData.label: "Risparmio energetico:"
                text: "Pausa quando il wallpaper non è visibile"
                checked: root.cfg_pauseWhenHidden
                onToggled: {
                    root.cfg_pauseWhenHidden = checked
                    root.configurationChanged()
                }
            }
            CheckBox {
                Kirigami.FormData.label: "Schermi coperti:"
                text: "Pausa e riprendi automaticamente ogni monitor"
                checked: root.cfg_pauseWhenCovered
                onToggled: {
                    root.cfg_pauseWhenCovered = checked
                    root.configurationChanged()
                }
            }
            CheckBox {
                Kirigami.FormData.label: "Rendering:"
                text: "Blocca completamente il wallpaper e il rendering GPU (disattiva per riprendere)"
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
            text: "Il preset Ultra usa la mesh 220×220 della v24 con indici a 32 bit. " +
                  "Il click sinistro attenua gradualmente la deformazione; al rilascio l'effetto ritorna progressivamente. " +
                  "Le particelle ora seguono i flussi attivi. La rotella esegue uno zoom fluido attorno al puntatore fino a 3.50×. " +
                  "I flussi superiori, centrali e inferiori sono indipendenti; con Multischermo attivo onda, prospettiva, " +
                  "zoom e particelle appartengono a un solo spazio globale, ritagliato per ciascun monitor."
        }
    }
    }
}
