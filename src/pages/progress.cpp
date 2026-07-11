#include "progress.h"
#include "../offline.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QJsonDocument>

ProgressPage::ProgressPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("progress");
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("<h2>Installing...</h2>"));
    layout->addWidget(title);

    m_output = new QPlainTextEdit;
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(1000);
    layout->addWidget(m_output);
}

void ProgressPage::start(const QString &recipePath)
{
    m_output->clear();
    m_buffer.clear();
    appendLine(QStringLiteral("Starting installation...\n"));

    // pkexec /app/bin/fisherman in Flatpak, sudo /usr/local/bin/fisherman otherwise.
    QStringList cmd = offline::fishermanCommand();
    cmd << recipePath;
    m_process = new QProcess(this);
    m_process->setProgram(cmd.takeFirst());
    m_process->setArguments(cmd);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        m_buffer += QString::fromUtf8(m_process->readAllStandardOutput());
        // Update output periodically or on newlines
        QStringList lines = m_buffer.split('\n');
        if (lines.size() > 1) {
            for (int i = 0; i < lines.size() - 1; ++i) {
                appendLine(lines[i]);
            }
            m_buffer = lines.last();
        }
    });

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        m_buffer += QString::fromUtf8(m_process->readAllStandardError());
        QStringList lines = m_buffer.split('\n');
        if (lines.size() > 1) {
            for (int i = 0; i < lines.size() - 1; ++i) {
                appendLine(QStringLiteral("[stderr] ") + lines[i]);
            }
            m_buffer = lines.last();
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        if (!m_buffer.isEmpty())
            appendLine(m_buffer);
        QString output = m_output->toPlainText();
        if (status == QProcess::CrashExit) {
            appendLine(QStringLiteral("\nERROR: fisherman crashed"));
            emit finished(1, output);
        } else {
            if (exitCode == 0)
                appendLine(QStringLiteral("\n✓ Installation complete!"));
            else
                appendLine(QStringLiteral("\n✗ Installation failed (exit code %1)").arg(exitCode));
            emit finished(exitCode, output);
        }
        m_process->deleteLater();
        m_process = nullptr;
    });

    m_process->start();
}

void ProgressPage::onError(const QString &message)
{
    appendLine(QStringLiteral("\nERROR: %1").arg(message));
    emit finished(1, message);
}

void ProgressPage::onFinished(int exitCode, const QString &output)
{
    if (exitCode == 0)
        appendLine(QStringLiteral("\n✓ Installation complete!"));
    else
        appendLine(QStringLiteral("\n✗ Installation failed (exit code %1)").arg(exitCode));
    emit finished(exitCode, output);
}

void ProgressPage::appendLine(const QString &line)
{
    m_output->appendPlainText(line);
    // Auto-scroll to bottom
    auto cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_output->setTextCursor(cursor);
}
