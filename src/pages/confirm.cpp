#include "confirm.h"
#include "../wizard.h"
#include "../recipe.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QShortcut>
#include <QKeySequence>

ConfirmPage::ConfirmPage(Wizard *wizard, QWidget *parent)
    : QWidget(parent), m_wizard(wizard)
{
    setObjectName("confirm");
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("<h2>Confirm Installation</h2>"));
    layout->addWidget(title);

    auto *desc = new QLabel(QStringLiteral(
        "Review your choices before starting the installation. "
        "All data on the selected disk will be erased."
    ));
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto *form = new QFormLayout;
    m_diskLabel = new QLabel;
    m_fsLabel = new QLabel;
    m_encLabel = new QLabel;
    m_hostnameLabel = new QLabel;
    m_imageLabel = new QLabel;
    m_imageLabel->setWordWrap(true);

    form->addRow(QStringLiteral("Target Disk:"), m_diskLabel);
    form->addRow(QStringLiteral("Filesystem:"), m_fsLabel);
    form->addRow(QStringLiteral("Encryption:"), m_encLabel);
    form->addRow(QStringLiteral("Hostname:"), m_hostnameLabel);
    form->addRow(QStringLiteral("Image:"), m_imageLabel);
    layout->addLayout(form);

    layout->addStretch();

    auto *btnLayout = new QHBoxLayout;
    auto *btnBack = new QPushButton(QStringLiteral("Back"));
    auto *btnInstall = new QPushButton(QStringLiteral("Install"));
    // Enter/Return must fire this page's primary action.
    //
    // setDefault()/setAutoDefault() do NOT work here: Qt only gives default
    // buttons their Enter behaviour inside a QDialog, and these pages are
    // plain QWidgets. Verified — with setDefault alone a synthetic Return
    // still does not emit clicked(). Binding the keys explicitly does.
    //
    // Without this the wizard cannot be advanced by keyboard at all: a
    // focused QPushButton takes Space, never Enter.
    for (auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto *sc = new QShortcut(QKeySequence(key), this);
        connect(sc, &QShortcut::activated, btnInstall, [btnInstall]() {
            if (btnInstall->isEnabled()) {
                btnInstall->animateClick();
            }
        });
    }
    btnInstall->setStyleSheet("QPushButton { background-color: #3584e4; color: white; padding: 8px 24px; }");
    btnLayout->addWidget(btnBack);
    btnLayout->addStretch();
    btnLayout->addWidget(btnInstall);
    layout->addLayout(btnLayout);

    connect(btnBack, &QPushButton::clicked, this, &ConfirmPage::backRequested);
    connect(btnInstall, &QPushButton::clicked, this, &ConfirmPage::installRequested);
}

void ConfirmPage::prepare()
{
    const auto &r = m_wizard->recipe();
    m_diskLabel->setText(r.disk);
    m_fsLabel->setText(r.filesystem + (r.btrfsSubvolumes ? QStringLiteral(" (with subvolumes)") : QString()));
    m_encLabel->setText(r.encryption.type == "luks" ? QStringLiteral("LUKS") : QStringLiteral("None"));
    m_hostnameLabel->setText(r.hostname);
    m_imageLabel->setText(r.image);
}
