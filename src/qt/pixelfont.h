// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QPainter>
#include <QString>

namespace art {

// Width in canvas pixels of `text` drawn at `scale` (one pixel of gap between letters, scaled).
int pixelTextWidth(const QString &text, int scale = 1);

// Draw text in the 5x7 font with its top-left at (x, y); returns the width drawn.
int drawPixelText(QPainter &p, const QString &text, int x, int y, int scale, const QColor &color);

// Centred on `cx`.
int drawPixelTextCentered(QPainter &p, const QString &text, int cx, int y, int scale, const QColor &color);

} // namespace art
