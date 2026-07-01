#include "recipe.h"
#include <QJsonDocument>

QJsonObject Recipe::toJson() const
{
    QJsonObject obj;
    obj["disk"] = disk;
    obj["filesystem"] = filesystem;
    obj["btrfsSubvolumes"] = btrfsSubvolumes;
    QJsonObject enc;
    enc["type"] = encryption.type;
    enc["passphrase"] = encryption.passphrase;
    obj["encryption"] = enc;
    obj["image"] = image;
    obj["targetImgref"] = targetImgref;
    obj["selinuxDisabled"] = selinuxDisabled;
    obj["hostname"] = hostname;
    return obj;
}

Recipe Recipe::fromJson(const QJsonObject &obj)
{
    Recipe r;
    r.disk = obj["disk"].toString();
    r.filesystem = obj["filesystem"].toString("xfs");
    r.btrfsSubvolumes = obj["btrfsSubvolumes"].toBool(false);
    auto enc = obj["encryption"].toObject();
    r.encryption.type = enc["type"].toString("none");
    r.encryption.passphrase = enc["passphrase"].toString();
    r.image = obj["image"].toString();
    r.targetImgref = obj["targetImgref"].toString();
    r.selinuxDisabled = obj["selinuxDisabled"].toBool(true);
    r.hostname = obj["hostname"].toString("tunaos");
    return r;
}

bool Recipe::isValid() const
{
    return validationError().isEmpty();
}

QString Recipe::validationError() const
{
    if (disk.isEmpty())
        return QStringLiteral("No disk selected");
    if (image.isEmpty())
        return QStringLiteral("No OS image specified");
    if (hostname.isEmpty())
        return QStringLiteral("Hostname is required");
    if (encryption.type == "luks" && encryption.passphrase.isEmpty())
        return QStringLiteral("LUKS passphrase is required");
    return {};
}
