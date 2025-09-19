include(GoogleTest)

if(OS_MACOS)
  add_executable(ShowDrawCanvasDetector_test ShowDrawCanvasDetector_test.mm)
  target_link_libraries(ShowDrawCanvasDetector_test PRIVATE GTest::gtest_main ShowDrawCanvasDetector)
  gtest_discover_tests(ShowDrawCanvasDetector_test DISCOVERY_MODE PRE_TEST)
endif()
