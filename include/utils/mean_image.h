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

#include <opencv2/core/core.hpp>

class MeanImage {
 public:
  MeanImage(unsigned int num_images);

  /** Accumulate one more image. */
  bool add(const cv::Mat& image, const cv::Mat& mask = cv::Mat());

  /** Get mean image. */
  cv::Mat getMean();

  /** Get the number of accumulated samples per pixel.
    * The returned matrix is only valid until add() has returned \c true. */
  cv::Mat getNumSamples(bool normalize = false);

 private:
  const unsigned int num_images_;
  bool done_;
  int type_;
  cv::Mat_<float> counter_;
  cv::Mat_<cv::Vec3f> sum_;
};
