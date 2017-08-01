if(OpenCV_VERSION VERSION_LESS 3.0.0)
  set(OpenCV_COMPONENTS highgui)
else()
  set(OpenCV_COMPONENTS imgcodecs highgui)
endif()

find_package(OpenCV COMPONENTS ${OpenCV_COMPONENTS} REQUIRED)

if(WITH_CERES)
  find_package(Ceres CONFIG)
  if(Ceres_FOUND)
    set_target_properties(ceres
      PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CERES_INCLUDE_DIRS}"
        INTERFACE_COMPILE_DEFINITIONS "HAVE_CERES")
  endif()
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR})

macro(APP_ADD _name)
  set(options)
  set(one_value_args)
  set(multi_value_args OPENCV2 OPENCV3 LINK_WITH)
  cmake_parse_arguments(APP_ADD "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(_enabled 0)
  if(NOT APP_ADD_OPENCV2 AND NOT APP_ADD_OPENCV3)
    set(_enabled 1)
  endif()
  if(OpenCV_VERSION VERSION_LESS 3.0.0 AND APP_ADD_OPENCV2)
    set(_enabled 1)
    set(_opencv_link ${APP_ADD_OPENCV2})
  elseif(NOT OpenCV_VERSION VERSION_LESS 3.0.0 AND APP_ADD_OPENCV3)
    set(_enabled 1)
    set(_opencv_link ${APP_ADD_OPENCV3})
  endif()
  if(_enabled)
    set(_executable ${_name})
    foreach(_lib ${APP_ADD_LINK_WITH})
      if(TARGET ${_lib})
        list(APPEND _link_with ${_lib})
      endif()
    endforeach()
    foreach(_lib ${_opencv_link})
      list(APPEND _link_with "opencv_${_lib}")
    endforeach()
    file(GLOB _sources *.cpp)
    add_executable(${_executable} ${_sources})
    target_include_directories(${_executable} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${_executable}
      ${LIB_NAME}
      ${_link_with}
      ${Boost_LIBRARIES}
    )
  endif()
endmacro(APP_ADD)

add_subdirectory(calibrate_radiometric_response)
add_subdirectory(calibrate_vignetting_response)
add_subdirectory(display_radiometric_response)
add_subdirectory(display_vignetting_response)
add_subdirectory(remove_vignetting)
