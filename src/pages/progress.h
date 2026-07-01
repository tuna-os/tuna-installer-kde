#ifndef PROGRESS_PAGE_H
#define PROGRESS_PAGE_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QProcess>

class ProgressPage : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressPage(QWidget *parent = nullptr);
    void start(const QString &recipePath);
    void onError(const QString &message);
    void onFinished(int exitCode, const QString &output);

signals:
    void finished(int exitCode, const QString &output);

private:
    QPlainTextEdit *m_output;
    QProcess *m_process = nullptr;
    QString m_buffer;

    void appendLine(const QString &line);
};

#endif // PROGRESS_PAGE_H
