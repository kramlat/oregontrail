// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "oregon.h"

#include <QWidget>
#include <memory>

class GameBridge;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QThread;
class ShootView;

// The 1978 rules in a text window. Every shot the BASIC would have timed ("TYPE BANG")
// is a short round of the first-person shootout instead; how quickly you drop the target
// becomes the same reaction-time number the original used.
class ClassicGame : public QWidget {
    Q_OBJECT
public:
    explicit ClassicGame(QWidget *parent = nullptr);
    ~ClassicGame() override;
    void stopGame();

public slots:
    void newGame();

private slots:
    void append(const QString &s);
    void awaitInput();
    void reply();
    void shoot(int target);
    void shotFinished();
    void gameEnded(bool arrived);

private:
    QStackedWidget *pages_;
    QPlainTextEdit *log_;
    QLineEdit *input_;
    ShootView *view_;
    GameBridge *bridge_ = nullptr;
    QThread *worker_ = nullptr;
    std::unique_ptr<oregon::Rng> rng_;    // dice for the shooting rounds (GUI thread)
    int target_ = 0;
};
