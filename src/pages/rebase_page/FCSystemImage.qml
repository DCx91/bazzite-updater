// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import org.kde.kirigamiaddons.formcard as FC
import io.github.rfrench3.bazzite_updater

FC.FormCard {
    id: root
    required property string url
    required property string name
    required property list<string> features
    required property list<string> tags
    property string currentTag: ""

    signal tagClicked(string tag)

    Component {
        id: buttonComp
        FC.FormButtonDelegate {
            required property string modelData

            text: (modelData !== root.currentTag) ? modelData : i18n("Current tag: ") + modelData

            enabled: modelData !== root.currentTag && AppState.allowCommands

            onClicked: root.tagClicked(modelData)
        }
    }

    Component {
        id: separatorComp
        FormDelegateSeparatorFixed {}
    }

    FormDelegateCollapsible {
        id: layout
        text: root.features.join(", ")
        description: `${root.url}/${root.name}`

        Component.onCompleted: {
            for (let i = 0; i < root.tags.length; i++) {
                let tag = root.tags[i];

                let button = buttonComp.createObject(null, {
                    modelData: tag
                });

                layout.childElements = [...layout.childElements, button];

                if (i < root.tags.length - 1) {
                    let sep = separatorComp.createObject(null);
                    layout.childElements = [...layout.childElements, sep];
                }
            }
        }
    }
}
