#include "xmbnativeplugin.h"
#include "xmbrendereritem.h"
#include "systemusageitem.h"
#include "outputvisibilitycontroller.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <qqml.h>
#include <QString>
#include <QStandardPaths>

namespace {
void resyncKWinScript()
{
    const QString script = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral("kwin/scripts/xmbfullscreenbridge/contents/code/main.js"));
    if (script.isEmpty())
        return;

    const QDBusConnection bus = QDBusConnection::sessionBus();
    auto message = [](const QString &method, const QVariantList &arguments = {}) {
        QDBusMessage request = QDBusMessage::createMethodCall(
            QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
            QStringLiteral("org.kde.kwin.Scripting"), method);
        request.setArguments(arguments);
        return request;
    };
    auto *unload = new QDBusPendingCallWatcher(bus.asyncCall(
        message(QStringLiteral("unloadScript"), {QStringLiteral("xmbfullscreenbridge")})),
        OutputVisibilityController::instance());
    QObject::connect(unload, &QDBusPendingCallWatcher::finished,
                     OutputVisibilityController::instance(), [bus, script, message](QDBusPendingCallWatcher *finished) {
        finished->deleteLater();
        auto *load = new QDBusPendingCallWatcher(bus.asyncCall(
            message(QStringLiteral("loadScript"), {script, QStringLiteral("xmbfullscreenbridge")})),
            OutputVisibilityController::instance());
        QObject::connect(load, &QDBusPendingCallWatcher::finished,
                         OutputVisibilityController::instance(), [bus, message](QDBusPendingCallWatcher *loaded) {
            loaded->deleteLater();
            bus.asyncCall(message(QStringLiteral("start")));
        });
    });
}

void registerFullscreenBridge()
{
    static bool registered = false;
    if (registered)
        return;

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.xmbflow.interactive")))
        return;
    if (!bus.registerObject(QStringLiteral("/XmbFlow"), OutputVisibilityController::instance(),
                            QDBusConnection::ExportAllSlots))
        return;
    registered = true;
    // KWin may outlive plasmashell. Reloading its observer forces a complete
    // per-output snapshot into the newly registered D-Bus controller.
    resyncKWinScript();
}
}

void XmbNativePlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QString::fromLatin1(uri) == QStringLiteral("org.xmbflow.native"));
    registerFullscreenBridge();
    qmlRegisterType<XmbRendererItem>(uri, 1, 0, "XmbRenderer");
    qmlRegisterType<SystemUsageItem>(uri, 1, 0, "SystemUsage");
}
