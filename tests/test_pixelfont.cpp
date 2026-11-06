// SPDX-License-Identifier: GPL-3.0-or-later
#include "qt/pixelfont_data.h"

#include <cstdio>
#include <cstring>
#include <initializer_list>

static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } \
    } while (0)

int main()
{
    // Every glyph is exactly 7 rows of 5 cells, '#' or '.'.
    for (const auto &g : pixelfont::kGlyphs) {
        for (int r = 0; r < pixelfont::kHeight; ++r) {
            CHECK(g.rows[r] && std::strlen(g.rows[r]) == pixelfont::kWidth);
            for (const char *c = g.rows[r]; c && *c; ++c) CHECK(*c == '#' || *c == '.');
        }
    }
    // Everything the title screen and menus say can be drawn.
    for (char c = 'A'; c <= 'Z'; ++c) CHECK(pixelfont::glyphFor(c).ch == c);
    for (char c = '0'; c <= '9'; ++c) CHECK(pixelfont::glyphFor(c).ch == c);
    for (char c : {' ', '.', ',', ':', '!', '?', '-', '\'', '/', '(', ')', '>', '<'}) CHECK(pixelfont::glyphFor(c).ch == c);
    CHECK(pixelfont::glyphFor('g').ch == 'G');              // lower case folds up
    CHECK(pixelfont::glyphFor('\x01').ch == '?');           // unknown characters show as '?'
    // No two glyphs look the same (a copy-paste slip would make two letters identical).
    const int n = int(sizeof pixelfont::kGlyphs / sizeof pixelfont::kGlyphs[0]);
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            bool same = true;
            for (int r = 0; r < pixelfont::kHeight && same; ++r) same = !std::strcmp(pixelfont::kGlyphs[i].rows[r], pixelfont::kGlyphs[j].rows[r]);
            CHECK(!same);
        }
    // Each letter lights some pixels (apart from the space).
    for (const auto &g : pixelfont::kGlyphs) {
        int lit = 0;
        for (int r = 0; r < pixelfont::kHeight; ++r) for (const char *c = g.rows[r]; *c; ++c) lit += *c == '#';
        CHECK(g.ch == ' ' ? lit == 0 : lit > 0);
    }
    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
