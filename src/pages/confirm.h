#ifndef CONFIRM_PAGE_H
#define CONFIRM_PAGE_H

#include <QWidget>
#include <QLabel>

class Wizard;

class ConfirmPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConfirmPage(Wizard *wizard, QWidget *parent = nullptr);
    void prepare();

signals:
    void backRequested();
    void installRequested();

private:
    Wizard *m_wizard;
    QLabel *m_diskLabel;
    QLabel *m_fsLabel;
    QLabel *m_encLabel;
    QLabel *m_hostnameLabel;
    QLabel *m_imageLabel;
};

#endif // CONFIRM_PAGE_H
