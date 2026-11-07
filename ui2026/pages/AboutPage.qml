/*
    Copyright (C) 2025 Rohith Namboothiri
    Modification Copyright (C) 2026 M8NLR

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../components"
import "../theme"

Page {
    id: page
    title: qsTr("About")
    padding: 0

    required property var droidstarRef
    property Component headerActions: null
    readonly property bool androidBuild: Qt.platform.os === "android"

    Tokens { id: t }

    background: Rectangle { color: t.bg }

    readonly property string buildVer: page.droidstarRef ? page.droidstarRef.get_software_build() : ""
    readonly property string updatePhase: page.droidstarRef ? (page.droidstarRef.appUpdatePhase || "") : ""
    readonly property string updateAvailable: page.droidstarRef ? (page.droidstarRef.appUpdateAvailable || "") : ""
    readonly property bool updateNewer: page.droidstarRef ? !!page.droidstarRef.appUpdateNewer : false
    readonly property int updatePercent: page.droidstarRef ? page.droidstarRef.appUpdatePercent : -1
    readonly property string updateDetail: page.droidstarRef ? (page.droidstarRef.appUpdateDetail || "") : ""

    readonly property string availableLabel: {
        if (page.updatePhase === "checking")
            return qsTr("Checking…")
        if (page.updatePhase === "failed")
            return qsTr("Check Failed")
        if (page.updatePhase === "downloading")
            return page.updateAvailable !== "" ? page.updateAvailable : qsTr("Downloading…")
        if (page.updateNewer && page.updateAvailable !== "")
            return page.updateAvailable
        if (page.updatePhase === "ready")
            return qsTr("None")
        return qsTr("Checking…")
    }

    readonly property string updateButtonText: {
        if (page.updatePhase === "downloading") {
            if (page.updatePercent >= 0)
                return qsTr("Update %1%").arg(page.updatePercent)
            return qsTr("Update…")
        }
        return qsTr("Update")
    }

    Component.onCompleted: {
        if (page.androidBuild && page.droidstarRef)
            page.droidstarRef.check_app_update()
    }

    Connections {
        target: page.droidstarRef
        enabled: page.androidBuild && !!page.droidstarRef
        function onConfig_transfer(result, detail) {
            settingsDetail.text = detail || ""
            settingsDetail.warning = (result === "failed")
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 40
        boundsBehavior: Flickable.DragOverBounds
        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: contentColumn
            x: Math.max(10, (flick.width - width) / 2)
            y: 12
            width: Math.min(flick.width - 20, 640)
            spacing: 12

            AppCard {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Image {
                        id: aboutWhiteLogo
                        readonly property real targetW: Math.min(Math.max(180, parent.width - 24), 260)
                        source: page.androidBuild
                                ? "qrc:/DroidStar/images/droidstar_android_logo_white.png"
                                : "qrc:/DroidStar/images/droidstar_logo_white.png"
                        sourceSize.width: 684
                        sourceSize.height: 228
                        Layout.preferredWidth: targetW
                        Layout.preferredHeight: Math.round(targetW * 341 / 1024)
                        Layout.topMargin: 4
                        Layout.alignment: Qt.AlignHCenter
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        asynchronous: true
                        onStatusChanged: {
                            if (status === Image.Error) {
                                source = page.androidBuild
                                         ? Qt.resolvedUrl("../../images/droidstar_android_logo_white.png")
                                         : Qt.resolvedUrl("../../images/droidstar_logo_white.png")
                            }
                        }
                    }

                    Label {
                        text: {
                            if (page.buildVer === "")
                                return page.androidBuild ? qsTr("Android build") : qsTr("iOS build")
                            return page.androidBuild
                                   ? qsTr("Android build - V%1").arg(page.buildVer)
                                   : qsTr("iOS build - V%1").arg(page.buildVer)
                        }
                        font.pixelSize: t.fsBody
                        color: t.text
                        opacity: 0.9
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle { Layout.fillWidth: true; Layout.topMargin: 4; Layout.preferredHeight: 1; color: t.stroke }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 4

                        Label { text: qsTr("Architecture"); color: t.textMuted; font.pixelSize: t.fsSmall }
                        Label { Layout.fillWidth: true; text: page.droidstarRef ? page.droidstarRef.get_arch() : ""; color: t.text; font.pixelSize: t.fsSmall; elide: Text.ElideRight }
                        Label { text: qsTr("Build ABI"); color: t.textMuted; font.pixelSize: t.fsSmall }
                        Label { Layout.fillWidth: true; text: page.droidstarRef ? page.droidstarRef.get_build_abi() : ""; color: t.text; font.pixelSize: t.fsSmall; elide: Text.ElideRight }
                        Label { text: qsTr("Software"); color: t.textMuted; font.pixelSize: t.fsSmall }
                        Label { Layout.fillWidth: true; text: page.buildVer; color: t.text; font.pixelSize: t.fsSmall; elide: Text.ElideRight }
                    }
                }
            }

            AppCard {
                visible: page.androidBuild
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label { text: qsTr("Updates"); font.pixelSize: 15; font.bold: true; color: t.text }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 6

                        Label { text: qsTr("Current version"); color: t.textMuted; font.pixelSize: t.fsSmall }
                        Label { Layout.fillWidth: true; text: page.buildVer; color: t.text; font.pixelSize: t.fsSmall; elide: Text.ElideRight }
                        Label { text: qsTr("Available update"); color: t.textMuted; font.pixelSize: t.fsSmall }
                        Label { Layout.fillWidth: true; text: page.availableLabel; color: t.text; font.pixelSize: t.fsSmall; elide: Text.ElideRight }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: page.updateDetail !== ""
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: t.warning
                        text: page.updateDetail
                    }

                    AppButton {
                        Layout.fillWidth: true
                        text: page.updateButtonText
                        glyph: "\uf019"
                        tone: page.updateNewer && page.updatePhase !== "downloading" ? "primary" : "neutral"
                        enabled: page.updateNewer && page.updatePhase !== "downloading"
                        onClicked: if (page.droidstarRef) page.droidstarRef.install_app_update()
                    }
                }
            }

            AppCard {
                visible: page.androidBuild
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label { text: qsTr("Settings"); font.pixelSize: 15; font.bold: true; color: t.text }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: t.textMuted
                        text: qsTr("Export writes a copy you can keep. The settings in the app stay where they are. Restore is the only button that puts a file back, and it keeps any settings this version already has. The file includes your hotspot passwords.")
                    }

                    Label {
                        id: settingsDetail
                        Layout.fillWidth: true
                        visible: text !== ""
                        property bool warning: false
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: warning ? t.warning : t.textMuted
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            Layout.fillWidth: true
                            text: qsTr("Export settings")
                            glyph: "\uf0c7"
                            tone: "primary"
                            onClicked: if (page.droidstarRef) page.droidstarRef.save_config()
                        }
                        AppButton {
                            Layout.fillWidth: true
                            text: qsTr("Restore settings")
                            glyph: "\uf0e2"
                            onClicked: if (page.droidstarRef) page.droidstarRef.restore_config()
                        }
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label { text: qsTr("Copyright & License"); font.pixelSize: 15; font.bold: true; color: t.text }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: t.text
                        opacity: 0.9
                        lineHeight: 1.3
                        text: page.androidBuild
                              ? qsTr("Original Copyright (C) 2019-2021 Doug McLain AD8DP\n\n" +
                                     "Upstream Copyright (C) 2024 Rohith Namboothiri VU3LVO\n\n" +
                                     "Modification Copyright (C) 2026 M8NLR\n\n" +
                                     "This program is free software: you can redistribute it and/or modify " +
                                     "it under the terms of the GNU General Public License as published by " +
                                     "the Free Software Foundation, either version 3 of the License, or " +
                                     "(at your option) any later version.\n\n" +
                                     "This program is distributed in the hope that it will be useful, " +
                                     "but WITHOUT ANY WARRANTY; without even the implied warranty of " +
                                     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the " +
                                     "GNU General Public License for more details.\n\n" +
                                     "You should have received a copy of the GNU General Public License " +
                                     "along with this program. If not, see https://www.gnu.org/licenses/.")
                              : qsTr("Original Copyright (C) 2019-2021 Doug McLain AD8DP\n\n" +
                                     "Modification Copyright (C) 2024 Rohith Namboothiri VU3LVO\n\n" +
                                     "This program is free software: you can redistribute it and/or modify " +
                                     "it under the terms of the GNU General Public License as published by " +
                                     "the Free Software Foundation, either version 3 of the License, or " +
                                     "(at your option) any later version.\n\n" +
                                     "This program is distributed in the hope that it will be useful, " +
                                     "but WITHOUT ANY WARRANTY; without even the implied warranty of " +
                                     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the " +
                                     "GNU General Public License for more details.\n\n" +
                                     "You should have received a copy of the GNU General Public License " +
                                     "along with this program. If not, see https://www.gnu.org/licenses/.")
                    }

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("View GPL License")
                        glyph: "\uf08e"
                        onClicked: Qt.openUrlExternally("https://www.gnu.org/licenses/")
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label { text: qsTr("Important Notice"); font.pixelSize: 15; font.bold: true; color: t.warning }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: t.text
                        opacity: 0.9
                        lineHeight: 1.3
                        text: page.androidBuild
                              ? qsTr("This Android version is made by M8NLR and is available for public release. It tracks the upstream DroidStar work of Rohith Namboothiri VU3LVO, based on the original app by Doug McLain AD8DP.")
                              : qsTr("This customized iOS version, built and distributed by VU3LVO, is specifically designed for use by a select group and is not intended for public use at the moment.")
                    }
                }
            }

            AppCard {
                visible: !page.androidBuild
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    Label { text: qsTr("Support the Project"); font.pixelSize: 15; font.bold: true; color: t.text }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: t.fsSmall
                        color: t.textMuted
                        text: qsTr("If you find this app useful, please consider supporting the development.")
                    }

                    DonateButtons {
                        Layout.fillWidth: true
                        buttonHeight: 52
                        fontSize: 15
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label { text: qsTr("Project Links"); font.pixelSize: 15; font.bold: true; color: t.text }

                    AppButton {
                        visible: !page.androidBuild
                        Layout.fillWidth: true
                        text: qsTr("Visit Official Website")
                        glyph: "\uf0ac"
                        onClicked: Qt.openUrlExternally("https://vu3lvo.com/droidstar")
                    }

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("GitHub Project Page")
                        glyph: "\uf09b"
                        onClicked: Qt.openUrlExternally(page.androidBuild
                                                        ? "https://github.com/sappsys/DroidStar-Android"
                                                        : "https://github.com/rohithzmoi/Droidstar-DMR")
                    }
                }
            }

            Item { Layout.preferredHeight: 20 }
        }
    }
}
