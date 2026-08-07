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
#include <QMetaMethod>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QRect>
#include <QTextStream>
#include <QTimer>
#include <QVariant>

#include "installercontroller.h"
#include "parity_report.h"

using namespace Qt::StringLiterals;

namespace {

// Pixel-audit thresholds, applied to the step content rect, not the whole
// window. Calibrated against measured CI output, not guessed; every run prints
// the numbers these were derived from.
//
// Density on its own cannot be thresholded here, which is what the first
// version of this got wrong. The welcome and done steps are hero pages: one
// icon, a heading, a line of prose, centred on a flat page background with no
// FormCard behind it, while the form steps have a card that paints a
// background over most of the rect. Measured on the six real screens (run
// 31135359770, 1000x700):
//
//   step           colours   ink%   rows%   cols%
//   01-welcome         233   1.32    23.2    54.2
//   02-disk            322  22.34    41.8    53.9
//   03-encryption      344  26.86    54.6    53.9
//   04-confirm         305  30.44    57.7    53.9
//   05-progress        326  10.98   100.0   100.0
//   06-done            161   0.60    15.5    40.7
//
// Ink spans a factor of fifty, so any ink floor loose enough to admit the done
// step is far too loose to catch a blank form step. What IS uniform is how far
// the drawn content is SPREAD: every real screen marks at least 15% of the
// rect's rows and 40% of its columns, and a step whose module drew nothing
// marks 0.0% of both. So the spread is the load-bearing check and ink is a
// weak second opinion.
//
// There is deliberately no max-flat threshold any more. The done step is 99.3%
// one colour by design, so the only passing value would be ~0.995, which a
// font change could cross on a screen that renders perfectly well: a threshold
// with no margin is a flake, not a check.
constexpr int MIN_COLOURS = 60;         // vs 161 measured on the sparsest screen
constexpr double MIN_INK_ROWS = 0.08;   // vs 0.155
constexpr double MIN_INK_COLS = 0.20;   // vs 0.407
constexpr double MIN_INK = 0.0025;      // vs 0.0060

void settle(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

struct Finding {
    QString name;
    int colours = 0;
    double ink = 0.0;      // fraction of samples differing from the background
    double inkRows = 0.0;  // fraction of rows containing any drawn content
    double inkCols = 0.0;  // fraction of columns containing any drawn content
    // Largest single-colour share of the content area. The row/column ratios
    // above are the stronger blank-screen signal and are what this harness
    // fails on; `flat` is carried because tunaOS's parity matrix reads it, and
    // it is MEASURED here rather than defaulted — publishing a field the
    // capture never computed would be a fabricated number in the one file
    // whose whole purpose is to be believed.
    double flat = 0.0;
    // Recorded alongside the existing ratios so the parity report can name
    // WHICH screen was blank rather than only how many were. Same thresholds,
    // same verdict — nothing about the audit is relaxed.
    double stddev = 0.0;
    bool rendered = false;
    QString png;
    QString text;
};

// Read the pixels back. A capture that only checks its PNGs exist will publish
// blank screens: the files are present, non-empty, and empty. That exact check
// passed in a sibling repo while shipping a blank page.
//
// Only `rect` is examined, which is the step content area rather than the whole
// window; see stepsContainer in Wizard.qml. The heading and the button row
// draw identically on all six steps, so including them lets a step whose module
// rendered nothing coast through on the chrome's ink.
//
// "ink" is measured against the region's own dominant colour rather than a fixed
// luma, because a Plasma dark colour scheme inverts the sense of "dark pixel".
Finding audit(const QImage &image, const QRect &rect, const QString &name)
{
    constexpr int STRIDE = 3;

    QMap<QRgb, int> counts;
    int samples = 0;
    for (int y = rect.top(); y <= rect.bottom(); y += STRIDE) {
        for (int x = rect.left(); x <= rect.right(); x += STRIDE) {
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
    const auto isInk = [&image, background](int x, int y) {
        const QRgb c = image.pixel(x, y) & 0x00FFFFFF;
        return qAbs(qRed(c) - qRed(background))
             + qAbs(qGreen(c) - qGreen(background))
             + qAbs(qBlue(c) - qBlue(background)) > 40;
    };

    int ink = 0;
    int inkRows = 0;
    int rows = 0;
    for (int y = rect.top(); y <= rect.bottom(); y += STRIDE) {
        rows += 1;
        bool any = false;
        for (int x = rect.left(); x <= rect.right(); x += STRIDE) {
            if (isInk(x, y)) {
                ink += 1;
                any = true;
            }
        }
        if (any)
            inkRows += 1;
    }

    int inkCols = 0;
    int cols = 0;
    for (int x = rect.left(); x <= rect.right(); x += STRIDE) {
        cols += 1;
        for (int y = rect.top(); y <= rect.bottom(); y += STRIDE) {
            if (isInk(x, y)) {
                inkCols += 1;
                break;
            }
        }
    }

    Finding f;
    f.name = name;
    f.colours = counts.size();
    f.ink = samples ? double(ink) / samples : 0.0;
    f.inkRows = rows ? double(inkRows) / rows : 0.0;
    f.inkCols = cols ? double(inkCols) / cols : 0.0;
    f.flat = samples ? double(largest) / samples : 0.0;
    f.stddev = parity::stddev(image);
    return f;
}

// Every string the CURRENT step put in its QML item tree — what the parity
// keywords are matched against.
//
// parity_report.h's pageText() is QWidget-only and cannot see a QQuickItem, so
// this is the QML equivalent, deliberately with the same two rules:
//
//   * scoped to the step being shown, not the whole wizard. Wizard.qml keeps
//     every module instantiated and slides between them, so walking from the
//     root would collect all six steps' text on every frame and credit every
//     screen on every frame — the exact false-parity failure the spec's
//     comments were written about.
//   * invisible items are skipped, so a step's hidden sub-tree (the passphrase
//     fields when encryption is off) does not contribute text the user cannot
//     read.
//
// Properties are read by name because the item types live in QML: `text` covers
// Label/Button/TextField, and FormCard's rows carry their heading in `title`
// with the explanatory line in `description`.
QString itemText(QQuickItem *item)
{
    if (!item || !item->isVisible())
        return QString();
    QStringList parts;
    const auto add = [&parts](const QVariant &v) {
        const QString s = v.toString();
        if (!s.trimmed().isEmpty())
            parts << s;
    };
    for (const char *prop : {"text", "title", "description", "subtitle"}) {
        const QVariant v = item->property(prop);
        if (v.isValid())
            add(v);
    }
    const auto kids = item->childItems();
    for (QQuickItem *k : kids) {
        const QString s = itemText(k);
        if (!s.isEmpty())
            parts << s;
    }
    return parts.join(QLatin1Char(' '));
}

// Where the current step draws, in image coordinates.
//
// Returned empty if the item is missing, and the caller treats that as fatal:
// silently falling back to the whole window would quietly restore the blind
// spot this exists to remove.
QRect stepContentRect(QQuickWindow *window, const QImage &image)
{
    auto *item = window->findChild<QQuickItem *>(u"stepsContainer"_s);
    if (!item || item->width() <= 0 || item->height() <= 0 || window->width() <= 0)
        return QRect();

    // grabWindow() hands back a device-pixel image; item geometry is in layout
    // pixels. Equal at the offscreen platform's ratio of 1, but not a safe
    // assumption to bake in.
    const qreal scale = qreal(image.width()) / window->width();
    const QPointF topLeft = item->mapToScene(QPointF(0, 0)) * scale;
    const QRectF scaled(topLeft, QSizeF(item->width() * scale, item->height() * scale));

    return scaled.toRect().intersected(QRect(QPoint(0, 0), image.size()));
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

    // Probe a name Breeze actually defines. The first version asked for
    // "drive-harddisk-symbolic" and reported "no" on a run whose icons all drew
    // fine: Breeze ships no -symbolic alias for its device icons, and
    // Kirigami.Icon resolves those names by stripping the suffix. So the probe
    // was answering "does Breeze define this alias" (no) while claiming to
    // answer "is Breeze installed" (yes). A diagnostic that sends you after
    // the wrong thing is worse than none.
    QTextStream out(stdout);
    out << "style: " << QQuickStyle::name()
        << "  icon theme: " << QIcon::themeName()
        << "  breeze present: " << (QIcon::hasThemeIcon(u"drive-harddisk"_s) ? "yes" : "no")
        << "\n";

    const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : u"docs/screenshots"_s;
    QDir().mkpath(outDir);

    // Clear anything already there. A run that dies on the first screen must
    // not leave the previous run's — or the previous UI's — images sitting in
    // the output directory, where the artifact upload will happily publish them
    // as if they were this run's output. That is the same "looks like output,
    // isn't" failure the pixel audit exists to catch.
    {
        QDir dir(outDir);
        const QStringList stale = dir.entryList({u"*.png"_s, u"*.gif"_s}, QDir::Files);
        for (const QString &name : stale) {
            if (!dir.remove(name)) {
                out << "FAIL: could not clear stale " << name << "\n";
                return 1;
            }
        }
        out << "cleared " << stale.size() << " stale image(s) from " << outDir << "\n";
    }

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

    QObject *rootObject = engine.rootObjects().constFirst();
    out << "root object: " << rootObject->metaObject()->className()
        << " (objectName \"" << rootObject->objectName() << "\")\n";

    auto *window = qobject_cast<QQuickWindow *>(rootObject);
    if (!window) {
        out << "FAIL: root object is not a window\n";
        return 1;
    }

    // A QML function declared with a typed parameter (`index: int`) lands in
    // the metaobject as an int, so Q_ARG(int) matches; an untyped one takes a
    // QVariant. Try both rather than assume, and say which worked — a bare
    // "could not invoke" names the symptom, not the cause.
    const auto callGoToStep = [&](int index) {
        if (QMetaObject::invokeMethod(window, "goToStep", Q_ARG(int, index)))
            return true;
        return QMetaObject::invokeMethod(window, "goToStep", Q_ARG(QVariant, QVariant(index)));
    };
    if (!callGoToStep(0)) {
        out << "FAIL: goToStep is not invokable on the root object. Methods:\n";
        const QMetaObject *mo = window->metaObject();
        for (int i = 0; i < mo->methodCount(); ++i)
            out << "    " << mo->method(i).methodSignature() << "\n";
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
    // Kept alongside `findings` so consecutive frames can be diffed for the
    // parity report's transition counts.
    QVector<QImage> images;
    for (const auto &step : steps) {
        out << "  -> " << step.second << "\n";
        out.flush();

        if (!callGoToStep(step.first)) {
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
        const QRect content = stepContentRect(window, image);
        if (content.isEmpty()) {
            out << "FAIL: could not locate the step content area for " << step.second << "\n";
            return 1;
        }
        Finding f = audit(image, content, step.second);
        f.png = path;
        // The step the wizard is CURRENTLY showing only. Wizard.goToStep sets
        // `item.visible = (i === index)` on every module, and itemText() skips
        // invisible items, so this cannot collect the other five steps' text
        // and credit every screen on every frame.
        f.text = itemText(window->contentItem());
        findings.append(f);
        images.append(image);
    }

    QStringList failures;
    for (auto &f : findings) {
        out << QStringLiteral("  %1  colours %2  ink %3%  rows %4%  cols %5%\n")
                   .arg(f.name, -14)
                   .arg(f.colours, 6)
                   .arg(f.ink * 100, 6, 'f', 2)
                   .arg(f.inkRows * 100, 5, 'f', 1)
                   .arg(f.inkCols * 100, 5, 'f', 1);
        QStringList stepFailures;
        if (f.colours < MIN_COLOURS)
            stepFailures << QStringLiteral("%1: %2 distinct colours — did not render").arg(f.name).arg(f.colours);
        if (f.inkRows < MIN_INK_ROWS)
            stepFailures << QStringLiteral("%1: content on only %2% of rows (blank screen)").arg(f.name).arg(f.inkRows * 100, 0, 'f', 1);
        if (f.inkCols < MIN_INK_COLS)
            stepFailures << QStringLiteral("%1: content on only %2% of columns (blank screen)").arg(f.name).arg(f.inkCols * 100, 0, 'f', 1);
        if (f.ink < MIN_INK)
            stepFailures << QStringLiteral("%1: %2% ink — nothing drawn").arg(f.name).arg(f.ink * 100, 0, 'f', 2);
        f.rendered = stepFailures.isEmpty();
        failures << stepFailures;
    }

    // Emitted before the failure gate on purpose: a frontend that renders a
    // blank screen is exactly the case tunaOS's parity matrix most needs a row
    // for. Returning first would leave the row unfilled, which is how the last
    // crop of frontend defects survived unseen.
    {
        QVector<parity::Page> reportPages;
        for (const auto &f : findings)
            reportPages.append({f.name, f.png, f.text, f.rendered, f.colours,
                                f.flat, f.ink, f.stddev});
        QVector<int> transitions;
        for (int i = 1; i < images.size(); ++i)
            transitions.append(parity::changedPixels(images[i - 1], images[i]));
        parity::write(outDir, QStringLiteral("kde"), reportPages,
                      QStringLiteral("tests/capture.cpp (Kirigami/QML, offscreen)"),
                      transitions, out);
    }

    if (!failures.isEmpty()) {
        for (const auto &m : failures)
            out << "FAIL: " << m << "\n";
        return 1;
    }

    out << "  wrote " << findings.size() << " screens\n";
    return 0;
}
