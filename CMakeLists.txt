cmake_minimum_required(VERSION 2.8.12.2)

if(POLICY CMP0054)
  # Only interpret if() arguments as variables or keywords when unquoted.
  cmake_policy(SET CMP0054 NEW)
endif()

set(LIB_NAME radical)
project("${LIB_NAME}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# On Windows we build a static library with statically linked dependencies and
# on Linux we build a shared library.
if(WIN32)
  set(LIB_TYPE STATIC)
else()
  set(LIB_TYPE SHARED)
endif()

# Library version
set(RADICAL_VERSION_MAJOR 1)
set(RADICAL_VERSION_MINOR 0)
set(RADICAL_VERSION_PATCH 0)
set(RADICAL_VERSION "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}.${RADICAL_VERSION_PATCH}")
set(RADICAL_VERSION_SO "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}")
set(RADICAL_VERSION_LIB "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}.${RADICAL_VERSION_PATCH}")

# Set build type to Release if not specified by user
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "-- No build type specified; defaulting to CMAKE_BUILD_TYPE=Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8.5)
    # Color diagnostics output was added in GCC 4.9.0 (and GCC 4.8.5 was the last release in the 4.8 series).
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
  endif()
endif()

RADICAL_OPTION(BUILD_APPS            "Build apps"                                      ON)
RADICAL_OPTION(BUILD_TESTS           "Build tests"                                    OFF)
RADICAL_OPTION(WITH_OPENNI2          "Enable support for OpenNI2 cameras"              ON)
RADICAL_OPTION(WITH_REALSENSE        "Enable support for RealSense cameras"            ON)
RADICAL_OPTION(WITH_PYLON            "Enable support for Pylon cameras"               OFF)
RADICAL_OPTION(WITH_CERES            "Enable features requiring Google Ceres solver"   ON)

find_package(OpenCV COMPONENTS core imgproc REQUIRED)

if(BUILD_APPS OR BUILD_TESTS)
  set(Boost_COMPONENTS system filesystem)
  if(BUILD_APPS)
    list(APPEND Boost_COMPONENTS program_options)
  endif()
  if(BUILD_TESTS)
    list(APPEND Boost_COMPONENTS unit_test_framework)
  endif()
  if(LIB_TYPE STREQUAL STATIC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(BOOST_ALL_DYN_LINK OFF)
  endif()
  find_package(Boost COMPONENTS ${Boost_COMPONENTS} REQUIRED)
endif()

include_directories("${PROJECT_SOURCE_DIR}/include")

set(RADICAL_SRC
  src/radical/radiometric_response.cpp
  src/radical/vignetting_response.cpp
  src/radical/vignetting_model.cpp
  src/radical/nonparametric_vignetting_model.cpp
  src/radical/polynomial_vignetting_model.cpp
  src/radical/mat_io.cpp
  src/radical/check.cpp
)

add_library(${LIB_NAME} ${LIB_TYPE} ${RADICAL_SRC})
set_target_properties(
  ${LIB_NAME} PROPERTIES
    VERSION "${RADICAL_VERSION_LIB}"
    SOVERSION "${RADICAL_VERSION_SO}"
)
target_compile_definitions(${LIB_NAME} PUBLIC "RADICAL_DEBUG=$<CONFIG:Debug>")
target_include_directories(${LIB_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    ${OpenCV_INCLUDE_DIRS} # Should be picked up automatically from linking, but
                           # does not work reliably on Windows
)

target_link_libraries(${LIB_NAME} PUBLIC opencv_core opencv_imgproc)

if(BUILD_APPS)
  add_subdirectory(src/utils)
  add_subdirectory(src/grabbers)
  add_subdirectory(src/apps)
endif()

if(BUILD_TESTS)
  enable_testing(true)
  add_subdirectory(tests)
endif()

#---------------------------------------------------------------------#
#                            Installation                             #
#---------------------------------------------------------------------#

## Layout:
#
#   <prefix>
#   ├── include
#   │   └── radical
#   └── lib
#       └── cmake
#           └── radical

set(_config_install_dir "lib/cmake/${PROJECT_NAME}")
set(_include_install_dir "include")
set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(_version_config_file "${_generated_dir}/${PROJECT_NAME}-config-version.cmake")
set(_project_config_file "${_generated_dir}/${PROJECT_NAME}-config.cmake")
set(_targets_export_name "${PROJECT_NAME}-targets")

## Create radical-config-version.cmake

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${_version_config_file}"
  VERSION ${RADICAL_VERSION}
  COMPATIBILITY AnyNewerVersion
)

## Create radical-config.cmake

configure_package_config_file(
  "cmake/Config.cmake.in"
  "${_project_config_file}"
  INSTALL_DESTINATION "${_config_install_dir}"
)

## Install targets

install(TARGETS ${LIB_NAME}
  EXPORT "${_targets_export_name}"
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION ${_include_install_dir}
)

## Install headers

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
  DESTINATION ${_include_install_dir}
  FILES_MATCHING PATTERN "*.h"
)

## Instal config files

install(
  FILES ${_project_config_file} ${_version_config_file}
  DESTINATION "${_config_install_dir}"
)

install(
  EXPORT "${_targets_export_name}"
  DESTINATION "${_config_install_dir}"
)

#---------------------------------------------------------------------#
#                           Uninstallation                            #
#---------------------------------------------------------------------#

## Uninstall target

configure_file(
  "cmake/Uninstall.cmake.in"
  "${_generated_dir}/uninstall.cmake"
  IMMEDIATE @ONLY
)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${_generated_dir}/uninstall.cmake
)
