#include "wizard.h"
#include "pages/welcome.h"
#include "pages/diskselection.h"
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
    m_confirm  = new ConfirmPage(this);
    m_progress = new ProgressPage(this);
    m_done     = new DonePage(this);

    m_stack->addWidget(m_welcome);
    m_stack->addWidget(m_disk);
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
        navigateTo("confirm");
    });

    // Confirm → Back to disk
    connect(m_confirm, &ConfirmPage::backRequested, this, [this]() {
        navigateTo("disk");
    });

    // Confirm → Start install
    connect(m_confirm, &ConfirmPage::installRequested, this, [this]() {
        startInstallation();
    });

    // Progress → Done
    connect(m_progress, &ProgressPage::finished, this, [this](int code, const QString &output) {
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
    else if (auto *c = qobject_cast<ConfirmPage *>(child))
        c->prepare();

    m_stack->setCurrentWidget(child);
}

void Wizard::startInstallation()
{
    // Write recipe to temp file
    QString recipePath = QDir::tempPath() + "/fisherman-recipe.json";
    QFile f(recipePath);
    if (!f.open(QIODevice::WriteOnly)) {
        m_progress->onError("Failed to write recipe file");
        return;
    }
    f.write(QJsonDocument(m_recipe.toJson()).toJson(QJsonDocument::Indented));
    f.close();
    qDebug() << "Recipe written to" << recipePath;

    m_progress->start(recipePath);
    navigateTo("progress");
}

void Wizard::onInstallFinished(int exitCode, const QString &output)
{
    m_progress->onFinished(exitCode, output);
}
