# Headless build of the Red Alert 4 simulation core.
#
# The core modules deliberately have no Unreal dependency, so they compile with a
# plain toolchain in seconds. CI runs this on every commit; the far slower Unreal
# build and Gauntlet suite run on merge. Both compile the identical sources -- this
# is not a parallel implementation.
cmake_minimum_required(VERSION 3.20)
project(RedAlert4Core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(RA4_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../Source)

# Warnings are errors here. The simulation is the one place where an implicit
# narrowing conversion or an uninitialised field turns into a desync that only
# reproduces on one player's machine.
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
else()
    add_compile_options(
        -Wall -Wextra -Werror
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wold-style-cast
        -Wno-unused-parameter
    )
endif()

add_library(RA4Core STATIC
    ${RA4_SOURCE_ROOT}/RA4Core/Private/FixedMath.cpp
    ${RA4_SOURCE_ROOT}/RA4Core/Private/Command.cpp
)
target_include_directories(RA4Core PUBLIC ${RA4_SOURCE_ROOT}/RA4Core/Public)

add_library(RA4Content STATIC
    ${RA4_SOURCE_ROOT}/RA4Content/Private/ContentDatabase.cpp
    ${RA4_SOURCE_ROOT}/RA4Content/Private/DefaultContent.cpp
)
target_include_directories(RA4Content PUBLIC ${RA4_SOURCE_ROOT}/RA4Content/Public)
target_link_libraries(RA4Content PUBLIC RA4Core)

add_library(RA4Simulation STATIC
    ${RA4_SOURCE_ROOT}/RA4Simulation/Private/SimWorld.cpp
)
target_include_directories(RA4Simulation PUBLIC ${RA4_SOURCE_ROOT}/RA4Simulation/Public)
target_link_libraries(RA4Simulation PUBLIC RA4Content)

add_library(RA4Replay STATIC
    ${RA4_SOURCE_ROOT}/RA4Replay/Private/Replay.cpp
)
target_include_directories(RA4Replay PUBLIC ${RA4_SOURCE_ROOT}/RA4Replay/Public)
target_link_libraries(RA4Replay PUBLIC RA4Simulation)

add_executable(RA4Tests
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestMain.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestCore.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestSimulation.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestVerticalSlice.cpp
)
target_link_libraries(RA4Tests PRIVATE RA4Replay)

enable_testing()
add_test(NAME core COMMAND RA4Tests)
