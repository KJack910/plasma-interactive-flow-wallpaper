#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QQuickItem>
#include <QTimer>
#include <QString>

class SystemUsageItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY cpuPercentChanged)
    Q_PROPERTY(double gpuPercent READ gpuPercent NOTIFY gpuPercentChanged)

public:
    explicit SystemUsageItem(QQuickItem *parent = nullptr);
    ~SystemUsageItem() override;

    double cpuPercent() const { return m_cpuPercent; }
    double gpuPercent() const { return m_gpuPercent; }

    Q_INVOKABLE void refresh();

signals:
    void cpuPercentChanged();
    void gpuPercentChanged();

private:
    void readCpuUsage();
    void readGpuUsage();
    bool publishGpuPercent(double value);

    double m_cpuPercent = 0.0;
    double m_gpuPercent = 0.0;
    QTimer m_timer;
    QElapsedTimer m_gpuClock;
    QHash<QString, qint64> m_previousEngineBusy;
    qint64 m_previousEngineSampleNs = 0;

    // Previous values for CPU calculation
    qlonglong m_prevTotal = 0;
    qlonglong m_prevIdle = 0;
};
