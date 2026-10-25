// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

class GameBridge;
class QLineEdit;
class QPlainTextEdit;
class QThread;

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
    void gameEnded(bool arrived);

private:
    QPlainTextEdit *log_;
    QLineEdit *input_;
    GameBridge *bridge_ = nullptr;
    QThread *worker_ = nullptr;
};
