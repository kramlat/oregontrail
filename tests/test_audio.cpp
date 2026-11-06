// SPDX-License-Identifier: GPL-3.0-or-later
//
// Audio: the state logic (what's playing, mute and volume saved), and that every shipped
// track really decodes through Qt's own player at its expected length. The decode check
// has no output device attached, so it makes no sound.
#include "qt/audio.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QMediaPlayer>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } \
    } while (0)

static void testState()
{
    QTemporaryDir tmp;
    qputenv("OREGONTRAIL_DATA_DIR", tmp.path().toUtf8());
    qputenv("OREGONTRAIL_AUDIO", "off");                     // no sound, whatever machine this runs on
    {
        Audio a;
        CHECK(!a.enabled() && a.current() == Music::None && !a.muted() && a.volume() == 60);
        a.play(Music::Title);
        CHECK(a.current() == Music::Title);                  // it remembers the request even when silent
        a.play(Music::Fight);
        CHECK(a.current() == Music::Fight);
        a.play(Music::None);
        CHECK(a.current() == Music::None);
        a.setMuted(true);
        a.setVolume(35);
        a.setVolume(500);                                    // clamped
        CHECK(a.volume() == 100);
        a.setVolume(35);
    }
    {   // it came back as left
        Audio b;
        CHECK(b.muted() && b.volume() == 35);
    }
    CHECK(Audio::loops(Music::Title) && Audio::loops(Music::Travel) && Audio::loops(Music::Fight));
    CHECK(!Audio::loops(Music::Victory) && !Audio::loops(Music::Taps));
    for (Music m : {Music::Title, Music::Travel, Music::Fight, Music::Victory, Music::Taps}) CHECK(!Audio::fileFor(m).isEmpty());
    CHECK(Audio::fileFor(Music::None).isEmpty());
    qunsetenv("OREGONTRAIL_AUDIO");
}

static void testFilesDecode()
{
    qputenv("OREGONTRAIL_AUDIO_DIR", QDir(SOURCE_DIR "/assets/audio").absolutePath().toUtf8());
    CHECK(Audio::findAudioDir() == QDir(SOURCE_DIR "/assets/audio").absolutePath());
    struct Expect { Music m; double seconds; };
    const Expect tracks[] = {{Music::Title, 152.0}, {Music::Travel, 101.4}, {Music::Fight, 121.8},
                             {Music::Victory, 134.6}, {Music::Taps, 76.2}};
    for (auto &t : tracks) {
        QMediaPlayer player;
        bool failed = false;
        QObject::connect(&player, &QMediaPlayer::errorOccurred, [&](QMediaPlayer::Error, const QString &msg) {
            std::printf("  player error: %s\n", qPrintable(msg));
            failed = true;
        });
        player.setSource(QUrl::fromLocalFile(QDir(Audio::findAudioDir()).filePath(Audio::fileFor(t.m))));
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, [&](QMediaPlayer::MediaStatus s) {
            if (s == QMediaPlayer::LoadedMedia || s == QMediaPlayer::InvalidMedia) loop.quit();
        });
        timeout.start(8000);
        if (player.mediaStatus() != QMediaPlayer::LoadedMedia) loop.exec();
        const double got = player.duration() / 1000.0;
        std::printf("  %-10s %6.1f s (expected %.1f)\n", qPrintable(Audio::fileFor(t.m)), got, t.seconds);
        CHECK(!failed && player.mediaStatus() == QMediaPlayer::LoadedMedia);
        CHECK(got > t.seconds - 2 && got < t.seconds + 2);
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    testState();
    testFilesDecode();
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
