cmake_minimum_required(VERSION 3.28...3.30)

project(shader_tests)

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/../../vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(DrawingEffect_shader_test DrawingEffect_shader_test.cpp)
target_link_libraries(DrawingEffect_shader_test PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(DrawingEffect_shader_test)
