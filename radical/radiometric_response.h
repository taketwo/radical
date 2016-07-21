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

#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

namespace radical {

class RadiometricResponse {
 public:
  enum class ChannelOrder {
    BGR,
    RGB,
  };

  RadiometricResponse(cv::InputArray response, ChannelOrder order = ChannelOrder::BGR);

  RadiometricResponse(const std::string& filename, ChannelOrder order = ChannelOrder::BGR);

  virtual ~RadiometricResponse();

  /** Compute pixel brightness from pixel irradiance (direct mapping).
    * \param[in] E pixel irradiance
    * \param[out] I pixel brightness */
  cv::Vec3b directMap(const cv::Vec3f& E) const;

  /** Compute image brightness from image irradiance (direct mapping).
    * \param[in] E image irradiance
    * \param[out] I image brightness */
  void directMap(cv::InputArray E, cv::OutputArray I) const;

 private:

  cv::Mat response_;
  cv::Mat log_response_;
  std::vector<cv::Mat> response_channels_;
  ChannelOrder order_;
};

}  // namespace radical
