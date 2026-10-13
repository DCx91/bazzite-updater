#pragma once

#include <qguiapplication.h>
#include <qjsengine.h>
#include <qlist.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include <QtConcurrent>

using namespace Qt::StringLiterals;

struct OstreeDeployment {
    Q_GADGET;

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

public:
    QList<OstreeDeployment> deployments;
    Q_PROPERTY(QList<OstreeDeployment> deployments READ getDeployments CONSTANT)
    QList<OstreeDeployment> getDeployments() const
    {
        return deployments;
    }
    Q_INVOKABLE bool onLatestDeployment() const
    {
        return deployments.last().isCurrent;
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
    Ostree getOstree() const
    {
        return ostree;
    }
    Ostree ostree;
    bool ostreeLoading;
    Q_INVOKABLE void reloadOstree();
    Q_SIGNAL void ostreeChanged();
};
inline static AppData *appData = AppData::instance();
