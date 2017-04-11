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

#include <iostream>

#include <boost/assert.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include "dataset_collection.h"

DatasetCollection::DatasetCollection(grabbers::Grabber::Ptr grabber, const Parameters& params)
: grabber_(grabber), params_(params), dataset_(new Dataset), mean_(false, params.num_average_frames) {
  BOOST_ASSERT(params_.exposure_min <= params_.exposure_max);
  BOOST_ASSERT(params_.exposure_factor > 1.0);
  exposure_ = params_.exposure_min;
  skip_frames_ = params_.exposure_control_lag;
  images_to_accumulate_ = params_.num_images;
  std::cout << "Starting data collection" << std::endl;
  std::cout << "Exposure range: " << params_.exposure_min << " → " << params_.exposure_max << " with factor "
            << params_.exposure_factor << std::endl;
  std::cout << "Exposure: " << exposure_ << std::flush;
}

bool DatasetCollection::addFrame(const cv::Mat& frame) {
  if (skip_frames_-- > 0)
    return false;

  if (!mean_.add(dilateOverexposedAreas(frame)))
    return false;

  dataset_->insert(exposure_, mean_.getMean().clone());

  if (--images_to_accumulate_ > 0)
    return false;

  exposure_ += std::ceil(exposure_ * (params_.exposure_factor - 1.0));
  skip_frames_ = params_.exposure_control_lag;
  images_to_accumulate_ = params_.num_images;
  grabber_->setExposure(exposure_);
  std::cout << " " << exposure_ << std::flush;

  if (exposure_ > params_.exposure_max) {
    std::cout << std::endl;
    return true;
  }
  return false;
}

Dataset::Ptr DatasetCollection::getDataset() const {
  return dataset_;
}

cv::Mat DatasetCollection::dilateOverexposedAreas(const cv::Mat& image) {
  static const cv::Mat morph = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(2, 2));
  cv::Mat dilated;
  cv::threshold(image, dilated, params_.valid_intensity_max, 255, cv::THRESH_BINARY);
  cv::dilate(dilated, dilated, morph);
  return cv::max(image, dilated);
}
