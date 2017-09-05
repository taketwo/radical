set(UTILS_SRC mean_image.cpp key_code.cpp colors.cpp plot_radiometric_response.cpp)
add_library(utils ${UTILS_SRC})
target_link_libraries(utils ${LIB_NAME} opencv_core opencv_imgproc)
set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
