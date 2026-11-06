// SPDX-License-Identifier: GPL-3.0-or-later
//
// Nothing ships unattributed: every audio file must be listed in assets/CREDITS.md, every
// credit must have a file, and each file must really be an Ogg of a sensible size.
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } \
    } while (0)

int main()
{
    const fs::path root = SOURCE_DIR;
    const fs::path audio = root / "assets" / "audio";
    std::ifstream in(root / "assets" / "CREDITS.md");
    CHECK(in.good());
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string credits = ss.str();

    // The files the game asks for (see Audio::fileFor).
    const std::set<std::string> wanted = {"title.ogg", "travel.ogg", "fight.ogg", "victory.ogg", "taps.ogg"};
    std::set<std::string> found;
    for (const auto &e : fs::directory_iterator(audio)) {
        if (e.path().extension() != ".ogg") continue;
        const std::string name = e.path().filename().string();
        found.insert(name);
        CHECK(credits.find("audio/" + name) != std::string::npos);   // credited
        const auto size = fs::file_size(e.path());
        CHECK(size > 20 * 1024 && size < 5 * 1024 * 1024);           // not empty, not a surprise
        std::ifstream f(e.path(), std::ios::binary);
        char magic[4] = {};
        f.read(magic, 4);
        CHECK(std::string(magic, 4) == "OggS");
    }
    CHECK(found == wanted);                                          // all there, and nothing unexpected

    // Each credited file is real: every `audio/xyz.ogg` named in the credits exists.
    for (size_t pos = credits.find("audio/"); pos != std::string::npos; pos = credits.find("audio/", pos + 1)) {
        size_t end = credits.find(".ogg", pos);
        if (end == std::string::npos || end - pos > 40) continue;
        CHECK(fs::exists(root / "assets" / credits.substr(pos, end + 4 - pos)));
    }
    // The credits say where each came from and what we could not verify.
    CHECK(credits.find("PD-USGov-Military-Air Force") != std::string::npos);
    CHECK(credits.find("could not read") != std::string::npos);
    std::printf(failures ? "%d FAILED\n" : "all passed (%zu files)\n", failures ? failures : int(found.size()));
    return failures ? 1 : 0;
}
