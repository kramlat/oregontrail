// SPDX-License-Identifier: GPL-3.0-or-later
#include "qt/graveyard.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdio>

static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } \
    } while (0)

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir tmp;
    const QString path = tmp.filePath("sub/dir/graves.json");

    {   // a missing file is an empty yard, and saving creates the folders
        GraveYard y(path);
        y.load();
        CHECK(y.graves().isEmpty());
        y.add({"Anna", "dysentery", "July 4, 1848", "She loved the open road", 620});
        y.add({"Ben", "drowning", "June 1, 1848", "", 185});
        CHECK(y.save());
    }
    {   // it comes back as written, in order, including awkward text
        GraveYard y(path);
        y.load();
        CHECK(y.graves().size() == 2);
        CHECK(y.graves()[0].name == "Anna" && y.graves()[0].mile == 620);
        CHECK(y.graves()[0].epitaph == "She loved the open road");
        CHECK(y.graves()[1].cause == "drowning" && y.graves()[1].epitaph.isEmpty());
    }
    {   // unicode and quotes survive
        GraveYard y(path);
        y.load();
        y.add({"Zoë \"Z\"", "cholera", "May 5, 1848", "Gone west — ¡adiós!", 99});
        CHECK(y.save());
        GraveYard z(path);
        z.load();
        CHECK(z.graves().size() == 3 && z.graves()[2].name == "Zoë \"Z\"" && z.graves()[2].epitaph == "Gone west — ¡adiós!");
    }
    {   // a damaged file is ignored rather than crashing
        QFile f(path);
        CHECK(f.open(QIODevice::WriteOnly));
        f.write("{ this is not json");
        f.close();
        GraveYard y(path);
        y.load();
        CHECK(y.graves().isEmpty());
    }
    {   // nonsense entries are skipped, sensible ones kept; absurd miles can't escape the trail
        QFile f(path);
        CHECK(f.open(QIODevice::WriteOnly));
        f.write(R"({"graves":[{"name":"","mile":10},{"name":"Ok","mile":50},{"name":"Far","mile":999999},{"mile":5},7]})");
        f.close();
        GraveYard y(path);
        y.load();
        CHECK(y.graves().size() == 2 && y.graves()[0].name == "Ok" && y.graves()[1].mile == 2040);
    }
    {   // the yard is capped: the oldest are forgotten
        GraveYard y(path);
        y.load();
        for (int i = 0; i < GraveYard::kMaxGraves + 25; ++i) y.add({QString("P%1").arg(i), "x", "d", "", i % 2000});
        CHECK(y.graves().size() == GraveYard::kMaxGraves);
        CHECK(y.graves().last().name == QString("P%1").arg(GraveYard::kMaxGraves + 24));
        CHECK(y.save());
        GraveYard z(path);
        z.load();
        CHECK(z.graves().size() == GraveYard::kMaxGraves);
    }
    {   // the data directory can be redirected, which is how the tests avoid touching real saves
        qputenv("OREGONTRAIL_DATA_DIR", tmp.path().toUtf8());
        CHECK(GraveYard::defaultPath() == QDir(tmp.path()).filePath("graves.json"));
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
