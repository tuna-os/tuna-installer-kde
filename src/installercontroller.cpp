#include "installercontroller.h"
#include "offline.h"
#include "productname.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>

InstallerController::InstallerController(QObject *parent)
    : QObject(parent)
{
    m_hasTpm = QFileInfo::exists(QStringLiteral("/sys/class/tpm/tpm0"));
    m_productName = product::resolve();
}

void InstallerController::setDisk(const QString &v)
{
    if (m_recipe.disk == v)
        return;
    m_recipe.disk = v;
    Q_EMIT recipeChanged();
}

void InstallerController::setFilesystem(const QString &v)
{
    if (m_recipe.filesystem == v)
        return;
    m_recipe.filesystem = v;
    Q_EMIT recipeChanged();
}

void InstallerController::setBtrfsSubvolumes(bool v)
{
    if (m_recipe.btrfsSubvolumes == v)
        return;
    m_recipe.btrfsSubvolumes = v;
    Q_EMIT recipeChanged();
}

void InstallerController::setEncryptionType(const QString &v)
{
    if (m_recipe.encryption.type == v)
        return;
    m_recipe.encryption.type = v;
    // A type that takes no passphrase must not keep one lying around in the
    // recipe file.
    if (!v.endsWith(QLatin1String("passphrase")))
        m_recipe.encryption.passphrase.clear();
    Q_EMIT recipeChanged();
}

void InstallerController::setPassphrase(const QString &v)
{
    if (m_recipe.encryption.passphrase == v)
        return;
    m_recipe.encryption.passphrase = v;
    Q_EMIT recipeChanged();
}

void InstallerController::setHostname(const QString &v)
{
    if (m_recipe.hostname == v)
        return;
    m_recipe.hostname = v;
    Q_EMIT recipeChanged();
}

void InstallerController::setImage(const QString &v)
{
    if (m_recipe.image == v)
        return;
    m_recipe.image = v;
    Q_EMIT recipeChanged();
}

QString InstallerController::encryptionLabel(const QString &type) const
{
    if (type == QLatin1String("luks-passphrase"))
        return QStringLiteral("Passphrase (LUKS)");
    if (type == QLatin1String("tpm2-luks"))
        return QStringLiteral("TPM");
    if (type == QLatin1String("tpm2-luks-passphrase"))
        return QStringLiteral("TPM + passphrase");
    return QStringLiteral("None");
}

void InstallerController::appendLine(const QString &line)
{
    m_log += line;
    if (!line.endsWith(QLatin1Char('\n')))
        m_log += QLatin1Char('\n');
    Q_EMIT logChanged();
}

void InstallerController::drainBuffer(const QString &prefix)
{
    QStringList lines = m_buffer.split(QLatin1Char('\n'));
    if (lines.size() <= 1)
        return;
    for (int i = 0; i < lines.size() - 1; ++i)
        appendLine(prefix + lines.at(i));
    m_buffer = lines.last();
}

void InstallerController::fail(const QString &message)
{
    appendLine(QStringLiteral("\nERROR: %1").arg(message));
    m_finished = true;
    m_exitCode = 1;
    Q_EMIT installCompleted(1);
}

void InstallerController::loadDemoState(const QString &log, int exitCode)
{
    m_log = log;
    m_finished = true;
    m_exitCode = exitCode;
    Q_EMIT logChanged();
    Q_EMIT installCompleted(exitCode);
}

void InstallerController::startInstall()
{
    if (m_process)
        return;

    // Offline install support (INSTALLER-FRONTENDS.md §4): live-ISO mode allows
    // an empty image; embedded stores are always passed — fisherman ignores
    // unhelpful ones.
    if (m_recipe.image.isEmpty() && !offline::liveIsoImage().isEmpty())
        m_recipe.liveMode = true;
    if (m_recipe.additionalImageStores.isEmpty())
        m_recipe.additionalImageStores = offline::offlineStores();

    m_log.clear();
    m_buffer.clear();
    m_finished = false;
    Q_EMIT logChanged();
    appendLine(QStringLiteral("Starting installation..."));

    // The recipe can hold a LUKS passphrase — write it 0600 in a fresh
    // 0700 private directory, never a fixed world-writable temp path.
    //
    // This used to be `<base>/tuna-installer/recipe.json` where base is
    // XDG_RUNTIME_DIR or /tmp (when unset — the default under sudo, whose
    // env_reset strips it). QDir::mkpath with a fixed name in a
    // world-writable directory:
    //   * follows a pre-existing attacker symlink when the file is opened
    //     with WriteOnly|Truncate,
    //   * creates the file 0644 (umask) and only THEN chmods it 0600, so
    //     the passphrase is briefly world-readable,
    //   * accepts a pre-created 0777 directory as-is (mode applied only on
    //     creation), giving a local user ownership of the path that is
    //     handed to root via sudo/pkexec fisherman.
    // QTemporaryDir is the mkdtemp analogue: unpredictable 0700 directory
    // with O_EXCL semantics, so none of those races exist. It is a member so
    // the directory survives until fisherman exits — a local QTemporaryDir
    // would auto-remove the recipe file the moment startInstall returns.
    if (!m_recipeDir.isValid())
        m_recipeDir = QTemporaryDir();
    if (!m_recipeDir.isValid()) {
        fail(QStringLiteral("Failed to create private recipe directory"));
        return;
    }
    m_recipePath = m_recipeDir.filePath(QStringLiteral("recipe.json"));
    QFile f(m_recipePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Failed to write recipe file"));
        return;
    }
    // Inside a 0700 dir the file needs no extra hardening, but keep 0600
    // explicit so the mode survives being copied/moved elsewhere.
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    f.write(QJsonDocument(m_recipe.toJson()).toJson(QJsonDocument::Indented));
    f.close();

    // pkexec /app/bin/fisherman in Flatpak, sudo /usr/local/bin/fisherman otherwise.
    QStringList cmd = offline::fishermanCommand();
    cmd << m_recipePath;

    m_process = new QProcess(this);
    m_process->setProgram(cmd.takeFirst());
    m_process->setArguments(cmd);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        m_buffer += QString::fromUtf8(m_process->readAllStandardOutput());
        drainBuffer({});
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        m_buffer += QString::fromUtf8(m_process->readAllStandardError());
        drainBuffer(QStringLiteral("[stderr] "));
    });
    connect(m_process, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
        if (!m_buffer.isEmpty()) {
            appendLine(m_buffer);
            m_buffer.clear();
        }
        // The recipe may hold secrets — remove it as soon as fisherman is done.
        if (!m_recipePath.isEmpty())
            QFile::remove(m_recipePath);

        if (status == QProcess::CrashExit) {
            appendLine(QStringLiteral("\nERROR: fisherman crashed"));
            m_exitCode = 1;
        } else {
            m_exitCode = exitCode;
            appendLine(exitCode == 0
                           ? QStringLiteral("\n✓ Installation complete!")
                           : QStringLiteral("\n✗ Installation failed (exit code %1)").arg(exitCode));
        }
        m_finished = true;
        m_process->deleteLater();
        m_process = nullptr;
        Q_EMIT installingChanged();
        Q_EMIT installCompleted(m_exitCode);
    });

    m_process->start();
    Q_EMIT installingChanged();
}
