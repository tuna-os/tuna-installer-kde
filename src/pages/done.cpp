#include "done.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>

DonePage::DonePage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("done");
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    m_icon = new QLabel;
    m_icon->setAlignment(Qt::AlignCenter);
    m_icon->setStyleSheet("font-size: 48px;");
    layout->addWidget(m_icon);

    m_title = new QLabel;
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setStyleSheet("font-size: 24px; font-weight: bold;");
    layout->addWidget(m_title);

    m_detail = new QLabel;
    m_detail->setAlignment(Qt::AlignCenter);
    m_detail->setWordWrap(true);
    layout->addWidget(m_detail);

    layout->addSpacing(20);

    auto *btnQuit = new QPushButton(QStringLiteral("Close"));
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
        connect(sc, &QShortcut::activated, btnQuit, [btnQuit]() {
            if (btnQuit->isEnabled()) {
                btnQuit->animateClick();
            }
        });
    }
    btnQuit->setFixedWidth(200);
    layout->addWidget(btnQuit, 0, Qt::AlignCenter);

    connect(btnQuit, &QPushButton::clicked, this, &DonePage::quitRequested);
}

void DonePage::setResult(int exitCode, const QString &output)
{
    if (exitCode == 0) {
        m_icon->setText(QStringLiteral("✓"));
        m_icon->setStyleSheet("font-size: 48px; color: #33d17a;");
        m_title->setText(QStringLiteral("Installation Complete"));
        m_detail->setText(QStringLiteral(
            "TunaOS has been installed successfully. "
            "Remove the installation media and restart your computer to boot into your new system."
        ));
    } else {
        m_icon->setText(QStringLiteral("✗"));
        m_icon->setStyleSheet("font-size: 48px; color: #e66100;");
        m_title->setText(QStringLiteral("Installation Failed"));
        m_detail->setText(QStringLiteral(
            "The installation encountered an error. Check the installation log for details."
        ));
    }
}
