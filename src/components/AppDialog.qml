// SPDX-FileCopyrightText: 2025-2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import io.github.rfrench3.bazzite_updater

Kirigami.PromptDialog {
    id: root

    property string titleStart: ""
    readonly property int __pendingCount: Math.max(0, AppDialogManager.dialogs.length - 1)

    title: __pendingCount > 0 ? `${titleStart} (${__pendingCount})` : titleStart

    maximumWidth: Kirigami.Units.gridUnit * 20

    Connections {
        target: root
        function onClosed() {
            AppDialogManager.remove(root);
        }
    }

    Component.onDestruction: AppDialogManager.remove(root)
}
