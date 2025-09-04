add_executable(preset_test test_mock.cpp test_preset.cpp obs_test_environment.cpp)

if(APPLE)
  set_target_properties(
    preset_test
    PROPERTIES 
     XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
     XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
     XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/preset_test.entitlements"
  )
endif()

target_include_directories(preset_test PRIVATE ../src)

target_link_libraries(
  preset_test
  PRIVATE
    GTest::gtest_main
    OBS::libobs
    ${CMAKE_PROJECT_NAME}_impl
)

include(GoogleTest)
gtest_discover_tests(preset_test DISCOVERY_MODE PRE_TEST)