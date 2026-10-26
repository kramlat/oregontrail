// SPDX-License-Identifier: GPL-3.0-or-later
//
// The shooting field, for both kinds of shooting in the game: the bandit and wolf
// attacks (targets run at the wagon, bandits shoot back) and hunting (targets are
// game animals that wander and bolt when you get close). It is a plain 2D
// simulation: a wagon on a trail, trees and bushes, a player who can walk and
// turn, bullets that fly and take time to arrive, and the targets. The first-person
// "fake 3D" view (src/qt/fightview.cpp) is only a renderer over this state.
//
// Qt-free so it can be unit-tested. The constants are our own: no source for the
// 1985 game is public, so they are tuned by feel, not copied.
#pragma once

#include "oregon.h"   // Rng

#include <cmath>
#include <vector>

namespace fight {

using oregon::Rng;

enum class Kind { Bandit, Wolf, Rabbit, Deer, Bison };
inline bool isGame(Kind k) { return k >= Kind::Rabbit; }

struct Enemy {                 // "enemy" or quarry: whatever you are supposed to shoot
    Kind kind;
    double x, y;
    bool alive = true;
    double phase = 0;          // walk-cycle / zigzag clock
    double pause = 0;          // seconds left standing still (aiming, grazing)
    double fireIn = 1.2;       // bandits: seconds until the next shot at the player
    double sinceShot = 99;     // bandits: seconds since the gun last went off (muzzle flash)
    double heading = 0;        // game: wander direction
    double turnIn = 0;         // game: seconds until it picks a new heading
};

// Things standing in the field. All solid to walkers and bullets; bushes are just
// lower (the renderer lets you see over them).
struct Prop {
    double x, y;
    bool tree;                 // false: bush
    int variant;               // which picture
    double radius;
};

struct Bullet {
    double x, y, dx, dy;       // position and unit direction
    double life;               // seconds left in flight
    bool hostile = false;      // fired by a bandit
};

class Field {
public:
    static constexpr int N = 24;                      // the field is N x N cells
    static constexpr double kFov = 1.15;              // radians, about 66 degrees (renderer only)
    static constexpr double kBulletSpeed = 26;        // cells per second
    static constexpr double kEnemyBulletSpeed = 13;   // slower than ours: strafing dodges them
    static constexpr double kWalkSpeed = 3.0;
    static constexpr int kMaxHits = 3;                // this many hits and you're down
    static constexpr double kHuntSeconds = 45;
    static constexpr double kDuelSeconds = 18;
    static constexpr double wx = N / 2.0, wy = N / 2.0;   // the wagon
    static constexpr double kTrailHalfWidth = 1.15;       // the dirt trail runs east-west through the wagon
    static bool onTrail(double y) { return std::abs(y - wy) < kTrailHalfWidth; }

    // An attack: `count` enemies of one kind (Bandit or Wolf) run for the wagon.
    Field(Kind kind, int count, int bullets, Rng &rng);
    // A hunt: game animals scattered about the field.
    struct HuntTag {};
    Field(HuntTag, int bullets, Rng &rng);
    // A duel: one target, for the 1978 rules' single "shot" (a game animal wandering in
    // the open, or one attacker coming at the wagon). Short: ends on a kill, when the
    // bullets are gone, or on the clock.
    struct DuelTag {};
    Field(DuelTag, Kind target, int bullets, Rng &rng);

    // --- Controls ---
    void turn(double radians) { angle += radians; }
    // Walking intent for the next update(): each in -1..1 (forward, strafe right).
    void walk(double forward, double strafe) { walkF_ = forward; walkS_ = strafe; }
    bool fire();                                      // true if a round left the rifle
    void update(double dt);                           // everything moves
    void stopHunting() { ended_ = true; }

    // --- State ---
    double px = N / 2.0, py = N / 2.0 + 5.5, angle = -1.5708;   // start south of the wagon, facing it
    int bullets;
    int total, spawned = 0, killed = 0, reached = 0, shots = 0;
    int hits = 0, wagonHits = 0, enemyShots = 0;
    double meat = 0;                                  // pounds, hunting
    double killTime = -1;                             // clock at the first kill, or -1
    double sinceFire = 99, sinceHit = 99;             // seconds, for the flashes
    double clock = 0;
    bool downed = false;                              // took kMaxHits: the rest of the gang gets to the wagon
    std::vector<Enemy> enemies;
    std::vector<Bullet> flying;
    std::vector<Prop> props;                          // trees and bushes (tests may replace these)

    bool hunting() const { return hunting_; }
    bool done() const;
    int alive() const;
    // The outermost ring of cells is the forest edge: impassable, drawn as trees.
    bool inField(double x, double y) const { return x >= 1 && y >= 1 && x < N - 1 && y < N - 1; }
    static double radius(Kind k);
    static double pounds(Kind k);                     // nominal meat, for the HUD/tests

private:
    Kind kind_;
    bool hunting_ = false, ended_ = false, duel_ = false;
    double timeLimit_ = kHuntSeconds;
    Rng &rng_;
    double cooldown_ = 0, spawnTimer_ = 0.4, walkF_ = 0, walkS_ = 0;

    void layOut();
    void spawn();
    void movePlayer(double dt);
    void moveBullets(double dt);
    void moveAttacker(Enemy &e, double dt);
    void moveGame(Enemy &e, double dt);
    void banditFire(Enemy &e);
    bool lineOfSight(double x0, double y0, double x1, double y1) const;
    // Open ground? `margin` fattens the props, for things with a body.
    bool free(double x, double y, double margin = 0) const;
    bool standable(double x, double y) const;
};

} // namespace fight
