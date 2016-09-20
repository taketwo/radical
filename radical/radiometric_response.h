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

#include <memory>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

namespace radical {

/** The RadiometricResponse class models camera response function (CRF) and
  * allows to map from pixel brightness to pixel irradiance and vice versa. */
class RadiometricResponse {
 public:
  using Ptr = std::shared_ptr<RadiometricResponse>;

  /** Construct RadiometricResponse from a cv::Mat with inverse CRF.
    * \param[in] response inverse CRF (CV_32FC3, 256 elements) */
  RadiometricResponse(cv::InputArray response);

  RadiometricResponse(const std::string& filename);

  virtual ~RadiometricResponse();

  /** Get inverse of the response function (a lookup table that maps pixel brightness to irradiance). */
  cv::Mat getInverseResponse() const;

  /** Write radiometric response to a file. */
  void save(const std::string& filename) const;

  /** Compute pixel brightness from pixel irradiance (direct mapping).
    * \param[in] E pixel irradiance
    * \returns pixel brightness */
  cv::Vec3b directMap(const cv::Vec3f& E) const;

  /** Compute image brightness from image irradiance (direct mapping).
    * \param[in] E image irradiance
    * \param[out] I image brightness */
  void directMap(cv::InputArray E, cv::OutputArray I) const;

  /** Compute pixel irradiance from pixel brightness (inverse mapping).
    * \param[in] I pixel brightness
    * \returns pixel irradiance */
  cv::Vec3f inverseMap(const cv::Vec3b& I) const;

  /** Compute image irradiance from image brightness (inverse mapping).
    * \param[in] I image brightness
    * \param[out] E image irradiance */
  void inverseMap(cv::InputArray I, cv::OutputArray E) const;

  /** Compute logarithm of pixel irradiance from pixel brightness (inverse mapping).
    * \param[in] I pixel brightness
    * \returns logarithm of pixel irradiance */
  cv::Vec3f inverseLogMap(const cv::Vec3b& I) const;

  /** Compute logarithm of image irradiance from image brightness (inverse mapping).
    * \param[in] I image brightness
    * \param[out] E logarithm of image irradiance */
  void inverseLogMap(cv::InputArray I, cv::OutputArray E) const;

 private:
  cv::Mat response_;
  cv::Mat log_response_;
  std::vector<cv::Mat> response_channels_;
};

}  // namespace radical
