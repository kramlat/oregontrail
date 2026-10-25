// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamebridge.h"

#include <QMutexLocker>

void GameBridge::text(const std::string &s)
{
    emit textOut(QString::fromStdString(s));
}

std::string GameBridge::ask(const std::string &prompt)
{
    emit textOut(QString::fromStdString(prompt) + "? ");
    emit inputRequested();
    return wait().toStdString();
}

oregon::Frontend::Shot GameBridge::shoot(const std::string &word)
{
    emit textOut(QString("TYPE %1\n").arg(QString::fromStdString(word)));
    emit inputRequested();
    double seconds = 0;
    std::string typed = wait(&seconds).toStdString();
    return {typed, seconds};
}

QString GameBridge::wait(double *seconds)
{
    QMutexLocker lock(&mutex_);
    while (!have_ && !cancelled_)
        reply_.wait(&mutex_);
    if (cancelled_)
        throw Cancelled{};
    have_ = false;
    if (seconds)
        *seconds = seconds_;
    return answer_;
}

void GameBridge::promptShown() { clock_.start(); }

void GameBridge::submit(const QString &reply)
{
    QMutexLocker lock(&mutex_);
    answer_ = reply;
    seconds_ = clock_.isValid() ? clock_.nsecsElapsed() / 1e9 : 0;
    have_ = true;
    reply_.wakeOne();
}

void GameBridge::cancel()
{
    QMutexLocker lock(&mutex_);
    cancelled_ = true;
    reply_.wakeAll();
}
