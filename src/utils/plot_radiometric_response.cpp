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

#include <cassert>

#include <opencv2/imgproc/imgproc.hpp>

#include <radical/check.h>

#include "utils/colors.h"
#include "utils/plot_radiometric_response.h"

namespace utils {

using radical::Check;

void plotRadiometricResponse(const cv::Mat& response, cv::Mat& canvas, const cv::Scalar& color) {
  Check("Radiometric response", response).hasDepth(CV_32F).hasSize(256);
  Check("Canvas", canvas).notEmpty().hasType(CV_8UC3);

  auto x_scale = static_cast<float>(canvas.cols) / 256.0f;
  double min, max;
  cv::minMaxIdx(response, &min, &max);
  // auto y_scale = static_cast<float>(canvas.rows) / 1.2;
  auto y_scale = static_cast<float>(canvas.rows) / max;

  auto circle = [&canvas, x_scale, y_scale](float x, float y, const cv::Scalar& color) {
    cv::circle(canvas, cv::Point(x * x_scale, canvas.size().height - y * y_scale), std::ceil(x_scale), color, -1);
  };

  for (int i = 0; i < 256; ++i) {
    if (response.channels() == 1)
      circle(i, response.at<float>(i), color);
    else
      for (int c = 0; c < response.channels(); ++c)
        circle(i, response.at<cv::Vec3f>(i)[c], utils::colors::BGR[c]);
  }
}

cv::Mat plotRadiometricResponse(const cv::Mat& response, const cv::Size& size, const cv::Scalar& color) {
  assert(size.area() > 0);
  cv::Mat canvas(size, CV_8UC3);
  canvas.setTo(255);
  plotRadiometricResponse(response, canvas, color);
  return canvas;
}

cv::Mat plotRadiometricResponse(const radical::RadiometricResponse& rr, cv::Size size) {
  return plotRadiometricResponse(rr.getInverseResponse(), size);
}
}
