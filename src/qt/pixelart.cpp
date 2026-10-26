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

void wheel(QPainter &p, int cx, int cy, int frame, int r = 7)
{
    p.setPen(White);
    p.setBrush(Black);
    p.drawEllipse(QPoint(cx, cy), r, r);
    p.setPen(Orange);
    int k = r - 1, d = r * 4 / 7;
    if (frame & 1) { p.drawLine(cx - k, cy, cx + k, cy); p.drawLine(cx, cy - k, cx, cy + k); }
    else           { p.drawLine(cx - d, cy - d, cx + d, cy + d); p.drawLine(cx - d, cy + d, cx + d, cy - d); }
}

// A covered wagon seen from the side, FACING RIGHT: the canvas is highest at the back and slopes
// down toward the front, the driver sits on a seat at the front, the tongue reaches forward to
// the oxen, the big wheels are at the back, and a bucket hangs from the tailgate.
void wagon(QPainter &p, int x, int y, int frame)
{
    rect(p, x, y - 16, 46, 9, Violet);                              // box
    rect(p, x, y - 16, 46, 1, Black);
    rect(p, x, y - 10, 46, 1, Black);
    poly(p, {{x + 1, y - 16}, {x + 4, y - 31}, {x + 28, y - 31}, {x + 40, y - 23}, {x + 41, y - 16}}, White);   // canvas, sloping to the front
    rect(p, x + 6, y - 26, 24, 1, Blue);                            // ribs
    rect(p, x + 33, y - 21, 6, 5, Black);                           // the open front of the canvas
    rect(p, x + 40, y - 18, 7, 2, Orange);                          // driver's seat
    rect(p, x + 42, y - 27, 4, 9, Black);                           // the driver
    rect(p, x + 42, y - 30, 3, 3, Orange);
    rect(p, x + 41, y - 31, 5, 1, Black);                           // hat brim
    rect(p, x + 46, y - 24, 8, 1, Black);                           // reins, out to the team
    rect(p, x + 46, y - 11, 14, 2, Black);                          // tongue: forward, to the oxen
    rect(p, x - 3, y - 14, 3, 4, Orange);                           // bucket on the tailgate
    wheel(p, x + 11, y - 4, frame, 8);                              // big wheels at the back
    wheel(p, x + 37, y - 2, frame, 6);                              // smaller at the front
}

