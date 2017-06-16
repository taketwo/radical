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

#include <iostream>

#include <boost/format.hpp>

#include "calibration.h"

using boost::format;
using boost::str;

static const char* NAMES[] = {"Blue", "Green", "Red"};

cv::Mat Calibration::calibrate(const Dataset& data) {
  if (verbosity_)
    std::cout << "Starting " << getMethodName() << " calibration procedure" << std::endl;

  auto datasets = data.splitChannels();

  std::vector<cv::Mat> response_channels;
  for (channel_ = 0; channel_ < static_cast<int>(datasets.size()); ++channel_)
    response_channels.push_back(calibrateChannel(datasets[channel_]));

  // Post-process the response:
  //  * rescale such that maximum value is 1
  //  * map intensity below min_valid to 0
  //  * map intensity above max_valid to 1
  //  * sort to ensure invertability
  for (size_t i = 0; i < response_channels.size(); ++i) {
    double min, max;
    cv::minMaxIdx(response_channels[i], &min, &max);
    cv::divide(response_channels[i], max, response_channels[i]);
    for (int j = 0; j < min_valid_; ++j)
      response_channels[i].at<float>(j) = 0;
    for (int j = max_valid_ + 1; j < 256; ++j)
      response_channels[i].at<float>(j) = 1;
    cv::sort(response_channels[i], response_channels[i], cv::SORT_EVERY_ROW | cv::SORT_ASCENDING);
  }

  cv::Mat response;
  cv::merge(response_channels, response);
  return response;
}

void Calibration::setMaxNumIterations(unsigned int max_num_iterations) {
  max_num_iterations_ = max_num_iterations;
}

void Calibration::setVerbosity(unsigned int level) {
  verbosity_ = level;
}

void Calibration::setValidPixelRange(unsigned char min_valid, unsigned char max_valid) {
  min_valid_ = min_valid;
  max_valid_ = max_valid;
}

void Calibration::setVisualizeProgress(const std::function<void(const cv::Mat&)>& imshow) {
  imshow_ = imshow;
}

bool Calibration::isPixelValid(unsigned char pixel) const {
  return pixel >= min_valid_ && pixel <= max_valid_;
}

bool Calibration::isPixelValid(const cv::Vec3b& pixel) const {
  return isPixelValid(pixel[0]) && isPixelValid(pixel[1]) && isPixelValid(pixel[2]);
}

void Calibration::printHeader() const {
  if (verbosity_)
    std::cout << str(format("| %=7s | %=5s | %=14s | %=14s |\n") % "Channel" % "Iter" % "Residual" % "Delta");
}

void Calibration::printFooter() const {
  if (verbosity_)
    std::cout << boost::format("%53T-") << std::endl;
}

void Calibration::printIteration(unsigned int iteration, double residual, double delta, const char extra) {
  if (verbosity_) {
    if (iteration == 1)
      std::cout << str(format("| %=7s | %=4d%c | %14.6f | %=14s |\n") % NAMES[channel_] % iteration % extra % residual %
                       "");
    else
      std::cout << str(format("| %=7s | %=4d%c | %14.6f | %14.6f |\n") % "" % iteration % extra % residual % delta);
  }
}
