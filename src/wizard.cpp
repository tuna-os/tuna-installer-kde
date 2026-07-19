#include "wizard.h"
#include "offline.h"
#include "pages/welcome.h"
#include "pages/diskselection.h"
#include "pages/encryption.h"
#include "pages/confirm.h"
#include "pages/progress.h"
#include "pages/done.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDebug>

Wizard::Wizard(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TunaOS Installer"));
    resize(800, 600);
    setupUi();
    connectSignals();
    navigateTo("welcome");
}

void Wizard::setupUi()
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_welcome  = new WelcomePage(this);
    m_disk     = new DiskSelectionPage(this);
    m_encryption = new EncryptionPage(this);
    m_confirm  = new ConfirmPage(this);
    m_progress = new ProgressPage(this);
    m_done     = new DonePage(this);

    m_stack->addWidget(m_welcome);
    m_stack->addWidget(m_disk);
    m_stack->addWidget(m_encryption);
    m_stack->addWidget(m_confirm);
    m_stack->addWidget(m_progress);
    m_stack->addWidget(m_done);
}

void Wizard::connectSignals()
{
    // Welcome → Disk selection
    connect(m_welcome, &WelcomePage::nextRequested, this, [this]() {
        navigateTo("disk");
    });

    // Disk selection → Confirm
    connect(m_disk, &DiskSelectionPage::nextRequested, this, [this]() {
        navigateTo("encryption");
    });

    // Encryption → Confirm. The page has already validated the passphrase, so
    // the recipe can be written straight through.
    connect(m_encryption, &EncryptionPage::nextRequested, this, [this]() {
        navigateTo("confirm");
    });
    connect(m_encryption, &EncryptionPage::backRequested, this, [this]() {
        navigateTo("disk");
    });

    // Confirm → Back to disk
    connect(m_confirm, &ConfirmPage::backRequested, this, [this]() {
        navigateTo("encryption");
    });

    // Confirm → Start install
    connect(m_confirm, &ConfirmPage::installRequested, this, [this]() {
        startInstallation();
    });

    // Progress → Done
    connect(m_progress, &ProgressPage::finished, this, [this](int code, const QString &output) {
        if (!m_recipePath.isEmpty())
            QFile::remove(m_recipePath); // recipe may hold secrets
        m_done->setResult(code, output);
        navigateTo("done");
    });

    // Done → Quit
    connect(m_done, &DonePage::quitRequested, this, [this]() {
        close();
    });
}

void Wizard::navigateTo(const QString &page)
{
    auto *child = m_stack->findChild<QWidget *>(page);
    if (auto *w = qobject_cast<WelcomePage *>(child))
        w->prepare();
    else if (auto *d = qobject_cast<DiskSelectionPage *>(child))
        d->prepare();
    else if (auto *e = qobject_cast<EncryptionPage *>(child))
        e->prepare();
    else if (auto *c = qobject_cast<ConfirmPage *>(child))
        c->prepare();

    m_stack->setCurrentWidget(child);
}

void Wizard::startInstallation()
{
    // Offline install support (spec §4): live-ISO mode allows an empty image;
    // embedded stores are always passed — fisherman ignores unhelpful ones.
    if (m_recipe.image.isEmpty() && !offline::liveIsoImage().isEmpty())
        m_recipe.liveMode = true;
    if (m_recipe.additionalImageStores.isEmpty())
        m_recipe.additionalImageStores = offline::offlineStores();

    // The recipe can hold a LUKS passphrase — write it 0600 under
    // XDG_RUNTIME_DIR, never a world-readable temp path.
    QString base = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (base.isEmpty())
        base = QDir::tempPath();
    QDir().mkpath(base + QStringLiteral("/tuna-installer"));
    m_recipePath = base + QStringLiteral("/tuna-installer/recipe.json");
    QFile f(m_recipePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_progress->onError("Failed to write recipe file");
        return;
    }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    f.write(QJsonDocument(m_recipe.toJson()).toJson(QJsonDocument::Indented));
    f.close();

    m_progress->start(m_recipePath);
    navigateTo("progress");
}

void Wizard::onInstallFinished(int exitCode, const QString &output)
{
    m_progress->onFinished(exitCode, output);
}
