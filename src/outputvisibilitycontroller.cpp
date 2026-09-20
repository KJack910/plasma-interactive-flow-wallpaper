#include "outputvisibilitycontroller.h"

OutputVisibilityController *OutputVisibilityController::instance()
{
    static OutputVisibilityController controller;
    return &controller;
}

bool OutputVisibilityController::isFrozen(const QString &outputName) const
{
    return m_frozenOutputs.value(outputName, false);
}

void OutputVisibilityController::setOutputFrozen(const QString &outputName, bool frozen)
{
    if (outputName.isEmpty())
        return;

    QTimer *resumeTimer = m_resumeTimers.value(outputName, nullptr);
    if (frozen)
    {
        if (resumeTimer)
            resumeTimer->stop();
        applyOutputFrozen(outputName, true);
        return;
    }

    if (!m_frozenOutputs.value(outputName, false))
    {
        applyOutputFrozen(outputName, false);
        return;
    }

    // KWin can briefly report uncovered during maximize/stacking changes.
    // Freeze immediately, but resume only when the state stays uncovered.
    if (!resumeTimer)
    {
        resumeTimer = new QTimer(this);
        resumeTimer->setSingleShot(true);
        resumeTimer->setTimerType(Qt::PreciseTimer);
        connect(resumeTimer, &QTimer::timeout, this, [this, outputName]() {
            applyOutputFrozen(outputName, false);
        });
        m_resumeTimers.insert(outputName, resumeTimer);
    }
    if (!resumeTimer->isActive())
        resumeTimer->start(10);
}

void OutputVisibilityController::applyOutputFrozen(const QString &outputName, bool frozen)
{
    if (m_frozenOutputs.contains(outputName) && m_frozenOutputs.value(outputName) == frozen)
        return;
    m_frozenOutputs.insert(outputName, frozen);
    emit outputFrozenChanged(outputName, frozen);
}
