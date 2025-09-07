cmake_minimum_required(VERSION 3.10)

project(obs_bridge_utils VERSION 1.0.0)

add_library(obs_bridge_utils INTERFACE)

target_include_directories(obs_bridge_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)
