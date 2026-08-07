#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(u"TunaOS Installer"_s);
    app.setApplicationVersion(u"0.1.0"_s);
    app.setOrganizationName(u"tuna-os"_s);
    app.setDesktopFileName(u"org.tunaos.InstallerKde"_s);

    // Plasma's own Qt Quick Controls style. The Widgets frontend this replaces
    // forced Fusion, which never matched the desktop it shipped on.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(u"org.kde.desktop"_s);

    QQmlApplicationEngine engine;
    engine.loadFromModule("org.tunaos.installer"_L1, "Main"_L1);
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
