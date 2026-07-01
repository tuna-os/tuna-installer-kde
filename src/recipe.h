#ifndef RECIPE_H
#define RECIPE_H

#include <QJsonObject>
#include <QString>

struct Recipe {
    QString disk;
    QString filesystem = "xfs";
    bool btrfsSubvolumes = false;
    struct {
        QString type = "none";
        QString passphrase;
    } encryption;
    QString image;
    QString targetImgref;
    bool selinuxDisabled = true;
    QString hostname = "tunaos";

    QJsonObject toJson() const;
    static Recipe fromJson(const QJsonObject &obj);
    bool isValid() const;
    QString validationError() const;
};

#endif // RECIPE_H
