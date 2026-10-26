// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fight/fight.h"
#include "pixelview.h"

#include <QElapsedTimer>
#include <memory>

class QKeyEvent;
class QTimer;

// A PixelView that can also host a shooting round: it owns the fight::Field, runs it
// on a fast timer, turns mouse and key input into walking, turning and firing, and
// draws the first-person view. Used by hunts, gunfights and the classic mode's shots.
class ShootView : public PixelView {
    Q_OBJECT
public:
    explicit ShootView(QWidget *parent = nullptr);

    void begin(std::unique_ptr<fight::Field> field);
    bool shooting() const { return bool(field_); }
    const fight::Field *field() const { return field_.get(); }
    std::unique_ptr<fight::Field> take();              // end the round and hand the result over

    // Feed keys from the owner (which holds keyboard focus). True if the key was used.
    bool handleKey(QKeyEvent *e, bool press);

protected:
    void keyPressEvent(QKeyEvent *e) override { if (!handleKey(e, true)) PixelView::keyPressEvent(e); }
    void keyReleaseEvent(QKeyEvent *e) override { if (!handleKey(e, false)) PixelView::keyReleaseEvent(e); }

signals:
    void finished();                                   // the round is over; call take()

private:
    void tick();

    std::unique_ptr<fight::Field> field_;
    QTimer *timer_;
    QElapsedTimer clock_;
    QPoint aim_{art::W / 2, art::H / 2};
    bool turnLeft_ = false, turnRight_ = false, walkFwd_ = false, walkBack_ = false,
         strafeLeft_ = false, strafeRight_ = false;
};
