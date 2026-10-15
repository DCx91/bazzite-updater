// SPDX-FileCopyrightText: 2026 Robert French <frenchrobertm@outlook.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rebase_model.h"
#include "k_config.h"
#include "other_utils.h"
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qvariant.h>

inline const QString KeyUrl = u"url"_s;
inline const QString KeyName = u"name"_s;
inline const QString KeyFeatures = u"features"_s;
inline const QString KeyImages = u"images"_s;
inline const QString KeyTags = u"tags"_s;

// Only available for the Current system image
inline const QString KeyCurrentTag = u"currentTag"_s;

RebaseModel::RebaseModel()
{
    for (const QVariant &item : appConfig()->rebaseTargets) {
        auto obj = item.toJsonObject();

        auto imgs = obj.value(KeyImages).toArray();
        auto url = obj.value(KeyUrl).toString();

        // find the current image to store its features.
        // Incompatible images cannot be shown.
        bool currentFound = false;
        for (const QJsonValue &img : imgs) {
            auto imgObj = img.toObject();

            auto imgName = imgObj.value(KeyName).toString();
            auto imgFeatures = imgObj.value(KeyFeatures).toArray();

            if (otherUtilsBackend->currentImage().m_imageRef == u"%1/%2"_s.arg(url).arg(imgName)) {
                for (const auto &feat : imgFeatures) {
                    current_image_features.insert(feat.toVariant());
                }
                imgObj.insert(KeyUrl, url);
                imgObj.insert(KeyCurrentTag, otherUtilsBackend->currentImage().m_imageBranch);
                currentFound = true;
                currentImage = imgObj;
                break;
            }
        }

        // Create a fallback image
        if (!currentFound) {
            QJsonObject obj;
            auto img = otherUtilsBackend->currentImage();
            int sep = img.m_imageRef.lastIndexOf(u'/');
            obj.insert(KeyUrl, img.m_imageRef.left(sep));
            obj.insert(KeyName, img.m_imageRef.mid(sep + 1));
            obj.insert(KeyTags, QJsonArray({img.m_imageBranch}));
            obj.insert(KeyCurrentTag, img.m_imageBranch);
            obj.insert(KeyFeatures, QJsonArray());
            currentImage = obj;
        }

        for (const QJsonValue &img : imgs) {
            auto imgObj = img.toObject();
            auto imgFeatures = imgObj.value(KeyFeatures).toArray();

            if (currentFound) {
                bool currentKde = current_image_features.contains(u"kde"_s);
                bool imgKde = imgFeatures.contains(u"kde"_s);

                if (currentKde != imgKde)
                    continue;

                if (img == currentImage)
                    continue;
            }

            imgObj.insert(KeyUrl, url);
            targets.append(imgObj);
        }
    }

    filtered = targets;
}

// convert set to list
QVariantList setToList(const QVariantSet &set)
{
    QVariantList list;
    list.reserve(set.size());
    for (const QVariant &item : set) {
        list.append(item);
    }
    return list;
}

void RebaseModel::setFeatures(QVariantList features)
{
    m_features.clear();
    for (const QVariant &item : features)
        m_features.insert(item);

    updateFiltered();
    Q_EMIT featuresChanged();
}

void RebaseModel::updateFiltered()
{
    filtered.clear();

    if (m_features.isEmpty()) {
        filtered = targets;
    } else {
        for (const QVariant &img : targets) {
            auto obj = img.toJsonObject();

            QVariantSet img_feats;
            for (const QJsonValue &item : obj.value(KeyFeatures).toArray()) {
                img_feats.insert(item.toVariant());
            }

            if (img_feats.contains(m_features)) {
                filtered.append(img);
            }
        }
    }

    Q_EMIT filteredChanged();
}
