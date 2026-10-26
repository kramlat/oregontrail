// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fight/fight.h"
#include "graveyard.h"
#include "trail/trail.h"

#include <QImage>
#include <QWidget>
#include <memory>
#include <random>
#include <vector>

class QKeyEvent;
class QLabel;
class ShootView;
class QPushButton;
class QTimer;

// The 1985-rules game: setup, store, the road, landmarks, rivers, hunting.
class TrailGame : public QWidget {
    Q_OBJECT
public:
    explicit TrailGame(QWidget *parent = nullptr);
    ~TrailGame() override;

public:
    void setDayTickMs(int ms);                // test seam: how fast the wagon animates

public slots:
    void newGame();

protected:
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;

private slots:
    void tick();
    void toggleGo();
    void choosePace();
    void chooseRations();
    void restClicked();
    void startHunt();
    void shop();
    void showSupplies();

private:
    enum class Mode { Idle, Travel, Shoot };
    struct RngImpl : oregon::Rng {
        std::mt19937 gen{std::random_device{}()};
        double next() override { return std::uniform_real_distribution<double>(0, 1)(gen); }
    };

    bool setup();
    void stepDay();
    void absorb(const std::vector<trail::Note> &notes);
    void deaths(const std::vector<trail::Note> &notes);
    void visitGraves();
    QImage grave(const QString &name, const QString &cause);   // asks for an epitaph, draws the stone
    void refresh();
    void landmarkArrival();
    void riverArrival();
    void victory();
    void defeat();
    void startFight();
    void beginShooting(std::unique_ptr<fight::Field> field);
    void endShooting();

    RngImpl rng_;
    std::unique_ptr<trail::Trail> t_;
    Mode mode_ = Mode::Idle;
    bool going_ = false;
    bool over_ = false;
    double scroll_ = 0;
    int frame_ = 0, dayTicks_ = 0;
    QStringList recent_;
    GraveYard yard_;          // tombstones saved from every game, on disk
    QImage leaderGrave_;      // the leader's tombstone, once they've died

    ShootView *view_;       // the picture, and the first-person shootout when one is on
    QLabel *info_, *log_;
    QPushButton *go_, *pace_, *rations_, *rest_, *hunt_, *shop_, *supplies_;
    QTimer *timer_;
};
