// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pixelart.h"

#include <QImage>
#include <QWidget>

// Shows a QImage scaled to fit with crisp pixels; reports mouse in canvas coords.
class PixelView : public QWidget {
    Q_OBJECT
public:
    explicit PixelView(QWidget *parent = nullptr);
    QImage image;
signals:
    void moved(QPoint canvasPos);
    void clicked(QPoint canvasPos);
protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
private:
    QRect target() const;
    QPoint toCanvas(QPoint widgetPos) const;
};
