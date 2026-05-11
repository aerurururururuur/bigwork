cmake_minimum_required(VERSION 3.16)
project(roadside_stroll LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ROADSIDE_STROLL_FETCH_SFML "Download SFML 2.6 when not found on the system" ON)

find_package(SFML 2.5 COMPONENTS graphics window system main CONFIG QUIET)
if(NOT SFML_FOUND)
    find_package(SFML 2.5 COMPONENTS graphics window system main QUIET)
endif()

if(NOT SFML_FOUND AND ROADSIDE_STROLL_FETCH_SFML)
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
        GIT_SHALLOW TRUE
    )
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SFML)
endif()

if(TARGET SFML::Graphics)
    set(ROADSIDE_SFML_LIBS SFML::Graphics SFML::Window SFML::System)
elseif(TARGET sfml-graphics)
    set(ROADSIDE_SFML_LIBS sfml-graphics sfml-window sfml-system)
else()
    message(FATAL_ERROR "SFML 2.x (graphics, window, system) is required. Install it or set ROADSIDE_STROLL_FETCH_SFML=ON.")
endif()

set(ROADSIDE_SFML_MAIN_LIBS "")
if(TARGET SFML::Main)
    list(APPEND ROADSIDE_SFML_MAIN_LIBS SFML::Main)
elseif(TARGET sfml-main)
    list(APPEND ROADSIDE_SFML_MAIN_LIBS sfml-main)
endif()

add_library(roadside_domain STATIC
    src/domain/playfield.cpp
    src/domain/world.cpp
)
target_include_directories(roadside_domain PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(roadside_application STATIC
    src/application/game_application.cpp
)
target_include_directories(roadside_application PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(roadside_application PUBLIC roadside_domain)

add_library(roadside_infrastructure STATIC
    src/infrastructure/frame_timer.cpp
    src/infrastructure/win32_keyboard.cpp
    src/infrastructure/win32_console_terminal.cpp
    src/infrastructure/file_audit_sink.cpp
    src/infrastructure/std_random.cpp
    src/infrastructure/steady_clock.cpp
)
target_include_directories(roadside_infrastructure PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(roadside_infrastructure PUBLIC roadside_domain)

if(WIN32)
    target_compile_definitions(roadside_infrastructure PRIVATE ROADSIDE_STROLL_WIN32=1)
endif()

add_library(roadside_representation STATIC
    src/representation/key_mapping.cpp
    src/representation/frame_composer.cpp
    src/representation/theme_palette.cpp
    src/representation/sfml_game_window.cpp
)
target_include_directories(roadside_representation PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(roadside_representation PUBLIC roadside_application roadside_infrastructure ${ROADSIDE_SFML_LIBS})

add_executable(roadside_stroll src/main.cpp)
target_include_directories(roadside_stroll PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(roadside_stroll PRIVATE
    roadside_representation
    roadside_application
    roadside_infrastructure
    roadside_domain
    ${ROADSIDE_SFML_MAIN_LIBS}
)

if(MSVC)
    target_compile_options(roadside_stroll PRIVATE /W4 /permissive-)
endif()

if(WIN32 AND TARGET sfml-graphics)
    foreach(_sfml_dll IN ITEMS sfml-graphics sfml-window sfml-system)
        if(TARGET ${_sfml_dll})
            add_custom_command(
                TARGET roadside_stroll
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_sfml_dll}>
                        $<TARGET_FILE_DIR:roadside_stroll>
            )
        endif()
    endforeach()
endif()
