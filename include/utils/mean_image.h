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

namespace utils {

/** Compute (weighted) running average and (optionally) variance for a stream of images. */
class MeanImage {
 public:
  /** Constructor.
    *
    * Until the first call to add() the object is in uninitialized state and all getter functions return empty matrices.
    * The accumulation is reset when the desired number of samples has been reached for every pixel.
    *
    * \param[in] compute_variance Enable/disable variance computation.
    * \param[in] num_samples required number of samples, 0 means no limits. */
  MeanImage(bool compute_variance, unsigned int num_samples);

  ~MeanImage();

  /** Accumulate one more image.
    *
    * Optional parameter \arg mask has the following meaning depending on its content:
    *
    *  - Empty    : Ignored.
    *               All pixels will be updated assuming unity weights.
    *  - CV_8UC1  : Per-pixel mask.
    *               Positive values indicate pixels that should be accumulated; zero values indicate pixels that should
    *               be skipped.
    *
    * Unless empty, \arg mask should have the same dimensions as images and have a single channel.
    *
    * \return flag indicating whether the reuired number of samples has been collected for every pixel. If unlimited
    * accumulation was selected at construction time, then this will always return false. */
  bool add(cv::InputArray image, cv::InputArray mask = cv::noArray());

  /** Accumulate one more image (with weights).
    *
    * Supports either single weight for the whole image (if \arg weights is a scalar), or per-pixel weights. In the
    * latter case \arg weights should be of type CV_64FC1 and have the same dimensions as image. If a weight is zero or
    * negative, then the corresponding pixel is not updated.
    *
    * \return flag indicating whether the reuired number of samples has been collected for every pixel. If unlimited
    * accumulation was selected at construction time, then this will always return false. */
  bool addWeighted(cv::InputArray image, cv::InputArray weights, cv::InputArray mask = cv::noArray());

  /** Get the current mean image.
    *
    * Accumulation happens in double-precision floating point numbers. The user has a choice either to get these numbers
    * as is, or to convert to the original type of the images.
    *
    * Note: returned matrix is only valid until the next add() or addWeighted() call, afterwards the memory it points to
    * will be reused. */
  cv::Mat getMean(bool as_original_type = true);

  /** Get the current variance of the mean image.
    *
    * Will be filled with zeros if variance computation is not enabled.
    *
    * Note: returned matrix is only valid until the next add() or addWeighted() call, afterwards the memory it points to
    * will be reused. */
  cv::Mat getVariance();

  /** Get the inverse of the current variance of the mean image.
    *
    * Will be filled with zeros if variance computation is not enabled.
    *
    * Note: returned matrix is only valid until the next add() or addWeighted() call, afterwards the memory it points to
    * will be reused. */
  cv::Mat getVarianceInverse();

  /** Get the number of accumulated samples per pixel.
    *
    * Note: returned matrix is only valid until the next add() or addWeighted() call, afterwards the memory it points to
    * will be reused.
    *
    * \param[in] normalize if set, the number of collected samples will be divided by the target number of samples,
    * producing a number between 0 and 1. Has no effect if unlitimed accumulation was selected at construction time. */
  cv::Mat getNumSamples(bool normalize = false);

 private:
  void reset(const cv::Size& size, int type);

  const bool compute_variance_;
  const unsigned int num_samples_;
  bool done_;
  int type_;
  cv::Size size_;
  cv::Mat M_;
  cv::Mat W_;
  cv::Mat S_;
  cv::Mat counter_;

  // Storage to avoid reallocations
  cv::Mat image_minus_M_;
  cv::Mat image_minus_M_new_;
  cv::Mat weights_32f_;
  cv::Mat mask_;
  cv::Mat get_mean_output_;
  cv::Mat get_num_samples_output_;
  cv::Mat get_variance_output_;
  cv::Mat get_inverse_variance_output_;
  cv::Mat zeros_mask_;
  cv::Mat nonzeros_mask_;
};
}
