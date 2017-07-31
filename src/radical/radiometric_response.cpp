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

#include <algorithm>

#include <opencv2/imgproc/imgproc.hpp>

#include <radical/radiometric_response.h>
#include <radical/check.h>
#include <radical/mat_io.h>

/** Helper function for inverse look-up in a table (cv::Vec3f → Vec3b). */
inline cv::Vec3b inverseLUT(const std::vector<cv::Mat>& lut, const cv::Vec3f& in) {
  cv::Vec3b out;
  for (int c = 0; c < 3; ++c) {
    auto begin = &lut[c].at<float>(0);
    auto p = std::lower_bound(begin, &lut[c].at<float>(255), in[c]);
    out[c] = std::distance(begin, p);
  }
  return out;
}

namespace radical {

RadiometricResponse::RadiometricResponse(cv::InputArray _response) {
  Check("Radiometric response", _response).hasSize(256).hasType(CV_32FC3);
  response_ = _response.getMat();
  cv::log(response_, log_response_);
  cv::split(response_, response_channels_);
}

RadiometricResponse::RadiometricResponse(const std::string& filename) : RadiometricResponse(readMat(filename)) {}

RadiometricResponse::~RadiometricResponse() {}

cv::Mat RadiometricResponse::getInverseResponse() const {
  return response_;
}

void RadiometricResponse::save(const std::string& filename) const {
  writeMat(filename, response_);
}

cv::Vec3b RadiometricResponse::directMap(const cv::Vec3f& E) const {
  return inverseLUT(response_channels_, E);
}

void RadiometricResponse::directMap(cv::InputArray _E, cv::OutputArray _I) const {
  if (_E.empty()) {
    _I.clear();
    return;
  }
  Check("Irradiance image", _E).hasType(CV_32FC3);
  auto E = _E.getMat();
  _I.create(_E.size(), CV_8UC3);
  auto I = _I.getMat();
#if CV_MAJOR_VERSION > 2
  E.forEach<cv::Vec3f>(
      [&I, this](cv::Vec3f& v, const int* p) { I.at<cv::Vec3b>(p[0], p[1]) = inverseLUT(response_channels_, v); });
#else
  for (int i = 0; i < E.rows; i++)
    for (int j = 0; j < E.cols; j++)
      I.at<cv::Vec3b>(i, j) = inverseLUT(response_channels_, E.at<cv::Vec3f>(i, j));
#endif
}

cv::Vec3f RadiometricResponse::inverseMap(const cv::Vec3b& _I) const {
  cv::Mat I(1, 1, CV_8UC3);
  I.at<cv::Vec3b>(0, 0) = _I;
  cv::Mat E;
  cv::LUT(I, response_, E);
  return E.at<cv::Vec3f>(0, 0);
}

void RadiometricResponse::inverseMap(cv::InputArray _I, cv::OutputArray _E) const {
  if (_I.empty()) {
    _E.clear();
    return;
  }
  Check("Brightness image", _I).hasType(CV_8UC3);
  cv::LUT(_I, response_, _E);
}

cv::Vec3f RadiometricResponse::inverseLogMap(const cv::Vec3b& _I) const {
  cv::Mat I(1, 1, CV_8UC3);
  I.at<cv::Vec3b>(0, 0) = _I;
  cv::Mat E;
  cv::LUT(I, log_response_, E);
  return E.at<cv::Vec3f>(0, 0);
}

void RadiometricResponse::inverseLogMap(cv::InputArray _I, cv::OutputArray _E) const {
  if (_I.empty()) {
    _E.clear();
    return;
  }
  Check("Brightness image", _I).hasType(CV_8UC3);
  cv::LUT(_I, log_response_, _E);
}

}  // namespace radical
