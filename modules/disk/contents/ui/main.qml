// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import org.tunaos.installer
import org.tunaos.installer.components as TunaComponents

TunaComponents.SetupModule {
    id: root

    // No disk, no install: Recipe::validationError() rejects an empty disk.
    nextEnabled: InstallerController.disk.length > 0

    // Called by the wizard whenever this step becomes current, so hotplugged
    // media show up. Replaces DiskSelectionPage::prepare().
    function onPageActivated(): void {
        disks.refresh();
    }

    DiskModel {
        id: disks
    }

    contentItem: ScrollView {
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: -1

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            Kirigami.InlineMessage {
                type: Kirigami.MessageType.Warning
                visible: true
                position: Kirigami.InlineMessage.Position.Header
                text: "Everything on the selected disk will be erased."

                Layout.fillWidth: true
                Layout.maximumWidth: root.cardWidth
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Kirigami.Units.largeSpacing
            }

            FormCard.FormCard {
                maximumWidth: root.cardWidth

                Layout.alignment: Qt.AlignHCenter

                Repeater {
                    model: disks

                    delegate: FormCard.FormRadioDelegate {
                        required property string device
                        required property string subtitle

                        text: device
                        description: subtitle
                        checked: InstallerController.disk === device

                        onToggled: if (checked) {
                            InstallerController.disk = device;
                        }
                    }
                }
            }

            FormCard.FormCard {
                maximumWidth: root.cardWidth
                visible: disks.count === 0

                Layout.alignment: Qt.AlignHCenter

                FormCard.FormTextDelegate {
                    text: "No disks found"
                    description: "lsblk reported no block devices of type “disk”."
                }
            }
        }
    }
}
