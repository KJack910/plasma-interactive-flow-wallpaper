#pragma once

#include <QElapsedTimer>
#include <QMetaObject>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QQuickFramebufferObject>
#include <QSizeF>
#include <QTimer>
#include <QVector2D>

class XmbRendererItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(int targetFps READ targetFps WRITE setTargetFps NOTIFY targetFpsChanged)
    Q_PROPERTY(int particleCount READ particleCount WRITE setParticleCount NOTIFY particleCountChanged)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed NOTIFY waveSpeedChanged)
    Q_PROPERTY(qreal particleSpeed READ particleSpeed WRITE setParticleSpeed NOTIFY particleSpeedChanged)
    Q_PROPERTY(qreal mouseStrength READ mouseStrength WRITE setMouseStrength NOTIFY mouseStrengthChanged)
    Q_PROPERTY(qreal interactionRadius READ interactionRadius WRITE setInteractionRadius NOTIFY interactionRadiusChanged)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal zoomSensitivity READ zoomSensitivity WRITE setZoomSensitivity NOTIFY zoomSensitivityChanged)
    Q_PROPERTY(qreal flowGroupOffset READ flowGroupOffset WRITE setFlowGroupOffset NOTIFY flowGroupOffsetChanged)
    Q_PROPERTY(qreal flowSpacing READ flowSpacing WRITE setFlowSpacing NOTIFY flowSpacingChanged)
    Q_PROPERTY(int upperFlowCount READ upperFlowCount WRITE setUpperFlowCount NOTIFY upperFlowCountChanged)
    Q_PROPERTY(int centerFlowCount READ centerFlowCount WRITE setCenterFlowCount NOTIFY centerFlowCountChanged)
    Q_PROPERTY(int lowerFlowCount READ lowerFlowCount WRITE setLowerFlowCount NOTIFY lowerFlowCountChanged)
    Q_PROPERTY(bool linkAcrossScreens READ linkAcrossScreens WRITE setLinkAcrossScreens NOTIFY linkAcrossScreensChanged)
    Q_PROPERTY(bool horizontalOverscan READ horizontalOverscan WRITE setHorizontalOverscan NOTIFY horizontalOverscanChanged)
    Q_PROPERTY(bool multiscreenDebug READ multiscreenDebug WRITE setMultiscreenDebug NOTIFY multiscreenDebugChanged)
    Q_PROPERTY(bool interactionEnabled READ interactionEnabled WRITE setInteractionEnabled NOTIFY interactionEnabledChanged)
    Q_PROPERTY(int pointerFollowMode READ pointerFollowMode WRITE setPointerFollowMode NOTIFY pointerFollowModeChanged)
    Q_PROPERTY(int interactionDirection READ interactionDirection WRITE setInteractionDirection NOTIFY interactionDirectionChanged)
    Q_PROPERTY(int renderStyle READ renderStyle WRITE setRenderStyle NOTIFY renderStyleChanged)
    Q_PROPERTY(int splineBackend READ splineBackend WRITE setSplineBackend NOTIFY splineBackendChanged)
    Q_PROPERTY(int particleStyle READ particleStyle WRITE setParticleStyle NOTIFY particleStyleChanged)
    Q_PROPERTY(int particleSimulation READ particleSimulation WRITE setParticleSimulation NOTIFY particleSimulationChanged)
    Q_PROPERTY(bool pauseWhenHidden READ pauseWhenHidden WRITE setPauseWhenHidden NOTIFY pauseWhenHiddenChanged)
    Q_PROPERTY(bool pauseWhenCovered READ pauseWhenCovered WRITE setPauseWhenCovered NOTIFY pauseWhenCoveredChanged)
    Q_PROPERTY(bool renderingPaused READ renderingPaused WRITE setRenderingPaused NOTIFY renderingPausedChanged)
    Q_PROPERTY(bool frozen READ frozen WRITE setFrozen NOTIFY frozenChanged)
    Q_PROPERTY(qreal interactionStrength READ interactionStrength NOTIFY interactionStrengthChanged)

