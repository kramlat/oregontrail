// SPDX-License-Identifier: GPL-3.0-or-later
#include "trail.h"
#include "tuning.h"

#include <algorithm>
#include <cmath>

namespace trail {

namespace {

// Distances are the commonly published figures for the route; treat as approximate.
const std::vector<Landmark> kLandmarks = {
    {"Independence, Missouri", 0, Kind::Start},
    {"Kansas River Crossing", 102, Kind::River},
    {"Big Blue River Crossing", 185, Kind::River},
    {"Fort Kearney", 304, Kind::Fort},
    {"Chimney Rock", 554, Kind::Landmark},
    {"Fort Laramie", 640, Kind::Fort},
    {"Independence Rock", 830, Kind::Landmark},
    {"South Pass", 932, Kind::Landmark},
    {"Fort Bridger", 989, Kind::Fort},
    {"Green River Crossing", 1070, Kind::River},
    {"Soda Springs", 1151, Kind::Landmark},
    {"Fort Hall", 1288, Kind::Fort},
    {"Snake River Crossing", 1431, Kind::River},
    {"Fort Boise", 1534, Kind::Fort},
    {"Blue Mountains", 1640, Kind::Landmark},
    {"Fort Walla Walla", 1710, Kind::Fort},
    {"The Dalles", 1830, Kind::Landmark},
    {"Oregon City, Oregon", 2040, Kind::End},
};

const int kMonthDays[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};   // 1848
const char *const kMonthNames[12] = {"January", "February", "March", "April", "May", "June", "July",
                                      "August", "September", "October", "November", "December"};
const double kMonthTempF[12] = {25, 30, 42, 55, 65, 74, 80, 78, 68, 55, 40, 28};

} // namespace

const std::vector<Landmark> &landmarks() { return kLandmarks; }

Terrain terrainAt(int mile)
{
    if (mile < 554) return Terrain::Plains;
    if (mile < 932) return Terrain::Hills;
    if (mile < 1288) return Terrain::Rockies;
    if (mile < 1640) return Terrain::Desert;
    if (mile < 1830) return Terrain::BlueMountains;
    return Terrain::Cascades;
}

const char *name(Illness i)
{
    static const char *n[] = {"healthy", "cholera", "dysentery", "measles", "typhoid", "exhaustion",
                              "a fever", "a snakebite", "a broken leg"};
    return n[int(i)];
}
const char *name(Health h)
{
    static const char *n[] = {"good", "fair", "poor", "very poor", "dead"};
    return n[int(h)];
}
const char *name(Weather w)
{
    static const char *n[] = {"hot", "warm", "cool", "cold", "rainy", "snowy"};
    return n[int(w)];
}
const char *name(Pace p)
{
    static const char *n[] = {"steady", "strenuous", "grueling"};
    return n[int(p)];
}
const char *name(Rations r)
{
    static const char *n[] = {"filling", "meager", "bare bones"};
    return n[int(r)];
}
const char *name(Profession p)
{
    static const char *n[] = {"Banker", "Carpenter", "Farmer"};
    return n[int(p)];
}

Health Member::health() const
{
    if (!alive) return Health::Dead;
    if (damage < 20) return Health::Good;
    if (damage < 45) return Health::Fair;
    if (damage < 75) return Health::Poor;
    return Health::VeryPoor;
}

Trail::Trail(Rng &rng, Profession p, int startMonth, std::array<std::string, 5> names)
    : profession(p), startMonth(std::clamp(startMonth, 3, 7)), rng_(rng)
{
    for (int i = 0; i < 5; ++i)
        party[i].name = names[i];
    sup.cash = p == Profession::Banker ? tune::kBanker : p == Profession::Carpenter ? tune::kCarpenter : tune::kFarmer;
    for (int m = startMonth - 1; m < 12; ++m) deadline_ += kMonthDays[m];   // January 1st, 1849
    rollWeather();
}

// --- store ---------------------------------------------------------------

double Trail::price(Item it) const
{
    double base = 0;
    switch (it) {
    case Item::Oxen: base = tune::kYoke; break;
    case Item::Food: base = tune::kFoodLb; break;
    case Item::Clothing: base = tune::kClothes; break;
    case Item::Bullets: base = tune::kBullets; break;
    default: base = tune::kPart; break;
    }
    return base * (1 + tune::kFortMarkupPer1000Miles * miles / 1000);
}

bool Trail::buy(Item it, int units)
{
    if (units <= 0) return false;
    double cost = price(it) * units;
    if (cost > sup.cash + 1e-9) return false;
    sup.cash -= cost;
    switch (it) {
    case Item::Oxen: sup.oxen += 2 * units; break;
    case Item::Food: sup.food += units; break;
    case Item::Clothing: sup.clothing += units; break;
    case Item::Bullets: sup.bullets += 20 * units; break;
    case Item::Wheel: sup.wheels += units; break;
    case Item::Axle: sup.axles += units; break;
    case Item::Tongue: sup.tongues += units; break;
    }
    return true;
}

// --- calendar ------------------------------------------------------------

int Trail::month() const
{
    int m = startMonth - 1, d = day;
    while (d >= kMonthDays[m % 12]) {
        d -= kMonthDays[m % 12];
        ++m;
    }
    return m % 12 + 1;
}

int Trail::dayOfMonth() const
{
    int m = startMonth - 1, d = day;
    while (d >= kMonthDays[m % 12]) {
        d -= kMonthDays[m % 12];
        ++m;
    }
    return d + 1;
}

std::string Trail::dateString() const
{
    return std::string(kMonthNames[month() - 1]) + " " + std::to_string(dayOfMonth()) + ", 1848";
}

bool Trail::winter() const
{
    int m = month();
    return m >= 11 || m <= 3;
}

bool Trail::mountainous() const
{
    Terrain t = terrainAt(int(miles));
    return t == Terrain::Rockies || t == Terrain::BlueMountains || t == Terrain::Cascades;
}

int Trail::livingCount() const
{
    int n = 0;
    for (auto &m : party) n += m.alive;
    return n;
}

Member *Trail::randomLiving()
{
    int n = livingCount();
    if (n == 0) return nullptr;
    int k = std::min(n - 1, int(r() * n));
    for (auto &m : party)
        if (m.alive && k-- == 0) return &m;
    return nullptr;
}

// --- the road ------------------------------------------------------------

int Trail::milesToNext() const
{
    if (next_ >= int(kLandmarks.size())) return 0;
    return int(std::ceil(kLandmarks[next_].mile - miles));
}

const Landmark &Trail::here() const { return kLandmarks[std::max(0, next_ - 1)]; }

double Trail::speed() const
{
    double s = tune::kBaseMilesPerDay;
    s *= std::min(1.0, double(sup.oxen) / tune::kOxenForFullSpeed);
    s *= tune::kPaceSpeed[int(pace)];
    if (weather == Weather::Rain) s *= 0.85;
    if (weather == Weather::Snow) s *= 0.5;
    if (mountainous()) {
        s *= 0.8;
        if (winter()) s *= tune::kWinterMountainSpeed;
    }
    return s;
}

void Trail::rollWeather()
{
    double t = kMonthTempF[month() - 1];
    Terrain ter = terrainAt(int(miles));
    if (ter == Terrain::Rockies) t -= 10;
    if (ter == Terrain::Desert) t += 6;
    if (ter == Terrain::Cascades) t -= 4;
    tempF = t + (r() - 0.5) * 24;
    bool precip = r() < 0.16;
    if (precip) weather = tempF < 32 ? Weather::Snow : Weather::Rain;
    else if (tempF > 85) weather = Weather::Hot;
    else if (tempF > 60) weather = Weather::Warm;
    else if (tempF > 40) weather = Weather::Cool;
    else weather = Weather::Cold;
}

void Trail::kill(Member &m, std::vector<Note> &n, const char *cause)
{
    m.alive = false;
    m.illness = Illness::None;
    n.push_back({m.name + " has died of " + cause + ".", true});
    if (&m == &party[0] || livingCount() == 0)
        wiped = true;
}

void Trail::spendFood(std::vector<Note> &n)
{
    double need = livingCount() * tune::kRationLb[int(rations)];
    if (sup.food >= need) {
        sup.food -= need;
        return;
    }
    sup.food = 0;
    n.push_back({"You have run out of food and your party is starving!", true});
    for (auto &m : party)
        if (m.alive) m.damage += tune::kStarvingDamage;
}

void Trail::dailyHealth(bool resting, std::vector<Note> &n)
{
    for (auto &m : party) {
        if (!m.alive) continue;
        double d = tune::kRationDamage[int(rations)];
        if (!resting) d += tune::kPaceDamage[int(pace)];
        else d -= tune::kRestHeal;
        if (tempF < 40) {
            // Warm clothes blunt the cold, but don't remove it.
            double covered = std::min(1.0, double(sup.clothing) / livingCount());
            d += tune::kColdDamage * (40 - tempF) / 40 * (1 - 0.75 * covered);
            if (winter() && mountainous()) d += tune::kSnowboundDamage;
        }

        if (m.illness != Illness::None) {
            d += tune::kIllnessDamage;
            double rec = tune::kRecoverChance + (resting ? tune::kRecoverRestBonus : 0);
            if (m.illness == Illness::Snakebite) rec = 0.35;
            if (r() < rec) {
                n.push_back({m.name + " is feeling better.", false});
                m.illness = Illness::None;
            } else if (r() < tune::kDeathChance[int(m.illness)] * (0.5 + m.damage / 60)) {
                kill(m, n, name(m.illness));
                continue;
            }
        } else if (r() < tune::kOnsetBase + tune::kOnsetPerDamage * m.damage) {
            static const Illness pool[] = {Illness::Dysentery, Illness::Dysentery, Illness::Cholera, Illness::Measles,
                                           Illness::Typhoid, Illness::Fever, Illness::Fever, Illness::Exhaustion};
            Illness ill = pool[std::min(7, int(r() * 8))];
            if (ill == Illness::Exhaustion && m.damage < 40) ill = Illness::Fever;
            m.illness = ill;
            n.push_back({m.name + " has " + name(ill) + ".", true});
        }
        m.damage = std::max(0.0, m.damage + d);
        if (m.damage >= 100) kill(m, n, m.illness != Illness::None ? name(m.illness) : "exhaustion");
    }
}

void Trail::passDay(bool resting, std::vector<Note> &n)
{
    spendFood(n);
    dailyHealth(resting, n);
    ++day;
    if (day >= deadline_ && !won) {
        n.push_back({"You have been on the trail too long. The first blizzard of winter buries your party.", true});
        wiped = true;
        return;
    }
    rollWeather();
}

std::vector<Note> Trail::rest(int days)
{
    std::vector<Note> n;
    for (int i = 0; i < days && !wiped; ++i) passDay(true, n);
    return n;
}

std::vector<Note> Trail::travelDay()
{
    std::vector<Note> n;
    if (wiped || won) return n;
    if (sup.oxen < 2) {
        n.push_back({"You have too few oxen to pull the wagon. Buy or trade for more.", true});
        return n;
    }
    leave();
    double step = speed();
    double target = next_ < int(kLandmarks.size()) ? kLandmarks[next_].mile : miles;
    bool arriving = miles + step >= target;
    miles = arriving ? target : miles + step;
    passDay(false, n);
    if (wiped) return n;
    if (arriving) {
        arrive(n);
        return n;
    }
    randomEvent(n);
    return n;
}

void Trail::arrive(std::vector<Note> &n)
{
    const Landmark &l = kLandmarks[next_];
    ++next_;
    atLandmark_ = true;
    n.push_back({std::string("You have reached ") + l.name + ".", false});
    if (l.kind == Kind::River) rollRiver();
    if (l.kind == Kind::End) won = true;
}

void Trail::rollRiver()
{
    // Spring melt makes the rivers deeper.
    double season = month() <= 6 ? 1.0 : 0.7;
    river_.widthFt = 80 + r() * 600;
    river_.depthFt = std::max(0.8, (1.5 + r() * 3.0) * season);
    river_.ferryCost = 5;
}

void Trail::randomEvent(std::vector<Note> &n)
{
    if (r() >= tune::kEventChance) return;
    struct Ev { int weight; int id; };
    static const Ev evs[] = {{10, 0}, {8, 1}, {10, 2}, {7, 3}, {4, 4}, {5, 5},
                             {8, 6},  {2, 7}, {6, 8},  {4, 9}, {3, 10}, {3, 11}};
    int total = 0;
    for (auto &e : evs) total += e.weight;
    int pick = int(r() * total), id = 0;
    for (auto &e : evs) {
        if (pick < e.weight) { id = e.id; break; }
        pick -= e.weight;
    }

    Member *m = randomLiving();
    switch (id) {
    case 0:   // bad water
        n.push_back({"Bad water. You lose a day finding a clean spring.", true});
        passDay(true, n);
        if (m && m->illness == Illness::None && r() < 0.3) {
            m->illness = Illness::Dysentery;
            n.push_back({m->name + " has dysentery.", true});
        }
        break;
    case 1:   // storm
        n.push_back({"A thunderstorm. You wait it out for a day.", true});
        passDay(true, n);
        break;
    case 2: { // broken part
        int which = int(r() * 3);
        int &spare = which == 0 ? sup.wheels : which == 1 ? sup.axles : sup.tongues;
        const char *part = which == 0 ? "wheel" : which == 1 ? "axle" : "wagon tongue";
        if (spare > 0) {
            --spare;
            n.push_back({std::string("The wagon broke a ") + part + ". You used a spare and lost a day.", true});
            passDay(true, n);
        } else {
            n.push_back({std::string("The wagon broke a ") + part + " and you have no spare! You lose three days.", true});
            for (int i = 0; i < 3 && !wiped; ++i) passDay(true, n);
        }
        break;
    }
    case 3:
        if (sup.oxen > 0) {
            --sup.oxen;
            n.push_back({"One of your oxen has died.", true});
        }
        break;
    case 4:   // snakebite
        if (m) {
            m->illness = Illness::Snakebite;
            m->damage += 15;
            n.push_back({m->name + " was bitten by a snake.", true});
        }
        break;
    case 5: { // theft
        double lost = std::floor(sup.food * (0.1 + r() * 0.2));
        sup.food -= lost;
        n.push_back({"Thieves came in the night and took " + std::to_string(int(lost)) + " pounds of food.", true});
        break;
    }
    case 6: { // lost trail
        int d = 1 + int(r() * 3);
        n.push_back({"You lost the trail. " + std::to_string(d) + " days wasted.", true});
        for (int i = 0; i < d && !wiped; ++i) passDay(true, n);
        break;
    }
    case 7: { // fire
        sup.clothing = std::max(0, sup.clothing - 2);
        sup.food = std::max(0.0, sup.food - 50);
        n.push_back({"Fire in the wagon! You lose food and clothing.", true});
        break;
    }
    case 8: { // wild fruit
        double f = 20 + r() * 20;
        sup.food += f;
        n.push_back({"You found wild fruit: " + std::to_string(int(f)) + " pounds of food.", false});
        break;
    }
    case 9: { // abandoned wagon
        int which = int(r() * 3);
        (which == 0 ? sup.wheels : which == 1 ? sup.axles : sup.tongues)++;
        n.push_back({"You found an abandoned wagon and took a spare part from it.", false});
        break;
    }
    case 10:
        if (m) {
            m->illness = Illness::BrokenLeg;
            m->damage += 10;
            n.push_back({m->name + " broke a leg.", true});
        }
        break;
    default:
        sup.food += 30;
        n.push_back({"Friendly travelers share 30 pounds of food with you.", false});
        break;
    }
}

// --- hunting ---------------------------------------------------------------

std::vector<Note> Trail::huntDay(double pounds, int bulletsUsed)
{
    std::vector<Note> n;
    sup.bullets = std::max(0, sup.bullets - bulletsUsed);
    // The wagon can't carry more than 200 lb of meat home.
    double carried = std::min(pounds, 200.0);
    sup.food += carried;
    n.push_back({"You brought back " + std::to_string(int(carried)) + " pounds of meat.", false});
    passDay(true, n);
    return n;
}

double Trail::simulatedHuntPounds()
{
    // A competent hunter: a few animals out of a handful of shots.
    return std::floor(40 + r() * 120);
}

// --- rivers ----------------------------------------------------------------

std::vector<Note> Trail::cross(Method m)
{
    std::vector<Note> n;
    const double depth = river_.depthFt;
    if (m == Method::Wait) {
        n = rest(1);
        river_.depthFt = std::max(0.8, depth + (r() - 0.55) * 0.8);
        return n;
    }
    double fail = 0, drown = 0;
    if (m == Method::Ford) {
        fail = depth <= 2.5 ? 0.05 : depth <= 3.5 ? 0.30 : 0.65;
        drown = 0.30;
    } else if (m == Method::Caulk) {
        fail = 0.08 + 0.05 * std::max(0.0, depth - 2.0);
        drown = 0.20;
    } else {
        if (sup.cash < river_.ferryCost) {
            n.push_back({"You can't afford the ferry.", true});
            return n;
        }
        sup.cash -= river_.ferryCost;
        fail = 0.01;
        drown = 0.05;
        // Ferries are slow when the river is up.
        int wait = 1 + int(r() * (depth > 3 ? 3 : 1));
        for (int i = 0; i < wait && !wiped; ++i) passDay(true, n);
    }
    if (m != Method::Ferry) passDay(false, n);

    if (wiped) return n;
    if (r() < fail) {
        n.push_back({"The wagon was swamped!", true});
        double food = std::floor(sup.food * (0.2 + r() * 0.4));
        sup.food -= food;
        sup.clothing = std::max(0, sup.clothing - int(r() * 3));
        sup.bullets -= int(sup.bullets * r() * 0.5);
        n.push_back({"You lost " + std::to_string(int(food)) + " pounds of food and other supplies.", true});
        if (r() < drown) {
            if (Member *v = randomLiving()) kill(*v, n, "drowning");
        }
    } else {
        n.push_back({"You made it across safely.", false});
    }
    leave();
    return n;
}

// --- score -------------------------------------------------------------------

int Trail::score() const
{
    int pts = 0;
    for (auto &m : party)
        if (m.alive) pts += tune::kScoreHealth[int(m.health())];
    pts += 50;   // the wagon
    pts += sup.oxen * 4 + (sup.wheels + sup.axles + sup.tongues) * 2 + sup.clothing * 2;
    pts += sup.bullets / 50 + int(sup.food / 25) + int(sup.cash / 5);
    int mult = profession == Profession::Banker ? 1 : profession == Profession::Carpenter ? 2 : 3;
    return pts * mult;
}

} // namespace trail
