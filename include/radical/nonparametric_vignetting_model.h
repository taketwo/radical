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

#include <radical/vignetting_model.h>

namespace radical {

/** Nonparametric (dense) model of vignetting response.
  *
  * Attenuation factor of every color channel at every image location is stored directly. */
class NonparametricVignettingModel : public VignettingModel {
 public:
  using Ptr = std::shared_ptr<NonparametricVignettingModel>;

  NonparametricVignettingModel(cv::InputArray coefficients);

  NonparametricVignettingModel(const std::string& filename);

  virtual std::string getName() const override;

  virtual void save(const std::string& filename) const override;

  virtual cv::Vec3f operator()(const cv::Vec2f& p) const override;

  using VignettingModel::operator();

  virtual cv::Size getImageSize() const override;

  virtual cv::Mat getModelCoefficients() const override;

 private:
  cv::Mat coefficients_;
};

}  // namespace radical
