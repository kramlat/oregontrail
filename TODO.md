# TODO

## Sound and music (the game is silent right now)

Goal for next week: **not silent.** A minimum set of effects and two music tracks,
with a credits file, then polish later.

### Licensing rules (decide before downloading anything)

The code is GPL-3.0-or-later. Audio is a separate work with its own licence, so:

- **Prefer CC0 / public domain.** No attribution, no strings, safe to commit.
- **CC-BY is acceptable** if every file is listed in `assets/CREDITS.md` and shown in a
  Credits dialog. **CC-BY-SA** needs care; ask before using it.
- **Avoid** anything NC (non-commercial) or ND (no derivatives), and anything whose terms
  forbid redistributing the raw file (that rules out committing it to a public repo).
- For every file keep: source URL, author, licence, date fetched, and any edits.
- Decide up front whether `*.ogg` goes in git-lfs (same question as in the Isekai repo).

### Where to look

Status: **verified** = I read the licence on the site; **per a guide** = only a secondary
write-up said so; **unchecked** = my own recollection, confirm before use.

| Source | Terms | Status |
|---|---|---|
| [Kenney](https://kenney.nl/assets) | CC0, game-ready UI clicks, impacts, retro SFX | per a guide |
| [OpenGameArt](https://opengameart.org) | Per item: CC0 / CC-BY / CC-BY-SA / GPL. Filter by CC0. | per a guide |
| [Freesound](https://freesound.org) | Per sound: CC0, CC-BY or CC-BY-NC. Filter by CC0; needs an account. | per a guide |
| [Incompetech](https://incompetech.com) | CC-BY 4.0 music; credit required | per a guide |
| [FreePD](https://freepd.com) | Public-domain music | unchecked; read its terms |
| [Musopen](https://musopen.org) | Public-domain recordings of old music | unchecked |
| [Internet Archive](https://archive.org) | Varies per item | check each item |
| [Pixabay](https://pixabay.com/music/) | Pixabay Content License: free to use, but **not** an open licence and usually forbids redistributing the raw file | per a guide; keep out of the public repo |
| Sonniss GDC bundle | Royalty-free for use in a game, but not for redistribution | per a guide; keep out of the repo |

Not recommended: the BBC Sound Effects archive (non-commercial only, per a guide).
The Library of Congress National Jukebox is streaming-only per 2011 sources and its
recordings belong to Sony; a 2024 data release of the audio exists but I did not find
its terms. Skip unless someone reads them.

### Own music is the cleanest route

The old tunes are public domain as *compositions* (all published before 1929 in the US):
"Oh! Susanna" (1848, the year the game is set), "Buffalo Gals", "Camptown Races",
"Turkey in the Straw", "Skip to My Lou", "Red River Valley", "Shenandoah", and the hymn
"Shall We Gather at the River" for tombstones. **A recording is a separate right**, so a
random recording of one of them is not automatically free. Arranging and rendering them
ourselves (fiddle/banjo/harmonica, FL Studio) gives music nobody can claim and costs
about a day.

### Minimum viable set (do these first)

Music (two tracks, looping, Ogg Vorbis):
- [ ] Travel theme (folk fiddle/banjo, relaxed)
- [ ] Fight tension (low drum or pulse), used for bandits, wolves and the Classic shots

Sound effects (WAV, short):
- [ ] Rifle shot (player), and a different shot for the bandits' guns
- [ ] Bullet hits a target; bullet hits a tree or bush
- [ ] Wagon rolling / creaking loop, played while travelling
- [ ] River splash (crossings and the swamped-wagon event)
- [ ] Wolf growl or howl
- [ ] Tolling bell for a death
- [ ] UI click and a coin sound for buying
- [ ] Victory fanfare, and a sad sting for game over

### Later

- [ ] Fort ambience, a fort-arrival tune, a quiet hunting theme, a title tune
- [ ] Weather beds (wind, rain, thunder for the thunderstorm event)
- [ ] Ox bellow (dead ox), snake rattle, breaking wood (wagon part), fire crackle
- [ ] Deer and bison sounds (rabbit and deer bolting, bison snort)
- [ ] The hymn for the tombstone scene; a different one when visiting an old grave
- [ ] Teletype clicks for the Classic text mode (optional, cute)

### Engineering

Qt Multimedia is installed (`/usr/include/qt6/QtMultimedia`).

- [ ] Add `Qt6::Multimedia` to the CMake targets that play sound
- [ ] An `Audio` class: `QSoundEffect` for short effects (low latency), `QMediaPlayer` +
      `QAudioOutput` for music with loop and crossfade; master / music / effects volume
      and mute, saved with `QSettings`; **M** toggles mute; a Credits entry in the Game menu
- [ ] **Never fail without a sound card:** the offscreen smoke tests and any machine
      with no audio device must run silently, not crash (test this)
- [ ] The shooting field needs to report sound events (shot fired, target hit, bandit
      fired, bullet stopped by a prop, target reached the wagon, player hit). Add a small
      event queue to `fight::Field` (Qt-free, unit-tested) that `ShootView` drains and turns
      into signals; do not poll counters from the renderer
- [ ] Hook the trail events (`trail::Note`s, deaths, graves, river results, victory, defeat)
      to sounds in `TrailGame`; keep the mapping in one table so it is easy to change
- [ ] Classic mode: the same shooting-round sounds through `ClassicGame`
- [ ] `assets/audio/`, `assets/CREDITS.md`, and a test that every audio file is listed in
      the credits (so nothing ships unattributed)

### Done when

- [ ] A full game, 1985 mode and Classic, has music and effects, and none are placeholders
- [ ] Every file is in `assets/CREDITS.md` with a licence we are allowed to ship
- [ ] Volume and mute work and persist; the game runs fine with no audio device

## Before promoting it publicly

The repository is already public; this is the list for when it is ready to be shown off.

- [ ] **Screenshots in the README.** `build/render_art <dir>` dumps every scene to PNG
      (road, forts, river crossings, fights, tombstone). Pick a handful, add a short gameplay
      GIF, and put them near the top of the README
- [ ] Play it by hand, start to finish, in both modes. Everything so far was checked by tests,
      renders and scripted drivers, never by a person
- [ ] Tune what only a human can judge: the shooting feel, the Classic shot constants
      (`src/qt/classicgame.cpp`), the 1985 balance (`src/trail/tuning.h`)
- [ ] Sound and music, with every file's licence recorded (see above)
- [ ] CI that builds and runs the tests on every push (the GUI smoke tests need
      `QT_QPA_PLATFORM=offscreen`)
- [ ] Packaging: an Arch `PKGBUILD` first (this machine runs CachyOS), then a plain `make`
      install for other distributions
- [ ] A tagged release with a short changelog
- [ ] Re-read the README for claims that are no longer true or are not proven, especially
      anything about the 1985 rules (we do not have its source and say so)

## Other ideas (not scheduled)

- Trading with other travellers; the Columbia River raft versus the Barlow toll road; route
  forks
- Move the Classic mode's shooting rounds onto hunts that use real animals nearby
- Tune the Classic shot-to-B1 constants after playing it by hand (`src/qt/classicgame.cpp`)
- Check the 1985 balance by hand: the simulator only proves the shape, not the feel
