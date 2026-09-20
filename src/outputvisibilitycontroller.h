#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

// KWin is the authoritative window/output observer on both Wayland and X11.
// QtDBus delivers its per-output state to this GUI-thread controller.
class OutputVisibilityController final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.xmbflow.Fullscreen")

public:
    static OutputVisibilityController *instance();
    bool isFrozen(const QString &outputName) const;

public slots:
    void setOutputFrozen(const QString &outputName, bool frozen);

signals:
    void outputFrozenChanged(const QString &outputName, bool frozen);

private:
    void applyOutputFrozen(const QString &outputName, bool frozen);
    QHash<QString, bool> m_frozenOutputs;
    QHash<QString, QTimer *> m_resumeTimers;
};
