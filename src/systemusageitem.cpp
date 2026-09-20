#include "systemusageitem.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>

SystemUsageItem::SystemUsageItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_timer.setInterval(300);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_gpuClock.start();
    connect(&m_timer, &QTimer::timeout, this, &SystemUsageItem::refresh);
    m_timer.start();
    refresh();
}

SystemUsageItem::~SystemUsageItem() = default;

void SystemUsageItem::refresh()
{
    readCpuUsage();
    readGpuUsage();
}

void SystemUsageItem::readCpuUsage()
{
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    if (!line.startsWith("cpu ")) {
        return;
    }

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 8) {
        return;
    }

    // Skip the "cpu" label at index 0
    qlonglong user = parts[1].toLongLong();
    qlonglong nice = parts[2].toLongLong();
    qlonglong system = parts[3].toLongLong();
    qlonglong idle = parts[4].toLongLong();
    qlonglong iowait = parts[5].toLongLong();
    qlonglong irq = parts[6].toLongLong();
    qlonglong softirq = parts[7].toLongLong();
    qlonglong steal = (parts.size() > 8) ? parts[8].toLongLong() : 0;

    qlonglong total = user + nice + system + idle + iowait + irq + softirq + steal;
    qlonglong active = total - idle - iowait;

    double percent = 0.0;
    if (m_prevTotal > 0 && total > m_prevTotal) {
        qlonglong totalDiff = total - m_prevTotal;
        qlonglong activeDiff = active - (m_prevTotal - m_prevIdle); // approximate previous active
        // Recompute previous active properly
        qlonglong prevActive = m_prevTotal - m_prevIdle;
        activeDiff = active - prevActive;
        if (totalDiff > 0) {
            percent = (double(activeDiff) / double(totalDiff)) * 100.0;
        }
    }

    m_prevTotal = total;
    m_prevIdle = idle + iowait;

    percent = qBound(0.0, percent, 100.0);

    if (!qFuzzyCompare(percent, m_cpuPercent)) {
        m_cpuPercent = percent;
        emit cpuPercentChanged();
    }
}

bool SystemUsageItem::publishGpuPercent(double value)
{
    value = qBound(0.0, value, 100.0);
    if (qFuzzyCompare(value, m_gpuPercent)) {
        return true;
    }
    m_gpuPercent = value;
    emit gpuPercentChanged();
    return true;
}

void SystemUsageItem::readGpuUsage()
{
    // DRM sysfs direct utilization. This covers AMD gpu_busy_percent and
    // drivers exposing the same metric under gt_busy_percent.
    const QStringList cards = QDir(QStringLiteral("/sys/class/drm"))
        .entryList(QStringList() << QStringLiteral("card*"), QDir::Dirs | QDir::NoDotAndDotDot,
                   QDir::Name);
    bool directMetricFound = false;
    double directMetricMax = 0.0;
    for (const QString &card : cards) {
        const QString device = QStringLiteral("/sys/class/drm/%1/device").arg(card);
        const QStringList directMetrics = {
            device + QStringLiteral("/gpu_busy_percent"),
            device + QStringLiteral("/gt_busy_percent"),
            QStringLiteral("/sys/class/drm/%1/gpu_busy_percent").arg(card),
            QStringLiteral("/sys/class/drm/%1/gt_busy_percent").arg(card)
        };
        for (const QString &path : directMetrics) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            bool ok = false;
            const double value = QString::fromUtf8(file.readAll()).trimmed().toDouble(&ok);
            if (ok) {
                directMetricFound = true;
                directMetricMax = qMax(directMetricMax, value);
                break;
            }
        }
    }
    if (directMetricFound) {
        // With multiple adapters, show the busiest one instead of returning
        // the first idle card (common on multi-GPU systems).
        publishGpuPercent(directMetricMax);
        return;
    }

    // Generic DRM engine counters fallback, used by Intel and other drivers
    // that expose per-engine busy_time but no aggregate percentage.
    QHash<QString, qint64> current;
    for (const QString &card : cards) {
        const QString engineRoot = QStringLiteral("/sys/class/drm/%1/device/engine").arg(card);
        const QStringList engines = QDir(engineRoot).entryList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &engine : engines) {
            const QString path = engineRoot + QLatin1Char('/') + engine + QStringLiteral("/busy_time");
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }
            bool ok = false;
            const qint64 busy = QString::fromUtf8(file.readAll()).trimmed().toLongLong(&ok);
            if (ok) {
                current.insert(path, busy);
            }
        }
    }

    const qint64 nowNs = m_gpuClock.nsecsElapsed();
    if (!current.isEmpty() && m_previousEngineSampleNs > 0 && nowNs > m_previousEngineSampleNs) {
        qint64 busyDelta = 0;
        int comparableEngines = 0;
        for (auto it = current.cbegin(); it != current.cend(); ++it) {
            const auto previous = m_previousEngineBusy.constFind(it.key());
            if (previous != m_previousEngineBusy.cend() && it.value() >= previous.value()) {
                busyDelta += it.value() - previous.value();
                ++comparableEngines;
            }
        }
        if (comparableEngines > 0) {
            const double wallNs = double(nowNs - m_previousEngineSampleNs);
            const double utilization = (double(busyDelta) / wallNs)
                / double(comparableEngines) * 100.0;
            m_previousEngineBusy = current;
            m_previousEngineSampleNs = nowNs;
            publishGpuPercent(utilization);
            return;
        }
    }

    m_previousEngineBusy = current;
    m_previousEngineSampleNs = nowNs;
    if (current.isEmpty()) {
        publishGpuPercent(0.0);
    }
}

