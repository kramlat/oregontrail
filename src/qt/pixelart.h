// SPDX-License-Identifier: GPL-3.0-or-later
//
// All artwork is drawn procedurally onto a 280x192 canvas (the Apple II hi-res
// grid) using its six-colour palette, then scaled up with nearest-neighbour.
// Original art; nothing here is taken from the 1985 release.
#pragma once

#include "trail/trail.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QString>

namespace art {

constexpr int W = 280, H = 192;

inline const QColor Black(0x00, 0x00, 0x00), White(0xff, 0xff, 0xff), Green(0x14, 0xf5, 0x3c),
    Violet(0xd0, 0x2b, 0xd4), Orange(0xff, 0x6a, 0x3c), Blue(0x14, 0xcf, 0xfd);

QImage newCanvas();

// Wagon train rolling along the trail. `scroll` is in canvas pixels.
void travelScene(QImage &img, trail::Terrain t, trail::Weather w, double scroll, int frame);
// Stationary scene at a landmark / fort / river / town.
void landmarkScene(QImage &img, const trail::Landmark &l, int frame);
// `epitaph` is whatever the player chose to have carved (may be empty).
void tombstone(QImage &img, const QString &name, const QString &cause, const QString &date,
               const QString &epitaph = {});
void oregonCity(QImage &img);

enum class Animal { Rabbit, Deer, Bison, Wolf, Bandit };
struct Sprite { int w, h; };
Sprite animalSize(Animal a);
void drawAnimal(QPainter &p, Animal a, int x, int y, bool facingRight, int frame);
// The covered wagon alone (46 px wide, 30 px tall above y, wheels just below).
void drawWagon(QPainter &p, int x, int y, int frame);

} // namespace art
