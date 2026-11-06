// SPDX-License-Identifier: GPL-3.0-or-later
#include "titlescreen.h"

#include <QKeyEvent>
#include <QTimer>

namespace {
enum Item { NewGame, Classic, MusicToggle, Quit };
}

TitleScreen::TitleScreen(QWidget *parent) : PixelView(parent)
{
    items_ = {"NEW GAME", "CLASSIC 1978 (TEXT)", "MUSIC: ON", "QUIT"};
    setFocusPolicy(Qt::StrongFocus);

    timer_ = new QTimer(this);
    timer_->setInterval(60);
    connect(timer_, &QTimer::timeout, this, [this] {
        ++frame_;
        scroll_ += 0.7;
        render();
    });
    connect(this, &PixelView::moved, this, [this](QPoint p) {
        int i = itemAt(p);
        if (i >= 0 && i != selected_) selected_ = i;
    });
    connect(this, &PixelView::clicked, this, [this](QPoint p) {
        int i = itemAt(p);
        if (i >= 0) activate(i);
    });
    render();
}

void TitleScreen::setMusicOn(bool on)
{
    items_[MusicToggle] = on ? "MUSIC: ON" : "MUSIC: OFF";
    render();
}

int TitleScreen::itemAt(QPoint canvasPos) const
{
    for (int i = 0; i < items_.size(); ++i)
        if (art::titleMenuRect(i, items_[i]).contains(canvasPos)) return i;
    return -1;
}

void TitleScreen::activate(int index)
{
    selected_ = index;
    switch (index) {
    case NewGame: emit newGame(); break;
    case Classic: emit classic(); break;
    case MusicToggle: emit toggleMusic(); break;
    case Quit: emit quit(); break;
    default: break;
    }
}

void TitleScreen::keyPressEvent(QKeyEvent *e)
{
    const int n = int(items_.size());
    switch (e->key()) {
    case Qt::Key_Up: case Qt::Key_W: selected_ = (selected_ + n - 1) % n; break;
    case Qt::Key_Down: case Qt::Key_S: selected_ = (selected_ + 1) % n; break;
    case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Space: activate(selected_); break;
    case Qt::Key_N: activate(NewGame); break;
    case Qt::Key_C: activate(Classic); break;
    case Qt::Key_M: activate(MusicToggle); break;
    default: PixelView::keyPressEvent(e); return;
    }
    render();
}

void TitleScreen::showEvent(QShowEvent *e)
{
    PixelView::showEvent(e);
    timer_->start();
}

void TitleScreen::hideEvent(QHideEvent *e)
{
    PixelView::hideEvent(e);
    timer_->stop();                       // no animation while a game is on
}

void TitleScreen::render()
{
    art::titleScene(image, frame_, scroll_, items_, selected_);
    update();
}
