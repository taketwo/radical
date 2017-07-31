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

#include <radical/check.h>

#include "utils/mean_image.h"

namespace utils {

using radical::Check;

MeanImage::MeanImage(bool compute_variance, unsigned int num_samples)
: compute_variance_(compute_variance), num_samples_(num_samples), done_(true), type_(-1), size_(0, 0) {}

MeanImage::~MeanImage() {}

bool MeanImage::add(cv::InputArray _image, cv::InputArray _mask) {
  return addWeighted(_image, 1, _mask);
}

bool MeanImage::addWeighted(cv::InputArray _image, cv::InputArray _weights, cv::InputArray _mask) {
  if (done_)
    reset(_image.size(), _image.type());
  else
    Check("Input image", _image).hasSize(size_).hasType(type_);

  cv::Mat mask;
  if (!_mask.empty()) {
    Check("Mask", _mask).hasSize(size_).hasType(CV_8UC1);
    mask = _mask.getMat();
  }

  bool unity = false;

  // Check if weights is a scalar
  auto weights = _weights.getMat();
  if (weights.dims <= 2 && weights.isContinuous() && weights.cols == 1 && weights.rows == 1) {
    auto w = weights.at<double>(0, 0);
    if (w == 0)
      return done_;
    if (w == 1)
      unity = true;
  } else {
    Check("Image", _image).hasChannels(1);  // per-pixel weights are supported only with single-channel images
    Check("Weights", _weights).hasSize(size_).hasType(CV_64FC1);
    weights.convertTo(weights_32f_, CV_32F);
    cv::threshold(weights_32f_, weights_32f_, 0, 255, cv::THRESH_BINARY);
    weights_32f_.convertTo(mask_, CV_8U);
    mask = mask.empty() ? mask_ : mask & mask_;
  }

  // Special case with unity weights
  // Do not need to take care of precision issues with first sample
  if (unity) {
    cv::subtract(_image, M_, image_minus_M_, mask, CV_64F);
    cv::add(W_, 1, W_, mask);
    cv::add(M_, image_minus_M_ / W_, M_, mask, CV_64F);
    if (compute_variance_) {
      cv::subtract(_image, M_, image_minus_M_new_, mask, CV_64F);
      cv::add(S_, image_minus_M_.mul(image_minus_M_new_), S_, mask);
    }
  } else {
    zeros_mask_ = (counter_ == 0);
    nonzeros_mask_ = (counter_ != 0);
    if (!mask.empty()) {
      zeros_mask_ &= mask;
      nonzeros_mask_ &= mask;
    }
    cv::subtract(_image, M_, image_minus_M_, mask, CV_64F);
    cv::add(W_, _weights, W_, mask);
    cv::multiply(image_minus_M_, _weights, image_minus_M_);
    cv::add(M_, image_minus_M_ / W_, M_, nonzeros_mask_, CV_64F);
    cv::add(M_, _image, M_, zeros_mask_, CV_64F);
    if (compute_variance_) {
      cv::subtract(_image, M_, image_minus_M_new_, nonzeros_mask_, CV_64F);
      cv::add(S_, image_minus_M_.mul(image_minus_M_new_), S_, nonzeros_mask_);
    }
  }

  // Need to explicitly specify depth for output array, otherwise fails with OpenCV 2.4
  cv::add(counter_, 1, counter_, mask, CV_32S);

  // If there is a prescribed number of samples, check if we fullfilled it
  if (num_samples_ > 0) {
    double min, max;
    cv::minMaxLoc(counter_, &min, &max);
    if (min >= num_samples_)
      done_ = true;
  }

  return done_;
}

cv::Mat MeanImage::getMean(bool as_original_type) {
  if (size_.area() == 0)
    return cv::Mat();
  if (as_original_type) {
    M_.convertTo(get_mean_output_, type_);
    return get_mean_output_;
  }
  return M_;
}

cv::Mat MeanImage::getVariance() {
  if (type_ == -1)
    return cv::Mat();
  if (!compute_variance_)
    return S_;
  cv::divide(S_, W_, get_variance_output_);
  return get_variance_output_;
}

cv::Mat MeanImage::getVarianceInverse() {
  if (type_ == -1)
    return cv::Mat();
  if (!compute_variance_)
    return S_;
  // Workaround for matrix element-wise division producing Infs instead of zeros
  // See: https://github.com/opencv/opencv/issues/8413#issuecomment-287475833
  cv::setUseOptimized(false);
  cv::divide(W_, S_, get_inverse_variance_output_);
  return get_inverse_variance_output_;
}

cv::Mat MeanImage::getNumSamples(bool normalize) {
  if (size_.area() == 0)
    return cv::Mat();
  if (normalize && num_samples_ != 0) {
    cv::divide(counter_, num_samples_, get_num_samples_output_, 1, CV_32F);
    return get_num_samples_output_;
  }
  return counter_;
}

void MeanImage::reset(const cv::Size& size, int type) {
  size_ = size;
  type_ = type;
  M_.create(size_, CV_MAKETYPE(CV_64F, CV_MAT_CN(type_)));
  M_.setTo(0);
  W_.create(size_, CV_MAKETYPE(CV_64F, CV_MAT_CN(type_)));
  W_.setTo(0);
  S_.create(size_, CV_MAKETYPE(CV_64F, CV_MAT_CN(type_)));
  S_.setTo(0);
  counter_.create(size_, CV_32SC1);
  counter_.setTo(0);
  done_ = false;
}
}
