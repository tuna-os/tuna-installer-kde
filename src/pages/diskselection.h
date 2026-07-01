#ifndef DISK_SELECTION_PAGE_H
#define DISK_SELECTION_PAGE_H

#include <QWidget>
#include <QListWidget>
#include <QJsonArray>

class Wizard;

class DiskSelectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiskSelectionPage(Wizard *wizard, QWidget *parent = nullptr);
    void prepare();

signals:
    void nextRequested();

private:
    Wizard *m_wizard;
    QListWidget *m_diskList;

    QJsonArray discoverDisks() const;
    QString diskDescription(const QJsonObject &disk) const;
};

#endif // DISK_SELECTION_PAGE_H
