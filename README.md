# Oregon Trail

A C++20 / Qt6 port of The Oregon Trail, in two rulesets.

| Mode | What it is |
|---|---|
| **1985 rules** (default) | Graphical game: professions, a party of five, pace and rations, landmarks, rivers, per-person illness, hunting, tombstones, scoring. |
| **Classic 1978** (Game menu) | A line-for-line transcode of the May/June 1978 *Creative Computing* listing (Heinemann 1971, Rawitsch, Dillenberger, MECC), text only. |

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
block (line ranges in comments, RNG call order preserved). Known original bugs are
kept unless an option in `oregon::Options` says otherwise.
`docs/original/oregon-1978-creative-computing.bas` is the reference listing.

## Build

    cmake -S . -B build -G Ninja && cmake --build build
    ctest --test-dir build
    ./build/oregontrail

Other targets: `sim [games] [reckless]` (win rates per profession and month),
`render_art <dir>` (dump every scene to PNG), `smoke_gui [seconds] [png]`
(drives the real GUI offscreen with `QT_QPA_PLATFORM=offscreen`).

## Layout

- `src/oregon.*` - 1978 engine (Qt-free, blocking Frontend interface)
- `src/trail/` - 1985-rules engine (Qt-free, non-blocking, tuned in `tuning.h`)
- `src/qt/` - the interface: `trailgame` (1985 game + hunting), `pixelart`
  (all artwork), `classicgame` + `gamebridge` (1978 text mode on a worker thread)

License: GPL-3.0-or-later.
