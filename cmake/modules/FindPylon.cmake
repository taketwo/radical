###############################################################################
# Find Pylon
#
#  find_package(Pylon)
#
# Variables defined by this module:
#
#  PYLON_FOUND
#  PYLON_VERSION
#  PYLON_INCLUDE_DIR
#  PYLON_LIBRARIES

find_path(_CONFIG_DIR NAMES pylon-config
          HINTS $ENV{PYLON_ROOT}/bin
                /opt/pylon5/bin
)

if(_CONFIG_DIR)
  set(_CONFIG "${_CONFIG_DIR}/pylon-config")

  set(PYLON_FOUND TRUE)

  execute_process(COMMAND ${_CONFIG} --version OUTPUT_VARIABLE PYLON_VERSION)
  string(REPLACE "\n" "" PYLON_VERSION "${PYLON_VERSION}")
  execute_process(COMMAND ${_CONFIG} --version-major OUTPUT_VARIABLE PYLON_VERSION_MAJOR)
  string(REPLACE "\n" "" PYLON_VERSION_MAJOR "${PYLON_VERSION_MAJOR}")
  execute_process(COMMAND ${_CONFIG} --version-minor OUTPUT_VARIABLE PYLON_VERSION_MINOR)
  string(REPLACE "\n" "" PYLON_VERSION_MINOR "${PYLON_VERSION_MINOR}")
  execute_process(COMMAND ${_CONFIG} --version-subminor OUTPUT_VARIABLE PYLON_VERSION_PATCH)
  string(REPLACE "\n" "" PYLON_VERSION_PATCH "${PYLON_VERSION_PATCH}")
  execute_process(COMMAND ${_CONFIG} --version-build OUTPUT_VARIABLE PYLON_VERSION_TWEAK)
  string(REPLACE "\n" "" PYLON_VERSION_TWEAK "${PYLON_VERSION_TWEAK}")

  execute_process(COMMAND ${_CONFIG} --cflags-only-I OUTPUT_VARIABLE PYLON_INCLUDE_DIR)
  string(REPLACE " " ";" PYLON_INCLUDE_DIR "${PYLON_INCLUDE_DIR}")
  string(REPLACE "-I" "" PYLON_INCLUDE_DIR "${PYLON_INCLUDE_DIR}")
  string(REPLACE "\n" "" PYLON_INCLUDE_DIR "${PYLON_INCLUDE_DIR}")

  execute_process(COMMAND ${_CONFIG} --libs-only-l OUTPUT_VARIABLE _LIBS)
  string(REPLACE " " ";" _LIBS "${_LIBS}")
  string(REPLACE "-l" "" _LIBS "${_LIBS}")
  string(REPLACE "\n" "" _LIBS "${_LIBS}")
  execute_process(COMMAND ${_CONFIG} --libdir OUTPUT_VARIABLE _LIBDIR)
  string(REPLACE "\n" ""  _LIBDIR "${_LIBDIR}")
  unset(PYLON_LIBRARIES CACHE)
  foreach (_LIB ${_LIBS})
    find_library(_LIBRARY NAMES ${_LIB} HINTS ${_LIBDIR})
    if(_LIBRARY)
      list(APPEND PYLON_LIBRARIES ${_LIB})
      add_library(${_LIB} SHARED IMPORTED)
      set_property(TARGET ${_LIB} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PYLON_INCLUDE_DIR})
      set_property(TARGET ${_LIB} PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${PYLON_INCLUDE_DIR})
      set_property(TARGET ${_LIB} PROPERTY IMPORTED_LOCATION ${_LIBRARY})
    endif()
    unset(_LIBRARY CACHE)
  endforeach()

  set(HAVE_PYLON TRUE)

  unset(_LIBS CACHE)
  unset(_LIBDIR CACHE)
  unset(_CONFIG CACHE)
else()
  set(PYLON_FOUND FALSE)
endif()
unset(_CONFIG_DIR CACHE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pylon
  FOUND_VAR PYLON_FOUND
  REQUIRED_VARS PYLON_LIBRARIES PYLON_INCLUDE_DIR
  VERSION_VAR PYLON_VERSION
)
