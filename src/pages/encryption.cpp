#include "encryption.h"
#include "../wizard.h"
#include "../recipe.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

namespace {

struct Choice {
    const char *value;
    const char *label;
    const char *explain;
};

// Mirrors ENCRYPTION_CHOICES in tuna-installer-xfce, which is the reference
// implementation — the wording is deliberately identical so the two frontends
// describe the same options the same way.
const Choice kChoices[] = {
    {"none", "No encryption", "Anyone with the disk can read your files."},
    {"luks-passphrase", "Passphrase", "You'll type it at every boot."},
    {"tpm2-luks", "TPM", "Unlocks automatically on this hardware."},
    {"tpm2-luks-passphrase", "TPM + passphrase", "Automatic unlock, passphrase as fallback."},
};

} // namespace

EncryptionPage::EncryptionPage(Wizard *wizard, QWidget *parent)
    : QWidget(parent), m_wizard(wizard)
{
    setObjectName("encryption");

    // Same probe the XFCE frontend uses.
    m_hasTpm = QFileInfo::exists(QStringLiteral("/sys/class/tpm/tpm0"));

    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("<h2>Disk Encryption</h2>"));
    layout->addWidget(title);

    auto *desc = new QLabel(QStringLiteral(
        "<p>Encryption protects your files if the disk is lost or stolen. "
        "It cannot be turned on later without reinstalling.</p>"));
    desc->setWordWrap(true);
    layout->addWidget(desc);
    layout->addSpacing(8);

    for (const auto &c : kChoices) {
        const QString value = QString::fromLatin1(c.value);
        // Offering a TPM option on a machine without a TPM would fail at
        // install time, so hide rather than disable — same as XFCE.
        if (value.startsWith(QStringLiteral("tpm2")) && !m_hasTpm)
            continue;

        auto *radio = new QRadioButton(QString::fromLatin1(c.label));
        radio->setProperty("encType", value);
        layout->addWidget(radio);

        auto *explain = new QLabel(QStringLiteral("<span style='color:gray'>%1</span>")
                                       .arg(QString::fromLatin1(c.explain)));
        explain->setIndent(24);
        layout->addWidget(explain);

        connect(radio, &QRadioButton::toggled, this, [this]() { syncPassphraseVisibility(); });
        m_choices.append(radio);
    }
    if (!m_choices.isEmpty())
        m_choices.first()->setChecked(true);

    // Passphrase entry, shown only for the options that need one.
    m_passphraseBox = new QWidget(this);
    auto *pbLayout = new QVBoxLayout(m_passphraseBox);
    pbLayout->setContentsMargins(24, 8, 0, 0);

    m_passphrase = new QLineEdit();
    m_passphrase->setEchoMode(QLineEdit::Password);
    m_passphrase->setPlaceholderText(QStringLiteral("Enter passphrase"));
    m_passphraseConfirm = new QLineEdit();
    m_passphraseConfirm->setEchoMode(QLineEdit::Password);
    m_passphraseConfirm->setPlaceholderText(QStringLiteral("Confirm passphrase"));
    m_passphraseError = new QLabel();
    m_passphraseError->setStyleSheet(QStringLiteral("color: #c0392b"));
    m_passphraseError->hide();

    pbLayout->addWidget(m_passphrase);
    pbLayout->addWidget(m_passphraseConfirm);
    pbLayout->addWidget(m_passphraseError);
    layout->addWidget(m_passphraseBox);

    layout->addStretch();

    auto *btnLayout = new QHBoxLayout();
    auto *btnBack = new QPushButton(QStringLiteral("Back"));
    auto *btnNext = new QPushButton(QStringLiteral("Continue"));
    btnLayout->addWidget(btnBack);
    btnLayout->addStretch();
    btnLayout->addWidget(btnNext);
    layout->addLayout(btnLayout);

    connect(btnBack, &QPushButton::clicked, this, &EncryptionPage::backRequested);
    connect(btnNext, &QPushButton::clicked, this, [this]() {
        if (!validate())
            return;
        // Commit to the recipe. Without this the page would look like it
        // works while every install still came out unencrypted — which is
        // the bug being fixed, not a new one.
        auto &enc = m_wizard->recipe().encryption;
        enc.type = selectedType();
        enc.passphrase = needsPassphrase() ? m_passphrase->text() : QString();
        emit nextRequested();
    });

    // Enter fires Continue. setDefault()/setAutoDefault() do not work here —
    // Qt only gives default buttons their Enter behaviour inside a QDialog,
    // and these pages are plain QWidgets. See tuna-installer-kde#4.
    for (auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        auto *sc = new QShortcut(QKeySequence(key), this);
        connect(sc, &QShortcut::activated, btnNext, [btnNext]() {
            if (btnNext->isEnabled())
                btnNext->animateClick();
        });
    }

    syncPassphraseVisibility();
}

QString EncryptionPage::selectedType() const
{
    for (auto *r : m_choices) {
        if (r->isChecked())
            return r->property("encType").toString();
    }
    return QStringLiteral("none");
}

bool EncryptionPage::needsPassphrase() const
{
    return selectedType().endsWith(QStringLiteral("passphrase"));
}

void EncryptionPage::syncPassphraseVisibility()
{
    m_passphraseBox->setVisible(needsPassphrase());
    if (!needsPassphrase()) {
        m_passphrase->clear();
        m_passphraseConfirm->clear();
        m_passphraseError->hide();
    }
}

bool EncryptionPage::validate()
{
    m_passphraseError->hide();
    if (!needsPassphrase())
        return true;

    // Recipe::validationError() rejects a *-passphrase type with an empty
    // passphrase, but that failure would surface as an opaque error after the
    // user has already confirmed. Catch it here where it can be corrected.
    if (m_passphrase->text().isEmpty()) {
        m_passphraseError->setText(QStringLiteral("Enter a passphrase."));
        m_passphraseError->show();
        return false;
    }
    if (m_passphrase->text() != m_passphraseConfirm->text()) {
        m_passphraseError->setText(QStringLiteral("Passphrases do not match."));
        m_passphraseError->show();
        return false;
    }
    return true;
}

void EncryptionPage::prepare()
{
    // Reflect whatever the recipe already holds, so going Back and returning
    // does not silently reset the choice.
    const QString current = m_wizard->recipe().encryption.type;
    for (auto *r : m_choices) {
        if (r->property("encType").toString() == current) {
            r->setChecked(true);
            break;
        }
    }
    syncPassphraseVisibility();
}
