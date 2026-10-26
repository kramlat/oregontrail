// SPDX-License-Identifier: GPL-3.0-or-later
#include "fight/fight.h"

#include <cmath>
#include <cstdio>
#include <random>

using namespace fight;

static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } \
    } while (0)

struct MtRng : oregon::Rng {
    std::mt19937 g;
    explicit MtRng(unsigned s) : g(s) {}
    double next() override { return std::uniform_real_distribution<double>(0, 1)(g); }
};

// A frozen target `dist` ahead of the player, `bearing` radians off the line of sight.
static void place(Field &f, double dist, double bearing, Kind k = Kind::Bandit)
{
    Enemy e{k, f.px + std::cos(f.angle + bearing) * dist, f.py + std::sin(f.angle + bearing) * dist};
    e.pause = 1e9;
    f.enemies.push_back(e);
    ++f.spawned;
}

static void fly(Field &f, double seconds)
{
    for (double t = 0; t < seconds; t += 0.01) f.update(0.01);
}

// A clear row far from the wagon, nothing in the way.
static void clearRow(Field &f)
{
    f.props.clear();
    f.px = 3.5; f.py = 21.5; f.angle = 0;
}

static void testBulletsTravel()
{
    MtRng r(1);
    {   // hits take time: nothing dies on the instant of firing
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        place(f, 8, 0);
        CHECK(f.fire() && f.bullets == 4 && f.killed == 0);
        f.update(0.05);
        CHECK(f.killed == 0 && f.flying.size() == 1);
        fly(f, 0.5);
        CHECK(f.killed == 1 && f.flying.empty());
    }
    {   // off to the side: the round misses, keeps flying, and ends at the forest edge
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        place(f, 8, 0.5);
        f.fire();
        fly(f, 1.5);
        CHECK(f.killed == 0 && f.flying.empty());
    }
    {   // a tree between you and the target stops the round; a bush does too
        for (bool tree : {true, false}) {
            Field f(Kind::Bandit, 1, 5, r);
            clearRow(f);
            f.props.push_back({7.0, 21.5, tree, 0, tree ? 0.4 : 0.55});
            place(f, 9, 0);
            f.fire();
            fly(f, 1.0);
            CHECK(f.killed == 0);
        }
        Field g(Kind::Bandit, 1, 5, r);          // same shot, no cover: kills
        clearRow(g);
        place(g, 9, 0);
        g.fire();
        fly(g, 1.0);
        CHECK(g.killed == 1);
    }
    {   // the nearer of two in line is the one that dies
        Field f(Kind::Wolf, 2, 5, r);
        clearRow(f);
        place(f, 9, 0, Kind::Wolf); place(f, 4, 0, Kind::Wolf);
        f.fire();
        fly(f, 1.0);
        CHECK(!f.enemies[1].alive && f.enemies[0].alive && f.killed == 1);
    }
}

static void testReturnFire()
{
    MtRng r(5);
    {   // a bandit with a clear line to a standing player shoots at them
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        place(f, 7, 0);
        f.enemies[0].fireIn = 0.05;
        fly(f, 0.2);
        CHECK(f.enemyShots >= 1);
    }
    {   // the bandit's gun flashes when it fires
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        place(f, 7, 0);
        f.enemies[0].fireIn = 0.0;
        f.update(0.01);
        CHECK(f.enemyShots == 1 && f.enemies[0].sinceShot < 0.05);
    }
    {   // no line of sight (a tree in the way): no shot
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        f.props.push_back({6.0, 21.5, true, 0, 0.4});
        place(f, 9, 0);
        f.enemies[0].fireIn = 0.05;
        fly(f, 1.0);
        CHECK(f.enemyShots == 0);
    }
    {   // wolves never shoot
        Field f(Kind::Wolf, 1, 5, r);
        clearRow(f);
        place(f, 7, 0, Kind::Wolf);
        fly(f, 3.0);
        CHECK(f.enemyShots == 0);
    }
    {   // a hostile bullet that reaches the player is a hit; one that reaches the wagon is a wagon hit
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        f.flying.push_back({6.5, 21.5, -1, 0, 1.0, true});
        fly(f, 0.5);
        CHECK(f.hits == 1 && f.flying.empty());
        f.flying.push_back({f.wx + 4, f.wy, -1, 0, 1.0, true});
        fly(f, 0.8);
        CHECK(f.wagonHits == 1 && f.hits == 1);
    }
    {   // moving out of the way works: a bullet aimed at the old spot misses
        Field f(Kind::Bandit, 1, 5, r);
        clearRow(f);
        f.flying.push_back({10.5, 21.5, -1, 0, 1.5, true});
        f.py = 19.5;                               // stepped aside
        fly(f, 1.0);
        CHECK(f.hits == 0);
    }
    {   // three hits put you down: the fight ends and the survivors count as having reached the wagon
        Field f(Kind::Bandit, 3, 5, r);
        clearRow(f);
        place(f, 7, 0); place(f, 8, 0);
        f.spawned = 2;
        for (int i = 0; i < 3; ++i) f.flying.push_back({4.5 + i * 0.3, 21.5, -1, 0, 1.0, true});
        fly(f, 0.2);
        CHECK(f.downed && f.done());
        CHECK(f.killed + f.reached == f.total);
    }
}

