###############################################################################
# Find librealsense
#
#     find_package(RealSense)
#
# Variables defined by this module:
#
#  REALSENSE_FOUND                 True if librealsense was found
#  REALSENSE_API_VERSION           The API version
#  REALSENSE_INCLUDE_DIR           The location of librealsense headers
#  REALSENSE_LIBRARY               Libraries needed to use librealsense
#
# Imported targets defined by this module:
#
#  realsense                       Library to link against

find_path(_REALSENSE_INCLUDE_DIR NAMES rs.h
          HINTS /usr/local/include/librealsense
                /usr/include/librealsense
                /usr/local/include
                /usr/local
          DOC "librealsense include directory"
)

if(_REALSENSE_INCLUDE_DIR)

  # Include directory
  get_filename_component(REALSENSE_INCLUDE_DIR ${_REALSENSE_INCLUDE_DIR} DIRECTORY)
  mark_as_advanced(REALSENSE_INCLUDE_DIR)

  ## Library
  find_library(REALSENSE_LIBRARY NAMES realsense)
  if(REALSENSE_LIBRARY)
    add_library(realsense SHARED IMPORTED)
    set_property(TARGET realsense PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${REALSENSE_INCLUDE_DIR})
    set_property(TARGET realsense PROPERTY IMPORTED_LOCATION ${REALSENSE_LIBRARY})
    set(HAVE_REALSENSE TRUE)
  endif()

  # Version
  set(REALSENSE_VERSION 0)
  file(READ "${_REALSENSE_INCLUDE_DIR}/rs.h" _rs_H_CONTENTS)
  string(REGEX MATCH "define[ \t]+RS_API_MAJOR_VERSION[ \t]+([0-9]+)" _major_version_match "${_rs_H_CONTENTS}")
  set(REALSENSE_API_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+RS_API_MINOR_VERSION[ \t]+([0-9]+)" _minor_version_match "${_rs_H_CONTENTS}")
  set(REALSENSE_API_MINOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+RS_API_PATCH_VERSION[ \t]+([0-9]+)" _patch_version_match "${_rs_H_CONTENTS}")
  set(REALSENSE_API_PATCH_VERSION "${CMAKE_MATCH_1}")
  set(REALSENSE_API_VERSION "${REALSENSE_API_MAJOR_VERSION}.${REALSENSE_API_MINOR_VERSION}.${REALSENSE_API_PATCH_VERSION}")
  unset(_rs_H_CONTENTS)

  unset(_REALSENSE_INCLUDE_DIR CACHE)

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RealSense
  FOUND_VAR REALSENSE_FOUND
  REQUIRED_VARS REALSENSE_LIBRARY REALSENSE_INCLUDE_DIR
  VERSION_VAR REALSENSE_API_VERSION
)

