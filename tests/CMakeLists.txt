include(GoogleTest)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_compile_definitions(-DCMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_executable(Preset_test Preset_test.cpp)
target_link_libraries(Preset_test PRIVATE GTest::gtest_main OBS::libobs ${CMAKE_PROJECT_NAME}_impl)
gtest_discover_tests(Preset_test DISCOVERY_MODE PRE_TEST)

add_executable(UpdateChecker_test UpdateChecker_test.cpp)
target_link_libraries(
  UpdateChecker_test
  PRIVATE GTest::gtest_main semver::semver cpr::cpr nlohmann_json::nlohmann_json OBS::libobs ${CMAKE_PROJECT_NAME}_impl
)
gtest_discover_tests(UpdateChecker_test DISCOVERY_MODE PRE_TEST)

add_executable(ShowDrawFilterContext_test ShowDrawFilterContext_test.cpp)
target_link_libraries(
  ShowDrawFilterContext_test
  PRIVATE GTest::gtest_main semver::semver cpr::cpr nlohmann_json::nlohmann_json OBS::libobs ${CMAKE_PROJECT_NAME}_impl
)
gtest_discover_tests(ShowDrawFilterContext_test DISCOVERY_MODE PRE_TEST)

add_executable(DrawingEffect_shader_test DrawingEffect_shader_test.cpp)
target_link_libraries(
  DrawingEffect_shader_test
  PRIVATE GTest::gtest_main OBS::libobs ${CMAKE_PROJECT_NAME}_impl opencv_core opencv_imgproc opencv_imgcodecs
)
gtest_discover_tests(DrawingEffect_shader_test DISCOVERY_MODE PRE_TEST)

set(TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}")
file(COPY ${CMAKE_SOURCE_DIR}/data/effects/drawing.effect DESTINATION ${TEST_DATA_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fixtures DESTINATION ${TEST_DATA_PATH})

set_target_properties(
  Preset_test
  UpdateChecker_test
  ShowDrawFilterContext_test
  DrawingEffect_shader_test
  PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/preset_test.entitlements"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "/Applications/OBS.app/Contents/Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "/Applications/OBS.app/Contents/Frameworks"
)
