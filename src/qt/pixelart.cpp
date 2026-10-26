// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixelart.h"

#include <QFont>
#include <QPolygon>

#include <cmath>

namespace art {

using trail::Terrain;
using trail::Weather;

namespace {

constexpr int kHorizon = 108;

void rect(QPainter &p, int x, int y, int w, int h, const QColor &c) { p.fillRect(x, y, w, h, c); }

void poly(QPainter &p, std::initializer_list<QPoint> pts, const QColor &c)
{
    p.setPen(c);
    p.setBrush(c);
    p.drawPolygon(QPolygon(QList<QPoint>(pts)));
}

struct Painter : QPainter {
    explicit Painter(QImage &img) : QPainter(&img) { setRenderHint(QPainter::Antialiasing, false); }
};

// A repeating row of shapes scrolling at some fraction of `scroll`.
template <class F> void tiled(double scroll, int period, F draw)
{
    int off = int(std::fmod(scroll, period));
    for (int x = -off - period; x < W + period; x += period)
        draw(x);
}

void mountains(QPainter &p, double scroll, int period, int baseY, int height, bool snow, const QColor &c)
{
    tiled(scroll, period, [&](int x) {
        poly(p, {{x, baseY}, {x + period / 2, baseY - height}, {x + period, baseY}}, c);
        if (snow)
            poly(p, {{x + period / 2 - height / 4, baseY - height * 3 / 4}, {x + period / 2, baseY - height},
                     {x + period / 2 + height / 4, baseY - height * 3 / 4}, {x + period / 2, baseY - height * 5 / 8}},
                 White);
    });
}

void pine(QPainter &p, int x, int y, int h)
{
    rect(p, x + h / 5, y - 3, 2, 3, Orange);
    poly(p, {{x, y - 3}, {x + h / 4 + 1, y - h}, {x + h / 2, y - 3}}, Green);
}

void cloud(QPainter &p, int x, int y)
{
    rect(p, x, y, 22, 4, White);
    rect(p, x + 4, y - 3, 12, 3, White);
}

void sky(QPainter &p, Weather w)
{
    QColor c = (w == Weather::Rain || w == Weather::Snow) ? Black : Blue;
    rect(p, 0, 0, W, kHorizon, c);
}

void ground(QPainter &p, Terrain t)
{
    QColor c = t == Terrain::Desert ? Orange : Green;
    rect(p, 0, kHorizon, W, H - kHorizon, c);
}

void road(QPainter &p, double scroll, bool desert)
{
    rect(p, 0, 150, W, 14, desert ? White : Orange);
    rect(p, 0, 156, W, 2, Violet);
    tiled(scroll, 24, [&](int x) { rect(p, x, 161, 8, 1, Black); });
}

void ox(QPainter &p, int x, int y, int frame)
{
    rect(p, x, y - 14, 20, 9, Violet);              // body
    rect(p, x + 20, y - 17, 7, 7, Violet);          // head
    rect(p, x + 25, y - 19, 2, 2, White);           // horn
    rect(p, x + 22, y - 15, 1, 1, Black);           // eye
    rect(p, x - 2, y - 13, 2, 5, Violet);           // tail
    rect(p, x + 3, y - 9, 14, 1, Black);            // shading
    int a = frame & 1;
    rect(p, x + 2 + a * 2, y - 5, 2, 5, Violet);
    rect(p, x + 7 - a * 2, y - 5, 2, 5, Black);
    rect(p, x + 13 + a * 2, y - 5, 2, 5, Black);
    rect(p, x + 17 - a * 2, y - 5, 2, 5, Violet);
}

void wheel(QPainter &p, int cx, int cy, int frame)
{
    p.setPen(White);
    p.setBrush(Black);
    p.drawEllipse(QPoint(cx, cy), 7, 7);
    p.setPen(Orange);
    if (frame & 1) { p.drawLine(cx - 6, cy, cx + 6, cy); p.drawLine(cx, cy - 6, cx, cy + 6); }
    else           { p.drawLine(cx - 4, cy - 4, cx + 4, cy + 4); p.drawLine(cx - 4, cy + 4, cx + 4, cy - 4); }
}

void wagon(QPainter &p, int x, int y, int frame)
{
    rect(p, x, y - 16, 46, 9, Violet);                              // box
    rect(p, x, y - 16, 46, 1, Black);
    rect(p, x, y - 10, 46, 1, Black);
    poly(p, {{x + 2, y - 16}, {x + 8, y - 30}, {x + 38, y - 30}, {x + 44, y - 16}}, White);   // canvas cover
    rect(p, x + 8, y - 25, 30, 1, Blue);
    rect(p, x - 14, y - 11, 14, 2, Black);                          // tongue
    wheel(p, x + 11, y - 3, frame);
    wheel(p, x + 35, y - 3, frame);
}

void train(QPainter &p, int frame, bool moving)
{
    int bob = moving ? (frame & 1) : 0;
    int y = 150 + bob;
    ox(p, 30, y, frame);
    ox(p, 62, y, frame + 1);
    wagon(p, 108, y, moving ? frame : 0);
}

void weatherOverlay(QPainter &p, Weather w, double scroll)
{
    if (w != Weather::Rain && w != Weather::Snow) return;
    for (int i = 0; i < 70; ++i) {
        int x = int(i * 41 + scroll * 2) % W, y = int(i * 29 + scroll * 5) % H;
        if (w == Weather::Rain) rect(p, x, y, 1, 4, Blue);
        else rect(p, x, y, 2, 2, White);
    }
}

} // namespace

QImage newCanvas()
{
    QImage img(W, H, QImage::Format_RGB32);
    img.fill(Black);
    return img;
}

void travelScene(QImage &img, Terrain t, Weather w, double scroll, int frame)
{
    Painter p(img);
    sky(p, w);
    if (w == Weather::Warm || w == Weather::Hot || w == Weather::Cool) {
        tiled(scroll * 0.1, 140, [&](int x) { cloud(p, x + 30, 24); });
        tiled(scroll * 0.15, 190, [&](int x) { cloud(p, x + 100, 50); });
    }
    ground(p, t);

    switch (t) {
    case Terrain::Plains:
        tiled(scroll * 0.25, 120, [&](int x) { poly(p, {{x, kHorizon}, {x + 60, kHorizon - 9}, {x + 120, kHorizon}}, Green); });
        break;
    case Terrain::Hills:
        mountains(p, scroll * 0.25, 130, kHorizon, 22, false, Violet);
        tiled(scroll * 0.5, 90, [&](int x) { poly(p, {{x, kHorizon}, {x + 45, kHorizon - 14}, {x + 90, kHorizon}}, Green); });
        break;
    case Terrain::Rockies:
        mountains(p, scroll * 0.2, 150, kHorizon, 64, true, Violet);
        mountains(p, scroll * 0.4, 100, kHorizon + 6, 40, true, Green);
        tiled(scroll * 0.9, 50, [&](int x) { pine(p, x, kHorizon + 14, 14); });
        break;
    case Terrain::Desert:
        mountains(p, scroll * 0.2, 170, kHorizon, 30, false, Violet);
        tiled(scroll * 0.9, 80, [&](int x) {      // sagebrush
            rect(p, x + 5, kHorizon + 22, 7, 4, Green);
            rect(p, x + 7, kHorizon + 20, 3, 2, Green);
        });
        break;
    case Terrain::BlueMountains:
        mountains(p, scroll * 0.25, 120, kHorizon, 46, false, Violet);
        tiled(scroll * 0.6, 34, [&](int x) { pine(p, x, kHorizon + 8, 22); });
        tiled(scroll * 0.9, 46, [&](int x) { pine(p, x + 14, kHorizon + 24, 28); });
        break;
    case Terrain::Cascades:
        mountains(p, scroll * 0.2, 140, kHorizon, 70, true, Violet);
        tiled(scroll * 0.6, 28, [&](int x) { pine(p, x, kHorizon + 10, 26); });
        tiled(scroll * 0.9, 38, [&](int x) { pine(p, x + 10, kHorizon + 26, 34); });
        break;
    }
    if (t == Terrain::Plains || t == Terrain::Hills)
        tiled(scroll * 1.2, 36, [&](int x) {      // grass tufts
            rect(p, x + 3, 134, 1, 3, Black);
            rect(p, x + 6, 135, 1, 2, Black);
        });

    road(p, scroll, t == Terrain::Desert);
    train(p, frame, true);
    weatherOverlay(p, w, scroll);
}

void landmarkScene(QImage &img, const trail::Landmark &l, int frame)
{
    Painter p(img);
    const QString n = QString::fromLatin1(l.name);
    bool snowy = n.contains("South Pass");
    sky(p, Weather::Warm);
    cloud(p, 40, 26);
    cloud(p, 180, 40);
    ground(p, n.contains("Desert") ? Terrain::Desert : Terrain::Plains);

    if (n.contains("Chimney Rock")) {
        poly(p, {{150, kHorizon}, {170, 40}, {174, 40}, {196, kHorizon}}, Orange);
        rect(p, 168, 24, 8, 20, Orange);
        mountains(p, 0, 140, kHorizon, 20, false, Violet);
    } else if (n.contains("Independence Rock")) {
        p.setBrush(Violet); p.setPen(Violet);
        p.drawEllipse(QPoint(160, kHorizon + 10), 80, 40);
    } else if (snowy || n.contains("Blue Mountains") || n.contains("Dalles")) {
        mountains(p, 0, 140, kHorizon, 66, true, Violet);
        tiled(0, 40, [&](int x) { pine(p, x + 6, kHorizon + 22, 24); });
    } else if (n.contains("Fort")) {
        for (int x = 70; x < 230; x += 8) {       // stockade
            rect(p, x, 92, 7, 46, Orange);
            poly(p, {{x, 92}, {x + 3, 84}, {x + 7, 92}}, Orange);
            rect(p, x + 6, 92, 1, 46, Violet);
        }
        rect(p, 136, 108, 24, 30, Black);          // gate
        rect(p, 147, 108, 2, 30, Orange);
        rect(p, 102, 60, 2, 34, White);            // flagpole
        rect(p, 104, 60, 14, 9, White);
        for (int s = 0; s < 3; ++s) rect(p, 104, 60 + s * 3, 14, 1, Orange);
        rect(p, 104, 60, 5, 4, Blue);
    } else if (n.contains("Independence, Missouri") || n.contains("Oregon City")) {
        for (int i = 0; i < 4; ++i) {
            int x = 40 + i * 52, h = 28 + (i % 2) * 10;
            rect(p, x, kHorizon + 24 - h, 34, h, i % 2 ? Orange : White);
            poly(p, {{x - 3, kHorizon + 24 - h}, {x + 17, kHorizon + 8 - h}, {x + 37, kHorizon + 24 - h}}, Violet);
            rect(p, x + 13, kHorizon + 12, 8, 12, Black);
        }
    } else if (l.kind == trail::Kind::River) {
        rect(p, 0, 114, W, 28, Blue);                 // the water, between two banks
        for (int y = 118; y < 140; y += 7)
            tiled(frame * 3.0 + y * 3, 28, [&](int x) { rect(p, x, y, 12, 1, White); });
    } else {
        mountains(p, 0, 150, kHorizon, 24, false, Violet);
    }

    if (l.kind != trail::Kind::River) {
        rect(p, 0, 150, W, 12, Orange);
    }
    // The wagon waits at the landmark.
    int bank = l.kind == trail::Kind::River ? 8 : 0;
    p.save();
    p.translate(0, bank ? 14 : 0);
    ox(p, 22, 150, 0);
    ox(p, 54, 150, 0);
    wagon(p, 100, 150, 0);
    p.restore();
}

void tombstone(QImage &img, const QString &name, const QString &cause, const QString &date, const QString &epitaph)
{
    Painter p(img);
    rect(p, 0, 0, W, H, Black);
    rect(p, 0, 140, W, H - 140, Green);
    // stone
    rect(p, 90, 50, 100, 94, White);
    p.setBrush(White); p.setPen(White);
    p.drawEllipse(QPoint(140, 54), 50, 30);
    rect(p, 90, 130, 100, 14, Violet);
    p.setPen(Black);
    QFont f("monospace");
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(90, 36, 100, 14), Qt::AlignCenter, "HERE LIES");
    p.drawText(QRect(90, 54, 100, 14), Qt::AlignCenter, name.toUpper());
    f.setBold(false);
    f.setPixelSize(8);
    f.setItalic(true);
    p.setFont(f);
    // What the family wrote on it (up to two lines).
    if (!epitaph.trimmed().isEmpty())
        p.drawText(QRect(94, 70, 92, 24), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, epitaph.trimmed());
    f.setItalic(false);
    p.setFont(f);
    p.drawText(QRect(90, 100, 100, 12), Qt::AlignCenter, "Died of " + cause);
    p.drawText(QRect(90, 112, 100, 12), Qt::AlignCenter, date);
    // crosses for the graves already behind you
    rect(p, 20, 90, 4, 50, Orange);
    rect(p, 12, 100, 20, 4, Orange);
    rect(p, 240, 96, 4, 44, Orange);
    rect(p, 232, 106, 20, 4, Orange);
}

