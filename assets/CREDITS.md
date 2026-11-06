# Credits

## Music

All music is public domain, and every track is a recording by a United States military
band of a composition that is itself out of copyright. Nothing here requires attribution;
we credit anyway.

| File | Used for | Piece | Performer |
|---|---|---|---|
| `audio/title.ogg` | title screen | "Simple Gifts" (Joseph Brackett, 1848), arr. Robert Thurston | Concert Band, United States Air Force Band (album *America*, 2017) |
| `audio/travel.ogg` | on the trail | "Turkey in the Straw" (traditional), from the "American Hoe Down Medley", arr. Charles Granofsky | United States Air Force Strings (album *International Mosaic*) |
| `audio/fight.ogg` | gunfights | Hungarian Dance No. 1 in G minor, WoO 1 (Johannes Brahms, 1869) | Strolling Strings, United States Air Force Band (album *On Wings of Song*, 1989) |
| `audio/victory.ogg` | reaching Oregon City | "Battle Hymn of the Republic" (William Steffe, c. 1856; words Julia Ward Howe, 1861), slow version | Concert Band, United States Air Forces in Europe Band (album *To the Colors*, 1999) |
| `audio/taps.ogg` | tombstone scene | "Taps" (traditional, 1862) | Concert Band, United States Air Force Heritage of America Band (album *Early Light*, 2001) |

### Why these are free to use

- **The recordings** are works of the United States Air Force Band, made by service
  members in their official duties. Works of the US government are not protected by
  copyright in the United States (17 U.S.C. § 105). Wikimedia Commons tags each file with
  its `PD-USGov-Military-Air Force` template and names the Air Force Band's own
  "Public Domain Music" page as the source.
- **The compositions** were all published long ago (1848 to 1869 for the written works;
  "Turkey in the Straw" and "Taps" are 19th-century traditional pieces), so they are in the
  public domain.
- **The arrangements** (Thurston, Granofsky) were made for the Air Force Band and are covered
  in the same way as the recordings. This is the least certain link in the chain, and we
  rely on the Band's and Commons' statements for it.

### What we could and could not verify

- We read each file's page on Wikimedia Commons (licence templates, stated source,
  performer, album) through the Commons API on 2026-11-06.
- We **could not read the Air Force Band's own "Public Domain Music" page**: its site
  refuses automated requests (HTTP 403), and the Internet Archive's copy is an empty
  script-driven shell. So we are relying on Commons' citation of that page and on the
  statute, not on a quotation of the Band's terms.
- The recordings are not for sale or endorsement by the Air Force; using them implies no
  endorsement of this game.
- We left out the US Marine Band (its stated terms restrict distribution and the picture
  is murkier) and the US Navy Band "Victory at Sea" files (they cite only a streaming
  service as their source).

### Where each came from

| File | Wikimedia Commons page | Original source named there |
|---|---|---|
| `title` | [Simple Gifts (2017) - Concert Band - United States Air Force Band.mp3](https://commons.wikimedia.org/wiki/File:Simple_Gifts_(2017)_-_Concert_Band_-_United_States_Air_Force_Band.mp3) | https://www.music.af.mil/Multimedia/Music/Public-Domain-Music/ |
| `travel` | [Turkey in the Straw.ogg](https://commons.wikimedia.org/wiki/File:Turkey_in_the_Straw.ogg) | https://media.defense.gov/2017/Sep/15/2001810688/-1/-1/1/03.%20American%20Hoe%20Down%20Medley.mp3 |
| `fight` | [Hungarian Dance No. 1 - Strolling Strings - United States Air Force Band.mp3](https://commons.wikimedia.org/wiki/File:Hungarian_Dance_No._1_-_Strolling_Strings_-_United_States_Air_Force_Band.mp3) | https://www.music.af.mil/Multimedia/Music/Public-Domain-Music/ |
| `victory` | [Battle Hymn of the Republic (slow) - Concert Band - United States Air Forces in Europe Band.mp3](https://commons.wikimedia.org/wiki/File:Battle_Hymn_of_the_Republic_(slow)_-_Concert_Band_-_United_States_Air_Forces_in_Europe_Band.mp3) | https://www.music.af.mil/Multimedia/Music/Public-Domain-Music/ |
| `taps` | [Taps - Concert Band - United States Air Force Heritage of America Band.mp3](https://commons.wikimedia.org/wiki/File:Taps_-_Concert_Band_-_United_States_Air_Force_Heritage_of_America_Band.mp3) | https://www.music.af.mil/Multimedia/Music/Public-Domain-Music/ |

### What we changed

`tools/build_audio.sh` rebuilds every file from the originals (it checks each download
against a pinned SHA-256 first). For each track it trims silence from the ends, adds a short
fade in and a longer fade out, normalises to -18 LUFS (true peak below -1.5 dB) so the
tracks sit at one volume with headroom for sound effects, strips the embedded cover art and
tags, and encodes Ogg Vorbis (quality 2, about 96 kbit/s). The musical content is not
altered.

## Art

All artwork is drawn by the game's own code (`src/qt/pixelart.cpp`, `fightview.cpp`) in the
Apple II colour palette. No images from any version of the original game are used.

## Sound effects

None yet; see `TODO.md`.
