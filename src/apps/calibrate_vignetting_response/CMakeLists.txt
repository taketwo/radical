find_package(Ceres CONFIG QUIET)

if(Ceres_FOUND)
  set_target_properties(ceres
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CERES_INCLUDE_DIRS}"
      INTERFACE_COMPILE_DEFINITIONS "HAVE_CERES")
  set(_ceres ceres)
else()
  message(STATUS "Ceres not found, vignetting calibration app will be compiled without polynomial model support")
endif()

APP_ADD(calibrate_vignetting_response
  OPENCV2 highgui imgproc
  OPENCV3 highgui imgproc
  LINK_WITH grabbers utils ${_ceres}
)

