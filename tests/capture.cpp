// Renders every wizard step to PNG for docs/gui-walkthrough.md and the README.
//
// Reuses the real QML module — the same Wizard.qml and the same six step
// modules the installer loads — so the images document the shipped UI rather
// than a replica. Only main.cpp is swapped.
//
// Qt's offscreen platform plugin plus the software scenegraph backend means no
// X server, no compositor and no GPU: this runs in a plain container. It does
// need the KF6 QML runtime (Kirigami, Kirigami Addons, qqc2-desktop-style) on
// the import path — see .github/workflows/screenshots.yml.
//
// Safe by construction: Wizard.goToStep() only moves the visible step and calls
// the target module's onPageActivated(), and no module's onPageActivated() does
// anything but re-read lsblk. The only caller of
// InstallerController.startInstall() is the Install button's onClicked, which
// nothing here presses. Worth stating because the sibling XFCE installer does
// start an install from its page-enter hook, so "drive the steps" is not
// automatically harmless across this family.

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QMap>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>
#include <QTimer>
#include <QVariant>

#include "installercontroller.h"

using namespace Qt::StringLiterals;

namespace {

// Pixel-audit thresholds. Calibrated against measured CI output, not guessed —
// every run prints the numbers these were derived from.
constexpr int MIN_COLOURS = 200;   // distinct colours in a real screen
constexpr double MAX_FLAT = 0.97;  // fraction of samples allowed to be one colour
constexpr double MIN_INK = 0.01;   // fraction of samples differing from the background

void settle(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

struct Finding {
    QString name;
    int colours = 0;
    double flat = 0.0;
    double ink = 0.0;
};

// Read the pixels back. A capture that only checks its PNGs exist will publish
// blank screens: the files are present, non-empty, and empty. That exact check
// passed in a sibling repo while shipping a blank page.
//
// "ink" is measured against the image's own dominant colour rather than a fixed
// luma, because a Plasma dark colour scheme inverts the sense of "dark pixel".
Finding audit(const QImage &image, const QString &name)
{
    QMap<QRgb, int> counts;
    int samples = 0;
    for (int y = 0; y < image.height(); y += 3) {
        for (int x = 0; x < image.width(); x += 3) {
            counts[image.pixel(x, y) & 0x00FFFFFF] += 1;
            samples += 1;
        }
    }

    QRgb background = 0;
    int largest = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > largest) {
            largest = it.value();
            background = it.key();
        }
    }

    // Anything far enough from the dominant colour counts as drawn content.
    int ink = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const int distance = qAbs(qRed(it.key()) - qRed(background))
                           + qAbs(qGreen(it.key()) - qGreen(background))
                           + qAbs(qBlue(it.key()) - qBlue(background));
        if (distance > 40)
            ink += it.value();
    }

    Finding f;
    f.name = name;
    f.colours = counts.size();
    f.flat = samples ? double(largest) / samples : 1.0;
    f.ink = samples ? double(ink) / samples : 0.0;
    return f;
}

// The progress and done steps are genuinely empty until an install has run — no
// log, no result. Screenshotting them as-is would document two blank screens, so
// they get a plausible finished install. Only the DATA is fixture; the QML is
// the shipped QML.
const char *kFixtureLog =
    "[1/9] Partitioning /dev/nvme0n1\n"
    "  created EFI system partition (1.0 GiB, FAT32)\n"
    "  created root partition (511.1 GiB)\n"
    "[2/9] Formatting boot partitions\n"
    "[3/9] Setting up encryption (luks-passphrase)\n"
    "[4/9] Formatting root filesystem (xfs)\n"
    "[5/9] Mounting target at /mnt\n"
    "[6/9] Installing image ghcr.io/tuna-os/albacore:kde\n"
    "  pulling layers... 1.9 GiB\n"
    "[7/9] Writing bootloader entries\n"
    "[8/9] Setting hostname tunaos\n"
    "[9/9] Finalising\n"
    "\n\xE2\x9C\x93 Installation complete!\n";

