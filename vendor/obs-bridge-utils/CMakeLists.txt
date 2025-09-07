cmake_minimum_required(VERSION 3.10)

	project(obs - bridge - utils VERSION 1.0.0)

		add_library(obs - bridge - utils INTERFACE)

#Include directories
			target_include_directories(obs - bridge - utils INTERFACE $ <
						   BUILD_INTERFACE : ${CMAKE_CURRENT_SOURCE_DIR} > $ <
						   INSTALL_INTERFACE :.>)
