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

#include <vector>

#include <boost/assert.hpp>

#include <opencv2/imgproc/imgproc.hpp>

/** Arrange a collection of images into a grid and return as a single cv::Mat.
  * The input images may have different depth and number of channels, but should have equal size. */
cv::Mat arrangeImagesInGrid(const std::vector<cv::Mat>& images, cv::Size grid, int depth = -1, int channels = -1) {
  cv::Mat out;
  if (images.empty())
    return out;

  std::vector<cv::Mat> homogenized_images(images.size());

  // All images should have same size, depth, and number of channels
  cv::Size size = images.front().size();
  depth = (depth == -1 ? images.front().depth() : depth);
  channels = (channels == -1 ? images.front().channels() : channels);
  int type = CV_MAKETYPE(depth, channels);
  for (size_t i = 0; i < images.size(); ++i) {
    BOOST_ASSERT(images[i].size() == size);
    if (images[i].depth() != depth)
      images[i].convertTo(homogenized_images[i], depth);
    else
      homogenized_images[i] = images[i];
    if (homogenized_images[i].channels() == 3 && channels == 1)
      cv::cvtColor(homogenized_images[i], homogenized_images[i], CV_BGR2GRAY);
    else if (homogenized_images[i].channels() == 1 && channels == 3)
      cv::cvtColor(homogenized_images[i], homogenized_images[i], CV_GRAY2BGR);
    else if (homogenized_images[i].channels() != channels)
      BOOST_ASSERT_MSG(false, "Number of channels does not match and can not be corrected");
  }

  // Grid dimensions should be positive and no less than the number of images
  BOOST_ASSERT(grid.width > 0 && grid.height > 0);
  BOOST_ASSERT(static_cast<unsigned int>(grid.area()) >= homogenized_images.size());

  int width = grid.width * size.width, height = grid.height * size.height;
  out.create(height, width, type);
  out.setTo(0);
  for (int i = 0; i < static_cast<int>(homogenized_images.size()); ++i) {
    int y = i / grid.width;
    int x = i % grid.width;
    cv::Mat roi(out, cv::Rect(x * size.width, y * size.height, size.width, size.height));
    homogenized_images[i].copyTo(roi);
  }
  return out;
}
