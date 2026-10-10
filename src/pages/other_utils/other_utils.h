// SPDX-FileCopyrightText: 2025-2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <KLocalizedString>
#include <QClipboard>
#include <QFileInfo>
#include <QJSValue>
#include <QProcess>
#include <QQmlEngine>
#include <qcontainerfwd.h>
#include <qjsvalue.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "console.h"

using namespace Qt::Literals::StringLiterals;

namespace Gpu
{
Q_NAMESPACE
QML_ELEMENT

enum class Drivers {
    UNKNOWN = -1,
    BASE,
    NVIDIA,
    NVIDIA_OPEN,
    UNSUPPORTED
};

Q_ENUM_NS(Drivers)
}

struct osImage {
    Q_GADGET
    QML_VALUE_TYPE(osImage)

    Q_PROPERTY(QString name MEMBER m_imageName CONSTANT)
    Q_PROPERTY(QString vendor MEMBER m_imageVendor CONSTANT)
    Q_PROPERTY(QString ref MEMBER m_imageRef CONSTANT)
    Q_PROPERTY(QString tag MEMBER m_imageTag CONSTANT)
    Q_PROPERTY(QString branch MEMBER m_imageBranch CONSTANT)
    Q_PROPERTY(QString baseName MEMBER m_baseImageName CONSTANT)
    Q_PROPERTY(QString fedoraVersion MEMBER m_fedoraVersion CONSTANT)
    Q_PROPERTY(QString version MEMBER m_version CONSTANT)
    Q_PROPERTY(QString versionPretty MEMBER m_versionPretty CONSTANT)
    Q_PROPERTY(QVariantMap datePretty MEMBER m_datePretty CONSTANT)
    Q_PROPERTY(bool load_successful MEMBER m_isValid CONSTANT)

public:
    osImage() = default;
    static osImage fromJson(const QString &filePath);

    QString m_imageName;
    QString m_imageVendor;
    QString m_imageRef;
    QString m_imageTag;
    QString m_imageBranch;
    QString m_baseImageName;
    QString m_fedoraVersion;
    QString m_version;
    QString m_versionPretty;
    QVariantMap m_datePretty;
    bool m_isValid = false;
};

class OtherUtilsBackend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_DISABLE_COPY_MOVE(OtherUtilsBackend)
    OtherUtilsBackend(QObject *parent = nullptr);

public: // singleton methods
    static OtherUtilsBackend *instance()
    {
        static OtherUtilsBackend *s_instance = new OtherUtilsBackend();
        return s_instance;
    }

    static OtherUtilsBackend *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine)
        OtherUtilsBackend *instancePtr = instance();
        qmlEngine->setObjectOwnership(instancePtr, QQmlEngine::CppOwnership);
        return instancePtr;
    }

    Q_PROPERTY(osImage currentImage READ currentImage CONSTANT)
    Q_PROPERTY(Console::Model *consoleModel MEMBER m_console CONSTANT)

    Q_PROPERTY(Gpu::Drivers bestDriver READ gpuDrivers NOTIFY recommendedDriverChanged)

    Gpu::Drivers best_driver;

    osImage m_osImage_current;
    void setGpuDrivers();

    // Check for rpm-ostree layered packages
    QStringList m_layeredPackages;
    Q_PROPERTY(QStringList layeredPackages MEMBER m_layeredPackages NOTIFY layeredPackagesChanged)
    Q_SIGNAL void layeredPackagesChanged();
    void checkLayeredPackages();

public:
    osImage currentImage() const
    {
        return m_osImage_current;
    }
    Gpu::Drivers gpuDrivers() const
    {
        return best_driver;
    }

    Q_SIGNAL void recommendedDriverChanged();

    // ROLLBACK
    Q_INVOKABLE void rollbackImage(QJSValue callback);

    // // REBASE
    // Q_INVOKABLE void rebaseImage(const QString new_image, QJSValue callback);

    Console::Model *m_console;
};
inline auto otherUtilsBackend = OtherUtilsBackend::instance();
