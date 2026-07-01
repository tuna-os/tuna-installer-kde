#include <QApplication>
#include "wizard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("TunaOS Installer"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("tuna-os"));

    // Apply a Fusion-style theme for consistent look across desktop environments
    app.setStyle(QStringLiteral("Fusion"));

    Wizard wizard;
    wizard.show();

    return app.exec();
}
