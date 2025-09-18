# include(GoogleTest)

# function(add_obs_showdraw_test test_name)
#   set(discover_tests_flag TRUE)
#   set(libraries)
#   foreach(arg IN ITEMS ${ARGN})
#     if(arg STREQUAL "NO_DISCOVER_TESTS")
#       set(discover_tests_flag FALSE)
#     else()
#       list(APPEND libraries ${arg})
#     endif()
#   endforeach()

#   add_executable(${test_name} ${test_name}.cpp)

#   file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/obs-studio/bin/64bit)

#   target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
#   target_compile_definitions(${test_name} PRIVATE CMAKE_SOURCE_DIR\="${CMAKE_SOURCE_DIR}")
#   target_link_libraries(
#     ${test_name}
#     PRIVATE GTest::gtest_main plugin-support OBS::libobs ${CMAKE_PROJECT_NAME}_impl ${libraries}
#   )

#   set_target_properties(
#     ${test_name}
#     PROPERTIES
#       XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
#       XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
#       XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/test.entitlements"
#       XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "/Applications/OBS.app/Contents/Frameworks"
#       BUILD_WITH_INSTALL_RPATH TRUE
#       INSTALL_RPATH "/Applications/OBS.app/Contents/Frameworks"
#   )

#   if(discover_tests_flag)
#     gtest_discover_tests(
#       ${test_name}
#       DISCOVERY_MODE PRE_TEST
#       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/obs-studio/bin/64bit
#     )
#   endif()
# endfunction()

# add_obs_showdraw_test(Preset_test)
# add_obs_showdraw_test(UpdateChecker_test semver::semver cpr::cpr
# )
# add_obs_showdraw_test(ShowDrawFilterContext_test semver::semver cpr::cpr opencv_core opencv_imgproc opencv_imgcodecs
# )
# add_obs_showdraw_test(AsyncTextureReader_test opencv_imgcodecs)
