// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "music.h"

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <functional>

class QAudioOutput;
class QMediaPlayer;
class QVariantAnimation;

// Plays the game's music: one track at a time, faded in and out. It never gets in the
// way: with no sound card, no audio files, or OREGONTRAIL_AUDIO=off it does nothing (but
// still remembers what was asked for, so the rest of the game and the tests can rely on
// current()).
class Audio : public QObject {
    Q_OBJECT
public:
    explicit Audio(QObject *parent = nullptr);
    ~Audio() override;

    // The folder holding title.ogg and friends: $OREGONTRAIL_AUDIO_DIR, then next to the
    // program (installed or built in place), then the source tree; empty if none has them.
    static QString findAudioDir();
    static QString fileFor(Music m);          // "title.ogg", ...; empty for None
    static bool loops(Music m);               // Title, Travel and Fight loop; Victory and Taps play once

    bool enabled() const { return enabled_; } // false: nothing will actually sound
    Music current() const { return current_; }
    bool playing() const;                     // really sounding (or silently playing while muted)
    qint64 position() const;                  // milliseconds into the current track
    void play(Music m);                       // Music::None stops

    bool muted() const { return muted_; }
    void setMuted(bool on);
    int volume() const { return volume_; }    // percent, 0..100
    void setVolume(int percent);

signals:
    void changed();                           // current track, mute or volume changed

private:
    void startCurrent();
    void fadeTo(double target, int ms, std::function<void()> done);
    void applyVolume();
    void save() const;

    QString dir_;
    bool enabled_ = false, broken_ = false;
    Music current_ = Music::None;
    bool muted_ = false;
    int volume_ = 60;
    double fade_ = 1.0;                       // 0..1, multiplies the volume during a fade
    QMediaPlayer *player_ = nullptr;
    QAudioOutput *output_ = nullptr;
    QVariantAnimation *fadeAnim_ = nullptr;
    std::function<void()> onFadeDone_;
};
