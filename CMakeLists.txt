cmake_minimum_required(VERSION 3.20)

project(dev_challenge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FRAMEWORK_SOURCES
        src/antworld.cpp
        src/useable_functions.cpp
)

set(STATE_SOURCES
    src/exploration.cpp
    src/food.cpp
    src/follow_trail.cpp
)

add_executable(dev_challenge
        src/main.cpp
        ${FRAMEWORK_SOURCES}
        ${STATE_SOURCES}
)

target_include_directories(dev_challenge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

option(CAV_DEBUG_MOVEMENT "Debug option to print stack on every instruction." OFF)
option(CAV_DEBUG_EXPLORATION "Debug option to print which action was chosen." OFF)
option(CAV_DEBUG_STATE_TRANSITION "Debug option to log state transitions." OFF)
option(CAV_DEBUG_SINGLE_ANT "Debug option to monitor one single ant only." OFF)

if (CAV_DEBUG_MOVEMENT)
    target_compile_definitions(dev_challenge PUBLIC DEBUG_MOVEMENT)
endif()

if (CAV_DEBUG_EXPLORATION)
    target_compile_definitions(dev_challenge PUBLIC DEBUG_EXPLORATION)
endif()

if (CAV_DEBUG_STATE_TRANSITION)
    target_compile_definitions(dev_challenge PUBLIC DEBUG_STATE_TRANSITION)
endif()

if (CAV_DEBUG_SINGLE_ANT)
    target_compile_definitions(dev_challenge PUBLIC DEBUG_SINGLE_ANT)
endif()

if (MSVC)
    target_compile_options(dev_challenge PRIVATE /W4)
else ()
    target_compile_options(dev_challenge PRIVATE -Wall -Wextra -Wpedantic)
endif ()

include(CTest)

if (BUILD_TESTING)
    add_executable(antworld_tests
            test/antworld_tests.cpp
            ${FRAMEWORK_SOURCES}
            ${STATE_SOURCES}
    )

    target_include_directories(antworld_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if (MSVC)
        target_compile_options(antworld_tests PRIVATE /W4)
    else ()
        target_compile_options(antworld_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif ()

    add_test(
            NAME antworld_framework_tests
            COMMAND antworld_tests
    )
endif ()
