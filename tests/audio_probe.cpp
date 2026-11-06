// SPDX-License-Identifier: GPL-3.0-or-later
//
// Manual check that the music really plays through this machine's audio system. Run it by
// hand (it needs a sound device, so it is not part of ctest). It plays MUTED, so it makes
// no sound, but the player really runs: we watch it start, advance, crossfade and stop.
#include "qt/audio.h"

#include <QApplication>
#include <QEventLoop>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

static void pump(int ms)
{
    QEventLoop l;
    QTimer::singleShot(ms, &l, &QEventLoop::quit);
    l.exec();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir data;
    qputenv("OREGONTRAIL_DATA_DIR", data.path().toUtf8());
    {   // start muted, via the saved settings, so nothing is heard
        Audio setup;
        setup.setMuted(true);
    }
    Audio a;
    std::printf("audio dir: %s\nenabled: %d  muted: %d\n", qPrintable(Audio::findAudioDir()), a.enabled(), a.muted());
    if (!a.enabled()) {
        std::printf("not enabled here (no device, no files, or OREGONTRAIL_AUDIO=off)\n");
        return 2;
    }
    int bad = 0;
    auto expect = [&](bool ok, const char *what) { std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what); bad += !ok; };

    a.play(Music::Title);
    pump(2500);
    const qint64 p1 = a.position();
    std::printf("title: playing=%d position=%lld ms\n", a.playing(), (long long)p1);
    expect(a.playing() && p1 > 1200, "title music starts and advances");

    a.play(Music::Travel);                       // crossfades: old fades out, new starts
    pump(2500);
    const qint64 p2 = a.position();
    std::printf("travel: playing=%d position=%lld ms\n", a.playing(), (long long)p2);
    expect(a.current() == Music::Travel && a.playing() && p2 > 800 && p2 < 2800, "travel music replaces it, from the start");

    a.play(Music::Taps);                         // a one-shot
    pump(1500);
    expect(a.current() == Music::Taps && a.playing(), "a one-shot plays");

    a.play(Music::None);
    pump(800);
    expect(!a.playing(), "None stops the music");
    std::printf(bad ? "%d FAILED\n" : "audio pipeline: all ok\n", bad);
    return bad ? 1 : 0;
}
