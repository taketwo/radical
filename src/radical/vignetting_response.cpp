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

#include <unordered_map>

#include <opencv2/imgproc/imgproc.hpp>

#include <radical/exceptions.h>
#include <radical/vignetting_model.h>
#include <radical/vignetting_response.h>
#include <radical/check.h>

// Custom hasher to allow unordered map with cv::Size keys.
namespace std {
template <>
struct hash<cv::Size> {
  typedef cv::Size argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const& s) const {
    result_type const h1(std::hash<int>()(s.width));
    result_type const h2(std::hash<int>()(s.height));
    return h1 ^ (h2 << 1);
  }
};
}

namespace radical {

struct VignettingResponse::ResponseCache {
  std::unordered_map<cv::Size, cv::Mat> precomputed_responses_;
  std::unordered_map<cv::Size, cv::Mat> precomputed_log_responses_;
  const VignettingModel& model_;

  ResponseCache(const VignettingModel& model) : model_(model) {}

  cv::Mat get(const cv::Size& image_size) {
    if (precomputed_responses_.count(image_size) == 0) {
      auto x_scale = static_cast<float>(model_.getImageSize().width) / image_size.width;
      auto y_scale = static_cast<float>(model_.getImageSize().height) / image_size.height;
      if (std::abs(x_scale - y_scale) > std::numeric_limits<float>::epsilon())
        throw Exception("Unable to compute vignetting response on the given image size (different aspect ratio)");
      cv::Mat response(image_size, CV_32FC3);
#if CV_MAJOR_VERSION > 2
      response.forEach<cv::Vec3f>(
          [x_scale, y_scale, this](cv::Vec3f& v, const int* p) { v = model_(x_scale * p[1], y_scale * p[0]); });
#else
      for (int i = 0; i < response.rows; i++)
        for (int j = 0; j < response.cols; j++)
          response.at<cv::Vec3f>(i, j) = model_(x_scale * j, y_scale * i);
#endif
      precomputed_responses_.insert({image_size, response});
    }
    return precomputed_responses_[image_size];
  }

  cv::Mat getLog(const cv::Size& image_size) {
    if (precomputed_log_responses_.count(image_size) == 0) {
      cv::Mat log;
      cv::log(get(image_size), log);
      precomputed_log_responses_.insert({image_size, log});
    }
    return precomputed_log_responses_[image_size];
  }
};

VignettingResponse::VignettingResponse(const std::string& filename) {
  model_ = VignettingModel::load(filename);
  if (!model_)
    throw SerializationException("File does not contain any valid vignetting model", filename);

  response_cache_.reset(new ResponseCache(*model_));
}

VignettingResponse::~VignettingResponse() {}

VignettingModel::ConstPtr VignettingResponse::getModel() const {
  return model_;
}

cv::Mat VignettingResponse::getResponse() const {
  return response_cache_->get(model_->getImageSize());
}

cv::Mat VignettingResponse::getResponse(cv::Size image_size) const {
  return response_cache_->get(image_size);
}

cv::Mat VignettingResponse::getLogResponse() const {
  return response_cache_->getLog(model_->getImageSize());
}

cv::Mat VignettingResponse::getLogResponse(cv::Size image_size) const {
  return response_cache_->getLog(image_size);
}

void VignettingResponse::remove(cv::InputArray _E, cv::OutputArray _L) const {
  if (_E.empty()) {
    _L.clear();
    return;
  }
  Check("Irradiance image", _E).hasType(CV_32FC3);
  cv::divide(_E, getResponse(_E.size()), _L);
}

void VignettingResponse::removeLog(cv::InputArray _E, cv::OutputArray _L) const {
  if (_E.empty()) {
    _L.clear();
    return;
  }
  Check("Irradiance image", _E).hasType(CV_32FC3);
  cv::subtract(_E, getLogResponse(_E.size()), _L);
}

void VignettingResponse::add(cv::InputArray _L, cv::OutputArray _E) const {
  if (_L.empty()) {
    _E.clear();
    return;
  }
  Check("Radiance image", _L).hasType(CV_32FC3);
  cv::multiply(_L, getResponse(_L.size()), _E);
}

void VignettingResponse::addLog(cv::InputArray _L, cv::OutputArray _E) const {
  if (_L.empty()) {
    _E.clear();
    return;
  }
  Check("Radiance image", _L).hasType(CV_32FC3);
  cv::add(_L, getLogResponse(_L.size()), _E);
}

}  // namespace radical
