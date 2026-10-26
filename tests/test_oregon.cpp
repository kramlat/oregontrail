// SPDX-License-Identifier: GPL-3.0-or-later
#include "oregon.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <sstream>

using namespace oregon;

namespace {

int failures = 0;
#define CHECK(c)                                                              \
    do {                                                                      \
        if (!(c)) {                                                           \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c);          \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

struct FixedRng : Rng {
    double v;
    explicit FixedRng(double v) : v(v) {}
    double next() override { return v; }
};

struct MtRng : Rng {
    std::mt19937 g;
    explicit MtRng(unsigned seed) : g(seed) {}
    double next() override { return std::uniform_real_distribution<double>(0, 1)(g); }
};

// Answers prompts by their text; records everything printed.
struct Script : Frontend {
    std::ostringstream out;
    std::function<std::string(const std::string &)> answer;
    double shotSeconds = 0.5;
    bool miss = false;
    int shots = 0;

    void text(const std::string &s) override { out << s; }
    std::string ask(const std::string &p) override
    {
        out << p << "? ";
        return answer(p);
    }
    double shoot(Target) override
    {
        ++shots;
        return miss ? 9 : shotSeconds;
    }
};

std::string stockAnswer(const std::string &p)
{
    auto has = [&](const char *s) { return p.find(s) != std::string::npos; };
    if (has("INSTRUCTIONS")) return "NO";
    if (has("OXEN")) return "250";
    if (has("FOOD") && has("SPEND ON")) return "200";
    if (has("AMMUNITION") && has("SPEND ON")) return "50";
    if (has("CLOTHING") && has("SPEND ON")) return "100";
    if (has("MISCELLANEOUS") && has("SPEND ON")) return "50";
    if (has("HUNT")) return "2";
    if (has("NEXT FORT")) return "3";
    if (has("(3) WELL")) return "2";
    if (has("KIN")) return "YES";
    return "3";   // marksmanship, tactics, funeral questions
}

void testDeterministic()
{
    std::string first;
    for (int run = 0; run < 2; ++run) {
        Script s; s.answer = stockAnswer;
        FixedRng r(0.5);
        Game g(s, r);
        Outcome o = g.play();
        if (run == 0) first = s.out.str(); else CHECK(first == s.out.str());
        CHECK(o == Outcome::Arrived || o == Outcome::Died);
    }
    CHECK(first.find("MONDAY MARCH 29 1847") != std::string::npos);
}

void testBuyingValidation()
{
    Script s;
    int step = 0;
    s.answer = [&](const std::string &p) -> std::string {
        if (p.find("INSTRUCTIONS") != std::string::npos) return "NO";
        if (p.find("OXEN") != std::string::npos) {
            static const char *tries[] = {"100", "400", "abc", "250"};
            return tries[std::min(step++, 3)];
        }
        return stockAnswer(p);
    };
    FixedRng r(0.5);
    Game(s, r).play();
    const std::string o = s.out.str();
    CHECK(o.find("NOT ENOUGH") != std::string::npos);
    CHECK(o.find("TOO MUCH") != std::string::npos);
    CHECK(o.find("?REDO FROM START") != std::string::npos);
}

// Original line 2340: a negative fort purchase shrinks your stock. Buying -1000
// food at the first fort drops food below 13 and starves the family at once.
void testFortNegativeSpend()
{
    for (bool fix : {false, true}) {
        Script s;
        s.answer = [&](const std::string &p) -> std::string {
            if (p.find("NEXT FORT") != std::string::npos) return "1";
            if (p == "FOOD") return "-1000";
            if (p == "AMMUNITION" || p == "CLOTHING" || p == "MISCELLANEOUS SUPPLIES") return "0";
            return stockAnswer(p);
        };
        FixedRng r(0.5);
        Options opt; opt.fixNegativeFortSpend = fix;
        Game(s, r, opt).play();
        const std::string o = s.out.str();
        size_t fort = o.find("ENTER WHAT YOU WISH TO SPEND");
        CHECK(fort != std::string::npos);
        size_t next = o.find("TOTAL MILEAGE", fort);
        size_t starved = o.find("STARVED", fort);
        CHECK((starved != std::string::npos && starved < next) == !fix);
    }
}

// A missed shot is always worst-case (B1=9) and a miss at that.
void testWrongWord()
{
    Script s; s.answer = stockAnswer; s.miss = true;
    s.answer = [&](const std::string &p) -> std::string {
        if (p.find("HUNT") != std::string::npos) return "1";
        return stockAnswer(p);
    };
    FixedRng r(0.5);
    Game(s, r).play();
    CHECK(s.out.str().find("YOU MISSED---AND YOUR DINNER GOT AWAY") != std::string::npos);
    CHECK(s.shots > 0);
}

// Whatever the dice and the player do, a game must end, and never crash.
void testFuzz()
{
    int arrived = 0, died = 0;
    for (unsigned seed = 1; seed <= 3000; ++seed) {
        MtRng rng(seed);
        std::mt19937 pick(seed * 7919u);
        Script s;
        s.shotSeconds = std::uniform_real_distribution<double>(0, 4)(pick);
        s.answer = [&](const std::string &p) -> std::string {
            auto has = [&](const char *x) { return p.find(x) != std::string::npos; };
            if (has("INSTRUCTIONS")) return "NO";
            if (has("OXEN")) return std::to_string(200 + int(pick() % 101));
            if (has("SPEND ON")) return std::to_string(int(pick() % 100));
            if (has("HUNT") || has("NEXT FORT")) return std::to_string(1 + int(pick() % 3));
            if (has("WELL")) return std::to_string(1 + int(pick() % 3));
            if (p == "FOOD" || p == "AMMUNITION" || p == "CLOTHING" || p == "MISCELLANEOUS SUPPLIES")
                return std::to_string(int(pick() % 60));
            if (p.empty()) return std::to_string(1 + int(pick() % 5));
            return "YES";
        };
        (Game(s, rng).play() == Outcome::Arrived ? arrived : died)++;
    }
    std::printf("fuzz: %d arrived, %d died\n", arrived, died);
    CHECK(arrived + died == 3000);
    CHECK(arrived > 0 && died > 0);
}

} // namespace

int main()
{
    testDeterministic();
    testBuyingValidation();
    testFortNegativeSpend();
    testWrongWord();
    testFuzz();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
