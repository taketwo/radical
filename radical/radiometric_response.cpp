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

#include <boost/throw_exception.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include <radical/mat_io.h>
#include <radical/exceptions.h>
#include <radical/radiometric_response.h>

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

RadiometricResponse::RadiometricResponse(cv::InputArray _response, ChannelOrder order) : order_(order) {
  if (_response.total() != 256)
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Radiometric response should have exactly 256 elements")
                          << RadiometricResponseException::Size(_response.size()));
  if (_response.type() != CV_32FC3)
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Radiometric response values should be 3-channel float")
                          << RadiometricResponseException::Type(_response.type()));
  if (order_ == ChannelOrder::RGB)
    cv::cvtColor(_response, response_, CV_BGR2RGB);
  else
    response_ = _response.getMat();
  cv::log(response_, log_response_);
  cv::split(response_, response_channels_);
}

RadiometricResponse::RadiometricResponse(const std::string& filename, ChannelOrder order)
: RadiometricResponse(readMat(filename), order) {}

RadiometricResponse::~RadiometricResponse() {}

void RadiometricResponse::save(const std::string& filename) const {
  cv::Mat response;
  if (order_ == ChannelOrder::RGB)
    cv::cvtColor(response_, response, CV_RGB2BGR);
  else
    response = response_;
  writeMat(filename, response);
}

cv::Vec3b RadiometricResponse::directMap(const cv::Vec3f& E) const {
  return inverseLUT(response_channels_, E);
}

void RadiometricResponse::directMap(cv::InputArray _E, cv::OutputArray _I) const {
  if (_E.empty()) {
    _I.clear();
    return;
  }
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
  if (_I.empty())
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Brightness image should not be empty"));
  if (_I.depth() != CV_8U && _I.depth() != CV_8S)
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Brightness image should have 8U or 8S depth"));
  cv::LUT(_I, response_, _E);
}

}  // namespace radical