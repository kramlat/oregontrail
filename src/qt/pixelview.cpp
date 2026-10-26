// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixelview.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

PixelView::PixelView(QWidget *parent) : QWidget(parent), image(art::newCanvas())
{
    setMinimumSize(art::W * 2, art::H * 2);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QRect PixelView::target() const
{
    // Fit the canvas, keeping the aspect ratio (nearest-neighbour keeps it crisp).
    double s = std::min(double(width()) / art::W, double(height()) / art::H);
    int w = int(art::W * s), h = int(art::H * s);
    return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

QPoint PixelView::toCanvas(QPoint p) const
{
    QRect r = target();
    return QPoint((p.x() - r.x()) * art::W / std::max(1, r.width()), (p.y() - r.y()) * art::H / std::max(1, r.height()));
}

void PixelView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(target(), image);
}

void PixelView::mouseMoveEvent(QMouseEvent *e) { emit moved(toCanvas(e->pos())); }
void PixelView::mousePressEvent(QMouseEvent *e) { emit clicked(toCanvas(e->pos())); }
