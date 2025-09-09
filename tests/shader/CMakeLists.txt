cmake_minimum_required(VERSION 3.28...3.30)

project(shader_tests)

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/../../vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

find_package(directxtk12 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(DrawingEffect_shader_test DrawingEffect_shader_test.cpp)
target_link_libraries(DrawingEffect_shader_test PRIVATE GTest::gtest GTest::gtest_main)

if(WIN32)
  target_link_libraries(DrawingEffect_shader_test PRIVATE Microsoft::DirectXTK12)
else()
  set_target_properties(
    DrawingEffect_shader_test
    PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
      XCODE_ATTRIBUTE_CODE_SIGN_ENTLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/../test.entitlements"
  )
endif()

gtest_discover_tests(DrawingEffect_shader_test DISCOVERY_MODE PRE_TEST)
