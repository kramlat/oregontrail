// SPDX-License-Identifier: GPL-3.0-or-later
#include "fightview.h"
#include "pixelart.h"

#include <QPainter>
#include <QPolygon>

#include <algorithm>
#include <cmath>

namespace {

using art::W;
using art::H;
using fight::Field;
using fight::Kind;

struct Billboard {
    double x, y;
    const QImage *tex;
    double worldH;     // height in world cells
    double lift;       // floats this far above the ground
};

// ------------------------------------------------------------------ sprites

QImage blank(int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    return img;
}

struct Pen {
    QPainter p;
    explicit Pen(QImage &img) : p(&img) { p.setRenderHint(QPainter::Antialiasing, false); p.setPen(Qt::NoPen); }
    void r(int x, int y, int w, int h, const QColor &c) { p.fillRect(x, y, w, h, c); }
};

// Front-on bandit with a gun he's pointing at you. `firing` adds the muzzle flash.
QImage makeBandit(int step, bool firing)
{
    QImage img = blank(16, 28);
    Pen d(img);
    d.r(5, 1, 6, 3, art::Black);                 // hat
    d.r(3, 4, 10, 1, art::Black);
    d.r(6, 5, 4, 4, art::Orange);                // face
    d.r(6, 6, 1, 1, art::Black);
    d.r(9, 6, 1, 1, art::Black);
    d.r(6, 8, 4, 2, art::Violet);                // bandana
    d.r(4, 10, 8, 10, art::Black);               // coat
    d.r(4, 16, 8, 1, art::Orange);               // belt
    d.r(2, 11, 2, 7, art::Black);                // left arm
    d.r(12, 11, 2, 6, art::Black);               // right arm, out in front
    d.r(6 + 0, 13, 4, 4, art::White);            // the gun, seen end-on: a steel block with a dark barrel
    d.r(7, 14, 2, 2, art::Black);
    d.r(5 + step * 2, 20, 2, 8, art::Black);     // legs
    d.r(9 - step * 2, 20, 2, 8, art::Black);
    if (firing) {                                // muzzle flash
        d.p.setBrush(art::White);
        d.p.drawPolygon(QPolygon(QList<QPoint>{{8, 8}, {10, 12}, {14, 14}, {10, 16}, {8, 20}, {6, 16}, {2, 14}, {6, 12}}));
        d.r(7, 13, 2, 2, art::Orange);
    }
    return img;
}

QImage fromAnimal(art::Animal a, int w, int h, int ox, int oy, int step)
{
    QImage img = blank(w, h);
    QPainter p(&img);
    art::drawAnimal(p, a, ox, oy, true, step);
    return img;
}

QImage makeTree(int variant)
{
    QImage img = blank(24, 48);
    Pen d(img);
    d.r(10, 30, 4, 18, art::Orange);             // trunk
    d.r(13, 30, 1, 18, art::Black);
    d.p.setPen(art::Black);
    d.p.setBrush(art::Green);
    if (variant == 0) {                          // broad tree
        d.p.drawEllipse(QPoint(12, 17), 11, 15);
        d.p.setPen(Qt::NoPen);
        for (int i = 0; i < 26; ++i)             // shaded leaves on the lower right
            d.r(6 + (i * 7) % 14, 14 + (i * 5) % 16, 1, 1, art::Black);
    } else {                                     // pine
        d.p.drawPolygon(QPolygon(QList<QPoint>{{12, 1}, {20, 16}, {16, 16}, {23, 30}, {1, 30}, {8, 16}, {4, 16}}));
        d.p.setPen(Qt::NoPen);
        for (int i = 0; i < 18; ++i) d.r(5 + (i * 7) % 14, 8 + (i * 5) % 20, 1, 1, art::Black);
    }
    return img;
}

QImage makeBush(int variant)
{
    QImage img = blank(28, 14);
    Pen d(img);
    d.p.setPen(art::Black);
    d.p.setBrush(art::Green);
    d.p.drawEllipse(QPoint(14, 8), 12, 6);
    d.p.setPen(Qt::NoPen);
    d.r(5, 9, 18, 1, art::Black);                // shading under the leaves
    const QColor berry = variant ? art::White : art::Orange;
    for (int i = 0; i < 5; ++i) d.r(7 + (i * 5) % 14, 4 + (i * 3) % 5, 1, 1, berry);
    return img;
}

struct Art {
    QImage bandit[2][2];     // [walk step][firing]
    QImage wolf[2], rabbit[2], deer[2], bison[2];
    QImage tree[2], bush[2], wagon;
    QImage mine, theirs;     // bullets
    Art()
    {
        for (int s = 0; s < 2; ++s) {
            bandit[s][0] = makeBandit(s, false);
            bandit[s][1] = makeBandit(s, true);
            wolf[s] = fromAnimal(art::Animal::Wolf, 22, 13, 1, 1, s);
            rabbit[s] = fromAnimal(art::Animal::Rabbit, 12, 9, 1, 1, s);
            deer[s] = fromAnimal(art::Animal::Deer, 24, 24, 1, 5, s);
            bison[s] = fromAnimal(art::Animal::Bison, 34, 22, 1, 1, s);
            tree[s] = makeTree(s);
            bush[s] = makeBush(s);
        }
        wagon = blank(48, 36);
        QPainter pw(&wagon);
        art::drawWagon(pw, 1, 31, 0);
        mine = QImage(3, 3, QImage::Format_ARGB32);
        mine.fill(art::White);
        mine.setPixel(1, 1, art::Orange.rgb());
        theirs = QImage(3, 3, QImage::Format_ARGB32);     // theirs are orange, with a dark core
        theirs.fill(art::Orange);
        theirs.setPixel(1, 1, art::Black.rgb());
    }
};

const Art &art_()
{
    static const Art a;
    return a;
}

// ------------------------------------------------------------------ scene

inline void put(QImage &img, int x, int y, const QColor &c)
{
    if (x >= 0 && y >= 0 && x < W && y < H)
        img.setPixel(x, y, c.rgb());
}

inline unsigned hash2(int a, int b)
{
    unsigned h = unsigned(a) * 73856093u ^ unsigned(b) * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

void sky(QImage &img, const Field &f)
{
    const int horizon = H / 2;
    QPainter p(&img);
    p.fillRect(0, 0, W, horizon, art::Blue);
    // Mountains and clouds are fixed in the world, so turning visibly moves them.
    const double fov = Field::kFov;
    for (int x = 0; x < W; ++x) {
        double a = f.angle + std::atan((2.0 * x / W - 1) * std::tan(fov / 2));
        int h = int(5 + 7 * std::abs(std::sin(a * 3.0)) + 5 * std::abs(std::sin(a * 7.0 + 1)));
        for (int y = horizon - h - 10; y < horizon - 10; ++y) put(img, x, y, art::Violet);
        if (h > 14) put(img, x, horizon - h - 10, art::White);   // snowy peaks
        for (int y = horizon - 10; y < horizon; ++y)             // the far treeline, behind the trees
            put(img, x, y, ((x * 7 + y * 3) % 5 == 0) ? art::Black : art::Green);
        for (int c = 0; c < 4; ++c) {
            double ca = c * 1.7 + 0.4, d = std::remainder(a - ca, 6.2831853);
            if (std::abs(d) < 0.16) {
                int cy = 18 + (c % 2) * 14;
                put(img, x, cy, art::White);
                if (std::abs(d) < 0.11) put(img, x, cy - 1, art::White);
                if (std::abs(d) < 0.06) put(img, x, cy - 2, art::White);
            }
        }
    }
}

// Floor casting: every pixel below the horizon is a point on the ground. The trail is
// dirt with two wheel ruts; the rest is grass with a few tufts.
void ground(QImage &img, const Field &f)
{
    const int horizon = H / 2;
    const double t = std::tan(Field::kFov / 2);
    const double dirX = std::cos(f.angle), dirY = std::sin(f.angle);
    const double planeX = -std::sin(f.angle) * t, planeY = std::cos(f.angle) * t;
    for (int y = horizon; y < H; ++y) {
        double ty = y == horizon ? 1e9 : H / (2.0 * (y - horizon));    // distance straight ahead of this row
        double fx = f.px + (dirX - planeX) * ty, fy = f.py + (dirY - planeY) * ty;
        double sx = 2 * planeX * ty / W, sy = 2 * planeY * ty / W;
        bool haze = y < horizon + 12;
        for (int x = 0; x < W; ++x, fx += sx, fy += sy) {
            QColor c = art::Green;
            double off = fy - Field::wy;
            if (std::abs(off) < Field::kTrailHalfWidth && fx > -1 && fx < Field::N + 1) {
                c = art::Orange;
                double a = std::abs(off);
                if (a > 0.45 && a < 0.62) c = art::Violet;                   // wheel ruts
                else if (hash2(int(fx * 5), int(fy * 5)) % 11 == 0) c = art::Violet;   // pebbles
            } else if (ty > 2.5 && ty < 12) {
                // Tufts of grass: tiny marks (in world units) on a sparse lattice.
                double gx = fx * 2, gy = fy * 2;
                if (std::abs(gx - std::floor(gx) - 0.5) < 0.05 && std::abs(gy - std::floor(gy) - 0.5) < 0.05 &&
                    hash2(int(std::floor(gx)), int(std::floor(gy))) % 3 == 0)
                    c = art::Black;
            }
            if (haze && ((x + y) & 1)) c = art::Blue;
            put(img, x, y, c);
        }
    }
}

// ------------------------------------------------------------------ billboards

const QImage &targetImage(const fight::Enemy &e, double &worldH)
{
    const Art &a = art_();
    int step = int(e.phase) & 1;
    switch (e.kind) {
    case Kind::Bandit: worldH = 1.05; return a.bandit[step][e.sinceShot < 0.12 ? 1 : 0];
    case Kind::Wolf: worldH = 0.55; return a.wolf[step];
    case Kind::Rabbit: worldH = 0.3; return a.rabbit[step];
    case Kind::Deer: worldH = 1.0; return a.deer[step];
    default: worldH = 1.3; return a.bison[step];
    }
}

void drawBillboards(QImage &img, const Field &f)
{
    const Art &art = art_();
    std::vector<Billboard> boards;
    boards.reserve(220);

    // The forest edge: a ring of trees just outside the playable area (never reachable).
    static const std::vector<Billboard> edge = [&] {
        std::vector<Billboard> v;
        const double spacing = 0.7;
        for (double s = 0.2; s < Field::N; s += spacing) {
            for (int side = 0; side < 4; ++side) {
                unsigned h = hash2(int(s * 10), side);
                double jit = 0.15 + (h % 100) / 100.0 * 0.6;
                double x = side < 2 ? s : (side == 2 ? jit : Field::N - jit);
                double y = side >= 2 ? s : (side == 0 ? jit : Field::N - jit);
                v.push_back({x, y, &art_().tree[(h >> 8) & 1], 2.3 + ((h >> 4) % 5) * 0.1, 0});
            }
        }
        return v;
    }();
    boards.insert(boards.end(), edge.begin(), edge.end());

    boards.push_back({Field::wx, Field::wy, &art.wagon, 1.3, 0});
    for (auto &p : f.props)
        boards.push_back({p.x, p.y, p.tree ? &art.tree[p.variant & 1] : &art.bush[p.variant & 1], p.tree ? 2.2 : 0.65, 0});
    for (auto &e : f.enemies) {
        if (!e.alive) continue;
        double h;
        const QImage &tex = targetImage(e, h);
        boards.push_back({e.x, e.y, &tex, h, 0});
    }
    for (auto &b : f.flying)
        boards.push_back({b.x, b.y, b.hostile ? &art.theirs : &art.mine, b.hostile ? 0.12 : 0.09, 0.45});

    const double t = std::tan(Field::kFov / 2);
    const double dirX = std::cos(f.angle), dirY = std::sin(f.angle);
    const double planeX = -std::sin(f.angle) * t, planeY = std::cos(f.angle) * t;
    const double inv = 1.0 / (planeX * dirY - dirX * planeY);

    struct Item { const Billboard *b; double ty, tx; int screenX, w, h, top; };
    std::vector<Item> items;
    for (auto &b : boards) {
        double sx = b.x - f.px, sy = b.y - f.py;
        double tx = inv * (dirY * sx - dirX * sy);
        double ty = inv * (-planeY * sx + planeX * sy);
        if (ty < 0.2) continue;
        int screenX = int(W / 2 * (1 + tx / ty));
        int h = std::max(1, std::abs(int(H * b.worldH / ty)));
        int w = std::max(1, h * b.tex->width() / b.tex->height());
        if (screenX + w / 2 < 0 || screenX - w / 2 >= W) continue;       // off to the side
        int feetY = int(H / 2 + H / (2 * ty)) - int(H * b.lift / ty);
        items.push_back({&b, ty, tx, screenX, w, h, feetY - h});
    }
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.ty > b.ty; });   // far to near

