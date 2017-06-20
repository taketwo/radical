[![Build Status](https://travis-ci.org/taketwo/radical.svg?branch=master)](https://travis-ci.org/taketwo/radical)
[![License](https://img.shields.io/badge/license-MIT-green.svg?style=flat)](https://github.com/taketwo/radical/blob/master/LICENSE.md)

Consumer-grade color cameras suffer from significant optical nonlinearities,
often referred to as vignetting effects. For example, in Asus Xtion Pro Live
cameras the pixels in the corners are two times darker than those in the center
of the image. The vignetting effects in Intel RealSense cameras are less severe,
but are still noticeable, as can be seen below:

![Vignetting responses](doc/vignetting-responses.png "Vignetting responses")

Luckily, it is possible to calibrate the camera and remove the effects from the
images:

<p align="center"><img src ="doc/pioneer.gif"/></p>

This repository contains a collection of apps to calibrate radiometric and
vignetting responses of a camera and a runtime library that you can link into
your project to load calibration files and radiometrically rectify the images
delivered by the camera.

# Requirements

You need a compiler that supports C++11.

## Runtime library:

  * OpenCV (2 or 3)

## Calibration apps:

  * OpenCV (2 or 3)
  * Boost
  * OpenNI2 (optional, only if you want to calibrate an Asus Xtion camera)
  * librealsense (optional, only if you want to calibrate a RealSense camera)
  * Ceres (optional, needed for radiometric calibration with Debevec method and
    for vignetting calibration with polynomial vignetting model)

# Installation

1. Clone this repository.

2. Configure the project. By default, both runtime library and calibration apps
   will be built. If only the runtime library is needed, then configure as
   follows:

   ```bash
   cmake .. -DBUILD_APPS=OFF
   ```

   Note that if you want to enable support for Debevec camera response function
   calibration and fitting of polynomial vignetting model in the calibration
   app, you need to tell CMake where Ceres is installed:

   ```bash
   cmake .. -DCeres_DIR=<CERES_INSTALL_PATH>/share/Ceres
   ```

3. Install the project with `make install`.

# Usage

## Runtime library

In your *CMakeLists.txt* find the library as follows:

   ```cmake
   find_package(radical CONFIG REQUIRED)
   ```

And link it to your target:

   ```cmake
   target_link_libraries(your_target radical)
   ```

When configuring your project provide the path where *radical* was installed:

   ```bash
   cmake .. -Dradical_DIR=<RADICAL_INSTALL_PATH>/lib/cmake/radical
   ```

In you app include the headers:

   ```cpp
   #include <radical/radiometric_response.h>
   #include <radical/vignetting_response.h>
   ```

Load calibration files:

   ```cpp
   radical::RadiometricResponse rr("calibration-file-path.crf");
   radical::VignettingResponse vr("calibration-file-path.vgn");
   ```

Undo vignetting effects in frames coming from the camera:

   ```cpp
   cv::Mat frame;                 // color image from the camera
   cv::Mat irradiance, radiance;  // temporary storage
   cv::Mat frame_corrected;       // output image with vignette removed
   rr.inverseMap(frame, irradiance);
   vr.remove(irradiance, radiance);
   rr.directMap(radiance, frame_corrected);
   ```

## Calibration apps

### Radiometric response calibration

Supports OpenNI2 and RealSense cameras. Fix your camera in front of a static
scene and run the app. It will take multiple images of the scene at different
exposure times and compute the radiometric response of the camera. The response
will be stored in a calibration file named after the camera (model type + serial
number). Run with `--help` to see different options.

### Vignetting response calibration

Supports OpenNI2 and RealSense cameras. Get a sheet of white paper and fix it on
a table so it is uniformly lit by the light sources in your office. If your
table has a bright color, it is better to make a contour around the paper with
black tape to make sure that the blob detector is able to distinguish between
the white paper and the table.

Point the camera at the paper and start the app. Adjust the exposure time with
up and down arrows. We want the exposure time to be as large as possible,
however there should be no saturated pixels. For your convenience, overexposed
pixels will be highlighted with red color. After adjusting the exposure time so
that there are no red pixels on the calibration target, press Enter to start
data collection. Move the camera around so that the paper is observed through
every camera pixel. Take care to move the camera so that it does not cast
shadows onto the paper. In the bottom-left corner the amount of collected
samples per pixels is visualized. By default, we aim to collect 100 samples per
pixel. Once this amount is collected, the corresponding pixel will turn green.
After the required number of samples is collected for every pixel, the
vignetting response will be calibrated and stored in a file named after the
camera (model type + serial number). Run with `--help` to see different options.

# Citing

If you use this in the academic context, please cite the following paper:

> **Calibration and Correction of Vignetting Effects with an Application to 3D
Mapping**, *S. V. Alexandrov, J. Prankl, M. Zillich, M. Vincze*, IROS'16

# Licence

MIT
