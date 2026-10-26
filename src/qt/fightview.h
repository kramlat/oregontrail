// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fight/fight.h"

#include <QImage>

// Raycasts the shootout onto a 280x192 canvas in the Apple II palette:
// wall columns by DDA, billboard sprites occluded by a depth buffer, haze by
// dithering, a rifle with muzzle flash, and arrows for enemies out of view.
void renderFight(QImage &img, const fight::Field &field);
