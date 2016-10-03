/******************************************************************************
 * Copyright (c) 2016 Sergey Alexandrov
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

#include <boost/assert.hpp>

#include <opencv2/core/core.hpp>

class MeanImage {
 public:
  MeanImage(unsigned int num_images) : num_images_(num_images), counter_(0) {
    BOOST_ASSERT(num_images_);
  }

  /** Accumulate one more image.
    * \returns flag indicating whether the prescribed number of images have been accumulated and the mean image is ready
    * to be retrieved with get() function. */
  bool add(const cv::Mat& image) {
    if (sum_.empty()) {
      sum_.create(image.size(), CV_32FC3);
      sum_.setTo(0);
    } else {
      BOOST_ASSERT(sum_.size() == image.size());
    }
    cv::Mat image_float;
    image.convertTo(image_float, CV_32FC3);
    sum_ += image_float;
    if (++counter_ == num_images_) {
      sum_ /= (1.0f * num_images_);
      mean_.release();  // to make sure that we do not touch data of previously computed mean images
      sum_.convertTo(mean_, CV_8UC3);
      reset();
      return true;
    }
    return false;
  }

  /** Get mean image.
    * The returned image is only valid after add() has returned \c true. */
  cv::Mat get() {
    return mean_;
  }

  void reset() {
    sum_.release();
    counter_ = 0;
  }

 private:
  const unsigned int num_images_;
  unsigned int counter_;
  cv::Mat sum_;
  cv::Mat mean_;
};
