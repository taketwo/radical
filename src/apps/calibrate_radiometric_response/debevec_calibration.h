/******************************************************************************
 * Copyright (c) 2017 Sergey Alexandrov
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

#include "calibration.h"

struct CeresIterationCallback;

class DebevecCalibration : public Calibration {
 public:
  /** Set the minimum number of samples per each intensity level used for optimization.
    * A subset of all image pixels is used in the optimization. The sampling procedure ensures that at least this many
    * observations per each intensity level (from min to max valid) is taken. */
  void setMinSamplesPerIntensityLevel(unsigned int min_samples);

  void setSmoothingLambda(double lambda);

  virtual std::string getMethodName() const override {
    return "Debevec";
  }

 protected:
  virtual cv::Mat calibrateChannel(const Dataset& data) override;

 private:
  void selectPixels();

  void visualizeProgress();

  unsigned int min_samples_ = 5;
  double lambda_ = 20;

  const Dataset* dataset_ = nullptr;
  std::vector<int> locations_;

  // Irradiances to optimize
  std::vector<double> X_;
  // Model parameters to optimize
  cv::Mat_<double> U_;

  friend struct CeresIterationCallback;
};
