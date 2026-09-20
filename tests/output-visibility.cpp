#include "outputvisibilitycontroller.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>

#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    OutputVisibilityController controller;
    using Change = std::pair<QString, bool>;
    std::vector<Change> changes;
    QObject::connect(&controller, &OutputVisibilityController::outputFrozenChanged,
                     &app, [&](const QString &output, bool frozen) {
        changes.emplace_back(output, frozen);
    });

    controller.setOutputFrozen(QString(), true);
    controller.setOutputFrozen(QString(), false);
    assert(changes.empty());
    assert(controller.findChildren<QTimer *>().isEmpty());
    assert(!controller.isFrozen("DP-1"));

    controller.setOutputFrozen("DP-1", false);
    controller.setOutputFrozen("DP-1", false);
    assert((changes == std::vector<Change>{{"DP-1", false}}));
    assert(controller.findChildren<QTimer *>().isEmpty());
    changes.clear();

    controller.setOutputFrozen("DP-1", true);
    controller.setOutputFrozen("DP-1", true);
    assert(controller.isFrozen("DP-1"));
    assert((changes == std::vector<Change>{{"DP-1", true}}));
    changes.clear();

    controller.setOutputFrozen("DP-1", false);
    assert(controller.isFrozen("DP-1"));
    assert(changes.empty());
    const auto timers = controller.findChildren<QTimer *>();
    assert(timers.size() == 1);
    QTimer *firstTimer = timers.first();
    assert(firstTimer->isSingleShot());
    assert(firstTimer->timerType() == Qt::PreciseTimer);
    assert(firstTimer->interval() == 10);
    assert(firstTimer->isActive());
    const int timerId = firstTimer->timerId();
    for (int i = 0; i < 100; ++i)
    {
        controller.setOutputFrozen("DP-1", false);
        assert(firstTimer->timerId() == timerId);
    }
    assert(changes.empty());

    controller.setOutputFrozen("DP-1", true);
    assert(!firstTimer->isActive());
    assert(controller.isFrozen("DP-1"));
    assert(changes.empty());

    controller.setOutputFrozen("DP-2", true);
    controller.setOutputFrozen("DP-2", false);
    controller.setOutputFrozen("DP-2", true);
    assert(controller.isFrozen("DP-2"));
    changes.clear();

    QEventLoop loop;
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    bool resumed = false;
    QObject::connect(&controller, &OutputVisibilityController::outputFrozenChanged,
                     &loop, [&](const QString &output, bool frozen) {
        if (output == "DP-1" && !frozen)
        {
            resumed = true;
            loop.quit();
        }
    });
    QElapsedTimer elapsed;
    elapsed.start();
    controller.setOutputFrozen("DP-1", false);
    assert(firstTimer->isActive());
    assert(controller.isFrozen("DP-1"));
    watchdog.start(5000);
    loop.exec();
    assert(resumed);
    assert(elapsed.elapsed() >= 10);
    assert(!firstTimer->isActive());
    assert(!controller.isFrozen("DP-1"));
    assert(controller.isFrozen("DP-2"));
    assert((changes == std::vector<Change>{{"DP-1", false}}));
    changes.clear();

    controller.setOutputFrozen("DP-1", false);
    assert(!firstTimer->isActive());
    assert(changes.empty());
    controller.setOutputFrozen("DP-1", true);
    assert(controller.isFrozen("DP-1"));
    assert((changes == std::vector<Change>{{"DP-1", true}}));

    std::cout << "Output visibility: immediate freeze, stable 10 ms resume, duplicates and outputs OK\n";
}
