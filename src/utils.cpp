// SPDX-FileCopyrightText: 2025-2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "utils.h"
#include "user_settings.h"
#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qprocess.h>

#include <KColorScheme>
#include <KColorSchemeManager>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QIcon>

#include <QPalette>
#include <optional>

namespace Utils
{

void startProcess(QProcess *process, const QStringList &command)
{
    qInfo() << "> " << command.join(u" ");
    process->start(command.at(0), command.mid(1));
}

DbusListener::DbusListener(QObject *parent)
    : QObject(parent)
{
    // Set the theme initially
    initSetColorScheme();

    // Set the theme when it changes as the app is already open
    QDBusConnection::sessionBus().connect(u"org.freedesktop.portal.Desktop"_s,
                                          u"/org/freedesktop/portal/desktop"_s,
                                          u"org.freedesktop.portal.Settings"_s,
                                          u"SettingChanged"_s,
                                          this,
                                          SLOT(onPortalSettingChanged(QString, QString, QDBusVariant)));
}

static uint readPortalColorScheme()
{
    QDBusInterface portal(u"org.freedesktop.portal.Desktop"_s,
                          u"/org/freedesktop/portal/desktop"_s,
                          u"org.freedesktop.portal.Settings"_s,
                          QDBusConnection::sessionBus());

    if (!portal.isValid())
        return NO_SCHEME;

    QDBusReply<QDBusVariant> reply = portal.call(u"Read"_s, u"org.freedesktop.appearance"_s, u"color-scheme"_s);

    if (!reply.isValid())
        return NO_SCHEME;

    // org.freedesktop.portal.Settings returns a variant containing another variant (v -> v -> uint)
    return reply.value().variant().value<QDBusVariant>().variant().toUInt();
}

// The desktop's color scheme is stored in ~/.config/kdeglobals, which Game Mode sessions can still read.
// Returns nullopt if no scheme has been written there (e.g. non-KDE images).
static std::optional<bool> systemPrefersDark()
{
    const auto globals = KSharedConfig::openConfig(u"kdeglobals"_s, KConfig::SimpleConfig);
    if (!globals->hasGroup(u"Colors:Window"_s))
        return std::nullopt;

    const QPalette palette = KColorScheme::createApplicationPalette(globals);
    return palette.color(QPalette::Window).lightness() < 128;
}

void applyColorScheme(uint portalScheme)
{
    static bool s_overridden = false;

    const bool forceDark = UserSettings::instance()->forceDarkMode();

    if (KDE_SESSION) {
        // Plasma manages the scheme itself, only step in while the user override is on
        if (forceDark) {
            KColorSchemeManager::instance()->activateSchemeId(u"BreezeDark"_s);
            s_overridden = true;
        } else {
            // Game Mode runs save their scheme in this app's own config, and KColorSchemeManager restores it on the
            // next launch, so a Game Mode session would otherwise leave the desktop app stuck on BreezeDark.
            KConfigGroup ui(KSharedConfig::openConfig(), u"UiSettings"_s);
            if (ui.hasKey("ColorScheme")) {
                ui.deleteEntry("ColorScheme");
                ui.sync();
                s_overridden = true;
            }

            if (s_overridden) {
                // empty id = follow the system scheme
                KColorSchemeManager::instance()->activateSchemeId(QString());
                s_overridden = false;
            }
        }
        return;
    }

    bool dark = true;

    if (!forceDark) {
        if (GAMESCOPE_SESSION) {
            // Follow the desktop's theme in Game Mode too. If it can't be read, keep the old default of dark.
            if (const auto prefersDark = systemPrefersDark()) {
                dark = *prefersDark;
            } else {
                if (portalScheme == NO_SCHEME)
                    portalScheme = readPortalColorScheme();
                dark = (portalScheme == NO_SCHEME) ? true : (portalScheme == 1);
            }
        } else {
            if (portalScheme == NO_SCHEME)
                portalScheme = readPortalColorScheme();

            // Portal unavailable and nothing forcing a scheme: leave the default alone
            if (portalScheme == NO_SCHEME)
                return;

            dark = (portalScheme == 1);
        }
    }

    KColorSchemeManager::instance()->activateSchemeId(dark ? u"BreezeDark"_s : u"BreezeLight"_s);
    QIcon::setThemeName(u"breeze"_s);
}

void DbusListener::initSetColorScheme()
{
    applyColorScheme();
    prevColorScheme = readPortalColorScheme();
}

void DbusListener::onPortalSettingChanged(const QString &nameSpace, const QString &key, const QDBusVariant &value)
{
    // Filter for the color-scheme setting
    if (nameSpace == u"org.freedesktop.appearance"_s && key == u"color-scheme"_s) {
        const auto colorScheme = value.variant().toUInt();
        if (colorScheme == prevColorScheme)
            return;

        prevColorScheme = colorScheme;
        applyColorScheme(colorScheme);
    }
}
}

namespace SingletonInternals
{

void AppState::setUpdateRunning(bool running)
{
    m_updateRunning = running;
    sendUpdateSignals(UPDATE);
}

void AppState::setRollbackRunning(bool running)
{
    m_rollbackRunning = running;
    sendUpdateSignals(ROLLBACK);
}

void AppState::setRebaseRunning(bool running)
{
    m_rebaseRunning = running;
    sendUpdateSignals(REBASE);
}

void AppState::setCommandSucceeded(bool succeeded)
{
    m_commandSucceeded = succeeded;
    Q_EMIT commandSucceededChanged();
    Q_EMIT allowCommandsChanged();
}

void AppState::sendUpdateSignals(sendSignals signal)
{
    Q_EMIT commandRunningChanged();
    Q_EMIT allowCommandsChanged();
    switch (signal) {
    case UPDATE:
        Q_EMIT updateRunningChanged();
        break;
    case ROLLBACK:
        Q_EMIT rollbackRunningChanged();
        break;
    case REBASE:
        Q_EMIT rebaseRunningChanged();
        break;
    }
}

void AppState::rebootSystem(QJSValue callback)
{
    QProcess reboot;
    Utils::startProcess(&reboot, {u"systemctl"_s, u"reboot"_s});
    reboot.waitForFinished();
    callback.call({1});
}

void AppState::appendCommandError(QByteArray error)
{
    cmd_out.append(error);
    Q_EMIT commandErrorChanged();
}

}
