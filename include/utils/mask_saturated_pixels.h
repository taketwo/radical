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

#include <opencv2/core/core.hpp>

/** Mask pixels where any of the color channels is under/over exposed.
  * If an existing mask of matching size is provided, it will be updated.
  * \param[i] mask_value value to assign to saturated pixels, if it is ≥ 1, then non-masked values will be zeros; if it
  *           is = 0, then non-masked values will be 255 */
void maskSaturatedPixels(cv::InputArray _image, cv::InputOutputArray _mask, uint8_t mask_value = 0, uint8_t min = 5,
                         uint8_t max = 250) {
  cv::Mat image, mask;
  CV_Assert(_image.type() == CV_8UC3);
  if (_mask.empty() || _mask.size() != _image.size() || _mask.type() != CV_8U) {
    _mask.create(_image.size(), CV_8U);
    mask = _mask.getMat();
    mask.setTo(mask_value == 0 ? 255 : 0);  // OpenCV 2 does not support setTo() on OutputArray
  } else {
    mask = _mask.getMat();
  }
  image = _image.getMat();

#if CV_MAJOR_VERSION > 2
  image.forEach<cv::Vec3b>([&mask, mask_value, min, max](const cv::Vec3b& v, const int* p) {
    for (int c = 0; c < 3; ++c)
      if (v[c] < min || v[c] > max) {
        mask.at<uint8_t>(p) = mask_value;
        break;
      }
  });
#else
  for (int i = 0; i < image.rows; i++)
    for (int j = 0; j < image.cols; j++) {
      const auto& v = image.at<cv::Vec3b>(i, j);
      for (size_t c = 0; c < 3; ++c)
        if (v[c] < min || v[c] > max) {
          mask.at<uint8_t>(i, j) = mask_value;
          break;
        }
    }
#endif
}
