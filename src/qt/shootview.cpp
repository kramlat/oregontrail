// SPDX-License-Identifier: GPL-3.0-or-later
#include "shootview.h"
#include "fightview.h"

#include <QKeyEvent>
#include <QTimer>

#include <algorithm>
#include <cmath>

ShootView::ShootView(QWidget *parent) : PixelView(parent)
{
    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, &ShootView::tick);
    connect(this, &PixelView::moved, this, [this](QPoint p) { aim_ = p; });
    connect(this, &PixelView::clicked, this, [this](QPoint) {
        if (field_) field_->fire();
    });
}

void ShootView::begin(std::unique_ptr<fight::Field> field)
{
    field_ = std::move(field);
    turnLeft_ = turnRight_ = walkFwd_ = walkBack_ = strafeLeft_ = strafeRight_ = false;
    clock_.start();
    timer_->start();
}

std::unique_ptr<fight::Field> ShootView::take()
{
    timer_->stop();
    return std::move(field_);
}

bool ShootView::handleKey(QKeyEvent *e, bool press)
{
    if (!field_) return false;
    switch (e->key()) {
    case Qt::Key_Q: case Qt::Key_Left: turnLeft_ = press; return true;
    case Qt::Key_E: case Qt::Key_Right: turnRight_ = press; return true;
    case Qt::Key_W: case Qt::Key_Up: walkFwd_ = press; return true;
    case Qt::Key_S: case Qt::Key_Down: walkBack_ = press; return true;
    case Qt::Key_A: strafeLeft_ = press; return true;
    case Qt::Key_D: strafeRight_ = press; return true;
    case Qt::Key_Space: case Qt::Key_Control:
        if (press) field_->fire();
        return true;
    case Qt::Key_Return: case Qt::Key_Enter:
        if (press) {
            if (field_->hunting()) field_->stopHunting();
            else field_->fire();
        }
        return true;
    default: return false;
    }
}

void ShootView::tick()
{
    if (!field_) return;
    double dt = std::min(0.1, clock_.restart() / 1000.0);
    // Turning: keys, plus the mouse's distance from the centre of the view (works with a
    // trackpad or stick-as-mouse, where the pointer can't be captured).
    double mouse = (aim_.x() - art::W / 2) / double(art::W / 2);
    if (std::abs(mouse) < 0.12) mouse = 0;
    field_->turn(((turnRight_ ? 1 : 0) - (turnLeft_ ? 1 : 0)) * 2.3 * dt + mouse * 2.6 * dt);
    field_->walk((walkFwd_ ? 1 : 0) - (walkBack_ ? 1 : 0), (strafeRight_ ? 1 : 0) - (strafeLeft_ ? 1 : 0));
    field_->update(dt);
    renderFight(image, *field_);
    update();
    if (field_->done()) {
        timer_->stop();
        emit finished();
    }
}
