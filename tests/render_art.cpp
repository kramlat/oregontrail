// SPDX-License-Identifier: GPL-3.0-or-later
// Dumps every scene to PNG (4x) so the artwork can be eyeballed without the GUI.
#include "qt/pixelart.h"

#include <QGuiApplication>
#include <QDir>

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
    QImage h = art::newCanvas();
    art::huntBackdrop(h, Terrain::Plains);
    QPainter p(&h);
    art::drawAnimal(p, art::Animal::Rabbit, 40, 150, true, 0);
    art::drawAnimal(p, art::Animal::Deer, 100, 130, false, 1);
    art::drawAnimal(p, art::Animal::Bison, 170, 140, true, 0);
    art::drawCrosshair(p, 110, 140);
    p.end();
    save(h, dir, "hunt");
    QImage t = art::newCanvas();
    art::tombstone(t, "Anna", "dysentery", "July 4, 1848");
    save(t, dir, "tombstone");
}
