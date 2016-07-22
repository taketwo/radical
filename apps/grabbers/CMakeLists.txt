find_package(OpenNI2)

set(GRABBERS_SRC grabber.cpp)

if(OPENNI2_FOUND)
  list(APPEND GRABBERS_SRC openni2_grabber.cpp)
  list(APPEND GRABBERS_DEPS openni2)
endif()

add_library(grabbers ${GRABBERS_SRC})
target_link_libraries(grabbers ${OpenCV_LIBS} ${GRABBERS_DEPS})
