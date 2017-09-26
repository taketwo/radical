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

#include "engel_calibration.h"

#include "utils/colors.h"
#include "utils/plot_radiometric_response.h"

cv::Mat EngelCalibration::calibrateChannel(const Dataset& dataset) {
  dataset_ = &dataset;
  converged_ = false;
  energy_ = 0;
  delta_ = 0;
  scale_ = 1.0;

  U_.create(1, 256, CV_64FC1);
  for (int i = 0; i < 256; ++i)
    U_.at<double>(i) = (1.0 / 255.0) * i;

  B_.create(dataset_->getImageSize(), CV_64FC1);

  printHeader();

  unsigned int iteration = 0;
  while (iteration < max_num_iterations_) {
    optimizeIrradiance();
    auto e = computeEnergy();
    if (iteration > 0) {
      delta_ = energy_ - e;
      if (delta_ < convergence_threshold_)
        converged_ = true;
    }
    energy_ = e;
    printIteration(++iteration, energy_, delta_, 'B');

    optimizeInverseResponse();
    e = computeEnergy();
    delta_ = energy_ - e;
    if (energy_ > 0 && delta_ < convergence_threshold_)
      converged_ = true;
    energy_ = e;
    printIteration(++iteration, energy_, delta_, 'U');

    rescale();
    visualizeProgress();

    if (converged_)
      break;
  }

  printFooter();
  visualizeProgress();

  cv::Mat response;
  U_.convertTo(response, CV_32F);
  return response;
}

void EngelCalibration::setConvergenceThreshold(double threshold) {
  convergence_threshold_ = threshold;
}

void EngelCalibration::optimizeInverseResponse() {
  // Eqn. 7
  sum_omega_k_.fill(0);
  size_omega_k_.fill(0);

  for (const auto& t : dataset_->getExposureTimes())
    for (const auto& image : dataset_->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j) {
          const auto& p = image.at<uint8_t>(i, j);
          sum_omega_k_[p] += t * B_.at<double>(i, j);
          size_omega_k_[p] += 1;
        }

  U_.setTo(0);
  for (int k = min_valid_; k <= max_valid_; ++k)
    U_.at<double>(k) = sum_omega_k_[k] /= size_omega_k_[k];

  double max = 2 * U_.at<double>(max_valid_) - U_.at<double>(max_valid_ - 1);
  for (int k = max_valid_ + 1; k < 256; ++k)
    U_.at<double>(k) = max;
}

void EngelCalibration::optimizeIrradiance() {
  // Eqn. 8
  sum_t2_i_.create(dataset_->getImageSize());
  sum_t2_i_.setTo(0);
  B_.setTo(0);

  for (const auto& t : dataset_->getExposureTimes())
    for (const auto& image : dataset_->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j) {
          const auto& p = image.at<uint8_t>(i, j);
          if (!isPixelValid(p))
            continue;
          B_.at<double>(i, j) += U_.at<double>(p) * t;
          sum_t2_i_(i, j) += t * t;
        }

  cv::divide(B_, sum_t2_i_, B_);
}

double EngelCalibration::computeEnergy() {
  long double energy = 0;
  long unsigned int num = 0;
  for (const auto& t : dataset_->getExposureTimes())
    for (const auto& image : dataset_->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j)
          if (isPixelValid(image.at<uint8_t>(i, j))) {
            long double r = U_.at<double>(image.at<uint8_t>(i, j)) - t * B_.at<double>(i, j);
            energy += r * r;
            num += 1;
          }
  // Scale the energy to account for all the rescalings that happened along the way
  return static_cast<double>(std::sqrt(energy / num) / scale_);
}

void EngelCalibration::rescale() {
  auto scale = 1.0 / U_.at<double>(128);
  cv::multiply(U_, scale, U_);
  cv::multiply(B_, scale, B_);
  // Remember the total scale relative to the initial energy
  scale_ *= scale;
}

void EngelCalibration::visualizeProgress() {
  if (imshow_) {
    cv::Mat response;
    U_.convertTo(response, CV_32F);
    imshow_(utils::plotRadiometricResponse(response, cv::Size(500, 500), utils::colors::BGR[channel_]));
  }
}
