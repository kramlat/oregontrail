// SPDX-License-Identifier: GPL-3.0-or-later
//
// Transcode of OREGON (Creative Computing, May/June 1978). The comment at the
// head of each block gives the original line range so it can be checked against
// docs/original/oregon-1978-creative-computing.bas. The order of RND(1) calls is
// kept identical to the original, so a scripted Rng reproduces BASIC runs.
#include "oregon.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace oregon {

namespace {

struct Died {};

double INT(double v) { return std::floor(v); }

std::string upper(std::string s)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    for (char &c : s)
        c = char(std::toupper((unsigned char)c));
    return s;
}

// BASIC's PRINT of a number, minus the padding spaces.
std::string fmt(double v)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.6g", v);
    return buf;
}

std::string tab(int n) { return std::string(std::max(0, n - 1), ' '); }

const char *const kDates[] = {
    "APRIL 12", "APRIL 26", "MAY 10", "MAY 24", "JUNE 7", "JUNE 21", "JULY 5",
    "JULY 19", "AUGUST 2", "AUGUST 16", "AUGUST 31", "SEPTEMBER 13",
    "SEPTEMBER 27", "OCTOBER 11", "OCTOBER 25", "NOVEMBER 8", "NOVEMBER 22",
    "DECEMBER 6", "DECEMBER 20",
};

} // namespace

Game::Game(Frontend &fe, Rng &rng, Options opt) : fe_(fe), rng_(rng), opt_(opt) {}

std::string Game::prompt(const std::string &p) { return upper(fe_.ask(p)); }

// INPUT of a number; BASIC says "?REDO FROM START" on anything else.
double Game::number(const std::string &p)
{
    for (;;) {
        std::string s = prompt(p);
        try {
            size_t used = 0;
            double v = std::stod(s, &used);
            if (used == s.size())
                return v;
        } catch (...) {
        }
        fe_.line("?REDO FROM START");
    }
}

Outcome Game::play()
{
    try {
        instructions();
        chooseMarksmanship();
        initialPurchases();
        fe_.line("MONDAY MARCH 29 1847");
        fe_.line();

        for (bool firstTurn = true;; firstTurn = false) {
            if (!firstTurn) {                     // 1230
                if (M >= 2040) {
                    finalTurn();
                    return Outcome::Arrived;
                }
                dateAndTurn();
            }
            beginTurn();
            chooseAction();
            eat();
            riders();
            event();
            mountains();
        }
    } catch (const Died &) {
        return Outcome::Died;
    }
}