public:
    explicit XmbRendererItem(QQuickItem *parent = nullptr);
    ~XmbRendererItem() override;

    Renderer *createRenderer() const override;

    int quality() const { return m_quality; }
    int targetFps() const { return m_targetFps; }
    int particleCount() const { return m_particleCount; }
    qreal waveSpeed() const { return m_waveSpeed; }
    qreal particleSpeed() const { return m_particleSpeed; }
    qreal mouseStrength() const { return m_mouseStrength; }
    qreal interactionRadius() const { return m_interactionRadius; }
    qreal brightness() const { return m_brightness; }
    qreal zoom() const { return m_targetZoom; }
    qreal zoomSensitivity() const { return m_zoomSensitivity; }
    qreal flowGroupOffset() const { return m_flowGroupOffset; }
    qreal flowSpacing() const { return m_flowSpacing; }
    int upperFlowCount() const { return m_upperFlowCount; }
    int centerFlowCount() const { return m_centerFlowCount; }
    int lowerFlowCount() const { return m_lowerFlowCount; }
    bool linkAcrossScreens() const { return m_linkAcrossScreens; }
    bool horizontalOverscan() const { return m_horizontalOverscan; }
    bool multiscreenDebug() const { return m_multiscreenDebug; }
    bool interactionEnabled() const { return m_interactionEnabled; }
    int pointerFollowMode() const { return m_pointerFollowMode; }
    int interactionDirection() const { return m_interactionDirection; }
    int renderStyle() const { return m_renderStyle; }
    int splineBackend() const { return m_splineBackend; }
    int particleStyle() const { return m_particleStyle; }
    int particleSimulation() const { return m_particleSimulation; }
    bool pauseWhenHidden() const { return m_pauseWhenHidden; }
    bool pauseWhenCovered() const { return m_pauseWhenCovered; }
    bool renderingPaused() const { return m_renderingPaused; }
    bool frozen() const { return m_frozen; }
    qreal interactionStrength() const { return m_interactionStrength; }

    void setQuality(int value);
    void setTargetFps(int value);
    void setParticleCount(int value);
    void setWaveSpeed(qreal value);
    void setParticleSpeed(qreal value);
    void setMouseStrength(qreal value);
    void setInteractionRadius(qreal value);
    void setBrightness(qreal value);
    void setZoom(qreal value);
    void setZoomSensitivity(qreal value);
    void setFlowGroupOffset(qreal value);
    void setFlowSpacing(qreal value);
    void setUpperFlowCount(int value);
    void setCenterFlowCount(int value);
    void setLowerFlowCount(int value);
    void setLinkAcrossScreens(bool value);
    void setHorizontalOverscan(bool value);
    void setMultiscreenDebug(bool value);
    void setInteractionEnabled(bool value);
    void setPointerFollowMode(int value);
    void setInteractionDirection(int value);
    void setRenderStyle(int value);
    void setSplineBackend(int value);
    void setParticleStyle(int value);
    void setParticleSimulation(int value);
    void setPauseWhenHidden(bool value);
    void setPauseWhenCovered(bool value);
    void setRenderingPaused(bool value);
    void setFrozen(bool value);

    Q_INVOKABLE void pointerMoved(qreal x, qreal y, bool present = true);
    Q_INVOKABLE void pointerEntered(qreal x, qreal y);
    Q_INVOKABLE void pointerLeft();
    Q_INVOKABLE void leftPressed(qreal x, qreal y);
    Q_INVOKABLE void leftReleased(qreal x, qreal y);
    Q_INVOKABLE void resetAnimation();

signals:
    void qualityChanged();
    void targetFpsChanged();
    void particleCountChanged();
    void waveSpeedChanged();
    void particleSpeedChanged();
    void mouseStrengthChanged();
    void interactionRadiusChanged();
    void brightnessChanged();
    void zoomChanged();
    void zoomSensitivityChanged();
    void flowGroupOffsetChanged();
    void flowSpacingChanged();
    void upperFlowCountChanged();
    void centerFlowCountChanged();
    void lowerFlowCountChanged();
    void linkAcrossScreensChanged();
    void horizontalOverscanChanged();
    void multiscreenDebugChanged();
    void interactionEnabledChanged();
    void pointerFollowModeChanged();
    void interactionDirectionChanged();
    void renderStyleChanged();
    void splineBackendChanged();
    void particleStyleChanged();
    void particleSimulationChanged();
    void pauseWhenHiddenChanged();
    void pauseWhenCoveredChanged();
    void renderingPausedChanged();
    void frozenChanged();
    void interactionStrengthChanged();

private slots:
    void tick();
    void updateTimerInterval();
    void pollSystemPointer();
    void updateDesktopGeometry();
    void updateOutputVisibility();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    friend class XmbRenderer;

    int meshResolution() const;

    int m_quality = 3;
    int m_targetFps = 120;
    int m_particleCount = 12000;
    qreal m_waveSpeed = 1.0;
    qreal m_particleSpeed = 1.0;
    qreal m_mouseStrength = 1.0;
    qreal m_interactionRadius = 1.0;
    qreal m_brightness = 1.0;
    qreal m_zoom = 1.0;
    qreal m_targetZoom = 1.0;
    qreal m_zoomSensitivity = 0.60;
    qreal m_flowGroupOffset = 1.28;
    qreal m_flowSpacing = 0.68;
    int m_upperFlowCount = 1;
    int m_centerFlowCount = 0;
    int m_lowerFlowCount = 1;
    bool m_linkAcrossScreens = false;
    bool m_horizontalOverscan = true;
    bool m_multiscreenDebug = false;
    bool m_interactionEnabled = true;
    int m_pointerFollowMode = 1;
    int m_interactionDirection = 0;
    int m_renderStyle = 0;
    int m_splineBackend = 1;
    int m_particleStyle = 0;
    int m_particleSimulation = 0;
    bool m_pauseWhenHidden = true;
    bool m_pauseWhenCovered = true;
    bool m_renderingPaused = false;
    bool m_frozen = false;

    QPointF m_pointer;
    QPointF m_prevPointer;
    QPointF m_zoomOffset;
    QPointF m_targetZoomOffset;
    QRectF m_zoomViewport;
    bool m_zoomChangeFromWheel = false;
    bool m_zoomReturningHome = false;
    bool m_baseZoomNeedsCentering = false;
    QVector2D m_flow;
    bool m_pointerPresent = false;
    bool m_leftHeld = false;
    bool m_systemPointerInitialized = false;
    qreal m_interactionStrength = 0.0;
    qreal m_waveTime = 0.0;
    qreal m_particleTime = 137.0;
    // The linked renderer treats all virtual siblings as one logical-pixel
    // surface. Each wallpaper instance renders that same surface and clips it
    // to m_viewportOriginPx/m_viewportSizePx.
    QPointF m_viewportOriginPx;
    QSizeF m_viewportSizePx = QSizeF(1.0, 1.0);
    QSizeF m_virtualSizePx = QSizeF(1.0, 1.0);
    QSizeF m_referenceSizePx = QSizeF(1.0, 1.0);

    QTimer m_frameTimer;
    QMetaObject::Connection m_screenChangedConnection;
    QElapsedTimer m_clock;
    qint64 m_lastNs = 0;
};
