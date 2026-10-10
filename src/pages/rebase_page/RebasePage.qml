// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FC

import io.github.rfrench3.controllable as GP

// TODO:
// - improve controller support for selections. It can be navigated, but left/right should move to buttons to the left/right
// - finish frontend
// - begin/finish backend
// - move all system image formcards to FCSystemImage

AppPage {
    id: page

    title: GP.Labels.east + GP.Labels.spacer_large + i18n("Rebase Helper")

    actions: [
        Kirigami.Action {
            id: toggleConsole
            text: "Toggle Console" + GP.Labels.spacer + GP.Labels.north
            shortcut: "F12"
            onTriggered: consoleDrawer.drawerOpen = !consoleDrawer.drawerOpen
        }
    ]

    GP.PageNavigation {
        targetScrollbar: page.scrollBar
        active: !globalDrawer.drawerOpen && !consoleDrawer.drawerOpen
    }
    drawer: consoleDrawer

    AsyncLoader {
        sourceComponent: PageContentLayout {
            id: content

            FC.FormHeader {
                title: i18n("Current System Image")
            }

            FCSystemImage {
                id: currentImage
                url: OtherUtilsBackend.currentImage.ref
                name: OtherUtilsBackend.currentImage.name
                tags: [OtherUtilsBackend.currentImage.branch]
                isCurrent: true
                currentTag: OtherUtilsBackend.currentImage.branch

                features: RebaseModel.currentFeatures

                // If the given image's features are compatible with the current one, return true.
                // For example, this would return false when the current image has kde and the given one has gnome.
                function compatibleImage(features: list<string>): bool {
                    let currentFeatures = currentImage.features || [];

                    if (features.includes("gnome"))
                        return !currentFeatures.includes("kde");

                    if (features.includes("kde"))
                        return !currentFeatures.includes("gnome");

                    return true;
                }
            }

            FC.FormHeader {
                title: i18n("Available System Images")
            }

            property var allFeatures: {
                let set = new Set();
                let targets = AppConfig.rebaseTargets || [];
                for (let t of targets) {
                    for (let img of (t.images || [])) {
                        for (let f of (img.features || []))
                            set.add(f);
                    }
                }
                return Array.from(set).sort();
            }

            FC.FormHeader {
                title: i18n("Filter by Features")
                visible: content.allFeatures.length > 0
            }
            FC.FormCard {
                visible: content.allFeatures.length > 0

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3

                    Repeater {
                        model: content.allFeatures
                        delegate: Button {
                            required property string modelData
                            text: modelData
                            Layout.fillWidth: true
                            checkable: true
                            flat: true

                            enabled: currentImage.compatibleImage([modelData])

                            onCheckedChanged: {
                                if (checked) {
                                    RebaseModel.features = RebaseModel.features.concat([modelData]);
                                } else {
                                    RebaseModel.features = RebaseModel.features.filter(f => f !== modelData);
                                }
                            }
                        }
                    }
                }
            }

            FC.FormHeader {
                title: i18n("Results (%1)", RebaseModel.filtered.length)
            }

            Repeater {
                model: RebaseModel.filtered

                delegate: FCSystemImage {
                    Layout.topMargin: Kirigami.Units.mediumSpacing
                }
            }
        }
    }

    ConsoleDrawer {
        id: consoleDrawer
        model: null
    }
}