    for (auto &it : items) {
        const QImage &tex = *it.b->tex;
        for (int sx = 0; sx < it.w; ++sx) {
            int x = it.screenX - it.w / 2 + sx;
            if (x < 0 || x >= W) continue;
            int u = sx * tex.width() / it.w;
            for (int sy = 0; sy < it.h; ++sy) {
                int y = it.top + sy;
                if (y < 0 || y >= H) continue;
                QRgb px = tex.pixel(u, sy * tex.height() / it.h);
                if (qAlpha(px) == 0) continue;
                if (it.ty > 15 && ((x + y) & 1)) px = art::Blue.rgb();    // haze with distance
                img.setPixel(x, y, px);
            }
        }
    }
}

// ------------------------------------------------------------------ rifle and HUD

void rifle(QImage &img, const Field &f)
{
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    int recoil = f.sinceFire < 0.14 ? 6 : 0;
    int bx = W / 2 + 34, by = H + recoil;
    p.setPen(Qt::NoPen);
    p.setBrush(art::Orange);                                       // stock
    p.drawPolygon(QPolygon(QList<QPoint>{{bx + 8, by}, {bx + 26, by}, {bx + 16, by - 30}, {bx + 2, by - 36}}));
    p.setBrush(art::Black);                                        // barrel
    p.drawPolygon(QPolygon(QList<QPoint>{{bx + 2, by - 30}, {bx + 14, by - 26}, {bx - 20, by - 70}, {bx - 24, by - 66}}));
    p.setBrush(art::Violet);
    p.drawRect(bx - 4, by - 36, 12, 6);
    if (f.sinceFire < 0.07) {                                      // muzzle flash
        int fx = bx - 24, fy = by - 70;
        p.setBrush(art::White);
        p.drawPolygon(QPolygon(QList<QPoint>{{fx, fy - 12}, {fx + 4, fy - 4}, {fx + 12, fy}, {fx + 4, fy + 4}, {fx, fy + 10}, {fx - 4, fy + 4}, {fx - 12, fy}, {fx - 4, fy - 4}}));
        p.setBrush(art::Orange);
        p.drawRect(fx - 3, fy - 3, 6, 6);
    }
}

