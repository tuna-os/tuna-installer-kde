#include "productname.h"
#include "recipe.h"

#include <QJsonArray>
#include <QTest>

class BackendTest : public QObject
{
    Q_OBJECT

private slots:
    void recipeDefaults();
    void recipeJsonOmitsOptionalValues();
    void recipeJsonIncludesConfiguredValues();
    void recipeValidation_data();
    void recipeValidation();
    void productNameParsing_data();
    void productNameParsing();
};

void BackendTest::recipeDefaults()
{
    const Recipe recipe;

    QCOMPARE(recipe.filesystem, QStringLiteral("xfs"));
    QCOMPARE(recipe.encryption.type, QStringLiteral("none"));
    QCOMPARE(recipe.distroID, QStringLiteral("tunaos"));
    QCOMPARE(recipe.hostname, QStringLiteral("tunaos"));
    QVERIFY(recipe.selinuxDisabled);
    QVERIFY(!recipe.liveMode);
}

void BackendTest::recipeJsonOmitsOptionalValues()
{
    const QJsonObject json = Recipe{}.toJson();

    QVERIFY(!json.contains(QStringLiteral("image")));
    QVERIFY(!json.contains(QStringLiteral("targetImgref")));
    QVERIFY(!json.contains(QStringLiteral("bootloader")));
    QVERIFY(!json.contains(QStringLiteral("composeFsBackend")));
    QVERIFY(!json.contains(QStringLiteral("flatpaks")));
    QVERIFY(!json.contains(QStringLiteral("additionalImageStores")));
    QCOMPARE(json.value(QStringLiteral("encryption")).toObject()
                 .value(QStringLiteral("type")).toString(),
             QStringLiteral("none"));
}

void BackendTest::recipeJsonIncludesConfiguredValues()
{
    Recipe recipe;
    recipe.disk = QStringLiteral("/dev/nvme0n1");
    recipe.image = QStringLiteral("ghcr.io/tuna-os/tunaos:latest");
    recipe.targetImgref = QStringLiteral("ghcr.io/tuna-os/tunaos:stable");
    recipe.bootloader = QStringLiteral("systemd");
    recipe.composeFsBackend = true;
    recipe.flatpaks = {QStringLiteral("org.mozilla.firefox")};
    recipe.additionalImageStores = {QStringLiteral("/run/media/oci")};
    recipe.encryption.type = QStringLiteral("luks-passphrase");
    recipe.encryption.passphrase = QStringLiteral("secret");

    const QJsonObject json = recipe.toJson();
    QCOMPARE(json.value(QStringLiteral("disk")).toString(), recipe.disk);
    QCOMPARE(json.value(QStringLiteral("image")).toString(), recipe.image);
    QCOMPARE(json.value(QStringLiteral("targetImgref")).toString(), recipe.targetImgref);
    QCOMPARE(json.value(QStringLiteral("bootloader")).toString(), recipe.bootloader);
    QVERIFY(json.value(QStringLiteral("composeFsBackend")).toBool());
    QCOMPARE(json.value(QStringLiteral("flatpaks")).toArray().first().toString(),
             recipe.flatpaks.first());
    QCOMPARE(json.value(QStringLiteral("additionalImageStores")).toArray().first().toString(),
             recipe.additionalImageStores.first());
    QCOMPARE(json.value(QStringLiteral("encryption")).toObject()
                 .value(QStringLiteral("passphrase")).toString(),
             recipe.encryption.passphrase);
}

void BackendTest::recipeValidation_data()
{
    QTest::addColumn<QString>("disk");
    QTest::addColumn<QString>("image");
    QTest::addColumn<bool>("liveMode");
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<QString>("encryptionType");
    QTest::addColumn<QString>("passphrase");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("valid-image") << "/dev/vda" << "example/image" << false << "tunaos" << "none" << "" << "";
    QTest::newRow("valid-live") << "/dev/vda" << "" << true << "tunaos" << "none" << "" << "";
    QTest::newRow("missing-disk") << "" << "example/image" << false << "tunaos" << "none" << "" << "No disk selected";
    QTest::newRow("missing-image") << "/dev/vda" << "" << false << "tunaos" << "none" << "" << "No OS image specified";
    QTest::newRow("missing-hostname") << "/dev/vda" << "example/image" << false << "" << "none" << "" << "Hostname is required";
    QTest::newRow("unknown-encryption") << "/dev/vda" << "example/image" << false << "tunaos" << "plain" << "" << "Unknown encryption type: plain";
    QTest::newRow("missing-passphrase") << "/dev/vda" << "example/image" << false << "tunaos" << "luks-passphrase" << "" << "Encryption passphrase is required";
    QTest::newRow("valid-passphrase") << "/dev/vda" << "example/image" << false << "tunaos" << "luks-passphrase" << "secret" << "";
}

void BackendTest::recipeValidation()
{
    QFETCH(QString, disk);
    QFETCH(QString, image);
    QFETCH(bool, liveMode);
    QFETCH(QString, hostname);
    QFETCH(QString, encryptionType);
    QFETCH(QString, passphrase);
    QFETCH(QString, expectedError);

    Recipe recipe;
    recipe.disk = disk;
    recipe.image = image;
    recipe.liveMode = liveMode;
    recipe.hostname = hostname;
    recipe.encryption.type = encryptionType;
    recipe.encryption.passphrase = passphrase;

    QCOMPARE(recipe.validationError(), expectedError);
    QCOMPARE(recipe.isValid(), expectedError.isEmpty());
}

void BackendTest::productNameParsing_data()
{
    QTest::addColumn<QString>("osRelease");
    QTest::addColumn<QString>("expected");

    QTest::newRow("double-quoted") << "NAME=TunaOS\nPRETTY_NAME=\"TunaOS KDE\"\n" << "TunaOS KDE";
    QTest::newRow("single-quoted") << "PRETTY_NAME='TunaOS KDE'\n" << "TunaOS KDE";
    QTest::newRow("unquoted") << "PRETTY_NAME=TunaOS\n" << "TunaOS";
    QTest::newRow("trimmed") << "  PRETTY_NAME=  \"TunaOS\"  \n" << "TunaOS";
    QTest::newRow("empty") << "PRETTY_NAME=\"\"\n" << "";
    QTest::newRow("absent") << "NAME=TunaOS\nID=tunaos\n" << "";
}

void BackendTest::productNameParsing()
{
    QFETCH(QString, osRelease);
    QFETCH(QString, expected);

    QCOMPARE(product::prettyNameFrom(osRelease), expected);
}

QTEST_APPLESS_MAIN(BackendTest)

#include "test_backend.moc"