void oregonCity(QImage &img)
{
    trail::Landmark l{"Oregon City, Oregon", 2040, trail::Kind::End};
    landmarkScene(img, l, 0);
}

Sprite animalSize(Animal a)
{
    switch (a) {
    case Animal::Rabbit: return {10, 7};
    case Animal::Deer: return {22, 18};
    case Animal::Wolf: return {20, 11};
    case Animal::Bandit: return {10, 20};
    default: return {32, 20};
    }
}

void drawAnimal(QPainter &p, Animal a, int x, int y, bool right, int frame)
{
    // Drawn facing right; mirror about the sprite's centre otherwise.
    Sprite s = animalSize(a);
    p.save();
    if (!right) {
        p.translate(x + s.w, y);
        p.scale(-1, 1);
    } else {
        p.translate(x, y);
    }
    int step = frame & 1;
    switch (a) {
    case Animal::Rabbit:
        rect(p, 1, 2, 6, 4, White);
        rect(p, 6, 1, 3, 3, White);
        rect(p, 7, 0, 1, 2, White);
        rect(p, 0, 3, 1, 2, White);
        rect(p, 2 + step * 2, 5, 2, 2, White);
        break;
    case Animal::Deer:
        rect(p, 4, 6, 12, 6, Orange);
        rect(p, 15, 2, 4, 6, Orange);
        rect(p, 17, 0, 4, 3, Orange);
        rect(p, 17, -3, 1, 4, White);
        rect(p, 20, -3, 1, 4, White);
        rect(p, 2, 6, 2, 3, White);
        rect(p, 5 + step * 3, 12, 2, 6, Orange);
        rect(p, 9 - step * 3, 12, 2, 6, Violet);
        rect(p, 13 + step * 2, 12, 2, 6, Violet);
        break;
    case Animal::Wolf:
        rect(p, 3, 3, 12, 5, Black);          // body
        rect(p, 14, 1, 5, 5, Black);          // head
        rect(p, 18, 3, 2, 2, Black);          // snout
        rect(p, 14, 0, 1, 2, Black);          // ears
        rect(p, 17, 0, 1, 2, Black);
        rect(p, 17, 2, 1, 1, Orange);         // eye
        rect(p, 18, 5, 2, 1, White);          // teeth
        rect(p, 0, 2, 4, 2, Black);           // tail
        rect(p, 4 + step * 3, 8, 2, 3, Black);
        rect(p, 8 - step * 3, 8, 2, 3, Black);
        rect(p, 12 + step * 2, 8, 2, 3, Black);
        break;
    case Animal::Bandit:
        rect(p, 1, 0, 8, 2, Black);           // hat brim
        rect(p, 3, -2, 4, 2, Black);          // crown
        rect(p, 3, 2, 4, 3, Orange);          // face
        rect(p, 3, 4, 4, 1, Black);           // bandana
        rect(p, 2, 5, 6, 8, Black);           // coat
        rect(p, 8, 7, 4, 1, Black);           // gun arm
        rect(p, 11, 6, 1, 2, White);          // gun
        rect(p, 2 + step * 2, 13, 2, 7, Black);
        rect(p, 6 - step * 2, 13, 2, 7, Black);
        break;
    default:   // bison
        rect(p, 6, 3, 20, 11, Violet);
        rect(p, 2, 7, 10, 9, Violet);
        rect(p, 0, 9, 5, 6, Orange);
        rect(p, 0, 7, 2, 2, White);
        rect(p, 6 + step * 3, 14, 3, 6, Orange);
        rect(p, 13 - step * 3, 14, 3, 6, Orange);
        rect(p, 20 + step * 2, 14, 3, 6, Orange);
        rect(p, 26, 4, 5, 2, Violet);
        break;
    }
    p.restore();
}

void drawWagon(QPainter &p, int x, int y, int frame) { wagon(p, x, y, frame); }

} // namespace art
