// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kirigamiaddons.formcard as FC
import io.github.rfrench3.bazzite_updater

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
                text: (modelData !== root.currentTag) ? modelData : i18n("Current tag: ") + modelData

                enabled: modelData !== root.currentTag && AppState.allowCommands

                onClicked: {
                    RebaseModel.runRebase(`${root.url}/${root.name}:${modelData}`, exit_code => {
                        if (exit_code === 0)
                            showPassiveNotification(i18n("The rebase succeeded! Restart to apply changes."));
                        else
                            showPassiveNotification(i18n("The rebase failed. Check the console for more information."));
                    });
                }
            }
        }
    }
}
