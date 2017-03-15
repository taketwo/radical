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

#include <boost/assert.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include "utils/mean_image.h"

MeanImage::MeanImage(unsigned int num_images) : num_images_(num_images), done_(true) {
  BOOST_ASSERT(num_images_);
}

bool MeanImage::add(const cv::Mat& image, const cv::Mat& mask) {
  if (done_) {
    sum_.create(image.size());
    sum_.setTo(0);
    counter_.create(image.size());
    counter_.setTo(0);
    type_ = image.type();
    done_ = false;
  } else {
    BOOST_ASSERT(sum_.size() == image.size() && counter_.size() == image.size() && type_ == image.type());
  }
  cv::Mat image_float;
  image.convertTo(image_float, CV_32FC3);
  cv::add(sum_, image_float, sum_, mask);
  cv::add(counter_, 1, counter_, mask);
  // Check if we are done
  double min, max;
  cv::minMaxLoc(counter_, &min, &max);
  if (min >= num_images_)
    done_ = true;
  return done_;
}

cv::Mat MeanImage::getMean() {
  cv::Mat counter_no_zeros, mean;
  cv::max(counter_, cv::Scalar(1), counter_no_zeros);
  cv::cvtColor(counter_no_zeros, counter_no_zeros, CV_GRAY2BGR);  // duplicate channels so that division works
  cv::divide(sum_, counter_no_zeros, mean);
  mean.convertTo(mean, type_);
  return mean;
}

cv::Mat MeanImage::getNumSamples(bool normalize) {
  if (normalize)
    return counter_ / num_images_;
  return counter_;
}
