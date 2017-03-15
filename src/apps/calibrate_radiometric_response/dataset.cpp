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
#include <map>

#if CV_MAJOR_VERSION >= 3
#include <opencv2/imgcodecs/imgcodecs.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#endif

#include <opencv2/core/core.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "utils/mat_io.h"

#include "dataset.h"

std::vector<cv::Mat> Dataset::getImages() const {
  std::vector<cv::Mat> images;
  std::transform(begin(), end(), std::back_inserter(images), [](const std::pair<cv::Mat, int>& p) { return p.first; });
  return images;
}

std::vector<int> Dataset::getExposures() const {
  std::vector<int> exposures;
  std::transform(begin(), end(), std::back_inserter(exposures),
                 [](const std::pair<cv::Mat, int>& p) { return p.second; });
  return exposures;
}

std::vector<Dataset> Dataset::splitChannels() const {
  int num_channels = at(0).first.channels();
  if (num_channels == 1)
    return std::vector<Dataset>(1, *this);
  std::vector<Dataset> splitted(num_channels);
  std::vector<cv::Mat> channels;
  for (size_t i = 0; i < size(); ++i) {
    cv::split(at(i).first, channels);
    for (int c = 0; c < num_channels; ++c)
      splitted[c].emplace_back(channels[c].clone(), at(i).second);
  }
  return splitted;
}

void Dataset::save(const std::string& path, Format format) const {
  namespace fs = boost::filesystem;
  fs::path dir(path);
  if (!fs::exists(dir))
    fs::create_directories(dir);
  boost::format fmt("%1$06d_%2$03d.%3$s");
  auto extension = format == PNG ? "png" : "mat";
  std::map<int, int> indices;
  for (const auto& item : *this) {
    if (indices.count(item.second) == 0)
      indices[item.second] = 0;
    auto filename = (dir / boost::str(fmt % item.second % indices[item.second] % extension)).native();
    if (format == PNG)
      cv::imwrite(filename, item.first);
    else
      utils::writeMat(filename, item.first);
    ++indices[item.second];
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
        if (extension == ".png")
          image = cv::imread(iter->path().string());
        else
          image = utils::readMat(iter->path().string());
        dataset->emplace_back(image, exposure);
      } catch (boost::bad_lexical_cast& e) {
      }
    }
    return dataset;
  } else {
    return nullptr;
  }
}
