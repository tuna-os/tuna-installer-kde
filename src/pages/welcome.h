#ifndef WELCOME_PAGE_H
#define WELCOME_PAGE_H

#include <QWidget>

class Wizard;

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(Wizard *wizard, QWidget *parent = nullptr);
    void prepare();

signals:
    void nextRequested();

private:
    Wizard *m_wizard;
};

#endif // WELCOME_PAGE_H