// 160-680
void Game::instructions()
{
    if (prompt("DO YOU NEED INSTRUCTIONS (YES/NO)") == "NO")
        return;
    static const char *const text[] = {
        "", "",
        "THIS PROGRAM SIMULATES A TRIP OVER THE OREGON TRAIL FROM",
        "INDEPENDENCE, MISSOURI TO OREGON CITY, OREGON IN 1847.",
        "YOUR FAMILY OF FIVE WILL COVER THE 2040 MILE OREGON TRAIL",
        "IN 5-6 MONTHS --- IF YOU MAKE IT ALIVE.",
        "",
        "YOU HAD SAVED $900 TO SPEND FOR THE TRIP, AND YOU'VE JUST",
        "   PAID $200 FOR A WAGON.",
        "YOU WILL NEED TO SPEND THE REST OF YOUR MONEY ON THE",
        "   FOLLOWING ITEMS:",
        "",
        "     OXEN - YOU CAN SPEND $200-$300 ON YOUR TEAM",
        "            THE MORE YOU SPEND, THE FASTER YOU'LL GO",
        "               BECAUSE YOU'LL HAVE BETTER ANIMALS",
        "",
        "     FOOD - THE MORE YOU HAVE, THE LESS CHANCE THERE",
        "               IS OF GETTING SICK",
        "",
        "     AMMUNITION - $1 BUYS A BELT OF 50 BULLETS",
        "            YOU WILL NEED BULLETS FOR ATTACKS BY ANIMALS",
        "               AND BANDITS, AND FOR HUNTING FOOD",
        "",
        "     CLOTHING - THIS IS ESPECIALLY IMPORTANT FOR THE COLD",
        "               WEATHER YOU WILL ENCOUNTER WHEN CROSSING",
        "               THE MOUNTAINS",
        "",
        "     MISCELLANEOUS SUPPLIES - THIS INCLUDES MEDICINE AND",
        "               OTHER THINGS YOU WILL NEED FOR SICKNESS",
        "               AND EMERGENCY REPAIRS",
        "", "",
        "YOU CAN SPEND ALL YOUR MONEY BEFORE YOU START YOUR TRIP -",
        "OR YOU CAN SAVE SOME OF YOUR CASH TO SPEND AT FORTS ALONG",
        "THE WAY WHEN YOU RUN LOW. HOWEVER, ITEMS COST MORE AT",
        "THE FORTS. YOU CAN ALSO GO HUNTING ALONG THE WAY TO GET",
        "MORE FOOD.",
        "WHENEVER YOU HAVE TO USE YOUR TRUSTY RIFLE ALONG THE WAY,",
        "YOU WILL BE TOLD TO TYPE IN A WORD (ONE THAT SOUNDS LIKE A ",
        "GUN SHOT). THE FASTER YOU TYPE IN THAT WORD AND HIT THE",
        "\"RETURN\" KEY, THE BETTER LUCK YOU'LL HAVE WITH YOUR GUN.",
        "",
        "AT EACH TURN, ALL ITEMS ARE SHOWN IN DOLLAR AMOUNTS",
        "EXCEPT BULLETS",
        "WHEN ASKED TO ENTER MONEY AMOUNTS, DON'T USE A \"$\".",
        "",
        "GOOD LUCK!!!",
    };
    for (const char *l : text)
        fe_.line(l);
}

// 690-790
void Game::chooseMarksmanship()
{
    fe_.line();
    fe_.line();
    fe_.line("HOW GOOD A SHOT ARE YOU WITH YOUR RIFLE?");
    fe_.line("  (1) ACE MARKSMAN,  (2) GOOD SHOT,  (3) FAIR TO MIDDLIN'");
    fe_.line("         (4) NEED MORE PRACTICE,  (5) SHAKY KNEES");
    fe_.line("ENTER ONE OF THE ABOVE -- THE BETTER YOU CLAIM YOU ARE, THE");
    fe_.line("FASTER YOU'LL HAVE TO BE WITH YOUR GUN TO BE SUCCESSFUL.");
    D9 = int(number(""));
    if (D9 > 5)
        D9 = 0;
}

// 800-1220
void Game::initialPurchases()
{
    X1 = -1;
    D3 = 0; M9 = 0; M = 0; F2 = 0; F1 = 0; S4 = 0; K8 = 0;

    for (;;) {
        fe_.line();
        fe_.line();
        for (;;) {
            A = number("HOW MUCH DO YOU WANT TO SPEND ON YOUR OXEN TEAM");
            if (A < 200) { fe_.line("NOT ENOUGH"); continue; }
            if (A > 300) { fe_.line("TOO MUCH"); continue; }
            break;
        }
        auto nonNegative = [&](const char *what) {
            for (;;) {
                double v = number(std::string("HOW MUCH DO YOU WANT TO SPEND ON ") + what);
                if (v >= 0)
                    return v;
                fe_.line("IMPOSSIBLE");
            }
        };
        F = nonNegative("FOOD");
        B = nonNegative("AMMUNITION");
        C = nonNegative("CLOTHING");
        M1 = nonNegative("MISCELLANEOUS SUPPLIES");
        T = 700 - A - F - B - C - M1;
        if (T >= 0)
            break;
        fe_.line("YOU OVERSPENT--YOU ONLY HAD $700 TO SPEND.  BUY AGAIN");
    }
    B = 50 * B;
    fe_.line("AFTER ALL YOUR PURCHASES. YOU NOW HAVE " + fmt(T) + " DOLLARS LEFT");
    fe_.line();
}

