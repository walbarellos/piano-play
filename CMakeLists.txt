cmake_minimum_required(VERSION 3.20)
project(abntpiano VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directory
include_directories(include)

# Core library (Modularized by Architecture Layers)
add_library(abntpiano_core STATIC
    src/content/MidiImporter.cpp
    src/content/SongCatalog.cpp
    src/chart/ChartGenerator.cpp
    src/game/JudgementEngine.cpp
    src/game/ScoringEngine.cpp
    src/game/SongModeController.cpp
    src/game/DemoPlayer.cpp
    src/audio/SynthEngine.cpp
    src/persistence/SaveGameRepository.cpp
)

target_include_directories(abntpiano_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Tests
enable_testing()

add_executable(test_note docs/test_note.cpp)
target_link_libraries(test_note PRIVATE abntpiano_core)
add_test(NAME test_note COMMAND test_note)

add_executable(test_judgement tests/test_judgement.cpp)
target_link_libraries(test_judgement PRIVATE abntpiano_core)
add_test(NAME test_judgement COMMAND test_judgement)

add_executable(test_midi_importer tests/test_midi_importer.cpp)
target_link_libraries(test_midi_importer PRIVATE abntpiano_core)
add_test(NAME test_midi_importer COMMAND test_midi_importer)

add_executable(test_chart_generator tests/test_chart_generator.cpp)
target_link_libraries(test_chart_generator PRIVATE abntpiano_core)
add_test(NAME test_chart_generator COMMAND test_chart_generator)

add_executable(test_save_game tests/test_save_game.cpp)
target_link_libraries(test_save_game PRIVATE abntpiano_core)
add_test(NAME test_save_game COMMAND test_save_game)

add_executable(test_scoring tests/test_scoring.cpp)
target_link_libraries(test_scoring PRIVATE abntpiano_core)
add_test(NAME test_scoring COMMAND test_scoring)

add_executable(test_controllers tests/test_controllers.cpp)
target_link_libraries(test_controllers PRIVATE abntpiano_core)
add_test(NAME test_controllers COMMAND test_controllers)

add_executable(test_synth tests/test_synth.cpp)
target_link_libraries(test_synth PRIVATE abntpiano_core)
add_test(NAME test_synth COMMAND test_synth)

# Main Application (SDL2 Interactive Game)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)

add_executable(abntpiano_app
    src/main.cpp
    src/app/App.cpp
    src/audio/AudioDevice.cpp
    src/ui/RenderTypes.cpp
    src/ui/ParticleSystem.cpp
    src/ui/KeyboardRenderer.cpp
    src/ui/HighwayRenderer.cpp
    src/ui/HudRenderer.cpp
    src/ui/ResultsOverlay.cpp
)
target_include_directories(abntpiano_app PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(abntpiano_app PRIVATE abntpiano_core ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
