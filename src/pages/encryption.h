#ifndef ENCRYPTION_PAGE_H
#define ENCRYPTION_PAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QList>
#include <QRadioButton>

class Wizard;

// Disk encryption choice. The recipe has carried encryption.type and
// encryption.passphrase since the beginning and fisherman honours both, but
// nothing ever set them: this frontend had no encryption step, so every
// install came out unencrypted with no way to ask otherwise (tunaOS#734).
class EncryptionPage : public QWidget
{
    Q_OBJECT

public:
    explicit EncryptionPage(Wizard *wizard, QWidget *parent = nullptr);
    void prepare();

signals:
    void nextRequested();
    void backRequested();

private:
    Wizard *m_wizard;
    QList<QRadioButton *> m_choices;
    QLineEdit *m_passphrase;
    QLineEdit *m_passphraseConfirm;
    QLabel *m_passphraseError;
    QWidget *m_passphraseBox;
    QRadioButton *m_continueTarget = nullptr;

    // TPM options are hidden when the machine has no TPM, rather than offered
    // and then failing at install time.
    bool m_hasTpm = false;

    QString selectedType() const;
    bool needsPassphrase() const;
    void syncPassphraseVisibility();
    bool validate();
};

#endif // ENCRYPTION_PAGE_H
