// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pixelview.h"

#include <QStringList>

class QTimer;

// The first thing you see: nothing starts until you choose New Game here.
class TitleScreen : public PixelView {
    Q_OBJECT
public:
    explicit TitleScreen(QWidget *parent = nullptr);

    void setMusicOn(bool on);
    int selected() const { return selected_; }
    QStringList items() const { return items_; }
    void activate(int index);                // as if chosen with Enter or a click

signals:
    void newGame();
    void classic();
    void toggleMusic();
    void quit();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;

private:
    void render();
    int itemAt(QPoint canvasPos) const;

    QStringList items_;
    int selected_ = 0, frame_ = 0;
    double scroll_ = 0;
    QTimer *timer_;
};
