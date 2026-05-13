include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    # Avoid shared gtest DLLs (Windows build-time discovery cannot find them next to roadside_tests.exe).
    set(_roads_saved_build_shared "${BUILD_SHARED_LIBS}")
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
    if(DEFINED _roads_saved_build_shared)
        set(BUILD_SHARED_LIBS "${_roads_saved_build_shared}")
    else()
        unset(BUILD_SHARED_LIBS)
    endif()
endif()

add_executable(
    roadside_tests
    game_config_test.cpp
    score_state_test.cpp
    playfield_test.cpp
)
target_include_directories(roadside_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(
    roadside_tests
    PRIVATE
    GTest::gtest_main
    roadside_infrastructure
    roadside_domain
)
if(MSVC)
    target_compile_options(roadside_tests PRIVATE /W4 /permissive-)
endif()

include(GoogleTest)
gtest_discover_tests(roadside_tests DISCOVERY_TIMEOUT 60)