void train(QPainter &p, int frame, bool moving)
{
    int bob = moving ? (frame & 1) : 0;
    int y = 150 + bob;
    // The wagon is pulled: oxen ahead (right), tongue and yoke chain back to the wagon.
    wagon(p, 40, y, moving ? frame : 0);
    ox(p, 100, y, frame);
    ox(p, 132, y, frame + 1);
    rect(p, 127, y - 11, 5, 1, Black);               // chain between the two oxen
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

// ------------------------------------------------------------------ river crossings (top-down)

namespace {

void oxTop(QPainter &p, int cx, int cy, int frame)
{
    rect(p, cx - 4, cy - 5, 8, 13, Violet);              // body, seen from above, facing up
    rect(p, cx - 1, cy - 5, 2, 13, Black);               // spine
    rect(p, cx - 3, cy - 9, 6, 5, Violet);               // head
    rect(p, cx - 6, cy - 9, 12, 1, White);               // horns
    rect(p, cx - 1, cy - 8, 2, 2, Orange);               // nose
    int a = frame & 1;                                   // legs, alternating
    rect(p, cx - 5, cy - 3 + a * 3, 1, 3, Black);
    rect(p, cx + 4, cy - 3 + (1 - a) * 3, 1, 3, Black);
    rect(p, cx - 5, cy + 3 - a * 3, 1, 3, Black);
    rect(p, cx + 4, cy + 3 - (1 - a) * 3, 1, 3, Black);
}

void wagonTop(QPainter &p, int cx, int cy, double yaw, int frame)
{
    p.save();
    p.translate(cx, cy);
    p.rotate(yaw);
    for (int sx : {-13, 10})                             // wheels, with a spoke that turns
        for (int y : {-13, 3}) {
            rect(p, sx, y, 3, 10, Black);
            rect(p, sx, y + ((frame & 1) ? 2 : 6), 3, 1, Orange);
        }
    rect(p, -10, -17, 20, 34, White);                    // the canvas cover
    for (int y = -11; y <= 9; y += 6) rect(p, -10, y, 20, 1, Blue);   // ribs
    rect(p, -10, -17, 20, 1, Black);
    rect(p, -10, 16, 20, 1, Black);
    rect(p, -6, 16, 12, 2, Black);                       // the open back
    rect(p, -1, -27, 2, 10, Black);                      // tongue
    p.restore();
}

// Foam around something moving through water.
void wake(QPainter &p, int cx, int cy, int w)
{
    rect(p, cx - w / 2 - 3, cy + 2, 3, 1, White);
    rect(p, cx + w / 2, cy + 2, 3, 1, White);
    rect(p, cx - w / 2 - 1, cy + 7, w + 2, 1, White);
}

// Make a patch of the image look half under water.
void submerge(QImage &img, QRect r, int top, int bottom, int density)
{
    for (int y = std::max(r.top(), top); y <= std::min(r.bottom(), bottom); ++y)
        for (int x = std::max(0, r.left()); x <= std::min(W - 1, r.right()); ++x)
            if (density == 1 ? ((x + y) & 1) : ((x & 1) && (y & 1)))
                img.setPixel(x, y, Blue.rgb());
}

} // namespace

void riverCrossingScene(QImage &img, Crossing how, double pr, int frame, double widthFt, double depthFt,
                        bool swamped, bool drowned)
{
    Painter p(img);
    const int bandH = std::clamp(int(40 + widthFt * 0.1), 50, 108);
    const int top = 96 - bandH / 2, bottom = top + bandH;
    const bool deep = depthFt > 3.2;

    // Banks: grass, a strip of sand at the edge, trees across the way.
    rect(p, 0, 0, W, H, Green);
    for (int i = 0; i < 14; ++i) {                       // far bank trees (round, from above)
        int x = 10 + i * 20 + (i % 3) * 5, y = std::max(6, top - 14 - (i % 4) * 10);
        p.setBrush(Black); p.setPen(Black);
        p.drawEllipse(QPoint(x, y), 8, 8);
        p.setBrush(Green); p.setPen(Green);
        p.drawEllipse(QPoint(x - 1, y - 1), 6, 6);
    }
    for (int i = 0; i < 20; ++i) rect(p, (i * 37) % W, bottom + 6 + (i * 53) % std::max(1, H - bottom - 8), 2, 1, Black);   // tufts
    rect(p, 0, top - 3, W, 3, Orange);
    rect(p, 0, bottom, W, 3, Orange);

    // Water, flowing left to right; deeper water is darker.
    rect(p, 0, top, W, bandH, Blue);
    if (deep)
        for (int y = top; y < bottom; ++y)
            for (int x = (y * 3) % 6; x < W; x += 6) img.setPixel(x, y, Violet.rgb());
    for (int k = 0; k * 11 + 7 < bandH; ++k)
        for (int i = 0; i < 9; ++i)
            rect(p, int(i * 34 + frame * 2 + k * 13) % (W + 8) - 8, top + 7 + k * 11, 7, 1, White);

    // Where is everything? (the train enters from the bottom, so its parts start below the screen)
    const bool ferry = how == Crossing::Ferry;
    const double wagonY = ferry ? 0 : 222 - pr * 250;
    double raftY = 0;
    int x0 = 140, wy;
    if (ferry) {
        raftY = (bottom + 58) + pr * ((top - 58) - (bottom + 58));
        wy = int(raftY + 25);
        // Ropes from bank to bank, with posts.
        rect(p, 112, 0, 1, H, Black);
        rect(p, 168, 0, 1, H, Black);
        for (int xp : {112, 168}) {
            p.setBrush(Orange); p.setPen(Black);
            p.drawEllipse(QPoint(xp, top - 8), 3, 3);
            p.drawEllipse(QPoint(xp, bottom + 8), 3, 3);
        }
        // The raft.
        rect(p, x0 - 31, int(raftY) - 60, 62, 112, Black);
        rect(p, x0 - 29, int(raftY) - 58, 58, 108, Orange);
        for (int y = int(raftY) - 58; y < int(raftY) + 50; y += 7) rect(p, x0 - 29, y, 58, 1, Black);
        wake(p, x0, int(raftY) + 50, 60);
        // The ferryman, poling along the side.
        int fx = x0 + 36, fy = int(raftY) + 6;
        rect(p, fx - 2, fy, 5, 6, Black);
        rect(p, fx - 1, fy - 3, 3, 3, Orange);
        rect(p, fx + 3, fy - 10, 1, 24, Black);
    } else {
        wy = int(wagonY);
    }

    // The oxen, in two pairs, then the wagon.
    const int pairA = wy - 68, pairB = wy - 46;     // two pairs of oxen, with room between them
    auto inWater = [&](int y) { return y >= top - 4 && y <= bottom + 4; };
    double yaw = 0;
    int jx = 0;
    if (how == Crossing::Caulk) {
        yaw = 3 * std::sin(frame * 0.8);                 // a floating wagon wobbles
        jx = int(std::sin(frame * 0.6) * 1.5);
    }
    if (swamped && how != Crossing::Ferry && pr > 0.40 && pr < 0.72) {
        double s = (pr - 0.40) / 0.32;
        yaw += 32 * std::sin(s * 3.14159);               // slewed round by the current
    }
    for (int row : {pairA, pairB})
        for (int dx : {-7, 7}) {
            oxTop(p, x0 + dx + jx, row, frame);
            if (how != Crossing::Ferry && inWater(row)) {
                wake(p, x0 + dx, row, 8);
                if (how == Crossing::Caulk || deep)
                    submerge(img, QRect(x0 + dx - 6, row - 8, 13, 17), top, bottom, 1);   // swimming: mostly under
            }
        }
    rect(p, x0 - 1 + jx, pairB + 8, 1, wy - 27 - pairB - 8, Black);   // chain: pair B back to the tongue
    rect(p, x0 - 1 + jx, pairA + 8, 1, pairB - 9 - pairA - 8, Black); // chain: pair A back to pair B
    wagonTop(p, x0 + jx, wy, yaw, frame);
    if (how != Crossing::Ferry && inWater(wy)) {
        wake(p, x0, wy + 14, 36);
        if (how == Crossing::Caulk)
            rect(p, x0 - 16 + jx, wy - 20, 32, 1, White);      // it rides on the water, no wheels
        else if (deep)
            submerge(img, QRect(x0 - 14, wy - 14, 28, 28), top, bottom, 2);
    }

    // A swamping spills the cargo; the current carries it off.
    if (swamped && pr > 0.40) {
        double s = std::min(1.0, (pr - 0.40) / 0.45);
        double spawnY = ferry ? raftY : 222 - 0.40 * 250;
        for (int i = 0; i < 6; ++i) {
            int ix = int(x0 + (i - 2.5) * 5 + s * (30 + 14 * i)), iy = int(spawnY + (i % 3 - 1) * 8 + std::sin(frame * 0.5 + i) * 1.5);
            if (pr > 0.88 && ((ix + iy) & 1)) continue;           // fading away
            rect(p, ix, iy, i % 2 ? 4 : 3, 3, i % 2 ? Orange : White);
            if (i % 2 == 0) rect(p, ix, iy, 3, 1, Black);
        }
    }
    // And a drowning: ripples where it happened, and a hat.
    if (drowned && pr > 0.50) {
        double s = pr - 0.50;
        double spawnY = ferry ? raftY : 222 - 0.50 * 250;
        int hx = x0 + 22, hy = int(spawnY) - 6;
        p.setBrush(Qt::NoBrush); p.setPen(White);
        for (int r = 0; r < 3; ++r) {
            int rad = int(s * 46) - r * 7;
            if (rad > 1) p.drawEllipse(QPoint(hx, hy), rad, rad / 2 + 1);
        }
        rect(p, hx + int(s * 40) - 4, hy - 1, 8, 3, Black);          // the hat, drifting off
        rect(p, hx + int(s * 40) - 2, hy - 3, 4, 2, Black);
    }
}

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
    wagon(p, 40, 150, 0);
    ox(p, 100, 150, 0);
    ox(p, 132, 150, 0);
    rect(p, 127, 139, 5, 1, Black);
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
