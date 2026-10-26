// SPDX-License-Identifier: GPL-3.0-or-later
//
// Oregon Trail engine: a structured C++ transcode of the May/June 1978
// Creative Computing listing (Heinemann 1971, Rawitsch/Dillenberger, MECC).
// No Qt, no I/O of its own: everything the player sees or types goes through
// Frontend, and all randomness through Rng, so the game is fully testable.
#pragma once

#include <memory>
#include <string>

namespace oregon {

// One call to RND(1) in the original: uniform in [0,1).
class Rng {
public:
    virtual ~Rng() = default;
    virtual double next() = 0;
};

class Frontend {
public:
    virtual ~Frontend() = default;

    // Output. text() is a PRINT without the trailing newline; line() adds it.
    virtual void text(const std::string &s) = 0;
    void line(const std::string &s = {}) { text(s + "\n"); }

    // INPUT: show the prompt, return the player's raw reply.
    virtual std::string ask(const std::string &prompt) = 0;

    // The shooting minigame. The original asked you to type a word ("BANG") and timed
    // you; here the front end plays a shooting round at whatever the situation calls for
    // and reports how long a clean kill took, in the same units the BASIC used
    // (seconds: 1 or less is a perfect shot). 9 or more means you missed.
    enum class Target { Game, Riders, Bandits, Wolves };
    virtual double shoot(Target target) = 0;
};

// Original bugs/typos we know of. Off = bug-for-bug original behaviour.
struct Options {
    // Fort purchases accept negative amounts (line 2340): no cash changes hands,
    // but the item stock shrinks by 2/3 of the amount. Pure self-harm.
    bool fixNegativeFortSpend = false;
};

enum class Outcome { Arrived, Died };

class Game {
public:
    Game(Frontend &fe, Rng &rng, Options opt = {});
    Outcome play();

private:
    // Each of these is a block of the original; line ranges are in oregon.cpp.
    // Death is a Died exception thrown by epilogue(), caught in play().
    void instructions();
    void chooseMarksmanship();
    void initialPurchases();
    void dateAndTurn();
    void beginTurn();
    void chooseAction();
    void fort();
    void hunt();
    void eat();
    void riders();
    void event();
    void mountains();
    void illness();
    double shoot(Frontend::Target t);   // SHOOTING SUB-ROUTINE; returns B1
    [[noreturn]] void diedOf();     // "YOU DIED OF ..." then epilogue
    [[noreturn]] void epilogue();   // the funeral formalities
    void finalTurn();

    std::string prompt(const std::string &p);
    double number(const std::string &p);

    Frontend &fe_;
    Rng &rng_;
    Options opt_;

    // Variable names follow the original's key (lines 6470-6790).
    double A = 0, B = 0, B1 = 0, C = 0, C1 = 0, F = 0, F1 = 0, F2 = 0, F9 = 0;
    double K8 = 0, L1 = 0, M = 0, M1 = 0, M2 = 0, M9 = 0, S4 = 0, T = 0, X1 = 0;
    int D3 = 0, D9 = 0, E = 0;
};

} // namespace oregon