static void testRifle()
{
    MtRng r(2);
    Field f(Kind::Bandit, 1, 1, r);
    CHECK(f.fire());
    CHECK(!f.fire());                       // cooling down
    f.update(1.0);
    CHECK(!f.fire() && f.bullets == 0);     // out of bullets
}

static void testWalking()
{
    MtRng r(3);
    Field f(Kind::Bandit, 0, 0, r);
    f.props.clear();
    f.px = 3.5; f.py = 12.5; f.angle = 0;
    f.walk(1, 0);                           // forward = east
    for (int i = 0; i < 30; ++i) f.update(0.02);
    CHECK(f.px > 3.5 && std::abs(f.py - 12.5) < 1e-6);
    double x0 = f.px;
    f.walk(0, 1);                           // strafe right of east = south (y grows)
    for (int i = 0; i < 30; ++i) f.update(0.02);
    CHECK(f.py > 12.5 && std::abs(f.px - x0) < 1e-6);
    // Can't walk through the forest edge or the wagon.
    f.walk(-1, 0);
    for (int i = 0; i < 400; ++i) f.update(0.02);
    CHECK(f.px >= 1.0);
    Field g(Kind::Bandit, 0, 0, r);
    g.props.clear();
    g.px = g.wx - 4; g.py = g.wy; g.angle = 0; g.walk(1, 0);
    for (int i = 0; i < 400; ++i) g.update(0.02);
    CHECK(std::hypot(g.px - g.wx, g.py - g.wy) >= 1.0);
    // ...or a tree.
    Field h(Kind::Bandit, 0, 0, r);
    h.props.clear();
    h.props.push_back({8.0, 21.5, true, 0, 0.4});
    h.px = 3.5; h.py = 21.5; h.angle = 0; h.walk(1, 0);
    for (int i = 0; i < 400; ++i) h.update(0.02);
    CHECK(h.px < 8.0 - 0.4);
}

static void testReach()
{
    MtRng r(4);
    Field f(Kind::Wolf, 3, 0, r);                  // no bullets: nobody can be stopped
    for (int i = 0; i < 4000 && !f.done(); ++i) f.update(0.03);
    CHECK(f.done());
    CHECK(f.killed == 0 && f.reached == 3 && f.spawned == 3);
}

// A bot that turns toward the nearest target and shoots when lined up.
static void aimAndShoot(Field &f, bool fromWagon)
{
    const Enemy *near = nullptr; double nd = 1e9;
    for (auto &e : f.enemies) if (e.alive) {
        double d = fromWagon ? std::hypot(e.x - f.wx, e.y - f.wy) : std::hypot(e.x - f.px, e.y - f.py);
        if (d < nd) { nd = d; near = &e; }
    }
    if (!near) return;
    double want = std::atan2(near->y - f.py, near->x - f.px);
    double diff = std::remainder(want - f.angle, 6.2831853);
    f.turn(std::max(-0.12, std::min(0.12, diff)));
    if (std::abs(diff) < 0.04) f.fire();
}

