#include "diskselection.h"
#include "../wizard.h"
#include "../recipe.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>
#include <QShortcut>
#include <QKeySequence>

DiskSelectionPage::DiskSelectionPage(Wizard *wizard, QWidget *parent)
    : QWidget(parent), m_wizard(wizard)
{
    setObjectName("disk");
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("<h2>Select Target Disk</h2>"));
    layout->addWidget(title);

    auto *desc = new QLabel(QStringLiteral(
        "Choose the disk where TunaOS will be installed. "
        "All data on the selected disk will be erased."
    ));
    desc->setWordWrap(true);
    layout->addWidget(desc);

    m_diskList = new QListWidget;
    m_diskList->setMinimumHeight(200);
    layout->addWidget(m_diskList);

    layout->addStretch();

    auto *btnNext = new QPushButton(QStringLiteral("Continue"));
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
        connect(sc, &QShortcut::activated, btnNext, [btnNext]() {
            if (btnNext->isEnabled()) {
                btnNext->animateClick();
            }
        });
    }
    btnNext->setFixedWidth(200);
    layout->addWidget(btnNext, 0, Qt::AlignRight);

    connect(btnNext, &QPushButton::clicked, this, [this]() {
        auto *item = m_diskList->currentItem();
        if (!item)
            return;
        QString dev = item->data(Qt::UserRole).toString();
        m_wizard->recipe().disk = dev;
        qDebug() << "Selected disk:" << dev;
        emit nextRequested();
    });
}

void DiskSelectionPage::prepare()
{
    m_diskList->clear();
    auto disks = discoverDisks();
    for (const auto &d : disks) {
        auto *item = new QListWidgetItem(diskDescription(d.toObject()));
        item->setData(Qt::UserRole, d.toObject()["name"].toString());
        m_diskList->addItem(item);
    }
    if (m_diskList->count() > 0)
        m_diskList->setCurrentRow(0);
}

QJsonArray DiskSelectionPage::discoverDisks() const
{
    QProcess proc;
    proc.start(QStringLiteral("lsblk"), {
        QStringLiteral("-J"), QStringLiteral("-o"),
        QStringLiteral("NAME,SIZE,TYPE,MODEL,TRAN")
    });
    proc.waitForFinished(5000);
    QByteArray out = proc.readAllStandardOutput();
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(out, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse lsblk output:" << err.errorString();
        return {};
    }
    auto blockdevices = doc.object()["blockdevices"].toArray();
    QJsonArray disks;
    for (const auto &dev : blockdevices) {
        auto obj = dev.toObject();
        if (obj["type"].toString() == "disk") {
            disks.append(obj);
        }
    }
    return disks;
}

QString DiskSelectionPage::diskDescription(const QJsonObject &disk) const
{
    QString name = disk["name"].toString();
    QString size = disk["size"].toString();
    QString model = disk["model"].toString();
    QString tran = disk["trans"].toString();

    if (tran == "nvme")
        tran = QStringLiteral("NVMe");
    else if (tran == "sata" || tran == "ata")
        tran = QStringLiteral("SATA");

    QString desc = QStringLiteral("/dev/%1  (%2)").arg(name, size);
    if (!model.isEmpty())
        desc += QStringLiteral(" — %1").arg(model);
    if (!tran.isEmpty())
        desc += QStringLiteral(" [%1]").arg(tran);
    return desc;
}
