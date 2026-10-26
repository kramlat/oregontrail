// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "oregon.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QWaitCondition>

// Connects the blocking engine (worker thread) to the GUI thread.
// The worker calls the Frontend methods; they post signals to the window and
// sleep until the GUI delivers the player's reply (typed text, or the result of a
// shooting round).
class GameBridge : public QObject, public oregon::Frontend {
    Q_OBJECT
public:
    struct Cancelled {};   // thrown in the worker when the game is abandoned

    using QObject::QObject;

    // Worker thread.
    void text(const std::string &s) override;
    std::string ask(const std::string &prompt) override;
    double shoot(Target target) override;

public slots:
    // GUI thread.
    void submit(const QString &reply);
    void submitShot(double seconds);
    void cancel();

signals:
    void textOut(const QString &s);
    void inputRequested();
    void shotRequested(int target);          // an oregon::Frontend::Target
    void gameEnded(bool arrived);

private:
    void wait();

    QMutex mutex_;
    QWaitCondition reply_;
    bool have_ = false, cancelled_ = false;
    QString answer_;
    double seconds_ = 0;
};
