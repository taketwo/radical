/******************************************************************************
 * Copyright (c) 2016-2017 Sergey Alexandrov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include <boost/assert.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include "utils/check.h"
#include "utils/colors.h"
#include "utils/plot_radiometric_response.h"

namespace utils {

void plotRadiometricResponse(const cv::Mat& response, cv::Mat& canvas) {
  Check("Radiometric response", response).hasType(CV_32FC3).hasSize(256);
  Check("Canvas", canvas).notEmpty().hasType(CV_8UC3);

  double min;
  double max;
  cv::minMaxLoc(response.reshape(1), &min, &max);

  auto size = canvas.size();
  float x_scale = static_cast<float>(size.width) / 256;
  float y_scale = static_cast<float>(size.height) / (max - min);

  for (size_t i = 0; i < 256; ++i) {
    auto pt = response.at<cv::Vec3f>(i);
    for (int c = 0; c < 3; ++c)
      cv::circle(canvas, cv::Point(i * x_scale, size.height - pt[c] * y_scale), std::ceil(x_scale),
                 utils::colors::BGR[c], -1);
  }
}

cv::Mat plotRadiometricResponse(const cv::Mat& response, cv::Size size) {
  BOOST_ASSERT(size.area() > 0);
  cv::Mat canvas(size, CV_8UC3);
  canvas.setTo(255);
  plotRadiometricResponse(response, canvas);
  return canvas;
}

cv::Mat plotRadiometricResponse(const radical::RadiometricResponse& rr, cv::Size size) {
  return plotRadiometricResponse(rr.getInverseResponse(), size);
}
}
