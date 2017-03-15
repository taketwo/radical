set(UTILS_SRC mat_io.cpp check.cpp mean_image.cpp)
add_library(utils ${UTILS_SRC})
target_link_libraries(utils opencv_core opencv_imgproc)
set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
