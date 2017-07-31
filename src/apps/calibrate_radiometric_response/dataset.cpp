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

#include <algorithm>

#include <boost/assert.hpp>

#include <opencv2/core/core.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <radical/exceptions.h>
#include <radical/mat_io.h>

#include "dataset.h"

Dataset::Dataset() : num_images_(0), image_size_(0, 0) {}

void Dataset::insert(int exposure_time, const cv::Mat& image) {
  if (image_size_.area() == 0)
    image_size_ = image.size();
  else
    BOOST_ASSERT_MSG(image_size_ == image.size(), "Attempted to insert images of different size into same dataset");
  data_[exposure_time].push_back(image);
  ++num_images_;
}

cv::Size Dataset::getImageSize() const {
  return image_size_;
}

size_t Dataset::getNumImages() const {
  return num_images_;
}

size_t Dataset::getNumImages(int exposure_time) const {
  if (data_.count(exposure_time))
    return data_.at(exposure_time).size();
  return 0;
}

std::vector<cv::Mat> Dataset::getImages(int exposure_time) const {
  if (data_.count(exposure_time))
    return data_.at(exposure_time);
  return {};
}

std::vector<int> Dataset::getExposureTimes() const {
  std::vector<int> times;
  std::transform(data_.begin(), data_.end(), std::back_inserter(times),
                 [](const std::pair<int, std::vector<cv::Mat>>& p) { return p.first; });
  std::sort(times.begin(), times.end(), std::greater<int>());
  return times;
}

std::vector<Dataset> Dataset::splitChannels() const {
  BOOST_ASSERT_MSG(!data_.empty(), "Attempted to split empty dataset");
  int num_channels = data_.begin()->second.begin()->channels();
  if (num_channels == 1)
    return std::vector<Dataset>(1, *this);
  std::vector<Dataset> splitted(num_channels);
  std::vector<cv::Mat> channels;
  for (const auto& time_images : data_)
    for (const auto& image : time_images.second) {
      cv::split(image, channels);
      for (int c = 0; c < num_channels; ++c)
        splitted[c].insert(time_images.first, channels[c].clone());
    }
  return splitted;
}

void Dataset::asImageAndExposureTimeVectors(std::vector<cv::Mat>& images, std::vector<int>& exposure_times) const {
  images.clear();
  exposure_times.clear();
  for (const auto& time_images : data_) {
    for (const auto& image : time_images.second) {
      images.push_back(image);
      exposure_times.push_back(time_images.first);
    }
  }
}

void Dataset::save(const std::string& path) const {
  namespace fs = boost::filesystem;
  fs::path dir(path);
  if (!fs::exists(dir))
    fs::create_directories(dir);
  boost::format fmt("%1$06d_%2$03d.mat");
  for (const auto& time_images : data_)
    for (size_t i = 0; i < time_images.second.size(); ++i) {
      auto filename = (dir / boost::str(fmt % time_images.first % i)).string();
      radical::writeMat(filename, time_images.second[i]);
    }
}

Dataset::Ptr Dataset::load(const std::string& path) {
  namespace fs = boost::filesystem;
  fs::path dir(path);
  if (fs::exists(dir) && fs::is_directory(dir)) {
    auto dataset = std::make_shared<Dataset>();
    for (fs::directory_iterator iter = fs::directory_iterator(dir); iter != fs::directory_iterator(); ++iter) {
      auto stem = iter->path().stem().string();
      auto extension = iter->path().extension().string();
      try {
        auto exposure = boost::lexical_cast<int>(stem.substr(0, 6));
        cv::Mat image;
        image = radical::readMat(iter->path().string());
        dataset->insert(exposure, image);
      } catch (boost::bad_lexical_cast&) {
      } catch (radical::SerializationException&) {
      }
    }
    return dataset;
  } else {
    return nullptr;
  }
}
