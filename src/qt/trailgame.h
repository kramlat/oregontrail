// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pixelart.h"
#include "trail/trail.h"

#include <QWidget>
#include <memory>
#include <random>
#include <vector>

class QLabel;
class QPushButton;
class QTimer;

// Shows a QImage scaled to fit with crisp pixels; reports mouse in canvas coords.
class PixelView : public QWidget {
    Q_OBJECT
public:
    explicit PixelView(QWidget *parent = nullptr);
    QImage image;
signals:
    void moved(QPoint canvasPos);
    void clicked(QPoint canvasPos);
protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
private:
    QRect target() const;
    QPoint toCanvas(QPoint widgetPos) const;
};

// The 1985-rules game: setup, store, the road, landmarks, rivers, hunting.
class TrailGame : public QWidget {
    Q_OBJECT
public:
    explicit TrailGame(QWidget *parent = nullptr);
    ~TrailGame() override;

public slots:
    void newGame();

protected:
    void keyPressEvent(QKeyEvent *) override;

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
    enum class Mode { Idle, Travel, Hunt };
    struct RngImpl : oregon::Rng {
        std::mt19937 gen{std::random_device{}()};
        double next() override { return std::uniform_real_distribution<double>(0, 1)(gen); }
    };
    struct Beast {
        art::Animal kind;
        double x, y, vx;
        bool alive = true;
    };

    bool setup();
    void stepDay();
    void absorb(const std::vector<trail::Note> &notes);
    void deaths(const std::vector<trail::Note> &notes);
    void refresh();
    void landmarkArrival();
    void riverArrival();
    void victory();
    void defeat();
    void endHunt();
    void spawnBeast();
    void tickHunt();
    void drawHunt();

    RngImpl rng_;
    std::unique_ptr<trail::Trail> t_;
    Mode mode_ = Mode::Idle;
    bool going_ = false;
    bool over_ = false;
    double scroll_ = 0;
    int frame_ = 0, dayTicks_ = 0;
    QStringList recent_;

    // Hunting.
    std::vector<Beast> beasts_;
    int huntBullets_ = 0, huntShots_ = 0, huntTicks_ = 0, huntEnd_ = 500;
    double meat_ = 0;
    QPoint aim_{140, 96};

    PixelView *view_;
    QLabel *info_, *log_;
    QPushButton *go_, *pace_, *rations_, *rest_, *hunt_, *shop_, *supplies_;
    QTimer *timer_;
};
