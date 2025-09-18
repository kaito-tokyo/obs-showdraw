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

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE plugin-support)

if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
elseif(NOT $ENV{VCPKG_INSTALLATION_ROOT} STREQUAL "")
  set(VCPKG_ROOT $ENV{VCPKG_INSTALLATION_ROOT})
else()
  message(FATAL_ERROR "VCPKG_ROOT or VCPKG_INSTALLATION_ROOT environment variable is not set")
endif()

if(OS_MACOS)
  message(STATUS "macOS detected. Running vcpkg setup script...")
  execute_process(
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/install-vcpkg-macos.bash"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _vcpkg_result
  )
  if(NOT _vcpkg_result EQUAL 0)
    message(FATAL_ERROR "vcpkg macOS script failed")
  endif()
else()
  message(STATUS "Windows/Linux detected. Running 'vcpkg install'...")
  execute_process(
    COMMAND ${VCPKG_ROOT}/vcpkg install --triplet=${VCPKG_TARGET_TRIPLET}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _vcpkg_result
  )
  if(NOT _vcpkg_result EQUAL 0)
    message(FATAL_ERROR "vcpkg install failed")
  endif()
endif()

set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed")
list(PREPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED core imgproc imgcodecs)
find_package(semver CONFIG REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC obs_bridge_utils)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE NOMINMAX)

target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE OBS::libobs semver::semver cpr::cpr nlohmann_json::nlohmann_json opencv_core opencv_imgproc opencv_imgcodecs
)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/Core/MainPluginContext_c.cpp src/Core/MainPluginContext.cpp src/plugin-main.c
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(BUILD_TESTING)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()
