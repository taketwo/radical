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

#include <functional>
#include <memory>

#include "dataset.h"

class Calibration {
 public:
  using Ptr = std::shared_ptr<Calibration>;

  virtual ~Calibration() {}

  cv::Mat calibrate(const Dataset& dataset);

  void setMaxNumIterations(unsigned int max_num_iterations);

  void setVerbosity(unsigned int level);

  void setValidPixelRange(unsigned char min_valid, unsigned char max_valid);

  /** Visualize calibration progress.
    *
    * \param[i] imshow a function that can display a given cv::Mat */
  void setVisualizeProgress(const std::function<void(const cv::Mat&)>& imshow);

  virtual std::string getMethodName() const = 0;

 protected:
  /** Calibrate a single color channel.
    * This should be implemented by deriving classes. The dataset is guaranteed to have a single color channel. */
  virtual cv::Mat calibrateChannel(const Dataset& dataset) = 0;

  bool isPixelValid(unsigned char pixel) const;

  bool isPixelValid(const cv::Vec3b& pixel) const;

  void printHeader() const;

  void printFooter() const;

  void printIteration(unsigned int iteration, double residual, double delta, const char extra = ' ');

  unsigned int max_num_iterations_ = 30;
  unsigned int verbosity_ = 0;
  unsigned char min_valid_ = 1;
  unsigned char max_valid_ = 254;
  std::function<void(const cv::Mat&)> imshow_;

  int channel_;
};
