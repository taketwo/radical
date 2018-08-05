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

#include <opencv2/imgproc/imgproc.hpp>

#include <radical/check.h>

#include "utils/colors.h"
#include "utils/plot_histogram.h"

namespace utils {

using radical::Check;

void plotHistogram(const cv::Mat& histogram, cv::Mat& canvas, const cv::Scalar& color) {
  Check("Histogram", histogram).notEmpty().hasDepth(CV_32F);
  Check("Canvas", canvas).notEmpty().hasType(CV_8UC3);

  const auto N = histogram.total();
  const auto bar_width = canvas.cols / N;
  const auto bar_height = canvas.rows / histogram.channels();
  assert(bar_width > 0);
  assert(bar_height > 0);

  double max = 0;
  cv::minMaxLoc(histogram, nullptr, &max, nullptr, nullptr);

  std::vector<cv::Mat> histogram_channels;
  cv::split(histogram, histogram_channels);

  const cv::Scalar* COLORS = histogram.channels() > 1 ? utils::colors::BGR : &color;
  for (int c = 0; c < histogram.channels(); ++c) {
    cv::Mat roi(canvas, cv::Rect(0, (histogram.channels() - 1 - c) * bar_height, N * bar_width, bar_height));
    roi.setTo(255);
    for (unsigned int b = 0; b < N; b++) {
      auto value = histogram_channels[c].at<float>(b);
      auto height = std::round(value * bar_height / max);
      if (height > 0)
        cv::rectangle(roi, cv::Point(b * bar_width, bar_height - 1),
                      cv::Point((b + 1) * bar_width - 1, bar_height - 1 - height), COLORS[c], -1);
    }
  }
}

cv::Mat plotHistogram(const cv::Mat& histogram, unsigned int bar_width, unsigned int height, const cv::Scalar& color) {
  assert(bar_width > 0);
  assert(height > 0);

  cv::Mat canvas(height * histogram.channels(), bar_width * histogram.total(), CV_8UC3);
  canvas.setTo(0);

  plotHistogram(histogram, canvas, color);
  return canvas;
}
}  // namespace utils