static void testAttackBot()
{
    int stopped = 0, total = 0;
    for (unsigned seed = 1; seed <= 200; ++seed) {
        MtRng r(seed);
        Kind k = seed % 2 ? Kind::Bandit : Kind::Wolf;
        Field f(k, 3 + seed % 3, 25, r);
        for (int i = 0; i < 6000 && !f.done(); ++i) { aimAndShoot(f, true); f.update(0.03); }
        CHECK(f.done());
        CHECK(f.killed + f.reached == f.total);
        stopped += f.killed; total += f.total;
    }
    std::printf("attack bot stopped %d of %d\n", stopped, total);
    CHECK(stopped > total / 3);
}

static void testHunting()
{
    MtRng r(7);
    {   // the field is stocked with game, none of it on top of you
        Field f(Field::HuntTag{}, 20, r);
        CHECK(f.hunting() && f.alive() >= 6 && f.total == f.alive());
        for (auto &e : f.enemies) {
            CHECK(isGame(e.kind));
            CHECK(std::hypot(e.x - f.px, e.y - f.py) >= 7);
        }
    }
    {   // game bolts from a hunter who gets close
        Field f(Field::HuntTag{}, 20, r);
        f.enemies.clear();
        f.props.clear();
        f.px = 6; f.py = 6; f.angle = 0;
        Enemy e{Kind::Deer, 10, 6};                // within its alarm distance
        f.enemies.push_back(e);
        f.total = f.spawned = 1;
        double d0 = std::hypot(e.x - f.px, e.y - f.py);
        fly(f, 0.8);
        CHECK(std::hypot(f.enemies[0].x - f.px, f.enemies[0].y - f.py) > d0 + 1.0);
    }
    {   // a kill gives meat; a bison is worth more than a rabbit
        double got[2];
        int i = 0;
        for (Kind k : {Kind::Rabbit, Kind::Bison}) {
            Field f(Field::HuntTag{}, 20, r);
            f.enemies.clear(); f.props.clear();
            f.px = 3.5; f.py = 21.5; f.angle = 0;
            place(f, 9, 0, k);                      // grazing, beyond its alarm distance (8 rabbit, 6 bison)
            f.enemies[0].heading = 0;
            f.total = 1;
            f.fire();
            fly(f, 0.6);
            got[i++] = f.meat;
        }
        CHECK(got[0] == 2 && got[1] == 350);
    }
    {   // the hunt ends when the bullets are spent, and never before they've landed
        Field f(Field::HuntTag{}, 1, r);
        CHECK(!f.done());
        f.fire();
        CHECK(!f.done());                           // bullet still flying
        fly(f, 2.0);
        CHECK(f.done());
    }
    {   // and on the clock
        Field f(Field::HuntTag{}, 20, r);
        for (int i = 0; i < 2000; ++i) f.update(0.03);
        CHECK(f.done());
    }
    {   // a bot with a rifle brings back meat most of the time
        int fed = 0;
        for (unsigned seed = 1; seed <= 60; ++seed) {
            MtRng rr(seed);
            Field f(Field::HuntTag{}, 20, rr);
            for (int i = 0; i < 1600 && !f.done(); ++i) {
                // Walk toward the nearest animal until it's in range, then aim.
                const Enemy *near = nullptr; double nd = 1e9;
                for (auto &e : f.enemies) if (e.alive) {
                    double d = std::hypot(e.x - f.px, e.y - f.py);
                    if (d < nd) { nd = d; near = &e; }
                }
                if (near && nd > 9) f.walk(1, 0); else f.walk(0, 0);
                aimAndShoot(f, false);
                f.update(0.03);
            }
            fed += f.meat > 0;
        }
        std::printf("hunt bot fed %d of 60\n", fed);
        CHECK(fed > 20);
    }
}

int main()
{
    testBulletsTravel();
    testReturnFire();
    testRifle();
    testWalking();
    testReach();
    testAttackBot();
    testHunting();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
