// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
// import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FC

FC.FormCard {
    id: root
    required property string url
    required property string name
    required property list<string> features
    required property list<string> tags
    property string currentTag: ""

    FormDelegateCollapsible {
        text: root.features.join(", ")
        description: `${root.url}/${root.name}`

        Repeater {
            model: root.tags
            delegate: FC.FormButtonDelegate {
                required property string modelData
                text: (enabled) ? modelData : i18n("Current tag: ") + modelData

                enabled: modelData !== root.currentTag

                onClicked: {
                    console.log(`${root.url}/${root.name}:${modelData}`);
                }
            }
        }
    }
}
