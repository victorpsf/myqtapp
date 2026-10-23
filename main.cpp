#include <QApplication>

#include "MainWindow.h"
#include "config/AppStyle.h"
#include "config/Env.h"
#include "security/SecuritySession.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    // No GNOME o Qt carrega o tema "gtk3", que aborta quando o schema do
    // gnome-settings-daemon não tem a chave "antialiasing" (GLib-GIO-ERROR).
    // O tema "gnome" é nativo do Qt e não depende do GTK.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME"))
        qputenv("QT_QPA_PLATFORMTHEME", "gnome");
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName("myqtapp");
    QApplication::setDesktopFileName("myqtapp"); // ícone correto no Wayland/AppImage

    Env::load();
    app.setStyleSheet(AppStyle::styleSheet());

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     []() { SecuritySession::instance().clear(); });

    MainWindow window;
    window.show();

    return app.exec();
}
