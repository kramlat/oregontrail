// SPDX-License-Identifier: GPL-3.0-or-later
#include "classicgame.h"
#include "gamebridge.h"
#include "shootview.h"

#include <QFontDatabase>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStackedWidget>
#include <QThread>
#include <QVBoxLayout>

#include <random>

namespace {
// How a shooting round becomes the BASIC's "B1" (seconds; <= 1 is the perfect shot, 9 a miss).
// A kill within kFreeSeconds is a perfect shot; each extra second costs kSlope; each hit
// you took from the bandits costs kPerHit. Tune here.
constexpr double kPerfect = 0.3, kFreeSeconds = 2.0, kSlope = 0.45, kPerHit = 0.6;
constexpr int kRoundBullets = 5;

struct MtRng : oregon::Rng {
    std::mt19937 gen{std::random_device{}()};
    double next() override { return std::uniform_real_distribution<double>(0, 1)(gen); }
};
} // namespace

ClassicGame::ClassicGame(QWidget *parent) : QWidget(parent)
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(14);

    log_ = new QPlainTextEdit;
    log_->setReadOnly(true);
    log_->setFont(mono);
    log_->setMaximumBlockCount(5000);
    input_ = new QLineEdit;
    input_->setFont(mono);
    input_->setEnabled(false);

    // Phosphor-on-black, like the Teletype it came from.
    const char *style = "background:#0b0b0b; color:#33ff66; border:none; padding:4px;";
    log_->setStyleSheet(style);
    input_->setStyleSheet(style);

    auto *text = new QWidget;
    auto *box = new QVBoxLayout(text);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(log_, 1);
    box->addWidget(input_);

    view_ = new ShootView;
    view_->setFocusPolicy(Qt::StrongFocus);
    rng_ = std::make_unique<MtRng>();

    pages_ = new QStackedWidget;
    pages_->addWidget(text);
    pages_->addWidget(view_);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(pages_);

    connect(input_, &QLineEdit::returnPressed, this, &ClassicGame::reply);
    connect(view_, &ShootView::finished, this, &ClassicGame::shotFinished);
}

ClassicGame::~ClassicGame() { stopGame(); }

void ClassicGame::stopGame()
{
    if (view_->shooting())
        view_->take();
    pages_->setCurrentIndex(0);
    if (!worker_)
        return;
    bridge_->cancel();
    worker_->wait();
    delete worker_;
    delete bridge_;
    worker_ = nullptr;
    bridge_ = nullptr;
}

void ClassicGame::newGame()
{
    stopGame();
    log_->clear();
    input_->setEnabled(false);

    bridge_ = new GameBridge(this);
    connect(bridge_, &GameBridge::textOut, this, &ClassicGame::append);
    connect(bridge_, &GameBridge::inputRequested, this, &ClassicGame::awaitInput);
    connect(bridge_, &GameBridge::shotRequested, this, &ClassicGame::shoot);
    connect(bridge_, &GameBridge::gameEnded, this, &ClassicGame::gameEnded);

    GameBridge *bridge = bridge_;
    worker_ = QThread::create([bridge] {
        try {
            MtRng rng;
            oregon::Game game(*bridge, rng);
            emit bridge->gameEnded(game.play() == oregon::Outcome::Arrived);
        } catch (const GameBridge::Cancelled &) {
        }
    });
    worker_->start();
}

void ClassicGame::append(const QString &s)
{
    QTextCursor c = log_->textCursor();
    c.movePosition(QTextCursor::End);
    c.insertText(s);
    log_->setTextCursor(c);
    log_->verticalScrollBar()->setValue(log_->verticalScrollBar()->maximum());
}

void ClassicGame::awaitInput()
{
    input_->setEnabled(true);
    input_->setFocus();
}

void ClassicGame::reply()
{
    if (!input_->isEnabled())
        return;
    const QString text = input_->text();
    input_->clear();
    input_->setEnabled(false);
    append(text + "\n");
    bridge_->submit(text);
}

// The BASIC wants a shot: play a round at whatever it is facing.
void ClassicGame::shoot(int target)
{
    target_ = target;
    using T = oregon::Frontend::Target;
    const char *what = target == int(T::Game) ? "A DEER! TAKE YOUR SHOT!"
                     : target == int(T::Wolves) ? "WOLVES! SHOOT!"
                     : target == int(T::Riders) ? "THE RIDERS ARE COMING! SHOOT!"
                                                : "BANDITS! SHOOT!";
    append(QString("%1   (W A S D walk, mouse turns, click fires)\n").arg(what));

    std::unique_ptr<fight::Field> field;
    switch (target) {
    case int(T::Game):
        field = std::make_unique<fight::Field>(fight::Field::DuelTag{}, fight::Kind::Deer, kRoundBullets, *rng_);
        break;
    case int(T::Wolves):
        field = std::make_unique<fight::Field>(fight::Field::DuelTag{}, fight::Kind::Wolf, kRoundBullets, *rng_);
        break;
    default:
        field = std::make_unique<fight::Field>(fight::Field::DuelTag{}, fight::Kind::Bandit, kRoundBullets, *rng_);
        break;
    }
    pages_->setCurrentWidget(view_);
    view_->begin(std::move(field));
    view_->setFocus();
}

void ClassicGame::shotFinished()
{
    auto field = view_->take();
    pages_->setCurrentIndex(0);
    // Reaction time, in the BASIC's units: 1 or less is a perfect shot, 9 is a miss.
    double seconds = 9;
    if (field->killTime >= 0 && !field->downed) {
        seconds = kPerfect + kSlope * std::max(0.0, field->killTime - kFreeSeconds) + kPerHit * field->hits;
        seconds = std::min(seconds, 8.9);
    }
    input_->setFocus();
    bridge_->submitShot(seconds);
}

void ClassicGame::gameEnded(bool arrived)
{
    append(arrived ? "\n*** You made it! ***   (Game menu > Classic 1978 for another go)\n"
                   : "\n*** Game over ***   (Game menu > Classic 1978 for another go)\n");
}
