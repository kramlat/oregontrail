// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "music.h"

#include <QMainWindow>

class Audio;
class ClassicGame;
class TitleScreen;
class TrailGame;
class QAction;
class QStackedWidget;

// The program's window. It opens on the title screen and starts nothing until you ask.
class AppWindow : public QMainWindow {
    Q_OBJECT
public:
    AppWindow();
    ~AppWindow() override;

    Audio *audio() const { return audio_; }
    QWidget *currentScreen() const;
    bool onTitle() const;

public slots:
    void showTitle();        // back to the title screen (asks first if a game is in progress)
    void playTrail();        // 1985 rules
    void playClassic();      // 1978 text game

private:
    bool confirmLeaveGame();
    void goToTitle();        // unconditional
    void syncMusicUi();

    QStackedWidget *stack_;
    TitleScreen *title_;
    TrailGame *trail_;
    ClassicGame *classic_;
    Audio *audio_;
    QAction *musicAction_ = nullptr;
    QList<QAction *> volumeActions_;
};
