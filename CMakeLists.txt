cmake_minimum_required(VERSION 3.20)

project(XSmallHMI LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(XSmallHMI
    src/main.cpp
)

set(SFML_DIR "C:/SFML-2.6.2/lib/cmake/SFML")
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

target_link_libraries(XSmallHMI PRIVATE sfml-graphics sfml-window sfml-system)

add_custom_command(TARGET XSmallHMI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/fonts/Roboto-Regular.ttf"
        "$<TARGET_FILE_DIR:XSmallHMI>/Roboto-Regular.ttf"
)

add_custom_command(TARGET XSmallHMI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:XSmallHMI>/assets"
)

if (WIN32)
    set(SFML_BIN "C:/SFML-2.6.2/bin")
    add_custom_command(TARGET XSmallHMI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<$<CONFIG:Debug>:${SFML_BIN}/sfml-graphics-d-2.dll>$<$<NOT:$<CONFIG:Debug>>:${SFML_BIN}/sfml-graphics-2.dll>"
            "$<$<CONFIG:Debug>:${SFML_BIN}/sfml-window-d-2.dll>$<$<NOT:$<CONFIG:Debug>>:${SFML_BIN}/sfml-window-2.dll>"
            "$<$<CONFIG:Debug>:${SFML_BIN}/sfml-system-d-2.dll>$<$<NOT:$<CONFIG:Debug>>:${SFML_BIN}/sfml-system-2.dll>"
            "$<TARGET_FILE_DIR:XSmallHMI>"
    )
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(XSmallHMI_tests
    tests/test_core.cpp
)

target_link_libraries(XSmallHMI_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(XSmallHMI_tests)