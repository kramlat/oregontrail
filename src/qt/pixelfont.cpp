// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixelfont.h"
#include "pixelfont_data.h"

namespace art {

int pixelTextWidth(const QString &text, int scale)
{
    if (text.isEmpty()) return 0;
    return int(text.size()) * (pixelfont::kWidth + 1) * scale - scale;
}

int drawPixelText(QPainter &p, const QString &text, int x, int y, int scale, const QColor &color)
{
    int cx = x;
    for (const QChar &qc : text) {
        const pixelfont::Glyph &g = pixelfont::glyphFor(qc.toLatin1());
        for (int row = 0; row < pixelfont::kHeight; ++row)
            for (int col = 0; col < pixelfont::kWidth; ++col)
                if (g.rows[row][col] == '#')
                    p.fillRect(cx + col * scale, y + row * scale, scale, scale, color);
        cx += (pixelfont::kWidth + 1) * scale;
    }
    return cx - x - scale;
}

int drawPixelTextCentered(QPainter &p, const QString &text, int cx, int y, int scale, const QColor &color)
{
    return drawPixelText(p, text, cx - pixelTextWidth(text, scale) / 2, y, scale, color);
}

} // namespace art
