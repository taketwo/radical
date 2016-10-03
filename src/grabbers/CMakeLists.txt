find_package(OpenNI2)
find_package(RealSense 1.9.7)

set(GRABBERS_SRC grabber.cpp)

if(OPENNI2_FOUND)
  list(APPEND GRABBERS_SRC openni2_grabber.cpp)
  list(APPEND GRABBERS_DEPS openni2)
endif()

if(REALSENSE_FOUND)
  list(APPEND GRABBERS_SRC realsense_grabber.cpp)
  list(APPEND GRABBERS_DEPS realsense)
endif()

add_library(grabbers ${GRABBERS_SRC})
target_link_libraries(grabbers ${OpenCV_LIBS} ${GRABBERS_DEPS})

foreach(grabber openni2 realsense)
  string(TOUPPER ${grabber} GRABBER)
  if(${GRABBER}_FOUND)
    target_compile_definitions(grabbers PRIVATE "-DHAVE_${GRABBER}")
  endif()
endforeach()