// 1250-1730
void Game::dateAndTurn()
{
    ++D3;
    fe_.line();
    if (D3 > 19) {
        fe_.line("YOU HAVE BEEN ON THE TRAIL TOO LONG  ------");
        fe_.line("YOUR FAMILY DIES IN THE FIRST BLIZZARD OF WINTER");
        epilogue();
    }
    fe_.line(std::string("MONDAY ") + kDates[D3 - 1] + " 1847");
    fe_.line();
}

// 1750-2260
void Game::beginTurn()
{
    F = std::max(F, 0.0);
    B = std::max(B, 0.0);
    C = std::max(C, 0.0);
    M1 = std::max(M1, 0.0);
    if (F < 13)
        fe_.line("YOU'D BETTER DO SOME HUNTING OR BUY FOOD AND SOON!!!!");
    F = INT(F); B = INT(B); C = INT(C); M1 = INT(M1); T = INT(T); M = INT(M);
    M2 = M;

    if (S4 == 1 || K8 == 1) {
        T -= 20;
        if (T < 0) {
            T = 0;
            fe_.line("YOU CAN'T AFFORD A DOCTOR");
            diedOf();
        }
        fe_.line("DOCTOR'S BILL IS $20");
        S4 = 0;
        K8 = 0;
    }

    if (M9 == 1) {
        fe_.line("TOTAL MILEAGE: 950");   // shown, but M itself is left alone
        M9 = 0;
    } else {
        fe_.line("TOTAL MILEAGE: " + fmt(M));
    }
    fe_.line("FOOD: " + fmt(F));
    fe_.line("BULLETS: " + fmt(B));
    fe_.line("CLOTHING: " + fmt(C));
    fe_.line("MISC. SUPPLIES: " + fmt(M1));
    fe_.line("CASH: $" + fmt(T));
}

// 2060-2720
void Game::chooseAction()
{
    enum { Fort = 1, Hunt = 2, Continue = 3 } x;

    if (X1 == -1) {
        // Fort option is off this turn.
        for (;;) {
            double r = number("DO YOU WANT TO (1) HUNT, OR (2) CONTINUE");
            x = (r == 1) ? Hunt : Continue;
            if (x == Continue || B > 39)
                break;
            fe_.line("TOUGH---YOU NEED MORE BULLETS TO GO HUNTING");
        }
        X1 = -X1;
    } else {
        X1 = -X1;
        for (;;) {
            double r = number("DO YOU WANT TO (1) STOP AT THE NEXT FORT, (2) HUNT, OR (3) CONTINUE");
            x = (r == 1) ? Fort : (r == 2) ? Hunt : Continue;
            if (x == Hunt && B <= 39) {
                fe_.line("TOUGH---YOU NEED MORE BULLETS TO GO HUNTING");
                continue;
            }
            break;
        }
    }

    if (x == Fort)
        fort();
    else if (x == Hunt)
        hunt();

    if (F < 13) {
        fe_.line("YOU RAN OUT OF FOOD AND STARVED TO DEATH");
        epilogue();
    }
}

// 2290-2520
void Game::fort()
{
    fe_.line("ENTER WHAT YOU WISH TO SPEND ON THE FOLLOWING");
    auto spend = [&](const char *what) {
        double p = number(what);
        if (p < 0)
            return opt_.fixNegativeFortSpend ? 0.0 : p;   // original: refund!
        T -= p;
        if (T < 0) {
            fe_.line("YOU DON'T HAVE THAT MUCH--KEEP YOUR SPENDING DOWN");
            T += p;
            p = 0;
        }
        return p;
    };
    F += 2.0 / 3 * spend("FOOD");
    B = INT(B + 2.0 / 3 * spend("AMMUNITION") * 50);
    C += 2.0 / 3 * spend("CLOTHING");
    M1 += 2.0 / 3 * spend("MISCELLANEOUS SUPPLIES");
    M -= 45;
}

