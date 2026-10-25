// SPDX-License-Identifier: GPL-3.0-or-later
#include "classicgame.h"
#include "gamebridge.h"

#include <QFontDatabase>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QThread>
#include <QVBoxLayout>

#include <random>

namespace {
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

    auto *box = new QVBoxLayout(this);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(log_, 1);
    box->addWidget(input_);

    connect(input_, &QLineEdit::returnPressed, this, &ClassicGame::reply);
}

ClassicGame::~ClassicGame() { stopGame(); }

void ClassicGame::stopGame()
{
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
    bridge_->promptShown();
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

void ClassicGame::gameEnded(bool arrived)
{
    append(arrived ? "\n*** You made it! ***   (Game menu > Classic 1978 for another go)\n"
                   : "\n*** Game over ***   (Game menu > Classic 1978 for another go)\n");
}
