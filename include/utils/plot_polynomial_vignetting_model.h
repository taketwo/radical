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

#pragma once

#include <cassert>

#include <opencv2/core/core.hpp>

#include <radical/polynomial_vignetting_model.h>

#include "utils/colors.h"

template <unsigned int Degree>
inline cv::Mat plotPolynomialVignettingModel(const radical::PolynomialVignettingModel<Degree>& pvm,
                                             cv::Size plot_size) {
  assert(plot_size.area() > 0);

  cv::Mat plot(plot_size, CV_8UC3);
  plot.setTo(255);

  auto image_size = pvm.getImageSize();
  const auto scale_x = 1.0 * plot_size.width / image_size.width;
  const auto scale_y = 1.0 * plot_size.height / image_size.height;

  cv::line(plot, cv::Point(plot_size.width / 2, 0), cv::Point(plot_size.width / 2, plot_size.height - 1),
           cv::Scalar(100, 100, 100), 2);
  cv::line(plot, cv::Point(0, plot_size.height / 2), cv::Point(plot_size.width - 1, plot_size.height / 2),
           cv::Scalar(100, 100, 100), 2);

  auto max_radius = 0.5 * std::sqrt(image_size.width * image_size.width + image_size.height * image_size.height);
  auto radius_step = max_radius / plot_size.width;
  auto radius_scale = 1.0 * plot_size.width / max_radius;
  auto coeff = pvm.getModelCoefficients().template ptr<cv::Vec3d>();

  for (size_t i = 0; i < 3; ++i) {
    auto cx = coeff[0][i];
    auto cy = coeff[1][i];
    cv::circle(plot, cv::Point(cx * scale_x, cy * scale_y), 7, utils::colors::BGR[i], -1);
    auto radius = 0.0;
    while (radius < max_radius) {
      auto v = pvm(cx + radius, cy)[i];
      cv::circle(plot, cv::Point(radius * radius_scale, (1.0 - v) * plot_size.height), 2, utils::colors::BGR[i], -1);
      radius += radius_step;
    }
  }

  return plot;
}

template <unsigned int Degree>
inline cv::Mat plotPolynomialVignettingModel(const radical::PolynomialVignettingModel<Degree>& pvm) {
  return plotPolynomialVignettingModel<Degree>(pvm, pvm.getImageSize());
}
