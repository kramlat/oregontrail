// SPDX-License-Identifier: GPL-3.0-or-later
#include "appwindow.h"
#include "audio.h"
#include "classicgame.h"
#include "titlescreen.h"
#include "trailgame.h"

#include <QActionGroup>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>

AppWindow::AppWindow()
{
    setWindowTitle("The Oregon Trail");
    audio_ = new Audio(this);
    stack_ = new QStackedWidget;
    title_ = new TitleScreen;
    trail_ = new TrailGame;
    classic_ = new ClassicGame;
    stack_->addWidget(title_);
    stack_->addWidget(trail_);
    stack_->addWidget(classic_);
    setCentralWidget(stack_);
    resize(980, 820);

    auto *game = menuBar()->addMenu("&Game");
    game->addAction("&New Game (1985 rules)", QKeySequence::New, this, &AppWindow::playTrail);
    game->addAction("&Classic 1978 (text)", this, &AppWindow::playClassic);
    game->addAction("&Title Screen", this, &AppWindow::showTitle);
    game->addSeparator();
    musicAction_ = game->addAction("&Music");
    musicAction_->setCheckable(true);
    musicAction_->setShortcut(Qt::Key_M);
    connect(musicAction_, &QAction::triggered, this, [this](bool on) { audio_->setMuted(!on); });
    auto *vol = game->addMenu("Music &Volume");
    auto *group = new QActionGroup(this);
    for (int pct : {25, 50, 75, 100}) {
        QAction *a = vol->addAction(QString("%1%").arg(pct));
        a->setCheckable(true);
        a->setData(pct);
        group->addAction(a);
        volumeActions_ << a;
        connect(a, &QAction::triggered, this, [this, pct] { audio_->setVolume(pct); });
    }
    game->addSeparator();
    game->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    connect(title_, &TitleScreen::newGame, this, &AppWindow::playTrail);
    connect(title_, &TitleScreen::classic, this, &AppWindow::playClassic);
    connect(title_, &TitleScreen::toggleMusic, this, [this] { audio_->setMuted(!audio_->muted()); });
    connect(title_, &TitleScreen::quit, this, &QWidget::close);
    connect(trail_, &TrailGame::music, audio_, &Audio::play);
    connect(audio_, &Audio::changed, this, &AppWindow::syncMusicUi);

    syncMusicUi();
    goToTitle();                          // nothing starts until the player says so
}

AppWindow::~AppWindow() = default;

QWidget *AppWindow::currentScreen() const { return stack_->currentWidget(); }
bool AppWindow::onTitle() const { return stack_->currentWidget() == title_; }

void AppWindow::syncMusicUi()
{
    musicAction_->setChecked(!audio_->muted());
    for (QAction *a : volumeActions_) a->setChecked(a->data().toInt() == audio_->volume());
    title_->setMusicOn(!audio_->muted());
}

void AppWindow::goToTitle()
{
    classic_->stopGame();
    trail_->abandon();
    stack_->setCurrentWidget(title_);
    title_->setFocus();
    audio_->play(Music::Title);
}

bool AppWindow::confirmLeaveGame()
{
    if (!trail_->inProgress() && !classic_->inProgress()) return true;
    return QMessageBox::question(this, "The Oregon Trail",
                                 "You have a journey in progress. Abandon it?") == QMessageBox::Yes;
}

void AppWindow::showTitle()
{
    if (confirmLeaveGame()) goToTitle();
}

void AppWindow::playTrail()
{
    if (!confirmLeaveGame()) return;
    classic_->stopGame();
    stack_->setCurrentWidget(trail_);
    if (!trail_->newGame())               // cancelled in the setup dialogs
        goToTitle();
}

void AppWindow::playClassic()
{
    if (!confirmLeaveGame()) return;
    trail_->abandon();
    stack_->setCurrentWidget(classic_);
    audio_->play(Music::Travel);
    classic_->newGame();
}
