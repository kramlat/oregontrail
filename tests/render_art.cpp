// SPDX-License-Identifier: GPL-3.0-or-later
// Dumps every scene to PNG (4x) so the artwork can be eyeballed without the GUI.
#include "qt/fightview.h"
#include "qt/pixelart.h"

#include <QGuiApplication>
#include <QDir>
#include <random>

using namespace trail;

static void save(const QImage &img, const QString &dir, const QString &name)
{
    img.scaled(img.size() * 3, Qt::KeepAspectRatio, Qt::FastTransformation).save(dir + "/" + name + ".png");
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QString dir = argc > 1 ? argv[1] : ".";
    QDir().mkpath(dir);
    const Terrain ters[] = {Terrain::Plains, Terrain::Hills, Terrain::Rockies, Terrain::Desert, Terrain::BlueMountains, Terrain::Cascades};
    const char *names[] = {"plains", "hills", "rockies", "desert", "bluemtn", "cascades"};
    for (int i = 0; i < 6; ++i) {
        QImage img = art::newCanvas();
        art::travelScene(img, ters[i], i == 2 ? Weather::Snow : Weather::Warm, 37 + i * 11, 1);
        save(img, dir, QString("travel_") + names[i]);
    }
    for (const Landmark &l : landmarks()) {
        QImage img = art::newCanvas();
        art::landmarkScene(img, l, 1);
        QString n = QString::fromLatin1(l.name).replace(' ', '_').replace(',', '_');
        save(img, dir, "landmark_" + n);
    }
    {   // a hunt: face the nearest animal
        struct R : oregon::Rng {
            std::mt19937 g;
            explicit R(unsigned s) : g(s) {}
            double next() override { return std::uniform_real_distribution<double>(0, 1)(g); }
        } rng(21);
        fight::Field f(fight::Field::HuntTag{}, 20, rng);
        f.py = fight::Field::N / 2.0 + 4;
        double best = 1e9;
        for (auto &e : f.enemies) {
            double d = std::hypot(e.x - f.px, e.y - f.py);
            if (d < best) { best = d; f.angle = std::atan2(e.y - f.py, e.x - f.px); }
        }
        for (int i = 0; i < 10; ++i) f.update(0.05);
        QImage img = art::newCanvas();
        renderFight(img, f);
        save(img, dir, "hunt");
    }
    for (int wolves = 0; wolves < 2; ++wolves) {
        struct R : oregon::Rng {
            std::mt19937 g;
            explicit R(unsigned s) : g(s) {}
            double next() override { return std::uniform_real_distribution<double>(0, 1)(g); }
        } rng(wolves ? 11u : 5u);
        fight::Field f(wolves ? fight::Kind::Wolf : fight::Kind::Bandit, 5, 20, rng);
        f.angle = -1.5708 + (wolves ? 0.15 : 0.6);  // looking toward the wagon
        for (int i = 0; i < (wolves ? 55 : 80); ++i) f.update(0.05);   // let the pack close in (bandits open fire)
        f.fire();
        f.update(0.05);
        QImage img = art::newCanvas();
        renderFight(img, f);
        save(img, dir, wolves ? "fight_wolves" : "fight_bandits");
    }
    QImage t = art::newCanvas();
    art::tombstone(t, "Anna", "dysentery", "July 4, 1848", "She loved the open road");
    save(t, dir, "tombstone");
}
