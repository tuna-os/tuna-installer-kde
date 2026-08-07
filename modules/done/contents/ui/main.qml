// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.tunaos.installer
import org.tunaos.installer.components as TunaComponents

TunaComponents.SetupModule {
    id: root

    nextEnabled: true

    contentItem: ScrollView {
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: -1

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing

            // Vertical centring without anchors.centerIn: inside a ScrollView
            // the content item's size is not reliable to anchor against.
            Item {
                Layout.fillHeight: true
            }

            Kirigami.Icon {
                source: InstallerController.succeeded ? "checkmark" : "dialog-error"
                implicitWidth: Kirigami.Units.iconSizes.enormous
                implicitHeight: Kirigami.Units.iconSizes.enormous

                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Kirigami.Units.gridUnit
            }

            Kirigami.Heading {
                text: InstallerController.succeeded ? "Installation complete" : "Installation failed"
                level: 1
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap

                Layout.fillWidth: true
            }

            Label {
                text: InstallerController.succeeded
                    ? "TunaOS has been installed. Remove the installation media and restart to boot into your new system."
                    : "The installation did not finish. The log above has the details — exit code "
                      + InstallerController.exitCode + "."
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap

                Layout.fillWidth: true
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
