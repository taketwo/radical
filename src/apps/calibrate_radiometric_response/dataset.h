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
#include <unordered_map>
#include <vector>

#include <opencv2/core/core.hpp>

class Dataset {
 public:
  using Ptr = std::shared_ptr<Dataset>;

  Dataset();

  /** Insert an image taken at a given exposure time in the dataset. */
  void insert(int exposure_time, const cv::Mat& image);

  /** Get the size of images in the dataset. */
  cv::Size getImageSize() const;

  /** Get the total number of images in the dataset. */
  size_t getNumImages() const;

  /** Get the number of images at a given exposure time in the dataset. */
  size_t getNumImages(int exposure_time) const;

  /** Get all images taken at a given exposure time. */
  std::vector<cv::Mat> getImages(int exposure_time) const;

  /** Get all exposure times present in the dataset, sorted in ascending order. */
  std::vector<int> getExposureTimes() const;

  /** Split a dataset with multi-channel images into multiple single-channel datasets. */
  std::vector<Dataset> splitChannels() const;

  /** Get the contents of the dataset as a flat vector of images and a flat vector of their corresponding exposure
    * times. This format is compatible with OpenCV built-in CRF calibration algorithms. */
  void asImageAndExposureTimeVectors(std::vector<cv::Mat>& images, std::vector<int>& exposure_times) const;

  /** Save the dataset to the disk. */
  void save(const std::string& path) const;

  /** Load a dataset from the disk. */
  static Ptr load(const std::string& path);

 private:
  size_t num_images_;
  cv::Size image_size_;
  std::unordered_map<int, std::vector<cv::Mat>> data_;
};