// 2540-2710
void Game::hunt()
{
    M -= 45;
    double b1 = shoot();
    if (b1 <= 1) {
        fe_.line("RIGHT BETWEEN THE EYES---YOU GOT A BIG ONE!!!!");
        fe_.line("FULL BELLIES TONIGHT!");
        F += 52 + rng_.next() * 6;
        B -= 10 + rng_.next() * 4;
    } else if (100 * rng_.next() < 13 * b1) {
        fe_.line("YOU MISSED---AND YOUR DINNER GOT AWAY.....");
    } else {
        F += 48 - 2 * b1;
        fe_.line("NICE SHOT--RIGHT ON TARGET--GOOD EATIN' TONIGHT!!");
        B -= 10 + 3 * b1;
    }
}

// 2750-2871
void Game::eat()
{
    for (;;) {
        fe_.line("DO YOU WANT TO EAT (1) POORLY  (2) MODERATELY");
        double r = number("OR (3) WELL");
        if (r > 3 || r < 1)
            continue;
        E = int(INT(r));
        F -= 8 + 5 * E;
        if (F >= 0)
            break;
        F += 8 + 5 * E;
        fe_.line("YOU CAN'T EAT THAT WELL");
    }
    M += 200 + (A - 220) / 5 + 10 * rng_.next();
    C1 = 0;
    L1 = 0;
}

// 2890-3540
void Game::riders()
{
    double q = std::pow(M / 100 - 4, 2);
    if (rng_.next() * 10 > (q + 72) / (q + 12) - 1)
        return;

    fe_.text("RIDERS AHEAD.  THEY ");
    int hostileFlag = 0;                       // S5: 1 means they're friendly
    if (rng_.next() >= .8) {
        fe_.text("DON'T ");
        hostileFlag = 1;
    }
    fe_.line("LOOK HOSTILE");
    fe_.line("TACTICS");
    int t1;
    for (;;) {
        fe_.line("(1) RUN  (2) ATTACK  (3) CONTINUE  (4) CIRCLE WAGONS");
        if (rng_.next() <= .2)
            hostileFlag = 1 - hostileFlag;     // they can fool you either way
        double r = number("");
        if (r >= 1 && r <= 4) {
            t1 = int(INT(r));
            break;
        }
    }

    if (hostileFlag == 1) {                    // 3330: friendly
        switch (t1) {
        case 1: M += 15; A -= 10; break;
        case 2: M -= 5; B -= 100; break;
        case 3: break;
        case 4: M -= 20; break;
        }
    } else {                                   // hostile
        bool shot = false;                     // reach the 3140 shoot-out result?
        switch (t1) {
        case 1:
            M += 20; M1 -= 15; B -= 150; A -= 40;
            break;
        case 2:
            B1 = shoot();
            B -= B1 * 40 + 80;
            shot = true;
            break;
        case 3:
            if (rng_.next() > .8) {
                fe_.line("THEY DID NOT ATTACK");
                return;                        // 3450 -> events
            }
            B -= 150; M1 -= 15;
            break;
        case 4:
            B1 = shoot();
            B -= B1 * 30 + 80;
            M -= 25;
            shot = true;
            break;
        }
        if (shot) {
            if (B1 <= 1) {
                fe_.line("NICE SHOOTING---YOU DROVE THEM OFF");
            } else if (B1 > 4) {
                fe_.line("LOUSY SHOT---YOU GOT KNIFED");
                K8 = 1;
                fe_.line("YOU HAVE TO SEE OL' DOC BLANCHARD");
            } else {
                fe_.line("KINDA SLOW WITH YOUR COLT .45");
            }
        }
    }

    if (hostileFlag == 1) {
        fe_.line("RIDERS WERE FRIENDLY, BUT CHECK FOR POSSIBLE LOSSES");
    } else {
        fe_.line("RIDERS WERE HOSTILE--CHECK FOR LOSES");
        if (B < 0) {
            fe_.line("YOU RAN OUT OF BULLETS AND GOT MASSACRED BY THE RIDERS");
            epilogue();
        }
    }
}

