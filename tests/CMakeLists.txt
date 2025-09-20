cmake_minimum_required(VERSION 4.1)

#include(GoogleTest)

if(APPLE)
  # テスト用の実行ファイルターゲットを定義
  add_executable(ShowDrawCanvasDetector_test)

target_sources(ShowDrawCanvasDetector_test PRIVATE
    ShowDrawCanvasDetector_test.mm
    ${CMAKE_SOURCE_DIR}/../src/CanvasDetector/ShowDrawCanvasDetector.mlpackage
)

# .mlpackageがXcode上で特別な「パッケージ」として扱われるようにプロパティを設定
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/../src/CanvasDetector/ShowDrawCanvasDetector.mlpackage
    PROPERTIES
    XCODE_EXPLICIT_FILE_TYPE "public.objective-c-plus-plus-source"
)
#  target_link_libraries(ShowDrawCanvasDetector_test PRIVATE GTest::gtest_main)

  # GTestの設定
#  gtest_discover_tests(ShowDrawCanvasDetector_test DISCOVERY_MODE PRE_TEST)
endif()
