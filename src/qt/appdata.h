// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

// Where the game keeps its saved files (graves, settings). $OREGONTRAIL_DATA_DIR if set
// (the tests use this so they never touch a real save), otherwise the user's application
// data directory.
QString appDataDir();