// 3550-4700
void Game::event()
{
    static const int cutoff[15] = {6, 11, 13, 15, 17, 22, 32, 35, 37, 42, 44, 54, 64, 69, 95};
    double r1 = 100 * rng_.next();
    int d1 = 1;
    while (d1 <= 15 && r1 > cutoff[d1 - 1])
        ++d1;

    switch (d1) {
    case 1:
        fe_.line("WAGON BREAKS DOWN--LOSE TIME AND SUPPLIES FIXING IT");
        M -= 15 + 5 * rng_.next();
        M1 -= 8;
        break;
    case 2:
        fe_.line("0X INJURES LEG---SLOWS YOU DOWN REST OF TRIP");
        M -= 25;
        A -= 20;
        break;
    case 3:
        fe_.line("BACK LUCK---YOUR DAUGHTER BROKE HER ARM");
        fe_.line("YOU HAD TO STOP AND USE SUPPLIES TO MAKE A SLING");
        M -= 5 + 4 * rng_.next();
        M1 -= 2 + 3 * rng_.next();
        break;
    case 4:
        fe_.line("OX WANDERS OFF---SPEND TIME LOOKING FOR IT");
        M -= 17;
        break;
    case 5:
        fe_.line("YOUR SON GETS LOST---SPEND HALF THE DAY LOOKING FOR HIM");
        M -= 10;
        break;
    case 6:
        fe_.line("UNSAFE WATER--LOSE TIME LOOKING FOR CLEAN SPRING");
        M -= 10 * rng_.next() + 2;
        break;
    case 7:
        if (M > 950) {                         // 4490: cold weather
            fe_.text("COLD WEATHER---BRRRRRRR!---YOU ");
            if (!(C > 22 + 4 * rng_.next())) {
                fe_.text("DONT'T ");
                C1 = 1;
            }
            fe_.line("HAVE ENOUGH CLOTHING TO KEEP YOU WARM");
            if (C1 != 0)
                illness();
        } else {
            fe_.line("HEAVY RAINS---TIME AND SUPPLIES LOST");
            F -= 10;
            B -= 500;
            M1 -= 15;
            M -= 10 * rng_.next() + 5;
        }
        break;
    case 8:
        fe_.line("BANDITS ATTACK");
        B1 = shoot();
        B -= 20 * B1;
        if (B < 0) {
            fe_.line("YOU RAN OUT OF BULLETS---THEY GET LOTS OF CASH");
            T /= 3;
        } else if (B1 <= 1) {
            fe_.line("QUICKEST DRAW OUTSIDE OF DODGE CITY!!!");
            fe_.line("YOU GOT 'EM!");
            break;
        }
        fe_.line("YOU GOT SHOT IN THE LEG AND THEY TOOK ONE OF YOUR OXEN");
        K8 = 1;
        fe_.line("BETTER HAVE A DOC LOOK AT YOUR WOUND");
        M1 -= 5;
        A -= 20;
        break;
    case 9:
        fe_.line("THERE WAS A FIRE IN YOUR WAGON--FOOD AND SUPPLIES DAMAGE!");
        F -= 40;
        B -= 400;
        M1 -= rng_.next() * 8 + 3;
        M -= 15;
        break;
    case 10:
        fe_.line("LOSE YOUR WAY IN HEAVY FOG---TIME IS LOST");
        M -= 10 + 5 * rng_.next();
        break;
    case 11:
        fe_.line("YOU KILLED A POISONOUS SNAKE AFTER IT BIT YOU");
        B -= 10;
        M1 -= 5;
        if (M1 < 0) {
            fe_.line("YOU DIE OF SNAKEBITE SINCE YOU HAVE NO MEDICINE");
            epilogue();
        }
        break;
    case 12:
        fe_.line("WAGON GETS SWAMPED FORDING RIVER--LOSE FOOD AND CLOTHES");
        F -= 30;
        C -= 20;
        M -= 20 + 20 * rng_.next();
        break;
    case 13: {
        fe_.line("WILD ANIMALS ATTACK!");
        double b1 = shoot();
        if (B <= 39) {
            fe_.line("YOU WERE TOO LOW ON BULLETS--");
            fe_.line("THE WOLVES OVERPOWERED YOU");
            K8 = 1;
            diedOf();
        }
        fe_.line(b1 > 2 ? "SLOW ON THE DRAW---THEY GOT AT YOUR FOOD AND CLOTHES"
                        : "NICE SHOOTIN' PARTNER---THEY DIDN'T GET MUCH");
        B -= 20 * b1;
        C -= b1 * 4;
        F -= b1 * 8;
        break;
    }
    case 14:
        fe_.line("HAIL STORM---SUPPLIES DAMAGED");
        M -= 5 + rng_.next() * 10;
        B -= 200;
        M1 -= 4 + rng_.next() * 3;
        break;
    case 15:
        // Eating poorly makes illness certain; the odds for E=2 vs E=3 are as
        // listed (they look swapped, but that is what the original does).
        if (E == 1 || (E == 2 && rng_.next() > .25) || (E == 3 && rng_.next() < .5))
            illness();
        break;
    default:
        fe_.line("HELPFUL INDIANS SHOW YOU WHERE TO FIND MORE FOOD");
        F += 14;
        break;
    }
}

