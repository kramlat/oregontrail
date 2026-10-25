// SPDX-License-Identifier: GPL-3.0-or-later
#include "trail/trail.h"

#include <cstdio>
#include <random>

using namespace trail;

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

static const std::array<std::string, 5> kNames = {"A", "B", "C", "D", "E"};

static void testStore()
{
    MtRng r(1);
    Trail t(r, Profession::Farmer, 4, kNames);
    CHECK(t.sup.cash == 400);
    CHECK(t.buy(Item::Oxen, 3));            // 3 yokes = 6 oxen, $120
    CHECK(t.sup.oxen == 6 && t.sup.cash == 280);
    CHECK(!t.buy(Item::Food, 100000));      // can't afford
    CHECK(t.sup.cash == 280);
    CHECK(t.buy(Item::Bullets, 2) && t.sup.bullets == 40);
    Trail b(r, Profession::Banker, 3, kNames);
    CHECK(b.sup.cash == 1600);
    Trail c(r, Profession::Carpenter, 3, kNames);
    CHECK(c.sup.cash == 800);
}

static void testCalendar()
{
    MtRng r(2);
    Trail t(r, Profession::Banker, 3, kNames);
    CHECK(t.dateString() == "March 1, 1848");
    t.sup.food = 10000;
    t.rest(31);
    CHECK(t.dateString() == "April 1, 1848");
}

static void testNoOxenNoTravel()
{
    MtRng r(3);
    Trail t(r, Profession::Banker, 3, kNames);
    t.sup.food = 1000;
    auto n = t.travelDay();
    CHECK(t.miles == 0 && !n.empty());
}

static void testDeadline()
{
    MtRng r(4);
    Trail t(r, Profession::Banker, 7, kNames);
    t.sup.food = 1e6;
    t.sup.clothing = 50;
    for (auto &m : t.party) m.damage = 0;
    t.rest(200);   // sit in Independence until the snow comes
    CHECK(t.wiped || t.livingCount() > 0);
    t.rest(200);
    CHECK(t.wiped);
}

static void testScoreMultiplier()
{
    MtRng r(5);
    Trail b(r, Profession::Banker, 3, kNames), f(r, Profession::Farmer, 3, kNames);
    b.sup.cash = f.sup.cash = 0;
    CHECK(f.score() == 3 * b.score());
}

// Any sequence of actions: the invariants hold and games end.
static void testFuzz()
{
    int won = 0, lost = 0;
    for (unsigned seed = 1; seed <= 1500; ++seed) {
        MtRng r(seed);
        Trail t(r, Profession(seed % 3), 3 + int(seed % 5), kNames);
        t.buy(Item::Oxen, 1 + int(seed % 5));
        t.buy(Item::Food, 100 + int(seed % 400));
        t.buy(Item::Bullets, int(seed % 8));
        t.buy(Item::Clothing, int(seed % 6));
        for (int step = 0; step < 800 && !t.won && !t.wiped; ++step) {
            t.pace = Pace(r.next() * 3);
            t.rations = Rations(r.next() * 3);
            if (t.atLandmark()) {
                if (t.here().kind == Kind::River) {
                    t.cross(Method(int(r.next() * 4)));
                    if (!t.atLandmark()) continue;
                    if (r.next() < 0.5) t.rest(1);   // "wait" leaves us at the bank
                    else t.leave();
                    continue;
                }
                t.buy(Item::Food, int(r.next() * 50));
                t.leave();
            }
            double k = r.next();
            if (k < 0.1) t.rest(1 + int(r.next() * 3));
            else if (k < 0.2) t.huntDay(t.simulatedHuntPounds(), 10);
            else t.travelDay();
            CHECK(t.sup.food >= 0 && t.sup.oxen >= 0 && t.sup.bullets >= 0 && t.sup.clothing >= 0);
            CHECK(t.livingCount() >= 0 && t.livingCount() <= 5);
            CHECK(t.miles >= 0 && t.miles <= 2040);
        }
        (t.won ? won : lost)++;
    }
    std::printf("fuzz: %d arrived, %d lost\n", won, lost);
    CHECK(won > 0 && lost > 0);
}

int main()
{
    testStore();
    testCalendar();
    testNoOxenNoTravel();
    testDeadline();
    testScoreMultiplier();
    testFuzz();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
