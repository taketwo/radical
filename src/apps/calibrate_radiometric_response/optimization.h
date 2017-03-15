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

#include <array>

#include <opencv2/core/core.hpp>

#include "dataset.h"

class Optimization {
 public:
  Optimization(const Dataset* data, uint8_t min_valid_intensity, uint8_t max_valid_intensity);

  bool isValid(uint8_t intensity);

  void optimizeInverseResponse();

  void optimizeIrradiance();

  void rescale();

  double computeEnergy();

  bool converged() const;

  void converged(bool state);

  cv::Mat getOptimizedInverseResponse() const;

  cv::Mat getOptimizedIrradiance() const;

 private:
  const Dataset* dataset_;
  uint8_t min_valid_;
  uint8_t max_valid_;
  bool converged_;
  cv::Mat B_;  // irradiance
  cv::Mat U_;  // inverse response

  // Storage for temporary matrices to avoid re-allocation
  cv::Mat_<double> sum_t2_i_;
  std::array<double, 256> sum_omega_k_;
  std::array<int, 256> size_omega_k_;
};
