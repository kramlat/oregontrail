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

# Qt interface.
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
add_executable(oregontrail
    src/qt/main.cpp src/qt/appwindow.cpp src/qt/appwindow.h
    src/qt/trailgame.cpp src/qt/trailgame.h src/qt/pixelart.cpp src/qt/pixelart.h
    src/qt/fightview.cpp src/qt/fightview.h src/qt/pixelview.cpp src/qt/pixelview.h src/qt/shootview.cpp src/qt/shootview.h src/qt/classicgame.cpp src/qt/classicgame.h src/qt/gamebridge.cpp src/qt/gamebridge.h
    src/qt/graveyard.cpp src/qt/graveyard.h)
target_link_libraries(oregontrail PRIVATE trail fight oregon Qt6::Widgets)
add_executable(render_art tests/render_art.cpp src/qt/pixelart.cpp src/qt/fightview.cpp)
target_include_directories(render_art PRIVATE src)
target_link_libraries(render_art PRIVATE trail fight Qt6::Gui)
add_executable(smoke_gui tests/smoke_gui.cpp src/qt/trailgame.cpp src/qt/trailgame.h src/qt/pixelart.cpp src/qt/pixelart.h src/qt/fightview.cpp src/qt/fightview.h src/qt/pixelview.cpp src/qt/pixelview.h src/qt/shootview.cpp src/qt/shootview.h
    src/qt/classicgame.cpp src/qt/classicgame.h src/qt/gamebridge.cpp src/qt/gamebridge.h
    src/qt/graveyard.cpp src/qt/graveyard.h)
target_include_directories(smoke_gui PRIVATE src)
target_link_libraries(smoke_gui PRIVATE trail fight oregon Qt6::Widgets)

# First-person shootout simulation (Qt-free).
add_library(fight STATIC src/fight/fight.cpp)
target_include_directories(fight PUBLIC src)
target_link_libraries(fight PUBLIC oregon)
target_compile_options(fight PRIVATE -Wall -Wextra)
add_executable(test_fight tests/test_fight.cpp)
target_link_libraries(test_fight PRIVATE fight)
add_test(NAME fight COMMAND test_fight)

add_executable(test_graveyard tests/test_graveyard.cpp src/qt/graveyard.cpp)
target_include_directories(test_graveyard PRIVATE src)
target_link_libraries(test_graveyard PRIVATE Qt6::Core)
add_test(NAME graveyard COMMAND test_graveyard)
