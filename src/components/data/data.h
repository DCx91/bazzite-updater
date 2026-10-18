#pragma once

#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qjsengine.h>
#include <qjsonobject.h>
#include <qjsvalue.h>
#include <qlist.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include <QtConcurrent>

using namespace Qt::StringLiterals;

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

struct GpuDrivers {
    Q_GADGET;
    QML_VALUE_TYPE(gpu);

public:
    Gpu::Drivers best = Gpu::Drivers::UNKNOWN;
    Q_PROPERTY(Gpu::Drivers best READ getBest CONSTANT)
    Gpu::Drivers getBest() const
    {
        return best;
    }

    static GpuDrivers load();
};

struct OstreeDeployment {
    Q_GADGET;
    QML_VALUE_TYPE(ostreeDeployment);

public:
    QStringList layeredPackages;
    QStringList getLayeredPackages() const
    {
        return layeredPackages;
    }
    bool isCurrent;
    bool getIsCurrent() const
    {
        return isCurrent;
    }
    bool isStaged;
    bool getIsStaged() const
    {
        return isStaged;
    }
    Q_PROPERTY(QStringList layeredPackages READ getLayeredPackages CONSTANT)
    Q_PROPERTY(bool isCurrent READ getIsCurrent CONSTANT)
    Q_PROPERTY(bool isStaged READ getIsStaged CONSTANT)
};

struct Ostree {
    Q_GADGET;
    QML_VALUE_TYPE(ostree);

public:
    QList<OstreeDeployment> deployments;
    Q_PROPERTY(QList<OstreeDeployment> deployments READ getDeployments CONSTANT)
    const QList<OstreeDeployment> &getDeployments() const
    {
        return deployments;
    }
    Q_INVOKABLE bool onLatestDeployment() const
    {
        if (deployments.isEmpty())
            return false;
        return deployments.first().isCurrent;
    }

    static Ostree load();
};

// Asynchronously loads data
class AppData : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_DISABLE_COPY_MOVE(AppData)
    AppData(QObject *parent = nullptr);

public: // singleton methods
    static AppData *instance()
    {
        static AppData *s_instance = new AppData();
        return s_instance;
    }

    static AppData *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine)
        AppData *instancePtr = instance();
        qmlEngine->setObjectOwnership(instancePtr, QQmlEngine::CppOwnership);
        return instancePtr;
    }

public:
    Q_PROPERTY(Ostree ostree READ getOstree NOTIFY ostreeChanged)
    const Ostree &getOstree() const
    {
        return ostree;
    }
    Ostree ostree;
    bool ostreeLoading = false;
    Q_INVOKABLE void reloadOstree(QJSValue callback = QJSValue());
    Q_SIGNAL void ostreeChanged();

    Q_PROPERTY(GpuDrivers gpu READ getGpu NOTIFY gpuChanged)
    const GpuDrivers &getGpu() const
    {
        return gpu;
    }
    GpuDrivers gpu;
    void loadBestDrivers();
    Q_SIGNAL void gpuChanged();

    //   /etc/os-release
    Q_PROPERTY(QJsonObject osRelease READ getOsRelease NOTIFY osReleaseChanged)
    QJsonObject osRelease;
    bool osReleaseLoading = false;
    const QJsonObject &getOsRelease() const
    {
        return osRelease;
    }
    Q_SIGNAL void osReleaseChanged();
    void loadOsRelease();
};
inline static AppData *appData = AppData::instance();

QJsonObject runJson(const QString &program, const QStringList &args);
