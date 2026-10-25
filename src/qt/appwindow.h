// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMainWindow>

class ClassicGame;
class TrailGame;
class QStackedWidget;

class AppWindow : public QMainWindow {
    Q_OBJECT
public:
    AppWindow();

private:
public slots:
    void playTrail();
private:
    void playClassic();

    QStackedWidget *stack_;
    TrailGame *trail_;
    ClassicGame *classic_;
};