void hud(QImage &img, const Field &f)
{
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);
    int cx = W / 2, cy = H / 2;                                    // crosshair
    p.setPen(art::White);
    p.drawLine(cx - 8, cy, cx - 3, cy);
    p.drawLine(cx + 3, cy, cx + 8, cy);
    p.drawLine(cx, cy - 8, cx, cy - 3);
    p.drawLine(cx, cy + 3, cx, cy + 8);

    if (!f.hunting()) {
        // Attackers out of view: arrows at the screen edge, bigger when closer.
        const double fov = Field::kFov;
        int leftRow = 0, rightRow = 0;
        for (auto &e : f.enemies) {
            if (!e.alive) continue;
            double bearing = std::remainder(std::atan2(e.y - f.py, e.x - f.px) - f.angle, 6.2831853);
            if (std::abs(bearing) < fov / 2 + 0.05) continue;
            double dist = std::hypot(e.x - f.px, e.y - f.py);
            int sz = std::clamp(int(24 / dist) + 3, 3, 9);
            bool right = bearing > 0;
            int &row = right ? rightRow : leftRow;
            int x = right ? W - 4 : 3, y = H / 2 - 40 + row * 14;
            ++row;
            p.setPen(Qt::NoPen);
            p.setBrush(dist < 4 ? art::White : art::Orange);
            int dx = right ? sz : -sz;
            p.drawPolygon(QPolygon(QList<QPoint>{{x, y - sz}, {x + dx, y}, {x, y + sz}}));
        }
    }

    p.fillRect(0, 0, W, 12, art::Black);
    QFont font("monospace");
    font.setPixelSize(9);
    p.setFont(font);
    p.setPen(art::White);
    if (f.hunting()) {
        p.drawText(4, 9, QString("BULLETS %1    MEAT %2 LB    TIME %3    ENTER: DONE")
                             .arg(f.bullets).arg(int(f.meat))
                             .arg(std::max(0, int(Field::kHuntSeconds - f.clock))));
    } else {
        p.drawText(4, 9, QString("BULLETS %1    STOPPED %2 OF %3    HEALTH %4%5")
                             .arg(f.bullets).arg(f.killed).arg(f.total)
                             .arg(QString(std::max(0, Field::kMaxHits - f.hits), '+'))
                             .arg(QString(std::min(f.hits, Field::kMaxHits), '-')));
    }
    if (f.sinceHit < 0.3) {                      // you were hit: flash the screen edge
        p.setPen(Qt::NoPen);
        p.setBrush(art::Orange);
        p.drawRect(0, 12, W, 4);
        p.drawRect(0, H - 4, W, 4);
        p.drawRect(0, 12, 4, H);
        p.drawRect(W - 4, 12, 4, H);
    }
}

} // namespace

void renderFight(QImage &img, const fight::Field &f)
{
    sky(img, f);
    ground(img, f);
    drawBillboards(img, f);
    rifle(img, f);
    hud(img, f);
}
