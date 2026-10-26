// SPDX-License-Identifier: GPL-3.0-or-later
#include "graveyard.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

GraveYard::GraveYard(QString path) : path_(std::move(path)) {}

QString GraveYard::defaultPath()
{
    QString dir = qEnvironmentVariable("OREGONTRAIL_DATA_DIR");
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dir).filePath("graves.json");
}

void GraveYard::load()
{
    graves_.clear();
    QFile f(path_);
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;                                      // damaged: start with an empty yard
    for (const QJsonValue &v : doc.object().value("graves").toArray()) {
        const QJsonObject o = v.toObject();
        Tombstone g;
        g.name = o.value("name").toString().left(40);
        g.cause = o.value("cause").toString().left(40);
        g.date = o.value("date").toString().left(40);
        g.epitaph = o.value("epitaph").toString().left(40);
        g.mile = std::clamp(o.value("mile").toInt(-1), -1, 2040);
        if (g.name.isEmpty() || g.mile < 0)
            continue;                                // skip entries that make no sense
        graves_.append(g);
    }
    while (graves_.size() > kMaxGraves)
        graves_.removeFirst();
}

bool GraveYard::save() const
{
    QDir().mkpath(QFileInfo(path_).absolutePath());
    QJsonArray arr;
    for (const Tombstone &g : graves_) {
        QJsonObject o;
        o["name"] = g.name;
        o["cause"] = g.cause;
        o["date"] = g.date;
        o["epitaph"] = g.epitaph;
        o["mile"] = g.mile;
        arr.append(o);
    }
    QJsonObject root;
    root["version"] = 1;
    root["graves"] = arr;
    QSaveFile f(path_);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

void GraveYard::add(const Tombstone &g)
{
    graves_.append(g);
    while (graves_.size() > kMaxGraves)
        graves_.removeFirst();
}
