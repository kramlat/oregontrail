// SPDX-License-Identifier: GPL-3.0-or-later
//
// "1985 rules" engine: professions, a party of five, pace and rations,
// landmarks, river crossings, per-person illness and the end-of-trail score.
//
// This is NOT a transcode: no source for the 1985 release is public. The feature
// set follows the documented game; every number lives in tuning.h so it can be
// balanced in one place. Qt-free and non-blocking: the UI drives it one action
// (a day of travel, a rest, a crossing) at a time and shows what comes back.
#pragma once

#include "oregon.h"   // Rng

#include <array>
#include <string>
#include <vector>

namespace trail {

using oregon::Rng;

enum class Profession { Banker, Carpenter, Farmer };
enum class Pace { Steady, Strenuous, Grueling };
enum class Rations { Filling, Meager, BareBones };
enum class Illness { None, Cholera, Dysentery, Measles, Typhoid, Exhaustion, Fever, Snakebite, BrokenLeg };
enum class Health { Good, Fair, Poor, VeryPoor, Dead };
enum class Terrain { Plains, Hills, Rockies, Desert, BlueMountains, Cascades };
enum class Kind { Start, Landmark, Fort, River, End };
enum class Weather { Hot, Warm, Cool, Cold, Rain, Snow };
enum class Method { Ford, Caulk, Ferry, Wait };

struct Landmark {
    const char *name;
    int mile;
    Kind kind;
};

const std::vector<Landmark> &landmarks();
Terrain terrainAt(int mile);
const char *name(Illness);
const char *name(Health);
const char *name(Weather);
const char *name(Pace);
const char *name(Rations);
const char *name(Profession);

struct Member {
    std::string name;
    double damage = 0;          // 0 = perfect; 100 = dead
    Illness illness = Illness::None;
    bool alive = true;
    Health health() const;
};

struct Supplies {
    int oxen = 0;               // individual animals; sold in yokes of two
    double food = 0;            // pounds
    int clothing = 0;           // sets
    int bullets = 0;
    int wheels = 0, axles = 0, tongues = 0;
    double cash = 0;
};

struct River {
    double widthFt = 0, depthFt = 0;
    double ferryCost = 5;
};

// One thing that happened; shown in the UI's message line / log.
struct Note {
    std::string text;
    bool bad = false;
};

enum class Item { Oxen, Food, Clothing, Bullets, Wheel, Axle, Tongue };

class Trail {
public:
    Trail(Rng &rng, Profession p, int startMonth /*3..7*/, std::array<std::string, 5> names);

    // --- Store (Independence, and the forts) ---
    // Price of one unit: a yoke of two oxen, a pound of food, a set of clothes,
    // a box of 20 bullets, one spare part.
    double price(Item) const;
    bool buy(Item, int units);

    // --- The road ---
    std::vector<Note> travelDay();            // one day on the trail
    std::vector<Note> rest(int days);         // rest in place
    // The land ahead: 0 = next landmark's index, miles to it.
    int nextIndex() const { return next_; }
    int milesToNext() const;
    bool atLandmark() const { return atLandmark_; }
    const Landmark &here() const;             // valid when atLandmark()
    void leave() { atLandmark_ = false; }

    // --- Rivers ---
    const River &river() const { return river_; }
    std::vector<Note> cross(Method);

    // --- Hunting: the UI plays the minigame and reports pounds brought back.
    // Costs the bullets fired and a day of the party's time.
    std::vector<Note> huntDay(double pounds, int bulletsUsed);
    double simulatedHuntPounds();             // for bots/tests: no UI

    // --- Settings ---
    Pace pace = Pace::Steady;
    Rations rations = Rations::Filling;

    // --- State (read by the UI) ---
    Profession profession;
    std::array<Member, 5> party;
    Supplies sup;
    int day = 0;                              // days since the 1st of the start month
    int startMonth;
    double miles = 0;
    Weather weather = Weather::Warm;
    double tempF = 60;
    bool won = false, wiped = false;
    int livingCount() const;
    int dayOfMonth() const;
    int month() const;                        // 1..12 (1848)
    std::string dateString() const;
    int score() const;

private:
    Rng &rng_;
    int next_ = 1;
    bool atLandmark_ = false;
    River river_;
    int deadline_ = 0;   // day number of Jan 1 1849: arrive before it or perish

    double r() { return rng_.next(); }
    void rollWeather();
    void dailyHealth(bool resting, std::vector<Note> &n);
    void randomEvent(std::vector<Note> &n);
    void arrive(std::vector<Note> &n);
    void rollRiver();
    void kill(Member &m, std::vector<Note> &n, const char *cause);
    Member *randomLiving();
    bool winter() const;
    bool mountainous() const;
    double speed() const;
    void spendFood(std::vector<Note> &n);
    void passDay(bool resting, std::vector<Note> &n);   // food, health, the date
};

} // namespace trail
