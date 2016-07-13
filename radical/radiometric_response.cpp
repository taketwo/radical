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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/throw_exception.hpp>

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
  if (_response.size().width != 256 || _response.size().height != 1)
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Radiometric response should have 1 x 256 size")
                          << RadiometricResponseException::Size(_response.size()));
  if (_response.type() != CV_32FC3)
    BOOST_THROW_EXCEPTION(RadiometricResponseException("Radiometric response values should be 3-channel float")
                          << RadiometricResponseException::Type(_response.type()));
  response_ = _response.getMat();
  cv::log(response_, log_response_);
  cv::split(response_, response_channels_);
}

RadiometricResponse::RadiometricResponse(const std::string& filename, ChannelOrder order)
: RadiometricResponse(load(filename), order) {}

RadiometricResponse::~RadiometricResponse() {}

cv::Mat RadiometricResponse::load(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open())
    BOOST_THROW_EXCEPTION(SerializationException("Failed to open radiometric response file")
                          << SerializationException::Filename(filename));

  cv::Mat response(1, 256, CV_32FC3);
  for (size_t i = 0; i < 3; ++i) {
    auto v = response.begin<cv::Vec3f>();
    for (size_t j = 0; j < 256; ++j) {
      file >> (*v++)[order_ == ChannelOrder::BGR ? i : 2 - i];
      if (file.fail())
        BOOST_THROW_EXCEPTION(SerializationException("Radiometric response file contains invalid data")
                              << SerializationException::Filename(filename));
    }
  }
  file.close();

  return response;
}

cv::Vec3b RadiometricResponse::directMap(const cv::Vec3f& E) const {
  return inverseLUT(response_channels_, E);
}

}  // namespace radical
