find_package(Boost COMPONENTS program_options REQUIRED)
find_package(OpenCV COMPONENTS highgui photo REQUIRED)

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(grabbers)

macro(APP_ADD _name)
  set(_executable ${_name})
  add_executable(${_executable} ${_executable}.cpp)
  target_link_libraries(${_executable}
    ${LIB_NAME}
    grabbers
    opencv_highgui
    opencv_photo
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )
endmacro(APP_ADD)

if(NOT OpenCV_VERSION VERSION_LESS 3.0.0)
  APP_ADD(calibrate_radiometric_response)
endif()
