#ifndef WIZARD_H
#define WIZARD_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QProcess>
#include "recipe.h"

class WelcomePage;
class DiskSelectionPage;
class EncryptionPage;
class ConfirmPage;
class ProgressPage;
class DonePage;

class Wizard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Wizard(QWidget *parent = nullptr);
    ~Wizard() override = default;

    Recipe &recipe() { return m_recipe; }
    void navigateTo(const QString &page);
    void startInstallation();
    void onInstallFinished(int exitCode, const QString &output);

private:
    QStackedWidget *m_stack;
    Recipe m_recipe;
    QString m_recipePath;

    WelcomePage *m_welcome;
    DiskSelectionPage *m_disk;
    EncryptionPage *m_encryption;
    ConfirmPage *m_confirm;
    ProgressPage *m_progress;
    DonePage *m_done;

    QProcess *m_fisherman = nullptr;

    void setupUi();
    void connectSignals();
};

#endif // WIZARD_H
