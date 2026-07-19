#include "welcome.h"
#include "../wizard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>

WelcomePage::WelcomePage(Wizard *wizard, QWidget *parent)
    : QWidget(parent), m_wizard(wizard)
{
    setObjectName("welcome");
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral("<h1>TunaOS Installer</h1>"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *desc = new QLabel(QStringLiteral(
        "<p>This wizard will guide you through installing TunaOS onto your computer.</p>"
        "<p>You'll select a target disk, configure filesystem and encryption options, "
        "and the installer will do the rest.</p>"
    ));
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    layout->addSpacing(20);

    auto *btn = new QPushButton(QStringLiteral("Get Started"));
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
        connect(sc, &QShortcut::activated, btn, [btn]() {
            if (btn->isEnabled()) {
                btn->animateClick();
            }
        });
    }
    btn->setFixedWidth(200);
    layout->addWidget(btn, 0, Qt::AlignCenter);

    connect(btn, &QPushButton::clicked, this, &WelcomePage::nextRequested);
}

void WelcomePage::prepare()
{
    // No state to refresh on the welcome page
}
