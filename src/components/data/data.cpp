#include "data.h"
#include <qcontainerfwd.h>
#include <qtconcurrentrun.h>

AppData::AppData(QObject *parent)
    : QObject(parent)
{
    reloadOstree();
    loadBestDrivers();
    loadOsRelease();
};

void AppData::loadBestDrivers()
{
    QtConcurrent::run([]() {
        return GpuDrivers::load();
    }).then(this, [this](GpuDrivers g) {
        this->gpu = std::move(g);
        Q_EMIT gpuChanged();
    });
}

GpuDrivers GpuDrivers::load()
{
    const auto path = u"/usr/libexec/bazzite-detect-nvidia-support-status"_s;

    if (!QFile::exists(path))
        return {};

    GpuDrivers result;

    QProcess proc;

    proc.start(path);
    if (!proc.waitForFinished(15000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        return {};

    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    if (output == u"supported"_s)
        result.best = Gpu::Drivers::NVIDIA_OPEN;
    else if (output == u"legacy"_s)
        result.best = Gpu::Drivers::NVIDIA;
    else if (output == u"unsupported"_s)
        result.best = Gpu::Drivers::UNSUPPORTED;
    else if (output.isEmpty())
        result.best = Gpu::Drivers::BASE;
    else
        result.best = Gpu::Drivers::UNKNOWN;

    return result;
}

void AppData::reloadOstree(QJSValue callback)
{
    if (ostreeLoading)
        return;

    ostreeLoading = true;
    QtConcurrent::run([]() {
        return Ostree::load();
    })
        .then(this,
              [this, callback](Ostree os) {
                  this->ostree = std::move(os);
                  Q_EMIT ostreeChanged();
                  this->ostreeLoading = false;
                  if (callback.isCallable())
                      callback.call();
              })
        .onCanceled(this, [this, callback]() {
            this->ostreeLoading = false;
            if (callback.isCallable())
                callback.call();
        });
}

QJsonObject runJson(const QString &program, const QStringList &args)
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForFinished(15000) || proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
        return {};
    return QJsonDocument::fromJson(proc.readAllStandardOutput()).object();
}

Ostree loadRpmOstree()
{
    Ostree result;
    const QJsonObject root = runJson(u"rpm-ostree"_s, {u"status"_s, u"--json"_s});

    for (const QJsonValue &v : root.value(u"deployments"_s).toArray()) {
        const QJsonObject d = v.toObject();
        OstreeDeployment dep;
        dep.isCurrent = d.value(u"booted"_s).toBool();
        dep.isStaged = d.value(u"staged"_s).toBool();
        for (const QJsonValue &p : d.value(u"requested-packages"_s).toArray())
            dep.layeredPackages << p.toString();
        for (const QJsonValue &p : d.value(u"requested-local-packages"_s).toArray())
            dep.layeredPackages << p.toString();
        result.deployments << dep;
    }
    return result;
}

Ostree loadBootc()
{
    Ostree result;

    // TODO: bootc status needs root
    //       Also review the aurora update script for handling this
    //       bootc has a compatibility value which can be used to determine if bootc or rpm-ostree should be used
    // https://github.com/get-aurora-dev/common/blob/f813004dd14804a674ea22f45dd538f0d4b56f08/system_files/shared/usr/share/ublue-os/just/update.just#L54
    //
    // also TODO: Recommend/enforce removing layered packages before a rebase
    const QJsonObject status = runJson(u"bootc"_s, {u"status"_s, u"--json"_s}).value(u"status"_s).toObject();

    // bootc reports up to three slots, each an object or null.
    // Same order as rpm-ostree: staged, booted, rollback.
    auto add = [&](const QString &key, bool current, bool staged) {
        if (!status.value(key).isObject())
            return;
        OstreeDeployment dep;
        dep.isCurrent = current;
        dep.isStaged = staged;
        result.deployments << dep;
    };
    add(u"staged"_s, false, true);
    add(u"booted"_s, true, false);
    add(u"rollback"_s, false, false);
    return result;
}

Ostree Ostree::load()
{
    if (!QStandardPaths::findExecutable(u"rpm-ostree"_s).isEmpty())
        return loadRpmOstree();
    if (!QStandardPaths::findExecutable(u"bootc"_s).isEmpty())
        return loadBootc();
    return {};
}

void AppData::loadOsRelease()
{
    if (osReleaseLoading)
        return;

    osReleaseLoading = true;
    QtConcurrent::run([]() {
        QJsonObject data = {};
        auto file = QFile(u"/etc/os-release"_s);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                QByteArray arr = file.readLine();

                auto line = QString::fromUtf8(arr).trimmed();

                if (line.isEmpty() || line.startsWith(u'#'))
                    continue;

                int split = line.indexOf(u'=');
                if (split == -1)
                    continue;

                auto key = line.left(split);
                auto value = line.mid(split + 1);

                // Remove surrounding quotations, if there are any
                if (value.length() >= 2 && ((value.startsWith(u'"') && value.endsWith(u'"')) || (value.startsWith(u'\'') && value.endsWith(u'\'')))) {
                    value = value.sliced(1, value.length() - 2);
                }

                data.insert(key, value);
            }
        }

        return data;
    })
        .then(this,
              [this](QJsonObject data) {
                  this->osRelease = std::move(data);
                  Q_EMIT osReleaseChanged();
                  this->osReleaseLoading = false;
              })
        .onCanceled(this, [this]() {
            this->osReleaseLoading = false;
        });
}
