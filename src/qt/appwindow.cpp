// SPDX-License-Identifier: GPL-3.0-or-later
#include "appwindow.h"
#include "classicgame.h"
#include "trailgame.h"

#include <QMenuBar>
#include <QStackedWidget>

AppWindow::AppWindow()
{
    setWindowTitle("The Oregon Trail");
    stack_ = new QStackedWidget;
    trail_ = new TrailGame;
    classic_ = new ClassicGame;
    stack_->addWidget(trail_);
    stack_->addWidget(classic_);
    setCentralWidget(stack_);
    resize(980, 820);

    auto *game = menuBar()->addMenu("&Game");
    game->addAction("&New Game (1985 rules)", QKeySequence::New, this, &AppWindow::playTrail);
    game->addAction("&Classic 1978 (text)", this, &AppWindow::playClassic);
    game->addSeparator();
    game->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);
}

void AppWindow::playTrail()
{
    classic_->stopGame();
    stack_->setCurrentWidget(trail_);
    trail_->newGame();
}

void AppWindow::playClassic()
{
    stack_->setCurrentWidget(classic_);
    classic_->newGame();
}
