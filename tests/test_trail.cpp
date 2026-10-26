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
    // All the jobs, by menu order: richest first, and a lower purse never earns a lower multiplier.
    const double cash[kProfessions] = {1600, 1200, 1200, 800, 800, 800, 400, 400};
    double lastMult = 0;
    for (int i = 0; i < kProfessions; ++i) {
        Trail p(r, Profession(i), 4, kNames);
        CHECK(p.sup.cash == cash[i]);
        CHECK(p.scoreMultiplier() >= lastMult);
        lastMult = p.scoreMultiplier();
    }
    Trail teacher(r, Profession::Teacher, 4, kNames), doctor(r, Profession::Doctor, 4, kNames);
    CHECK(teacher.sup.cash == 400 && doctor.sup.cash == 1200);
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

static void testNoOxen()
{
    MtRng r(3);
    {   // at Independence with money: told to buy a team, not stranded
        Trail t(r, Profession::Banker, 3, kNames);
        t.sup.food = 1000;
        auto n = t.travelDay();
        CHECK(t.miles == 0 && !n.empty() && !t.wiped);
    }
    {   // one ox still pulls the wagon, slowly
        Trail t(r, Profession::Banker, 3, kNames);
        t.sup.food = 1000; t.sup.oxen = 1;
        Trail full(r, Profession::Banker, 3, kNames);
        full.sup.food = 1000; full.sup.oxen = 6;
        CHECK(t.speed() > 0 && t.speed() < full.speed() / 4);
        t.travelDay();
        CHECK(t.miles > 0);
    }
    {   // out on the trail with none left and nowhere to buy: stranded
        Trail t(r, Profession::Banker, 3, kNames);
        t.sup.food = 1000; t.sup.oxen = 2;
        for (int i = 0; i < 5; ++i) t.travelDay();
        CHECK(t.miles > 0 && !t.atLandmark());
        t.sup.oxen = 0;
        t.travelDay();
        CHECK(t.wiped);
    }
    {   // at a fort with the cash for a yoke: not stranded
        Trail t(r, Profession::Banker, 3, kNames);
        t.sup.food = 1000; t.sup.oxen = 6;
        for (int i = 0; i < 80 && !(t.atLandmark() && t.here().kind == Kind::Fort) && !t.wiped; ++i) {
            if (t.atLandmark()) { if (t.here().kind == Kind::River) t.cross(Method::Ferry); if (t.atLandmark()) t.leave(); }
            t.travelDay();
        }
        if (t.atLandmark() && t.here().kind == Kind::Fort) {
            t.sup.oxen = 0;
            t.travelDay();
            CHECK(!t.wiped);
        }
    }
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

// Graves from earlier games are met once, as an event, when the wagon passes their mile.
static void testGraves()
{
    MtRng r(11);
    Trail t(r, Profession::Banker, 4, kNames);
    t.buy(Item::Oxen, 4); t.buy(Item::Food, 1500); t.buy(Item::Clothing, 5);
    t.setGraves({{"Anna", "cholera", "July 4, 1848", "Gone west", 50},
                 {"Ben", "drowning", "June 1, 1848", "", 50},        // two on the same mile
                 {"Cy", "snakebite", "May 5, 1848", "", 150},
                 {"Far", "typhoid", "May 6, 1848", "", 1900}});
    CHECK(t.takePassedGraves().empty());
    int events = 0, seen = 0;
    bool announced = false;
    for (int i = 0; i < 40 && !t.wiped && t.miles < 400; ++i) {
        if (t.atLandmark()) {
            if (t.here().kind == Kind::River) { t.cross(Method::Ferry); if (t.atLandmark()) t.leave(); }
            else t.leave();
        }
        double before = t.miles;
        auto notes = t.travelDay();
        for (auto &n : notes) announced |= n.text.find("You pass the grave of") != std::string::npos;
        auto passed = t.takePassedGraves();
        for (auto &g : passed) {
            CHECK(before < g.mile && g.mile <= t.miles);       // only when the wagon actually crosses the mile
            ++seen;
        }
        events += !passed.empty();
        CHECK(t.takePassedGraves().empty());                     // reported once
    }
    CHECK(announced && seen == 3 && events == 2);                // Anna+Ben together, then Cy; Far is too far away
    // Resting in place never meets a grave, and neither does going over the same ground twice.
    Trail u(r, Profession::Banker, 4, kNames);
    u.buy(Item::Oxen, 4); u.buy(Item::Food, 1500);
    u.setGraves({{"X", "x", "d", "", 0}});                        // at the start: miles must *cross* it
    u.rest(3);
    CHECK(u.takePassedGraves().empty());
    u.travelDay();
    CHECK(u.takePassedGraves().empty());
}

// A drowning is buried on the far bank: a later wagon meets the stone after crossing, not on arrival.
static void testRiverGrave()
{
    MtRng r(13);
    Trail t(r, Profession::Banker, 4, kNames);
    t.buy(Item::Oxen, 4); t.buy(Item::Food, 1500); t.buy(Item::Clothing, 5);
    // March to the first river (Kansas River, mile 102).
    for (int i = 0; i < 40 && !(t.atLandmark() && t.here().kind == Kind::River) && !t.wiped; ++i) t.travelDay();
    CHECK(t.atLandmark() && t.here().kind == Kind::River && int(t.miles) == 102);
    CHECK(t.burialMile("drowning") == 103);        // far bank
    CHECK(t.burialMile("cholera") == 102);         // anything else: where it happened

    // A second wagon, with that drowning on file, reaches the river without meeting the stone...
    Trail u(r, Profession::Banker, 4, kNames);
    u.buy(Item::Oxen, 4); u.buy(Item::Food, 1500); u.buy(Item::Clothing, 5);
    u.setGraves({{"Ben", "drowning", "May 1, 1848", "", 103}});
    bool metBeforeRiver = false;
    for (int i = 0; i < 40 && !(u.atLandmark() && u.here().kind == Kind::River) && !u.wiped; ++i) {
        u.travelDay();
        metBeforeRiver |= !u.takePassedGraves().empty();
    }
    CHECK(!metBeforeRiver && u.atLandmark());
    // ...and meets it on the far side, once it has crossed and moved on.
    u.cross(Method::Ferry);
    CHECK(u.takePassedGraves().empty());
    bool metAfter = false;
    for (int i = 0; i < 4 && !u.wiped; ++i) {
        if (u.atLandmark()) u.leave();
        u.travelDay();
        metAfter |= !u.takePassedGraves().empty();
    }
    CHECK(metAfter);
}

// Fights: killing everything costs nothing; killing nothing costs something.
static void testEncounters()
{
    int bandits = 0, wolves = 0;
    for (unsigned seed = 1; seed <= 4000 && (bandits < 20 || wolves < 20); ++seed) {
        MtRng r(seed);
        Trail t(r, Profession::Banker, 4, kNames);
        t.buy(Item::Oxen, 4); t.buy(Item::Food, 1500); t.buy(Item::Bullets, 5); t.buy(Item::Clothing, 5);
        for (int i = 0; i < 40 && t.pendingEncounter() == Encounter::None && !t.wiped; ++i) t.travelDay();
        Encounter e = t.pendingEncounter();
        if (e == Encounter::None) continue;
        CHECK(t.encounterSize() >= 3 && t.encounterSize() <= 5);
        Supplies before = t.sup;
        // A clean win: nothing lost.
        Trail copy = t;   // (Trail holds a reference to the Rng, so the copy shares dice - fine here)
        t.resolveEncounter(t.encounterSize(), 5);
        CHECK(t.pendingEncounter() == Encounter::None);
        CHECK(t.sup.cash >= before.cash && t.sup.bullets == before.bullets - 5);
        // Not fighting back: the bandits take something / the wolves bite.
        copy.resolveEncounter(0, 0);
        if (e == Encounter::Bandits) { ++bandits; CHECK(copy.sup.cash < before.cash || copy.sup.food < before.food); }
        else { ++wolves; CHECK(copy.sup.food < before.food); }
    }
    CHECK(bandits >= 1 && wolves >= 1);
}

// Return fire: hits wound the shooter, wagon hits cost food.
static void testReturnFireAftermath()
{
    for (unsigned seed = 1; seed <= 4000; ++seed) {
        MtRng r(seed);
        Trail t(r, Profession::Banker, 4, kNames);
        t.buy(Item::Oxen, 4); t.buy(Item::Food, 1500); t.buy(Item::Bullets, 5); t.buy(Item::Clothing, 5);
        for (int i = 0; i < 40 && t.pendingEncounter() != Encounter::Bandits && !t.wiped; ++i) t.travelDay();
        if (t.pendingEncounter() != Encounter::Bandits) continue;
        double dmg = t.party[0].damage, food = t.sup.food;
        t.resolveEncounter(t.encounterSize(), 5, 2, 3);     // won the fight, but took 2 hits and 3 hit the wagon
        CHECK(t.party[0].damage >= dmg + 40);
        CHECK(t.sup.food <= food - 24 + 20 + 1);             // wagon hits cost 8 each (wolf meat bonus doesn't apply)
        return;
    }
    CHECK(false);   // never saw a bandit encounter
}

// Any sequence of actions: the invariants hold and games end.
static void testFuzz()
{
    int won = 0, lost = 0;
    for (unsigned seed = 1; seed <= 1500; ++seed) {
        MtRng r(seed);
        Trail t(r, Profession(seed % kProfessions), 3 + int(seed % 5), kNames);
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
    testNoOxen();
    testDeadline();
    testScoreMultiplier();
    testGraves();
    testRiverGrave();
    testEncounters();
    testReturnFireAftermath();
    testFuzz();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
