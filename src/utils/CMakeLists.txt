set(UTILS_SRC mean_image.cpp)
add_library(utils ${UTILS_SRC})
target_link_libraries(utils opencv_core opencv_imgproc)
