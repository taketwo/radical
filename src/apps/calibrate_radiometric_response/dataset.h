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

#include <memory>
#include <vector>

#include <opencv2/core/core.hpp>

class Dataset : public std::vector<std::pair<cv::Mat, int>> {
 public:
  using Ptr = std::shared_ptr<Dataset>;

  std::vector<cv::Mat> getImages() const;

  std::vector<int> getExposures() const;

  std::vector<Dataset> splitChannels() const;

  /** Dataset ormats supported by save/load functions. */
  enum Format {
    PNG,  ///< Images are stored as PNG files (compressed)
    MAT,  ///< Images are stored as binary MAT files (raw)
  };

  void save(const std::string& path, Format format) const;

  static Ptr load(const std::string& path);
};
