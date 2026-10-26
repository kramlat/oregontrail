// SPDX-License-Identifier: GPL-3.0-or-later
// Plays the 1985-rules engine with a sensible bot and prints win rates, so the
// numbers in src/trail/tuning.h can be balanced by looking rather than guessing.
#include "trail/trail.h"

#include <cstdio>
#include <cstdlib>
#include <random>

using namespace trail;

struct MtRng : oregon::Rng {
    std::mt19937 g;
    explicit MtRng(unsigned s) : g(s) {}
    double next() override { return std::uniform_real_distribution<double>(0, 1)(g); }
};

struct Result { bool won; int days; int alive; int score; bool leaderDied; bool starved; int month; };

static Result play(unsigned seed, Profession p, int month, bool careful)
{
    MtRng rng(seed);
    Trail t(rng, p, month, {"A", "B", "C", "D", "E"});
    // Shopping list scaled to the purse.
    const bool poor = t.sup.cash <= 400;
    int yokes = poor ? 3 : 4;
    t.buy(Item::Oxen, yokes);
    double budget = t.sup.cash;
    if (careful) {
        t.buy(Item::Wheel, 1); t.buy(Item::Axle, 1); t.buy(Item::Tongue, 1);
    }
    t.buy(Item::Clothing, 5);
    t.buy(Item::Bullets, poor ? 12 : 6);
    int foodLb = int((t.sup.cash - (poor ? 0 : 60)) / t.price(Item::Food));
    t.buy(Item::Food, std::min(foodLb, 1500));
    (void)budget;

    t.rations = Rations::Meager;
    int guard = 0;
    bool starved = false;
    while (!t.won && !t.wiped && guard++ < 600) {
        if (t.atLandmark()) {
            const Landmark &l = t.here();
            if (l.kind == Kind::Fort) {
                int want = int((t.sup.cash - 5) / t.price(Item::Food) * 0.8);
                if (t.sup.food < 500 && want > 0) t.buy(Item::Food, std::min(want, 600));
            }
            if (l.kind == Kind::River) {
                Method m = t.river().depthFt > 3.0 && t.sup.cash >= 5 ? Method::Ferry
                         : t.river().depthFt <= 2.5 ? Method::Ford : Method::Caulk;
                while (t.atLandmark() && !t.wiped) t.cross(m);
                continue;
            }
            t.leave();
        }
        if (t.sup.food <= 0) starved = true;
        // Late starts have no time to dawdle: push on, rest only when really ill.
        bool late = false;
        if (t.pendingEncounter() != Encounter::None) { int k = t.simulatedKills(); t.resolveEncounter(k, 10); }
        // Sick or worn-out party: rest, if the schedule allows.
        double daysLeft = (2040 - t.miles) / 16.0 * 1.15;
        int slack = (t.startMonth <= 5 ? 245 : 214 - 30 * (t.startMonth - 6)) - t.day;

        bool sick = false;
        for (auto &m : t.party)
            if (m.alive && (m.illness != Illness::None || m.health() >= Health::Poor)) sick = true;
        if (sick && careful && daysLeft < slack - 20) { t.rest(2); continue; }
        if (t.sup.food < 40 * t.livingCount() && t.sup.bullets >= 20) {
            t.huntDay(t.simulatedHuntPounds(), 10);
            continue;
        }
        // Slow down for the Rockies and winter risk when the party is weak.
        // Behind schedule for the winter deadline? Push harder.
        t.pace = daysLeft > slack ? Pace::Grueling : daysLeft > slack - 25 ? Pace::Strenuous : Pace::Steady;
        t.travelDay();
    }
    return {t.won, t.day, t.livingCount(), t.won ? t.score() : 0, !t.party[0].alive, starved, t.month()};
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? std::atoi(argv[1]) : 2000;
    bool careful = !(argc > 2 && std::string(argv[2]) == "reckless");
    const Profession profs[] = {Profession::Banker, Profession::Doctor, Profession::Blacksmith, Profession::Teacher};
    std::printf("%-10s %-6s %8s %8s %9s %8s %8s %8s\n", "profession", "month", "arrive%", "avgDays", "avgAlive", "avgScore", "leader+", "starved");
    for (Profession p : profs) {
        for (int month = 3; month <= 7; ++month) {
            int won = 0, leader = 0, starv = 0; double days = 0, alive = 0, score = 0;
            for (int i = 0; i < n; ++i) {
                Result r = play(1000u * month + i, p, month, careful);
                won += r.won; leader += r.leaderDied; starv += r.starved;
                if (r.won) { days += r.days; alive += r.alive; score += r.score; }
            }
            std::printf("%-10s %-6d %7.1f%% %8.0f %9.2f %8.0f %7.0f%% %7.0f%%\n", name(p), month, 100.0 * won / n,
                        won ? days / won : 0, won ? alive / won : 0, won ? score / won : 0,
                        100.0 * leader / n, 100.0 * starv / n);
        }
    }
}
