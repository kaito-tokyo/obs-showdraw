include(GoogleTest)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(Preset_test obs_test_environment.cpp test_preset.cpp)
target_link_libraries(Preset_test PRIVATE GTest::gtest_main OBS::libobs ${CMAKE_PROJECT_NAME}_impl)
gtest_discover_tests(Preset_test DISCOVERY_MODE PRE_TEST)

add_executable(UpdateChecker_test obs_test_environment.cpp UpdateChecker_test.cpp)
target_link_libraries(UpdateChecker_test PRIVATE GTest::gtest_main cpr::cpr OBS::libobs ${CMAKE_PROJECT_NAME}_impl)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)

set_target_properties(
  Preset_test
  UpdateChecker_test
  PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/preset_test.entitlements"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "/Applications/OBS.app/Contents/Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/Applications/OBS.app/Contents/Frameworks"
)
