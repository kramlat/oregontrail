// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio.h"
#include "appdata.h"

#include <QAudio>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QSettings>
#include <QUrl>
#include <QVariantAnimation>

#include <algorithm>

#ifndef OREGONTRAIL_SOURCE_ASSETS
#define OREGONTRAIL_SOURCE_ASSETS ""
#endif
#ifndef OREGONTRAIL_INSTALL_DATA
#define OREGONTRAIL_INSTALL_DATA ""
#endif

QString Audio::fileFor(Music m)
{
    switch (m) {
    case Music::Title: return "title.ogg";
    case Music::Travel: return "travel.ogg";
    case Music::Fight: return "fight.ogg";
    case Music::Victory: return "victory.ogg";
    case Music::Taps: return "taps.ogg";
    default: return {};
    }
}

bool Audio::loops(Music m) { return m == Music::Title || m == Music::Travel || m == Music::Fight; }

QString Audio::findAudioDir()
{
    QStringList candidates;
    const QString env = qEnvironmentVariable("OREGONTRAIL_AUDIO_DIR");
    if (!env.isEmpty()) candidates << env;
    const QString app = QCoreApplication::applicationDirPath();
    candidates << app + "/../share/oregontrail/audio" << app + "/assets/audio" << app + "/../assets/audio";
    if (QString(OREGONTRAIL_INSTALL_DATA).size()) candidates << QString(OREGONTRAIL_INSTALL_DATA) + "/audio";
    if (QString(OREGONTRAIL_SOURCE_ASSETS).size()) candidates << QString(OREGONTRAIL_SOURCE_ASSETS) + "/audio";
    for (const QString &c : candidates)
        if (QFile::exists(QDir(c).filePath("title.ogg")))
            return QDir(c).absolutePath();
    return {};
}

Audio::Audio(QObject *parent) : QObject(parent)
{
    QSettings s(QDir(appDataDir()).filePath("settings.ini"), QSettings::IniFormat);
    muted_ = s.value("music/muted", false).toBool();
    volume_ = std::clamp(s.value("music/volume", 60).toInt(), 0, 100);

    fadeAnim_ = new QVariantAnimation(this);
    connect(fadeAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        fade_ = v.toDouble();
        applyVolume();
    });
    connect(fadeAnim_, &QVariantAnimation::finished, this, [this] {
        auto done = std::move(onFadeDone_);
        onFadeDone_ = nullptr;
        if (done) done();
    });

    if (qEnvironmentVariable("OREGONTRAIL_AUDIO") == "off") return;
    dir_ = findAudioDir();
    if (dir_.isEmpty() || QMediaDevices::defaultAudioOutput().isNull()) return;

    output_ = new QAudioOutput(this);
    player_ = new QMediaPlayer(this);
    player_->setAudioOutput(output_);
    connect(player_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &msg) {
        qWarning("music: %s (giving up on sound)", qPrintable(msg));
        broken_ = true;                                  // never let sound problems bother the game
    });
    connect(player_, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus st) {
        if (st == QMediaPlayer::EndOfMedia && !loops(current_)) {   // a one-shot finished
            current_ = Music::None;
            emit changed();
        }
    });
    enabled_ = true;
    applyVolume();
}

Audio::~Audio() = default;

bool Audio::playing() const { return player_ && player_->playbackState() == QMediaPlayer::PlayingState; }

qint64 Audio::position() const { return player_ ? player_->position() : 0; }

void Audio::play(Music m)
{
    if (m == current_) return;
    current_ = m;
    emit changed();
    if (!enabled_ || broken_) return;
    if (player_->playbackState() == QMediaPlayer::PlayingState && fade_ > 0.05)
        fadeTo(0.0, 250, [this] { startCurrent(); });    // let the old track fade out first
    else
        startCurrent();
}

void Audio::startCurrent()
{
    player_->stop();
    if (current_ == Music::None || broken_) return;
    const QString file = QDir(dir_).filePath(fileFor(current_));
    if (!QFile::exists(file)) {
        qWarning("music: missing %s", qPrintable(file));
        return;
    }
    player_->setSource(QUrl::fromLocalFile(file));
    player_->setLoops(loops(current_) ? QMediaPlayer::Infinite : QMediaPlayer::Once);
    fade_ = 0;
    applyVolume();
    player_->play();
    fadeTo(1.0, 800, nullptr);
}

void Audio::fadeTo(double target, int ms, std::function<void()> done)
{
    fadeAnim_->stop();
    onFadeDone_ = std::move(done);
    fadeAnim_->setDuration(ms);
    fadeAnim_->setStartValue(fade_);
    fadeAnim_->setEndValue(target);
    fadeAnim_->start();
}

void Audio::applyVolume()
{
    if (!output_) return;
    // Volume is perceptual: the slider is linear in loudness, the output is linear in amplitude.
    const double user = QAudio::convertVolume(volume_ / 100.0, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
    output_->setVolume(muted_ ? 0.0f : float(user * fade_));
}

void Audio::setMuted(bool on)
{
    if (on == muted_) return;
    muted_ = on;
    applyVolume();
    save();
    emit changed();
}

void Audio::setVolume(int percent)
{
    percent = std::clamp(percent, 0, 100);
    if (percent == volume_) return;
    volume_ = percent;
    applyVolume();
    save();
    emit changed();
}

void Audio::save() const
{
    QSettings s(QDir(appDataDir()).filePath("settings.ini"), QSettings::IniFormat);
    s.setValue("music/muted", muted_);
    s.setValue("music/volume", volume_);
}
