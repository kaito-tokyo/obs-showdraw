include(GoogleTest)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(preset_test obs_test_environment.cpp test_preset.cpp)
target_include_directories(preset_test PRIVATE ../src)
target_link_libraries(preset_test PRIVATE GTest::gtest_main OBS::libobs ${CMAKE_PROJECT_NAME}_impl)
gtest_discover_tests(preset_test DISCOVERY_MODE PRE_TEST)

add_executable(UpdateChecker_test obs_test_environment.cpp UpdateChecker_test.cpp)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest OBS::libobs ${CMAKE_PROJECT_NAME}_impl)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)

set_target_properties(
  preset_test
  PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/preset_test.entitlements"
)

set_target_properties(
  UpdateChecker_test
  PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/preset_test.entitlements"
)