const char *kFixtureDisks = R"({"blockdevices":[
  {"name":"nvme0n1","size":"512G","type":"disk","model":"Samsung SSD 990 PRO","tran":"nvme"},
  {"name":"sda","size":"1.8T","type":"disk","model":"WDC WD20EZBX","tran":"sata"},
  {"name":"sdb","size":"57.3G","type":"disk","model":"Cruzer Blade","tran":"usb"}
]})";

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    // Software scenegraph: grabWindow() must work with no GPU.
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("QSG_RENDER_LOOP", "basic");

    QApplication app(argc, argv);

    // Same style the shipped binary sets, or the screenshots would document a
    // look the user never sees.
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(u"org.kde.desktop"_s);

    // On a real Plasma session the platform theme supplies this. Offscreen
    // there is no platform theme, so every Kirigami.Icon and every button icon
    // would come back null and the screenshots would document a UI full of
    // holes.
    if (QIcon::themeName().isEmpty() || QIcon::themeName() == u"hicolor"_s)
        QIcon::setThemeName(u"breeze"_s);
    QIcon::setFallbackThemeName(u"breeze"_s);

    QTextStream out(stdout);
    out << "style: " << QQuickStyle::name()
        << "  icon theme: " << QIcon::themeName()
        << "  breeze present: " << (QIcon::hasThemeIcon(u"drive-harddisk-symbolic"_s) ? "yes" : "no")
        << "\n";

    const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : u"docs/screenshots"_s;
    QDir().mkpath(outDir);

    // Feed the disk step a realistic machine; a CI container exposes no disks,
    // and the step would honestly but uselessly render "No disks found".
    if (qEnvironmentVariableIsEmpty("TUNA_INSTALLER_FAKE_LSBLK")) {
        const QString path = QDir(outDir).filePath(u".lsblk-fixture.json"_s);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out << "FAIL: could not write disk fixture\n";
            return 1;
        }
        f.write(kFixtureDisks);
        f.close();
        qputenv("TUNA_INSTALLER_FAKE_LSBLK", path.toLocal8Bit());
    }

    QQmlApplicationEngine engine;
    engine.loadFromModule("org.tunaos.installer"_L1, "Main"_L1);
    if (engine.rootObjects().isEmpty()) {
        out << "FAIL: QML failed to load — see the errors above\n";
        return 1;
    }

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window) {
        out << "FAIL: root object is not a window\n";
        return 1;
    }

    auto *controller = engine.singletonInstance<InstallerController *>(
        u"org.tunaos.installer"_s, u"InstallerController"_s);
    if (!controller) {
        out << "FAIL: InstallerController singleton unavailable\n";
        return 1;
    }
    controller->setDisk(u"/dev/nvme0n1"_s);
    controller->setImage(u"ghcr.io/tuna-os/albacore:kde"_s);
    controller->setEncryptionType(u"luks-passphrase"_s);
    controller->setPassphrase(u"correct horse battery staple"_s);
    // Fills the log and the result. Runs no process and touches no disk.
    controller->loadDemoState(QString::fromUtf8(kFixtureLog), 0);

    window->resize(1000, 700);
    window->show();
    settle(700);

    const QVector<QPair<int, QString>> steps = {
        {0, u"01-welcome"_s},
        {1, u"02-disk"_s},
        {2, u"03-encryption"_s},
        {3, u"04-confirm"_s},
        {4, u"05-progress"_s},
        {5, u"06-done"_s},
    };

    QVector<Finding> findings;
    for (const auto &step : steps) {
        out << "  -> " << step.second << "\n";
        out.flush();

        if (!QMetaObject::invokeMethod(window, "goToStep", Q_ARG(QVariant, step.first))) {
            out << "FAIL: could not invoke goToStep(" << step.first << ")\n";
            return 1;
        }
        settle(400);

        const QImage image = window->grabWindow();
        if (image.isNull()) {
            out << "FAIL: grabWindow() returned nothing for " << step.second << "\n";
            return 1;
        }
        const QString path = outDir + u"/"_s + step.second + u".png"_s;
        if (!image.save(path)) {
            out << "FAIL: could not write " << path << "\n";
            return 1;
        }
        findings.append(audit(image, step.second));
    }

    QStringList failures;
    for (const auto &f : findings) {
        out << QStringLiteral("  %1  colours %2  largest-flat %3%  ink %4%\n")
                   .arg(f.name, -14)
                   .arg(f.colours, 6)
                   .arg(f.flat * 100, 5, 'f', 1)
                   .arg(f.ink * 100, 5, 'f', 2);
        if (f.colours < MIN_COLOURS)
            failures << QStringLiteral("%1: %2 distinct colours — did not render").arg(f.name).arg(f.colours);
        if (f.flat > MAX_FLAT)
            failures << QStringLiteral("%1: %2% one flat colour — blank screen").arg(f.name).arg(f.flat * 100, 0, 'f', 1);
        if (f.ink < MIN_INK)
            failures << QStringLiteral("%1: %2% ink — nothing drawn").arg(f.name).arg(f.ink * 100, 0, 'f', 2);
    }

    if (!failures.isEmpty()) {
        for (const auto &m : failures)
            out << "FAIL: " << m << "\n";
        return 1;
    }

    out << "  wrote " << findings.size() << " screens\n";
    return 0;
}
