if(OpenCV_VERSION VERSION_LESS 3.0.0)
  set(OpenCV_COMPONENTS highgui)
else()
  set(OpenCV_COMPONENTS imgcodecs highgui photo)
endif()

find_package(OpenCV COMPONENTS ${OpenCV_COMPONENTS} REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(grabbers)

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
    foreach(_lib ${_opencv_link})
      list(APPEND APP_ADD_LINK_WITH "opencv_${_lib}")
    endforeach()
    set(_executable ${_name})
    add_executable(${_executable} ${_executable}.cpp)
    target_link_libraries(${_executable}
      ${LIB_NAME}
      ${APP_ADD_LINK_WITH}
      ${Boost_PROGRAM_OPTIONS_LIBRARY}
    )
  endif()
endmacro(APP_ADD)

APP_ADD(display_radiometric_response
  OPENCV2 highgui
  OPENCV3 highgui imgcodecs
)

APP_ADD(display_vignetting_response
  OPENCV2 highgui
  OPENCV3 highgui imgcodecs
)

APP_ADD(calibrate_radiometric_response
  OPENCV2 highgui imgproc
  OPENCV3 highgui imgproc photo
  LINK_WITH grabbers
)
