cmake_minimum_required(VERSION 3.21)
project(oregontrail VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt-free engine: a transcode of the 1978 Creative Computing listing.
add_library(oregon STATIC src/oregon.cpp)
target_include_directories(oregon PUBLIC src)
target_compile_options(oregon PRIVATE -Wall -Wextra)

enable_testing()
add_executable(test_oregon tests/test_oregon.cpp)
target_link_libraries(test_oregon PRIVATE oregon)
add_test(NAME oregon COMMAND test_oregon)

# 1985-rules engine and its balance simulator.
add_library(trail STATIC src/trail/trail.cpp)
target_include_directories(trail PUBLIC src)
target_link_libraries(trail PUBLIC oregon)
target_compile_options(trail PRIVATE -Wall -Wextra)
add_executable(sim tests/sim.cpp)
target_link_libraries(sim PRIVATE trail)
add_executable(test_trail tests/test_trail.cpp)
target_link_libraries(test_trail PRIVATE trail)
add_test(NAME trail COMMAND test_trail)

# Qt interface: one library shared by the game, the smoke driver and the art renderer.
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)
add_library(oregonui STATIC
    src/qt/appdata.cpp src/qt/appwindow.cpp src/qt/audio.cpp src/qt/classicgame.cpp src/qt/fightview.cpp
    src/qt/gamebridge.cpp src/qt/graveyard.cpp src/qt/pixelart.cpp src/qt/pixelfont.cpp src/qt/pixelview.cpp
    src/qt/shootview.cpp src/qt/titlescreen.cpp src/qt/trailgame.cpp)
target_include_directories(oregonui PUBLIC src)
target_link_libraries(oregonui PUBLIC trail fight oregon Qt6::Widgets Qt6::Multimedia)
# Where audio.cpp looks for the music: the installed copy, then the source tree (for a build in place).
set_source_files_properties(src/qt/audio.cpp PROPERTIES COMPILE_DEFINITIONS
    "OREGONTRAIL_INSTALL_DATA=\"${CMAKE_INSTALL_FULL_DATADIR}/oregontrail\";OREGONTRAIL_SOURCE_ASSETS=\"${CMAKE_SOURCE_DIR}/assets\"")

add_executable(oregontrail src/qt/main.cpp)
target_link_libraries(oregontrail PRIVATE oregonui)
install(TARGETS oregontrail RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/audio DESTINATION ${CMAKE_INSTALL_DATADIR}/oregontrail FILES_MATCHING PATTERN "*.ogg")
install(FILES assets/CREDITS.md DESTINATION ${CMAKE_INSTALL_DATADIR}/oregontrail)

add_executable(audio_probe tests/audio_probe.cpp)
target_link_libraries(audio_probe PRIVATE oregonui)
add_executable(render_art tests/render_art.cpp)
target_link_libraries(render_art PRIVATE oregonui)
add_executable(smoke_gui tests/smoke_gui.cpp)
target_link_libraries(smoke_gui PRIVATE oregonui)

# First-person shootout simulation (Qt-free).
add_library(fight STATIC src/fight/fight.cpp)
target_include_directories(fight PUBLIC src)
target_link_libraries(fight PUBLIC oregon)
target_compile_options(fight PRIVATE -Wall -Wextra)
add_executable(test_fight tests/test_fight.cpp)
target_link_libraries(test_fight PRIVATE fight)
add_test(NAME fight COMMAND test_fight)

add_executable(test_graveyard tests/test_graveyard.cpp src/qt/graveyard.cpp src/qt/appdata.cpp)
target_include_directories(test_graveyard PRIVATE src)
target_link_libraries(test_graveyard PRIVATE Qt6::Core)
add_test(NAME graveyard COMMAND test_graveyard)

# The title screen font (Qt-free data), and the audio files against their credits.
add_executable(test_pixelfont tests/test_pixelfont.cpp)
target_include_directories(test_pixelfont PRIVATE src)
add_test(NAME pixelfont COMMAND test_pixelfont)
add_executable(test_audio_assets tests/test_audio_assets.cpp)
target_compile_definitions(test_audio_assets PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME audio_assets COMMAND test_audio_assets)

# Audio that really decodes through Qt, and the window that opens on the title screen.
add_executable(test_audio tests/test_audio.cpp)
target_link_libraries(test_audio PRIVATE oregonui)
target_compile_definitions(test_audio PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME audio COMMAND test_audio)
set_tests_properties(audio PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
add_test(NAME title_screen COMMAND smoke_gui 0 /dev/null title)
set_tests_properties(title_screen PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;OREGONTRAIL_AUDIO=off")
