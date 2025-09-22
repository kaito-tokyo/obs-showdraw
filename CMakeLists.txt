cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Network Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

find_package(cpr CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(semver CONFIG REQUIRED)
find_package(Backward CONFIG REQUIRED)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE NOMINMAX)

target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE OBS::libobs semver::semver cpr::cpr fmt::fmt Backward::Interface
)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/Core/MainPluginContext_c.cpp
    src/Core/MainPluginContext.cpp
    src/Core/RenderingContext.cpp
    src/plugin-main.c
)

if(APPLE)
  find_library(
      COREML_SHOWDRAW_CANVAS_DETECTOR_LOCATION
      NAMES CoreMLShowDrawCanvasDetector
      HINTS ${CMAKE_SOURCE_DIR}/build_vendor/Release
      REQUIRED
  )

  target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      src/Bridge/ShowDrawCanvasDetectorBridge.mm
  )

  target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      ${COREML_SHOWDRAW_CANVAS_DETECTOR_LOCATION}
      "-framework CoreML"
      "-framework Vision"
      "-framework CoreGraphics"
  )

  add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${COREML_SHOWDRAW_CANVAS_DETECTOR_LOCATION}
            $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/../Frameworks/CoreMLShowDrawCanvasDetector.framework
  )

  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "@loader_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH ON
  )
endif()

if(BUILD_TESTING)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BUILD_TESTING)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTING)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()
