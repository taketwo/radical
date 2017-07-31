###############################################################################
# Find OpenNI2
#
#     find_package(OpenNI2)
#
# Variables defined by this module:
#
#  OPENNI2_FOUND                 True if OpenNI2 was found
#  OPENNI2_VERSION               The version of found OpenNI2 library
#  OPENNI2_INCLUDE_DIR           The location of OpenNI2 headers
#  OPENNI2_LIBRARY               Libraries needed to use OpenNI2
#
# Imported targets defined by this module:
#
#  openni2                       Library to link against
#
# Based on the CMake script from OpenCV

if(NOT "${OPENNI2_LIB_DIR}" STREQUAL "${OPENNI2_LIB_DIR_INTERNAL}")
  unset(OPENNI2_LIBRARY CACHE)
  unset(OPENNI2_LIB_DIR CACHE)
endif()

if(NOT "${OPENNI2_INCLUDE_DIR}" STREQUAL "${OPENNI2_INCLUDE_DIR_INTERNAL}")
  unset(OPENNI2_INCLUDES CACHE)
  unset(OPENNI2_INCLUDE_DIR CACHE)
endif()

if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    find_file(OPENNI2_INCLUDES "OpenNI.h" PATHS "$ENV{OPEN_NI_INSTALL_PATH}Include" DOC "OpenNI2 c++ interface header")
    find_library(OPENNI2_LIBRARY "OpenNI2" PATHS $ENV{OPENNI2_LIB} DOC "OpenNI2 library")
  else()
    find_file(OPENNI2_INCLUDES "OpenNI.h" PATHS $ENV{OPENNI2_INCLUDE64} "$ENV{OPEN_NI_INSTALL_PATH64}Include" DOC "OpenNI2 c++ interface header")
    find_library(OPENNI2_LIBRARY "OpenNI2" PATHS $ENV{OPENNI2_LIB64} DOC "OpenNI2 library")
  endif()
  set(LIBRARY_TYPE STATIC)
elseif(UNIX OR APPLE)
  find_file(OPENNI2_INCLUDES "OpenNI.h" PATHS "/usr/include/ni2" "/usr/include/openni2" $ENV{OPENNI2_INCLUDE} DOC "OpenNI2 c++ interface header")
  find_library(OPENNI2_LIBRARY "OpenNI2" PATHS "/usr/lib" $ENV{OPENNI2_REDIST} DOC "OpenNI2 library")
  set(LIBRARY_TYPE SHARED)
endif()

if(OPENNI2_LIBRARY AND OPENNI2_INCLUDES)
  set(HAVE_OPENNI2 TRUE)
endif()

get_filename_component(OPENNI2_LIB_DIR "${OPENNI2_LIBRARY}" PATH)
get_filename_component(OPENNI2_INCLUDE_DIR ${OPENNI2_INCLUDES} PATH)

if(HAVE_OPENNI2)
  set(OPENNI2_LIB_DIR "${OPENNI2_LIB_DIR}" CACHE PATH "Path to OpenNI2 libraries" FORCE)
  set(OPENNI2_INCLUDE_DIR "${OPENNI2_INCLUDE_DIR}" CACHE PATH "Path to OpenNI2 headers" FORCE)
endif()

if(OPENNI2_LIBRARY)
  set(OPENNI2_LIB_DIR_INTERNAL "${OPENNI2_LIB_DIR}" CACHE INTERNAL "This is the value of the last time OPENNI_LIB_DIR was set successfully." FORCE)
else()
  message(WARNING, "OpenNI2 library directory (set by OPENNI2_LIB_DIR variable) is not found or does not have OpenNI2 libraries.")
endif()

if(OPENNI2_INCLUDES)
  set(OPENNI2_INCLUDE_DIR_INTERNAL "${OPENNI2_INCLUDE_DIR}" CACHE INTERNAL "This is the value of the last time OPENNI2_INCLUDE_DIR was set successfully." FORCE)
else()
  message(WARNING, "OpenNI2 include directory (set by OPENNI2_INCLUDE_DIR variable) is not found or does not have OpenNI2 include files.")
endif()

mark_as_advanced(FORCE OPENNI2_LIBRARY)
mark_as_advanced(FORCE OPENNI2_INCLUDES)

if(HAVE_OPENNI2)

  ## Imported target
  add_library(openni2 ${LIBRARY_TYPE} IMPORTED)
  set_property(TARGET openni2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENNI2_INCLUDE_DIR})
  set_property(TARGET openni2 PROPERTY IMPORTED_LOCATION ${OPENNI2_LIBRARY})

  ## Version
  parse_header("${OPENNI2_INCLUDE_DIR}/OniVersion.h" ONI_VERSION_LINE ONI_VERSION_MAJOR ONI_VERSION_MINOR ONI_VERSION_MAINTENANCE ONI_VERSION_BUILD)
  if(ONI_VERSION_MAJOR)
    set(OPENNI2_VERSION ${ONI_VERSION_MAJOR}.${ONI_VERSION_MINOR}.${ONI_VERSION_MAINTENANCE} CACHE INTERNAL "OpenNI2 version")
    set(OPENNI2_VERSION_BUILD ${ONI_VERSION_BUILD} CACHE INTERNAL "OpenNI2 build version")
  endif()

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenNI2
  FOUND_VAR OPENNI2_FOUND
  REQUIRED_VARS OPENNI2_LIBRARY OPENNI2_INCLUDE_DIR
  VERSION_VAR OPENNI2_VERSION
)

