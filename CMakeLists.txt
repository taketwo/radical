cmake_minimum_required(VERSION 2.8.11.2)

set(LIB_NAME radical)
project("lib${LIB_NAME}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")
include("${CMAKE_SOURCE_DIR}/cmake/Utils.cmake")

add_definitions(-Wall -Wextra -std=c++11)

RADICAL_OPTION(BUILD_APPS                        "Build apps"                ON)
RADICAL_OPTION(BUILD_TESTS                       "Build tests"              OFF)

find_package(OpenCV REQUIRED)
include_directories(${CMAKE_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})

set(RADICAL_SRC
  radical/radiometric_response.cpp
  radical/mat_io.cpp
)

add_library(${LIB_NAME} SHARED ${RADICAL_SRC})
target_link_libraries(${LIB_NAME} ${OpenCV_LIBS})

if(BUILD_APPS)
  add_subdirectory(apps)
endif()

if(BUILD_TESTS)
  enable_testing(true)
  add_subdirectory(tests)
endif()
