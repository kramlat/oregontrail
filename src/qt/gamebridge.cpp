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
    wait();
    QMutexLocker lock(&mutex_);
    return answer_.toStdString();
}

double GameBridge::shoot(Target target)
{
    emit shotRequested(int(target));
    wait();
    QMutexLocker lock(&mutex_);
    return seconds_;
}

void GameBridge::wait()
{
    QMutexLocker lock(&mutex_);
    while (!have_ && !cancelled_)
        reply_.wait(&mutex_);
    if (cancelled_)
        throw Cancelled{};
    have_ = false;
}

void GameBridge::submit(const QString &reply)
{
    QMutexLocker lock(&mutex_);
    answer_ = reply;
    have_ = true;
    reply_.wakeOne();
}

void GameBridge::submitShot(double seconds)
{
    QMutexLocker lock(&mutex_);
    seconds_ = seconds;
    have_ = true;
    reply_.wakeOne();
}

void GameBridge::cancel()
{
    QMutexLocker lock(&mutex_);
    cancelled_ = true;
    reply_.wakeAll();
}
