cmake_minimum_required(VERSION 2.8.12.2)

set(LIB_NAME radical)
project("${LIB_NAME}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Library version
set(RADICAL_VERSION_MAJOR 1)
set(RADICAL_VERSION_MINOR 0)
set(RADICAL_VERSION_PATCH 0)
set(RADICAL_VERSION "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}.${RADICAL_VERSION_PATCH}")
set(RADICAL_VERSION_SO "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}")
set(RADICAL_VERSION_LIB "${RADICAL_VERSION_MAJOR}.${RADICAL_VERSION_MINOR}.${RADICAL_VERSION_PATCH}")

# Set build type to Release if not specified by user
if(NOT CMAKE_BUILD_TYPE)
  message("-- No build type specified; defaulting to CMAKE_BUILD_TYPE=Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

add_definitions(-Wall -Wextra -std=c++11)

RADICAL_OPTION(BUILD_APPS            "Build apps"                                      ON)
RADICAL_OPTION(BUILD_TESTS           "Build tests"                                    OFF)
RADICAL_OPTION(WITH_OPENNI2          "Enable support for OpenNI2 cameras"              ON)
RADICAL_OPTION(WITH_REALSENSE        "Enable support for RealSense cameras"            ON)
RADICAL_OPTION(WITH_CERES            "Enable features requiring Google Ceres solver"   ON)

find_package(OpenCV COMPONENTS core imgproc REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")

add_subdirectory(src/utils)

set(RADICAL_SRC
  src/radical/radiometric_response.cpp
  src/radical/vignetting_response.cpp
  src/radical/vignetting_model.cpp
  src/radical/nonparametric_vignetting_model.cpp
  src/radical/polynomial_vignetting_model.cpp
)

add_library(${LIB_NAME} SHARED ${RADICAL_SRC})
set_target_properties(
  ${LIB_NAME} PROPERTIES
    VERSION "${RADICAL_VERSION_LIB}"
    SOVERSION "${RADICAL_VERSION_SO}"
)
target_compile_definitions(${LIB_NAME} PUBLIC "RADICAL_DEBUG=$<CONFIG:Debug>")
target_include_directories(
  ${LIB_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(${LIB_NAME} PUBLIC opencv_core opencv_imgproc)
target_link_libraries(${LIB_NAME} PRIVATE utils)

if(BUILD_APPS)
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
