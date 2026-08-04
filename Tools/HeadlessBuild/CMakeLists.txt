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

# Two spellings, both load-bearing: RA4Presentation gates on HEADLESS_BUILD and
# FogOfWarGrid.h on RA4_HEADLESS. Defining only the first left the fog header taking
# its Unreal branch and reaching for CoreMinimal.h, which broke this build -- and with
# it the determinism job that is the whole reason the build exists.
add_compile_definitions(HEADLESS_BUILD=1 RA4_HEADLESS=1)

add_library(RA4Core STATIC
    ${RA4_SOURCE_ROOT}/RA4Core/Private/FixedMath.cpp
    ${RA4_SOURCE_ROOT}/RA4Core/Private/Command.cpp
)
target_include_directories(RA4Core PUBLIC ${RA4_SOURCE_ROOT}/RA4Core/Public)

add_library(RA4Content STATIC
    ${RA4_SOURCE_ROOT}/RA4Content/Private/ContentDatabase.cpp
    ${RA4_SOURCE_ROOT}/RA4Content/Private/DefaultContent.cpp
    ${RA4_SOURCE_ROOT}/RA4Content/Private/JsonParser.cpp
    ${RA4_SOURCE_ROOT}/RA4Content/Private/BibleContentLoader.cpp
)
target_include_directories(RA4Content PUBLIC ${RA4_SOURCE_ROOT}/RA4Content/Public)
target_link_libraries(RA4Content PUBLIC RA4Core)

add_library(RA4FogOfWar STATIC
    ${RA4_SOURCE_ROOT}/RA4FogOfWar/Private/FogOfWarGrid.cpp
)
target_include_directories(RA4FogOfWar PUBLIC ${RA4_SOURCE_ROOT}/RA4FogOfWar/Public)
target_link_libraries(RA4FogOfWar PUBLIC RA4Core)

add_library(RA4Navigation STATIC
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/FlowField.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/ReservationGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/MNavRouter.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/Formation.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavDebug.cpp
)
target_include_directories(RA4Navigation PUBLIC ${RA4_SOURCE_ROOT}/RA4Navigation/Public)
target_link_libraries(RA4Navigation PUBLIC RA4Core)

add_library(RA4Combat STATIC
    ${RA4_SOURCE_ROOT}/RA4Combat/Private/ArmorMatrix.cpp
)
target_include_directories(RA4Combat PUBLIC ${RA4_SOURCE_ROOT}/RA4Combat/Public)
target_link_libraries(RA4Combat PUBLIC RA4Content)

add_library(RA4Simulation STATIC
    ${RA4_SOURCE_ROOT}/RA4Simulation/Private/SimWorld.cpp
    ${RA4_SOURCE_ROOT}/RA4Simulation/Private/CommandBus.cpp
    ${RA4_SOURCE_ROOT}/RA4Simulation/Private/LockstepSession.cpp
)
target_include_directories(RA4Simulation PUBLIC ${RA4_SOURCE_ROOT}/RA4Simulation/Public)
target_link_libraries(RA4Simulation PUBLIC RA4Content RA4Navigation RA4FogOfWar RA4Combat)

add_library(RA4Input STATIC
    ${RA4_SOURCE_ROOT}/RA4Input/Private/CameraController.cpp
    ${RA4_SOURCE_ROOT}/RA4Input/Private/SelectionModel.cpp
    ${RA4_SOURCE_ROOT}/RA4Input/Private/OrderResolver.cpp
    ${RA4_SOURCE_ROOT}/RA4Input/Private/HitTest.cpp
    ${RA4_SOURCE_ROOT}/RA4Input/Private/ControlScheme.cpp
)
target_include_directories(RA4Input PUBLIC ${RA4_SOURCE_ROOT}/RA4Input/Public)
target_link_libraries(RA4Input PUBLIC RA4Simulation)