// 4710-4960
void Game::mountains()
{
    if (M <= 950)
        return;

    double q = std::pow(M / 100 - 15, 2);
    if (!(rng_.next() * 10 > 9 - (q + 72) / (q + 12))) {
        fe_.line("RUGGED MOUNTAINS");
        if (rng_.next() <= .1) {
            fe_.line("YOU GOT LOST---LOSE VALUABLE TIME TRYING TO FIND TRAIL!");
            M -= 60;
        } else if (rng_.next() <= .11) {
            fe_.line("WAGON DAMAGED!---LOSE TIME AND SUPPLIES");
            M1 -= 5;
            B -= 200;
            M -= 20 + 30 * rng_.next();
        } else {
            fe_.line("THE GOING GETS SLOW");
            M -= 45 + rng_.next() / .02;
        }
    }

    bool blizzard = false;
    if (F1 != 1) {                             // South Pass
        F1 = 1;
        if (rng_.next() < .8)
            blizzard = true;
        else
            fe_.line("YOU MADE IT SAFELY THROUGH SOUTH PASS--NO SNOW");
    }
    if (!blizzard && M >= 1700 && F2 != 1) {   // Blue Mountains
        F2 = 1;
        if (rng_.next() < .7)
            blizzard = true;
    }
    if (blizzard) {                            // 4970
        fe_.line("BLIZZARD IN MOUNTAIN PASS--TIME AND SUPPLIES LOST");
        L1 = 1;
        F -= 25;
        M1 -= 10;
        B -= 300;
        M -= 30 + 40 * rng_.next();
        if (C < 18 + 2 * rng_.next())
            illness();
    }
    if (M <= 950)                              // 4940: knocked back over the line
        M9 = 1;
}

// 6300-6440
void Game::illness()
{
    if (100 * rng_.next() < 10 + 35 * (E - 1)) {
        fe_.line("WILD ILLNESS---MEDICINE USED");
        M -= 5;
        M1 -= 2;
    } else if (100 * rng_.next() < 100 - 40 / std::pow(4, E - 1)) {
        fe_.line("BAD ILLNESS---MEDICINE USED");
        M -= 5;
        M1 -= 5;
    } else {
        fe_.line("SERIOUS ILLNESS---");
        fe_.line("YOU MUST STOP FOR MEDICAL ATTENTION");
        M1 -= 10;
        S4 = 1;
    }
    if (M1 < 0) {
        fe_.line("YOU RAN OUT OF MEDICAL SUPPLIES");
        diedOf();
    }
}

// 6140-6280
double Game::shoot()
{
    static const char *const words[] = {"BANG", "BLAM", "POW", "WHAM"};
    std::string word = words[int(rng_.next() * 4)];
    Frontend::Shot s = fe_.shoot(word);
    fe_.line();
    double b1 = s.seconds + opt_.shootingSlack;
    b1 -= (D9 - 1);
    if (b1 <= 0)
        b1 = 0;
    if (upper(s.typed) != word)
        b1 = 9;
    return b1;
}

