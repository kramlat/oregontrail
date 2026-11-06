// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// The pieces of music the game asks for. What plays for each is in Audio.
enum class Music {
    None,
    Title,      // the title screen
    Travel,     // on the trail (and the Classic text mode)
    Fight,      // a gunfight with bandits or wolves
    Victory,    // arriving in Oregon (plays once)
    Taps,       // at a tombstone (plays once)
};
