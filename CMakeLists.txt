cmake_minimum_required(VERSION 2.8.12.2)

set(LIB_NAME radical)
project("lib${LIB_NAME}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

# Extract version from header
execute_process(
  COMMAND grep "RADICAL_VERSION " radical/version.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE GREP_VERSION_RESULT
  OUTPUT_VARIABLE PROJECT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT GREP_VERSION_RESULT EQUAL 0)
  message(SEND_ERROR "Cannot extract version number: ${GREP_VERSION_RESULT}")
endif()
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PROJECT_VERSION "${PROJECT_VERSION}" )

add_definitions(-Wall -Wextra -std=c++11)

RADICAL_OPTION(BUILD_APPS                        "Build apps"                ON)
RADICAL_OPTION(BUILD_TESTS                       "Build tests"              OFF)

find_package(OpenCV COMPONENTS core imgproc REQUIRED)

set(RADICAL_SRC
  radical/radiometric_response.cpp
  radical/mat_io.cpp
)

add_library(${LIB_NAME} SHARED ${RADICAL_SRC})
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib)
set_target_properties(${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION 1)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/radical
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include
  FILES_MATCHING PATTERN "*.h"
)
target_include_directories(
  ${LIB_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_link_libraries(${LIB_NAME} PUBLIC opencv_core opencv_imgproc)
if(BUILD_APPS)
  add_subdirectory(apps)
endif()

if(BUILD_TESTS)
  enable_testing(true)
  add_subdirectory(tests)
endif()

### Uninstall target
set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
  "cmake/Uninstall.cmake.in"
  "${_generated_dir}/uninstall.cmake"
  IMMEDIATE @ONLY
)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${_generated_dir}/uninstall.cmake
)
