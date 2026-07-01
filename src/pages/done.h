#ifndef DONE_PAGE_H
#define DONE_PAGE_H

#include <QWidget>
#include <QLabel>

class DonePage : public QWidget
{
    Q_OBJECT

public:
    explicit DonePage(QWidget *parent = nullptr);
    void setResult(int exitCode, const QString &output);

signals:
    void quitRequested();

private:
    QLabel *m_icon;
    QLabel *m_title;
    QLabel *m_detail;
};

#endif // DONE_PAGE_H
