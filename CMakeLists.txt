cmake_minimum_required(VERSION 3.16)
project(PesMotion
        VERSION 0.1.0
        DESCRIPTION "Hyper Motion ultra-leve recriado para eFootball PES 2013-2021"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(PESMOTION_SOURCES
    src/math3d.cpp
    src/skeleton.cpp
    src/bvh.cpp
    src/retarget.cpp
    src/pmca.cpp
    src/pesbin.cpp
    src/selftest.cpp)

add_library(pesmotion_core STATIC ${PESMOTION_SOURCES})
target_include_directories(pesmotion_core PUBLIC src)
target_compile_definitions(pesmotion_core PUBLIC PESMOTION_VERSION="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(pesmotion_core PRIVATE /W3)
else()
    target_compile_options(pesmotion_core PRIVATE -Wall -Wextra)
endif()

add_executable(pesmotion src/cli.cpp)
target_link_libraries(pesmotion PRIVATE pesmotion_core)
set_target_properties(pesmotion PROPERTIES OUTPUT_NAME "pesmotion")

install(TARGETS pesmotion RUNTIME DESTINATION bin)

# Testes (ctest)
include(CTest)
if(BUILD_TESTING)
    add_executable(pesmotion_tests tests/test_main.cpp)
    target_link_libraries(pesmotion_tests PRIVATE pesmotion_core)
    add_test(NAME pesmotion_tests COMMAND pesmotion_tests)
    # Smoke test: o binário deve responder ao selftest com exit code 0.
    add_test(NAME pesmotion_selftest COMMAND pesmotion selftest)
endif()
