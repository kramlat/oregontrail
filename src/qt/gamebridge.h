// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "oregon.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QWaitCondition>

// Connects the blocking engine (worker thread) to the GUI thread.
// The worker calls the Frontend methods; they post signals to the window and
// sleep until submit() delivers the player's reply.
class GameBridge : public QObject, public oregon::Frontend {
    Q_OBJECT
public:
    struct Cancelled {};   // thrown in the worker when the game is abandoned

    using QObject::QObject;

    // Worker thread.
    void text(const std::string &s) override;
    std::string ask(const std::string &prompt) override;
    Shot shoot(const std::string &word) override;

public slots:
    // GUI thread.
    void promptShown();               // the input box just became usable
    void submit(const QString &reply);
    void cancel();

signals:
    void textOut(const QString &s);
    void inputRequested();
    void gameEnded(bool arrived);

private:
    QString wait(double *seconds = nullptr);

    QMutex mutex_;
    QWaitCondition reply_;
    bool have_ = false, cancelled_ = false;
    QString answer_;
    double seconds_ = 0;
    QElapsedTimer clock_;             // GUI thread only
};
