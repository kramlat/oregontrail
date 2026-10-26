// SPDX-License-Identifier: GPL-3.0-or-later
//
// Every balance number of the 1985-rules engine, in one place. Starting cash,
// store prices and the broad shape (three paces, three rations, five travellers)
// follow the documented 1985 game; the rest is tuned by tests/sim.cpp so that a
// sensible player's odds feel right per profession.
#pragma once

namespace trail::tune {

// Starting cash and score multiplier, by Profession (enum order). The three 1985 jobs keep
// their figures; the others are the Deluxe/Windows jobs, whose cash we found documented.
// Their multipliers are ours: we follow the 1985 rule that less money earns a higher
// multiplier, scaled by starting cash (we have no source for the originals).
//                                       Banker Doctor Merchant Blacksmith Carpenter Saddlemaker Farmer Teacher
inline constexpr double kStartCash[8] = {1600,  1200,  1200,     800,       800,      800,        400,   400};
inline constexpr double kScoreMultiplier[8] = {1, 1.5,  1.5,       2,         2,        2,          3,     3};

// Store prices at Independence (forts charge more, see Trail::price).
inline constexpr double kYoke = 40, kFoodLb = 0.20, kClothes = 10, kBullets = 2, kPart = 10;
inline constexpr double kFortMarkupPer1000Miles = 0.6;

// Travel.
inline constexpr double kBaseMilesPerDay = 20;
inline constexpr int kOxenForFullSpeed = 6;
inline constexpr double kPaceSpeed[3] = {1.0, 1.35, 1.7};
inline constexpr double kPaceDamage[3] = {0.0, 0.6, 1.6};

// Food, pounds per person per day.
inline constexpr double kRationLb[3] = {3, 2, 1};
inline constexpr double kRationDamage[3] = {-0.4, 0.2, 0.9};
inline constexpr double kStarvingDamage = 3.5;

// Health.
inline constexpr double kRestHeal = 3.0;
inline constexpr double kIllnessDamage = 2.0;
inline constexpr double kColdDamage = 4.0;           // per day at 0 F with no clothes
inline constexpr double kSnowboundDamage = 1.5;      // extra, winter in the mountains
inline constexpr double kWinterMountainSpeed = 0.35;
inline constexpr double kOnsetBase = 0.005, kOnsetPerDamage = 0.0003;
inline constexpr double kRecoverChance = 0.10, kRecoverRestBonus = 0.15;
// Daily chance an ill traveller dies, by Illness enum order (None first).
inline constexpr double kDeathChance[9] = {0, 0.05, 0.03, 0.018, 0.03, 0.012, 0.015, 0.0, 0.0};

// Events.
inline constexpr double kEventChance = 0.16;

// Fights (bandits and wolves). Weights are relative to the other events.
inline constexpr int kBanditWeight = 4, kWolfWeight = 4;
inline constexpr double kBanditCash = 0.12, kBanditFood = 0.08, kBanditBullets = 0.15;   // per surviving bandit
inline constexpr double kWolfBite = 14;
inline constexpr double kBulletWound = 22, kWagonHitFood = 8;   // return fire

// Score.
inline constexpr int kScoreHealth[4] = {500, 400, 300, 200};

} // namespace trail::tune
