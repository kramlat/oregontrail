# Oregon Trail

A C++20 / Qt6 port of The Oregon Trail, in two rulesets.

| Mode | What it is |
|---|---|
| **1985 rules** (default) | Graphical game: professions, a party of five, pace and rations, landmarks, rivers, per-person illness, tombstones, scoring, and first-person hunting and gunfights (see below). |
| **Classic 1978** (Game menu) | A line-for-line transcode of the May/June 1978 *Creative Computing* listing (Heinemann 1971, Rawitsch, Dillenberger, MECC), in a text window. Its "TYPE BANG" timing is replaced by a short first-person shooting round at the right target (a deer, a wolf, riders, bandits). |

## Honesty about "1985"

No source for the 1985 Apple II release is public, so the 1985 mode is **not** a
transcode. It implements the documented feature set (three professions at
$1600/$800/$400 with x1/x2/x3 score, a March-July departure, pace/rations,
ford/caulk/ferry, forts with rising prices, a hunting minigame) with the numbers
in one file, `src/trail/tuning.h`. Those numbers are balanced by simulation
(`build/sim`), not copied from the original. Landmark distances are the commonly
published figures and are approximate. All art is original, drawn procedurally in
the Apple II six-colour palette.

The 1978 mode *is* a transcode: `src/oregon.cpp` mirrors the BASIC block by
block (line ranges in comments, RNG call order preserved), apart from how shots are taken
(below). Known original bugs are kept unless an option in `oregon::Options` says otherwise.
`docs/original/oregon-1978-creative-computing.bas` is the reference listing.

## The shootout

Bandit and wolf attacks and hunting all play in one first-person view (Classic 1978
uses it too, for its single shots; see below): a raycast, fake-3D look at a plain 2D
simulation (`src/fight/`) of a clearing: the wagon on a dirt trail, trees and bushes,
a forest edge. Whatever you are supposed to shoot is just a sprite in that field.

- **Controls:** walk W A S D, turn with the mouse (or Q/E, arrows), fire with a click
  or Space. Enter ends a hunt.
- **Physics:** bullets take time to arrive, so lead what's running; trees and bushes
  stop bullets and block movement.
- **Attacks:** bandits and wolves run for the wagon. The bandits have guns and shoot
  back (their slower bullets can be dodged by strafing); three hits and you are down.
  What you stop, and what gets through, decides what the party loses
  (`Trail::resolveEncounter`). Wolves zigzag.
- **Hunting:** rabbits, deer and bison graze and bolt when you get close; meat comes
  from what you drop (the wagon carries 200 lb home).

In Classic 1978 each shot the BASIC would have timed is a short round against one
target; how fast you drop it becomes the original's reaction-time number (`B1`: 1 or
less is a perfect shot, 9 a miss; constants at the top of `src/qt/classicgame.cpp`).
This is the one deliberate change to that mode's input; every rule around it is the
1978 listing. `Frontend::shoot(Target)` is the seam.

The physics constants are our own, tuned by feel; they are not claimed to match the 1985 game.

## Build

    cmake -S . -B build -G Ninja && cmake --build build
    ctest --test-dir build
    ./build/oregontrail

Other targets: `sim [games] [reckless]` (win rates per profession and month),
`render_art <dir>` (dump every scene to PNG), `smoke_gui [seconds] [png]`
(drives the real GUI offscreen with `QT_QPA_PLATFORM=offscreen`), `test_fight`.

## Layout

- `src/oregon.*` - 1978 engine (Qt-free, blocking Frontend interface)
- `src/trail/` - 1985-rules engine (Qt-free, non-blocking, tuned in `tuning.h`)
- `src/qt/` - the interface: `trailgame` (1985 game + hunting), `pixelart`
  (all artwork), `classicgame` + `gamebridge` (1978 text mode on a worker thread)

License: GPL-3.0-or-later.
