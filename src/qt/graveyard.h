// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>
#include <QString>

// A tombstone left on the trail by someone who died in a past game. The list is kept in a
// small JSON file so later games meet the graves of earlier ones.
struct Tombstone {
    QString name, cause, date, epitaph;
    int mile = 0;                       // where on the trail (miles from Independence)
};

class GraveYard {
public:
    static constexpr int kMaxGraves = 300;           // oldest are forgotten past this

    explicit GraveYard(QString path = defaultPath());

    // $OREGONTRAIL_DATA_DIR/graves.json if that variable is set (tests use it), otherwise
    // the user's application data directory.
    static QString defaultPath();

    void load();                         // a missing or damaged file just means an empty yard
    bool save() const;                   // atomic: a crash can't leave half a file
    void add(const Tombstone &g);            // keeps the newest kMaxGraves
    const QList<Tombstone> &graves() const { return graves_; }
    const QString &path() const { return path_; }

private:
    QString path_;
    QList<Tombstone> graves_;
};