add_library(RA4Presentation STATIC
    ${RA4_SOURCE_ROOT}/RA4Presentation/Private/HudSnapshot.cpp
    ${RA4_SOURCE_ROOT}/RA4Presentation/Private/RA4ArtMapping.cpp
)
target_include_directories(RA4Presentation PUBLIC 
    ${RA4_SOURCE_ROOT}/RA4Presentation/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/UnrealStub
)
target_link_libraries(RA4Presentation PUBLIC RA4Simulation)

add_library(RA4AI STATIC
    ${RA4_SOURCE_ROOT}/RA4AI/Private/AIStrategy.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/AICommander.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/AIWorldView.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/TacticalOperation.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/ArmyGroup.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/AIDoctrine.cpp
    ${RA4_SOURCE_ROOT}/RA4AI/Private/AIDebugOverlay.cpp
)
target_include_directories(RA4AI PUBLIC ${RA4_SOURCE_ROOT}/RA4AI/Public)
target_link_libraries(RA4AI PUBLIC RA4Simulation RA4FogOfWar)

add_library(RA4Replay STATIC
    ${RA4_SOURCE_ROOT}/RA4Replay/Private/Replay.cpp
)
target_include_directories(RA4Replay PUBLIC ${RA4_SOURCE_ROOT}/RA4Replay/Public)
target_link_libraries(RA4Replay PUBLIC RA4Simulation)

add_library(RA4Campaign STATIC
    ${RA4_SOURCE_ROOT}/RA4Campaign/Private/CampaignDatabase.cpp
    ${RA4_SOURCE_ROOT}/RA4Campaign/Private/MissionRuntime.cpp
)
target_include_directories(RA4Campaign PUBLIC ${RA4_SOURCE_ROOT}/RA4Campaign/Public)
# MissionRuntime evaluates objectives against SimWorld, so the campaign layer now
# depends on the simulation rather than on content alone.
target_link_libraries(RA4Campaign PUBLIC RA4Simulation)

add_executable(RA4Tests
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestMain.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestCore.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestSimulation.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestVerticalSlice.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestNavigation.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestFogOfWar.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestInput.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestPresentation.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestAI.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestBibleContent.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestBibleImport.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestVeterancy.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestFactionResources.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestSaveSystem.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestCampaign.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/Test_HUDIntegrationAndGameState.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestProvingGround.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestNetwork.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestUI.cpp
)

target_link_libraries(RA4Tests PRIVATE RA4Replay RA4Campaign RA4Input RA4Presentation RA4AI RA4FogOfWar)
target_include_directories(RA4Tests PRIVATE ${RA4_SOURCE_ROOT}/RA4UI/Public)

enable_testing()
add_test(NAME core COMMAND RA4Tests)
set_tests_properties(core PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Focused binary for the input layer.
add_executable(RA4InputTests
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestMain.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestInput.cpp
)
target_link_libraries(RA4InputTests PRIVATE RA4Input)
add_test(NAME input COMMAND RA4InputTests)
set_tests_properties(input PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Focused binary for presentation projection.
add_executable(RA4PresentationTests
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestMain.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestPresentation.cpp
)
target_link_libraries(RA4PresentationTests PRIVATE RA4Presentation)
add_test(NAME presentation COMMAND RA4PresentationTests)
set_tests_properties(presentation PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Focused binary for AI.
add_executable(RA4AITests
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestMain.cpp
    ${RA4_SOURCE_ROOT}/RA4Tests/Private/TestAI.cpp
)
target_link_libraries(RA4AITests PRIVATE RA4AI)
add_test(NAME ai COMMAND RA4AITests)
set_tests_properties(ai PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Plays an AI-versus-AI match and writes it out for Tools/MatchViewer/render.py.
add_executable(RA4MatchDump ${CMAKE_CURRENT_SOURCE_DIR}/../MatchViewer/dump_match.cpp)
target_link_libraries(RA4MatchDump PRIVATE RA4AI)
