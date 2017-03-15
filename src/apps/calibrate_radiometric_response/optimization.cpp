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

#include <opencv2/imgproc/imgproc.hpp>

#include "optimization.h"

Optimization::Optimization(const Dataset* data, uint8_t min_valid_intensity, uint8_t max_valid_intensity)
: dataset_(data), min_valid_(min_valid_intensity), max_valid_(max_valid_intensity), converged_(false) {
  B_.create(data->getImageSize(), CV_64FC1);
  B_.setTo(0);
  for (const auto& t : data->getExposureTimes())
    for (const auto& image : data->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j)
          B_.at<double>(i, j) += static_cast<double>(image.at<uint8_t>(i, j)) / data->getNumImages() / 256;

  U_.create(1, 256, CV_64FC1);
  for (int i = 0; i < 256; ++i)
    U_.at<double>(i) = (1.0 / 255.0) * i;
}

bool Optimization::isValid(uint8_t intensity) {
  return intensity < min_valid_ || intensity > max_valid_;
}

void Optimization::optimizeInverseResponse() {
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

  // There is no useful data for 255, so force extrapolation
  size_omega_k_[255] = 0;

  for (int k = 1; k < 256; ++k) {
    U_.at<double>(k) = sum_omega_k_[k] /= size_omega_k_[k];
    if (!std::isfinite(U_.at<double>(k)) && k > 1)
      U_.at<double>(k) = 2 * U_.at<double>(k - 1) - U_.at<double>(k - 2);
  }
}

void Optimization::optimizeIrradiance() {
  // Eqn. 8
  sum_t2_i_.create(dataset_->getImageSize());
  sum_t2_i_.setTo(0);
  B_.setTo(0);

  for (const auto& t : dataset_->getExposureTimes())
    for (const auto& image : dataset_->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j) {
          const auto& p = image.at<uint8_t>(i, j);
          if (isValid(p))
            continue;
          B_.at<double>(i, j) += U_.at<double>(p) * t;
          sum_t2_i_(i, j) += t * t;
        }

  cv::divide(B_, sum_t2_i_, B_);
}

void Optimization::rescale() {
  double min, max;
  cv::minMaxLoc(U_, &min, &max);
  auto scale = 1.0 / max;
  cv::multiply(U_, scale, U_);
  cv::multiply(B_, scale, B_);
}

double Optimization::computeEnergy() {
  long double energy = 0;
  long unsigned int num = 0;
  for (const auto& t : dataset_->getExposureTimes())
    for (const auto& image : dataset_->getImages(t))
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j)
          if (!isValid(image.at<uint8_t>(i, j))) {
            long double r = U_.at<double>(image.at<uint8_t>(i, j)) - t * B_.at<double>(i, j);
            energy += r * r;
            num += 1;
          }
  return std::sqrt(energy / num);
}

bool Optimization::converged() const {
  return converged_;
}

void Optimization::converged(bool state) {
  converged_ = state;
}

cv::Mat Optimization::getOptimizedInverseResponse() const {
  return U_;
}

cv::Mat Optimization::getOptimizedIrradiance() const {
  return B_;
}
