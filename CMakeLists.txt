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
    src/qt/classicgame.cpp src/qt/classicgame.h src/qt/gamebridge.cpp src/qt/gamebridge.h)
target_link_libraries(oregontrail PRIVATE trail oregon Qt6::Widgets)
add_executable(render_art tests/render_art.cpp src/qt/pixelart.cpp)
target_include_directories(render_art PRIVATE src)
target_link_libraries(render_art PRIVATE trail Qt6::Gui)
add_executable(smoke_gui tests/smoke_gui.cpp src/qt/trailgame.cpp src/qt/trailgame.h src/qt/pixelart.cpp src/qt/pixelart.h)
target_include_directories(smoke_gui PRIVATE src)
target_link_libraries(smoke_gui PRIVATE trail oregon Qt6::Widgets)
