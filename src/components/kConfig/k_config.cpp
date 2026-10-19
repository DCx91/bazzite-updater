// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "k_config.h"
#include "data.h"
#include <cassert>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <qcontainerfwd.h>
#include <qdir.h>
#include <qevent.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlogging.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtenvironmentvariables.h>
#include <qvariant.h>

AppConfig::AppConfig()
{
    QFile rebaseFile(findConfigFile(u"bazzite-updater/rebase-targets.json"_s));
    if (!rebaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open rebase-targets.json";
    } else {
        auto file = rebaseFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(file);
        rebaseTargets = doc.array().toVariantList();
        rebaseFile.close();
    }
}

namespace SingletonInternals
{

ConfigIni::ConfigIni()
{
    auto filepath = findConfigFile(u"bazzite-updater/config.ini"_s);
    auto base_path = QFileInfo(filepath).absolutePath();

    KSharedConfigPtr config = KSharedConfig::openConfig(filepath, KConfig::SimpleConfig);

    for (const QString &group : config->groupList()) {
        auto map = config->entryMap(group);

        QVariantMap groupMap;

        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            const QString key = it.key();
            QString val = it.value();

            // convert relative paths to absolute form
            if (val.startsWith(u"./"_s))
                val = base_path + val.mid(1);

            groupMap.insert(key, val);
        }

        ini.insert(group, groupMap);
    }
}

QString VarMapPlus::getValue(const QString &group, const QString &key, const QString &fallback)
{
    if (!configIni.contains(group))
        return fallback;

    auto groupMap = configIni.value(group).toMap();

    if (!groupMap.contains(key))
        return fallback;

    return groupMap.value(key).toString();
}

}
