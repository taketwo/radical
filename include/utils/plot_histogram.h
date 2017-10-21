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

#pragma once

#include <opencv2/core/core.hpp>

namespace utils {

/** Plot a one-dimensional histogram on a given canvas.
  * The histogram bar width is computed automatically such that it is as wide as possible while the histogram still
  * entirely fits on the canvas.
  * Single-channel and three-channel histograms are supported. In the latter case
  *  - per-channel histograms will be plotted one on top of another in reverse order;
  *  - \arg color will be ignored and the histograms will be plotted with Blue, Green, and Red colors respectively. */
void plotHistogram(const cv::Mat& histogram, cv::Mat& canvas, const cv::Scalar& color = {0, 0, 0, 0});

/** Plot a one-dimensional histogram.
  * Canvas is created automatically to fit the histogram.
  * \param[in] histogram
  * \param[in] bar_width width in pixels of each histogram bin bar
  * \param[in] height height of the histogram of each channel, thus total height of the output plot will be \arg height
  *            multiplied by the number of channels
  * \param[in] color color of the histogram bars, ignored in the case of multi-channel histogram */
cv::Mat plotHistogram(const cv::Mat& histogram, unsigned int bar_width, unsigned int height,
                      const cv::Scalar& color = {0, 0, 0, 0});
}
