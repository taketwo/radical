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

#include "calibration.h"

class EngelCalibration : public Calibration {
 public:
  void setConvergenceThreshold(double threshold);

  virtual std::string getMethodName() const override {
    return "Engel";
  }

 protected:
  virtual cv::Mat calibrateChannel(const Dataset& dataset) override;

 private:
  void optimizeInverseResponse();

  void optimizeIrradiance();

  double computeEnergy();

  void rescale();

  void visualizeProgress();

  const Dataset* dataset_ = nullptr;

  bool converged_;
  cv::Mat B_;  // irradiance
  cv::Mat U_;  // inverse response

  double energy_, delta_;
  double convergence_threshold_ = 1e-5;
  double scale_;

  // Storage for temporary matrices to avoid re-allocation
  cv::Mat_<double> sum_t2_i_;
  std::array<double, 256> sum_omega_k_;
  std::array<int, 256> size_omega_k_;
};