// 5120-5160
void Game::diedOf()
{
    fe_.text("YOU DIED OF ");
    fe_.line(K8 == 1 ? "INJURIES" : "PNEUMONIA");
    epilogue();
}

// 5170-5410
void Game::epilogue()
{
    fe_.line();
    fe_.line("DUE TO YOUR UNFORTUNATE SITUATION, THERE ARE A FEW");
    fe_.line("FORMALITIES WE MUST GO THROUGH");
    fe_.line();
    prompt("WOULD YOU LIKE A MINISTER?");
    prompt("WOULD YOU LIKE A FANCY FUNERAL?");
    if (prompt("WOULD YOU LIKE US TO INFORM YOUR NEXT OF KIN?") == "YES") {
        fe_.line("THAT WILL BE $4.50 FOR THE TELEGRAPH CHARGE.");
    } else {
        fe_.line("BUT YOUR AUNT SADIE IN ST. LOUIS IS REALLY WORRIED ABOUT YOU");
    }
    fe_.line();
    fe_.line("WE THANK YOU FOR THIS INFORMATION AND WE ARE SORRY YOU");
    fe_.line("DIDN'T MAKE IT TO THE GREAT TERRITORY OF OREGON");
    fe_.line("BETTER LUCK NEXT TIME");
    fe_.line();
    fe_.line();
    fe_.line(tab(30) + "SINCERELY");
    fe_.line();
    fe_.line(tab(17) + "THE OREGON CITY CHAMBER OF COMMERCE");
    throw Died{};
}

// 5430-6120
void Game::finalTurn()
{
    F9 = (2040 - M2) / (M - M2);
    F += (1 - F9) * (8 + 5 * E);
    fe_.line();
    fe_.line("YOU FINALLY ARRIVED AT OREGON CITY");
    fe_.line("AFTER 2040 LONG MILES---HOORAY!!!!!");
    fe_.line("A REAL PIONEER!");
    fe_.line();

    int f9 = int(INT(F9 * 14));
    int d3 = D3 * 14 + f9;
    ++f9;
    if (f9 >= 8)
        f9 -= 7;
    static const char *const days[] = {"MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY",
                                       "FRIDAY", "SATURDAY", "SUNDAY"};
    // ON..GOTO falls through to the first entry when out of range.
    fe_.text(std::string(f9 >= 1 && f9 <= 7 ? days[f9 - 1] : days[0]) + " ");

    static const struct { int upTo; int base; const char *name; } months[] = {
        {124, 93, "JULY"}, {155, 124, "AUGUST"}, {185, 155, "SEPTEMBER"},
        {216, 185, "OCTOBER"}, {246, 216, "NOVEMBER"},
    };
    const char *name = "DECEMBER";
    int base = 246;
    for (auto &m : months) {
        if (d3 <= m.upTo) { name = m.name; base = m.base; break; }
    }
    fe_.line(std::string(name) + " " + std::to_string(d3 - base) + " 1847");
    fe_.line();

    B = std::max(B, 0.0); C = std::max(C, 0.0); M1 = std::max(M1, 0.0);
    T = std::max(T, 0.0); F = std::max(F, 0.0);
    fe_.line("FOOD: " + fmt(F));
    fe_.line("BULLETS: " + fmt(B));
    fe_.line("CLOTHING: " + fmt(C));
    fe_.line("MISC. SUPP.: " + fmt(M1));
    fe_.line("CASH: " + fmt(T));
    fe_.line();
    fe_.line(tab(11) + "PRESIDENT JAMES K. POLK SENDS YOU HIS");
    fe_.line(tab(17) + "HEARTIEST CONGRATULATIONS");
    fe_.line();
    fe_.line(tab(11) + "AND WISHES YOU A PROSPEROUS LIFE AHEAD");
    fe_.line();
    fe_.line(tab(22) + "AT YOUR NEW HOME");
}

} // namespace oregon
