// Renders every wizard page to PNG for docs/gui-walkthrough.md and the README.
//
// Reuses the real Wizard and the real pages — nothing about the UI is mocked.
// Qt's offscreen platform plugin means no X server, no compositor and no GPU
// are needed, so this runs on a stock CI runner.
//
// Safe by construction: Wizard::navigateTo() only switches the stacked widget
// and calls the page's prepare(); the install is a separate entry point
// (startInstallation) which this never calls. Worth stating because the
// sibling XFCE installer does start an install from its page-enter hook, so
// "drive the pages" is not automatically harmless across this family.

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QEventLoop>
#include <QTextStream>
#include <QMap>
#include <QPlainTextEdit>
#include "wizard.h"
#include "pages/done.h"

namespace {

void settle(int ms = 250)
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
// blank pages: the files are present, non-empty, and empty.
Finding audit(const QImage &img, const QString &name)
{
    QMap<QRgb, int> counts;
    int samples = 0, ink = 0;
    for (int y = 0; y < img.height(); y += 3) {
        for (int x = 0; x < img.width(); x += 3) {
            const QRgb c = img.pixel(x, y) & 0x00FFFFFF;
            counts[c] += 1;
            samples += 1;
            const int luma = (30 * qRed(c) + 59 * qGreen(c) + 11 * qBlue(c)) / 100;
            if (luma < 160)
                ink += 1;
        }
    }
    int largest = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        largest = qMax(largest, it.value());

    Finding f;
    f.name = name;
    f.colours = counts.size();
    f.flat = samples ? double(largest) / samples : 1.0;
    f.ink = samples ? double(ink) / samples : 0.0;
    return f;
}

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);
    // Same style the shipped binary sets, or the screenshots would document a
    // look the user never sees.
    app.setStyle(QStringLiteral("Fusion"));

    const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                    : QStringLiteral("docs/screenshots");
    QDir().mkpath(outDir);

    Wizard wizard;
    wizard.resize(900, 650);
    wizard.show();
    settle(400);

    const QVector<QPair<QString, QString>> pages = {
        {QStringLiteral("welcome"), QStringLiteral("01-welcome")},
        {QStringLiteral("disk"), QStringLiteral("02-disk")},
        {QStringLiteral("encryption"), QStringLiteral("03-encryption")},
        {QStringLiteral("confirm"), QStringLiteral("04-confirm")},
        {QStringLiteral("progress"), QStringLiteral("05-progress")},
        {QStringLiteral("done"), QStringLiteral("06-done")},
    };

    QTextStream out(stdout);
    QVector<Finding> findings;
    // The progress and done screens are genuinely empty until an install has
    // run — no log, no result. Screenshotting them as-is would document two
    // blank pages, so they are seeded with a plausible finished install. Only
    // the DATA is fixture; the widgets are the real ones.
    const QString fixtureLog = QStringLiteral(
        "[1/9] Partitioning /dev/nvme0n1\n"
        "  created EFI system partition (1.0 GiB, FAT32)\n"
        "  created root partition (511.1 GiB)\n"
        "[2/9] Formatting boot partitions\n"
        "[3/9] Setting up encryption\n"
        "[4/9] Formatting root filesystem (btrfs)\n"
        "[5/9] Mounting target at /mnt\n"
        "[6/9] Installing image ghcr.io/tuna-os/albacore:kde\n"
        "  pulling layers... 1.9 GiB\n");

    if (auto *log = wizard.findChild<QPlainTextEdit *>())
        log->setPlainText(fixtureLog);
    if (auto *done = wizard.findChild<DonePage *>())
        done->setResult(0, fixtureLog);

    for (const auto &p : pages) {
        out << "  -> " << p.first << "\n";
        out.flush();
        wizard.navigateTo(p.first);
        settle(250);
        const QImage img = wizard.grab().toImage();
        const QString path = outDir + QStringLiteral("/") + p.second + QStringLiteral(".png");
        if (!img.save(path)) {
            out << "FAIL: could not write " << path << "\n";
            return 1;
        }
        findings.append(audit(img, p.second));
    }

    QStringList failures;
    for (const auto &f : findings) {
        out << QStringLiteral("  %1  colours %2  largest-flat %3%  ink %4%\n")
                   .arg(f.name, -14)
                   .arg(f.colours, 5)
                   .arg(f.flat * 100, 5, 'f', 1)
                   .arg(f.ink * 100, 5, 'f', 1);
        if (f.colours < 40)
            failures << QStringLiteral("%1: %2 distinct colours — did not render").arg(f.name).arg(f.colours);
        if (f.flat > 0.985)
            failures << QStringLiteral("%1: %2% one flat colour — blank page").arg(f.name).arg(f.flat * 100, 0, 'f', 1);
        if (f.ink < 0.003)
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
