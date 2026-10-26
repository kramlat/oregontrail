// SPDX-License-Identifier: GPL-3.0-or-later
#include "fight.h"

#include <algorithm>

namespace fight {

namespace {
constexpr double kReach = 1.3;          // an attacker this close to the wagon has got to it
constexpr double kWagonRadius = 1.0;    // the wagon blocks the player
constexpr double kBodyRadius = 0.25;    // the player's collision radius
constexpr double kRifleCooldown = 0.55; // seconds between shots
constexpr double kSpawnRadius = 9.0;
constexpr double kTimeLimit = 75;       // seconds; stragglers then count as having reached the wagon
constexpr double kBulletLife = 1.1;
constexpr double kTwoPi = 6.2831853;
}

double Field::radius(Kind k)
{
    switch (k) {
    case Kind::Wolf: return 0.45;
    case Kind::Bandit: return 0.4;
    case Kind::Rabbit: return 0.3;
    case Kind::Deer: return 0.5;
    default: return 0.75;
    }
}

double Field::pounds(Kind k)
{
    switch (k) {
    case Kind::Rabbit: return 2;
    case Kind::Deer: return 80;
    case Kind::Bison: return 350;
    default: return 0;
    }
}

void Field::layOut()
{
    // A fixed layout so the field is learnable: clumps of trees, bushes to hide behind,
    // and nothing on the trail itself.
    const struct { double x, y; bool tree; int v; } layout[] = {
        {5.5, 5.5, true, 0},  {6.6, 6.5, true, 1},  {18.2, 5.8, true, 1}, {17.1, 7.0, true, 0},
        {4.8, 17.4, true, 1}, {6.0, 18.3, true, 0}, {19.0, 18.0, true, 0}, {17.8, 19.2, true, 1},
        {12.5, 3.8, true, 1}, {11.2, 20.5, true, 0}, {3.6, 9.8, true, 0},  {20.4, 14.2, true, 1},
        {9.0, 8.4, false, 0}, {15.2, 9.0, false, 1}, {8.4, 15.6, false, 1}, {15.6, 15.4, false, 0},
        {10.4, 6.0, false, 0}, {13.8, 18.2, false, 1}, {6.2, 14.4, false, 0}, {19.4, 9.8, false, 1},
        {2.9, 15.6, false, 1}, {21.2, 17.0, false, 0},
    };
    for (auto &l : layout) props.push_back({l.x, l.y, l.tree, l.v, l.tree ? 0.4 : 0.55});
}

Field::Field(Kind kind, int count, int bulletsAvail, Rng &rng)
    : bullets(bulletsAvail), total(count), kind_(kind), rng_(rng)
{
    layOut();
}

Field::Field(HuntTag, int bulletsAvail, Rng &rng)
    : bullets(bulletsAvail), total(0), kind_(Kind::Deer), hunting_(true), rng_(rng)
{
    layOut();
    // Scatter the game, out of immediate sight of where you start.
    int count = 8 + int(rng_.next() * 4);
    for (int i = 0; i < count; ++i) {
        for (int tries = 0; tries < 60; ++tries) {
            double x = 2 + rng_.next() * (N - 4), y = 2 + rng_.next() * (N - 4);
            if (std::hypot(x - px, y - py) < 7 || onTrail(y)) continue;
            double k = rng_.next();
            Kind kind = k < 0.45 ? Kind::Rabbit : k < 0.88 ? Kind::Deer : Kind::Bison;
            if (!free(x, y, radius(kind))) continue;
            Enemy e{kind, x, y};
            e.heading = rng_.next() * kTwoPi;
            e.turnIn = rng_.next() * 2;
            e.phase = rng_.next() * 6;
            enemies.push_back(e);
            break;
        }
    }
    total = spawned = int(enemies.size());
}

Field::Field(DuelTag, Kind target, int bulletsAvail, Rng &rng)
    : bullets(bulletsAvail), total(1), kind_(target), rng_(rng)
{
    layOut();
    timeLimit_ = kDuelSeconds;
    duel_ = true;
    if (isGame(target)) {
        hunting_ = true;                                 // a hunt of one animal, ending on the kill
        for (int tries = 0; tries < 80; ++tries) {
            double x = 3 + rng_.next() * (N - 6), y = 3 + rng_.next() * (N - 6);
            double d = std::hypot(x - px, y - py);
            if (d < 9 || d > 14 || onTrail(y) || !free(x, y, radius(target))) continue;
            Enemy e{target, x, y};
            e.heading = rng_.next() * kTwoPi;
            e.turnIn = 1 + rng_.next() * 2;
            enemies.push_back(e);
            break;
        }
        spawned = int(enemies.size());
        total = std::max(1, spawned);
    } else {
        spawnTimer_ = 0.3;                               // the attacker arrives almost at once
    }
}

bool Field::free(double x, double y, double margin) const
{
    if (!inField(x, y)) return false;
    for (auto &p : props)
        if (std::hypot(x - p.x, y - p.y) < p.radius + margin) return false;
    return true;
}

int Field::alive() const
{
    int n = 0;
    for (auto &e : enemies) n += e.alive;
    return n;
}

bool Field::done() const
{
    if (!flying.empty()) return false;
    if (duel_)                                            // one shot at one target
        return killed > 0 || downed || bullets == 0 || clock >= timeLimit_ || ended_ ||
               (!hunting_ && spawned >= total && alive() == 0);
    if (hunting_)
        return ended_ || clock >= timeLimit_ || bullets == 0 || alive() == 0;
    return downed || (spawned >= total && alive() == 0);
}

void Field::spawn()
{
    for (int tries = 0; tries < 40; ++tries) {
        double a = rng_.next() * kTwoPi;
        double x = wx + std::cos(a) * kSpawnRadius, y = wy + std::sin(a) * kSpawnRadius;
        if (!free(x, y, 0.3)) continue;
        Enemy e{kind_, x, y};
        e.phase = rng_.next() * 6.28;
        enemies.push_back(e);
        ++spawned;
        return;
    }
    ++spawned;   // no room found (can't happen on this map, but never stall the fight)
    ++reached;
}

bool Field::standable(double x, double y) const
{
    return free(x, y, kBodyRadius) && std::hypot(x - wx, y - wy) >= kWagonRadius + kBodyRadius;
}

void Field::movePlayer(double dt)
{
    if (walkF_ == 0 && walkS_ == 0) return;
    double len = std::hypot(walkF_, walkS_);
    double f = walkF_ / std::max(1.0, len), s = walkS_ / std::max(1.0, len);
    double fx = std::cos(angle), fy = std::sin(angle);
    double dx = (fx * f - fy * s) * kWalkSpeed * dt;     // strafe-right is forward rotated by +90 degrees
    double dy = (fy * f + fx * s) * kWalkSpeed * dt;
    if (standable(px + dx, py)) px += dx;                // slide along obstacles
    if (standable(px, py + dy)) py += dy;
}

bool Field::lineOfSight(double x0, double y0, double x1, double y1) const
{
    double dx = x1 - x0, dy = y1 - y0, len = std::hypot(dx, dy);
    int steps = std::max(1, int(len / 0.2));
    for (int i = 1; i < steps; ++i)
        if (!free(x0 + dx * i / steps, y0 + dy * i / steps)) return false;
    return true;
}

// A bandit fires at where the player is now (no leading), with some inaccuracy.
void Field::banditFire(Enemy &e)
{
    double a = std::atan2(py - e.y, px - e.x) + (rng_.next() - 0.5) * 0.30;
    flying.push_back({e.x + std::cos(a) * 0.5, e.y + std::sin(a) * 0.5, std::cos(a), std::sin(a), 1.5, true});
    ++enemyShots;
    e.sinceShot = 0;
}

void Field::moveBullets(double dt)
{
    for (auto &b : flying) {
        double left = (b.hostile ? kEnemyBulletSpeed : kBulletSpeed) * dt;
        while (left > 0 && b.life > 0) {
            double step = std::min(left, 0.15);          // small steps: no tunnelling through targets or trees
            left -= step;
            b.x += b.dx * step;
            b.y += b.dy * step;
            if (!free(b.x, b.y)) { b.life = 0; break; }  // stopped by a tree, a bush or the forest edge
            if (b.hostile) {
                if (!downed && std::hypot(px - b.x, py - b.y) < 0.35) {
                    ++hits;
                    sinceHit = 0;
                    b.life = 0;
                    break;
                }
                if (std::hypot(wx - b.x, wy - b.y) < kWagonRadius) { ++wagonHits; b.life = 0; break; }
                continue;
            }
            for (auto &e : enemies) {
                if (e.alive && std::hypot(e.x - b.x, e.y - b.y) < radius(e.kind)) {
                    e.alive = false;
                    ++killed;
                    if (killTime < 0) killTime = clock;
                    if (isGame(e.kind))
                        meat += e.kind == Kind::Deer ? 60 + int(rng_.next() * 40) : pounds(e.kind);
                    b.life = 0;
                    break;
                }
            }
        }
        b.life -= dt;
    }
    flying.erase(std::remove_if(flying.begin(), flying.end(), [](const Bullet &b) { return b.life <= 0; }), flying.end());
}

// Bandits and wolves: run at the wagon. Bandits stop to shoot back.
void Field::moveAttacker(Enemy &e, double dt)
{
    e.phase += dt * (e.kind == Kind::Wolf ? 9 : 5);
    e.sinceShot += dt;
    double vx = wx - e.x, vy = wy - e.y;
    double dist = std::hypot(vx, vy);
    if (dist < kReach || clock > kTimeLimit) {
        e.alive = false;
        ++reached;
        return;
    }
    if (e.kind == Kind::Bandit) {
        // Shoot back whenever there's a clear line to the player; stop for a moment to do it.
        double pd = std::hypot(px - e.x, py - e.y);
        e.fireIn -= dt;
        if (e.fireIn <= 0 && pd < 11 && lineOfSight(e.x, e.y, px, py)) {
            banditFire(e);
            e.fireIn = 1.5 + rng_.next() * 1.1;
            if (e.pause <= 0) e.pause = 0.45;
        } else if (e.fireIn <= 0) {
            e.fireIn = 0.3;                              // no line yet: look again soon
        }
    }
    if (e.pause > 0) { e.pause -= dt; return; }
    // Wolves zigzag, which makes them hard to lead.
    double speed = e.kind == Kind::Wolf ? 3.4 : 1.7;
    vx /= dist; vy /= dist;
    double side = e.kind == Kind::Wolf ? std::sin(e.phase * 0.5) * 0.9 : 0;
    double mx = (vx - vy * side) * speed * dt, my = (vy + vx * side) * speed * dt;
    if (free(e.x + mx, e.y, 0.15)) e.x += mx;            // slide round trees and bushes
    if (free(e.x, e.y + my, 0.15)) e.y += my;
}

// Game animals: graze and wander; bolt away when the hunter gets close.
void Field::moveGame(Enemy &e, double dt)
{
    e.phase += dt * (e.kind == Kind::Rabbit ? 12 : 8);
    double pd = std::hypot(e.x - px, e.y - py);
    double alarm = e.kind == Kind::Rabbit ? 8 : e.kind == Kind::Deer ? 10 : 6;
    bool fleeing = pd < alarm;
    double speed;
    if (fleeing) {
        // Away from the hunter, with a little noise; rabbits zigzag.
        double away = std::atan2(e.y - py, e.x - px);
        e.heading = away + std::sin(e.phase * 0.4) * (e.kind == Kind::Rabbit ? 0.8 : 0.25);
        speed = e.kind == Kind::Rabbit ? 4.2 : e.kind == Kind::Deer ? 4.6 : 2.3;
        e.pause = 0;
    } else {
        e.turnIn -= dt;
        if (e.turnIn <= 0) {                             // pick a new direction, or graze a while
            e.heading = rng_.next() * kTwoPi;
            e.turnIn = 1.5 + rng_.next() * 2.5;
            if (rng_.next() < 0.45) e.pause = 0.8 + rng_.next() * 1.5;
        }
        if (e.pause > 0) { e.pause -= dt; return; }
        speed = e.kind == Kind::Bison ? 0.5 : 0.9;
    }
    double mx = std::cos(e.heading) * speed * dt, my = std::sin(e.heading) * speed * dt;
    bool movedX = free(e.x + mx, e.y, radius(e.kind) * 0.6);
    bool movedY = free(e.x, e.y + my, radius(e.kind) * 0.6);
    if (movedX) e.x += mx;
    if (movedY) e.y += my;
    if (!movedX && !movedY) e.turnIn = 0;                // boxed in: choose another way
}

void Field::update(double dt)
{
    clock += dt;
    sinceFire += dt;
    sinceHit += dt;
    cooldown_ = std::max(0.0, cooldown_ - dt);

    movePlayer(dt);
    moveBullets(dt);
    if (hits >= kMaxHits && !downed) {                   // you're down; the rest of them reach the wagon
        downed = true;
        for (auto &e : enemies)
            if (e.alive) { e.alive = false; ++reached; }
        reached += total - spawned;
        spawned = total;
    }
    if (downed) return;

    if (!hunting_) {
        spawnTimer_ -= dt;
        if (spawned < total && spawnTimer_ <= 0) {
            spawn();
            spawnTimer_ = 1.1 + rng_.next() * 0.8;
        }
    }
    for (auto &e : enemies) {
        if (!e.alive) continue;
        if (isGame(e.kind)) moveGame(e, dt);
        else moveAttacker(e, dt);
    }
}

bool Field::fire()
{
    if (bullets <= 0 || cooldown_ > 0) return false;
    --bullets;
    ++shots;
    cooldown_ = kRifleCooldown;
    sinceFire = 0;
    flying.push_back({px + std::cos(angle) * 0.4, py + std::sin(angle) * 0.4, std::cos(angle), std::sin(angle), kBulletLife});
    return true;
}

} // namespace fight
